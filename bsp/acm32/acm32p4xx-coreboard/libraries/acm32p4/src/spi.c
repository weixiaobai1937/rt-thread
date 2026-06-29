/**
 * @file spi.c
 * @brief ACM32P4 SPI 驱动实现
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#include "hardware/spi.h"
#include "hardware/clocks.h"
#include "hardware/system.h"
#include "device/acm32p4xx.h"
#include <assert.h>

//===========================================
// 内部宏定义
//===========================================

#define SPI_CTL_MST_MODE_MSK        (1UL << 0)
#define SPI_CTL_SFILTER_MSK         (1UL << 1)
#define SPI_CTL_CPHA_MSK            (1UL << 2)
#define SPI_CTL_CPOL_MSK            (1UL << 3)
#define SPI_CTL_LSB_FIRST_MSK       (1UL << 4)
#define SPI_CTL_X_MODE_POS          5U
#define SPI_CTL_X_MODE_MSK          (0x3UL << SPI_CTL_X_MODE_POS)
#define SPI_CTL_IO_MODE_MSK         (1UL << 7)
#define SPI_CTL_SLAVE_EN_MSK        (1UL << 8)
#define SPI_CTL_CS_RST_MSK          (1UL << 9)
#define SPI_CTL_CS_FILTER_MSK       (1UL << 10)
#define SPI_CTL_CS_TIME_POS         11U
#define SPI_CTL_CS_TIME_MSK         (0xFFUL << SPI_CTL_CS_TIME_POS)
#define SPI_CTL_SWCS_MSK            (1UL << 20)
#define SPI_CTL_SWCS_EN_MSK         (1UL << 21)
#define SPI_CTL_SID_EN_MSK          (1UL << 22)

#define SPI_TX_CTL_EN_MSK           (1UL << 0)
#define SPI_TX_CTL_FIFO_RESET_MSK   (1UL << 1)
#define SPI_TX_CTL_MODE_MSK         (1UL << 2)
#define SPI_TX_CTL_DMA_REQ_EN_MSK   (1UL << 3)
#define SPI_TX_CTL_DMA_LEVEL_POS    4U
#define SPI_TX_CTL_DMA_LEVEL_MSK    (0xFUL << SPI_TX_CTL_DMA_LEVEL_POS)
#define SPI_TX_CTL_DUMMY_POS        8U
#define SPI_TX_CTL_DUMMY_MSK        (0xFFUL << SPI_TX_CTL_DUMMY_POS)

#define SPI_RX_CTL_EN_MSK           (1UL << 0)
#define SPI_RX_CTL_FIFO_RESET_MSK   (1UL << 1)
#define SPI_RX_CTL_DMA_REQ_EN_MSK   (1UL << 3)
#define SPI_RX_CTL_DMA_LEVEL_POS    4U
#define SPI_RX_CTL_DMA_LEVEL_MSK    (0xFUL << SPI_RX_CTL_DMA_LEVEL_POS)
#define SPI_RX_CTL_SSHIFT1_POS      8U
#define SPI_RX_CTL_SSHIFT1_MSK      (0x3UL << SPI_RX_CTL_SSHIFT1_POS)
#define SPI_RX_CTL_MSDA_EN_MSK      (1UL << 10)
#define SPI_RX_CTL_MSDA_X0_MSK      (1UL << 11)
#define SPI_RX_CTL_MSDA_X1_MSK      (1UL << 12)
#define SPI_RX_CTL_MSDA_X2_MSK      (1UL << 13)
#define SPI_RX_CTL_MSDA_X3_MSK      (1UL << 14)
#define SPI_RX_CTL_MSDHA_X0_MSK     (1UL << 16)
#define SPI_RX_CTL_MSDHA_X1_MSK     (1UL << 17)
#define SPI_RX_CTL_MSDHA_X2_MSK     (1UL << 18)
#define SPI_RX_CTL_MSDHA_X3_MSK     (1UL << 19)
#define SPI_RX_CTL_SSHIFT2_MSK      (1UL << 24)

#define SPI_CS_MASK                 0x1FUL
#define SPI_CSMAP_EN_MSK            (1UL << 8)

#define SPI_OUT_EN_MASK             0x0FUL

#define SPI_MEM_ACC_ACC_EN_MSK              (1UL << 0)
#define SPI_MEM_ACC_CS_TOUT_EN_MSK          (1UL << 1)
#define SPI_MEM_ACC_CON_MODE_EN_MSK         (1UL << 3)
#define SPI_MEM_ACC_SEND_INSTR_ONCE_EN_MSK  (1UL << 4)
#define SPI_MEM_ACC_WR_AB_EN_MSK            (1UL << 5)
#define SPI_MEM_ACC_RD_AB_EN_MSK            (1UL << 6)
#define SPI_MEM_ACC_ALTER_BYTE_SIZE_POS     7U
#define SPI_MEM_ACC_ALTER_BYTE_SIZE_MSK     (0x3UL << SPI_MEM_ACC_ALTER_BYTE_SIZE_POS)
#define SPI_MEM_ACC_WR_DB_EN_MSK            (1UL << 10)
#define SPI_MEM_ACC_RD_DB_EN_MSK            (1UL << 11)
#define SPI_MEM_ACC_DUMMY_CYCLE_SIZE_POS    12U
#define SPI_MEM_ACC_DUMMY_CYCLE_SIZE_MSK    (0x7UL << SPI_MEM_ACC_DUMMY_CYCLE_SIZE_POS)
#define SPI_MEM_ACC_ADDR_WIDTH_POS          17U
#define SPI_MEM_ACC_ADDR_WIDTH_MSK          (0x3UL << SPI_MEM_ACC_ADDR_WIDTH_POS)
#define SPI_MEM_ACC_INSTR_MODE_POS          19U
#define SPI_MEM_ACC_INSTR_MODE_MSK          (0x3UL << SPI_MEM_ACC_INSTR_MODE_POS)
#define SPI_MEM_ACC_ADDR_MODE_POS           21U
#define SPI_MEM_ACC_ADDR_MODE_MSK           (0x3UL << SPI_MEM_ACC_ADDR_MODE_POS)
#define SPI_MEM_ACC_ALTER_BYTE_MODE_POS     23U
#define SPI_MEM_ACC_ALTER_BYTE_MODE_MSK     (0x3UL << SPI_MEM_ACC_ALTER_BYTE_MODE_POS)
#define SPI_MEM_ACC_DATA_MODE_POS           25U
#define SPI_MEM_ACC_DATA_MODE_MSK           (0x3UL << SPI_MEM_ACC_DATA_MODE_POS)
#define SPI_MEM_ACC_ONCE_INSTR_CLR_MSK      (1UL << 27)

#define SPI_STATUS_W1C_MASK         ((uint32_t)SPI_FLAG_BATCH_DONE             | \
                                     (uint32_t)SPI_FLAG_TX_FIFO_EMPTY          | \
                                     (uint32_t)SPI_FLAG_TX_FIFO_FULL           | \
                                     (uint32_t)SPI_FLAG_RX_FIFO_EMPTY          | \
                                     (uint32_t)SPI_FLAG_RX_FIFO_FULL           | \
                                     (uint32_t)SPI_FLAG_TX_FIFO_HALF_EMPTY     | \
                                     (uint32_t)SPI_FLAG_TX_FIFO_HALF_FULL      | \
                                     (uint32_t)SPI_FLAG_RX_FIFO_HALF_EMPTY     | \
                                     (uint32_t)SPI_FLAG_RX_FIFO_HALF_FULL      | \
                                     (uint32_t)SPI_FLAG_CS_POS                 | \
                                     (uint32_t)SPI_FLAG_RX_FIFO_NOT_EMPTY      | \
                                     (uint32_t)SPI_FLAG_RX_FIFO_EMPTY_OVERFLOW | \
                                     (uint32_t)SPI_FLAG_RX_FIFO_FULL_OVERFLOW  | \
                                     (uint32_t)SPI_FLAG_TX_BATCH_DONE          | \
                                     (uint32_t)SPI_FLAG_RX_BATCH_DONE          | \
                                     (uint32_t)SPI_FLAG_INSTR_SEND_DONE)

//===========================================
// 内部类型与状态变量
//===========================================

typedef struct {
    spi_callback_t     callback;
    spi_chip_select_t  selected_cs;
} spi_state_t;

static SPI_TypeDef * const s_spi_regs[SPI_COUNT] = {
    SPI1, SPI2, SPI3, SPI4, SPI7,
};

static const clock_periph_t s_spi_clks[SPI_COUNT] = {
    CLK_SPI1, CLK_SPI2, CLK_SPI3, CLK_SPI4, CLK_QSPI7,
};

static const IRQn_Type s_spi_irqs[SPI_COUNT] = {
    SPI1_IRQn, SPI2_IRQn, SPI3_IRQn, SPI4_IRQn, SPI7_IRQn,
};

static spi_state_t s_spi_state[SPI_COUNT] = {
    { NULL, SPI_CS_0 }, { NULL, SPI_CS_0 }, { NULL, SPI_CS_0 }, { NULL, SPI_CS_0 },
    { NULL, SPI_CS_0 },
};

//===========================================
// 内部辅助函数
//===========================================

static inline SPI_TypeDef *get_spi(spi_instance_t inst)
{
    return s_spi_regs[inst];
}

static inline bool is_valid_inst(spi_instance_t inst)
{
    return ((uint32_t)inst < (uint32_t)SPI_COUNT);
}

static uint32_t spi_get_clock_hz(void)
{
    return clock_get_hclk_hz();
}

static uint32_t normalize_timeout(uint32_t timeout_ms)
{
    if (timeout_ms == 0U) {
        return SPI_DEFAULT_TIMEOUT_MS;
    }
    return timeout_ms;
}

static bool wait_flag_state(SPI_TypeDef *spi, uint32_t mask, bool set, uint32_t timeout_ms)
{
    uint64_t start = system_get_tick();
    uint32_t timeout = normalize_timeout(timeout_ms);

    while (((spi->STATUS & mask) != 0U) != set) {
        if (system_elapsed(start, timeout)) {
            return false;
        }
    }
    return true;
}

static bool calc_baud_config(uint32_t target_hz, spi_baud_config_t *config)
{
    uint32_t hclk_hz;
    uint32_t best_hz = 0U;
    uint8_t best_div1 = 0U;
    uint8_t best_div2 = 0U;

    assert(config != NULL);

    hclk_hz = spi_get_clock_hz();
    if (target_hz == 0U || hclk_hz == 0U) {
        return false;
    }

    for (uint32_t div1 = SPI_DIV1_MIN; div1 <= SPI_DIV1_MAX; div1 += 2U) {
        uint64_t denom = (uint64_t)target_hz * div1;
        uint32_t div2;

        if (denom == 0U) {
            continue;
        }

        div2 = (uint32_t)(((uint64_t)hclk_hz + denom - 1U) / denom);
        if (div2 == 0U) {
            div2 = 1U;
        }
        div2 -= 1U;

        if (div2 > SPI_DIV2_MAX) {
            continue;
        }

        uint32_t actual_hz = hclk_hz / (div1 * (div2 + 1U));
        if (actual_hz <= target_hz && actual_hz >= best_hz) {
            best_hz = actual_hz;
            best_div1 = (uint8_t)div1;
            best_div2 = (uint8_t)div2;
        }
    }

    if (best_hz == 0U) {
        return false;
    }

    config->div1 = best_div1;
    config->div2 = best_div2;
    return true;
}

static void spi_set_csmapping(SPI_TypeDef *spi, spi_chip_select_t cs)
{
    if ((((uint32_t)cs & (uint32_t)SPI_CS_1) != 0U) ||
        (((uint32_t)cs & (uint32_t)SPI_CS_2) != 0U)) {
        spi->CS |= SPI_CSMAP_EN_MSK;
    } else {
        spi->CS &= ~SPI_CSMAP_EN_MSK;
    }
}

static void spi_handle_irq(spi_instance_t inst)
{
    spi_irq_context_t ctx;
    SPI_TypeDef *spi;
    uint32_t active;

    if (!is_valid_inst(inst)) {
        return;
    }

    spi = get_spi(inst);
    active = spi->STATUS & spi->IE;
    if ((active == 0U) || (s_spi_state[inst].callback == NULL)) {
        return;
    }

    ctx.inst = inst;
    ctx.flags = spi->STATUS;
    s_spi_state[inst].callback(&ctx);
}

//===========================================
// 第1层：快速初始化 API
//===========================================

/**
 * @brief 快速初始化 SPI 主模式
 *
 * 默认配置：模式0、1线、MSB 先行、自动 IO 方向、CS0、batch 传输、Dummy=0xFF。
 * 波特率通过 HCLK 自动分频到不高于 spi_clk_hz 的最近值。
 *
 * @param[in] inst        SPI 实例
 * @param[in] spi_clk_hz  目标 SPI 时钟频率（Hz）
 * @param[in] work_mode   SPI 工作模式
 * @return true: 初始化成功, false: 参数非法或分频无法满足
 *
 * @note 覆盖寄存器：CTL, BAUD, TX_CTL, RX_CTL, BATCH
 *
 * @code
 * spi_init_master(SPI_1, 8000000U, SPI_WORK_MODE_0);
 * @endcode
 */
