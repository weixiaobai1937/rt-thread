/**
 * @file ospi.c
 * @brief ACM32P4 OSPI 驱动实现
 *
 * 内部辅助函数和状态管理实现
 *
 * @author ACM32P4 SDK Team
 * @version 1.0.0
 */

#include <stddef.h>
#include "device/acm32p4xx.h"
#include "hardware/clocks.h"
#include "hardware/system.h"
#include "hardware/ospi.h"

//===========================================
// 内部宏定义
//===========================================

#define OSPI_TX_DAT_OFFSET      0x00U
#define OSPI_RX_DAT_OFFSET      0x00U
#define OSPI_BAUD_OFFSET        0x04U
#define OSPI_CTL_OFFSET         0x08U
#define OSPI_TX_CTL_OFFSET      0x0CU
#define OSPI_RX_CTL_OFFSET      0x10U
#define OSPI_IE_OFFSET          0x14U
#define OSPI_STATUS_OFFSET      0x18U
#define OSPI_TXDELAY_OFFSET     0x1CU
#define OSPI_BATCH_OFFSET       0x20U
#define OSPI_CS_OFFSET          0x24U
#define OSPI_OUT_EN_OFFSET      0x28U
#define OSPI_MEMO_ACC1_OFFSET   0x2CU
#define OSPI_CMD_OFFSET         0x30U
#define OSPI_ALTER_BYTE_OFFSET  0x34U
#define OSPI_CS_TOUT_VAL_OFFSET 0x38U
#define OSPI_MEMO_ACC2_OFFSET   0x3CU
#define OSPI_MEMO_ACC3_OFFSET   0x40U

#define OSPI_MEMO_ACC1_BIT28_MASK  ~(1UL << 28)

//===========================================
// 内部类型定义
//===========================================

typedef struct {
    bool is_configured;
    ospi_baudrate_config_t baudrate;
    ospi_basic_config_t basic;
    ospi_protocol_config_t protocol;
    ospi_memory_mapped_config_t mem_mapped;
    ospi_dma_config_t dma;
    ospi_cs_config_t cs;
    ospi_wrap_size_t wrap_size;
} ospi_config_state_t;

typedef struct {
    ospi_callback_t batch_done_cb;
    ospi_callback_t tx_fifo_empty_cb;
    ospi_callback_t tx_fifo_full_cb;
    ospi_callback_t rx_fifo_empty_cb;
    ospi_callback_t rx_fifo_full_cb;
    ospi_callback_t tx_fifo_half_empty_cb;
    ospi_callback_t tx_fifo_half_full_cb;
    ospi_callback_t rx_fifo_half_empty_cb;
    ospi_callback_t rx_fifo_half_full_cb;
    ospi_callback_t rx_fifo_not_empty_cb;
    ospi_callback_t tx_batch_done_cb;
    ospi_callback_t rx_batch_done_cb;
    ospi_callback_t apm_dumy_done_cb;
    uint32_t irq_enable;
} ospi_irq_state_t;

//===========================================
// 内部状态变量
//===========================================

static OSPI_TypeDef * const s_ospi_regs[] = { OSPI1, OSPI2 };
static const uint8_t s_ospi_count = sizeof(s_ospi_regs) / sizeof(s_ospi_regs[0]);

static ospi_config_state_t s_ospi_config_state[OSPI_COUNT] = {
    { .is_configured = false },
    { .is_configured = false },
};

static volatile ospi_irq_state_t s_ospi_irq_state[OSPI_COUNT];

//===========================================
// 内部辅助函数
//===========================================

/**
 * @brief 获取 OSPI 外设基地址
 *
 * @param[in] inst  OSPI 实例号
 * @return OSPI_TypeDef 指针，无效实例返回 NULL
 */
static inline OSPI_TypeDef *ospi_get_base(ospi_instance_t inst)
{
    if ((uint32_t)inst >= (uint32_t)s_ospi_count) {
        return NULL;
    }
    return s_ospi_regs[inst];
}

/**
 * @brief 读取 OSPI 寄存器
 *
 * @param[in] base   OSPI 外设基地址
 * @param[in] offset 寄存器偏移量
 * @return 寄存器值
 */
static uint32_t ospi_read_reg(OSPI_TypeDef *base, uint32_t offset)
{
    return *(volatile uint32_t *)((uint32_t)base + offset);
}

/**
 * @brief 写入 OSPI 寄存器
 *
 * @param[in] base   OSPI 外设基地址
 * @param[in] offset 寄存器偏移量
 * @param[in] value  要写入的值
 */
static void ospi_write_reg(OSPI_TypeDef *base, uint32_t offset, uint32_t value)
{
    *(volatile uint32_t *)((uint32_t)base + offset) = value;
}

/**
 * @brief 修改 OSPI 寄存器位域
 *
 * @param[in] base   OSPI 外设基地址
 * @param[in] offset 寄存器偏移量
 * @param[in] mask   位掩码（要修改的位域）
 * @param[in] value  新值
 *
 * @note 特殊处理：对于 OSPI_MEMO_ACC1 寄存器（偏移 0x2C），bit[28] 必须强制清零
 */
static void ospi_modify_reg(OSPI_TypeDef *base, uint32_t offset,
                                   uint32_t mask, uint32_t value)
{
    uint32_t reg = ospi_read_reg(base, offset);
    reg = (reg & ~mask) | (value & mask);

    if (offset == OSPI_MEMO_ACC1_OFFSET) {
        reg &= OSPI_MEMO_ACC1_BIT28_MASK;
    }

    ospi_write_reg(base, offset, reg);
}

/**
 * @brief 等待 OSPI 状态标志
 *
 * 轮询 OSPI STATUS 寄存器，等待指定标志位达到期望状态。
 *
 * @param[in] inst       OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] mask       状态掩码（使用 ospi_irq_type_t）
 * @param[in] wait_set   true: 等待标志置位, false: 等待标志清零
 * @param[in] timeout_ms 超时时间（毫秒），0 使用默认值 OSPI_TIMEOUT_MS
 * @return true: 成功, false: 超时或实例无效
 *
 * @note 覆盖寄存器：OSPI_STATUS[31:0]
 *
 * @code
 * // 等待 TX FIFO 非满
 * ospi_wait_status(OSPI_1, OSPI_IRQ_TX_FIFO_FULL, false, 1000);
 * @endcode
 */
bool ospi_wait_status(ospi_instance_t inst, uint32_t mask,
                      bool wait_set, uint32_t timeout_ms)
{
    OSPI_TypeDef *base;
    uint64_t start;
    uint32_t timeout;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    start = system_get_tick();
    timeout = timeout_ms;

    if (timeout_ms == 0U) {
        timeout = OSPI_TIMEOUT_MS;
    }

    while (((base->STATUS & mask) != 0U) != wait_set) {
        if (system_elapsed(start, timeout)) {
            return false;
        }
    }
    return true;
}

//===========================================
// 外设时钟与复位 API
//===========================================

/**
 * @brief 使能 OSPI 时钟
 *
 * 通过 RCC 使能指定 OSPI 实例的外设时钟。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return true: 成功, false: 实例无效
 *
 * @note 覆盖寄存器：RCC.OSPI1CKEN / OSPI2CKEN
 * @note 在调用任何 OSPI 配置 API 之前应先使能时钟
 *
 * @code
 * ospi_enable_clock(OSPI_1);
 * ospi_init_standard(OSPI_1, &baud, &basic, &proto);
 * @endcode
 */
bool ospi_enable_clock(ospi_instance_t inst)
{
    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    clock_periph_enable((inst == OSPI_1) ? CLK_OSPI1 : CLK_OSPI2);
    return true;
}

/**
 * @brief 禁用 OSPI 时钟
 *
 * 通过 RCC 禁用指定 OSPI 实例的外设时钟。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return true: 成功, false: 实例无效
 *
 * @note 覆盖寄存器：RCC.OSPI1CKEN / OSPI2CKEN
 * @note 禁用时钟前应确保 OSPI 已停止传输
 *
 * @code
 * ospi_stop(OSPI_1);
 * ospi_disable_clock(OSPI_1);
 * @endcode
 */
bool ospi_disable_clock(ospi_instance_t inst)
{
    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    clock_periph_disable((inst == OSPI_1) ? CLK_OSPI1 : CLK_OSPI2);
    return true;
}

/**
 * @brief 复位 OSPI 外设
 *
 * 通过 RCC 复位指定 OSPI 实例，将所有寄存器恢复为默认值。
 * 复位脉冲由硬件自动撤销，无需手动清除。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return true: 成功, false: 实例无效
 *
 * @note 覆盖寄存器：RCC.OSPI1RST / OSPI2RST
 * @note 复位后需重新调用 ospi_init_xxx() 配置 OSPI
 * @note 复位同时会清空 FIFO 和内部状态机
 *
 * @code
 * ospi_reset(OSPI_1);
 * ospi_init_standard(OSPI_1, &baud, &basic, &proto);
 * @endcode
 */
bool ospi_reset(ospi_instance_t inst)
{
    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    clock_periph_reset((inst == OSPI_1) ? CLK_OSPI1 : CLK_OSPI2);
    return true;
}

//===========================================
// 第1层：快速初始化 API
//===========================================

/**
 * @brief 标准 SPI 模式快速初始化
 *
 * 一行代码完成标准 SPI 模式的基本配置，包括波特率、时钟极性/相位、
 * 传输线数、片选和收发使能。
 *
 * @param[in] inst     OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] baudrate 波特率分频配置
 * @param[in] basic    基本模式配置（CPOL、CPHA、MSB/LSB、IO 模式）
 * @param[in] protocol 协议配置（线数、速率、存储器类型）
 * @return true: 初始化成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_BAUD, OSPI_CTL, OSPI_CS, OSPI_OUT_EN,
 *       OSPI_TX_CTL, OSPI_RX_CTL
 * @note 标准 SPI 默认使能 MOSI[bit0] 和 MISO[bit1] 输出
 *
 * @code
 * ospi_baudrate_config_t baud = { .div1 = 4, .div2 = 2 };
 * ospi_basic_config_t basic = {
 *     .cpol = OSPI_CPOL_LOW,
 *     .cpha = OSPI_CPHA_ODD_EDGE,
 *     .msb_first = true,
 *     .hw_io_mode = false,
 * };
 * ospi_protocol_config_t proto = {
 *     .line = OSPI_LINE_1,
 *     .rate = OSPI_RATE_SDR,
 *     .mem_type = OSPI_MEM_XCC_ELA,
 * };
 * ospi_init_standard(OSPI_1, &baud, &basic, &proto);
 * @endcode
 */
bool ospi_init_standard(ospi_instance_t inst, const ospi_baudrate_config_t *baudrate,
                        const ospi_basic_config_t *basic, const ospi_protocol_config_t *protocol)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }
    if ((baudrate == NULL) || (basic == NULL) || (protocol == NULL)) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    // 校验 DIV1 偶数约束（2-254 之间的偶数）
    if (baudrate->div1 < 2 || baudrate->div1 > 254 || (baudrate->div1 & 1U) != 0) {
        return false;
    }

    // 复位收发控制
    ospi_write_reg(base, OSPI_TX_CTL_OFFSET, 0U);
    ospi_write_reg(base, OSPI_RX_CTL_OFFSET, 0U);

    // 配置波特率
    ospi_write_reg(base, OSPI_BAUD_OFFSET,
                   ((uint32_t)baudrate->div1 & 0xFFU) |
                   (((uint32_t)baudrate->div2 & 0xFFU) << 8U));

    // 配置控制寄存器：主机模式 + 基本参数 + 协议参数
    uint32_t ctl = (1U << 0);  // MST_MODE: 主机模式
    ctl |= ((uint32_t)basic->cpha & 0x1U) << 2;   // CPHA
    ctl |= ((uint32_t)basic->cpol & 0x1U) << 3;   // CPOL
    ctl |= (basic->msb_first ? 0U : 1U) << 4;     // LSB_FIRST: 0=MSB在前, 1=LSB在前
    ctl |= ((uint32_t)protocol->line & 0x3U) << 5; // X_MODE
    ctl |= (basic->hw_io_mode ? 1U : 0U) << 7;    // IO_MODE
    ctl |= ((uint32_t)protocol->rate & 0x1U) << 8; // DTRM
    ctl |= ((uint32_t)protocol->mem_type & 0x3U) << 21; // MEM_MODE
    ospi_write_reg(base, OSPI_CTL_OFFSET, ctl);

    // 配置片选：选择 CS0
    ospi_write_reg(base, OSPI_CS_OFFSET, (uint32_t)OSPI_CS_0);

    // 根据线宽配置管脚输出使能
    uint32_t out_en = (protocol->line == OSPI_LINE_1) ? 0x03U :
                      (protocol->line == OSPI_LINE_2) ? 0x03U :
                      (protocol->line == OSPI_LINE_4) ? 0x0FU : 0xFFU;
    ospi_write_reg(base, OSPI_OUT_EN_OFFSET, out_en);

    // 注意：TX_EN/RX_EN 由 ospi_start_tx/rx 在启动传输时使能

    s_ospi_config_state[inst].is_configured = true;
    s_ospi_config_state[inst].baudrate = *baudrate;
    s_ospi_config_state[inst].basic = *basic;
    s_ospi_config_state[inst].protocol = *protocol;

    return true;
}

