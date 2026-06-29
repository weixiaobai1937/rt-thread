/**
 * @file fdcan.c
 * @brief ACM32P4 FDCAN驱动实现
 *
 * 实现了ACM32P4芯片FDCAN外设的完整驱动，支持CAN2.0B和CAN FD协议。
 *
 * 主要功能：
 * - 快速初始化（CAN2.0B / CAN FD）
 * - 位时间配置（标称/数据）
 * - 16组硬件过滤器管理
 * - PTB + STB双发送缓冲管理
 * - 数据收发（含CAN FD 64字节）
 * - 回环/静默/单次发送等高级模式
 * - TTCAN时间触发通信
 * - CiA 603时间戳
 * - 完整的中断和回调机制
 * - 错误诊断（错误计数/错误类型/仲裁丢失位置）
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#include "hardware/fdcan.h"
#include "hardware/clocks.h"
#include "device/acm32p4xx.h"
#include <assert.h>
#include <string.h>

//===========================================
// 内部宏定义
//===========================================

#define FDCAN_RBUF_OFFSET        0x00U   ///< 接收缓冲偏移
#define FDCAN_TBUF_OFFSET        0x50U   ///< 发送缓冲偏移
#define FDCAN_RBUF_SIZE          80U     ///< 接收缓冲大小（字节）
#define FDCAN_TBUF_SIZE          72U     ///< 发送缓冲大小（字节）
#define FDCAN_RESET_TIMEOUT      10000U  ///< 复位超时计数值
#define FDCAN_BIT_TIME_MIN_TQ    8U      ///< 最小时间份额
#define FDCAN_BIT_TIME_MAX_TQ    25U     ///< 最大时间份额
#define FDCAN_SAMPLE_POINT_PCT   80U     ///< 默认采样点80%

//===========================================
// 内部类型定义
//===========================================

/**
 * @brief FDCAN上下文（每个实例一份）
 */
typedef struct {
    fdcan_callback_t callback;     ///< 事件回调函数
    bool initialized;              ///< 是否已初始化
} fdcan_context_t;

//===========================================
// 内部状态变量
//===========================================

static fdcan_context_t fdcan_ctx[FDCAN_INSTANCE_COUNT] = {0};

static FDCAN_TypeDef *const fdcan_instances[FDCAN_INSTANCE_COUNT] = {
    FDCAN1,
    FDCAN2,
};

static const clock_periph_t fdcan_clock_periph[FDCAN_INSTANCE_COUNT] = {
    CLK_FDCAN1,
    CLK_FDCAN2,
};

//===========================================
// 内部辅助函数
//===========================================

/**
 * @brief 获取FDCAN寄存器指针
 */
static inline FDCAN_TypeDef *fdcan_get_regs(fdcan_instance_t inst)
{
    assert(inst < FDCAN_INSTANCE_COUNT);
    return fdcan_instances[inst];
}

/**
 * @brief 获取RBUF的字节指针（用于直接访问接收/发送缓冲区）
 */
static inline volatile uint8_t *fdcan_get_rbuf_ptr(fdcan_instance_t inst)
{
    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    return (volatile uint8_t *)regs + FDCAN_RBUF_OFFSET;
}

/**
 * @brief 获取TBUF的字节指针
 */
static inline volatile uint8_t *fdcan_get_tbuf_ptr(fdcan_instance_t inst)
{
    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    return (volatile uint8_t *)regs + FDCAN_TBUF_OFFSET;
}

/**
 * @brief 计算位时间配置（采样点约80%）
 */
static bool fdcan_calculate_bit_timing(uint32_t baud, uint32_t pclk_hz,
                                       fdcan_bit_timing_config_t *timing)
{
    if (baud == 0 || pclk_hz == 0 || timing == NULL) {
        return false;
    }

    // 寻找最优预分频和TQ数，使采样点接近80%
    uint32_t best_error = UINT32_MAX;
    uint8_t best_presc = 0;
    uint8_t best_seg1 = 0;
    uint8_t best_seg2 = 0;

    for (uint32_t presc = 0; presc <= 255; presc++) {
        uint32_t tq_freq = pclk_hz / (presc + 1);
        uint32_t total_tq = tq_freq / baud;

        if (total_tq < FDCAN_BIT_TIME_MIN_TQ || total_tq > FDCAN_BIT_TIME_MAX_TQ) {
            continue;
        }

        // 计算 seg1 + seg2 + 1 = total_tq，采样点 = (1 + seg1 + 1) / total_tq
        // seg1 + seg2 = total_tq - 1（减1为同步段）
        // seg2 ≈ total_tq * (1 - 0.80) = total_tq * 0.2
        uint32_t ideal_seg2 = (total_tq * (100 - FDCAN_SAMPLE_POINT_PCT)) / 100;
        if (ideal_seg2 == 0) ideal_seg2 = 1;
        if (ideal_seg2 > 127) ideal_seg2 = 127;

        for (int32_t seg2 = (int32_t)ideal_seg2; seg2 >= 1; seg2--) {
            int32_t seg1 = (int32_t)(total_tq - 1 - (uint32_t)seg2);
            if (seg1 < 1 || seg1 > 255) continue;

            uint32_t actual_baud = pclk_hz / ((presc + 1) * total_tq);
            uint32_t error = (baud > actual_baud) ? (baud - actual_baud) : (actual_baud - baud);

            if (error < best_error) {
                best_error = error;
                best_presc = (uint8_t)presc;
                best_seg1 = (uint8_t)seg1;
                best_seg2 = (uint8_t)seg2;
            }

            if (error == 0) break;
        }
    }

    if (best_error == UINT32_MAX) {
        return false;
    }

    timing->prescaler = best_presc;
    timing->seg1 = best_seg1;
    timing->seg2 = best_seg2;
    timing->sjw = (best_seg2 > 3) ? 3 : best_seg2;  // SJW不超过4 TQ且不超过seg2

    return true;
}