bool spi_init_master(spi_instance_t inst, uint32_t spi_clk_hz, spi_work_mode_t work_mode)
{
    spi_config_t config;
    spi_baud_config_t baud;
    spi_tx_config_t tx_config;
    spi_rx_config_t rx_config;

    if (!is_valid_inst(inst)) {
        return false;
    }

    if (!calc_baud_config(spi_clk_hz, &baud)) {
        return false;
    }

    clock_periph_enable(s_spi_clks[inst]);
    spi_select_cs(inst, SPI_CS_0);

    config.mode = SPI_MODE_MASTER;
    config.work_mode = work_mode;
    config.line_mode = SPI_LINE_MODE_1;
    config.bit_order = SPI_BIT_ORDER_MSB_FIRST;
    config.io_direction = SPI_IO_DIRECTION_AUTO;
    config.slave_enable = false;
    config.sck_filter_enable = false;
    config.cs_reset_enable = false;
    config.cs_filter_enable = false;
    config.cs_time = 0U;
    config.software_cs_enable = false;
    config.software_cs_inactive = true;
    if (!spi_configure(inst, &config)) {
        return false;
    }

    if (!spi_config_baud(inst, &baud)) {
        return false;
    }

    tx_config.enable = false;
    tx_config.batch_mode = false;
    tx_config.dma_enable = false;
    tx_config.dma_level = 0U;
    tx_config.dummy_data = 0xFFU;
    if (!spi_config_tx(inst, &tx_config)) {
        return false;
    }

    rx_config.enable = false;
    rx_config.dma_enable = false;
    rx_config.dma_level = 0U;
    rx_config.sample_shift = SPI_SAMPLE_SHIFT_NONE;
    rx_config.multi_sample_enable = false;
    rx_config.msda_mask = 0U;
    rx_config.msdha_mask = 0U;
    if (!spi_config_rx(inst, &rx_config)) {
        return false;
    }

    return spi_set_batch_size(inst, 1U);
}

