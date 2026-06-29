/**
 * @file sdmmc.c
 * @brief ACM32P4 SDMMC 驱动实现
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#include "hardware/sdmmc.h"
#include "hardware/clocks.h"
#include "hardware/system.h"
#include "device/acm32p4xx.h"
#include <assert.h>
#include <string.h>

//===========================================
// 内部宏定义
//===========================================

#define SDMMC_CMD_START_CMD_POS       31U
#define SDMMC_CMD_USE_HOLD_REG_POS    29U
#define SDMMC_CMD_VOLT_SWITCH_POS     28U
#define SDMMC_CMD_BOOT_MODE_POS       27U
#define SDMMC_CMD_DIS_BOOT_POS        26U
#define SDMMC_CMD_EXPECT_BOOT_ACK_POS 25U
#define SDMMC_CMD_ENA_BOOT_POS        24U
#define SDMMC_CMD_CCS_EXPECTED_POS    23U
#define SDMMC_CMD_READ_CEATA_POS      22U
#define SDMMC_CMD_UPDATE_CLK_POS      21U
#define SDMMC_CMD_CARD_NUM_POS        16U
#define SDMMC_CMD_SEND_INI_SEQ_POS    15U
#define SDMMC_CMD_STOP_ABORT_POS      14U
#define SDMMC_CMD_WAIT_PRVDATA_POS    13U
#define SDMMC_CMD_SEND_AUTO_STOP_POS  12U
#define SDMMC_CMD_TRANSFER_MODE_POS   11U
#define SDMMC_CMD_READ_WRITE_POS      10U
#define SDMMC_CMD_DATA_EXPECTED_POS   9U
#define SDMMC_CMD_CHECK_RESP_CRC_POS  8U
#define SDMMC_CMD_RESP_LENGTH_POS     7U
#define SDMMC_CMD_RESP_EXPECT_POS     6U
#define SDMMC_CMD_CMD_INDEX_POS       0U

#define SDMMC_CTRL_CONTROLLER_RESET_POS   0U
#define SDMMC_CTRL_FIFO_RESET_POS         1U
#define SDMMC_CTRL_DMA_RESET_POS          2U
#define SDMMC_CTRL_INT_ENABLE_POS         4U
#define SDMMC_CTRL_READ_WAIT_POS          6U
#define SDMMC_CTRL_SEND_AS_CCSD_POS       7U
#define SDMMC_CTRL_ABORT_READ_DATA_POS    8U
#define SDMMC_CTRL_SEND_IRQ_RESPONSE_POS  9U
#define SDMMC_CTRL_CEATA_DEV_INT_POS      11U

#define SDMMC_PWREN_POWER_ON_POS          0U

#define SDMMC_CLKENA_CCLK_ENABLE_POS      0U
#define SDMMC_CLKENA_LP_ENABLE_POS        16U

#define SDMMC_CTYPE_WIDTH0_POS            0U
#define SDMMC_CTYPE_WIDTH1_POS            16U

#define SDMMC_FIFOTH_TX_WMARK_POS         0U
#define SDMMC_FIFOTH_RX_WMARK_POS         16U

#define SDMMC_TMOUT_DATA_POS              8U
#define SDMMC_TMOUT_RESP_POS              0U

#define SDMMC_BMOD_SWR_POS                0U
#define SDMMC_BMOD_FB_POS                 1U
#define SDMMC_BMOD_DSL_POS                2U
#define SDMMC_BMOD_DE_POS                 7U
#define SDMMC_BMOD_PBL_POS                8U

#define SDMMC_STATUS_FIFO_FULL_POS        3U   /* STATUS_f.FIFO_FULL       bit[3] */
#define SDMMC_STATUS_FIFO_EMPTY_POS       2U   /* STATUS_f.FIFO_EMPTY      bit[2] */
#define SDMMC_STATUS_FIFO_COUNT_POS       17U  /* STATUS_f.FIFO_COUNT      bit[29:17] */
#define SDMMC_STATUS_DATA_BUSY_POS        9U   /* STATUS_f.DATA_BUSY       bit[9] */
#define SDMMC_STATUS_DATA_3_POS          8U   /* STATUS_f.DATA_3_STATUS   bit[8] */

//===========================================
// 内部类型定义
//===========================================

typedef struct {
    bool              initialized;       ///< 是否已初始化
    sdmmc_card_info_t card;              ///< 当前卡信息
    sdmmc_transfer_t  active_transfer;   ///< 当前传输
    volatile bool     transfer_done;     ///< 传输完成标志
    volatile bool     transfer_success;  ///< 传输成功标志
    sdmmc_irq_callbacks_t irq_cbs;       ///< 中断回调注册表
} sdmmc_state_t;

//===========================================
// 内部状态变量
//===========================================

static sdmmc_state_t s_sdmmc_state;

//===========================================
// 内部辅助函数
//===========================================

/**
 * @brief 等待寄存器位清零（带超时）
 */
static bool sdmmc_wait_bit_clear(volatile uint32_t *reg, uint32_t mask, uint32_t timeout_ms)
{
    uint32_t start = system_get_tick();
    while ((*reg & mask) != 0U) {
        if (system_elapsed(start, timeout_ms)) {
            return false;
        }
    }
    return true;
}

/**
 * @brief 等待寄存器位置位（带超时）
 */
static bool sdmmc_wait_bit_set(volatile uint32_t *reg, uint32_t mask, uint32_t timeout_ms)
{
    uint32_t start = system_get_tick();
    while ((*reg & mask) == 0U) {
        if (system_elapsed(start, timeout_ms)) {
            return false;
        }
    }
    return true;
}

//===========================================
// 第1层：快速初始化 API
//===========================================

/**
 * @brief 初始化 SDMMC 主机控制器
 *
 * @param[in]  config  初始化配置（时钟/位宽/传输模式等）
 * @return true: 成功, false: 失败
 *
 * @note 覆盖寄存器：CTRL, PWREN, CLKDIV, CLKSRC, CLKENA, CTYPE,
 *       TMOUT, BLKSIZ, FIFOTH, INTMASK, RSTN, BMOD
 * @note 自动使能 RCC SDMMC 时钟和复位释放
 *
 * @code
 * sdmmc_init_config_t cfg = {
 *     .clk_div = 4U,
 *     .bus_width = SDMMC_BUS_WIDTH_1BIT,
 *     .transfer_mode = SDMMC_TRANSFER_MODE_POLLING,
 * };
 * sdmmc_init(&cfg);
 * @endcode
 */
bool sdmmc_init(const sdmmc_init_config_t *config)
{
    assert(config != NULL);
    if (config == NULL) {
        return false;
    }

    /* 参数范围检查 */
    if ((config->clk_div < SDMMC_CLK_DIV_MIN) && (config->clk_div != 0U)) {
        return false;
    }
    if (config->clk_div > SDMMC_CLK_DIV_MAX) {
        return false;
    }
    if (config->bus_width > SDMMC_BUS_WIDTH_8BIT) {
        return false;
    }
    if (config->transfer_mode > SDMMC_TRANSFER_MODE_IDMA) {
        return false;
    }

    /* 使能 RCC SDMMC 时钟 */
    clock_periph_enable(CLK_SDMMC);
    /* 释放 RCC SDMMC 复位 */
    clock_periph_reset(CLK_SDMMC);

    /* 复位控制器、FIFO、DMA */
    SDMMC->CTRL |= (1UL << SDMMC_CTRL_CONTROLLER_RESET_POS)
                |  (1UL << SDMMC_CTRL_FIFO_RESET_POS)
                |  (1UL << SDMMC_CTRL_DMA_RESET_POS);

    /* 等待复位完成 */
    sdmmc_wait_bit_clear(&SDMMC->CTRL,
                         (1UL << SDMMC_CTRL_CONTROLLER_RESET_POS)
                       | (1UL << SDMMC_CTRL_FIFO_RESET_POS)
                       | (1UL << SDMMC_CTRL_DMA_RESET_POS),
                         SDMMC_DEFAULT_TIMEOUT_MS);

    /* 使能全局中断 */
    SDMMC->CTRL |= (1UL << SDMMC_CTRL_INT_ENABLE_POS);

    /* 屏蔽所有中断 */
    SDMMC->INTMASK = 0U;

    /* 配置超时 */
    SDMMC->TMOUT = 0xFFFFFF40U;

    /* 配置 FIFO 阈值（TX/RX = FIFO 深度的一半） */
    SDMMC->FIFOTH = ((SDMMC_FIFO_DEPTH / 2U) << SDMMC_FIFOTH_TX_WMARK_POS)
                  | ((SDMMC_FIFO_DEPTH / 2U) << SDMMC_FIFOTH_RX_WMARK_POS);

    /* 配置时钟分频 */
    if (config->clk_div > 0U) {
        SDMMC->CLKDIV = config->clk_div;
    }
    SDMMC->CLKSRC = 0U;  /* 使用卡时钟源 0 */
    SDMMC->CLKENA |= (1UL << SDMMC_CLKENA_CCLK_ENABLE_POS);

    /* 配置总线宽度 */
    SDMMC->CTYPE = (uint32_t)config->bus_width;

    /* 配置 DDR 模式 */
    if (config->enable_ddr) {
        SDMMC->UHS_REG |= (1UL << 16);  /* card0 DDR */
    }

    /* 配置硬件复位 */
    if (config->enable_hw_reset) {
        SDMMC->RSTN = 1U;  /* 释放复位 */
    }

    /* 上电 */
    SDMMC->PWREN |= (1UL << SDMMC_PWREN_POWER_ON_POS);

    /* 保存状态 */
    s_sdmmc_state.initialized = true;
    if (config->irq_cbs != NULL) {
        s_sdmmc_state.irq_cbs = *config->irq_cbs;
    }

    return true;
}