/**
 * @brief 内存映射模式快速初始化
 *
 * 一行代码完成内存映射模式的基本配置，适用于连接 NOR Flash 等
 * 需要 XIP（就地执行）的外部存储器。
 *
 * @param[in] inst     OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] baudrate 波特率分频配置
 * @param[in] basic    基本模式配置（CPOL、CPHA、MSB/LSB、IO 模式）
 * @param[in] mem      内存映射配置（读写命令、地址宽度、dummy 周期等）
 * @return true: 初始化成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_BAUD, OSPI_CTL, OSPI_CS, OSPI_CMD,
 *       OSPI_MEMO_ACC1, OSPI_MEMO_ACC2, OSPI_ALTER_BYTE, OSPI_CS_TOUT_VAL
 * @note CTL 仅设置 MST_MODE/CPHA/CPOL/LSB_FIRST/IO_MODE，X_MODE/DTRM/MEM_MODE
 *       需通过 ospi_config_protocol() 单独配置
 * @note 内存映射模式自动使能 ACC_EN（bit[0]），片选超时使用默认值 0
 *
 * @code
 * ospi_baudrate_config_t baud = { .div1 = 2, .div2 = 1 };
 * ospi_basic_config_t basic = { .cpol = OSPI_CPOL_LOW, .cpha = OSPI_CPHA_ODD_EDGE,
 *                                .msb_first = true, .hw_io_mode = false };
 * ospi_memory_mapped_config_t mem = {
 *     .read_cmd = 0x03, .write_cmd = 0x02,
 *     .instr_mode = OSPI_INSTR_1LINE, .addr_mode = OSPI_ADDR_1LINE,
 *     .data_mode = OSPI_DATA_1LINE, .addr_width = OSPI_ADDR_24BIT,
 * };
 * ospi_init_memory_mapped(OSPI_1, &baud, &basic, &mem);
 * @endcode
 */
bool ospi_init_memory_mapped(ospi_instance_t inst, const ospi_baudrate_config_t *baudrate,
                             const ospi_basic_config_t *basic, const ospi_memory_mapped_config_t *mem)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }
    if ((baudrate == NULL) || (basic == NULL) || (mem == NULL)) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    // 校验 DIV1 偶数约束（2-254 之间的偶数）
    if (baudrate->div1 < 2 || baudrate->div1 > 254 || (baudrate->div1 & 1U) != 0) {
        return false;
    }

    // 复位收发控制
    ospi_write_reg(base, OSPI_TX_CTL_OFFSET, 0U);
    ospi_write_reg(base, OSPI_RX_CTL_OFFSET, 0U);

    // 配置波特率
    ospi_write_reg(base, OSPI_BAUD_OFFSET,
                   ((uint32_t)baudrate->div1 & 0xFFU) |
                   (((uint32_t)baudrate->div2 & 0xFFU) << 8U));

    // 配置控制寄存器
    uint32_t ctl = (1U << 0);  // MST_MODE
    ctl |= ((uint32_t)basic->cpha & 0x1U) << 2;
    ctl |= ((uint32_t)basic->cpol & 0x1U) << 3;
    ctl |= (basic->msb_first ? 0U : 1U) << 4;     // LSB_FIRST: 0=MSB在前, 1=LSB在前
    ctl |= (basic->hw_io_mode ? 1U : 0U) << 7;
    ospi_write_reg(base, OSPI_CTL_OFFSET, ctl);

    // 配置命令寄存器（写命令在高16位，读命令在低16位）
    ospi_write_reg(base, OSPI_CMD_OFFSET,
                   ((uint32_t)mem->write_cmd << 16U) | (uint32_t)mem->read_cmd);

    // 配置内存访问寄存器1
    uint32_t acc1 = 0U;
    acc1 |= ((uint32_t)mem->instr_mode & 0x3U) << 19; // INSTR_MODE
    acc1 |= ((uint32_t)mem->addr_mode & 0x3U) << 21;  // ADDR_MODE
    acc1 |= ((uint32_t)mem->data_mode & 0x3U) << 25;  // DATA_MODE
    acc1 |= ((uint32_t)mem->addr_width & 0x3U) << 17; // ADDR_WIDTH
    acc1 |= ((uint32_t)mem->read_dummy_cycles & 0x1FU) << 12; // DUMMY_CYCLE_SIZE
    acc1 |= (mem->continuous_mode ? 1U : 0U) << 3;    // CON_MODE_EN
    acc1 |= (mem->send_instr_once ? 1U : 0U) << 4;    // SEND_INSTR_ONCE_EN
    acc1 |= (mem->rd_db_en ? 1U : 0U) << 11;          // RD_DB_EN
    acc1 |= (mem->wr_db_en ? 1U : 0U) << 10;          // WR_DB_EN
    acc1 |= (mem->alternate_bytes_enable ? 1U : 0U) << 6; // RD_AB_EN
    acc1 |= (mem->alternate_bytes_enable ? 1U : 0U) << 5; // WR_AB_EN
    acc1 |= ((uint32_t)mem->alternate_bytes_mode & 0x3U) << 23; // ALTER_BYTE_MODE
    acc1 |= ((uint32_t)mem->alternate_bytes_size & 0x3U) << 7;  // ALTER_BYTE_SIZE
    acc1 |= (1U << 0); // ACC_EN: 使能内存访问
    ospi_write_reg(base, OSPI_MEMO_ACC1_OFFSET, acc1);

    // 配置内存访问寄存器2：突发长度 + 写 Dummy 周期
    uint32_t acc2_mask = (0xFUL << 8) | (0xFUL << 15) | (0x1FUL << 27);
    uint32_t acc2_value = ((uint32_t)mem->read_burst & 0xFU) << 8 |
                          ((uint32_t)mem->write_burst & 0xFU) << 15 |
                          ((uint32_t)mem->write_dummy_cycles & 0x1FU) << 27;
    ospi_modify_reg(base, OSPI_MEMO_ACC2_OFFSET, acc2_mask, acc2_value);

    // 配置替代字节
    ospi_write_reg(base, OSPI_ALTER_BYTE_OFFSET, 0U);

    // 配置片选
    ospi_write_reg(base, OSPI_CS_OFFSET, (uint32_t)OSPI_CS_0);

    // 根据数据模式配置管脚输出使能
    if (!basic->hw_io_mode) {
        // 软件切换模式下，需要手动配置 OUT_EN
        uint32_t out_en = (mem->data_mode == OSPI_DATA_1LINE) ? 0x03U :
                          (mem->data_mode == OSPI_DATA_2LINE) ? 0x03U : 0x0FU;
        ospi_write_reg(base, OSPI_OUT_EN_OFFSET, out_en);
    }
    // 硬件自动切换模式下，OUT_EN 由硬件控制，无需配置

    // 配置片选超时（默认禁用）
    ospi_write_reg(base, OSPI_CS_TOUT_VAL_OFFSET, 0U);

    // 注意：TX_EN/RX_EN 由 ospi_start_tx/rx 在启动传输时使能

    s_ospi_config_state[inst].is_configured = true;
    s_ospi_config_state[inst].baudrate = *baudrate;
    s_ospi_config_state[inst].mem_mapped = *mem;

    return true;
}

/**
 * @brief HyperBus 协议快速初始化
 *
 * 一行代码完成 HyperBus 协议的基本配置，包括延迟控制、突发长度
 * 和读写延迟参数。HyperBus 使用 8 线 DTR 传输。
 *
 * @param[in] inst     OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] baudrate 波特率分频配置
 * @param[in] hyper    HyperBus 配置（突发类型、延迟控制 LC0/LC1）
 * @return true: 初始化成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_BAUD, OSPI_CTL, OSPI_CS, OSPI_OUT_EN,
 *       OSPI_MEMO_ACC1, OSPI_MEMO_ACC2, OSPI_TX_CTL, OSPI_RX_CTL
 * @note HyperBus 模式固定为 8 线 DTR，MEM_MODE 设置为 OSPI_MEM_HYPER(2)
 * @note 默认使能全部 8 个 IO 线（IO0-IO7）
 * @note 高速 HyperBus 模式建议通过 DLYB（dlyb.h）配置延迟校准
 *
 * @code
 * ospi_baudrate_config_t baud = { .div1 = 2, .div2 = 1 };
 * ospi_hyperbus_config_t hyper = {
 *     .burst_type = OSPI_HYPER_LINEAR,
 *     .lc0 = 6,
 *     .lc1 = 6,
 * };
 * ospi_init_hyperbus(OSPI_1, &baud, &hyper);
 * @endcode
 */
bool ospi_init_hyperbus(ospi_instance_t inst, const ospi_baudrate_config_t *baudrate,
                        const ospi_hyperbus_config_t *hyper)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }
    if ((baudrate == NULL) || (hyper == NULL)) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    // 校验 DIV1 偶数约束（2-254 之间的偶数）
    if (baudrate->div1 < 2 || baudrate->div1 > 254 || (baudrate->div1 & 1U) != 0) {
        return false;
    }

    // 复位收发控制
    ospi_write_reg(base, OSPI_TX_CTL_OFFSET, 0U);
    ospi_write_reg(base, OSPI_RX_CTL_OFFSET, 0U);

    // 配置波特率
    ospi_write_reg(base, OSPI_BAUD_OFFSET,
                   ((uint32_t)baudrate->div1 & 0xFFU) |
                   (((uint32_t)baudrate->div2 & 0xFFU) << 8U));

    // 配置控制寄存器：主机模式 + HyperBus 8线DTR
    uint32_t ctl = (1U << 0);    // MST_MODE
    ctl |= (3U << 5);           // X_MODE: 8线
    ctl |= (1U << 8);           // DTRM: 双沿采样
    // CPOL=0, CPHA=0: DTR 模式强制要求（bit3/bit2 保持默认 0）
    ctl |= ((uint32_t)OSPI_MEM_HYPER & 0x3U) << 21; // MEM_MODE
    ospi_write_reg(base, OSPI_CTL_OFFSET, ctl);

    // 配置内存访问寄存器1：使能 HyperBus 模式
    uint32_t acc1 = 0U;
    acc1 |= (1U << 0);          // ACC_EN
    acc1 |= ((uint32_t)hyper->burst_type & 0x1U) << 27;  // HYPER_BT: 突发类型
    acc1 |= (3U << 17);         // ADDR_WIDTH: 32bit（HyperBus 强制要求）
    ospi_write_reg(base, OSPI_MEMO_ACC1_OFFSET, acc1);

    // 配置内存访问寄存器2：延迟控制和突发长度（固定 64B，如需其他值请使用 ospi_config_memory_mapped）
    uint32_t acc2 = 0U;
    acc2 |= ((uint32_t)hyper->lc0 & 0xFU);            // HXSPI_LC0
    acc2 |= ((uint32_t)hyper->lc1 & 0xFU) << 4;      // HXSPI_LC1
    acc2 |= ((uint32_t)OSPI_BURST_64B & 0xFU) << 8;  // RBL: 读突发长度（固定 64B）
    acc2 |= ((uint32_t)OSPI_BURST_64B & 0xFU) << 15; // WBL: 写突发长度（固定 64B）
    ospi_write_reg(base, OSPI_MEMO_ACC2_OFFSET, acc2);

    // 配置片选和全部8个IO线
    ospi_write_reg(base, OSPI_CS_OFFSET, (uint32_t)OSPI_CS_0);
    ospi_write_reg(base, OSPI_OUT_EN_OFFSET, 0xFFU);

    // 使能收发
    // 注意：TX_EN/RX_EN 由 ospi_start_tx/rx 在启动传输时使能

    s_ospi_config_state[inst].is_configured = true;
    s_ospi_config_state[inst].baudrate = *baudrate;

    return true;
}

/**
 * @brief xOSPI 协议快速初始化
 *
 * 一行代码完成 xOSPI（扩展 OSPI）协议的基本配置。xOSPI 使用
 * 8 线 SDR 模式，适用于需要高带宽但不需要双沿采样的场景。
 *
 * @param[in] inst     OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] baudrate 波特率分频配置
 * @return true: 初始化成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_BAUD, OSPI_CTL, OSPI_CS, OSPI_OUT_EN,
 *       OSPI_TX_CTL, OSPI_RX_CTL
 * @note xOSPI 模式固定为 8 线 SDR，MEM_MODE 设置为 OSPI_MEM_XOSPI(3)
 * @note 默认使能全部 8 个 IO 线（IO0-IO7）
 *
 * @code
 * ospi_baudrate_config_t baud = { .div1 = 2, .div2 = 1 };
 * ospi_init_xospi(OSPI_1, &baud);
 * @endcode
 */
bool ospi_init_xospi(ospi_instance_t inst, const ospi_baudrate_config_t *baudrate)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }
    if (baudrate == NULL) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    // 校验 DIV1 偶数约束（2-254 之间的偶数）
    if (baudrate->div1 < 2 || baudrate->div1 > 254 || (baudrate->div1 & 1U) != 0) {
        return false;
    }

    // 复位收发控制
    ospi_write_reg(base, OSPI_TX_CTL_OFFSET, 0U);
    ospi_write_reg(base, OSPI_RX_CTL_OFFSET, 0U);

    // 配置波特率
    ospi_write_reg(base, OSPI_BAUD_OFFSET,
                   ((uint32_t)baudrate->div1 & 0xFFU) |
                   (((uint32_t)baudrate->div2 & 0xFFU) << 8U));

    // 配置控制寄存器：主机模式 + xOSPI 8线SDR
    uint32_t ctl = (1U << 0);    // MST_MODE
    ctl |= (3U << 5);           // X_MODE: 8线
    ctl |= ((uint32_t)OSPI_MEM_XOSPI & 0x3U) << 21; // MEM_MODE
    ospi_write_reg(base, OSPI_CTL_OFFSET, ctl);

    // 配置片选和全部8个IO线
    ospi_write_reg(base, OSPI_CS_OFFSET, (uint32_t)OSPI_CS_0);
    ospi_write_reg(base, OSPI_OUT_EN_OFFSET, 0xFFU);

    // 注意：TX_EN/RX_EN 由 ospi_start_tx/rx 在启动传输时使能

    s_ospi_config_state[inst].is_configured = true;
    s_ospi_config_state[inst].baudrate = *baudrate;

    return true;
}