/**
 * @brief 将DLC编码转换为实际数据长度
 */
static uint8_t fdcan_dlc_to_length(uint8_t dlc)
{
    static const uint8_t dlc_table[16] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64
    };
    return (dlc < 16) ? dlc_table[dlc] : 0;
}

/**
 * @brief 将实际数据长度转换为DLC编码
 */
static uint8_t fdcan_length_to_dlc(uint8_t length)
{
    if (length <= 8) return length;
    if (length <= 12) return 9;
    if (length <= 16) return 10;
    if (length <= 20) return 11;
    if (length <= 24) return 12;
    if (length <= 32) return 13;
    if (length <= 48) return 14;
    return 15;  // up to 64
}

/**
 * @brief 写入RBUF/TBUF用到的控制/状态字
 */
static void fdcan_write_tbuf_frame(fdcan_instance_t inst, const fdcan_frame_t *frame)
{
    volatile uint8_t *tbuf = fdcan_get_tbuf_ptr(inst);
    bool is_extended = (frame->id_type == FDCAN_ID_EXTENDED);
    bool is_fd = (frame->frame_type != FDCAN_FRAME_CLASSIC);
    bool has_brs = (frame->frame_type == FDCAN_FRAME_FD_BRS);
    uint8_t dlc = frame->dlc;

    // 清空TBUF
    memset((void *)tbuf, 0, FDCAN_TBUF_SIZE);

    // ID字段 (字节0-3)
    if (is_extended) {
        tbuf[0] = (uint8_t)(frame->id & 0xFF);
        tbuf[1] = (uint8_t)((frame->id >> 8) & 0xFF);
        tbuf[2] = (uint8_t)((frame->id >> 16) & 0xFF);
        tbuf[3] = (uint8_t)(((frame->id >> 24) & 0x1F) | 0x00);  // bit7=TTSEN=0
    } else {
        tbuf[0] = (uint8_t)(frame->id & 0xFF);
        tbuf[1] = (uint8_t)((frame->id >> 8) & 0x07);
    }

    // 控制字段 (字节4)
    uint8_t ctrl = dlc & 0x0F;
    if (is_fd) {
        ctrl |= (1U << 2);  // FDF=1
        if (has_brs) {
            ctrl |= (1U << 3);  // BRS=1
        }
    }
    if (frame->kind == FDCAN_FRAME_REMOTE) {
        ctrl |= (1U << 1);  // RTR=1
    }
    if (is_extended) {
        ctrl |= (1U << 0);  // IDE=1
    }
    tbuf[4] = ctrl;

    // 数据字段 (字节8-71)
    uint8_t data_len = fdcan_dlc_to_length(dlc);
    for (uint8_t i = 0; i < data_len && i < FDCAN_MAX_DATA_LEN; i++) {
        tbuf[8 + i] = frame->data[i];
    }
}

/**
 * @brief 从RBUF解析接收到的帧
 */