/**
 * @brief 快速初始化 SPI 从模式
 *
 * 默认配置：模式0、1线、MSB 先行、自动 IO 方向、从机状态机使能、batch=1。
 *
 * @param[in] inst       SPI 实例
 * @param[in] work_mode  SPI 工作模式
 * @return true: 初始化成功, false: 参数非法
 *
 * @note 覆盖寄存器：CTL, TX_CTL, RX_CTL, BATCH
 *
 * @code
 * spi_init_slave(SPI_1, SPI_WORK_MODE_0);
 * @endcode
 */
bool spi_init_slave(spi_instance_t inst, spi_work_mode_t work_mode)
{
    spi_config_t config;
    spi_tx_config_t tx_config;
    spi_rx_config_t rx_config;

    if (!is_valid_inst(inst)) {
        return false;
    }

    clock_periph_enable(s_spi_clks[inst]);
    spi_select_cs(inst, SPI_CS_0);

    config.mode = SPI_MODE_SLAVE;
    config.work_mode = work_mode;
    config.line_mode = SPI_LINE_MODE_1;
    config.bit_order = SPI_BIT_ORDER_MSB_FIRST;
    config.io_direction = SPI_IO_DIRECTION_AUTO;
    config.slave_enable = true;
    config.sck_filter_enable = false;
    config.cs_reset_enable = false;
    config.cs_filter_enable = false;
    config.cs_time = 0U;
    config.software_cs_enable = false;
    config.software_cs_inactive = true;
    if (!spi_configure(inst, &config)) {
        return false;
    }

    tx_config.enable = false;
    tx_config.batch_mode = false;
    tx_config.dma_enable = false;
    tx_config.dma_level = 0U;
    tx_config.dummy_data = 0xFFU;
    if (!spi_config_tx(inst, &tx_config)) {
        return false;
    }

    rx_config.enable = true;
    rx_config.dma_enable = false;
    rx_config.dma_level = 0U;
    rx_config.sample_shift = SPI_SAMPLE_SHIFT_NONE;
    rx_config.multi_sample_enable = false;
    rx_config.msda_mask = 0U;
    rx_config.msdha_mask = 0U;
    if (!spi_config_rx(inst, &rx_config)) {
        return false;
    }

    return spi_set_batch_size(inst, 1U);
}

//===========================================
// 第2层：基础配置 API
//===========================================

/**
 * @brief 配置 SPI 基础控制寄存器
 *
 * @param[in] inst    SPI 实例
 * @param[in] config  配置结构体，不能为 NULL
 * @return true: 配置成功, false: 参数非法
 *
 * @note 覆盖寄存器：CTL（MST_MODE, SFILTER, CPHA, CPOL, LSB_FIRST, X_MODE,
 *       IO_MODE, SLAVE_EN, CS_RST, CS_FILTER, CS_TIME, SWCS, SWCS_EN）
 *
 * @code
 * spi_config_t cfg = {
 *     .mode = SPI_MODE_MASTER,
 *     .work_mode = SPI_WORK_MODE_0,
 *     .line_mode = SPI_LINE_MODE_1,
 *     .bit_order = SPI_BIT_ORDER_MSB_FIRST,
 *     .io_direction = SPI_IO_DIRECTION_AUTO,
 *     .slave_enable = false,
 * };
 * spi_configure(SPI_1, &cfg);
 * @endcode
 */
bool spi_configure(spi_instance_t inst, const spi_config_t *config)
{
    SPI_TypeDef *spi;
    uint32_t ctl;

    if (!is_valid_inst(inst) || (config == NULL) || (config->cs_time > 0xFFU)) {
        return false;
    }

    spi = get_spi(inst);
    ctl = 0U;

    if (config->mode == SPI_MODE_MASTER) {
        ctl |= SPI_CTL_MST_MODE_MSK;
    }
    if (config->sck_filter_enable) {
        ctl |= SPI_CTL_SFILTER_MSK;
    }
    if ((config->work_mode == SPI_WORK_MODE_1) || (config->work_mode == SPI_WORK_MODE_3)) {
        ctl |= SPI_CTL_CPHA_MSK;
    }
    if ((config->work_mode == SPI_WORK_MODE_2) || (config->work_mode == SPI_WORK_MODE_3)) {
        ctl |= SPI_CTL_CPOL_MSK;
    }
    if (config->bit_order == SPI_BIT_ORDER_LSB_FIRST) {
        ctl |= SPI_CTL_LSB_FIRST_MSK;
    }
    ctl |= ((uint32_t)config->line_mode << SPI_CTL_X_MODE_POS) & SPI_CTL_X_MODE_MSK;
    if (config->io_direction == SPI_IO_DIRECTION_AUTO) {
        ctl |= SPI_CTL_IO_MODE_MSK;
    }
    if (config->slave_enable) {
        ctl |= SPI_CTL_SLAVE_EN_MSK;
    }
    if (config->cs_reset_enable) {
        ctl |= SPI_CTL_CS_RST_MSK;
    }
    if (config->cs_filter_enable) {
        ctl |= SPI_CTL_CS_FILTER_MSK;
    }
    ctl |= ((uint32_t)config->cs_time << SPI_CTL_CS_TIME_POS) & SPI_CTL_CS_TIME_MSK;

    spi->CTL = ctl;

    if (config->software_cs_enable) {
        spi->CTL |= SPI_CTL_SWCS_EN_MSK;
        if (config->software_cs_inactive) {
            spi->CTL |= SPI_CTL_SWCS_MSK;
        } else {
            spi->CTL &= ~SPI_CTL_SWCS_MSK;
        }
    }

    return true;
}