//===========================================
// 第2层：基础配置 API
//===========================================

/**
 * @brief 配置波特率分频参数
 *
 * @param[in] inst     OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] baudrate 波特率分频配置
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_BAUD.DIV1[7:0], OSPI_BAUD.DIV2[15:8]
 * @note DIV1 必须是 2-254 之间的偶数，DIV2 为 0-255
 *
 * @code
 * ospi_baudrate_config_t baud = { .div1 = 4, .div2 = 2 };
 * ospi_config_baudrate(OSPI_1, &baud);
 * @endcode
 */
bool ospi_config_baudrate(ospi_instance_t inst, const ospi_baudrate_config_t *baudrate)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }
    if (baudrate == NULL) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    if (baudrate->div1 < 2 || baudrate->div1 > 254 || (baudrate->div1 & 1U) != 0) {
        return false;
    }

    ospi_write_reg(base, OSPI_BAUD_OFFSET,
                   ((uint32_t)baudrate->div1 & 0xFFU) |
                   (((uint32_t)baudrate->div2 & 0xFFU) << 8U));

    s_ospi_config_state[inst].baudrate = *baudrate;

    return true;
}

/**
 * @brief 配置基本模式参数
 *
 * @param[in] inst   OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] basic  基本模式配置（CPOL、CPHA、MSB/LSB、IO 模式）
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_CTL.CPHA[2], OSPI_CTL.CPOL[3],
 *       OSPI_CTL.LSB_FIRST[4], OSPI_CTL.IO_MODE[7]
 *
 * @code
 * ospi_basic_config_t basic = {
 *     .cpol = OSPI_CPOL_LOW,
 *     .cpha = OSPI_CPHA_ODD_EDGE,
 *     .msb_first = true,
 *     .hw_io_mode = false,
 * };
 * ospi_config_mode(OSPI_1, &basic);
 * @endcode
 */
bool ospi_config_mode(ospi_instance_t inst, const ospi_basic_config_t *basic)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }
    if (basic == NULL) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    uint32_t ctl = ospi_read_reg(base, OSPI_CTL_OFFSET);
    ctl &= ~((1UL << 2) | (1UL << 3) | (1UL << 4) | (1UL << 7));
    ctl |= ((uint32_t)basic->cpha & 0x1U) << 2;
    ctl |= ((uint32_t)basic->cpol & 0x1U) << 3;
    ctl |= (basic->msb_first ? 0U : 1U) << 4;
    ctl |= (basic->hw_io_mode ? 1U : 0U) << 7;
    ospi_write_reg(base, OSPI_CTL_OFFSET, ctl);

    s_ospi_config_state[inst].basic = *basic;

    return true;
}

/**
 * @brief 配置协议参数
 *
 * @param[in] inst     OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] protocol 协议配置（线数、速率、存储器类型）
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_CTL.X_MODE[6:5], OSPI_CTL.DTRM[8],
 *       OSPI_CTL.MEM_MODE[22:21]
 *
 * @code
 * ospi_protocol_config_t proto = {
 *     .line = OSPI_LINE_4,
 *     .rate = OSPI_RATE_SDR,
 *     .mem_type = OSPI_MEM_XCC_ELA,
 * };
 * ospi_config_protocol(OSPI_1, &proto);
 * @endcode
 */
bool ospi_config_protocol(ospi_instance_t inst, const ospi_protocol_config_t *protocol)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }
    if (protocol == NULL) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    uint32_t ctl = ospi_read_reg(base, OSPI_CTL_OFFSET);
    ctl &= ~((0x3UL << 5) | (1UL << 8) | (0x3UL << 21));
    ctl |= ((uint32_t)protocol->line & 0x3U) << 5;
    ctl |= ((uint32_t)protocol->rate & 0x1U) << 8;
    ctl |= ((uint32_t)protocol->mem_type & 0x3U) << 21;
    ospi_write_reg(base, OSPI_CTL_OFFSET, ctl);

    s_ospi_config_state[inst].protocol = *protocol;

    return true;
}

/**
 * @brief 配置内存映射参数
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] mem  内存映射配置（读写命令、地址宽度、dummy 周期等）
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_MEMO_ACC1（全部位域）,
 *       OSPI_CMD.RD_CMD[15:0]/WR_CMD[31:16],
 *       OSPI_ALTER_BYTE[31:0],
 *       OSPI_MEMO_ACC2（RBL、WBL 等）
 *
 * @code
 * ospi_memory_mapped_config_t mem = {
 *     .read_cmd = 0x03, .write_cmd = 0x02,
 *     .instr_mode = OSPI_INSTR_1LINE, .addr_mode = OSPI_ADDR_1LINE,
 *     .data_mode = OSPI_DATA_1LINE, .addr_width = OSPI_ADDR_24BIT,
 * };
 * ospi_config_memory_mapped(OSPI_1, &mem);
 * @endcode
 */
bool ospi_config_memory_mapped(ospi_instance_t inst, const ospi_memory_mapped_config_t *mem)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }
    if (mem == NULL) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    // 配置命令寄存器
    ospi_write_reg(base, OSPI_CMD_OFFSET,
                   ((uint32_t)mem->write_cmd << 16U) | (uint32_t)mem->read_cmd);

    // 配置替代字节
    ospi_write_reg(base, OSPI_ALTER_BYTE_OFFSET, 0U);

    // 配置内存访问寄存器1：读-改-写，保留 ACC_EN/CS_TOUT_EN/HYPER_BT/ONCE_INSTR_CLR 等位
    // bit28 手册要求"需保持为零"，通过 mask 强制清零（防御性编程）
    uint32_t acc1_mask = (0x1FU << 12) |   // DUMMY_CYCLE_SIZE
                         (0x3U << 17) |    // ADDR_WIDTH
                         (0x3U << 19) |    // INSTR_MODE
                         (0x3U << 21) |    // ADDR_MODE
                         (0x3U << 23) |    // ALTER_BYTE_MODE
                         (0x3U << 25) |    // DATA_MODE
                         (0x1U << 3) |     // CON_MODE_EN
                         (0x1U << 4) |     // SEND_INSTR_ONCE_EN
                         (0x1U << 5) |     // WR_AB_EN
                         (0x1U << 6) |     // RD_AB_EN
                         (0x3U << 7) |     // ALTER_BYTE_SIZE
                         (0x1U << 10) |    // WR_DB_EN
                         (0x1U << 11) |    // RD_DB_EN
                         (0x1U << 28);     // RSV bit28（强制清零）
    uint32_t acc1 = ((uint32_t)mem->read_dummy_cycles & 0x1FU) << 12;
    acc1 |= ((uint32_t)mem->addr_width & 0x3U) << 17;
    acc1 |= ((uint32_t)mem->instr_mode & 0x3U) << 19;
    acc1 |= ((uint32_t)mem->addr_mode & 0x3U) << 21;
    acc1 |= ((uint32_t)mem->alternate_bytes_mode & 0x3U) << 23;
    acc1 |= ((uint32_t)mem->data_mode & 0x3U) << 25;
    acc1 |= (mem->continuous_mode ? 1U : 0U) << 3;
    acc1 |= (mem->send_instr_once ? 1U : 0U) << 4;
    acc1 |= (mem->alternate_bytes_enable ? 1U : 0U) << 5;
    acc1 |= (mem->alternate_bytes_enable ? 1U : 0U) << 6;
    acc1 |= ((uint32_t)mem->alternate_bytes_size & 0x3U) << 7;
    acc1 |= (mem->wr_db_en ? 1U : 0U) << 10;
    acc1 |= (mem->rd_db_en ? 1U : 0U) << 11;
    ospi_modify_reg(base, OSPI_MEMO_ACC1_OFFSET, acc1_mask, acc1);

    // 配置内存访问寄存器2：突发长度 + 写 Dummy 周期
    uint32_t acc2_mask = (0xFUL << 8) | (0xFUL << 15) | (0x1FUL << 27);
    uint32_t acc2_value = ((uint32_t)mem->read_burst & 0xFU) << 8 |
                          ((uint32_t)mem->write_burst & 0xFU) << 15 |
                          ((uint32_t)mem->write_dummy_cycles & 0x1FU) << 27;
    ospi_modify_reg(base, OSPI_MEMO_ACC2_OFFSET, acc2_mask, acc2_value);

    s_ospi_config_state[inst].mem_mapped = *mem;

    return true;
}

/**
 * @brief 配置 DMA 参数
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] dma  DMA 配置（收发使能、FIFO 水平）
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_TX_CTL.TX_DMA_REQ_EN[3], OSPI_TX_CTL.TX_DMA_LEVEL[7:4],
 *       OSPI_RX_CTL.RX_DMA_REQ_EN[3], OSPI_RX_CTL.RX_DMA_LEVEL[7:4]
 *
 * @code
 * ospi_dma_config_t dma = {
 *     .tx_enable = true, .rx_enable = true,
 *     .tx_level = OSPI_DMA_LEVEL_8, .rx_level = OSPI_DMA_LEVEL_8,
 * };
 * ospi_config_dma(OSPI_1, &dma);
 * @endcode
 */
bool ospi_config_dma(ospi_instance_t inst, const ospi_dma_config_t *dma)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }
    if (dma == NULL) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    // 配置 TX_CTL: TX_DMA_REQ_EN[3], TX_DMA_LEVEL[7:4]
    uint32_t tx_ctl = ospi_read_reg(base, OSPI_TX_CTL_OFFSET);
    tx_ctl &= ~((1UL << 3) | (0xFUL << 4));
    tx_ctl |= (dma->tx_enable ? 1U : 0U) << 3;
    tx_ctl |= ((uint32_t)dma->tx_level & 0xFU) << 4;
    ospi_write_reg(base, OSPI_TX_CTL_OFFSET, tx_ctl);

    // 配置 RX_CTL: RX_DMA_REQ_EN[3], RX_DMA_LEVEL[7:4]
    uint32_t rx_ctl = ospi_read_reg(base, OSPI_RX_CTL_OFFSET);
    rx_ctl &= ~((1UL << 3) | (0xFUL << 4));
    rx_ctl |= (dma->rx_enable ? 1U : 0U) << 3;
    rx_ctl |= ((uint32_t)dma->rx_level & 0xFU) << 4;
    ospi_write_reg(base, OSPI_RX_CTL_OFFSET, rx_ctl);

    s_ospi_config_state[inst].dma = *dma;

    return true;
}

/**
 * @brief 配置片选
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] cs   片选配置（CS 选择、映射使能）
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_CS.SPI_CS[2:0], OSPI_CS.CSMAP_EN[3]
 * @note CS 枚举值：OSPI_CS_0=1, OSPI_CS_1=2, OSPI_CS_2=4（位掩码）
 *
 * @code
 * ospi_cs_config_t cs = { .cs = OSPI_CS_0, .cs_mapping_enable = false };
 * ospi_config_cs(OSPI_1, &cs);
 * @endcode
 */
bool ospi_config_cs(ospi_instance_t inst, const ospi_cs_config_t *cs)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }
    if (cs == NULL) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    uint32_t cs_val = (uint32_t)cs->cs & 0x7U;
    cs_val |= (cs->cs_mapping_enable ? 1U : 0U) << 3;
    ospi_write_reg(base, OSPI_CS_OFFSET, cs_val);

    s_ospi_config_state[inst].cs = *cs;

    return true;
}

/**
 * @brief 配置 CS 高电平周期
 *
 * @param[in] inst   OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] cycles CS 高电平周期数（0-255）
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_CTL.CS_TIME[18:11]
 *
 * @code
 * ospi_config_cs_time(OSPI_1, 8);  // 8 个时钟周期的 CS 间隔
 * @endcode
 */
bool ospi_config_cs_time(ospi_instance_t inst, uint8_t cycles)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    ospi_modify_reg(base, OSPI_CTL_OFFSET, 0xFFUL << 11,
                    ((uint32_t)cycles & 0xFFU) << 11);

    return true;
}

/**
 * @brief 配置发送等待周期（Dummy 周期）
 *
 * @param[in] inst   OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] cycles 等待周期数（0-31）
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_TXDELAY.TDY[31:0]
 * @note 主模式下每发送一个字节间的 HCLK 等待计数；八线及八线 DTR 模式不支持
 *
 * @code
 * ospi_config_wait_cycles(OSPI_1, 8);  // 8 个 dummy 周期
 * @endcode
 */
bool ospi_config_wait_cycles(ospi_instance_t inst, uint32_t cycles)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    ospi_write_reg(base, OSPI_TXDELAY_OFFSET, cycles);

    return true;
}

/**
 * @brief 配置交替字节
 *
 * @param[in] inst  OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] bytes 交替字节值（32 位）
 * @param[in] size  交替字节大小
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_ALTER_BYTE[31:0],
 *       OSPI_MEMO_ACC1.ALTER_BYTE_SIZE[8:7]
 *
 * @code
 * ospi_config_alternate_bytes(OSPI_1, 0xA5A5A5A5U, OSPI_ALT_32BIT);
 * @endcode
 */
