/**
 * @file i2s.c
 * @brief ACM32P4 I2S 驱动实现 — 片上音频接口
 * @author ACM32P4 SDK Team
 * @version 1.0
 *
 * @details
 * 支持 I2S1/I2S2 两个实例，4 种音频标准（飞利浦/MSB对齐/LSB对齐/PCM），
 * 主/从模式、全双工、DMA、中断。时钟源自 PCLK1。
 */

#include "hardware/i2s.h"
#include "hardware/clocks.h"
#include "device/acm32p4xx.h"
#include <assert.h>

//===========================================
// 内部宏定义
//===========================================

#define I2S_CR_EN_MSK            (1U << 11)
#define I2S_CR_TEN_MSK           (1U << 9)
#define I2S_CR_REN_MSK           (1U << 10)
#define I2S_CR_MODE_MSK          (1U << 8)
#define I2S_CR_STD_POS           4U
#define I2S_CR_STD_MSK           (0x3U << I2S_CR_STD_POS)
#define I2S_CR_DLEN_POS          1U
#define I2S_CR_DLEN_MSK          (0x3U << I2S_CR_DLEN_POS)
#define I2S_CR_CKPL_MSK          (1U << 3)
#define I2S_CR_CHLEN_MSK         (1U << 0)
#define I2S_CR_PCMMODE_MSK       (1U << 7)
#define I2S_CR_IOSWP_MSK         (1U << 12)
#define I2S_CR_START_MSK         (1U << 13)
#define I2S_CR_STOP_MSK          (1U << 14)
#define I2S_CR_TXDMAEN_MSK       (1U << 16)
#define I2S_CR_RXDMAEN_MSK       (1U << 15)

#define I2S_SR_RXNE_MSK          (1U << 0)
#define I2S_SR_TXE_MSK           (1U << 1)
#define I2S_SR_CH_MSK            (1U << 2)
#define I2S_SR_UDR_MSK           (1U << 3)
#define I2S_SR_OVR_MSK           (1U << 4)
#define I2S_SR_FE_MSK            (1U << 5)
#define I2S_SR_MSUSP_MSK         (1U << 9)
#define I2S_SR_SVTC_MSK          (1U << 10)

#define I2S_TIMEOUT_COUNT         1000000U
#define I2S_PR_DIV_MAX            511U
#define I2S_PR_DIV_MIN            1U

//===========================================
// 内部状态变量
//===========================================

static I2S_TypeDef *const i2s_instances[] = {
    I2S1,  // I2S1
    I2S2,  // I2S2
};

static i2s_callback_t i2s_callbacks[I2S_COUNT] = { NULL, NULL };

//===========================================
// 内部辅助函数
//===========================================

/**
 * @brief 获取 I2S 实例寄存器基地址
 */
static inline I2S_TypeDef *get_i2s_regs(i2s_instance_t i2s)
{
    if (i2s >= I2S_COUNT) {
        return NULL;
    }
    return i2s_instances[i2s];
}

/**
 * @brief 根据采样率和配置计算 PR 寄存器分频值
 *
 * 根据用户手册表 28-2 计算 DIV 和 OF：
 * - MCKOE=0, CHLEN=0: PCLK / (32 * (DIV*2+OF))
 * - MCKOE=0, CHLEN=1: PCLK / (64 * (DIV*2+OF))
 * - MCKOE=1, CHLEN=0: PCLK / (256 * (DIV*2+OF))
 * - MCKOE=1, CHLEN=1: PCLK / (256 * (DIV*2+OF))
 *
 * @param[in]  sample_rate 目标采样率（Hz）
 * @param[in]  chlen_32    通道长度是否为 32 位
 * @param[in]  mckoe       是否输出主时钟
 * @param[out] div         计算出的 DIV 值
 * @param[out] of          计算出的 OF 值
 * @return true: 计算成功, false: 无法达到目标采样率
 */