/**
 * @brief 配置 SPI 波特率
 *
 * @param[in] inst    SPI 实例
 * @param[in] config  波特率配置结构体，不能为 NULL
 * @return true: 配置成功, false: 参数非法
 *
 * @note 覆盖寄存器：BAUD（DIV1, DIV2）
 * @note 波特率公式：SPI_CLK = fHCLK / (DIV1 * (DIV2+1))
 * @note DIV1 必须为 2-254 间的偶数，DIV2 为 0-255
 *
 * @code
 * spi_baud_config_t baud = { .div1 = 8, .div2 = 0 };
 * spi_config_baud(SPI_1, &baud);
 * @endcode
 */
bool spi_config_baud(spi_instance_t inst, const spi_baud_config_t *config)
{
    SPI_TypeDef *spi;

    if (!is_valid_inst(inst) || (config == NULL) ||
        (config->div1 < SPI_DIV1_MIN) || (config->div1 > SPI_DIV1_MAX) ||
        ((config->div1 & 1U) != 0U)) {
        return false;
    }

    spi = get_spi(inst);
    spi->BAUD = ((uint32_t)config->div2 << 8) | (uint32_t)config->div1;
    return true;
}

/**
 * @brief 配置 SPI 发送控制寄存器
 *
 * @param[in] inst    SPI 实例
 * @param[in] config  发送配置结构体，不能为 NULL
 * @return true: 配置成功, false: 参数非法
 *
 * @note 覆盖寄存器：TX_CTL（TX_EN, TX_MODE, TX_DMA_REQ_EN, TX_DMA_LEVEL, DUMMY）
 *
 * @code
 * spi_tx_config_t tx_cfg = {
 *     .enable = true,
 *     .batch_mode = false,
 *     .dma_enable = false,
 *     .dma_level = 0,
 *     .dummy_data = 0xFF,
 * };
 * spi_config_tx(SPI_1, &tx_cfg);
 * @endcode
 */
bool spi_config_tx(spi_instance_t inst, const spi_tx_config_t *config)
{
    SPI_TypeDef *spi;
    uint32_t tx_ctl;

    if (!is_valid_inst(inst) || (config == NULL) || (config->dma_level > 0x0FU)) {
        return false;
    }

    spi = get_spi(inst);
    tx_ctl = 0U;
    if (config->enable) {
        tx_ctl |= SPI_TX_CTL_EN_MSK;
    }
    if (config->batch_mode) {
        tx_ctl |= SPI_TX_CTL_MODE_MSK;
    }
    if (config->dma_enable) {
        tx_ctl |= SPI_TX_CTL_DMA_REQ_EN_MSK;
    }
    tx_ctl |= ((uint32_t)config->dma_level << SPI_TX_CTL_DMA_LEVEL_POS) & SPI_TX_CTL_DMA_LEVEL_MSK;
    tx_ctl |= ((uint32_t)config->dummy_data << SPI_TX_CTL_DUMMY_POS) & SPI_TX_CTL_DUMMY_MSK;
    spi->TX_CTL = tx_ctl;
    return true;
}

/**
 * @brief 配置 SPI 接收控制寄存器
 *
 * @param[in] inst    SPI 实例
 * @param[in] config  接收配置结构体，不能为 NULL
 * @return true: 配置成功, false: 参数非法
 *
 * @note 覆盖寄存器：RX_CTL（RX_EN, RX_DMA_REQ_EN, RX_DMA_LEVEL, SSHIFT1/2,
 *       MSDA_EN, MSDA_X0-X3, MSDHA_X0-X3）
 *
 * @code
 * spi_rx_config_t rx_cfg = {
 *     .enable = true,
 *     .dma_enable = false,
 *     .dma_level = 0,
 *     .sample_shift = SPI_SAMPLE_SHIFT_NONE,
 *     .multi_sample_enable = false,
 *     .msda_mask = 0,
 *     .msdha_mask = 0,
 * };
 * spi_config_rx(SPI_1, &rx_cfg);
 * @endcode
 */
bool spi_config_rx(spi_instance_t inst, const spi_rx_config_t *config)
{
    SPI_TypeDef *spi;
    uint32_t rx_ctl;

    if (!is_valid_inst(inst) || (config == NULL) || (config->dma_level > 0x0FU)) {
        return false;
    }

    spi = get_spi(inst);
    rx_ctl = 0U;
    if (config->enable) {
        rx_ctl |= SPI_RX_CTL_EN_MSK;
    }
    if (config->dma_enable) {
        rx_ctl |= SPI_RX_CTL_DMA_REQ_EN_MSK;
    }
    rx_ctl |= ((uint32_t)config->dma_level << SPI_RX_CTL_DMA_LEVEL_POS) & SPI_RX_CTL_DMA_LEVEL_MSK;
    rx_ctl |= (uint32_t)config->sample_shift &
              (SPI_RX_CTL_SSHIFT1_MSK | SPI_RX_CTL_SSHIFT2_MSK);
    if (config->multi_sample_enable) {
        rx_ctl |= SPI_RX_CTL_MSDA_EN_MSK;
    }
    if ((config->msda_mask & 0x01U) != 0U) {
        rx_ctl |= SPI_RX_CTL_MSDA_X0_MSK;
    }
    if ((config->msda_mask & 0x02U) != 0U) {
        rx_ctl |= SPI_RX_CTL_MSDA_X1_MSK;
    }
    if ((config->msda_mask & 0x04U) != 0U) {
        rx_ctl |= SPI_RX_CTL_MSDA_X2_MSK;
    }
    if ((config->msda_mask & 0x08U) != 0U) {
        rx_ctl |= SPI_RX_CTL_MSDA_X3_MSK;
    }
    if ((config->msdha_mask & 0x01U) != 0U) {
        rx_ctl |= SPI_RX_CTL_MSDHA_X0_MSK;
    }
    if ((config->msdha_mask & 0x02U) != 0U) {
        rx_ctl |= SPI_RX_CTL_MSDHA_X1_MSK;
    }
    if ((config->msdha_mask & 0x04U) != 0U) {
        rx_ctl |= SPI_RX_CTL_MSDHA_X2_MSK;
    }
    if ((config->msdha_mask & 0x08U) != 0U) {
        rx_ctl |= SPI_RX_CTL_MSDHA_X3_MSK;
    }

    spi->RX_CTL = rx_ctl;
    return true;
}