bool ospi_config_alternate_bytes(ospi_instance_t inst, uint32_t bytes, ospi_alt_size_t size)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    // 配置替代字节值
    ospi_write_reg(base, OSPI_ALTER_BYTE_OFFSET, bytes);

    // 配置替代字节大小
    ospi_modify_reg(base, OSPI_MEMO_ACC1_OFFSET, 0x3UL << 7,
                    ((uint32_t)size & 0x3U) << 7);

    return true;
}

/**
 * @brief 配置 FIFO 模式
 *
 * @param[in] inst    OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] tx_mode 发送 FIFO 模式
 * @param[in] rx_mode 接收 FIFO 模式
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_CTL.FW_MODE[29:28], OSPI_CTL.FR_MODE[31:30]
 *
 * @code
 * ospi_config_fifo_mode(OSPI_1, OSPI_FIFO_HALF, OSPI_FIFO_HALF);
 * @endcode
 */
bool ospi_config_fifo_mode(ospi_instance_t inst, ospi_fifo_mode_t tx_mode,
                           ospi_fifo_mode_t rx_mode)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    uint32_t mask = (0x3UL << 28) | (0x3UL << 30);
    uint32_t value = (((uint32_t)tx_mode & 0x3U) << 28) |
                     (((uint32_t)rx_mode & 0x3U) << 30);
    ospi_modify_reg(base, OSPI_CTL_OFFSET, mask, value);

    return true;
}

/**
 * @brief 配置数据掩码
 *
 * @param[in] inst     OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] enable   使能数据掩码
 * @param[in] mask_odd 数据掩码控制（true: 掩码奇数位, false: 掩码偶数位）
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_CTL.DM_EN[26], OSPI_CTL.DMCTRL[27]
 *
 * @code
 * ospi_config_data_mask(OSPI_1, true, false);  // 使能数据掩码，掩码偶数位
 * @endcode
 */
bool ospi_config_data_mask(ospi_instance_t inst, bool enable, bool mask_odd)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    uint32_t mask = (1UL << 26) | (1UL << 27);
    uint32_t value = (enable ? 1UL : 0UL) << 26 | (mask_odd ? 0UL : 1UL) << 27;
    ospi_modify_reg(base, OSPI_CTL_OFFSET, mask, value);

    return true;
}

/**
 * @brief 配置输出延迟
 *
 * @param[in] inst  OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] delay 输出延迟值（0-3 个时钟周期）
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_TX_CTL.OUTDLY[17:16]
 *
 * @code
 * ospi_config_output_delay(OSPI_1, 2);  // 2 个时钟周期输出延迟
 * @endcode
 */
bool ospi_config_output_delay(ospi_instance_t inst, uint8_t delay)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    ospi_modify_reg(base, OSPI_TX_CTL_OFFSET, 0x3UL << 16,
                    ((uint32_t)delay & 0x3U) << 16);

    return true;
}

/**
 * @brief 配置读时序参数
 *
 * 同时配置读 Dummy 周期数和采样移位值，用于时序诊断和调试。
 * 函数会临时禁用内存映射，配置完成后恢复。
 *
 * @param[in] inst               OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] read_dummy_cycles  读 Dummy 周期数（0-31）
 * @param[in] sample_shift       采样移位值（0-15）
 * @return true: 配置成功, false: 参数无效或恢复失败
 *
 * @note 覆盖寄存器：OSPI_MEMO_ACC1.DUMMY_CYCLE_SIZE[16:12],
 *       OSPI_RX_CTL.SSHIFT[31:28]
 * @note 用于调试场景下的时序扫描
 *
 * @code
 * ospi_config_read_timing(OSPI_1, 8, 4);  // dummy=8, shift=4
 * @endcode
 */
bool ospi_config_read_timing(ospi_instance_t inst, uint8_t read_dummy_cycles,
                             uint8_t sample_shift)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    ospi_disable_memory_mapped(inst);

    // 配置 Dummy 周期：MEMO_ACC1.DUMMY_CYCLE_SIZE[16:12]
    ospi_modify_reg(base, OSPI_MEMO_ACC1_OFFSET, 0x1FUL << 12,
                    ((uint32_t)read_dummy_cycles & 0x1FU) << 12);

    // 配置采样移位：RX_CTL.SSHIFT[31:28]
    ospi_modify_reg(base, OSPI_RX_CTL_OFFSET, 0xFUL << 28,
                    ((uint32_t)sample_shift & 0xFU) << 28);

    return ospi_enable_memory_mapped(inst);
}

/**
 * @brief 配置读写时序参数
 *
 * 同时配置读 Dummy、采样移位和输出延迟，用于三维时序扫描。
 * 函数会临时禁用内存映射，配置完成后恢复。
 *
 * @param[in] inst               OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] read_dummy_cycles  读 Dummy 周期数（0-31）
 * @param[in] sample_shift       采样移位值（0-15）
 * @param[in] output_delay       输出延迟值（0-3）
 * @return true: 配置成功, false: 参数无效或恢复失败
 *
 * @note 覆盖寄存器：OSPI_MEMO_ACC1.DUMMY_CYCLE_SIZE[16:12],
 *       OSPI_RX_CTL.SSHIFT[31:28], OSPI_TX_CTL.OUTDLY[17:16]
 * @note 用于调试场景下的三维时序全扫描
 *
 * @code
 * ospi_config_read_write_timing(OSPI_1, 8, 4, 1);  // dummy=8, shift=4, out_delay=1
 * @endcode
 */
bool ospi_config_read_write_timing(ospi_instance_t inst, uint8_t read_dummy_cycles,
                                   uint8_t sample_shift, uint8_t output_delay)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    ospi_disable_memory_mapped(inst);

    // 配置 Dummy 周期：MEMO_ACC1.DUMMY_CYCLE_SIZE[16:12]
    ospi_modify_reg(base, OSPI_MEMO_ACC1_OFFSET, 0x1FUL << 12,
                    ((uint32_t)read_dummy_cycles & 0x1FU) << 12);

    // 配置采样移位：RX_CTL.SSHIFT[31:28]
    ospi_modify_reg(base, OSPI_RX_CTL_OFFSET, 0xFUL << 28,
                    ((uint32_t)sample_shift & 0xFU) << 28);

    // 配置输出延迟：TX_CTL.OUTDLY[17:16]
    ospi_modify_reg(base, OSPI_TX_CTL_OFFSET, 0x3UL << 16,
                    ((uint32_t)output_delay & 0x3U) << 16);

    return ospi_enable_memory_mapped(inst);
}

/**
 * @brief 配置 Dummy 字节
 *
 * @param[in] inst   OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] dummy  Dummy 字节数（0-255）
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_TX_CTL.DUMMY[15:8]
 * @note 用于常规 SPI 模式下的读操作 Dummy 周期
 *
 * @code
 * ospi_config_dummy_byte(OSPI_1, 8);  // 8 个 Dummy 字节
 * @endcode
 */
bool ospi_config_dummy_byte(ospi_instance_t inst, uint8_t dummy)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    ospi_modify_reg(base, OSPI_TX_CTL_OFFSET, 0xFFUL << 8,
                    ((uint32_t)dummy & 0xFFU) << 8);

    return true;
}

/**
 * @brief 配置 CS 超时参数
 *
 * @param[in] inst    OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] timeout CS 超时计数值（0-65535）
 * @param[in] delay   CS 延迟计数值（0-65535）
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_CS_TOUT_VAL.CS_TOUT_VAL[15:0],
 *       OSPI_CS_TOUT_VAL.CS_DELAY_VAL[31:16]
 *
 * @code
 * ospi_config_cs_timeout(OSPI_1, 1000, 10);  // 超时 1000，延迟 10
 * @endcode
 */
bool ospi_config_cs_timeout(ospi_instance_t inst, uint16_t timeout, uint16_t delay)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    uint32_t value = ((uint32_t)delay << 16U) | ((uint32_t)timeout & 0xFFFFU);
    ospi_write_reg(base, OSPI_CS_TOUT_VAL_OFFSET, value);

    return true;
}

/**
 * @brief 配置 CMD 格式
 *
 * @param[in] inst          OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] two_byte_cmd  使能双字节命令格式
 * @param[in] dual_edge_cmd 使能双沿命令传输
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_MEMO_ACC3.CMD_SD[0], OSPI_MEMO_ACC3.CMD_BYTE[1]
 *
 * @code
 * ospi_config_cmd_format(OSPI_1, true, false);  // 双字节命令，单沿传输
 * @endcode
 */
bool ospi_config_cmd_format(ospi_instance_t inst, bool two_byte_cmd, bool dual_edge_cmd)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    uint32_t mask = (1UL << 0) | (1UL << 1);
    uint32_t value = (dual_edge_cmd ? 1UL : 0UL) | ((two_byte_cmd ? 1UL : 0UL) << 1);
    ospi_modify_reg(base, OSPI_MEMO_ACC3_OFFSET, mask, value);

    return true;
}

/**
 * @brief 直接配置 CMD 寄存器值
 *
 * 直接写入 OSPI_CMD 寄存器，用于需要精确控制命令编码的场景。
 * 与 ospi_config_memory_mapped() 不同，本函数接受完整的 32 位命令值，
 * 低 16 位为读命令，高 16 位为写命令。
 *
 * @param[in] inst      OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] cmd_value CMD 寄存器值（32 位）
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_CMD[31:0]
 * @note 通常在 ospi_config_memory_mapped() 之后调用，覆盖默认命令编码
 *
 * @code
 * // DTR 双字节命令编码：读 0x20→0x2020，写 0xA0→0xA0A0
 * ospi_config_cmd_value(OSPI_1, 0xA0A02020U);
 * @endcode
 */
bool ospi_config_cmd_value(ospi_instance_t inst, uint32_t cmd_value)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    ospi_write_reg(base, OSPI_CMD_OFFSET, cmd_value);
    return true;
}

/**
 * @brief 配置 APM Dummy Clock
 *
 * @param[in] inst        OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] clock_count APM Dummy 时钟数（0-7）
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_CTL.APMD_CLK[25:23]
 *
 * @code
 * ospi_config_apm_dummy_clock(OSPI_1, 4);  // 4 个 APM Dummy 时钟
 * @endcode
 */
bool ospi_config_apm_dummy_clock(ospi_instance_t inst, uint8_t clock_count)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    ospi_modify_reg(base, OSPI_CTL_OFFSET, 0x7UL << 23,
                    ((uint32_t)clock_count & 0x7U) << 23);

    return true;
}

/**
 * @brief 配置主从模式
 *
 * @param[in] inst   OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] enable true: 主机模式, false: 从机模式
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_CTL.MST_MODE[0]
 *
 * @code
 * ospi_config_master_mode(OSPI_1, true);  // 配置为主机模式
 * @endcode
 */
bool ospi_config_master_mode(ospi_instance_t inst, bool enable)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    ospi_modify_reg(base, OSPI_CTL_OFFSET, 1UL << 0,
                    (enable ? 1UL : 0UL) << 0);

    return true;
}

/**
 * @brief 配置 CS 超时使能
 *
 * @param[in] inst   OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] enable true: 使能 CS 超时, false: 禁止 CS 超时
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_MEMO_ACC1.CS_TOUT_EN[1]
 *
 * @code
 * ospi_config_cs_timeout_enable(OSPI_1, true);  // 使能 CS 超时检测
 * @endcode
 */
bool ospi_config_cs_timeout_enable(ospi_instance_t inst, bool enable)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    ospi_modify_reg(base, OSPI_MEMO_ACC1_OFFSET, 1UL << 1,
                    (enable ? 1UL : 0UL) << 1);

    return true;
}

/**
 * @brief 配置交替字节模式
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] mode 交替字节传输模式
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_MEMO_ACC1.ALTER_BYTE_MODE[24:23]
 *
 * @code
 * ospi_config_alternate_bytes_mode(OSPI_1, OSPI_DATA_1LINE);  // 单线交替字节
 * @endcode
 */
bool ospi_config_alternate_bytes_mode(ospi_instance_t inst, ospi_data_mode_t mode)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    ospi_modify_reg(base, OSPI_MEMO_ACC1_OFFSET, 0x3UL << 23,
                    ((uint32_t)mode & 0x3U) << 23);

    return true;
}

/**
 * @brief 配置写 Dummy 大小
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] size 写 Dummy 大小（0-31 个周期）
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_MEMO_ACC2.WRITE_DUMMY_SIZE[31:27]（CMSIS 名: WLC）
 * @note 用于内存映射模式下写操作的 Dummy 周期
 *
 * @code
 * ospi_config_write_dummy_size(OSPI_1, 4);  // 4 个写 Dummy 周期
 * @endcode
 */
bool ospi_config_write_dummy_size(ospi_instance_t inst, uint8_t size)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    ospi_modify_reg(base, OSPI_MEMO_ACC2_OFFSET, 0x1FUL << 27,
                    ((uint32_t)size & 0x1FU) << 27);

    return true;
}

/**
 * @brief 配置回卷大小
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] size 回卷大小配置
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_MEMO_ACC2.WRPS[14:12]
 * @note 回卷大小写入 ACC2.WRPS，同时保存在驱动内部状态中
 *
 * @code
 * ospi_config_wrap_size(OSPI_1, OSPI_WRAP_64B);  // 64 字节回卷
 * @endcode
 */