/**
 * @brief 反初始化 SDMMC 主机控制器
 *
 * @note 覆盖寄存器：CTRL, CLKENA, PWREN, INTMASK
 */
void sdmmc_deinit(void)
{
    /* 禁用卡时钟 */
    SDMMC->CLKENA = 0U;

    /* 下电 */
    SDMMC->PWREN = 0U;

    /* 禁用全局中断 */
    SDMMC->CTRL &= ~(1UL << SDMMC_CTRL_INT_ENABLE_POS);

    /* 清除中断屏蔽 */
    SDMMC->INTMASK = 0U;

    /* 复位控制器 */
    SDMMC->CTRL |= (1UL << SDMMC_CTRL_CONTROLLER_RESET_POS);

    /* 禁用 RCC 时钟 */
    clock_periph_disable(CLK_SDMMC);

    s_sdmmc_state.initialized = false;
}

//===========================================
// 第2层：基础配置 API
//===========================================

/**
 * @brief 配置 SDMMC 卡时钟分频
 *
 * @param[in]  clk_div  时钟分频值（0=旁路, 1-510=分频, 分频=2×clk_div）
 * @return true: 成功, false: 失败
 *
 * @note 覆盖寄存器：CLKDIV, CLKSRC, CLKENA
 * @note 需要先更新 CLKDIV 和 CLKSRC，再通过 CMD.UPDATE_CLK 使新分频生效
 */
bool sdmmc_config_clock(uint32_t clk_div)
{
    if (clk_div > SDMMC_CLK_DIV_MAX) {
        return false;
    }

    SDMMC->CLKDIV = clk_div;
    /* 更新时钟：发送 UPDATE_CLK 命令 */
    SDMMC->CMDARG = 0U;
    SDMMC->CMD = (1UL << SDMMC_CMD_UPDATE_CLK_POS)
               | (1UL << SDMMC_CMD_START_CMD_POS)
               | (1UL << SDMMC_CMD_USE_HOLD_REG_POS);

    return sdmmc_wait_bit_clear(&SDMMC->CMD, (1UL << SDMMC_CMD_START_CMD_POS),
                                SDMMC_DEFAULT_TIMEOUT_MS);
}

/**
 * @brief 配置 SDMMC 总线宽度
 *
 * @param[in]  width  总线宽度
 * @return true: 成功, false: 参数无效
 *
 * @note 覆盖寄存器：CTYPE
 */
bool sdmmc_config_bus_width(sdmmc_bus_width_t width)
{
    if (width > SDMMC_BUS_WIDTH_8BIT) {
        return false;
    }

    SDMMC->CTYPE = (uint32_t)width;
    return true;
}

/**
 * @brief 配置 SDMMC 超时参数
 *
 * @param[in]  data_timeout  数据超时（bit[31:8]）
 * @param[in]  resp_timeout  响应超时（bit[7:0]）
 * @return true: 成功
 *
 * @note 覆盖寄存器：TMOUT
 */
bool sdmmc_config_timeout(uint32_t data_timeout, uint8_t resp_timeout)
{
    SDMMC->TMOUT = (data_timeout << SDMMC_TMOUT_DATA_POS)
                 | ((uint32_t)resp_timeout << SDMMC_TMOUT_RESP_POS);
    return true;
}

/**
 * @brief 配置 SDMMC 块大小
 *
 * @param[in]  block_size  块大小（1-65536）
 * @return true: 成功, false: 参数无效
 *
 * @note 覆盖寄存器：BLKSIZ
 */
bool sdmmc_config_block_size(uint32_t block_size)
{
    if ((block_size == 0U) || (block_size > SDMMC_BLOCK_SIZE_MAX)) {
        return false;
    }

    SDMMC->BLKSIZ = block_size;
    return true;
}

/**
 * @brief 配置 SDMMC FIFO 阈值
 *
 * @param[in]  tx_level  TX FIFO 阈值（0-15）
 * @param[in]  rx_level  RX FIFO 阈值（0-15）
 * @return true: 成功, false: 参数无效
 *
 * @note 覆盖寄存器：FIFOTH
 * @note TX 阈值 = FIFO_DEPTH - tx_level，RX 阈值 = rx_level
 */
bool sdmmc_config_fifo_threshold(uint8_t tx_level, uint8_t rx_level)
{
    if ((tx_level > SDMMC_FIFO_DEPTH) || (rx_level > SDMMC_FIFO_DEPTH)) {
        return false;
    }

    SDMMC->FIFOTH = ((uint32_t)tx_level << SDMMC_FIFOTH_TX_WMARK_POS)
                  | ((uint32_t)rx_level << SDMMC_FIFOTH_RX_WMARK_POS);
    return true;
}

/**
 * @brief 配置 SDMMC 卡检测去抖动
 *
 * @param[in]  debounce_count  去抖动计数
 * @return true: 成功
 *
 * @note 覆盖寄存器：DEBNCE
 */
bool sdmmc_config_card_detect(uint32_t debounce_count)
{
    SDMMC->DEBNCE = debounce_count;
    return true;
}

//===========================================
// 第3层：高级功能 API
//===========================================

/**
 * @brief 复位 SDMMC 控制器
 *
 * @note 覆盖寄存器：CTRL.CONTROLLER_RESET
 */
void sdmmc_reset_controller(void)
{
    SDMMC->CTRL |= (1UL << SDMMC_CTRL_CONTROLLER_RESET_POS);
    sdmmc_wait_bit_clear(&SDMMC->CTRL, (1UL << SDMMC_CTRL_CONTROLLER_RESET_POS),
                         SDMMC_DEFAULT_TIMEOUT_MS);
}

/**
 * @brief 复位 SDMMC FIFO
 *
 * @note 覆盖寄存器：CTRL.FIFO_RESET
 */
void sdmmc_reset_fifo(void)
{
    SDMMC->CTRL |= (1UL << SDMMC_CTRL_FIFO_RESET_POS);
    sdmmc_wait_bit_clear(&SDMMC->CTRL, (1UL << SDMMC_CTRL_FIFO_RESET_POS),
                         SDMMC_DEFAULT_TIMEOUT_MS);
}

/**
 * @brief 复位 SDMMC DMA
 *
 * @note 覆盖寄存器：CTRL.DMA_RESET, BMOD.SWR
 */
void sdmmc_reset_dma(void)
{
    SDMMC->CTRL |= (1UL << SDMMC_CTRL_DMA_RESET_POS);
    sdmmc_wait_bit_clear(&SDMMC->CTRL, (1UL << SDMMC_CTRL_DMA_RESET_POS),
                         SDMMC_DEFAULT_TIMEOUT_MS);
    SDMMC->BMOD |= (1UL << SDMMC_BMOD_SWR_POS);
    sdmmc_wait_bit_clear(&SDMMC->BMOD, (1UL << SDMMC_BMOD_SWR_POS),
                         SDMMC_DEFAULT_TIMEOUT_MS);
}

/**
 * @brief SDMMC 卡上电
 *
 * @note 覆盖寄存器：PWREN
 */
void sdmmc_power_on(void)
{
    SDMMC->PWREN |= (1UL << SDMMC_PWREN_POWER_ON_POS);
}

/**
 * @brief SDMMC 卡下电
 *
 * @note 覆盖寄存器：PWREN
 */
void sdmmc_power_off(void)
{
    SDMMC->PWREN &= ~(1UL << SDMMC_PWREN_POWER_ON_POS);
}

/**
 * @brief 使能卡时钟
 *
 * @note 覆盖寄存器：CLKENA
 */
void sdmmc_clock_enable(void)
{
    SDMMC->CLKENA |= (1UL << SDMMC_CLKENA_CCLK_ENABLE_POS);
}

/**
 * @brief 禁用卡时钟
 *
 * @note 覆盖寄存器：CLKENA
 */
void sdmmc_clock_disable(void)
{
    SDMMC->CLKENA &= ~(1UL << SDMMC_CLKENA_CCLK_ENABLE_POS);
}

/**
 * @brief 写 FIFO 数据
 *
 * @param[in]  data       数据缓冲区（4 字节对齐）
 * @param[in]  word_count 写入字数
 *
 * @note 覆盖寄存器：DATA
 * @note 调用前需确保 FIFO 有足够空间
 */
void sdmmc_write_fifo(const uint32_t *data, uint32_t word_count)
{
    uint32_t i;
    for (i = 0U; i < word_count; i++) {
        SDMMC->DATA = data[i];
    }
}

/**
 * @brief 读 FIFO 数据
 *
 * @param[out] data       数据缓冲区（4 字节对齐）
 * @param[in]  word_count 读取字数
 *
 * @note 覆盖寄存器：DATA
 * @note 调用前需确保 FIFO 有足够数据
 */
void sdmmc_read_fifo(uint32_t *data, uint32_t word_count)
{
    uint32_t i;
    for (i = 0U; i < word_count; i++) {
        data[i] = SDMMC->DATA;
    }
}

/**
 * @brief 使能 SDMMC 中断
 *
 * @param[in]  mask  中断掩码（sdmmc_irq_t 位或组合）
 *
 * @note 覆盖寄存器：INTMASK
 */
void sdmmc_irq_enable(sdmmc_irq_t mask)
{
    SDMMC->INTMASK |= (uint32_t)mask;
}

/**
 * @brief 禁用 SDMMC 中断
 *
 * @param[in]  mask  中断掩码（sdmmc_irq_t 位或组合）
 *
 * @note 覆盖寄存器：INTMASK
 */