static void fdcan_read_rbuf_frame(fdcan_instance_t inst, fdcan_frame_t *frame)
{
    volatile uint8_t *rbuf = fdcan_get_rbuf_ptr(inst);

    // 读取控制字段 (字节4)
    uint8_t ctrl = rbuf[4];
    bool is_extended = (ctrl & 0x01) != 0;
    bool is_remote = (ctrl & 0x02) != 0;
    bool is_fdf = (ctrl & 0x04) != 0;
    bool is_brs = (ctrl & 0x08) != 0;
    uint8_t dlc = ctrl >> 4;

    // 读取ID
    if (is_extended) {
        frame->id = rbuf[0] | ((uint32_t)rbuf[1] << 8) | ((uint32_t)rbuf[2] << 16)
                    | (((uint32_t)rbuf[3] & 0x1F) << 24);
        frame->id_type = FDCAN_ID_EXTENDED;
    } else {
        frame->id = rbuf[0] | (((uint32_t)rbuf[1] & 0x07) << 8);
        frame->id_type = FDCAN_ID_STANDARD;
    }

    // 解析ESI位 (字节3 bit7)
    frame->esi = (rbuf[3] & 0x80) != 0;

    // 帧类型
    if (is_fdf) {
        frame->frame_type = is_brs ? FDCAN_FRAME_FD_BRS : FDCAN_FRAME_FD_NO_BRS;
    } else {
        frame->frame_type = FDCAN_FRAME_CLASSIC;
    }

    // 帧种类
    frame->kind = is_remote ? FDCAN_FRAME_REMOTE : FDCAN_FRAME_DATA;

    // DLC和数据长度
    frame->dlc = dlc;
    frame->data_len = fdcan_dlc_to_length(dlc);
    uint8_t data_len = frame->data_len;

    // 读取错误码 (字节5 bits 7:5)
    frame->error_code = (rbuf[5] >> 5) & 0x07;

    // 读取周期时间 (字节6-7)
    frame->cycle_time = rbuf[6] | ((uint16_t)rbuf[7] << 8);

    // 读取数据 (字节8-71)
    for (uint8_t i = 0; i < data_len && i < FDCAN_MAX_DATA_LEN; i++) {
        frame->data[i] = rbuf[8 + i];
    }

    // 读取时间戳 (字节72-79)
    frame->timestamp = 0;
    for (uint8_t i = 0; i < 8; i++) {
        frame->timestamp |= ((uint64_t)rbuf[72 + i] << (i * 8));
    }
}

/**
 * @brief 通过STB槽发送帧（内部辅助）
 */
static bool fdcan_send_frame_internal(fdcan_instance_t inst, const fdcan_frame_t *frame,
                                      fdcan_tx_buffer_t buf)
{
    FDCAN_TypeDef *regs = fdcan_get_regs(inst);

    if (buf == FDCAN_TX_PTB) {
        // 检查PTB是否空闲
        if (regs->CR_f.TACTIVE == 0 && regs->IR_f.TPIF == 0) {
            // PTB忙，等待或返回失败
        }
        regs->CR_f.TBSEL = 0;  // 选择PTB
        fdcan_write_tbuf_frame(inst, frame);
        regs->CR_f.TPE = 1;  // 触发PTB发送
    } else {
        // 检查STB是否满
        if (regs->IR_f.TSFF) {
            return false;
        }
        regs->CR_f.TBSEL = 1;  // 选择STB
        fdcan_write_tbuf_frame(inst, frame);
        regs->CR_f.TSNEXT = 1;  // 确认写入
    }

    return true;
}

//===========================================
// 第1层：快速初始化 API
//===========================================

bool fdcan_init_default(fdcan_instance_t inst)
{
    return fdcan_init_can20(inst, 500000U, 110000000U);
}

bool fdcan_init_can20(fdcan_instance_t inst, uint32_t baud_rate, uint32_t pclk_hz)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    if (baud_rate == 0 || pclk_hz == 0) {
        return false;
    }

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);

    // 使能时钟
    clock_periph_enable(fdcan_clock_periph[inst]);

    // 复位FDCAN控制器（RESET=1才能配置位时间寄存器）
    regs->CR_f.RESET = 1;

    // 计算位时间
    fdcan_bit_timing_config_t timing;
    if (!fdcan_calculate_bit_timing(baud_rate, pclk_hz, &timing)) {
        return false;
    }

    // 配置标称位时间
    regs->SBTR_f.S_PRESC = timing.prescaler;
    regs->SBTR_f.S_SEG_1 = timing.seg1;
    regs->SBTR_f.S_SEG_2 = timing.seg2;
    regs->SBTR_f.S_SJW   = timing.sjw;

    // 禁用FD模式（CAN2.0B）
    regs->CR_f.FD_ISO = 0;

    // 正常模式（非回环、非静默）
    regs->CR_f.LBMI = 0;
    regs->CR_f.LBME = 0;
    regs->CR_f.LOM  = 0;

    // 使能过滤器0接收所有帧（初始配置）
    regs->ACFCR_f.ACFADR = 0;
    regs->ACFCR_f.SELMASK = 0;
    regs->ACODR_f.ACODE_AMASK = 0;
    regs->ACFCR_f.AE |= (1U << 0);
    regs->ACFMODE_f.ACF_NOMASK |= (1U << 0);  // 过滤器0不使用MASK

    // 使能所有中断
    regs->IR_f.RIE   = 1;
    regs->IR_f.ROIE  = 1;
    regs->IR_f.RFIE  = 1;
    regs->IR_f.RAFIE = 1;
    regs->IR_f.TPIE  = 1;
    regs->IR_f.TSIE  = 1;
    regs->IR_f.EIE   = 1;

    // 释放复位，启动CAN通信
    regs->CR_f.RESET = 0;

    // 标记已初始化
    fdcan_ctx[inst].initialized = true;

    return true;
}