bool ospi_config_wrap_size(ospi_instance_t inst, ospi_wrap_size_t size)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    ospi_modify_reg(base, OSPI_MEMO_ACC2_OFFSET, 0x7UL << 12,
                    ((uint32_t)size & 0x7U) << 12);

    s_ospi_config_state[inst].wrap_size = size;

    return true;
}

/**
 * @brief 配置 HyperBus 延迟控制参数
 *
 * @param[in] inst  OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] hyper HyperBus 配置（突发类型、延迟控制 LC0/LC1）
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_MEMO_ACC1.HYPER_BT[27], OSPI_MEMO_ACC1.ADDR_WIDTH[18:17],
 *       OSPI_MEMO_ACC2.HXSPI_LC0[3:0], OSPI_MEMO_ACC2.HXSPI_LC1[7:4]
 *
 * @code
 * ospi_hyperbus_config_t hyper = {
 *     .burst_type = OSPI_HYPER_LINEAR,
 *     .lc0 = 6,
 *     .lc1 = 6,
 * };
 * ospi_config_hyperbus(OSPI_1, &hyper);
 * @endcode
 */
bool ospi_config_hyperbus(ospi_instance_t inst, const ospi_hyperbus_config_t *hyper)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }
    if (hyper == NULL) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    // 配置 HyperBus 突发类型：MEMO_ACC1.HYPER_BT[27]
    ospi_modify_reg(base, OSPI_MEMO_ACC1_OFFSET, 1UL << 27,
                    ((uint32_t)hyper->burst_type & 0x1U) << 27);

    // HyperBus 模式要求地址位数必须为 32bit：MEMO_ACC1.ADDR_WIDTH[18:17]=3
    ospi_modify_reg(base, OSPI_MEMO_ACC1_OFFSET, 0x3UL << 17, 3UL << 17);

    // 配置延迟控制：MEMO_ACC2.HXSPI_LC0[3:0], HXSPI_LC1[7:4]
    uint32_t acc2_mask = 0xFUL | (0xFUL << 4);
    uint32_t acc2_value = ((uint32_t)hyper->lc0 & 0xFU) |
                          (((uint32_t)hyper->lc1 & 0xFU) << 4);
    ospi_modify_reg(base, OSPI_MEMO_ACC2_OFFSET, acc2_mask, acc2_value);

    return true;
}

//===========================================
// 第3层：高级功能 API
//===========================================

/**
 * @brief 使能内存映射模式
 *
 * 设置 MEMO_ACC1.ACC_EN 位，使能 OSPI 外设的内存映射访问功能。
 * 使能后可通过外部存储器映射地址空间直接访问外部 Flash。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return true: 使能成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_MEMO_ACC1.ACC_EN[0]
 * @note 使用前需先调用 ospi_config_memory_mapped() 配置内存映射参数
 *
 * @code
 * ospi_config_memory_mapped(OSPI_1, &mem_cfg);
 * ospi_enable_memory_mapped(OSPI_1);
 * @endcode
 */
bool ospi_enable_memory_mapped(ospi_instance_t inst)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    ospi_modify_reg(base, OSPI_MEMO_ACC1_OFFSET, 1UL << 0, 1UL << 0);

    return true;
}

/**
 * @brief 禁用内存映射模式
 *
 * 清除 MEMO_ACC1.ACC_EN 位，禁止 OSPI 外设的内存映射访问功能。
 * 禁用后外部存储器映射地址空间不再可访问。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return void
 *
 * @note 覆盖寄存器：OSPI_MEMO_ACC1.ACC_EN[0]
 * @note 禁用前需确保没有正在执行的内存映射访问
 */
void ospi_disable_memory_mapped(ospi_instance_t inst)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return;
    }

    ospi_modify_reg(base, OSPI_MEMO_ACC1_OFFSET, 1UL << 0, 0UL << 0);
}

/**
 * @brief 发送命令（间接模式）
 *
 * 配置 OSPI 命令参数。函数根据 ospi_command_config_t
 * 配置指令、地址、Dummy 周期、替代字节等参数，将地址写入 TX FIFO。
 * 不启动发送，需调用 ospi_start_tx() 启动传输。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] cmd  命令配置结构体指针
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_CMD, OSPI_MEMO_ACC1, OSPI_ALTER_BYTE, OSPI_TX_DAT
 * @note 函数仅配置命令和地址，不使能 TX_EN，需调用 ospi_start_tx() 启动传输
 * @note 地址按 MSB-first 顺序写入 TX FIFO
 *
 * @code
 * ospi_command_config_t cmd = {
 *     .instruction = 0x03,
 *     .address = 0x000000,
 *     .instr_mode = OSPI_INSTR_1LINE,
 *     .addr_mode = OSPI_ADDR_1LINE,
 *     .data_mode = OSPI_DATA_1LINE,
 *     .addr_width = OSPI_ADDR_24BIT,
 *     .dummy_cycles = 0,
 * };
 * ospi_send_command(OSPI_1, &cmd);
 * while (ospi_is_busy(OSPI_1)) { }
 * @endcode
 */
bool ospi_send_command(ospi_instance_t inst, const ospi_command_config_t *cmd)
{
    OSPI_TypeDef *base;
    uint32_t acc1;
    uint8_t addr_bytes;
    uint8_t i;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }
    if (cmd == NULL) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    // 配置命令寄存器：指令同时写入 RD_CMD[15:0] 和 WR_CMD[31:16]
    ospi_write_reg(base, OSPI_CMD_OFFSET,
                   ((uint32_t)cmd->instruction << 16U) | (uint32_t)cmd->instruction);

    // 配置内存访问寄存器1：保留 ACC_EN[0]、CS_TOUT_EN[1]、HYPER_BT[27]、ONCE_INSTR_CLR[29]
    acc1 = ospi_read_reg(base, OSPI_MEMO_ACC1_OFFSET);
    acc1 &= (1UL << 0) | (1UL << 1) | (1UL << 27) | (1UL << 29);
    acc1 |= ((uint32_t)cmd->instr_mode & 0x3U) << 19;   // INSTR_MODE
    acc1 |= ((uint32_t)cmd->addr_mode & 0x3U) << 21;    // ADDR_MODE
    acc1 |= ((uint32_t)cmd->data_mode & 0x3U) << 25;    // DATA_MODE
    acc1 |= ((uint32_t)cmd->addr_width & 0x3U) << 17;   // ADDR_WIDTH
    acc1 |= ((uint32_t)cmd->dummy_cycles & 0x1FU) << 12; // DUMMY_CYCLE_SIZE
    if (cmd->alternate_bytes_enable) {
        acc1 |= (1UL << 5);                               // WR_AB_EN
        acc1 |= (1UL << 6);                               // RD_AB_EN
        acc1 |= ((uint32_t)cmd->alternate_bytes_mode & 0x3U) << 23; // ALTER_BYTE_MODE
        acc1 |= ((uint32_t)cmd->alt_size & 0x3U) << 7;   // ALTER_BYTE_SIZE
    }
    ospi_write_reg(base, OSPI_MEMO_ACC1_OFFSET, acc1);

    // 配置替代字节
    if (cmd->alternate_bytes_enable) {
        ospi_write_reg(base, OSPI_ALTER_BYTE_OFFSET, cmd->alternate_bytes);
    }

    // 按地址宽度写入地址到 TX FIFO（MSB-first）
    addr_bytes = 0U;
    switch (cmd->addr_width) {
        case OSPI_ADDR_8BIT:  addr_bytes = 1U; break;
        case OSPI_ADDR_16BIT: addr_bytes = 2U; break;
        case OSPI_ADDR_24BIT: addr_bytes = 3U; break;
        case OSPI_ADDR_32BIT: addr_bytes = 4U; break;
        default: break;
    }
    for (i = 0U; i < addr_bytes; i++) {
        ospi_write_reg(base, OSPI_TX_DAT_OFFSET,
                       (cmd->address >> ((uint32_t)(addr_bytes - 1U - i) * 8U)) & 0xFFU);
    }

    return true;
}

/**
 * @brief 设置批量传输字节数
 *
 * 配置 OSPI_BATCH 寄存器，设置批量传输的数据单元数量。
 * 批量传输完成后触发 BATCH_DONE 中断标志。
 *
 * @param[in] inst  OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] count 批量传输数据单元数量（0-4294967295）
 * @return void
 *
 * @note 覆盖寄存器：OSPI_BATCH.BATCH_NUMBER[31:0]
 * @note 批量计数在传输启动后生效
 *
 * @code
 * ospi_set_batch(OSPI_1, 256);  // 批量传输 256 个数据单元
 * ospi_start_tx(OSPI_1);
 * @endcode
 */
void ospi_set_batch(ospi_instance_t inst, uint32_t count)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return;
    }

    ospi_write_reg(base, OSPI_BATCH_OFFSET, count);
}

/**
 * @brief 读取 FIFO（字节）
 *
 * 从 OSPI RX FIFO 读取一个字节数据。读取前应检查 FIFO 非空，
 * 避免读取无效数据。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return 读取的数据字节，实例无效时返回 0
 *
 * @note 覆盖寄存器：OSPI_RX_DAT[7:0]
 * @note FIFO 深度为 16 个数据单元
 * @note 实例无效时返回 0，调用者应先检查返回值
 *
 * @code
 * if (ospi_get_rx_fifo_level(OSPI_1) > 0U) {
 *     uint8_t data = ospi_read_fifo(OSPI_1);
 * }
 * @endcode
 */
uint8_t ospi_read_fifo(ospi_instance_t inst)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return 0U;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return 0U;
    }

    return (uint8_t)(ospi_read_reg(base, OSPI_RX_DAT_OFFSET) & 0xFFU);
}

/**
 * @brief 写入 FIFO（字节）
 *
 * 向 OSPI TX FIFO 写入一个字节数据。写入前应检查 FIFO 未满，
 * 避免数据丢失。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] data 要写入的数据字节
 * @return void
 *
 * @note 覆盖寄存器：OSPI_TX_DAT[7:0]
 * @note FIFO 深度为 16 个数据单元
 *
 * @code
 * if (!(ospi_get_status(OSPI_1) & (1UL << 3))) {  // TX_FIFO_FULL
 *     ospi_write_fifo(OSPI_1, 0xA5);
 * }
 * @endcode
 */
void ospi_write_fifo(ospi_instance_t inst, uint8_t data)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return;
    }

    ospi_write_reg(base, OSPI_TX_DAT_OFFSET, (uint32_t)data);
}

/**
 * @brief 读取 FIFO（半字）
 *
 * 从 OSPI RX FIFO 读取一个半字（16 位）数据。
 * 读取前应检查 FIFO 中至少有 2 个字节数据。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return 读取的半字数据，实例无效时返回 0
 *
 * @note 覆盖寄存器：OSPI_RX_DAT[15:0]
 * @note 需先配置 FIFO 模式为 OSPI_FIFO_HALF（ospi_config_fifo_mode）
 *
 * @code
 * ospi_config_fifo_mode(OSPI_1, OSPI_FIFO_HALF, OSPI_FIFO_HALF);
 * uint16_t data = ospi_read_fifo_half(OSPI_1);
 * @endcode
 */
uint16_t ospi_read_fifo_half(ospi_instance_t inst)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return 0U;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return 0U;
    }

    return (uint16_t)(ospi_read_reg(base, OSPI_RX_DAT_OFFSET) & 0xFFFFU);
}

/**
 * @brief 写入 FIFO（半字）
 *
 * 向 OSPI TX FIFO 写入一个半字（16 位）数据。
 * 写入前应检查 FIFO 未满。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] data 要写入的半字数据
 * @return void
 *
 * @note 覆盖寄存器：OSPI_TX_DAT[15:0]
 * @note 需先配置 FIFO 模式为 OSPI_FIFO_HALF（ospi_config_fifo_mode）
 *
 * @code
 * ospi_config_fifo_mode(OSPI_1, OSPI_FIFO_HALF, OSPI_FIFO_HALF);
 * ospi_write_fifo_half(OSPI_1, 0xA5A5);
 * @endcode
 */
void ospi_write_fifo_half(ospi_instance_t inst, uint16_t data)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return;
    }

    ospi_write_reg(base, OSPI_TX_DAT_OFFSET, (uint32_t)data);
}

/**
 * @brief 写入 FIFO（半字，带超时）
 *
 * 向 OSPI TX FIFO 写入一个半字（16 位）数据，写入前等待 FIFO 非满。
 * 与 ospi_write_fifo_half() 不同，本函数包含超时等待机制。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] data 要写入的半字数据
 * @return true: 写入成功, false: 超时或实例无效
 *
 * @note 覆盖寄存器：OSPI_TX_DAT[15:0], OSPI_STATUS.TX_FIFO_FULL[3]
 * @note 超时时间使用 OSPI_TIMEOUT_MS 默认值
 *
 * @code
 * if (!ospi_write_fifo_half_safe(OSPI_1, 0xA5A5U)) {
 *     // 超时处理
 * }
 * @endcode
 */
bool ospi_write_fifo_half_safe(ospi_instance_t inst, uint16_t data)
{
    if (!ospi_wait_status(inst, OSPI_IRQ_TX_FIFO_FULL, false, OSPI_TIMEOUT_MS)) {
        return false;
    }

    ospi_write_fifo_half(inst, data);
    return true;
}