void sdmmc_irq_disable(sdmmc_irq_t mask)
{
    SDMMC->INTMASK &= ~((uint32_t)mask);
}

/**
 * @brief 清除 SDMMC 中断标志
 *
 * @param[in]  mask  中断掩码（sdmmc_irq_t 位或组合）
 *
 * @note 覆盖寄存器：RINTSTS（写 1 清除）
 */
void sdmmc_irq_clear(sdmmc_irq_t mask)
{
    SDMMC->RINTSTS = (uint32_t)mask;
}

/**
 * @brief 注册中断回调（可在运行时动态更新）
 *
 * @param[in]  cbs  回调注册表（NULL = 清空所有回调）
 *
 * @note 每个成员对应一个中断源，NULL 表示不注册
 * @note ISR 中按中断源分别派发到对应回调
 */
void sdmmc_irq_register(const sdmmc_irq_callbacks_t *cbs)
{
    if (cbs != NULL) {
        s_sdmmc_state.irq_cbs = *cbs;
    } else {
        memset(&s_sdmmc_state.irq_cbs, 0, sizeof(s_sdmmc_state.irq_cbs));
    }
}

/**
 * @brief 单独注册/取消某个中断源的回调
 *
 * @param[in]  irq   中断源（单个，不能是组合掩码）
 * @param[in]  cb    回调函数（NULL = 取消注册）
 *
 * @note 不影响其他已注册的中断回调
 *
 * @code
 * sdmmc_irq_set_callback(SDMMC_IRQ_CDET, on_card_detect);
 * sdmmc_irq_set_callback(SDMMC_IRQ_DTO, NULL);  // 取消
 * @endcode
 */
void sdmmc_irq_set_callback(sdmmc_irq_t irq, sdmmc_irq_callback_t cb)
{
    switch (irq) {
        case SDMMC_IRQ_CDET:  s_sdmmc_state.irq_cbs.card_detect       = cb; break;
        case SDMMC_IRQ_CDONE: s_sdmmc_state.irq_cbs.cmd_done          = cb; break;
        case SDMMC_IRQ_TXDR:  s_sdmmc_state.irq_cbs.tx_data_request   = cb; break;
        case SDMMC_IRQ_RXDR:  s_sdmmc_state.irq_cbs.rx_data_request   = cb; break;
        case SDMMC_IRQ_DTO:   s_sdmmc_state.irq_cbs.data_timeout      = cb; break;
        case SDMMC_IRQ_RTO:   s_sdmmc_state.irq_cbs.resp_timeout      = cb; break;
        case SDMMC_IRQ_RCRC:  s_sdmmc_state.irq_cbs.resp_crc_error    = cb; break;
        case SDMMC_IRQ_DCRC:  s_sdmmc_state.irq_cbs.data_crc_error    = cb; break;
        case SDMMC_IRQ_DRTO:  s_sdmmc_state.irq_cbs.data_read_timeout = cb; break;
        case SDMMC_IRQ_HTO:   s_sdmmc_state.irq_cbs.data_starve_timeout = cb; break;
        case SDMMC_IRQ_FRUN:  s_sdmmc_state.irq_cbs.fifo_run          = cb; break;
        case SDMMC_IRQ_HLE:   s_sdmmc_state.irq_cbs.hw_lock_error     = cb; break;
        case SDMMC_IRQ_SBE:   s_sdmmc_state.irq_cbs.start_bit_error   = cb; break;
        case SDMMC_IRQ_EBE:   s_sdmmc_state.irq_cbs.end_bit_error     = cb; break;
        case SDMMC_IRQ_ACD:   s_sdmmc_state.irq_cbs.auto_cmd_done     = cb; break;
        case SDMMC_IRQ_SDIO0: s_sdmmc_state.irq_cbs.sdio_interrupt0   = cb; break;
        case SDMMC_IRQ_SDIO1: s_sdmmc_state.irq_cbs.sdio_interrupt1   = cb; break;
        default: break;
    }
}

/**
 * @brief 配置 IDMAC
 *
 * @param[in]  desc_base_addr  描述符列表基地址（4 字节对齐）
 * @param[in]  pbl             可编程突发长度（1/4/8/16/32/64/128/256）
 * @return true: 成功, false: 地址未对齐
 *
 * @note 覆盖寄存器：BMOD, DBADDR, IDINTEN
 */
bool sdmmc_dma_config(uint32_t desc_base_addr, uint8_t pbl)
{
    if ((desc_base_addr & 0x3U) != 0U) {
        return false;
    }

    /* PBL 编码检查：0=1, 1=4, 2=8, 3=16, 4=32, 5=64, 6=128, 7=256 */
    if (pbl > 7U) {
        return false;
    }

    /* 设置描述符基地址 */
    SDMMC->DBADDR = desc_base_addr;

    /* 配置 IDMAC：固定突发 + PBL */
    SDMMC->BMOD = (1UL << SDMMC_BMOD_FB_POS)
                | ((uint32_t)pbl << SDMMC_BMOD_PBL_POS);

    /* 使能 IDMA 中断 */
    SDMMC->IDINTEN = (1UL << 0)  /* TI_EN */
                   | (1UL << 1)  /* RI_EN */
                   | (1UL << 4)  /* DU_EN */
                   | (1UL << 8)  /* NIS_EN */
                   | (1UL << 9); /* AIS_EN */

    return true;
}

/**
 * @brief 启动 IDMAC
 *
 * @note 覆盖寄存器：BMOD.DE, PLDMND
 */
void sdmmc_dma_start(void)
{
    SDMMC->BMOD |= (1UL << SDMMC_BMOD_DE_POS);
    SDMMC->PLDMND = 1U;
}

/**
 * @brief 停止 IDMAC
 *
 * @note 覆盖寄存器：BMOD.DE
 */
void sdmmc_dma_stop(void)
{
    SDMMC->BMOD &= ~(1UL << SDMMC_BMOD_DE_POS);
}

/**
 * @brief 恢复 IDMAC（描述符不可用时）
 *
 * @return true: 成功
 *
 * @note 覆盖寄存器：PLDMND
 */
bool sdmmc_dma_resume(void)
{
    SDMMC->PLDMND = 1U;
    return true;
}

/**
 * @brief 使能/禁用 DDR 模式
 *
 * @param[in]  enable  true: 使能 DDR, false: 禁用
 *
 * @note 覆盖寄存器：UHS_REG
 */
void sdmmc_ddr_enable(bool enable)
{
    if (enable) {
        SDMMC->UHS_REG |= (1UL << 16);  /* card0 DDR */
    } else {
        SDMMC->UHS_REG &= ~(1UL << 16);
    }
}

/**
 * @brief 硬件复位卡（拉低 RSTN）
 *
 * @note 覆盖寄存器：RSTN
 */
void sdmmc_hw_reset_assert(void)
{
    SDMMC->RSTN &= ~1U;  /* card0 复位 = 0 */
}

/**
 * @brief 释放硬件复位（拉高 RSTN）
 *
 * @note 覆盖寄存器：RSTN
 */
void sdmmc_hw_reset_deassert(void)
{
    SDMMC->RSTN |= 1U;  /* card0 复位 = 1 */
}

/**
 * @brief 发送初始化序列（80 个时钟周期）
 *
 * @note 覆盖寄存器：CMD.SEND_INI_SEQ
 */
void sdmmc_send_init_sequence(void)
{
    SDMMC->CMDARG = 0U;
    SDMMC->CMD = (1UL << SDMMC_CMD_SEND_INI_SEQ_POS)
               | (1UL << SDMMC_CMD_START_CMD_POS)
               | (1UL << SDMMC_CMD_USE_HOLD_REG_POS);
    sdmmc_wait_bit_clear(&SDMMC->CMD, (1UL << SDMMC_CMD_START_CMD_POS),
                         SDMMC_DEFAULT_TIMEOUT_MS);
}

//===========================================
// 第4层：控制与查询 API
//===========================================

/**
 * @brief 获取原始中断状态
 *
 * @return 中断状态（sdmmc_irq_t 位掩码）
 *
 * @note 覆盖寄存器：RINTSTS
 */
sdmmc_irq_t sdmmc_get_raw_irq_status(void)
{
    return (sdmmc_irq_t)SDMMC->RINTSTS;
}

/**
 * @brief 获取屏蔽后中断状态
 *
 * @return 中断状态（sdmmc_irq_t 位掩码）
 *
 * @note 覆盖寄存器：MINTSTS
 */
sdmmc_irq_t sdmmc_get_masked_irq_status(void)
{
    return (sdmmc_irq_t)SDMMC->MINTSTS;
}

/**
 * @brief 获取 SDMMC 状态寄存器值
 *
 * @return STATUS 寄存器原始值
 *
 * @note 覆盖寄存器：STATUS
 */
uint32_t sdmmc_get_status(void)
{
    return SDMMC->STATUS;
}

/**
 * @brief 检查卡是否插入
 *
 * @return true: 卡已插入, false: 无卡
 *
 * @note 覆盖寄存器：STATUS.DATA_3_STATUS（bit[8]，DATA[3] 原始值用于卡检测）
 */
bool sdmmc_is_card_inserted(void)
{
    /* STATUS bit[8] = DATA_3_STATUS，原始数据线 3 的值用于检测卡插入 */
    return ((SDMMC->STATUS & (1UL << SDMMC_STATUS_DATA_3_POS)) != 0U);
}

/**
 * @brief 获取已传输卡字节数
 *
 * @return 卡字节计数
 *
 * @note 覆盖寄存器：TCBCNT
 */
