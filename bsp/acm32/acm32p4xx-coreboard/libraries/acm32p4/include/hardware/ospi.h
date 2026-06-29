/**
 * @file ospi.h
 * @brief OSPI 八线 SPI 接口驱动
 * @author ACM32 SDK Team
 * @version 1.0.0
 *
 * @details DLYB 联动说明：OSPI1 和 OSPI2 各自对应一个 DLYB 延迟块实例
 *         （DLYB1/DLYB2，基地址见 acm32p4xx.h 中 OSPI1_DLYB/OSPI2_DLYB）。
 *          在高速通信（八线 DTR / HyperBus / xOSPI）场景下，建议通过 DLYB
 *          驱动（dlyb.h）配置延迟校准以保证数据采样时序。本驱动不直接
 *          操作 DLYB 寄存器，需用户自行调用 dlyb_init / dlyb_config_xxx。
 */
#ifndef _HARDWARE_OSPI_H
#define _HARDWARE_OSPI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//===========================================
// 宏定义
//===========================================
#define OSPI_TIMEOUT_MS     1000U
#define OSPI_FIFO_SIZE      16U

//===========================================
// 枚举类型定义
//===========================================

typedef enum { OSPI_1 = 0, OSPI_2 = 1, OSPI_COUNT = 2 } ospi_instance_t;

typedef enum {
    OSPI_CPOL_LOW  = 0,
    OSPI_CPOL_HIGH = 1,
} ospi_cpol_t;

typedef enum {
    OSPI_CPHA_ODD_EDGE  = 0,
    OSPI_CPHA_EVEN_EDGE = 1,
} ospi_cpha_t;

typedef enum {
    OSPI_LINE_1  = 0,
    OSPI_LINE_2  = 1,
    OSPI_LINE_4  = 2,
    OSPI_LINE_8  = 3,
} ospi_line_t;

typedef enum {
    OSPI_RATE_SDR = 0,
    OSPI_RATE_DTR = 1,
} ospi_rate_t;

typedef enum {
    OSPI_MEM_XCC_ELA = 0,
    OSPI_MEM_APM     = 1,
    OSPI_MEM_HYPER   = 2,
    OSPI_MEM_XOSPI   = 3,
} ospi_memory_type_t;

typedef enum {
    OSPI_DATA_1LINE = 0,
    OSPI_DATA_2LINE = 1,
    OSPI_DATA_4LINE = 2,
} ospi_data_mode_t;

typedef enum {
    OSPI_ADDR_1LINE = 0,
    OSPI_ADDR_2LINE = 1,
    OSPI_ADDR_4LINE = 2,
} ospi_addr_mode_t;

typedef enum {
    OSPI_INSTR_1LINE = 0,
    OSPI_INSTR_2LINE = 1,
    OSPI_INSTR_4LINE = 2,
} ospi_instr_mode_t;

typedef enum {
    OSPI_ADDR_8BIT  = 0,
    OSPI_ADDR_16BIT = 1,
    OSPI_ADDR_24BIT = 2,
    OSPI_ADDR_32BIT = 3,
} ospi_addr_width_t;

typedef enum {
    OSPI_IRQ_BATCH_DONE         = (1U << 1),
    OSPI_IRQ_TX_FIFO_EMPTY      = (1U << 2),
    OSPI_IRQ_TX_FIFO_FULL       = (1U << 3),
    OSPI_IRQ_RX_FIFO_EMPTY      = (1U << 4),
    OSPI_IRQ_RX_FIFO_FULL       = (1U << 5),
    OSPI_IRQ_TX_FIFO_HALF_EMPTY = (1U << 6),
    OSPI_IRQ_TX_FIFO_HALF_FULL  = (1U << 7),
    OSPI_IRQ_RX_FIFO_HALF_EMPTY = (1U << 8),
    OSPI_IRQ_RX_FIFO_HALF_FULL  = (1U << 9),
    OSPI_IRQ_RX_FIFO_NOT_EMPTY  = (1U << 11),
    OSPI_IRQ_TX_BATCH_DONE      = (1U << 14),
    OSPI_IRQ_RX_BATCH_DONE      = (1U << 15),
    OSPI_IRQ_APM_DUMY_DONE      = (1U << 16),
} ospi_irq_type_t;