/**
 * @brief 读取 FIFO（半字，带超时）
 *
 * 从 OSPI RX FIFO 读取一个半字（16 位）数据，读取前等待 FIFO 非空。
 * 与 ospi_read_fifo_half() 不同，本函数包含超时等待机制。
 *
 * @param[in]  inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[out] data 读取的半字数据指针
 * @return true: 读取成功, false: 超时、参数无效或实例无效
 *
 * @note 覆盖寄存器：OSPI_RX_DAT[15:0], OSPI_STATUS.RX_FIFO_EMPTY[4]
 * @note 超时时间使用 OSPI_TIMEOUT_MS 默认值
 *
 * @code
 * uint16_t data;
 * if (ospi_read_fifo_half_safe(OSPI_1, &data)) {
 *     // 处理 data
 * }
 * @endcode
 */
bool ospi_read_fifo_half_safe(ospi_instance_t inst, uint16_t *data)
{
    if (data == NULL) {
        return false;
    }

    if (!ospi_wait_status(inst, OSPI_IRQ_RX_FIFO_EMPTY, false, OSPI_TIMEOUT_MS)) {
        return false;
    }

    *data = ospi_read_fifo_half(inst);
    return true;
}

/**
 * @brief 读取 FIFO（字）
 *
 * 从 OSPI RX FIFO 读取一个字（32 位）数据。
 * 读取前应检查 FIFO 中至少有 4 个字节数据。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return 读取的字数据，实例无效时返回 0
 *
 * @note 覆盖寄存器：OSPI_RX_DAT[31:0]
 * @note 需先配置 FIFO 模式为 OSPI_FIFO_WORD（ospi_config_fifo_mode）
 *
 * @code
 * ospi_config_fifo_mode(OSPI_1, OSPI_FIFO_WORD, OSPI_FIFO_WORD);
 * uint32_t data = ospi_read_fifo_word(OSPI_1);
 * @endcode
 */
uint32_t ospi_read_fifo_word(ospi_instance_t inst)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return 0U;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return 0U;
    }

    return ospi_read_reg(base, OSPI_RX_DAT_OFFSET);
}

/**
 * @brief 写入 FIFO（字）
 *
 * 向 OSPI TX FIFO 写入一个字（32 位）数据。
 * 写入前应检查 FIFO 未满。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] data 要写入的字数据
 * @return void
 *
 * @note 覆盖寄存器：OSPI_TX_DAT[31:0]
 * @note 需先配置 FIFO 模式为 OSPI_FIFO_WORD（ospi_config_fifo_mode）
 *
 * @code
 * ospi_config_fifo_mode(OSPI_1, OSPI_FIFO_WORD, OSPI_FIFO_WORD);
 * ospi_write_fifo_word(OSPI_1, 0xA5A5A5A5U);
 * @endcode
 */
void ospi_write_fifo_word(ospi_instance_t inst, uint32_t data)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return;
    }

    ospi_write_reg(base, OSPI_TX_DAT_OFFSET, data);
}

/**
 * @brief 配置采样移位
 *
 * 配置 RX_CTL 寄存器中的采样移位参数，包括采样点偏移值、
 * 多比特 SD 线使能及掩码、多比特 SD 保持调整掩码。
 *
 * @param[in] inst  OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] shift 采样移位配置结构体指针
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_RX_CTL.SSHIFT[31:28], OSPI_RX_CTL.MSDA_EN[10],
 *       OSPI_RX_CTL.MSDA_X0-X7[11:18], OSPI_RX_CTL.MSDHA_X0-X7[19:26]
 * @note shift_value 有效范围 0-13，表示采样点延迟的时钟周期数
 * @note msda_mask 和 msdha_mask 为位掩码，每个 bit 对应一条 IO 线
 *
 * @code
 * ospi_sample_shift_config_t shift = {
 *     .enable = true,
 *     .shift_value = 2,
 *     .msda_mask = 0x0FU,
 *     .msdha_mask = 0x0FU,
 * };
 * ospi_config_sample_shift(OSPI_1, &shift);
 * @endcode
 */
bool ospi_config_sample_shift(ospi_instance_t inst, const ospi_sample_shift_config_t *shift)
{
    OSPI_TypeDef *base;
    uint32_t rx_ctl;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }
    if (shift == NULL) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    // 读取当前 RX_CTL 值
    rx_ctl = ospi_read_reg(base, OSPI_RX_CTL_OFFSET);

    // 清除 SSHIFT[31:28]、MSDA_EN[10]、MSDA_X0-X7[11:18]、MSDHA_X0-X7[19:26]
    rx_ctl &= ~((0xFUL << 28) | (1UL << 10) | (0xFFUL << 11) | (0xFFUL << 19));

    // 设置采样移位值
    if (shift->enable) {
        rx_ctl |= ((uint32_t)shift->shift_value & 0xFU) << 28;
    }

    // 设置 MSDA 掩码和使能
    rx_ctl |= ((uint32_t)shift->msda_mask & 0xFFU) << 11;
    if (shift->msda_mask != 0U) {
        rx_ctl |= (1UL << 10);  // MSDA_EN
    }

    // 设置 MSDHA 掩码
    rx_ctl |= ((uint32_t)shift->msdha_mask & 0xFFU) << 19;

    ospi_write_reg(base, OSPI_RX_CTL_OFFSET, rx_ctl);

    return true;
}

/**
 * @brief 配置 DQS（数据 strobe）
 *
 * 配置 DQS 信号的发送使能和接收采样使能。DQS 用于源同步传输，
 * 在高速模式下提供数据采样时钟。
 *
 * @param[in] inst       OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] tx_enable  使能 DQS 输出（CTL.DQSOE）
 * @param[in] rx_enable  使能 DQS 采样（RX_CTL.DQS_SAMP_EN + MEMO_ACC2.DQS_EN）
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_CTL.DQSOE[9], OSPI_RX_CTL.DQS_SAMP_EN[8],
 *       OSPI_MEMO_ACC2.DQS_EN[26]
 * @note DQS 使能后硬件自动管理数据采样时序
 *
 * @code
 * ospi_config_dqs(OSPI_1, false, true);  // 仅使能接收 DQS
 * @endcode
 */
bool ospi_config_dqs(ospi_instance_t inst, bool tx_enable, bool rx_enable)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    // 配置 DQS 输出使能：CTL.DQSOE[9]
    ospi_modify_reg(base, OSPI_CTL_OFFSET, 1UL << 9,
                    (tx_enable ? 1UL : 0UL) << 9);

    // 配置 DQS 采样使能：RX_CTL.DQS_SAMP_EN[8]
    ospi_modify_reg(base, OSPI_RX_CTL_OFFSET, 1UL << 8,
                    (rx_enable ? 1UL : 0UL) << 8);

    // 配置内存访问 DQS 使能：MEMO_ACC2.DQS_EN[26]
    ospi_modify_reg(base, OSPI_MEMO_ACC2_OFFSET, 1UL << 26,
                    (rx_enable ? 1UL : 0UL) << 26);

    return true;
}

/**
 * @brief 配置空周期关闭
 *
 * 配置 Dummy 周期关闭功能，可减少读/写操作中的空闲周期以提升传输效率。
 * 读 Dummy 关闭配置 MEMO_ACC2 寄存器，写 Dummy 关闭配置 MEMO_ACC3 寄存器。
 *
 * @param[in] inst      OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] enable    读 Dummy 关闭使能（MEMO_ACC2.DUMMYOFF_EN）
 * @param[in] size      读 Dummy 关闭延迟值（MEMO_ACC2.RDUMMYOFF_LC，0-63）
 * @param[in] wr_enable 写 Dummy 关闭使能（非零时配置 MEMO_ACC3，为零时不修改 ACC3）
 * @param[in] wr_size   写 Dummy 关闭延迟值（MEMO_ACC3.WDUMMYOFF_LC，0-63）
 * @return true: 配置成功, false: 参数无效
 *
 * @note 覆盖寄存器：OSPI_MEMO_ACC2.DUMMYOFF_EN[19],
 *       OSPI_MEMO_ACC2.RDUMMYOFF_LC[25:20],
 *       OSPI_MEMO_ACC3.WDUMMYOFF_LC[7:2]
 * @note Dummy 关闭功能通过减少空闲周期来提高带宽利用率
 *
 * @code
 * ospi_config_dummy_off(OSPI_1, true, 6, true, 6);
 * @endcode
 */
bool ospi_config_dummy_off(ospi_instance_t inst, bool enable, uint8_t size,
                           bool wr_enable, uint8_t wr_size)
{
    OSPI_TypeDef *base;
    uint32_t acc2_mask;
    uint32_t acc2_value;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    // 配置读 Dummy 关闭：MEMO_ACC2.DUMMYOFF_EN[19], RDUMMYOFF_LC[25:20]
    acc2_mask = (1UL << 19) | (0x3FUL << 20);
    acc2_value = ((enable ? 1UL : 0UL) << 19) |
                 (((uint32_t)size & 0x3FU) << 20);
    ospi_modify_reg(base, OSPI_MEMO_ACC2_OFFSET, acc2_mask, acc2_value);

    // 配置写 Dummy 关闭：MEMO_ACC3.WDUMMYOFF_LC[7:2]
    if (wr_enable) {
        ospi_modify_reg(base, OSPI_MEMO_ACC3_OFFSET, 0x3FUL << 2,
                        ((uint32_t)wr_size & 0x3FU) << 2);
    }

    return true;
}

/**
 * @brief 清除一次指令标志
 *
 * 向 MEMO_ACC1.ONCE_INSTR_CLR 位写 1，清除发送一次指令完成标志。
 * 在连续模式下，使能 SEND_INSTR_ONCE_EN 后，首次传输会发送指令，
 * 后续传输不再发送。调用此函数可重新触发指令发送。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return void
 *
 * @note 覆盖寄存器：OSPI_MEMO_ACC1.ONCE_INSTR_CLR[29]（写 1 清除）
 * @note 仅在连续模式且 SEND_INSTR_ONCE_EN 使能时有意义
 *
 * @code
 * // 连续模式下重新发送指令
 * ospi_clear_once_instr_flag(OSPI_1);
 * ospi_start_tx(OSPI_1);
 * @endcode
 */
void ospi_clear_once_instr_flag(ospi_instance_t inst)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return;
    }

    // ONCE_INSTR_CLR[29] 写 1 清除
    ospi_modify_reg(base, OSPI_MEMO_ACC1_OFFSET, 1UL << 29, 1UL << 29);
}

//===========================================
// 第4层：控制与查询 API
//===========================================

/**
 * @brief 启动发送
 *
 * 使能 OSPI 发送器，设置 TX_CTL.TX_EN 位。使能后硬件开始将
 * TX FIFO 中的数据通过 SPI 总线发送出去。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return void
 *
 * @note 覆盖寄存器：OSPI_TX_CTL.TX_EN[0]
 * @note 发送前需先将数据写入 TX FIFO（ospi_write_fifo）
 * @note 仅启动发送，如需同时接收请使用 ospi_start()
 *
 * @code
 * ospi_write_fifo(OSPI_1, 0xA5);
 * ospi_start_tx(OSPI_1);
 * while (ospi_is_busy(OSPI_1)) { }
 * @endcode
 */
void ospi_start_tx(ospi_instance_t inst)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return;
    }

    ospi_modify_reg(base, OSPI_TX_CTL_OFFSET, 1UL << 0, 1UL << 0);
}

/**
 * @brief 启动接收
 *
 * 使能 OSPI 接收器，设置 RX_CTL.RX_EN 位。使能后硬件开始将
 * SPI 总线上的数据接收到 RX FIFO 中。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return void
 *
 * @note 覆盖寄存器：OSPI_RX_CTL.RX_EN[0]
 * @note 接收数据通过 ospi_read_fifo() 从 RX FIFO 读取
 * @note 仅启动接收，如需同时发送请使用 ospi_start()
 *
 * @code
 * ospi_start_rx(OSPI_1);
 * while (ospi_get_rx_fifo_level(OSPI_1) == 0U) { }
 * uint8_t data = ospi_read_fifo(OSPI_1);
 * @endcode
 */
void ospi_start_rx(ospi_instance_t inst)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return;
    }

    ospi_modify_reg(base, OSPI_RX_CTL_OFFSET, 1UL << 0, 1UL << 0);
}

/**
 * @brief 启动传输（同时使能 TX 和 RX）
 *
 * 同时使能 OSPI 发送器和接收器，适用于全双工 SPI 传输。
 * 设置 TX_CTL.TX_EN 和 RX_CTL.RX_EN 位。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return void
 *
 * @note 覆盖寄存器：OSPI_TX_CTL.TX_EN[0], OSPI_RX_CTL.RX_EN[0]
 * @note 全双工模式下，发送和接收同时进行
 *
 * @code
 * ospi_write_fifo(OSPI_1, 0xA5);
 * ospi_start(OSPI_1);
 * while (ospi_is_busy(OSPI_1)) { }
 * uint8_t rx = ospi_read_fifo(OSPI_1);
 * @endcode
 */
void ospi_start(ospi_instance_t inst)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return;
    }

    ospi_modify_reg(base, OSPI_TX_CTL_OFFSET, 1UL << 0, 1UL << 0);
    ospi_modify_reg(base, OSPI_RX_CTL_OFFSET, 1UL << 0, 1UL << 0);
}