uint32_t sdmmc_get_tcbcnt(void)
{
    return SDMMC->TCBCNT;
}

/**
 * @brief 获取已传输 FIFO 字节数
 *
 * @return FIFO 字节计数
 *
 * @note 覆盖寄存器：TBBCNT
 */
uint32_t sdmmc_get_tbbcnt(void)
{
    return SDMMC->TBBCNT;
}

/**
 * @brief 检查 FIFO 是否满
 *
 * @return true: FIFO 满
 *
 * @note 覆盖寄存器：STATUS.FIFO_FULL
 */
bool sdmmc_is_fifo_full(void)
{
    return ((SDMMC->STATUS & (1UL << SDMMC_STATUS_FIFO_FULL_POS)) != 0U);
}

/**
 * @brief 检查 FIFO 是否空
 *
 * @return true: FIFO 空
 *
 * @note 覆盖寄存器：STATUS.FIFO_EMPTY
 */
bool sdmmc_is_fifo_empty(void)
{
    return ((SDMMC->STATUS & (1UL << SDMMC_STATUS_FIFO_EMPTY_POS)) != 0U);
}

/**
 * @brief 获取 FIFO 中数据数量
 *
 * @return FIFO 数据计数（字数）
 *
 * @note 覆盖寄存器：STATUS.FIFO_COUNT
 */
uint32_t sdmmc_get_fifo_count(void)
{
    return ((SDMMC->STATUS >> SDMMC_STATUS_FIFO_COUNT_POS) & 0x1FFFU);
}

/**
 * @brief 获取 IDMA 状态
 *
 * @return IDSTS 寄存器原始值
 *
 * @note 覆盖寄存器：IDSTS
 */
uint32_t sdmmc_dma_get_status(void)
{
    return SDMMC->IDSTS;
}

/**
 * @brief 获取当前描述符地址
 *
 * @return 当前描述符地址
 *
 * @note 覆盖寄存器：DSCADDR
 */
uint32_t sdmmc_dma_get_current_desc(void)
{
    return SDMMC->DSCADDR;
}

/**
 * @brief 获取当前缓冲区地址
 *
 * @return 当前缓冲区地址
 *
 * @note 覆盖寄存器：BUFADDR
 */
uint32_t sdmmc_dma_get_current_buffer(void)
{
    return SDMMC->BUFADDR;
}

/**
 * @brief 获取命令响应
 *
 * @param[in]  index  响应索引（0-3）
 * @return 响应寄存器值
 *
 * @note 覆盖寄存器：RESP0-3
 * @note 短响应使用 RESP0，长响应使用 RESP0-3
 */
uint32_t sdmmc_get_response(uint8_t index)
{
    uint32_t resp = 0U;
    switch (index) {
        case 0: resp = SDMMC->RESP0; break;
        case 1: resp = SDMMC->RESP1; break;
        case 2: resp = SDMMC->RESP2; break;
        case 3: resp = SDMMC->RESP3; break;
        default: break;
    }
    return resp;
}

//===========================================
// 通用卡命令层
//===========================================

/**
 * @brief 等待命令完成（内部辅助）
 */
static bool sdmmc_wait_cmd_done(uint32_t timeout_ms)
{
    sdmmc_irq_t irq;

    if (!sdmmc_wait_bit_set(&SDMMC->RINTSTS, SDMMC_IRQ_CDONE, timeout_ms)) {
        return false;
    }

    irq = sdmmc_get_raw_irq_status();

    /* 检查是否有错误 */
    if ((irq & SDMMC_IRQ_ERROR) != 0U) {
        sdmmc_irq_clear(irq);
        return false;
    }

    /* 清除 CDONE */
    sdmmc_irq_clear(SDMMC_IRQ_CDONE);
    return true;
}

/**
 * @brief 发送 SD/MMC 命令（核心 API）
 *
 * @param[in]  cmd_index   命令索引（0-63）
 * @param[in]  cmd_arg     命令参数（32 位）
 * @param[in]  resp_type   响应类型
 * @param[in]  transfer    数据传输配置（NULL 表示无数据阶段）
 * @param[in]  timeout_ms  超时时间（毫秒）
 * @return true: 命令成功, false: 超时或错误
 *
 * @note 覆盖寄存器：CMDARG, CMD, RINTSTS, RESP0-3, BLKSIZ, BYTCNT
 * @note 自动处理：CMDARG 写入 → CMD 写入 → 等待 CDONE → 读取响应
 *
 * @code
 * uint32_t ocr;
 * sdmmc_send_cmd(8, 0x1AA, SDMMC_RESPONSE_SHORT, NULL, 1000);
 * ocr = sdmmc_get_response(0);
 * @endcode
 */
bool sdmmc_send_cmd(uint8_t cmd_index, uint32_t cmd_arg,
                    sdmmc_response_type_t resp_type,
                    const sdmmc_transfer_t *transfer,
                    uint32_t timeout_ms)
{
    uint32_t cmd_reg;

    /* 清除中断标志 */
    sdmmc_irq_clear(SDMMC_IRQ_ALL);

    /* 设置命令参数 */
    SDMMC->CMDARG = cmd_arg;

    /* 构建命令寄存器 */
    cmd_reg = (1UL << SDMMC_CMD_USE_HOLD_REG_POS)
            | (1UL << SDMMC_CMD_START_CMD_POS)
            | ((uint32_t)cmd_index << SDMMC_CMD_CMD_INDEX_POS);

    /* 配置响应 */
    switch (resp_type) {
        case SDMMC_RESPONSE_NONE:
            break;
        case SDMMC_RESPONSE_SHORT:
            cmd_reg |= (1UL << SDMMC_CMD_RESP_EXPECT_POS);
            cmd_reg |= (1UL << SDMMC_CMD_CHECK_RESP_CRC_POS);
            break;
        case SDMMC_RESPONSE_LONG:
            cmd_reg |= (1UL << SDMMC_CMD_RESP_EXPECT_POS);
            cmd_reg |= (1UL << SDMMC_CMD_RESP_LENGTH_POS);
            cmd_reg |= (1UL << SDMMC_CMD_CHECK_RESP_CRC_POS);
            break;
        case SDMMC_RESPONSE_SHORT_BUSY:
            cmd_reg |= (1UL << SDMMC_CMD_RESP_EXPECT_POS);
            cmd_reg |= (1UL << SDMMC_CMD_CHECK_RESP_CRC_POS);
            cmd_reg |= (1UL << SDMMC_CMD_WAIT_PRVDATA_POS);
            break;
    }

    /* 配置数据传输 */
    if (transfer != NULL) {
        cmd_reg |= (1UL << SDMMC_CMD_DATA_EXPECTED_POS);
        if (transfer->dir == SDMMC_TRANSFER_DIR_WRITE) {
            cmd_reg |= (1UL << SDMMC_CMD_READ_WRITE_POS);
        }
        if (transfer->block_count > 1U) {
            cmd_reg |= (1UL << SDMMC_CMD_TRANSFER_MODE_POS);
            cmd_reg |= (1UL << SDMMC_CMD_SEND_AUTO_STOP_POS);
        }

        SDMMC->BLKSIZ = transfer->block_size;
        SDMMC->BYTCNT = transfer->block_size * transfer->block_count;

        /* 保存传输上下文供中断处理使用 */
        s_sdmmc_state.active_transfer = *transfer;
        s_sdmmc_state.transfer_done = false;
        s_sdmmc_state.transfer_success = true;

        /* 中断模式：使能数据传输和错误中断 */
        if (transfer->mode == SDMMC_TRANSFER_MODE_INTERRUPT) {
            sdmmc_irq_enable(SDMMC_IRQ_TXDR | SDMMC_IRQ_RXDR
                           | SDMMC_IRQ_DTO | SDMMC_IRQ_DCRC
                           | SDMMC_IRQ_DRTO | SDMMC_IRQ_EBE
                           | SDMMC_IRQ_SBE);
            if (transfer->dir == SDMMC_TRANSFER_DIR_READ) {
                /* 读传输：先使能 RXDR 中断，等 RX FIFO 有数据时触发 */
                /* RXDR 在使能后硬件自动触发 */
            }
        }

        /* IDMA 模式：启动 IDMAC */
        if (transfer->mode == SDMMC_TRANSFER_MODE_IDMA) {
            sdmmc_dma_start();
        }
    }

    /* 发送命令 */
    SDMMC->CMD = cmd_reg;

    /* 根据传输模式选择等待方式 */
    if ((transfer != NULL) && (transfer->mode != SDMMC_TRANSFER_MODE_POLLING)) {
        /* 中断/IDMA 模式：仅等待命令完成，数据传输由 ISR 或 IDMAC 处理 */
        return sdmmc_wait_cmd_done(timeout_ms);
    }

    /* 轮询模式：等待命令完成 */
    return sdmmc_wait_cmd_done(timeout_ms);
}

/**
 * @brief 执行 FIFO 轮询数据传输（内部辅助）
 */