typedef enum {
    OSPI_DMA_LEVEL_1  = 1,
    OSPI_DMA_LEVEL_2  = 2,
    OSPI_DMA_LEVEL_4  = 4,
    OSPI_DMA_LEVEL_8  = 8,
    OSPI_DMA_LEVEL_16 = 16,
} ospi_dma_level_t;

typedef enum {
    OSPI_FIFO_BYTE = 0,
    OSPI_FIFO_HALF = 1,
    OSPI_FIFO_WORD = 2,
} ospi_fifo_mode_t;

typedef enum {
    OSPI_CS_0 = 1,
    OSPI_CS_1 = 2,
    OSPI_CS_2 = 4,
} ospi_cs_t;

typedef enum {
    OSPI_HYPER_WRAP   = 0,
    OSPI_HYPER_LINEAR = 1,
} ospi_hyper_burst_t;

typedef enum {
    OSPI_BURST_NONE = 0,
    OSPI_BURST_16B  = 1,
    OSPI_BURST_32B  = 2,
    OSPI_BURST_64B  = 3,
    OSPI_BURST_128B = 4,
    OSPI_BURST_256B = 5,
    OSPI_BURST_512B = 6,
    OSPI_BURST_1KB  = 7,
} ospi_burst_t;

typedef enum {
    OSPI_ALT_8BIT  = 0,
    OSPI_ALT_16BIT = 1,
    OSPI_ALT_24BIT = 2,
    OSPI_ALT_32BIT = 3,
} ospi_alt_size_t;

typedef enum {
    OSPI_WRAP_DISABLE = 0,
    OSPI_WRAP_16B     = 2,
    OSPI_WRAP_32B     = 3,
    OSPI_WRAP_64B     = 4,
} ospi_wrap_size_t;

//===========================================
// 结构体类型定义
//===========================================

typedef struct {
    uint8_t div1;
    uint8_t div2;
} ospi_baudrate_config_t;

typedef struct {
    ospi_cpol_t cpol;
    ospi_cpha_t cpha;
    bool msb_first;
    bool hw_io_mode;
} ospi_basic_config_t;

typedef struct {
    ospi_line_t line;
    ospi_rate_t rate;
    ospi_memory_type_t mem_type;
} ospi_protocol_config_t;

typedef struct {
    uint16_t read_cmd;
    uint16_t write_cmd;
    ospi_instr_mode_t instr_mode;
    ospi_addr_mode_t addr_mode;
    ospi_data_mode_t data_mode;
    ospi_addr_width_t addr_width;
    uint16_t read_dummy_cycles;
    uint16_t write_dummy_cycles;
    bool rd_db_en;
    bool wr_db_en;
    bool alternate_bytes_enable;
    ospi_data_mode_t alternate_bytes_mode;
    ospi_alt_size_t alternate_bytes_size;
    bool continuous_mode;
    bool send_instr_once;
    ospi_burst_t read_burst;
    ospi_burst_t write_burst;
} ospi_memory_mapped_config_t;

typedef struct {
    uint8_t instruction;
    uint32_t address;
    ospi_instr_mode_t instr_mode;
    ospi_addr_mode_t addr_mode;
    ospi_data_mode_t data_mode;
    ospi_addr_width_t addr_width;
    uint16_t dummy_cycles;
    bool alternate_bytes_enable;
    ospi_data_mode_t alternate_bytes_mode;
    uint32_t alternate_bytes;
    ospi_alt_size_t alt_size;
} ospi_command_config_t;