bool fdcan_init_canfd(fdcan_instance_t inst, uint32_t nom_baud, uint32_t data_baud,
                      uint32_t pclk_hz, fdcan_fd_mode_t fd_mode)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    if (nom_baud == 0 || data_baud == 0 || pclk_hz == 0) {
        return false;
    }

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);

    // 使能时钟
    clock_periph_enable(fdcan_clock_periph[inst]);

    // 复位FDCAN控制器
    regs->CR_f.RESET = 1;

    // 配置FD协议模式
    regs->CR_f.FD_ISO = (fd_mode == FDCAN_FD_MODE_ISO) ? 1 : 0;

    // 计算标称位时间
    fdcan_bit_timing_config_t nom_timing;
    if (!fdcan_calculate_bit_timing(nom_baud, pclk_hz, &nom_timing)) {
        return false;
    }

    regs->SBTR_f.S_PRESC = nom_timing.prescaler;
    regs->SBTR_f.S_SEG_1 = nom_timing.seg1;
    regs->SBTR_f.S_SEG_2 = nom_timing.seg2;
    regs->SBTR_f.S_SJW   = nom_timing.sjw;

    // 计算数据位时间
    fdcan_bit_timing_config_t data_timing;
    if (data_baud == nom_baud) {
        // 数据波特率与标称相同
        data_timing = nom_timing;
    } else {
        if (!fdcan_calculate_bit_timing(data_baud, pclk_hz, &data_timing)) {
            return false;
        }
    }

    regs->FBTR_f.F_PRESC = data_timing.prescaler;
    regs->FBTR_f.F_SEG_1 = data_timing.seg1;
    regs->FBTR_f.F_SEG_2 = data_timing.seg2;
    regs->FBTR_f.F_SJW   = data_timing.sjw;

    // 高速模式使能发送延迟补偿（数据波特率 > 1Mbps）
    if (data_baud > 1000000U) {
        regs->TDC_f.TDCEN = 1;
        regs->TDC_f.SSPOFF = data_timing.seg1;  // SSP偏移 = seg1 TQ
    }

    // 正常模式
    regs->CR_f.LBMI = 0;
    regs->CR_f.LBME = 0;
    regs->CR_f.LOM  = 0;

    // 使能过滤器0接收所有帧
    regs->ACFCR_f.ACFADR = 0;
    regs->ACFCR_f.SELMASK = 0;
    regs->ACODR_f.ACODE_AMASK = 0;
    regs->ACFCR_f.AE |= (1U << 0);
    regs->ACFMODE_f.ACF_NOMASK |= (1U << 0);

    // 使能所有中断
    regs->IR_f.RIE   = 1;
    regs->IR_f.ROIE  = 1;
    regs->IR_f.RFIE  = 1;
    regs->IR_f.RAFIE = 1;
    regs->IR_f.TPIE  = 1;
    regs->IR_f.TSIE  = 1;
    regs->IR_f.EIE   = 1;

    // 释放复位
    regs->CR_f.RESET = 0;

    fdcan_ctx[inst].initialized = true;

    return true;
}

void fdcan_deinit(fdcan_instance_t inst)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);

    // 复位控制器
    regs->CR_f.RESET = 1;

    // 清空上下文
    fdcan_ctx[inst].callback = NULL;
    fdcan_ctx[inst].initialized = false;

    // 禁用时钟
    clock_periph_disable(fdcan_clock_periph[inst]);
}

//===========================================
// 第2层：基础配置 API
//===========================================

bool fdcan_config_nominal_bit_timing(fdcan_instance_t inst,
                                     const fdcan_bit_timing_config_t *config)
{
    assert(inst < FDCAN_INSTANCE_COUNT);
    assert(config != NULL);

    if (config == NULL) {
        return false;
    }

    // 参数检查
    if (config->seg1 < 1 || config->seg1 > 255) {
        return false;
    }
    if (config->seg2 > 127) {
        return false;
    }
    if (config->sjw > 127) {
        return false;
    }

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);

    // 仅在RESET=1时可配置
    if (regs->CR_f.RESET == 0) {
        return false;
    }

    regs->SBTR_f.S_PRESC = config->prescaler;
    regs->SBTR_f.S_SEG_1 = config->seg1;
    regs->SBTR_f.S_SEG_2 = config->seg2;
    regs->SBTR_f.S_SJW   = config->sjw;

    return true;
}

bool fdcan_config_data_bit_timing(fdcan_instance_t inst,
                                  const fdcan_bit_timing_config_t *config)
{
    assert(inst < FDCAN_INSTANCE_COUNT);
    assert(config != NULL);

    if (config == NULL) {
        return false;
    }

    if (config->seg1 < 1 || config->seg1 > 31) {
        return false;
    }
    if (config->seg2 > 15) {
        return false;
    }
    if (config->sjw > 15) {
        return false;
    }

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);

    if (regs->CR_f.RESET == 0) {
        return false;
    }

    regs->FBTR_f.F_PRESC = config->prescaler;
    regs->FBTR_f.F_SEG_1 = config->seg1;
    regs->FBTR_f.F_SEG_2 = config->seg2;
    regs->FBTR_f.F_SJW   = config->sjw;

    return true;
}