/**
 * @brief 配置 SPI 内存映射访问模式
 *
 * @param[in] inst    SPI 实例
 * @param[in] config  内存映射配置结构体，不能为 NULL
 * @return true: 配置成功, false: 参数非法
 *
 * @note 覆盖寄存器：MEMO_ACC（ACC_EN, CS_TOUT_EN, CON_MODE_EN, SEND_INSTR_ONCE_EN,
 *       WR_AB_EN, RD_AB_EN, ALTER_BYTE_SIZE, WR_DB_EN, RD_DB_EN, DUMMY_CYCLE_SIZE,
 *       ADDR_WIDTH, INSTR_MODE, ADDR_MODE, ALTER_BYTE_MODE, DATA_MODE）
 *
 * @code
 * spi_memory_config_t mem_cfg = {
 *     .access_enable = false,
 *     .cs_timeout_enable = true,
 *     .continuous_mode_enable = false,
 *     .dummy_cycle = SPI_DUMMY_CYCLE_8,
 *     .addr_width = SPI_ADDR_WIDTH_24,
 *     .instr_mode = SPI_MEM_LINE_MODE_1,
 *     .addr_mode = SPI_MEM_LINE_MODE_1,
 *     .data_mode = SPI_MEM_LINE_MODE_1,
 * };
 * spi_config_memory(SPI_1, &mem_cfg);
 * @endcode
 */
bool spi_config_memory(spi_instance_t inst, const spi_memory_config_t *config)
{
    SPI_TypeDef *spi;
    uint32_t memo_acc;

    if (!is_valid_inst(inst) || (config == NULL)) {
        return false;
    }

    spi = get_spi(inst);
    memo_acc = 0U;

    if (config->access_enable) {
        memo_acc |= SPI_MEM_ACC_ACC_EN_MSK;
    }
    if (config->cs_timeout_enable) {
        memo_acc |= SPI_MEM_ACC_CS_TOUT_EN_MSK;
    }
    if (config->continuous_mode_enable) {
        memo_acc |= SPI_MEM_ACC_CON_MODE_EN_MSK;
    }
    if (config->send_instr_once_enable) {
        memo_acc |= SPI_MEM_ACC_SEND_INSTR_ONCE_EN_MSK;
    }
    if (config->write_alter_byte_enable) {
        memo_acc |= SPI_MEM_ACC_WR_AB_EN_MSK;
    }
    if (config->read_alter_byte_enable) {
        memo_acc |= SPI_MEM_ACC_RD_AB_EN_MSK;
    }
    memo_acc |= ((uint32_t)config->alter_byte_size << SPI_MEM_ACC_ALTER_BYTE_SIZE_POS) &
                SPI_MEM_ACC_ALTER_BYTE_SIZE_MSK;
    if (config->write_data_enable) {
        memo_acc |= SPI_MEM_ACC_WR_DB_EN_MSK;
    }
    if (config->read_data_enable) {
        memo_acc |= SPI_MEM_ACC_RD_DB_EN_MSK;
    }
    memo_acc |= ((uint32_t)config->dummy_cycle << SPI_MEM_ACC_DUMMY_CYCLE_SIZE_POS) &
                SPI_MEM_ACC_DUMMY_CYCLE_SIZE_MSK;
    memo_acc |= ((uint32_t)config->addr_width << SPI_MEM_ACC_ADDR_WIDTH_POS) &
                SPI_MEM_ACC_ADDR_WIDTH_MSK;
    memo_acc |= ((uint32_t)config->instr_mode << SPI_MEM_ACC_INSTR_MODE_POS) &
                SPI_MEM_ACC_INSTR_MODE_MSK;
    memo_acc |= ((uint32_t)config->addr_mode << SPI_MEM_ACC_ADDR_MODE_POS) &
                SPI_MEM_ACC_ADDR_MODE_MSK;
    memo_acc |= ((uint32_t)config->alter_byte_mode << SPI_MEM_ACC_ALTER_BYTE_MODE_POS) &
                SPI_MEM_ACC_ALTER_BYTE_MODE_MSK;
    memo_acc |= ((uint32_t)config->data_mode << SPI_MEM_ACC_DATA_MODE_POS) &
                SPI_MEM_ACC_DATA_MODE_MSK;

    spi->MEMO_ACC = memo_acc;
    return true;
}

/**
 * @brief 配置 SPI 内存映射命令寄存器
 *
 * @param[in] inst    SPI 实例
 * @param[in] config  命令配置结构体，不能为 NULL
 *
 * @note 覆盖寄存器：CMD（RD_CMD, WR_CMD）
 *
 * @code
 * spi_mem_command_t cmd = { .read_cmd = 0x03, .write_cmd = 0x02 };
 * spi_config_mem_command(SPI_1, &cmd);
 * @endcode
 */
void spi_config_mem_command(spi_instance_t inst, const spi_mem_command_t *config)
{
    SPI_TypeDef *spi;

    assert(is_valid_inst(inst));
    assert(config != NULL);

    spi = get_spi(inst);
    spi->CMD = ((uint32_t)config->write_cmd << 8) | (uint32_t)config->read_cmd;
}

//===========================================
// 第3层：高级功能 API
//===========================================

/**
 * @brief 选择默认片选线
 *
 * @param[in] inst  SPI 实例
 * @param[in] cs    片选掩码，可组合
 *
 * @note 覆盖寄存器：CS（CS, CSMAP_EN）
 *
 * @code
 * spi_select_cs(SPI_1, SPI_CS_0);
 * @endcode
 */
void spi_select_cs(spi_instance_t inst, spi_chip_select_t cs)
{
    SPI_TypeDef *spi;

    assert(is_valid_inst(inst));

    spi = get_spi(inst);
    s_spi_state[inst].selected_cs = cs;
    spi_set_csmapping(spi, cs);
}

/**
 * @brief 手动拉低当前默认片选
 *
 * @param[in] inst SPI 实例
 *
 * @note 覆盖寄存器：CS（CS）
 */
void spi_assert_cs(spi_instance_t inst)
{
    SPI_TypeDef *spi;

    assert(is_valid_inst(inst));

    spi = get_spi(inst);
    spi_set_csmapping(spi, s_spi_state[inst].selected_cs);
    spi->CS = (spi->CS & ~SPI_CS_MASK) | ((uint32_t)s_spi_state[inst].selected_cs & SPI_CS_MASK) |
              (spi->CS & SPI_CSMAP_EN_MSK);
}

/**
 * @brief 手动释放当前默认片选
 *
 * @param[in] inst SPI 实例
 *
 * @note 覆盖寄存器：CS（CS）
 */
void spi_release_cs(spi_instance_t inst)
{
    SPI_TypeDef *spi;

    assert(is_valid_inst(inst));

    spi = get_spi(inst);
    spi->CS &= ~SPI_CS_MASK;
}

/**
 * @brief 配置批量传输数据量
 *
 * @param[in] inst   SPI 实例
 * @param[in] batch  批次数（1 ~ SPI_BATCH_MAX）
 * @return true: 配置成功, false: 参数非法
 *
 * @note 覆盖寄存器：BATCH（BATCH_NUMBER）
 */
bool spi_set_batch_size(spi_instance_t inst, uint32_t batch)
{
    SPI_TypeDef *spi;

    if (!is_valid_inst(inst) || (batch == 0U) || (batch > SPI_BATCH_MAX)) {
        return false;
    }

    spi = get_spi(inst);
    spi->BATCH = batch;
    return true;
}