typedef struct {
    bool tx_enable;
    bool rx_enable;
    ospi_dma_level_t tx_level;
    ospi_dma_level_t rx_level;
} ospi_dma_config_t;

typedef struct {
    ospi_cs_t cs;
    bool cs_mapping_enable;
} ospi_cs_config_t;

typedef struct {
    bool enable;
    uint8_t shift_value;
    uint8_t msda_mask;
    uint8_t msdha_mask;
} ospi_sample_shift_config_t;

typedef struct {
    ospi_hyper_burst_t burst_type;
    uint8_t lc0;
    uint8_t lc1;
} ospi_hyperbus_config_t;

typedef void (*ospi_callback_t)(ospi_instance_t inst);

//===========================================
// 函数声明（外设时钟与复位 API）
//===========================================
bool ospi_enable_clock(ospi_instance_t inst);
bool ospi_disable_clock(ospi_instance_t inst);
bool ospi_reset(ospi_instance_t inst);

//===========================================
// 函数声明（第1层：快速初始化 API）
//===========================================
bool ospi_init_standard(ospi_instance_t inst, const ospi_baudrate_config_t *baudrate,
                        const ospi_basic_config_t *basic, const ospi_protocol_config_t *protocol);
bool ospi_init_memory_mapped(ospi_instance_t inst, const ospi_baudrate_config_t *baudrate,
                             const ospi_basic_config_t *basic, const ospi_memory_mapped_config_t *mem);
bool ospi_init_hyperbus(ospi_instance_t inst, const ospi_baudrate_config_t *baudrate,
                        const ospi_hyperbus_config_t *hyper);
bool ospi_init_xospi(ospi_instance_t inst, const ospi_baudrate_config_t *baudrate);

//===========================================
// 函数声明（第2层：基础配置 API）
//===========================================
bool ospi_config_baudrate(ospi_instance_t inst, const ospi_baudrate_config_t *baudrate);
bool ospi_config_mode(ospi_instance_t inst, const ospi_basic_config_t *basic);
bool ospi_config_protocol(ospi_instance_t inst, const ospi_protocol_config_t *protocol);
bool ospi_config_memory_mapped(ospi_instance_t inst, const ospi_memory_mapped_config_t *mem);
bool ospi_config_dma(ospi_instance_t inst, const ospi_dma_config_t *dma);
bool ospi_config_cs(ospi_instance_t inst, const ospi_cs_config_t *cs);
bool ospi_config_cs_time(ospi_instance_t inst, uint8_t cycles);
bool ospi_config_wait_cycles(ospi_instance_t inst, uint32_t cycles);
bool ospi_config_alternate_bytes(ospi_instance_t inst, uint32_t bytes, ospi_alt_size_t size);
bool ospi_config_fifo_mode(ospi_instance_t inst, ospi_fifo_mode_t tx_mode, ospi_fifo_mode_t rx_mode);
bool ospi_config_data_mask(ospi_instance_t inst, bool enable, bool mask_odd);
bool ospi_config_output_delay(ospi_instance_t inst, uint8_t delay);
bool ospi_config_dummy_byte(ospi_instance_t inst, uint8_t dummy);
bool ospi_config_cs_timeout(ospi_instance_t inst, uint16_t timeout, uint16_t delay);
bool ospi_config_cmd_format(ospi_instance_t inst, bool two_byte_cmd, bool dual_edge_cmd);
bool ospi_config_cmd_value(ospi_instance_t inst, uint32_t cmd_value);
bool ospi_config_apm_dummy_clock(ospi_instance_t inst, uint8_t clock_count);
bool ospi_config_master_mode(ospi_instance_t inst, bool enable);
bool ospi_config_cs_timeout_enable(ospi_instance_t inst, bool enable);
bool ospi_config_alternate_bytes_mode(ospi_instance_t inst, ospi_data_mode_t mode);
bool ospi_config_write_dummy_size(ospi_instance_t inst, uint8_t size);
bool ospi_config_wrap_size(ospi_instance_t inst, ospi_wrap_size_t size);
bool ospi_config_hyperbus(ospi_instance_t inst, const ospi_hyperbus_config_t *hyper);
bool ospi_config_read_timing(ospi_instance_t inst, uint8_t read_dummy_cycles,
                             uint8_t sample_shift);