bool fdcan_config_filter(fdcan_instance_t inst, uint8_t filter_index,
                         const fdcan_filter_config_t *config)
{
    assert(inst < FDCAN_INSTANCE_COUNT);
    assert(config != NULL);

    if (config == NULL || filter_index >= FDCAN_FILTER_COUNT) {
        return false;
    }

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);

    if (regs->CR_f.RESET == 0) {
        return false;
    }

    // 选择过滤组地址并写入CODE寄存器
    regs->ACFCR_f.ACFADR = filter_index;
    regs->ACFCR_f.SELMASK = 0;  // 选择CODE寄存器
    regs->ACODR_f.ACODE_AMASK = config->code;

    // 写入MASK寄存器
    regs->ACFCR_f.SELMASK = 1;  // 选择MASK寄存器
    regs->ACODR_f.ACODE_AMASK = config->mask;

    // 配置过滤器宽度
    if (config->width == FDCAN_FILTER_16BIT) {
        regs->ACFMODE_f.ACF_W16 |= (1U << filter_index);
    } else {
        regs->ACFMODE_f.ACF_W16 &= ~(1U << filter_index);
    }

    // 配置MASK模式
    if (!config->mask_enabled) {
        regs->ACFMODE_f.ACF_NOMASK |= (1U << filter_index);
    } else {
        regs->ACFMODE_f.ACF_NOMASK &= ~(1U << filter_index);
    }

    // 使能/禁用过滤器
    if (config->enabled) {
        regs->ACFCR_f.AE |= (1U << filter_index);
    } else {
        regs->ACFCR_f.AE &= ~(1U << filter_index);
    }

    return true;
}

void fdcan_config_global_filter(fdcan_instance_t inst,
                                const fdcan_global_filter_config_t *config)
{
    assert(inst < FDCAN_INSTANCE_COUNT);
    assert(config != NULL);

    if (config == NULL) {
        return;
    }

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);

    // FIFO溢出模式
    regs->CR_f.ROM = (config->overflow_mode == FDCAN_RX_OVERFLOW_DISCARD_NEW) ? 1 : 0;

    // 接收所有帧（含错误帧）
    regs->CR_f.RBALL = config->accept_all ? 1 : 0;

    // 阈值
    regs->LIMIT_f.AFWL = config->limit.afwl & 0x0F;
    regs->LIMIT_f.EWL  = config->limit.ewl & 0x0F;
}

void fdcan_set_fd_mode(fdcan_instance_t inst, fdcan_fd_mode_t mode)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    regs->CR_f.FD_ISO = (mode == FDCAN_FD_MODE_ISO) ? 1 : 0;
}

void fdcan_config_auto_retransmit(fdcan_instance_t inst, fdcan_tx_buffer_t buf,
                                  bool enable)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);

    // enable=true 表示使能自动重发（即禁用Single Shot）
    if (buf == FDCAN_TX_PTB) {
        regs->CR_f.TPSS = enable ? 0 : 1;
    } else {
        regs->CR_f.TSSS = enable ? 0 : 1;
    }
}

//===========================================
// 第3层：高级功能 API
//===========================================

void fdcan_config_loopback(fdcan_instance_t inst, fdcan_loopback_mode_t mode)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);

    switch (mode) {
        case FDCAN_LOOPBACK_NONE:
            regs->CR_f.LBMI = 0;
            regs->CR_f.LBME = 0;
            break;
        case FDCAN_LOOPBACK_INTERNAL:
            regs->CR_f.LBMI = 1;
            regs->CR_f.LBME = 0;
            break;
        case FDCAN_LOOPBACK_EXTERNAL:
            regs->CR_f.LBMI = 0;
            regs->CR_f.LBME = 1;
            break;
        default:
            break;
    }
}

void fdcan_config_silent_mode(fdcan_instance_t inst, bool enable)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    regs->CR_f.LOM = enable ? 1 : 0;
}

void fdcan_config_self_ack(fdcan_instance_t inst, bool enable)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    regs->CR_f.SACK = enable ? 1 : 0;
}

void fdcan_config_tx_delay_compensation(fdcan_instance_t inst, bool enable,
                                        uint8_t ssp_offset)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    regs->TDC_f.TDCEN  = enable ? 1 : 0;
    regs->TDC_f.SSPOFF = ssp_offset & 0x7F;
}

void fdcan_config_timestamp(fdcan_instance_t inst,
                            const fdcan_timestamp_config_t *config)
{
    assert(inst < FDCAN_INSTANCE_COUNT);
    assert(config != NULL);

    if (config == NULL) {
        return;
    }

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    regs->TIMCFG_f.TIMEEN  = config->enabled ? 1 : 0;
    regs->TIMCFG_f.TIMEPOS = (config->position == FDCAN_TIMESTAMP_EOF) ? 1 : 0;
}

uint16_t fdcan_get_cycle_time(fdcan_instance_t inst)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    // 从接收缓冲的CYCLE_TIME字段读取（此处返回0表示需自行从RBUF读取）
    // TTCAN的16位计数器值通过RBUF+6/7返回
    volatile uint8_t *rbuf = fdcan_get_rbuf_ptr(inst);
    return rbuf[6] | ((uint16_t)rbuf[7] << 8);
}