/**
 * @brief 配置 TX 延时寄存器
 *
 * @param[in] inst       SPI 实例
 * @param[in] delay_hclk 延时 HCLK 周期数
 *
 * @note 覆盖寄存器：TXDELAY（TDY）
 */
void spi_set_tx_delay(spi_instance_t inst, uint32_t delay_hclk)
{
    assert(is_valid_inst(inst));
    get_spi(inst)->TXDELAY = delay_hclk;
}

/**
 * @brief 配置手动 IO 输出使能
 *
 * @param[in] inst     SPI 实例
 * @param[in] outputs  输出掩码
 *
 * @note 覆盖寄存器：OUT_EN（MOSI_EN, MISO_EN, WP_EN, HOLD_EN）
 */
void spi_set_output_enable(spi_instance_t inst, spi_io_output_t outputs)
{
    assert(is_valid_inst(inst));
    get_spi(inst)->OUT_EN = (uint32_t)outputs & SPI_OUT_EN_MASK;
}

/**
 * @brief 设置交替字节值
 *
 * @param[in] inst   SPI 实例
 * @param[in] value  交替字节寄存器值
 *
 * @note 覆盖寄存器：ALTER_BYTE（ALTER_BYTE）
 */
void spi_set_alter_byte(spi_instance_t inst, uint32_t value)
{
    assert(is_valid_inst(inst));
    get_spi(inst)->ALTER_BYTE = value;
}

/**
 * @brief 设置 CS 超时值
 *
 * @param[in] inst     SPI 实例
 * @param[in] timeout  超时计数值（16bit）
 *
 * @note 覆盖寄存器：CS_TOUT_VAL（CS_TOUT_VAL）
 */
void spi_set_cs_timeout(spi_instance_t inst, uint16_t timeout)
{
    assert(is_valid_inst(inst));
    get_spi(inst)->CS_TOUT_VAL = (uint32_t)timeout;
}

/**
 * @brief 清除"仅发送一次指令"状态
 *
 * @param[in] inst SPI 实例
 *
 * @note 覆盖寄存器：MEMO_ACC（ONCE_INSTR_CLR）
 */
void spi_mem_clear_once_instruction(spi_instance_t inst)
{
    assert(is_valid_inst(inst));
    get_spi(inst)->MEMO_ACC |= SPI_MEM_ACC_ONCE_INSTR_CLR_MSK;
}

/**
 * @brief 使能/禁用单线归零码（SID）模式
 *
 * SID 模式用于 LED 灯带控制（如 WS2812）。
 * 使能后，SPI 自动将 RGB 数据打包为归零码格式发送。
 *
 * @param[in] inst   SPI 实例
 * @param[in] enable true: 使能, false: 禁用
 *
 * @note 覆盖寄存器：CTL（SID_EN）
 * @note 使能 SID 模式前，需先配置 SPI 主模式和波特率（SCK = 3.2MHz）
 *
 * @code
 * spi_sid_enable(SPI_1, true);
 * @endcode
 */
void spi_sid_enable(spi_instance_t inst, bool enable)
{
    SPI_TypeDef *spi;

    assert(is_valid_inst(inst));

    spi = get_spi(inst);
    if (enable) {
        spi->CTL |= SPI_CTL_SID_EN_MSK;
    } else {
        spi->CTL &= ~SPI_CTL_SID_EN_MSK;
    }
}

/**
 * @brief 写入 RGB 数据
 *
 * 向 RGB_DATA 寄存器写入 24bit RGB 数据（8bit Red + 8bit Green + 8bit Blue）。
 * 硬件自动将 RGB 数据打包为归零码格式写入 FIFO。
 *
 * @param[in] inst  SPI 实例
 * @param[in] rgb   24bit RGB 数据（0x000000 ~ 0xFFFFFF）
 *
 * @note 覆盖寄存器：RGB_DATA（RGB_DATA）
 * @note 必须在 SID 模式使能后使用
 *
 * @code
 * // 发送红色（R=255, G=0, B=0）
 * spi_sid_write_rgb(SPI_1, 0xFF0000U);
 * @endcode
 */
void spi_sid_write_rgb(spi_instance_t inst, uint32_t rgb)
{
    assert(is_valid_inst(inst));
    get_spi(inst)->RGB_DATA = rgb & 0x00FFFFFFU;
}

/**
 * @brief 设置 SID 复位码计数值
 *
 * 设置复位码的持续时间（单位：HCLK 周期）。
 * 复位码用于指示一帧 RGB 数据传输结束，典型值 > 280µs。
 *
 * @param[in] inst      SPI 实例
 * @param[in] hclk_cnt  复位码计数值（24bit）
 *
 * @note 覆盖寄存器：SID_RESET_VAL（RESET_VAL）
 * @note 计算公式：hclk_cnt = 复位时间(µs) × fHCLK(MHz)
 *
 * @code
 * // 64MHz HCLK 下，280µs 复位码 = 280 * 64 = 17920
 * spi_sid_set_reset_val(SPI_1, 17920U);
 * @endcode
 */
void spi_sid_set_reset_val(spi_instance_t inst, uint32_t hclk_cnt)
{
    assert(is_valid_inst(inst));
    get_spi(inst)->SID_RESET_VAL = hclk_cnt & 0x00FFFFFFU;
}

/**
 * @brief 注册 SPI 中断回调
 *
 * @param[in] inst      SPI 实例
 * @param[in] callback  回调函数，可为 NULL
 */
void spi_irq_register(spi_instance_t inst, spi_callback_t callback)
{
    assert(is_valid_inst(inst));
    s_spi_state[inst].callback = callback;
}

/**
 * @brief 使能 SPI 中断
 *
 * @param[in] inst  SPI 实例
 * @param[in] irq   中断掩码，可组合
 *
 * @note 覆盖寄存器：IE
 */
void spi_irq_enable(spi_instance_t inst, spi_irq_t irq)
{
    assert(is_valid_inst(inst));
    get_spi(inst)->IE |= (uint32_t)irq;
}

/**
 * @brief 禁用 SPI 中断
 *
 * @param[in] inst  SPI 实例
 * @param[in] irq   中断掩码，可组合
 *
 * @note 覆盖寄存器：IE
 */
void spi_irq_disable(spi_instance_t inst, spi_irq_t irq)
{
    assert(is_valid_inst(inst));
    get_spi(inst)->IE &= ~((uint32_t)irq);
}

//===========================================
// 第4层：控制与查询 API
//===========================================

/**
 * @brief 复位 TX FIFO
 *
 * @param[in] inst SPI 实例
 *
 * @note 覆盖寄存器：TX_CTL（TX_FIFO_RESET）
 */
void spi_tx_fifo_reset(spi_instance_t inst)
{
    SPI_TypeDef *spi;

    assert(is_valid_inst(inst));
    spi = get_spi(inst);
    spi->TX_CTL |= SPI_TX_CTL_FIFO_RESET_MSK;
    spi->TX_CTL &= ~SPI_TX_CTL_FIFO_RESET_MSK;
}