static bool i2s_calculate_clock(uint32_t sample_rate, bool chlen_32, bool mckoe,
                                uint16_t *div, bool *of)
{
    assert(sample_rate > 0);

    uint32_t pclk_hz = clock_get_pclk1_hz();
    uint32_t multiplier;

    if (mckoe) {
        // MCKOE=1: 分母 = 256 * (DIV*2+OF)
        multiplier = 256U;
    } else if (chlen_32) {
        // MCKOE=0, CHLEN=1: 分母 = 64 * (DIV*2+OF)
        multiplier = 64U;
    } else {
        // MCKOE=0, CHLEN=0: 分母 = 32 * (DIV*2+OF)
        multiplier = 32U;
    }

    // 理想分频值 = PCLK / (multiplier * sample_rate)
    uint32_t ideal = pclk_hz / (multiplier * sample_rate);

    // 搜索最接近的 DIV+OF 组合
    for (uint32_t d = I2S_PR_DIV_MIN; d <= I2S_PR_DIV_MAX; d++) {
        uint32_t base = d * 2U;
        uint32_t val_even = pclk_hz / (multiplier * base);
        uint32_t val_odd  = pclk_hz / (multiplier * (base + 1U));

        int32_t diff_even = (int32_t)sample_rate - (int32_t)val_even;
        int32_t diff_odd  = (int32_t)sample_rate - (int32_t)val_odd;

        if (diff_even < 0) diff_even = -diff_even;
        if (diff_odd < 0)  diff_odd  = -diff_odd;

        if (diff_even <= diff_odd) {
            if (diff_even <= (int32_t)(sample_rate / 50U)) {  // 2% 容差
                *div = (uint16_t)d;
                *of  = false;
                return true;
            }
        } else {
            if (diff_odd <= (int32_t)(sample_rate / 50U)) {
                *div = (uint16_t)d;
                *of  = true;
                return true;
            }
        }
    }

    // 如果精确匹配失败，使用最接近偶数分频
    if (ideal >= I2S_PR_DIV_MIN && ideal <= I2S_PR_DIV_MAX) {
        *div = (uint16_t)ideal;
        *of  = false;
        return true;
    }
    if (ideal / 2U >= I2S_PR_DIV_MIN && ideal / 2U <= I2S_PR_DIV_MAX) {
        *div = (uint16_t)(ideal / 2U);
        *of  = false;
        return true;
    }

    return false;
}

/**
 * @brief 写入 CR 寄存器配置（在 EN=0 前提下）
 *
 * 将配置结构体的各个字段写入 CR 寄存器的对应位域。
 * 使用 CR_f 位域结构体逐位配置，确保不干扰保留位。
 */
static void i2s_write_cr_config(I2S_TypeDef *regs, const i2s_config_t *config)
{
    uint32_t cr = regs->CR;

    // 清除可配置位域（保留 EN/START/STOP/TXDMAEN/RXDMAEN）
    cr &= ~(I2S_CR_CHLEN_MSK | I2S_CR_DLEN_MSK | I2S_CR_CKPL_MSK
            | I2S_CR_STD_MSK | I2S_CR_PCMMODE_MSK | I2S_CR_MODE_MSK
            | I2S_CR_TEN_MSK | I2S_CR_REN_MSK | I2S_CR_IOSWP_MSK);

    // 通道长度
    if (config->chlen == I2S_CHLEN_32) {
        cr |= I2S_CR_CHLEN_MSK;
    }

    // 数据宽度
    cr |= ((uint32_t)config->data_width << I2S_CR_DLEN_POS) & I2S_CR_DLEN_MSK;

    // 时钟极性
    if (config->ckpl == I2S_CKPL_HIGH) {
        cr |= I2S_CR_CKPL_MSK;
    }

    // 音频标准
    cr |= ((uint32_t)config->standard << I2S_CR_STD_POS) & I2S_CR_STD_MSK;

    // PCM 帧同步模式（仅 PCM 标准有效，但不影响其他标准）
    if (config->pcm_mode == I2S_PCM_LONG_FRAME) {
        cr |= I2S_CR_PCMMODE_MSK;
    }

    // 主从模式
    if (config->mode == I2S_MODE_MASTER) {
        cr |= I2S_CR_MODE_MSK;
    }

    // 发送/接收使能
    if (config->direction == I2S_DIR_TX || config->direction == I2S_DIR_FULL) {
        cr |= I2S_CR_TEN_MSK;
    }
    if (config->direction == I2S_DIR_RX || config->direction == I2S_DIR_FULL) {
        cr |= I2S_CR_REN_MSK;
    }

    // IO 交换
    if (config->ioswp) {
        cr |= I2S_CR_IOSWP_MSK;
    }

    regs->CR = cr;
}