void fdcan_config_ttcan(fdcan_instance_t inst, const fdcan_ttcan_config_t *config)
{
    assert(inst < FDCAN_INSTANCE_COUNT);
    assert(config != NULL);

    if (config == NULL) {
        return;
    }

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);

    // TTCAN使能
    regs->TTCFG_f.TTEN = config->ttcan_enabled ? 1 : 0;

    // TTCAN发送缓冲模式
    regs->CR_f.TTTBM = config->ttcan_tbm_mode ? 1 : 0;

    // 时间预分频
    regs->TTCFG_f.T_PRESC = config->time_prescaler & 0x03;

    // 参考消息
    regs->REFMSG_f.REF_ID  = config->ref_id & 0x1FFFFFFF;
    regs->REFMSG_f.REF_IDE = config->ref_ide ? 1 : 0;
}

void fdcan_config_ttcan_trigger(fdcan_instance_t inst,
                                const fdcan_ttcan_trigger_config_t *config)
{
    assert(inst < FDCAN_INSTANCE_COUNT);
    assert(config != NULL);

    if (config == NULL) {
        return;
    }

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);

    // 触发类型和目标槽
    regs->TRIGCFG_f.TTYPE = (uint32_t)config->type & 0x07;
    regs->TRIGCFG_f.TTPTR = config->slot & 0x3F;

    // 触发时间
    regs->TTTRIG_f.TT_TRIG = config->trigger_time;

    // 发送使能窗口
    regs->TRIGCFG_f.TEW = config->tx_enable_window & 0x0F;

    // 使能时间触发中断
    regs->TTCFG_f.TTIE = 1;
}

void fdcan_config_ttcan_watchdog(fdcan_instance_t inst, uint16_t trigger_time)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);

    regs->WTRIG_f.TT_WTRIG = trigger_time;

    if (trigger_time > 0) {
        regs->TTCFG_f.WTIE = 1;  // 使能看门狗触发中断
    } else {
        regs->TTCFG_f.WTIE = 0;
    }
}

void fdcan_set_ttbuf_slot_status(fdcan_instance_t inst, uint8_t slot, bool full)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);

    regs->TTCFG_f.TBPTR = slot & 0x3F;
    if (full) {
        regs->TTCFG_f.TBF = 1;
    } else {
        regs->TTCFG_f.TBE = 1;
    }
}

void fdcan_set_stb_mode(fdcan_instance_t inst, fdcan_stb_mode_t mode)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    regs->CR_f.TSMODE = (mode == FDCAN_STB_MODE_PRIORITY) ? 1 : 0;
}

//===========================================
// 第4层：控制与查询 API
//===========================================

void fdcan_reset(fdcan_instance_t inst)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    regs->CR_f.RESET = 1;
}

void fdcan_start(fdcan_instance_t inst)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    regs->CR_f.RESET = 0;
}

bool fdcan_send_ptb(fdcan_instance_t inst, const fdcan_frame_t *frame)
{
    assert(inst < FDCAN_INSTANCE_COUNT);
    assert(frame != NULL);

    if (frame == NULL) {
        return false;
    }

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);

    // 检查PTB是否忙
    if (regs->CR_f.TACTIVE == 0 && regs->IR_f.TPIF == 0 && regs->CR_f.TPE == 1) {
        return false;
    }

    // 清除旧标志
    regs->IR_f.TPIF = 1;

    return fdcan_send_frame_internal(inst, frame, FDCAN_TX_PTB);
}

bool fdcan_send_stb(fdcan_instance_t inst, const fdcan_frame_t *frame)
{
    assert(inst < FDCAN_INSTANCE_COUNT);
    assert(frame != NULL);

    if (frame == NULL) {
        return false;
    }

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);

    // 检查STB是否满
    if (regs->IR_f.TSFF) {
        return false;
    }

    return fdcan_send_frame_internal(inst, frame, FDCAN_TX_STB);
}

void fdcan_transmit_stb_one(fdcan_instance_t inst)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    regs->CR_f.TSONE = 1;
}

void fdcan_transmit_stb_all(fdcan_instance_t inst)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    regs->CR_f.TSALL = 1;
}

void fdcan_abort_ptb(fdcan_instance_t inst)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    regs->CR_f.TPA = 1;
}

void fdcan_abort_stb(fdcan_instance_t inst)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    regs->CR_f.TSA = 1;
}

bool fdcan_receive(fdcan_instance_t inst, fdcan_frame_t *frame)
{
    assert(inst < FDCAN_INSTANCE_COUNT);
    assert(frame != NULL);

    if (frame == NULL) {
        return false;
    }

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);

    // 检查接收FIFO状态
    if (regs->CR_f.RSTAT == 0) {
        return false;
    }

    // 读取帧数据
    fdcan_read_rbuf_frame(inst, frame);

    return true;
}

bool fdcan_is_frame_available(fdcan_instance_t inst)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    return (regs->CR_f.RSTAT != 0);
}