/**
 * @brief 复位 RX FIFO
 *
 * @param[in] inst SPI 实例
 *
 * @note 覆盖寄存器：RX_CTL（RX_FIFO_RESET）
 */
void spi_rx_fifo_reset(spi_instance_t inst)
{
    SPI_TypeDef *spi;

    assert(is_valid_inst(inst));
    spi = get_spi(inst);
    spi->RX_CTL |= SPI_RX_CTL_FIFO_RESET_MSK;
    spi->RX_CTL &= ~SPI_RX_CTL_FIFO_RESET_MSK;
}

/**
 * @brief 使能 TX
 *
 * @param[in] inst SPI 实例
 *
 * @note 覆盖寄存器：TX_CTL（TX_EN）
 */
void spi_tx_enable(spi_instance_t inst)
{
    assert(is_valid_inst(inst));
    get_spi(inst)->TX_CTL |= SPI_TX_CTL_EN_MSK;
}

/**
 * @brief 禁用 TX
 *
 * @param[in] inst SPI 实例
 *
 * @note 覆盖寄存器：TX_CTL（TX_EN）
 */
void spi_tx_disable(spi_instance_t inst)
{
    assert(is_valid_inst(inst));
    get_spi(inst)->TX_CTL &= ~SPI_TX_CTL_EN_MSK;
}

/**
 * @brief 使能 RX
 *
 * @param[in] inst SPI 实例
 *
 * @note 覆盖寄存器：RX_CTL（RX_EN）
 */
void spi_rx_enable(spi_instance_t inst)
{
    assert(is_valid_inst(inst));
    get_spi(inst)->RX_CTL |= SPI_RX_CTL_EN_MSK;
}

/**
 * @brief 禁用 RX
 *
 * @param[in] inst SPI 实例
 *
 * @note 覆盖寄存器：RX_CTL（RX_EN）
 */
void spi_rx_disable(spi_instance_t inst)
{
    assert(is_valid_inst(inst));
    get_spi(inst)->RX_CTL &= ~SPI_RX_CTL_EN_MSK;
}

/**
 * @brief 向 DAT 写入 1 字节
 *
 * @param[in] inst  SPI 实例
 * @param[in] data  数据字节
 *
 * @note 覆盖寄存器：DAT
 */
void spi_write_data(spi_instance_t inst, uint8_t data)
{
    assert(is_valid_inst(inst));
    get_spi(inst)->DAT = data;
}

/**
 * @brief 从 DAT 读取 1 字节
 *
 * @param[in] inst SPI 实例
 * @return 接收到的 8bit 数据
 *
 * @note 覆盖寄存器：DAT
 */
uint8_t spi_read_data(spi_instance_t inst)
{
    assert(is_valid_inst(inst));
    return (uint8_t)(get_spi(inst)->DAT & 0xFFU);
}

/**
 * @brief 获取 STATUS 寄存器
 *
 * @param[in] inst SPI 实例
 * @return STATUS 寄存器快照
 *
 * @note 覆盖寄存器：STATUS
 */
uint32_t spi_get_flags(spi_instance_t inst)
{
    assert(is_valid_inst(inst));
    return get_spi(inst)->STATUS;
}

/**
 * @brief 判断指定状态标志是否置位
 *
 * @param[in] inst   SPI 实例
 * @param[in] flag   状态标志
 * @return true: 已置位, false: 未置位
 *
 * @note 覆盖寄存器：STATUS
 */
bool spi_flag_is_set(spi_instance_t inst, spi_flag_t flag)
{
    assert(is_valid_inst(inst));
    return (get_spi(inst)->STATUS & (uint32_t)flag) != 0U;
}

/**
 * @brief 清除状态标志
 *
 * @param[in] inst   SPI 实例
 * @param[in] flags  要清除的标志掩码
 *
 * @note 覆盖寄存器：STATUS（写1清除）
 * @note 批量完成相关标志（TX_BATCH_DONE/RX_BATCH_DONE）统一通过 BATCH_DONE 位清除
 */
void spi_clear_flags(spi_instance_t inst, uint32_t flags)
{
    uint32_t clear_mask;

    assert(is_valid_inst(inst));

    clear_mask = flags;

    /*
     * 批量完成相关标志在硬件上通过 STATUS.BATCH_DONE 统一清除。
     * 因此外部若请求清 TX/RX batch done，这里一并补上 BATCH_DONE，
     * 避免中断状态残留导致重复进入回调。
     */
    if ((flags & ((uint32_t)SPI_FLAG_TX_BATCH_DONE | (uint32_t)SPI_FLAG_RX_BATCH_DONE)) != 0U) {
        clear_mask |= (uint32_t)SPI_FLAG_BATCH_DONE;
    }

    *(volatile uint32_t *)&get_spi(inst)->STATUS = clear_mask & SPI_STATUS_W1C_MASK;
}

/**
 * @brief 等待 SPI 空闲
 *
 * @param[in] inst        SPI 实例
 * @param[in] timeout_ms  超时时间，0 使用默认值
 * @return true: 空闲, false: 超时
 *
 * @note 覆盖寄存器：STATUS（BUSY）
 */
bool spi_wait_idle(spi_instance_t inst, uint32_t timeout_ms)
{
    if (!is_valid_inst(inst)) {
        return false;
    }
    return wait_flag_state(get_spi(inst), (uint32_t)SPI_FLAG_BUSY, false, timeout_ms);
}

/**
 * @brief 阻塞式发送
 *
 * @param[in] inst        SPI 实例
 * @param[in] data        发送缓冲区
 * @param[in] len         发送长度
 * @param[in] timeout_ms  超时时间，0 使用默认值
 * @return true: 发送成功, false: 参数非法或超时
 *
 * @note 覆盖寄存器：DAT, TX_CTL, RX_CTL, BATCH, CS, STATUS
 *
 * @code
 * uint8_t tx_buf[] = { 0x9F, 0x00, 0x00, 0x00 };
 * spi_transmit(SPI_1, tx_buf, sizeof(tx_buf), 100U);
 * @endcode
 */
bool spi_transmit(spi_instance_t inst, const uint8_t *data, size_t len, uint32_t timeout_ms)
{
    SPI_TypeDef *spi;
    uint32_t timeout;
    uint64_t start;
    size_t sent = 0U;

    if (!is_valid_inst(inst) || (data == NULL) || (len == 0U) || (len > SPI_BATCH_MAX)) {
        return false;
    }

    spi = get_spi(inst);
    timeout = normalize_timeout(timeout_ms);
    start = system_get_tick();

    spi_clear_flags(inst, (uint32_t)SPI_FLAG_TX_BATCH_DONE | (uint32_t)SPI_FLAG_BATCH_DONE);
    spi_tx_fifo_reset(inst);
    spi_rx_disable(inst);
    spi_set_batch_size(inst, (uint32_t)len);
    spi_tx_enable(inst);
    spi_assert_cs(inst);

    while (sent < len) {
        if ((spi->STATUS & (uint32_t)SPI_FLAG_TX_FIFO_FULL) == 0U) {
            spi->DAT = data[sent++];
            start = system_get_tick();
            continue;
        }
        if (system_elapsed(start, timeout)) {
            spi_tx_disable(inst);
            spi_release_cs(inst);
            return false;
        }
    }

    if (!wait_flag_state(spi, (uint32_t)SPI_FLAG_TX_BATCH_DONE, true, timeout)) {
        spi_tx_disable(inst);
        spi_release_cs(inst);
        return false;
    }

    spi_clear_flags(inst, (uint32_t)SPI_FLAG_TX_BATCH_DONE | (uint32_t)SPI_FLAG_BATCH_DONE);
    spi_tx_disable(inst);
    spi_release_cs(inst);
    return true;
}