bool ospi_config_read_write_timing(ospi_instance_t inst, uint8_t read_dummy_cycles,
                                   uint8_t sample_shift, uint8_t output_delay);

//===========================================
// 函数声明（第3层：高级功能 API）
//===========================================
bool ospi_enable_memory_mapped(ospi_instance_t inst);
void ospi_disable_memory_mapped(ospi_instance_t inst);
bool ospi_send_command(ospi_instance_t inst, const ospi_command_config_t *cmd);
void ospi_set_batch(ospi_instance_t inst, uint32_t count);
uint8_t ospi_read_fifo(ospi_instance_t inst);
void ospi_write_fifo(ospi_instance_t inst, uint8_t data);
uint16_t ospi_read_fifo_half(ospi_instance_t inst);
void ospi_write_fifo_half(ospi_instance_t inst, uint16_t data);
uint32_t ospi_read_fifo_word(ospi_instance_t inst);
void ospi_write_fifo_word(ospi_instance_t inst, uint32_t data);
bool ospi_config_sample_shift(ospi_instance_t inst, const ospi_sample_shift_config_t *shift);
bool ospi_config_dqs(ospi_instance_t inst, bool tx_enable, bool rx_enable);
bool ospi_config_dummy_off(ospi_instance_t inst, bool enable, uint8_t size, bool wr_enable, uint8_t wr_size);
void ospi_clear_once_instr_flag(ospi_instance_t inst);
bool ospi_write_fifo_half_safe(ospi_instance_t inst, uint16_t data);
bool ospi_read_fifo_half_safe(ospi_instance_t inst, uint16_t *data);

//===========================================
// 函数声明（第4层：控制与查询 API）
//===========================================
void ospi_start_tx(ospi_instance_t inst);
void ospi_start_rx(ospi_instance_t inst);
void ospi_start(ospi_instance_t inst);
void ospi_stop_tx(ospi_instance_t inst);
void ospi_stop_rx(ospi_instance_t inst);
void ospi_stop(ospi_instance_t inst);
void ospi_reset_fifo(ospi_instance_t inst, bool tx, bool rx);
uint32_t ospi_get_status(ospi_instance_t inst);
uint8_t ospi_get_tx_fifo_level(ospi_instance_t inst);
uint8_t ospi_get_rx_fifo_level(ospi_instance_t inst);
bool ospi_is_busy(ospi_instance_t inst);
void ospi_enable_interrupt(ospi_instance_t inst, uint32_t irq_type);
void ospi_disable_interrupt(ospi_instance_t inst, uint32_t irq_type);
void ospi_register_callback(ospi_instance_t inst, uint32_t irq_type, ospi_callback_t callback);
uint32_t ospi_get_memory_address(ospi_instance_t inst);
void ospi_config_out_en(ospi_instance_t inst, uint8_t io_enable_bits);
uint32_t ospi_get_irq_status(ospi_instance_t inst);
void ospi_clear_irq_status(ospi_instance_t inst, uint32_t mask);
bool ospi_get_rwds(ospi_instance_t inst);
bool ospi_is_instr_sent(ospi_instance_t inst);
bool ospi_wait_status(ospi_instance_t inst, uint32_t mask, bool wait_set, uint32_t timeout_ms);
bool ospi_is_memory_mapped_enabled(ospi_instance_t inst);
void ospi_assert_cs(ospi_instance_t inst);
void ospi_release_cs(ospi_instance_t inst);

#ifdef __cplusplus
}
#endif

#endif // _HARDWARE_OSPI_H