void fdcan_release_rx_buffer(fdcan_instance_t inst)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    regs->CR_f.RREL = 1;
}

void fdcan_set_standby(fdcan_instance_t inst, bool enable)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    regs->CR_f.STBY = enable ? 1 : 0;
}

bool fdcan_is_bus_off(fdcan_instance_t inst)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    return (regs->CR_f.BUSOFF != 0);
}

void fdcan_get_error_counters(fdcan_instance_t inst, fdcan_error_counters_t *counters)
{
    assert(inst < FDCAN_INSTANCE_COUNT);
    assert(counters != NULL);

    if (counters == NULL) {
        return;
    }

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    counters->tecnt = (uint8_t)regs->ECC_f.TECNT;
    counters->recnt = (uint8_t)regs->ECC_f.RECNT;
}

fdcan_error_type_t fdcan_get_error_type(fdcan_instance_t inst)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    return (fdcan_error_type_t)regs->ECC_f.KOER;
}

uint8_t fdcan_get_arbitration_lost_pos(fdcan_instance_t inst)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    return (uint8_t)regs->ECC_f.ALC;
}

fdcan_rx_status_t fdcan_get_rx_status(fdcan_instance_t inst)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    return (fdcan_rx_status_t)regs->CR_f.RSTAT;
}

uint8_t fdcan_get_rx_count(fdcan_instance_t inst)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    uint32_t status = regs->CR_f.RSTAT;

    // RSTAT编码了FIFO状态，具体帧数需结合标志位判断
    switch (status) {
        case 0: return 0;   // 空
        case 3: return 16;  // 满
        default: {
            // AFWL阈值和RSTAT结合估算
            uint8_t afwl = (uint8_t)regs->LIMIT_f.AFWL;
            if (status == 1) return afwl / 2;   // 低于AFWL
            return afwl + (16 - afwl) / 2;       // 介于AFWL和满之间
        }
    }
}

uint8_t fdcan_get_stb_count(fdcan_instance_t inst)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    return (uint8_t)regs->CR_f.TSSTAT;
}

bool fdcan_is_rx_overflow(fdcan_instance_t inst)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    return (regs->CR_f.ROV != 0);
}

bool fdcan_is_stb_full(fdcan_instance_t inst)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    return (regs->IR_f.TSFF != 0);
}

//===========================================
// 中断管理 API
//===========================================

void fdcan_irq_enable(fdcan_instance_t inst, uint32_t irqs)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);

    if (irqs & FDCAN_IRQ_RX)              regs->IR_f.RIE   = 1;
    if (irqs & FDCAN_IRQ_RX_OVERFLOW)     regs->IR_f.ROIE  = 1;
    if (irqs & FDCAN_IRQ_RX_FULL)         regs->IR_f.RFIE  = 1;
    if (irqs & FDCAN_IRQ_RX_ALMOST_FULL)  regs->IR_f.RAFIE = 1;
    if (irqs & FDCAN_IRQ_TX_PTB)          regs->IR_f.TPIE  = 1;
    if (irqs & FDCAN_IRQ_TX_STB)          regs->IR_f.TSIE  = 1;
    if (irqs & FDCAN_IRQ_ERROR)           regs->IR_f.EIE   = 1;
    if (irqs & FDCAN_IRQ_ARBITRATION_LOST) regs->IR_f.ALIE = 1;
    if (irqs & FDCAN_IRQ_BUS_ERROR)       regs->IR_f.BEIE  = 1;
    if (irqs & FDCAN_IRQ_ERROR_PASSIVE)    regs->IR_f.EPIE = 1;
}

void fdcan_irq_disable(fdcan_instance_t inst, uint32_t irqs)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);

    if (irqs & FDCAN_IRQ_RX)              regs->IR_f.RIE   = 0;
    if (irqs & FDCAN_IRQ_RX_OVERFLOW)     regs->IR_f.ROIE  = 0;
    if (irqs & FDCAN_IRQ_RX_FULL)         regs->IR_f.RFIE  = 0;
    if (irqs & FDCAN_IRQ_RX_ALMOST_FULL)  regs->IR_f.RAFIE = 0;
    if (irqs & FDCAN_IRQ_TX_PTB)          regs->IR_f.TPIE  = 0;
    if (irqs & FDCAN_IRQ_TX_STB)          regs->IR_f.TSIE  = 0;
    if (irqs & FDCAN_IRQ_ERROR)           regs->IR_f.EIE   = 0;
    if (irqs & FDCAN_IRQ_ARBITRATION_LOST) regs->IR_f.ALIE = 0;
    if (irqs & FDCAN_IRQ_BUS_ERROR)       regs->IR_f.BEIE  = 0;
    if (irqs & FDCAN_IRQ_ERROR_PASSIVE)    regs->IR_f.EPIE = 0;
}