static bool sdmmc_do_polling_transfer(const sdmmc_transfer_t *transfer)
{
    uint32_t total_words = transfer->block_size * transfer->block_count / 4U;
    uint32_t words_done = 0U;

    if (transfer->dir == SDMMC_TRANSFER_DIR_READ) {
        while (words_done < total_words) {
            /* 等待 RX FIFO 有数据 */
            uint32_t start = system_get_tick();
            while (sdmmc_is_fifo_empty()) {
                if (system_elapsed(start, SDMMC_DEFAULT_TIMEOUT_MS)) {
                    return false;
                }
            }
            /* 一次读取最多 FIFO 深度个字 */
            uint32_t fifo_count = sdmmc_get_fifo_count();
            if (fifo_count > (total_words - words_done)) {
                fifo_count = total_words - words_done;
            }
            sdmmc_read_fifo(&transfer->buffer[words_done], fifo_count);
            words_done += fifo_count;
        }
    } else {
        while (words_done < total_words) {
            /* 等待 TX FIFO 有空间 */
            uint32_t start = system_get_tick();
            while (sdmmc_is_fifo_full()) {
                if (system_elapsed(start, SDMMC_DEFAULT_TIMEOUT_MS)) {
                    return false;
                }
            }
            /* 一次写入最多 FIFO 深度个字 */
            uint32_t space = SDMMC_FIFO_DEPTH - sdmmc_get_fifo_count();
            uint32_t to_write = total_words - words_done;
            if (to_write > space) {
                to_write = space;
            }
            sdmmc_write_fifo(&transfer->buffer[words_done], to_write);
            words_done += to_write;
        }
    }
    return true;
}

/* --- 卡识别与初始化命令 --- */

bool sdmmc_cmd0_go_idle(void)
{
    return sdmmc_send_cmd(0U, 0U, SDMMC_RESPONSE_NONE, NULL, SDMMC_DEFAULT_TIMEOUT_MS);
}