/**
 * @brief 阻塞式接收
 *
 * @param[in] inst        SPI 实例
 * @param[out] data       接收缓冲区
 * @param[in] len         接收长度
 * @param[in] timeout_ms  超时时间，0 使用默认值
 * @return true: 接收成功, false: 参数非法或超时
 *
 * @note 覆盖寄存器：DAT, TX_CTL, RX_CTL, BATCH, CS, STATUS
 *
 * @code
 * uint8_t rx_buf[8];
 * spi_receive(SPI_1, rx_buf, sizeof(rx_buf), 100U);
 * @endcode
 */
bool spi_receive(spi_instance_t inst, uint8_t *data, size_t len, uint32_t timeout_ms)
{
    SPI_TypeDef *spi;
    uint32_t timeout;
    uint64_t start;
    size_t recv = 0U;

    if (!is_valid_inst(inst) || (data == NULL) || (len == 0U) || (len > SPI_BATCH_MAX)) {
        return false;
    }

    spi = get_spi(inst);
    timeout = normalize_timeout(timeout_ms);
    start = system_get_tick();

    spi_rx_fifo_reset(inst);
    spi_clear_flags(inst, (uint32_t)SPI_FLAG_RX_BATCH_DONE | (uint32_t)SPI_FLAG_BATCH_DONE);
    spi_tx_disable(inst);
    spi_set_batch_size(inst, (uint32_t)len);
    spi_rx_enable(inst);
    spi_assert_cs(inst);

    while (recv < len) {
        if ((spi->STATUS & (uint32_t)SPI_FLAG_RX_FIFO_EMPTY) == 0U) {
            data[recv++] = (uint8_t)(spi->DAT & 0xFFU);
            start = system_get_tick();
            continue;
        }
        if (system_elapsed(start, timeout)) {
            spi_rx_disable(inst);
            spi_release_cs(inst);
            return false;
        }
    }

    if (!wait_flag_state(spi, (uint32_t)SPI_FLAG_RX_BATCH_DONE, true, timeout)) {
        spi_rx_disable(inst);
        spi_release_cs(inst);
        return false;
    }

    spi_clear_flags(inst, (uint32_t)SPI_FLAG_RX_BATCH_DONE | (uint32_t)SPI_FLAG_BATCH_DONE);
    spi_rx_disable(inst);
    spi_release_cs(inst);
    return true;
}

/**
 * @brief 阻塞式全双工收发
 *
 * @param[in] inst        SPI 实例
 * @param[in] tx_data     发送缓冲区
 * @param[out] rx_data    接收缓冲区
 * @param[in] len         数据长度
 * @param[in] timeout_ms  超时时间，0 使用默认值
 * @return true: 收发成功, false: 参数非法或超时
 *
 * @note 覆盖寄存器：DAT, TX_CTL, RX_CTL, BATCH, CS, STATUS
 * @note 推荐仅在 1 线模式下使用全双工；2/4 线模式硬件文档定义为半双工
 *
 * @code
 * uint8_t tx_buf[] = { 0x9F, 0xAA, 0x55, 0x00 };
 * uint8_t rx_buf[4];
 * spi_transceive(SPI_1, tx_buf, rx_buf, sizeof(tx_buf), 100U);
 * @endcode
 */
bool spi_transceive(spi_instance_t inst, const uint8_t *tx_data, uint8_t *rx_data,
                    size_t len, uint32_t timeout_ms)
{
    SPI_TypeDef *spi;
    uint32_t timeout;
    uint64_t start;
    size_t sent = 0U;
    size_t recv = 0U;

    if (!is_valid_inst(inst) || (tx_data == NULL) || (rx_data == NULL) ||
        (len == 0U) || (len > SPI_BATCH_MAX)) {
        return false;
    }

    spi = get_spi(inst);
    timeout = normalize_timeout(timeout_ms);
    start = system_get_tick();

    spi_tx_fifo_reset(inst);
    spi_rx_fifo_reset(inst);
    spi_clear_flags(inst, (uint32_t)SPI_FLAG_TX_BATCH_DONE |
                          (uint32_t)SPI_FLAG_RX_BATCH_DONE |
                          (uint32_t)SPI_FLAG_BATCH_DONE);
    spi_set_batch_size(inst, (uint32_t)len);
    spi_tx_enable(inst);
    spi_rx_enable(inst);
    spi_assert_cs(inst);

    while (recv < len) {
        if ((sent < len) && ((spi->STATUS & (uint32_t)SPI_FLAG_TX_FIFO_FULL) == 0U)) {
            spi->DAT = tx_data[sent++];
            start = system_get_tick();
        }

        while ((spi->STATUS & (uint32_t)SPI_FLAG_RX_FIFO_EMPTY) == 0U) {
            rx_data[recv++] = (uint8_t)(spi->DAT & 0xFFU);
            start = system_get_tick();
            if (recv == len) {
                break;
            }
        }

        if (system_elapsed(start, timeout)) {
            spi_tx_disable(inst);
            spi_rx_disable(inst);
            spi_release_cs(inst);
            return false;
        }
    }

    if (!wait_flag_state(spi, (uint32_t)SPI_FLAG_TX_BATCH_DONE, true, timeout) ||
        !wait_flag_state(spi, (uint32_t)SPI_FLAG_RX_BATCH_DONE, true, timeout)) {
        spi_tx_disable(inst);
        spi_rx_disable(inst);
        spi_release_cs(inst);
        return false;
    }

    spi_clear_flags(inst, (uint32_t)SPI_FLAG_TX_BATCH_DONE |
                          (uint32_t)SPI_FLAG_RX_BATCH_DONE |
                          (uint32_t)SPI_FLAG_BATCH_DONE);
    spi_tx_disable(inst);
    spi_rx_disable(inst);
    spi_release_cs(inst);
    return true;
}

//===========================================
// 中断服务函数
//===========================================

void QSPI1_IRQHandler(void) { spi_handle_irq(SPI_1); }
void QSPI2_IRQHandler(void) { spi_handle_irq(SPI_2); }
void QSPI3_IRQHandler(void) { spi_handle_irq(SPI_3); }
void QSPI4_IRQHandler(void) { spi_handle_irq(SPI_4); }
void QSPI7_IRQHandler(void) { spi_handle_irq(SPI_5); }