uint32_t fdcan_get_irq_status(fdcan_instance_t inst)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);

    uint32_t flags = 0;

    if (regs->IR_f.RIF)    flags |= FDCAN_FLAG_RX;
    if (regs->IR_f.ROIF)   flags |= FDCAN_FLAG_RX_OVERFLOW;
    if (regs->IR_f.RFIF)   flags |= FDCAN_FLAG_RX_FULL;
    if (regs->IR_f.RAFIF)  flags |= FDCAN_FLAG_RX_ALMOST_FULL;
    if (regs->IR_f.TPIF)   flags |= FDCAN_FLAG_TX_PTB;
    if (regs->IR_f.TSIF)   flags |= FDCAN_FLAG_TX_STB;
    if (regs->IR_f.EIF)    flags |= FDCAN_FLAG_ERROR;
    if (regs->IR_f.AIF)    flags |= FDCAN_FLAG_ABORT;
    if (regs->IR_f.BEIF)   flags |= FDCAN_FLAG_BUS_ERROR;
    if (regs->IR_f.ALIF)   flags |= FDCAN_FLAG_ARBITRATION_LOST;
    if (regs->IR_f.EPIF)   flags |= FDCAN_FLAG_ERROR_PASSIVE;
    if (regs->IR_f.EWARN)  flags |= FDCAN_FLAG_ERROR_WARNING;
    if (regs->IR_f.TSFF)   flags |= FDCAN_FLAG_STB_FULL;

    return flags;
}

void fdcan_clear_irq_flags(fdcan_instance_t inst, uint32_t flags)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);

    // 对中断标志位写1清零
    if (flags & FDCAN_FLAG_RX)              regs->IR_f.RIF   = 1;
    if (flags & FDCAN_FLAG_RX_OVERFLOW)     regs->IR_f.ROIF  = 1;
    if (flags & FDCAN_FLAG_RX_FULL)         regs->IR_f.RFIF  = 1;
    if (flags & FDCAN_FLAG_RX_ALMOST_FULL)  regs->IR_f.RAFIF = 1;
    if (flags & FDCAN_FLAG_TX_PTB)          regs->IR_f.TPIF  = 1;
    if (flags & FDCAN_FLAG_TX_STB)          regs->IR_f.TSIF  = 1;
    if (flags & FDCAN_FLAG_ERROR)           regs->IR_f.EIF   = 1;
    if (flags & FDCAN_FLAG_ABORT)           regs->IR_f.AIF   = 1;
    if (flags & FDCAN_FLAG_BUS_ERROR)       regs->IR_f.BEIF  = 1;
    if (flags & FDCAN_FLAG_ARBITRATION_LOST) regs->IR_f.ALIF = 1;
    if (flags & FDCAN_FLAG_ERROR_PASSIVE)    regs->IR_f.EPIF = 1;
}

fdcan_error_state_t fdcan_get_error_state(fdcan_instance_t inst)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);

    if (regs->CR_f.BUSOFF) {
        return FDCAN_ERROR_BUS_OFF;
    }

    if (regs->IR_f.EPASS) {
        return FDCAN_ERROR_PASSIVE;
    }

    return FDCAN_ERROR_ACTIVE;
}

//===========================================
// 回调管理 API
//===========================================

void fdcan_register_callback(fdcan_instance_t inst, fdcan_callback_t callback)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    fdcan_ctx[inst].callback = callback;
}

//===========================================
// 查询 API
//===========================================

volatile void *fdcan_get_base(fdcan_instance_t inst)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    return (volatile void *)fdcan_instances[inst];
}

bool fdcan_is_transmitting(fdcan_instance_t inst)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    return (regs->CR_f.TACTIVE == 0);
}

bool fdcan_is_receiving(fdcan_instance_t inst)
{
    assert(inst < FDCAN_INSTANCE_COUNT);

    FDCAN_TypeDef *regs = fdcan_get_regs(inst);
    return (regs->CR_f.RACTIVE == 0);
}

//===========================================
// 中断处理函数
//===========================================

/**
 * @brief FDCAN1中断处理函数
 *
 * 所有FDCAN事件（接收、发送、错误等）共用此中断线。
 * 通过查询IR寄存器区分事件类型，调用注册的回调函数。
 */
void FDCAN1_IRQHandler(void)
{
    uint32_t flags = fdcan_get_irq_status(FDCAN_INSTANCE_1);

    if (fdcan_ctx[FDCAN_INSTANCE_1].callback != NULL) {
        fdcan_ctx[FDCAN_INSTANCE_1].callback(FDCAN_INSTANCE_1, flags);
    }

    // 清除已处理的中断标志
    fdcan_clear_irq_flags(FDCAN_INSTANCE_1, flags);
}

/**
 * @brief FDCAN2中断处理函数
 */
void FDCAN2_IRQHandler(void)
{
    uint32_t flags = fdcan_get_irq_status(FDCAN_INSTANCE_2);

    if (fdcan_ctx[FDCAN_INSTANCE_2].callback != NULL) {
        fdcan_ctx[FDCAN_INSTANCE_2].callback(FDCAN_INSTANCE_2, flags);
    }

    fdcan_clear_irq_flags(FDCAN_INSTANCE_2, flags);
}