/**
 * @brief 等待指定状态标志置位（带超时）
 *
 * @return true: 标志已置位, false: 超时
 */
static bool i2s_wait_flag(I2S_TypeDef *regs, uint32_t mask, bool expect_set)
{
    uint32_t timeout = I2S_TIMEOUT_COUNT;
    while (timeout > 0U) {
        uint32_t sr = regs->SR;
        bool is_set = ((sr & mask) != 0U);
        if (is_set == expect_set) {
            return true;
        }
        timeout--;
    }
    return false;
}

//===========================================
// 第1层：快速初始化 API
//===========================================

/**
 * @brief 快速初始化 I2S 主机发送模式
 */
bool i2s_init_master_tx(i2s_instance_t i2s, uint32_t sample_rate)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL || sample_rate == 0U) {
        return false;
    }

    // 先禁用 I2S
    regs->CR &= ~I2S_CR_EN_MSK;

    i2s_config_t config = {
        .mode       = I2S_MODE_MASTER,
        .standard   = I2S_STD_PHILIPS,
        .data_width = I2S_DATA_WIDTH_16,
        .chlen      = I2S_CHLEN_16,
        .ckpl       = I2S_CKPL_LOW,
        .pcm_mode   = I2S_PCM_SHORT_FRAME,
        .ioswp      = false,
        .direction  = I2S_DIR_TX,
    };
    i2s_write_cr_config(regs, &config);

    // 计算分频
    uint16_t div;
    bool of;
    if (!i2s_calculate_clock(sample_rate, false, false, &div, &of)) {
        return false;
    }

    regs->PR_f.DIV   = div;
    regs->PR_f.OF    = of ? 1U : 0U;
    regs->PR_f.MCKOE = 0U;

    // 使能 I2S
    regs->CR |= I2S_CR_EN_MSK;

    return true;
}

/**
 * @brief 快速初始化 I2S 主机接收模式
 */
bool i2s_init_master_rx(i2s_instance_t i2s, uint32_t sample_rate)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL || sample_rate == 0U) {
        return false;
    }

    regs->CR &= ~I2S_CR_EN_MSK;

    i2s_config_t config = {
        .mode       = I2S_MODE_MASTER,
        .standard   = I2S_STD_PHILIPS,
        .data_width = I2S_DATA_WIDTH_16,
        .chlen      = I2S_CHLEN_16,
        .ckpl       = I2S_CKPL_LOW,
        .pcm_mode   = I2S_PCM_SHORT_FRAME,
        .ioswp      = false,
        .direction  = I2S_DIR_RX,
    };
    i2s_write_cr_config(regs, &config);

    uint16_t div;
    bool of;
    if (!i2s_calculate_clock(sample_rate, false, false, &div, &of)) {
        return false;
    }

    regs->PR_f.DIV   = div;
    regs->PR_f.OF    = of ? 1U : 0U;
    regs->PR_f.MCKOE = 0U;

    regs->CR |= I2S_CR_EN_MSK;

    return true;
}

/**
 * @brief 快速初始化 I2S 从机发送模式
 */
bool i2s_init_slave_tx(i2s_instance_t i2s)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL) {
        return false;
    }

    regs->CR &= ~I2S_CR_EN_MSK;

    i2s_config_t config = {
        .mode       = I2S_MODE_SLAVE,
        .standard   = I2S_STD_PHILIPS,
        .data_width = I2S_DATA_WIDTH_16,
        .chlen      = I2S_CHLEN_16,
        .ckpl       = I2S_CKPL_LOW,
        .pcm_mode   = I2S_PCM_SHORT_FRAME,
        .ioswp      = false,
        .direction  = I2S_DIR_TX,
    };
    i2s_write_cr_config(regs, &config);

    regs->CR |= I2S_CR_EN_MSK;

    return true;
}

/**
 * @brief 快速初始化 I2S 从机接收模式
 */