bool sdmmc_cmd2_all_send_cid(uint32_t *cid)
{
    assert(cid != NULL);
    if (!sdmmc_send_cmd(2U, 0U, SDMMC_RESPONSE_LONG, NULL, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    cid[0] = SDMMC->RESP0;
    cid[1] = SDMMC->RESP1;
    cid[2] = SDMMC->RESP2;
    cid[3] = SDMMC->RESP3;
    return true;
}

bool sdmmc_cmd3_send_relative_addr(uint32_t *rca)
{
    assert(rca != NULL);
    if (!sdmmc_send_cmd(3U, 0U, SDMMC_RESPONSE_SHORT, NULL, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    *rca = SDMMC->RESP0;
    return true;
}

bool sdmmc_cmd7_select_card(uint32_t rca)
{
    return sdmmc_send_cmd(7U, rca, SDMMC_RESPONSE_SHORT_BUSY, NULL, SDMMC_DEFAULT_TIMEOUT_MS);
}

bool sdmmc_cmd7_deselect_card(void)
{
    return sdmmc_send_cmd(7U, 0U, SDMMC_RESPONSE_NONE, NULL, SDMMC_DEFAULT_TIMEOUT_MS);
}

/* --- 卡信息查询命令 --- */

bool sdmmc_cmd8_send_if_cond(uint8_t vhs, uint8_t pattern, uint32_t *response)
{
    uint32_t arg = 0x00000100U | ((uint32_t)vhs << 8) | (uint32_t)pattern;
    assert(response != NULL);
    if (!sdmmc_send_cmd(8U, arg, SDMMC_RESPONSE_SHORT, NULL, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    *response = SDMMC->RESP0;
    return true;
}

bool sdmmc_cmd9_send_csd(uint32_t *csd)
{
    assert(csd != NULL);
    if (!sdmmc_send_cmd(9U, s_sdmmc_state.card.rca, SDMMC_RESPONSE_LONG,
                        NULL, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    csd[0] = SDMMC->RESP0;
    csd[1] = SDMMC->RESP1;
    csd[2] = SDMMC->RESP2;
    csd[3] = SDMMC->RESP3;
    return true;
}

bool sdmmc_cmd10_send_cid(uint32_t rca, uint32_t *cid)
{
    assert(cid != NULL);
    if (!sdmmc_send_cmd(10U, rca, SDMMC_RESPONSE_LONG, NULL, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    cid[0] = SDMMC->RESP0;
    cid[1] = SDMMC->RESP1;
    cid[2] = SDMMC->RESP2;
    cid[3] = SDMMC->RESP3;
    return true;
}

bool sdmmc_cmd13_send_status(uint32_t rca, uint32_t *status)
{
    assert(status != NULL);
    if (!sdmmc_send_cmd(13U, rca, SDMMC_RESPONSE_SHORT, NULL, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    *status = SDMMC->RESP0;
    return true;
}

/* --- 数据传输命令 --- */

bool sdmmc_cmd17_read_single_block(uint32_t addr, uint32_t *buffer, uint32_t block_size)
{
    sdmmc_transfer_t transfer = {
        .mode = SDMMC_TRANSFER_MODE_POLLING,
        .dir = SDMMC_TRANSFER_DIR_READ,
        .buffer = buffer,
        .block_size = block_size,
        .block_count = 1U,
    };
    assert(buffer != NULL);
    if (!sdmmc_send_cmd(17U, addr, SDMMC_RESPONSE_SHORT, &transfer, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    return sdmmc_do_polling_transfer(&transfer);
}

bool sdmmc_cmd18_read_multiple_block(uint32_t addr, uint32_t *buffer,
                                     uint32_t block_size, uint32_t block_count)
{
    sdmmc_transfer_t transfer = {
        .mode = SDMMC_TRANSFER_MODE_POLLING,
        .dir = SDMMC_TRANSFER_DIR_READ,
        .buffer = buffer,
        .block_size = block_size,
        .block_count = block_count,
    };
    assert(buffer != NULL);
    if (!sdmmc_send_cmd(18U, addr, SDMMC_RESPONSE_SHORT, &transfer, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    return sdmmc_do_polling_transfer(&transfer);
}

bool sdmmc_cmd19_send_tuning_block(uint32_t *buffer)
{
    sdmmc_transfer_t transfer = {
        .mode = SDMMC_TRANSFER_MODE_POLLING,
        .dir = SDMMC_TRANSFER_DIR_READ,
        .buffer = buffer,
        .block_size = 64U,
        .block_count = 1U,
    };
    assert(buffer != NULL);
    if (!sdmmc_send_cmd(19U, 0U, SDMMC_RESPONSE_SHORT, &transfer, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    return sdmmc_do_polling_transfer(&transfer);
}

bool sdmmc_cmd24_write_single_block(uint32_t addr, const uint32_t *buffer, uint32_t block_size)
{
    sdmmc_transfer_t transfer = {
        .mode = SDMMC_TRANSFER_MODE_POLLING,
        .dir = SDMMC_TRANSFER_DIR_WRITE,
        .buffer = (uint32_t *)buffer,
        .block_size = block_size,
        .block_count = 1U,
    };
    assert(buffer != NULL);
    if (!sdmmc_send_cmd(24U, addr, SDMMC_RESPONSE_SHORT, &transfer, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    return sdmmc_do_polling_transfer(&transfer);
}

bool sdmmc_cmd25_write_multiple_block(uint32_t addr, const uint32_t *buffer,
                                      uint32_t block_size, uint32_t block_count)
{
    sdmmc_transfer_t transfer = {
        .mode = SDMMC_TRANSFER_MODE_POLLING,
        .dir = SDMMC_TRANSFER_DIR_WRITE,
        .buffer = (uint32_t *)buffer,
        .block_size = block_size,
        .block_count = block_count,
    };
    assert(buffer != NULL);
    if (!sdmmc_send_cmd(25U, addr, SDMMC_RESPONSE_SHORT, &transfer, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    return sdmmc_do_polling_transfer(&transfer);
}

/* --- 卡状态与控制命令 --- */

bool sdmmc_cmd12_stop_transmission(void)
{
    return sdmmc_send_cmd(12U, 0U, SDMMC_RESPONSE_SHORT_BUSY, NULL, SDMMC_DEFAULT_TIMEOUT_MS);
}

bool sdmmc_cmd15_go_inactive(uint32_t rca)
{
    return sdmmc_send_cmd(15U, rca, SDMMC_RESPONSE_NONE, NULL, SDMMC_DEFAULT_TIMEOUT_MS);
}

bool sdmmc_cmd16_set_block_len(uint32_t block_len)
{
    return sdmmc_send_cmd(16U, block_len, SDMMC_RESPONSE_SHORT, NULL, SDMMC_DEFAULT_TIMEOUT_MS);
}

/* --- 卡状态解析辅助函数 --- */

sdmmc_card_state_t sdmmc_get_card_state(uint32_t status)
{
    return (sdmmc_card_state_t)((status >> 9U) & 0xFU);
}

bool sdmmc_is_card_locked(uint32_t status)
{
    return ((status & (1UL << 25)) != 0U);
}

bool sdmmc_is_app_cmd(uint32_t status)
{
    return ((status & (1UL << 5)) != 0U);
}

bool sdmmc_is_card_ready(uint32_t status)
{
    return ((status & (1UL << 8)) != 0U);
}

//===========================================
// SD 协议层
//===========================================

/**
 * @brief SD 卡完整初始化
 *
 * @param[out] card  卡信息（CID/CSD/SCR/RCA/OCR/容量等）
 * @return true: 成功, false: 初始化失败
 *
 * @note 流程：CMD0 → CMD8 → ACMD41（轮询）→ CMD2 → CMD3 → CMD7 → ACMD6 → ACMD51
 * @note 覆盖寄存器：CMDARG, CMD, RESP0-3, BLKSIZ, BYTCNT
 */
bool sdmmc_sd_init(sdmmc_card_info_t *card)
{
    uint32_t ocr = 0U;
    uint32_t resp;
    uint32_t rca;
    uint32_t csd[4];
    uint32_t scr[2];
    bool is_hc = false;
    uint32_t timeout;

    assert(card != NULL);

    /* CMD0: 复位所有卡到 Idle */
    if (!sdmmc_cmd0_go_idle()) {
        return false;
    }

    /* CMD8: 检查接口条件（SD 2.0+） */
    if (sdmmc_cmd8_send_if_cond(1U, 0xAAU, &resp)) {
        if ((resp & 0xFFU) == 0xAAU) {
            is_hc = true;
        }
    }

    /* ACMD41: 发送操作条件（轮询直到卡就绪） */
    timeout = 1000U;
    do {
        /* CMD55: APP_CMD 前缀 */
        if (!sdmmc_send_cmd(55U, 0U, SDMMC_RESPONSE_SHORT, NULL, SDMMC_DEFAULT_TIMEOUT_MS)) {
            return false;
        }
        /* ACMD41 */
        uint32_t arg = is_hc ? 0x40FF8000U : 0x00FF8000U;
        if (!sdmmc_send_cmd(41U, arg, SDMMC_RESPONSE_SHORT, NULL, SDMMC_DEFAULT_TIMEOUT_MS)) {
            return false;
        }
        ocr = SDMMC->RESP0;
        if (--timeout == 0U) {
            return false;
        }
    } while ((ocr & (1UL << 31)) == 0U);

    /* CMD2: 获取 CID */
    if (!sdmmc_cmd2_all_send_cid(card->cid)) {
        return false;
    }

    /* CMD3: 获取 RCA */
    if (!sdmmc_cmd3_send_relative_addr(&rca)) {
        return false;
    }
    card->rca = rca;

    /* CMD7: 选择卡 */
    if (!sdmmc_cmd7_select_card(rca)) {
        return false;
    }

    /* CMD9: 获取 CSD */
    if (!sdmmc_cmd9_send_csd(csd)) {
        return false;
    }
    card->csd[0] = csd[0];
    card->csd[1] = csd[1];
    card->csd[2] = csd[2];
    card->csd[3] = csd[3];

    /* ACMD6: 切换到 4 位总线 */
    if (!sdmmc_acmd6_set_bus_width(rca, SDMMC_BUS_WIDTH_4BIT)) {
        return false;
    }

    /* ACMD51: 获取 SCR */
    if (sdmmc_acmd51_send_scr(scr)) {
        card->scr[0] = scr[0];
        card->scr[1] = scr[1];
    }

    card->card_type = SDMMC_CARD_TYPE_SD;
    card->ocr = ocr;
    card->high_capacity = is_hc;

    /* 解析 CSD 获取容量和块大小 */
    uint32_t csd_structure = (csd[3] >> 30U) & 0x3U;
    if (csd_structure >= 1U) {
        /* CSD v2.0 (SDHC/SDXC) */
        uint32_t c_size = (csd[3] >> 16U) & 0x3FFFU;
        card->capacity = (c_size + 1U) * 1024U;
        card->block_size = 512U;
    } else {
        /* CSD v1.0 (SDSC) */
        uint32_t read_bl_len = (csd[2] >> 16U) & 0xFU;
        uint32_t c_size = ((csd[3] & 0x3FFU) << 2U) | ((csd[2] >> 30U) & 0x3U);
        uint32_t c_size_mult = (csd[1] >> 15U) & 0x7U;
        card->block_size = 1UL << read_bl_len;
        card->capacity = (c_size + 1U) * (1UL << (c_size_mult + 2U));
    }

    /* 保存卡信息 */
    s_sdmmc_state.card = *card;

    return true;
}

bool sdmmc_acmd6_set_bus_width(uint32_t rca, sdmmc_bus_width_t width)
{
    if (!sdmmc_send_cmd(55U, rca, SDMMC_RESPONSE_SHORT, NULL, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    return sdmmc_send_cmd(6U, (uint32_t)width, SDMMC_RESPONSE_SHORT, NULL,
                          SDMMC_DEFAULT_TIMEOUT_MS);
}

bool sdmmc_acmd13_sd_status(uint32_t *status)
{
    assert(status != NULL);
    if (!sdmmc_send_cmd(55U, s_sdmmc_state.card.rca,
                        SDMMC_RESPONSE_SHORT, NULL, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    if (!sdmmc_send_cmd(13U, 0U, SDMMC_RESPONSE_SHORT, NULL, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    *status = SDMMC->RESP0;
    return true;
}

bool sdmmc_acmd41_send_op_cond(bool hcs, uint32_t *ocr)
{
    uint32_t arg = hcs ? 0x40FF8000U : 0x00FF8000U;
    assert(ocr != NULL);
    if (!sdmmc_send_cmd(55U, 0U, SDMMC_RESPONSE_SHORT, NULL, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    if (!sdmmc_send_cmd(41U, arg, SDMMC_RESPONSE_SHORT, NULL, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    *ocr = SDMMC->RESP0;
    return true;
}

bool sdmmc_acmd51_send_scr(uint32_t *scr)
{
    sdmmc_transfer_t transfer = {
        .mode = SDMMC_TRANSFER_MODE_POLLING,
        .dir = SDMMC_TRANSFER_DIR_READ,
        .buffer = scr,
        .block_size = 8U,
        .block_count = 1U,
    };
    assert(scr != NULL);
    if (!sdmmc_send_cmd(55U, s_sdmmc_state.card.rca,
                        SDMMC_RESPONSE_SHORT, NULL, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    if (!sdmmc_send_cmd(51U, 0U, SDMMC_RESPONSE_SHORT, &transfer, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    return sdmmc_do_polling_transfer(&transfer);
}

bool sdmmc_sd_read_blocks(uint32_t start_block, uint32_t *buffer, uint32_t block_count)
{
    uint32_t addr;
    assert(buffer != NULL);
    if (s_sdmmc_state.card.high_capacity) {
        addr = start_block;
    } else {
        addr = start_block * s_sdmmc_state.card.block_size;
    }
    if (block_count == 1U) {
        return sdmmc_cmd17_read_single_block(addr, buffer, s_sdmmc_state.card.block_size);
    }
    return sdmmc_cmd18_read_multiple_block(addr, buffer, s_sdmmc_state.card.block_size,
                                           block_count);
}

bool sdmmc_sd_write_blocks(uint32_t start_block, const uint32_t *buffer, uint32_t block_count)
{
    uint32_t addr;
    assert(buffer != NULL);
    if (s_sdmmc_state.card.high_capacity) {
        addr = start_block;
    } else {
        addr = start_block * s_sdmmc_state.card.block_size;
    }
    if (block_count == 1U) {
        return sdmmc_cmd24_write_single_block(addr, buffer, s_sdmmc_state.card.block_size);
    }
    return sdmmc_cmd25_write_multiple_block(addr, buffer, s_sdmmc_state.card.block_size,
                                            block_count);
}

bool sdmmc_sd_erase(uint32_t start_block, uint32_t end_block)
{
    uint32_t start_addr, end_addr;
    if (s_sdmmc_state.card.high_capacity) {
        start_addr = start_block;
        end_addr = end_block;
    } else {
        start_addr = start_block * s_sdmmc_state.card.block_size;
        end_addr = end_block * s_sdmmc_state.card.block_size;
    }
    /* CMD32: 擦除起始地址 */
    if (!sdmmc_send_cmd(32U, start_addr, SDMMC_RESPONSE_SHORT, NULL, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    /* CMD33: 擦除结束地址 */
    if (!sdmmc_send_cmd(33U, end_addr, SDMMC_RESPONSE_SHORT, NULL, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    /* CMD38: 执行擦除 */
    return sdmmc_send_cmd(38U, 0U, SDMMC_RESPONSE_SHORT_BUSY, NULL, 10000U);
}

bool sdmmc_sd_get_card_info(sdmmc_card_info_t *card)
{
    assert(card != NULL);
    *card = s_sdmmc_state.card;
    return s_sdmmc_state.initialized;
}

uint64_t sdmmc_sd_get_capacity(void)
{
    return (uint64_t)s_sdmmc_state.card.capacity * (uint64_t)s_sdmmc_state.card.block_size;
}

//===========================================
// SDIO 协议层
//===========================================

bool sdmmc_sdio_init(sdmmc_card_info_t *card)
{
    uint32_t resp;
    uint32_t rca;

    assert(card != NULL);

    /* CMD5: IO_SEND_OP_COND */
    if (!sdmmc_send_cmd(5U, 0U, SDMMC_RESPONSE_SHORT, NULL, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    resp = SDMMC->RESP0;

    /* CMD3: 获取 RCA */
    if (!sdmmc_cmd3_send_relative_addr(&rca)) {
        return false;
    }

    /* CMD7: 选择卡 */
    if (!sdmmc_cmd7_select_card(rca)) {
        return false;
    }

    /* CMD52: 读取 CCCR 确认 SDIO 版本 */
    uint8_t ccrc_data;
    if (!sdmmc_sdio_read_direct(0U, 0x00U, &ccrc_data)) {
        return false;
    }

    card->card_type = SDMMC_CARD_TYPE_SDIO;
    card->rca = rca;
    card->ocr = resp;

    s_sdmmc_state.card = *card;
    return true;
}

bool sdmmc_sdio_read_direct(uint8_t func, uint32_t addr, uint8_t *data)
{
    uint32_t arg = ((uint32_t)func << 28U) | ((addr & 0x1FFFFU) << 9U) | (1UL << 31);
    assert(data != NULL);
    if (!sdmmc_send_cmd(52U, arg, SDMMC_RESPONSE_SHORT, NULL, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    *data = (uint8_t)(SDMMC->RESP0 & 0xFFU);
    return true;
}

bool sdmmc_sdio_write_direct(uint8_t func, uint32_t addr, uint8_t data, uint8_t *resp)
{
    uint32_t arg = ((uint32_t)func << 28U) | ((addr & 0x1FFFFU) << 9U)
                 | (1UL << 31) | ((uint32_t)data & 0xFFU);
    if (!sdmmc_send_cmd(52U, arg, SDMMC_RESPONSE_SHORT, NULL, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    if (resp != NULL) {
        *resp = (uint8_t)(SDMMC->RESP0 & 0xFFU);
    }
    return true;
}

bool sdmmc_sdio_read_extended(uint8_t func, uint32_t addr,
                              bool block_mode, bool op_code,
                              uint32_t *buffer, uint32_t count)
{
    uint32_t arg = ((uint32_t)func << 28U) | ((addr & 0x1FFFFU) << 9U) | (1UL << 31);
    sdmmc_transfer_t transfer;
    assert(buffer != NULL);

    if (count == 0U) {
        return false;
    }

    if (block_mode) {
        arg |= (1UL << 27);
        transfer.block_size = count;
        transfer.block_count = 1U;
    } else {
        transfer.block_size = count;
        transfer.block_count = 1U;
    }
    if (op_code) {
        arg |= (1UL << 26);
    }

    transfer.mode = SDMMC_TRANSFER_MODE_POLLING;
    transfer.dir = SDMMC_TRANSFER_DIR_READ;
    transfer.buffer = buffer;

    if (!sdmmc_send_cmd(53U, arg, SDMMC_RESPONSE_SHORT, &transfer, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    return sdmmc_do_polling_transfer(&transfer);
}

bool sdmmc_sdio_write_extended(uint8_t func, uint32_t addr,
                               bool block_mode, bool op_code,
                               const uint32_t *buffer, uint32_t count)
{
    uint32_t arg = ((uint32_t)func << 28U) | ((addr & 0x1FFFFU) << 9U) | (1UL << 31);
    sdmmc_transfer_t transfer;
    assert(buffer != NULL);

    if (count == 0U) {
        return false;
    }

    if (block_mode) {
        arg |= (1UL << 27);
        transfer.block_size = count;
        transfer.block_count = 1U;
    } else {
        transfer.block_size = count;
        transfer.block_count = 1U;
    }
    if (op_code) {
        arg |= (1UL << 26);
    }

    transfer.mode = SDMMC_TRANSFER_MODE_POLLING;
    transfer.dir = SDMMC_TRANSFER_DIR_WRITE;
    transfer.buffer = (uint32_t *)buffer;

    if (!sdmmc_send_cmd(53U, arg, SDMMC_RESPONSE_SHORT, &transfer, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    return sdmmc_do_polling_transfer(&transfer);
}

bool sdmmc_sdio_read_byte(uint8_t func, uint32_t addr, uint8_t *data)
{
    return sdmmc_sdio_read_direct(func, addr, data);
}

bool sdmmc_sdio_write_byte(uint8_t func, uint32_t addr, uint8_t data)
{
    return sdmmc_sdio_write_direct(func, addr, data, NULL);
}

bool sdmmc_sdio_read_bytes(uint8_t func, uint32_t addr, uint8_t *data, uint32_t len)
{
    uint32_t words = (len + 3U) / 4U;
    uint32_t *buf = (uint32_t *)data;
    return sdmmc_sdio_read_extended(func, addr, false, true, buf, len);
}

bool sdmmc_sdio_write_bytes(uint8_t func, uint32_t addr, const uint8_t *data, uint32_t len)
{
    uint32_t words = (len + 3U) / 4U;
    uint32_t *buf = (uint32_t *)data;
    return sdmmc_sdio_write_extended(func, addr, false, true, buf, len);
}

bool sdmmc_sdio_enable_irq(void)
{
    sdmmc_irq_enable(SDMMC_IRQ_SDIO0);
    return true;
}

bool sdmmc_sdio_disable_irq(void)
{
    sdmmc_irq_disable(SDMMC_IRQ_SDIO0);
    return true;
}

//===========================================
// MMC 协议层
//===========================================

bool sdmmc_mmc_init(sdmmc_card_info_t *card)
{
    uint32_t ocr = 0U;
    uint32_t rca;
    uint32_t csd[4];
    uint32_t timeout;

    assert(card != NULL);

    /* CMD0: 复位 */
    if (!sdmmc_cmd0_go_idle()) {
        return false;
    }

    /* CMD1: 发送操作条件（轮询直到卡就绪） */
    timeout = 1000U;
    do {
        if (!sdmmc_send_cmd(1U, 0x40FF8000U, SDMMC_RESPONSE_SHORT, NULL,
                            SDMMC_DEFAULT_TIMEOUT_MS)) {
            return false;
        }
        ocr = SDMMC->RESP0;
        if (--timeout == 0U) {
            return false;
        }
    } while ((ocr & (1UL << 31)) == 0U);

    /* CMD2: 获取 CID */
    if (!sdmmc_cmd2_all_send_cid(card->cid)) {
        return false;
    }

    /* CMD3: 获取 RCA */
    if (!sdmmc_cmd3_send_relative_addr(&rca)) {
        return false;
    }
    card->rca = rca;

    /* CMD7: 选择卡 */
    if (!sdmmc_cmd7_select_card(rca)) {
        return false;
    }

    /* CMD9: 获取 CSD */
    if (!sdmmc_cmd9_send_csd(csd)) {
        return false;
    }
    card->csd[0] = csd[0];
    card->csd[1] = csd[1];
    card->csd[2] = csd[2];
    card->csd[3] = csd[3];

    card->card_type = SDMMC_CARD_TYPE_MMC;
    card->ocr = ocr;
    card->block_size = 512U;

    /* 解析 CSD 获取容量 */
    uint32_t csd_structure = (csd[3] >> 30U) & 0x3U;
    if (csd_structure >= 2U) {
        uint32_t c_size = (csd[3] >> 16U) & 0x3FFFU;
        card->capacity = (c_size + 1U) * 1024U;
    } else {
        uint32_t read_bl_len = (csd[2] >> 16U) & 0xFU;
        uint32_t c_size = ((csd[3] & 0x3FFU) << 2U) | ((csd[2] >> 30U) & 0x3U);
        uint32_t c_size_mult = (csd[1] >> 15U) & 0x7U;
        card->block_size = 1UL << read_bl_len;
        card->capacity = (c_size + 1U) * (1UL << (c_size_mult + 2U));
    }

    s_sdmmc_state.card = *card;
    return true;
}

bool sdmmc_cmd1_send_op_cond(uint32_t *ocr)
{
    assert(ocr != NULL);
    if (!sdmmc_send_cmd(1U, 0x40FF8000U, SDMMC_RESPONSE_SHORT, NULL,
                        SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    *ocr = SDMMC->RESP0;
    return true;
}

bool sdmmc_cmd6_switch(uint8_t index, uint8_t value, uint32_t *status)
{
    uint32_t arg = ((uint32_t)SDMMC_MMC_ACCESS_WRITE_BYTE << 24U)
                 | ((uint32_t)index << 16U)
                 | ((uint32_t)value << 8U);
    assert(status != NULL);
    if (!sdmmc_send_cmd(6U, arg, SDMMC_RESPONSE_SHORT_BUSY, NULL, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    *status = SDMMC->RESP0;
    return true;
}

bool sdmmc_mmc_read_blocks(uint32_t start_block, uint32_t *buffer, uint32_t block_count)
{
    assert(buffer != NULL);
    if (block_count == 1U) {
        return sdmmc_cmd17_read_single_block(start_block, buffer,
                                             s_sdmmc_state.card.block_size);
    }
    return sdmmc_cmd18_read_multiple_block(start_block, buffer,
                                           s_sdmmc_state.card.block_size, block_count);
}

bool sdmmc_mmc_write_blocks(uint32_t start_block, const uint32_t *buffer, uint32_t block_count)
{
    assert(buffer != NULL);
    if (block_count == 1U) {
        return sdmmc_cmd24_write_single_block(start_block, buffer,
                                              s_sdmmc_state.card.block_size);
    }
    return sdmmc_cmd25_write_multiple_block(start_block, buffer,
                                            s_sdmmc_state.card.block_size, block_count);
}

bool sdmmc_mmc_erase(uint32_t start_block, uint32_t end_block)
{
    /* CMD35: 擦除组起始 */
    if (!sdmmc_send_cmd(35U, start_block, SDMMC_RESPONSE_SHORT, NULL,
                        SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    /* CMD36: 擦除组结束 */
    if (!sdmmc_send_cmd(36U, end_block, SDMMC_RESPONSE_SHORT, NULL,
                        SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    /* CMD38: 执行擦除 */
    return sdmmc_send_cmd(38U, 0U, SDMMC_RESPONSE_SHORT_BUSY, NULL, 10000U);
}

bool sdmmc_emmc_set_ddr(bool enable)
{
    uint32_t status;
    uint8_t value = enable ? 0x04U : 0x00U;
    return sdmmc_cmd6_switch(185U, value, &status);
}

bool sdmmc_emmc_set_hs200(void)
{
    uint32_t status;
    return sdmmc_cmd6_switch(185U, 0x02U, &status);
}

//===========================================
// CE-ATA 协议层
//===========================================

bool sdmmc_ceata_init(sdmmc_card_info_t *card)
{
    uint32_t rca;

    assert(card != NULL);

    /* CE-ATA 设备响应 CMD0 */
    if (!sdmmc_cmd0_go_idle()) {
        return false;
    }

    /* CMD3: 获取 RCA */
    if (!sdmmc_cmd3_send_relative_addr(&rca)) {
        return false;
    }

    /* CMD7: 选择卡 */
    if (!sdmmc_cmd7_select_card(rca)) {
        return false;
    }

    card->card_type = SDMMC_CARD_TYPE_CEATA;
    card->rca = rca;

    s_sdmmc_state.card = *card;
    return true;
}

bool sdmmc_ceata_send_command(uint16_t cmd, uint8_t features,
                              uint32_t sectors, uint8_t device)
{
    uint32_t arg = ((uint32_t)cmd << 16U) | ((uint32_t)features) | ((uint32_t)device << 4U);
    sdmmc_transfer_t transfer = {
        .mode = SDMMC_TRANSFER_MODE_POLLING,
        .dir = SDMMC_TRANSFER_DIR_NONE,
        .buffer = NULL,
        .block_size = 512U,
        .block_count = sectors,
    };

    /* 使能 CE-ATA 模式 */
    SDMMC->CTRL |= (1UL << SDMMC_CTRL_CEATA_DEV_INT_POS);

    return sdmmc_send_cmd(60U, arg, SDMMC_RESPONSE_SHORT,
                          (sectors > 0U) ? &transfer : NULL, SDMMC_DEFAULT_TIMEOUT_MS);
}

bool sdmmc_ceata_read_sectors(uint32_t lba, uint32_t *buffer, uint32_t sector_count)
{
    sdmmc_transfer_t transfer = {
        .mode = SDMMC_TRANSFER_MODE_POLLING,
        .dir = SDMMC_TRANSFER_DIR_READ,
        .buffer = buffer,
        .block_size = 512U,
        .block_count = sector_count,
    };
    assert(buffer != NULL);
    if (!sdmmc_send_cmd(60U, lba, SDMMC_RESPONSE_SHORT, &transfer, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    return sdmmc_do_polling_transfer(&transfer);
}

bool sdmmc_ceata_write_sectors(uint32_t lba, const uint32_t *buffer, uint32_t sector_count)
{
    sdmmc_transfer_t transfer = {
        .mode = SDMMC_TRANSFER_MODE_POLLING,
        .dir = SDMMC_TRANSFER_DIR_WRITE,
        .buffer = (uint32_t *)buffer,
        .block_size = 512U,
        .block_count = sector_count,
    };
    assert(buffer != NULL);
    if (!sdmmc_send_cmd(60U, lba, SDMMC_RESPONSE_SHORT, &transfer, SDMMC_DEFAULT_TIMEOUT_MS)) {
        return false;
    }
    return sdmmc_do_polling_transfer(&transfer);
}

//===========================================
// 中断服务函数
//===========================================

/**
 * @brief SDMMC 中断服务函数
 *
 * @note 每个中断源独立派发到对应的回调函数（通过 sdmmc_irq_callbacks_t 注册）
 * @note 回调为 NULL 的中断源自动跳过，不做任何处理
 * @note 错误中断（RTO/DTO/DCRC/EBE/SBE/HLE）在派发前自动做硬件清理
 * @note 传输中断（TXDR/RXDR）仅在内核主动传输模式下由 ISR 自动填充/读取 FIFO
 */
void SDMMC_IRQHandler(void)
{
    sdmmc_irq_t masked_irq = sdmmc_get_masked_irq_status();
    const sdmmc_irq_callbacks_t *cb = &s_sdmmc_state.irq_cbs;

    if (masked_irq == 0U) {
        return;
    }

    /* ---- 卡检测 ---- */
    if ((masked_irq & SDMMC_IRQ_CDET) != 0U) {
        if (cb->card_detect != NULL) { cb->card_detect(); }
        sdmmc_irq_clear(SDMMC_IRQ_CDET);
    }

    /* ---- 命令完成 ---- */
    if ((masked_irq & SDMMC_IRQ_CDONE) != 0U) {
        if (cb->cmd_done != NULL) { cb->cmd_done(); }
        sdmmc_irq_clear(SDMMC_IRQ_CDONE);
    }

    /* ---- 响应超时 ---- */
    if ((masked_irq & SDMMC_IRQ_RTO) != 0U) {
        if (cb->resp_timeout != NULL) { cb->resp_timeout(); }
        sdmmc_irq_clear(SDMMC_IRQ_RTO);
    }

    /* ---- 响应 CRC 错误 ---- */
    if ((masked_irq & SDMMC_IRQ_RCRC) != 0U) {
        if (cb->resp_crc_error != NULL) { cb->resp_crc_error(); }
        sdmmc_irq_clear(SDMMC_IRQ_RCRC);
    }

    /* ---- 数据超时 ---- */
    if ((masked_irq & SDMMC_IRQ_DTO) != 0U) {
        sdmmc_reset_fifo();
        s_sdmmc_state.transfer_done = true;
        s_sdmmc_state.transfer_success = false;
        if (cb->data_timeout != NULL) { cb->data_timeout(); }
        sdmmc_irq_clear(SDMMC_IRQ_DTO);
    }

    /* ---- 数据 CRC 错误 ---- */
    if ((masked_irq & SDMMC_IRQ_DCRC) != 0U) {
        sdmmc_reset_fifo();
        s_sdmmc_state.transfer_done = true;
        s_sdmmc_state.transfer_success = false;
        if (cb->data_crc_error != NULL) { cb->data_crc_error(); }
        sdmmc_irq_clear(SDMMC_IRQ_DCRC);
    }

    /* ---- 数据读超时 ---- */
    if ((masked_irq & SDMMC_IRQ_DRTO) != 0U) {
        sdmmc_reset_fifo();
        if (cb->data_read_timeout != NULL) { cb->data_read_timeout(); }
        sdmmc_irq_clear(SDMMC_IRQ_DRTO);
    }

    /* ---- 数据饥饿超时 ---- */
    if ((masked_irq & SDMMC_IRQ_HTO) != 0U) {
        if (cb->data_starve_timeout != NULL) { cb->data_starve_timeout(); }
        sdmmc_irq_clear(SDMMC_IRQ_HTO);
    }

    /* ---- 起始位错误 ---- */
    if ((masked_irq & SDMMC_IRQ_SBE) != 0U) {
        sdmmc_reset_fifo();
        if (cb->start_bit_error != NULL) { cb->start_bit_error(); }
        sdmmc_irq_clear(SDMMC_IRQ_SBE);
    }

    /* ---- 结束位错误 ---- */
    if ((masked_irq & SDMMC_IRQ_EBE) != 0U) {
        sdmmc_reset_fifo();
        if (cb->end_bit_error != NULL) { cb->end_bit_error(); }
        sdmmc_irq_clear(SDMMC_IRQ_EBE);
    }

    /* ---- 硬件锁定错误 ---- */
    if ((masked_irq & SDMMC_IRQ_HLE) != 0U) {
        sdmmc_reset_controller();
        if (cb->hw_lock_error != NULL) { cb->hw_lock_error(); }
        sdmmc_irq_clear(SDMMC_IRQ_HLE);
    }

    /* ---- FIFO 运行中 ---- */
    if ((masked_irq & SDMMC_IRQ_FRUN) != 0U) {
        if (cb->fifo_run != NULL) { cb->fifo_run(); }
        sdmmc_irq_clear(SDMMC_IRQ_FRUN);
    }

    /* ---- 自动命令完成 ---- */
    if ((masked_irq & SDMMC_IRQ_ACD) != 0U) {
        if (cb->auto_cmd_done != NULL) { cb->auto_cmd_done(); }
        sdmmc_irq_clear(SDMMC_IRQ_ACD);
    }

    /* ---- SDIO 卡中断 0 ---- */
    if ((masked_irq & SDMMC_IRQ_SDIO0) != 0U) {
        if (cb->sdio_interrupt0 != NULL) { cb->sdio_interrupt0(); }
        sdmmc_irq_clear(SDMMC_IRQ_SDIO0);
    }

    /* ---- SDIO 卡中断 1 ---- */
    if ((masked_irq & SDMMC_IRQ_SDIO1) != 0U) {
        if (cb->sdio_interrupt1 != NULL) { cb->sdio_interrupt1(); }
        sdmmc_irq_clear(SDMMC_IRQ_SDIO1);
    }

    /* ---- TX FIFO 数据请求（内核自动处理传输）---- */
    if ((masked_irq & SDMMC_IRQ_TXDR) != 0U) {
        if (cb->tx_data_request != NULL) {
            cb->tx_data_request();
        } else if ((s_sdmmc_state.active_transfer.buffer != NULL) &&
                   (s_sdmmc_state.active_transfer.dir == SDMMC_TRANSFER_DIR_WRITE)) {
            /* 无用户回调时，ISR 自动从 active_transfer.buffer 填充 FIFO */
            uint32_t space = SDMMC_FIFO_DEPTH - sdmmc_get_fifo_count();
            if (space > 0U) {
                sdmmc_write_fifo(s_sdmmc_state.active_transfer.buffer, space);
            }
        }
        sdmmc_irq_clear(SDMMC_IRQ_TXDR);
    }

    /* ---- RX FIFO 数据请求（内核自动处理传输）---- */
    if ((masked_irq & SDMMC_IRQ_RXDR) != 0U) {
        if (cb->rx_data_request != NULL) {
            cb->rx_data_request();
        } else if ((s_sdmmc_state.active_transfer.buffer != NULL) &&
                   (s_sdmmc_state.active_transfer.dir == SDMMC_TRANSFER_DIR_READ)) {
            /* 无用户回调时，ISR 自动将 FIFO 数据读取到 active_transfer.buffer */
            uint32_t fifo_count = sdmmc_get_fifo_count();
            if (fifo_count > 0U) {
                sdmmc_read_fifo(s_sdmmc_state.active_transfer.buffer, fifo_count);
            }
        }
        sdmmc_irq_clear(SDMMC_IRQ_RXDR);
    }
}