/**
 * @brief 停止发送
 *
 * 禁用 OSPI 发送器，清除 TX_CTL.TX_EN 位。禁用后硬件停止发送
 * TX FIFO 中剩余的数据。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return void
 *
 * @note 覆盖寄存器：OSPI_TX_CTL.TX_EN[0]
 * @note 停止发送后，TX FIFO 中未发送的数据将被丢弃
 *
 * @code
 * ospi_start_tx(OSPI_1);
 * delay_ms(10);
 * ospi_stop_tx(OSPI_1);
 * @endcode
 */
void ospi_stop_tx(ospi_instance_t inst)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return;
    }

    ospi_modify_reg(base, OSPI_TX_CTL_OFFSET, 1UL << 0, 0UL << 0);
}

/**
 * @brief 停止接收
 *
 * 禁用 OSPI 接收器，清除 RX_CTL.RX_EN 位。禁用后硬件停止接收
 * SPI 总线上的数据。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return void
 *
 * @note 覆盖寄存器：OSPI_RX_CTL.RX_EN[0]
 * @note 停止接收后，RX FIFO 中已接收的数据仍可读取
 *
 * @code
 * ospi_start_rx(OSPI_1);
 * delay_ms(10);
 * ospi_stop_rx(OSPI_1);
 * @endcode
 */
void ospi_stop_rx(ospi_instance_t inst)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return;
    }

    ospi_modify_reg(base, OSPI_RX_CTL_OFFSET, 1UL << 0, 0UL << 0);
}

/**
 * @brief 停止传输（同时禁用 TX 和 RX）
 *
 * 同时禁用 OSPI 发送器和接收器，清除 TX_CTL.TX_EN 和 RX_CTL.RX_EN 位。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return void
 *
 * @note 覆盖寄存器：OSPI_TX_CTL.TX_EN[0], OSPI_RX_CTL.RX_EN[0]
 *
 * @code
 * ospi_start(OSPI_1);
 * delay_ms(10);
 * ospi_stop(OSPI_1);
 * @endcode
 */
void ospi_stop(ospi_instance_t inst)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return;
    }

    ospi_modify_reg(base, OSPI_TX_CTL_OFFSET, 1UL << 0, 0UL << 0);
    ospi_modify_reg(base, OSPI_RX_CTL_OFFSET, 1UL << 0, 0UL << 0);
}

/**
 * @brief 复位 FIFO
 *
 * 通过设置 TX_CTL.TX_FIFO_RESET 或 RX_CTL.RX_FIFO_RESET 位来
 * 复位对应的 FIFO 缓冲区。复位后 FIFO 中的数据将被清空。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] tx   true: 复位发送 FIFO
 * @param[in] rx   true: 复位接收 FIFO
 * @return void
 *
 * @note 覆盖寄存器：OSPI_TX_CTL.TX_FIFO_RESET[1], OSPI_RX_CTL.RX_FIFO_RESET[1]
 * @note 复位需写1后写0撤销，代码已自动处理
 * @note 复位期间不要访问对应的 FIFO
 *
 * @code
 * ospi_reset_fifo(OSPI_1, true, true);  // 同时复位 TX 和 RX FIFO
 * @endcode
 */
void ospi_reset_fifo(ospi_instance_t inst, bool tx, bool rx)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return;
    }

    if (tx) {
        ospi_modify_reg(base, OSPI_TX_CTL_OFFSET, 1UL << 1, 1UL << 1);
        ospi_modify_reg(base, OSPI_TX_CTL_OFFSET, 1UL << 1, 0UL << 1);
    }
    if (rx) {
        ospi_modify_reg(base, OSPI_RX_CTL_OFFSET, 1UL << 1, 1UL << 1);
        ospi_modify_reg(base, OSPI_RX_CTL_OFFSET, 1UL << 1, 0UL << 1);
    }
}

/**
 * @brief 获取状态寄存器值
 *
 * 读取 OSPI STATUS 寄存器的完整值，包含所有状态标志位。
 * 调用者可使用 ospi_irq_type_t 中定义的位掩码进行判断。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return STATUS 寄存器值，实例无效时返回 0
 *
 * @note 覆盖寄存器：OSPI_STATUS[31:0]
 * @note 状态标志位定义见 ospi_irq_type_t 枚举
 *
 * @code
 * uint32_t status = ospi_get_status(OSPI_1);
 * if (status & OSPI_IRQ_TX_FIFO_EMPTY) {
 *     // TX FIFO 为空
 * }
 * @endcode
 */
uint32_t ospi_get_status(ospi_instance_t inst)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return 0U;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return 0U;
    }

    return ospi_read_reg(base, OSPI_STATUS_OFFSET);
}

/**
 * @brief 获取 TX FIFO 水平
 *
 * 根据 STATUS 寄存器中的 FIFO 状态标志，估算当前 TX FIFO 中的
 * 数据单元数量。FIFO 深度为 16 个数据单元。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return TX FIFO 中的数据单元数量（0/1/8/16），实例无效时返回 0
 *
 * @note 覆盖寄存器：OSPI_STATUS.TX_FIFO_EMPTY[2],
 *       OSPI_STATUS.TX_FIFO_FULL[3],
 *       OSPI_STATUS.TX_FIFO_HALF_EMPTY[6],
 *       OSPI_STATUS.TX_FIFO_HALF_FULL[7]
 * @note 返回值为估算值：0=空，1=有数据但<8，8=半满（>=8），16=满
 *
 * @code
 * if (ospi_get_tx_fifo_level(OSPI_1) < OSPI_FIFO_SIZE) {
 *     ospi_write_fifo(OSPI_1, data);
 * }
 * @endcode
 */
uint8_t ospi_get_tx_fifo_level(ospi_instance_t inst)
{
    OSPI_TypeDef *base;
    uint32_t status;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return 0U;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return 0U;
    }

    status = ospi_read_reg(base, OSPI_STATUS_OFFSET);

    if (status & (1UL << 2)) {
        return 0U;  // TX_FIFO_EMPTY
    }
    if (status & (1UL << 3)) {
        return OSPI_FIFO_SIZE;  // TX_FIFO_FULL
    }
    if (status & (1UL << 7)) {
        return OSPI_FIFO_SIZE / 2U;  // TX_FIFO_HALF_FULL：>=8
    }
    if (status & (1UL << 6)) {
        return 1U;  // TX_FIFO_HALF_EMPTY：<8 但非空
    }
    return 0U;
}

/**
 * @brief 获取 RX FIFO 水平
 *
 * 根据 STATUS 寄存器中的 FIFO 状态标志，估算当前 RX FIFO 中的
 * 数据单元数量。FIFO 深度为 16 个数据单元。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return RX FIFO 中的数据单元数量（0/1/8/16），实例无效时返回 0
 *
 * @note 覆盖寄存器：OSPI_STATUS.RX_FIFO_EMPTY[4],
 *       OSPI_STATUS.RX_FIFO_FULL[5],
 *       OSPI_STATUS.RX_FIFO_HALF_EMPTY[8],
 *       OSPI_STATUS.RX_FIFO_HALF_FULL[9]
 * @note 返回值为估算值：0=空，1=有数据但<8，8=半满（>=8），16=满
 *
 * @code
 * if (ospi_get_rx_fifo_level(OSPI_1) > 0U) {
 *     uint8_t data = ospi_read_fifo(OSPI_1);
 * }
 * @endcode
 */
uint8_t ospi_get_rx_fifo_level(ospi_instance_t inst)
{
    OSPI_TypeDef *base;
    uint32_t status;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return 0U;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return 0U;
    }

    status = ospi_read_reg(base, OSPI_STATUS_OFFSET);

    if (status & (1UL << 4)) {
        return 0U;  // RX_FIFO_EMPTY
    }
    if (status & (1UL << 5)) {
        return OSPI_FIFO_SIZE;  // RX_FIFO_FULL
    }
    if (status & (1UL << 9)) {
        return OSPI_FIFO_SIZE / 2U;  // RX_FIFO_HALF_FULL：>=8
    }
    if (status & (1UL << 8)) {
        return 1U;  // RX_FIFO_HALF_EMPTY：<8 但非空
    }
    return 0U;
}

/**
 * @brief 查询忙状态
 *
 * 读取 STATUS.TX_BUSY 标志，判断 OSPI 是否正在执行传输操作。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return true: 正在传输, false: 空闲或实例无效
 *
 * @note 覆盖寄存器：OSPI_STATUS.TX_BUSY[0]
 * @note 发送或接收启动后，TX_BUSY 会置位；传输完成后自动清零
 *
 * @code
 * ospi_start_tx(OSPI_1);
 * while (ospi_is_busy(OSPI_1)) { }
 * @endcode
 */
bool ospi_is_busy(ospi_instance_t inst)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    return (ospi_read_reg(base, OSPI_STATUS_OFFSET) & (1UL << 0)) != 0U;
}

/**
 * @brief 使能中断
 *
 * 使能指定的 OSPI 中断源。可同时使能多个中断源（使用位或组合）。
 * 函数同时更新内部中断状态跟踪变量。
 *
 * @param[in] inst     OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] irq_type 中断类型掩码（可使用 ospi_irq_type_t 位或组合）
 * @return void
 *
 * @note 覆盖寄存器：OSPI_IE[15:1]
 * @note 中断使能后，对应事件发生时硬件会置位 STATUS 中的标志位
 * @note 需在 NVIC 中同时使能对应的 OSPI 中断向量
 * @note APM_DUMY_DONE 无对应 IE 使能位，硬件直接置位 STATUS，无需通过此函数使能
 *
 * @code
 * ospi_enable_interrupt(OSPI_1, OSPI_IRQ_TX_FIFO_EMPTY | OSPI_IRQ_RX_FIFO_FULL);
 * @endcode
 */
void ospi_enable_interrupt(ospi_instance_t inst, uint32_t irq_type)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return;
    }

    ospi_modify_reg(base, OSPI_IE_OFFSET, irq_type, irq_type);
    s_ospi_irq_state[inst].irq_enable |= irq_type;
}

/**
 * @brief 禁用中断
 *
 * 禁用指定的 OSPI 中断源。可同时禁用多个中断源（使用位或组合）。
 * 函数同时更新内部中断状态跟踪变量。
 *
 * @param[in] inst     OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] irq_type 中断类型掩码（可使用 ospi_irq_type_t 位或组合）
 * @return void
 *
 * @note 覆盖寄存器：OSPI_IE[15:1]
 *
 * @code
 * ospi_disable_interrupt(OSPI_1, OSPI_IRQ_TX_FIFO_EMPTY);
 * @endcode
 */
void ospi_disable_interrupt(ospi_instance_t inst, uint32_t irq_type)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return;
    }

    ospi_modify_reg(base, OSPI_IE_OFFSET, irq_type, 0U);
    s_ospi_irq_state[inst].irq_enable &= ~irq_type;
}

/**
 * @brief 注册中断回调函数
 *
 * 为指定的中断类型注册回调函数。当对应中断发生时，
 * 中断处理函数会调用已注册的回调。
 *
 * @param[in] inst     OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] irq_type 中断类型（仅支持单个中断源）
 * @param[in] callback 回调函数指针，传 NULL 可取消注册
 * @return void
 *
 * @note 回调函数在中断上下文中执行，应尽量简短
 * @note 每个中断类型只能注册一个回调，重复注册会覆盖前一个
 *
 * @code
 * void my_callback(ospi_instance_t inst) {
 *     // 处理中断
 * }
 * ospi_register_callback(OSPI_1, OSPI_IRQ_TX_FIFO_EMPTY, my_callback);
 * @endcode
 */
void ospi_register_callback(ospi_instance_t inst, uint32_t irq_type, ospi_callback_t callback)
{
    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return;
    }

    switch (irq_type) {
        case OSPI_IRQ_BATCH_DONE:
            s_ospi_irq_state[inst].batch_done_cb = callback;
            break;
        case OSPI_IRQ_TX_FIFO_EMPTY:
            s_ospi_irq_state[inst].tx_fifo_empty_cb = callback;
            break;
        case OSPI_IRQ_TX_FIFO_FULL:
            s_ospi_irq_state[inst].tx_fifo_full_cb = callback;
            break;
        case OSPI_IRQ_RX_FIFO_EMPTY:
            s_ospi_irq_state[inst].rx_fifo_empty_cb = callback;
            break;
        case OSPI_IRQ_RX_FIFO_FULL:
            s_ospi_irq_state[inst].rx_fifo_full_cb = callback;
            break;
        case OSPI_IRQ_TX_FIFO_HALF_EMPTY:
            s_ospi_irq_state[inst].tx_fifo_half_empty_cb = callback;
            break;
        case OSPI_IRQ_TX_FIFO_HALF_FULL:
            s_ospi_irq_state[inst].tx_fifo_half_full_cb = callback;
            break;
        case OSPI_IRQ_RX_FIFO_HALF_EMPTY:
            s_ospi_irq_state[inst].rx_fifo_half_empty_cb = callback;
            break;
        case OSPI_IRQ_RX_FIFO_HALF_FULL:
            s_ospi_irq_state[inst].rx_fifo_half_full_cb = callback;
            break;
        case OSPI_IRQ_RX_FIFO_NOT_EMPTY:
            s_ospi_irq_state[inst].rx_fifo_not_empty_cb = callback;
            break;
        case OSPI_IRQ_TX_BATCH_DONE:
            s_ospi_irq_state[inst].tx_batch_done_cb = callback;
            break;
        case OSPI_IRQ_RX_BATCH_DONE:
            s_ospi_irq_state[inst].rx_batch_done_cb = callback;
            break;
        case OSPI_IRQ_APM_DUMY_DONE:
            s_ospi_irq_state[inst].apm_dumy_done_cb = callback;
            break;
        default:
            break;
    }
}