bool i2s_init_slave_rx(i2s_instance_t i2s)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL) {
        return false;
    }

    regs->CR &= ~I2S_CR_EN_MSK;

    i2s_config_t config = {
        .mode       = I2S_MODE_SLAVE,
        .standard   = I2S_STD_PHILIPS,
        .data_width = I2S_DATA_WIDTH_16,
        .chlen      = I2S_CHLEN_16,
        .ckpl       = I2S_CKPL_LOW,
        .pcm_mode   = I2S_PCM_SHORT_FRAME,
        .ioswp      = false,
        .direction  = I2S_DIR_RX,
    };
    i2s_write_cr_config(regs, &config);

    regs->CR |= I2S_CR_EN_MSK;

    return true;
}

//===========================================
// 第2层：基础配置 API
//===========================================

/**
 * @brief 配置 I2S 工作参数
 *
 * @note 覆盖寄存器：CR.CHLEN, CR.DTLEN, CR.CKPL, CR.STD, CR.PCMMODE, CR.MODE, CR.TEN, CR.REN, CR.IOSWP
 */
bool i2s_configure(i2s_instance_t i2s, const i2s_config_t *config)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL || config == NULL) {
        return false;
    }

    // 参数校验
    if (config->data_width > I2S_DATA_WIDTH_32) {
        return false;
    }
    if (config->standard > I2S_STD_PCM) {
        return false;
    }
    // 通道长度必须 ≥ 数据宽度
    if (config->chlen == I2S_CHLEN_16 && config->data_width > I2S_DATA_WIDTH_16) {
        return false;
    }

    // 确保 I2S 关闭状态下配置
    regs->CR &= ~I2S_CR_EN_MSK;

    i2s_write_cr_config(regs, config);

    return true;
}

/**
 * @brief 配置 I2S 时钟预分频器
 *
 * @note 覆盖寄存器：PR.DIV, PR.OF, PR.MCKOE
 * @note 当 I2S 模式关闭时才能配置
 */
bool i2s_config_clock(i2s_instance_t i2s, const i2s_clock_config_t *config)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL || config == NULL) {
        return false;
    }

    if (config->div < I2S_PR_DIV_MIN || config->div > I2S_PR_DIV_MAX) {
        return false;
    }

    regs->PR_f.DIV   = config->div;
    regs->PR_f.OF    = config->of ? 1U : 0U;
    regs->PR_f.MCKOE = config->mckoe ? 1U : 0U;

    return true;
}

//===========================================
// 第3层：高级功能 API
//===========================================

/**
 * @brief 配置主模式接收数据量
 *
 * @note 覆盖寄存器：RSIZE
 */
void i2s_set_rx_count(i2s_instance_t i2s, uint16_t count)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL) {
        return;
    }
    regs->RSIZE_f.RSIZE = count;
}

/**
 * @brief 启动 I2S 传输（主模式）
 *
 * @note 覆盖寄存器：CR.START
 */
void i2s_start(i2s_instance_t i2s)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL) {
        return;
    }
    regs->CR_f.START = 1U;
}

/**
 * @brief 停止 I2S 传输（主模式）
 *
 * @note 覆盖寄存器：CR.STOP
 */
void i2s_stop(i2s_instance_t i2s)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL) {
        return;
    }
    regs->CR_f.STOP = 1U;
}

/**
 * @brief 使能 I2S DMA 发送
 *
 * @note 覆盖寄存器：CR.TXDMAEN
 */
void i2s_dma_tx_enable(i2s_instance_t i2s, bool enable)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL) {
        return;
    }
    if (enable) {
        regs->CR_f.TXDMAEN = 1U;
    } else {
        regs->CR_f.TXDMAEN = 0U;
    }
}

/**
 * @brief 使能 I2S DMA 接收
 *
 * @note 覆盖寄存器：CR.RXDMAEN
 */
void i2s_dma_rx_enable(i2s_instance_t i2s, bool enable)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL) {
        return;
    }
    if (enable) {
        regs->CR_f.RXDMAEN = 1U;
    } else {
        regs->CR_f.RXDMAEN = 0U;
    }
}

/**
 * @brief 注册 I2S 中断回调
 */
void i2s_irq_register(i2s_instance_t i2s, i2s_callback_t callback)
{
    if (i2s >= I2S_COUNT) {
        return;
    }
    i2s_callbacks[i2s] = callback;
}

/**
 * @brief 使能 I2S 中断
 *
 * @note 覆盖寄存器：IER
 */
void i2s_irq_enable(i2s_instance_t i2s, i2s_irq_t irq)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL) {
        return;
    }
    regs->IER |= (uint32_t)irq;
}