/**
 * @brief 获取内存映射基地址
 *
 * 返回 OSPI 外设对应的内存映射地址空间基地址。CPU 可通过此地址
 * 直接读取外部 Flash 数据（XIP 模式）。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return 内存映射基地址，实例无效时返回 0
 *
 * @note OSPI1 映射地址：0x80000000
 * @note OSPI2 映射地址：0x10000000
 * @note 使用前需先调用 ospi_enable_memory_mapped() 使能内存映射
 *
 * @code
 * ospi_enable_memory_mapped(OSPI_1);
 * uint32_t addr = ospi_get_memory_address(OSPI_1);
 * uint32_t *flash_data = (uint32_t *)addr;
 * @endcode
 */
uint32_t ospi_get_memory_address(ospi_instance_t inst)
{
    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return 0U;
    }

    if (inst == OSPI_1) {
        return 0x80000000U;
    }
    return 0x10000000U;
}

/**
 * @brief 配置管脚输出使能
 *
 * 配置 OSPI_OUT_EN 寄存器，控制各 IO 线的输出驱动使能。
 * 每个 bit 对应一条 IO 线的输出使能。
 *
 * @param[in] inst           OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] io_enable_bits IO 输出使能位掩码（bit0=MOSI, bit1=MISO,
 *                           bit2=WP, bit3=HOLD, bit4-7=IO4-IO7）
 * @return void
 *
 * @note 覆盖寄存器：OSPI_OUT_EN[7:0]
 * @note 1 线模式：0x03（MOSI + MISO）
 * @note 4 线模式：0x0F（IO0-IO3）
 * @note 8 线模式：0xFF（IO0-IO7）
 *
 * @code
 * ospi_config_out_en(OSPI_1, 0x0FU);  // 使能 IO0-IO3
 * @endcode
 */
void ospi_config_out_en(ospi_instance_t inst, uint8_t io_enable_bits)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return;
    }

    ospi_write_reg(base, OSPI_OUT_EN_OFFSET, (uint32_t)io_enable_bits);
}

/**
 * @brief 获取中断状态
 *
 * 读取 OSPI STATUS 寄存器，返回当前所有中断标志位的状态。
 * 与 ospi_get_status() 相同，但语义上用于中断处理场景。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return STATUS 寄存器值，实例无效时返回 0
 *
 * @note 覆盖寄存器：OSPI_STATUS[31:0]
 * @note 在中断服务函数中使用此函数获取中断源
 *
 * @code
 * void OSPI1_IRQHandler(void) {
 *     uint32_t status = ospi_get_irq_status(OSPI_1);
 *     if (status & OSPI_IRQ_TX_FIFO_EMPTY) {
 *         // 处理 TX FIFO 空中断
 *     }
 * }
 * @endcode
 */
uint32_t ospi_get_irq_status(ospi_instance_t inst)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return 0U;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return 0U;
    }

    return ospi_read_reg(base, OSPI_STATUS_OFFSET);
}

/**
 * @brief 清除中断状态
 *
 * 向 STATUS 寄存器写入指定的掩码来清除中断标志位。
 * 写 1 清除对应的标志位。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @param[in] mask 要清除的中断标志位掩码（使用 ospi_irq_type_t）
 * @return void
 *
 * @note 覆盖寄存器：OSPI_STATUS[15:1], OSPI_STATUS[18:16]
 * @note 写 1 清除：向对应 bit 写 1 可清除该标志
 * @note 在中断服务函数中处理完中断后必须清除对应标志
 *
 * @code
 * ospi_clear_irq_status(OSPI_1, OSPI_IRQ_BATCH_DONE);
 * @endcode
 */
void ospi_clear_irq_status(ospi_instance_t inst, uint32_t mask)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return;
    }

    // 写 1 清除对应标志位
    // 联动清除：写 BATCH_DONE(bit1) 可清除 TX/RX_BATCH_DONE(bit14/bit15)
    // 用户传 TX/RX_BATCH_DONE 时必须追加 BATCH_DONE 才能生效
    uint32_t clear_mask = mask;
    if (mask & (OSPI_IRQ_TX_BATCH_DONE | OSPI_IRQ_RX_BATCH_DONE)) {
        clear_mask |= OSPI_IRQ_BATCH_DONE;
    }

    ospi_write_reg(base, OSPI_STATUS_OFFSET, clear_mask);
}

/**
 * @brief 获取 RWDS 状态
 *
 * 读取 STATUS.RWDS 标志位，获取读写数据选通信号（DQS）的状态。
 * RWDS 用于 HyperBus 协议中的源同步数据采样。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return true: RWDS 为高, false: RWDS 为低或实例无效
 *
 * @note 覆盖寄存器：OSPI_STATUS.RWDS[17]
 * @note 仅在 HyperBus 模式下有效
 *
 * @code
 * if (ospi_get_rwds(OSPI_1)) {
 *     // DQS 为高，可采样数据
 * }
 * @endcode
 */
bool ospi_get_rwds(ospi_instance_t inst)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    return (ospi_read_reg(base, OSPI_STATUS_OFFSET) & (1UL << 17)) != 0U;
}

/**
 * @brief 查询指令是否已发送
 *
 * 读取 STATUS.INSTR_SEND_DONE 标志位，判断指令是否已通过
 * SPI 总线发送完成。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return true: 指令已发送完成, false: 未完成或实例无效
 *
 * @note 覆盖寄存器：OSPI_STATUS.INSTR_SEND_DONE[18]
 * @note 在间接模式下，指令发送完成后才开始数据传输
 *
 * @code
 * ospi_send_command(OSPI_1, &cmd);
 * while (!ospi_is_instr_sent(OSPI_1)) { }
 * @endcode
 */
bool ospi_is_instr_sent(ospi_instance_t inst)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    return (ospi_read_reg(base, OSPI_STATUS_OFFSET) & (1UL << 18)) != 0U;
}

/**
 * @brief 查询内存映射是否使能
 *
 * 读取 MEMO_ACC1.ACC_EN 位，判断内存映射模式是否已使能。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return true: 已使能, false: 未使能或实例无效
 *
 * @note 覆盖寄存器：OSPI_MEMO_ACC1.ACC_EN[0]
 *
 * @code
 * if (!ospi_is_memory_mapped_enabled(OSPI_1)) {
 *     ospi_enable_memory_mapped(OSPI_1);
 * }
 * @endcode
 */
bool ospi_is_memory_mapped_enabled(ospi_instance_t inst)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return false;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return false;
    }

    return (ospi_read_reg(base, OSPI_MEMO_ACC1_OFFSET) & (1UL << 0)) != 0U;
}

/**
 * @brief 断言片选（拉低）
 *
 * 直接写 OSPI_CS 寄存器，选择 CS0。用于 FIFO 事务前手动控制片选。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return void
 *
 * @note 覆盖寄存器：OSPI_CS.SPI_CS[2:0]
 * @note 与 ospi_config_cs() 不同，本函数直接写寄存器，不更新内部状态
 *
 * @code
 * ospi_assert_cs(OSPI_1);
 * // ... FIFO 事务 ...
 * ospi_release_cs(OSPI_1);
 * @endcode
 */
void ospi_assert_cs(ospi_instance_t inst)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return;
    }

    ospi_write_reg(base, OSPI_CS_OFFSET, (uint32_t)OSPI_CS_0);
}

/**
 * @brief 释放片选（拉高）
 *
 * 直接写 OSPI_CS 寄存器，清除所有 CS 选择。用于 FIFO 事务后释放片选。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 * @return void
 *
 * @note 覆盖寄存器：OSPI_CS.SPI_CS[2:0]
 * @note 与 ospi_assert_cs() 配对使用
 *
 * @code
 * ospi_assert_cs(OSPI_1);
 * // ... FIFO 事务 ...
 * ospi_release_cs(OSPI_1);
 * @endcode
 */
void ospi_release_cs(ospi_instance_t inst)
{
    OSPI_TypeDef *base;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return;
    }

    base = ospi_get_base(inst);
    if (base == NULL) {
        return;
    }

    ospi_write_reg(base, OSPI_CS_OFFSET, 0U);
}

//===========================================
// 中断处理函数
//===========================================

/**
 * @brief OSPI 中断通用处理函数
 *
 * 读取状态寄存器与中断使能寄存器，按优先级分发回调函数。
 * 优先级：BATCH_DONE > TX/RX_BATCH_DONE > FIFO 状态
 * APM_DUMY_DONE 无对应 IE 使能位，直接检查状态标志。
 * BATCH_DONE 和 APM_DUMY_DONE 在回调后写 1 清除。
 *
 * @param[in] inst OSPI 实例号（OSPI_1 或 OSPI_2）
 */
static void ospi_irq_handler(ospi_instance_t inst)
{
    OSPI_TypeDef *base;
    volatile ospi_irq_state_t *irq_state;
    uint32_t status;
    uint32_t ie;
    uint32_t pending;
    uint32_t clear_mask = 0U;

    if ((uint32_t)inst >= (uint32_t)OSPI_COUNT) {
        return;
    }

    base = s_ospi_regs[inst];
    irq_state = &s_ospi_irq_state[inst];

    // 读取状态寄存器和中断使能寄存器
    status = ospi_read_reg(base, OSPI_STATUS_OFFSET);
    ie = ospi_read_reg(base, OSPI_IE_OFFSET);

    // 计算待处理中断（状态 & 使能）
    pending = status & ie;

    // 第1优先级：BATCH_DONE
    if ((pending & OSPI_IRQ_BATCH_DONE) != 0U) {
        if (irq_state->batch_done_cb != NULL) {
            irq_state->batch_done_cb(inst);
        }
        clear_mask |= OSPI_IRQ_BATCH_DONE;
    }

    // 第2优先级：TX/RX BATCH_DONE（需追加 BATCH_DONE 以确保清除）
    if ((pending & OSPI_IRQ_TX_BATCH_DONE) != 0U) {
        if (irq_state->tx_batch_done_cb != NULL) {
            irq_state->tx_batch_done_cb(inst);
        }
    }
    if ((pending & OSPI_IRQ_RX_BATCH_DONE) != 0U) {
        if (irq_state->rx_batch_done_cb != NULL) {
            irq_state->rx_batch_done_cb(inst);
        }
    }
    if ((pending & (OSPI_IRQ_TX_BATCH_DONE | OSPI_IRQ_RX_BATCH_DONE)) != 0U) {
        clear_mask |= OSPI_IRQ_BATCH_DONE;
    }

    // 第3优先级：FIFO 状态
    if ((pending & OSPI_IRQ_TX_FIFO_EMPTY) != 0U) {
        if (irq_state->tx_fifo_empty_cb != NULL) {
            irq_state->tx_fifo_empty_cb(inst);
        }
    }
    if ((pending & OSPI_IRQ_TX_FIFO_FULL) != 0U) {
        if (irq_state->tx_fifo_full_cb != NULL) {
            irq_state->tx_fifo_full_cb(inst);
        }
    }
    if ((pending & OSPI_IRQ_TX_FIFO_HALF_EMPTY) != 0U) {
        if (irq_state->tx_fifo_half_empty_cb != NULL) {
            irq_state->tx_fifo_half_empty_cb(inst);
        }
    }
    if ((pending & OSPI_IRQ_TX_FIFO_HALF_FULL) != 0U) {
        if (irq_state->tx_fifo_half_full_cb != NULL) {
            irq_state->tx_fifo_half_full_cb(inst);
        }
    }
    if ((pending & OSPI_IRQ_RX_FIFO_EMPTY) != 0U) {
        if (irq_state->rx_fifo_empty_cb != NULL) {
            irq_state->rx_fifo_empty_cb(inst);
        }
    }
    if ((pending & OSPI_IRQ_RX_FIFO_FULL) != 0U) {
        if (irq_state->rx_fifo_full_cb != NULL) {
            irq_state->rx_fifo_full_cb(inst);
        }
    }
    if ((pending & OSPI_IRQ_RX_FIFO_HALF_EMPTY) != 0U) {
        if (irq_state->rx_fifo_half_empty_cb != NULL) {
            irq_state->rx_fifo_half_empty_cb(inst);
        }
    }
    if ((pending & OSPI_IRQ_RX_FIFO_HALF_FULL) != 0U) {
        if (irq_state->rx_fifo_half_full_cb != NULL) {
            irq_state->rx_fifo_half_full_cb(inst);
        }
    }
    if ((pending & OSPI_IRQ_RX_FIFO_NOT_EMPTY) != 0U) {
        if (irq_state->rx_fifo_not_empty_cb != NULL) {
            irq_state->rx_fifo_not_empty_cb(inst);
        }
    }

    // APM_DUMY_DONE：无对应 IE 使能位，直接检查状态
    if ((status & OSPI_IRQ_APM_DUMY_DONE) != 0U) {
        if (irq_state->apm_dumy_done_cb != NULL) {
            irq_state->apm_dumy_done_cb(inst);
        }
        clear_mask |= OSPI_IRQ_APM_DUMY_DONE;
    }

    // 清除 BATCH_DONE 和 APM_DUMY_DONE（写 1 清除）
    if (clear_mask != 0U) {
        ospi_write_reg(base, OSPI_STATUS_OFFSET, clear_mask);
    }
}

void OSPI1_IRQHandler(void)
{
    ospi_irq_handler(OSPI_1);
}

void OSPI2_IRQHandler(void)
{
    ospi_irq_handler(OSPI_2);
}