/**
 * @brief 禁用 I2S 中断
 *
 * @note 覆盖寄存器：IER
 */
void i2s_irq_disable(i2s_instance_t i2s, i2s_irq_t irq)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL) {
        return;
    }
    regs->IER &= ~(uint32_t)irq;
}

//===========================================
// 第4层：控制与查询 API
//===========================================

/**
 * @brief 使能 I2S 外设
 *
 * @note 覆盖寄存器：CR.EN
 */
void i2s_enable(i2s_instance_t i2s)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL) {
        return;
    }
    regs->CR_f.EN = 1U;
}

/**
 * @brief 禁用 I2S 外设
 *
 * @note 覆盖寄存器：CR.EN
 * @note 主机模式应在 MSUSP=1 后清零；从机发送模式应在 TXE=1 且 SVTC=1 后清零
 */
void i2s_disable(i2s_instance_t i2s)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL) {
        return;
    }
    regs->CR_f.EN = 0U;
}

/**
 * @brief 发送数据（阻塞，等待 TXE=1）
 *
 * @note 覆盖寄存器：TXDR
 */
bool i2s_send(i2s_instance_t i2s, uint32_t data)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL) {
        return false;
    }

    // 等待发送缓冲区空（TXE=1）
    if (!i2s_wait_flag(regs, I2S_SR_TXE_MSK, true)) {
        return false;
    }

    regs->TXDR = data;
    return true;
}

/**
 * @brief 接收数据（阻塞，等待 RXNE=1）
 *
 * @note 覆盖寄存器：RXDR
 */
bool i2s_receive(i2s_instance_t i2s, uint32_t *data)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL || data == NULL) {
        return false;
    }

    // 等待接收缓冲区非空（RXNE=1）
    if (!i2s_wait_flag(regs, I2S_SR_RXNE_MSK, true)) {
        return false;
    }

    *data = regs->RXDR;
    return true;
}

/**
 * @brief 获取 SR 状态寄存器值
 *
 * @note 覆盖寄存器：SR
 */
uint32_t i2s_get_flags(i2s_instance_t i2s)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL) {
        return 0U;
    }
    return regs->SR;
}

/**
 * @brief 判断指定状态标志是否置位
 *
 * @note 覆盖寄存器：SR
 */
bool i2s_flag_is_set(i2s_instance_t i2s, i2s_flag_t flag)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL) {
        return false;
    }
    return (regs->SR & (uint32_t)flag) != 0U;
}

/**
 * @brief 清除状态标志
 *
 * @note 覆盖寄存器：SR
 * @note 可清除的标志：UDR/OVR/FE/MSUSP/SVTC（写 0 清除）
 * @note OVR 清除序列：先读 RXDR，再写 SR 对应位清零
 */
void i2s_clear_flags(i2s_instance_t i2s, uint32_t flags)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL) {
        return;
    }

    // OVR 清除序列：先读 RXDR 再清标志
    if (flags & I2S_SR_OVR_MSK) {
        (void)regs->RXDR;
    }

    // 可清除标志写 0 清除（其他位写 1 保持）
    uint32_t clearable = I2S_SR_UDR_MSK | I2S_SR_OVR_MSK | I2S_SR_FE_MSK
                         | I2S_SR_MSUSP_MSK | I2S_SR_SVTC_MSK;
    uint32_t mask = flags & clearable;
    if (mask != 0U) {
        regs->SR &= ~mask;
    }
}

//===========================================
// 中断处理函数
//===========================================

/**
 * @brief I2S 通用中断处理
 */
static void i2s_irq_handler(i2s_instance_t i2s)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL) {
        return;
    }

    uint32_t sr = regs->SR;
    uint32_t ier = regs->IER;

    // 计算已使能且已触发的中断标志
    uint32_t pending = sr & ier;

    if (pending != 0U && i2s_callbacks[i2s] != NULL) {
        i2s_callbacks[i2s](i2s, pending);
    }
}

/**
 * @brief I2S1 中断服务函数
 */
void I2S1_IRQHandler(void)
{
    i2s_irq_handler(I2S_1);
}

/**
 * @brief I2S2 中断服务函数
 */
void I2S2_IRQHandler(void)
{
    i2s_irq_handler(I2S_2);
}
