/**
 * @file spi.h
 * @brief ACM32P4 SPI 驱动头文件
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#ifndef _HARDWARE_SPI_H
#define _HARDWARE_SPI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//===========================================
// 宏定义
//===========================================

#define SPI_FIFO_DEPTH               16U
#define SPI_DEFAULT_TIMEOUT_MS       100U
#define SPI_DIV1_MIN                 2U
#define SPI_DIV1_MAX                 254U
#define SPI_DIV2_MAX                 255U
#define SPI_BATCH_MAX                0x000FFFFFU

//===========================================
// 枚举类型定义
//===========================================

/**
 * @brief SPI 外设实例
 */
typedef enum {
    SPI_1     = 0,  ///< SPI1，对应 QSPI1_BASE
    SPI_2     = 1,  ///< SPI2，对应 QSPI2_BASE
    SPI_3     = 2,  ///< SPI3，对应 QSPI3_BASE
    SPI_4     = 3,  ///< SPI4，对应 QSPI4_BASE
    SPI_5     = 4,  ///< SPI5，对应 QSPI7_BASE
    SPI_COUNT = 5,
} spi_instance_t;

/**
 * @brief SPI 主从模式
 */
typedef enum {
    SPI_MODE_SLAVE  = 0,
    SPI_MODE_MASTER = 1,
} spi_mode_t;

/**
 * @brief SPI 工作模式
 */
typedef enum {
    SPI_WORK_MODE_0 = 0,  ///< CPOL=0, CPHA=0
    SPI_WORK_MODE_1 = 1,  ///< CPOL=0, CPHA=1
    SPI_WORK_MODE_2 = 2,  ///< CPOL=1, CPHA=0
    SPI_WORK_MODE_3 = 3,  ///< CPOL=1, CPHA=1
} spi_work_mode_t;

/**
 * @brief SPI 线模式
 */
typedef enum {
    SPI_LINE_MODE_1 = 0,
    SPI_LINE_MODE_2 = 1,
    SPI_LINE_MODE_4 = 2,
} spi_line_mode_t;

/**
 * @brief SPI 位序
 */
typedef enum {
    SPI_BIT_ORDER_MSB_FIRST = 0,
    SPI_BIT_ORDER_LSB_FIRST = 1,
} spi_bit_order_t;

/**
 * @brief SPI IO 方向控制模式
 */
typedef enum {
    SPI_IO_DIRECTION_MANUAL = 0,
    SPI_IO_DIRECTION_AUTO   = 1,
} spi_io_direction_t;

/**
 * @brief SPI 片选信号（可组合）
 */
typedef enum {
    SPI_CS_NONE = 0x00U,
    SPI_CS_0    = 0x01U,
    SPI_CS_1    = 0x02U,
    SPI_CS_2    = 0x04U,
    SPI_CS_3    = 0x08U,
    SPI_CS_4    = 0x10U,
} spi_chip_select_t;

/**
 * @brief SPI 手动输出使能位（OUT_EN）
 */
typedef enum {
    SPI_IO_NONE = 0x00U,
    SPI_IO_MOSI = 0x01U,
    SPI_IO_MISO = 0x02U,
    SPI_IO_WP   = 0x04U,
    SPI_IO_HOLD = 0x08U,
} spi_io_output_t;

/**
 * @brief SPI DMA / 中断阈值（4bit）
 */
typedef uint8_t spi_level_t;

/**
 * @brief SPI 主机采样移位配置
 *
 * 该枚举值为 RX_CTL.SSHIFT1/SSHIFT2 的原始编码值。
 */
typedef enum {
    SPI_SAMPLE_SHIFT_NONE      = 0x00000000UL,
    SPI_SAMPLE_SHIFT_1_HCLK    = 0x00000100UL,
    SPI_SAMPLE_SHIFT_1_5_HCLK  = 0x00000200UL,
    SPI_SAMPLE_SHIFT_2_HCLK    = 0x00000300UL,
    SPI_SAMPLE_SHIFT_2_5_HCLK  = 0x01000000UL,
    SPI_SAMPLE_SHIFT_3_HCLK    = 0x01000100UL,
} spi_sample_shift_t;

/**
 * @brief SPI 中断类型（IE 寄存器位掩码）
 */
typedef enum {
    SPI_IRQ_BATCH_DONE              = (1UL << 1),
    SPI_IRQ_TX_FIFO_EMPTY           = (1UL << 2),
    SPI_IRQ_TX_FIFO_FULL            = (1UL << 3),
    SPI_IRQ_RX_FIFO_EMPTY           = (1UL << 4),
    SPI_IRQ_RX_FIFO_FULL            = (1UL << 5),
    SPI_IRQ_TX_FIFO_HALF_EMPTY      = (1UL << 6),
    SPI_IRQ_TX_FIFO_HALF_FULL       = (1UL << 7),
    SPI_IRQ_RX_FIFO_HALF_EMPTY      = (1UL << 8),
    SPI_IRQ_RX_FIFO_HALF_FULL       = (1UL << 9),
    SPI_IRQ_CS_POS                  = (1UL << 10),
    SPI_IRQ_RX_FIFO_NOT_EMPTY       = (1UL << 11),
    SPI_IRQ_RX_FIFO_EMPTY_OVERFLOW  = (1UL << 12),
    SPI_IRQ_RX_FIFO_FULL_OVERFLOW   = (1UL << 13),
    SPI_IRQ_TX_BATCH_DONE           = (1UL << 14),
    SPI_IRQ_RX_BATCH_DONE           = (1UL << 15),
    SPI_IRQ_ALL                     = 0x0000FFFEL,
} spi_irq_t;

/**
 * @brief SPI 状态标志（STATUS 寄存器位掩码）
 */
typedef enum {
    SPI_FLAG_BUSY                    = (1UL << 0),
    SPI_FLAG_BATCH_DONE              = (1UL << 1),
    SPI_FLAG_TX_FIFO_EMPTY           = (1UL << 2),
    SPI_FLAG_TX_FIFO_FULL            = (1UL << 3),
    SPI_FLAG_RX_FIFO_EMPTY           = (1UL << 4),
    SPI_FLAG_RX_FIFO_FULL            = (1UL << 5),
    SPI_FLAG_TX_FIFO_HALF_EMPTY      = (1UL << 6),
    SPI_FLAG_TX_FIFO_HALF_FULL       = (1UL << 7),
    SPI_FLAG_RX_FIFO_HALF_EMPTY      = (1UL << 8),
    SPI_FLAG_RX_FIFO_HALF_FULL       = (1UL << 9),
    SPI_FLAG_CS_POS                  = (1UL << 10),
    SPI_FLAG_RX_FIFO_NOT_EMPTY       = (1UL << 11),
    SPI_FLAG_RX_FIFO_EMPTY_OVERFLOW  = (1UL << 12),
    SPI_FLAG_RX_FIFO_FULL_OVERFLOW   = (1UL << 13),
    SPI_FLAG_TX_BATCH_DONE           = (1UL << 14),
    SPI_FLAG_RX_BATCH_DONE           = (1UL << 15),
    SPI_FLAG_INSTR_SEND_DONE         = (1UL << 16),
} spi_flag_t;

/**
 * @brief SPI 内存映射线模式
 */
typedef enum {
    SPI_MEM_LINE_MODE_1 = 0,
    SPI_MEM_LINE_MODE_2 = 1,
    SPI_MEM_LINE_MODE_4 = 2,
} spi_mem_line_mode_t;

/**
 * @brief SPI 地址宽度
 */
typedef enum {
    SPI_ADDR_WIDTH_8  = 0,
    SPI_ADDR_WIDTH_16 = 1,
    SPI_ADDR_WIDTH_24 = 2,
    SPI_ADDR_WIDTH_32 = 3,
} spi_addr_width_t;

/**
 * @brief 交替字节长度
 */
typedef enum {
    SPI_ALTER_BYTE_SIZE_8  = 0,
    SPI_ALTER_BYTE_SIZE_16 = 1,
    SPI_ALTER_BYTE_SIZE_24 = 2,
    SPI_ALTER_BYTE_SIZE_32 = 3,
} spi_alter_byte_size_t;

/**
 * @brief Dummy 周期数配置
 */
typedef enum {
    SPI_DUMMY_CYCLE_1 = 0,
    SPI_DUMMY_CYCLE_2 = 1,
    SPI_DUMMY_CYCLE_3 = 2,
    SPI_DUMMY_CYCLE_4 = 3,
    SPI_DUMMY_CYCLE_5 = 4,
    SPI_DUMMY_CYCLE_6 = 5,
    SPI_DUMMY_CYCLE_7 = 6,
    SPI_DUMMY_CYCLE_8 = 7,
} spi_dummy_cycle_t;

//===========================================
// 结构体类型定义
//===========================================

/**
 * @brief SPI 基础配置结构体
 */
typedef struct {
    spi_mode_t         mode;                  ///< 主从模式
    spi_work_mode_t    work_mode;             ///< 时钟模式
    spi_line_mode_t    line_mode;             ///< 线模式
    spi_bit_order_t    bit_order;             ///< 位序
    spi_io_direction_t io_direction;          ///< IO 自动/手动方向控制
    bool               slave_enable;          ///< 从机状态机使能（CTL.SLAVE_EN）
    bool               sck_filter_enable;     ///< 从机 SCK 滤波（CTL.SFILTER）
    bool               cs_reset_enable;       ///< 片选复位（CTL.CS_RST）
    bool               cs_filter_enable;      ///< CS 滤波（CTL.CS_FILTER）
    uint8_t            cs_time;               ///< CS 保持时间（0-255）
    bool               software_cs_enable;    ///< 从机软件 CS 使能（CTL.SWCS_EN）
    bool               software_cs_inactive;  ///< 软件 CS 状态（true=无效高，false=有效低）
} spi_config_t;

/**
 * @brief SPI 波特率配置结构体
 */
typedef struct {
    uint8_t div1;  ///< 一级分频，必须为 2-254 间偶数
    uint8_t div2;  ///< 二级分频，0-255
} spi_baud_config_t;

/**
 * @brief SPI 发送配置结构体
 */
typedef struct {
    bool        enable;         ///< TX 使能
    bool        batch_mode;     ///< TX_MODE（0=batch，1=非 batch）
    bool        dma_enable;     ///< TX DMA 请求使能
    spi_level_t dma_level;      ///< TX DMA 请求阈值（0-15）
    uint8_t     dummy_data;     ///< 半双工接收时输出的 Dummy 电平字节
} spi_tx_config_t;

/**
 * @brief SPI 接收配置结构体
 */
typedef struct {
    bool               enable;               ///< RX 使能
    bool               dma_enable;           ///< RX DMA 请求使能
    spi_level_t        dma_level;            ///< RX DMA 请求阈值（0-15）
    spi_sample_shift_t sample_shift;         ///< 主机采样移位
    bool               multi_sample_enable;  ///< 多数据线采样调节使能（MSDA_EN）
    uint8_t            msda_mask;            ///< 数据线采样位移掩码 bit0-3 -> X0-X3
    uint8_t            msdha_mask;           ///< 半沿采样掩码 bit0-3 -> X0-X3
} spi_rx_config_t;

/**
 * @brief SPI 内存映射配置结构体
 */
typedef struct {
    bool                   access_enable;            ///< MEMO_ACC.ACC_EN
    bool                   cs_timeout_enable;        ///< MEMO_ACC.CS_TOUT_EN
    bool                   continuous_mode_enable;   ///< MEMO_ACC.CON_MODE_EN
    bool                   send_instr_once_enable;   ///< MEMO_ACC.SEND_INSTR_ONCE_EN
    bool                   write_alter_byte_enable;  ///< MEMO_ACC.WR_AB_EN
    bool                   read_alter_byte_enable;   ///< MEMO_ACC.RD_AB_EN
    spi_alter_byte_size_t  alter_byte_size;          ///< MEMO_ACC.ALTER_BYTE_SIZE
    bool                   write_data_enable;        ///< MEMO_ACC.WR_DB_EN
    bool                   read_data_enable;         ///< MEMO_ACC.RD_DB_EN
    spi_dummy_cycle_t      dummy_cycle;              ///< MEMO_ACC.DUMMY_CYCLE_SIZE
    spi_addr_width_t       addr_width;               ///< MEMO_ACC.ADDR_WIDTH
    spi_mem_line_mode_t    instr_mode;               ///< MEMO_ACC.INSTR_MODE
    spi_mem_line_mode_t    addr_mode;                ///< MEMO_ACC.ADDR_MODE
    spi_mem_line_mode_t    alter_byte_mode;          ///< MEMO_ACC.ALTER_BYTE_MODE
    spi_mem_line_mode_t    data_mode;                ///< MEMO_ACC.DATA_MODE
} spi_memory_config_t;

/**
 * @brief SPI 内存映射命令配置结构体
 */
typedef struct {
    uint8_t read_cmd;   ///< 读命令
    uint8_t write_cmd;  ///< 写命令
} spi_mem_command_t;

/**
 * @brief SPI 中断上下文
 */
typedef struct {
    spi_instance_t inst;   ///< SPI 实例
    uint32_t       flags;  ///< STATUS 快照
} spi_irq_context_t;

//===========================================
// 回调类型定义
//===========================================

/**
 * @brief SPI 中断回调函数类型
 */
typedef void (*spi_callback_t)(const spi_irq_context_t *ctx);

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
 */
bool spi_init_master(spi_instance_t inst, uint32_t spi_clk_hz, spi_work_mode_t work_mode);

/**
 * @brief 快速初始化 SPI 从模式
 *
 * 默认配置：模式0、1线、MSB 先行、自动 IO 方向、从机状态机使能、batch=1。
 *
 * @param[in] inst       SPI 实例
 * @param[in] work_mode  SPI 工作模式
 * @return true: 初始化成功, false: 参数非法
 */
bool spi_init_slave(spi_instance_t inst, spi_work_mode_t work_mode);

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
 */
bool spi_configure(spi_instance_t inst, const spi_config_t *config);

/**
 * @brief 配置 SPI 波特率
 *
 * @param[in] inst    SPI 实例
 * @param[in] config  波特率配置结构体，不能为 NULL
 * @return true: 配置成功, false: 参数非法
 *
 */
bool spi_config_baud(spi_instance_t inst, const spi_baud_config_t *config);

/**
 * @brief 配置 SPI 发送控制寄存器
 *
 * @param[in] inst    SPI 实例
 * @param[in] config  发送配置结构体，不能为 NULL
 * @return true: 配置成功, false: 参数非法
 *
 */
bool spi_config_tx(spi_instance_t inst, const spi_tx_config_t *config);

/**
 * @brief 配置 SPI 接收控制寄存器
 *
 * @param[in] inst    SPI 实例
 * @param[in] config  接收配置结构体，不能为 NULL
 * @return true: 配置成功, false: 参数非法
 *
 */
bool spi_config_rx(spi_instance_t inst, const spi_rx_config_t *config);

/**
 * @brief 配置 SPI 内存映射访问模式
 *
 * @param[in] inst    SPI 实例
 * @param[in] config  内存映射配置结构体，不能为 NULL
 * @return true: 配置成功, false: 参数非法
 *
 */
bool spi_config_memory(spi_instance_t inst, const spi_memory_config_t *config);

/**
 * @brief 配置 SPI 内存映射命令寄存器
 *
 * @param[in] inst    SPI 实例
 * @param[in] config  命令配置结构体，不能为 NULL
 */
void spi_config_mem_command(spi_instance_t inst, const spi_mem_command_t *config);

//===========================================
// 第3层：高级功能 API
//===========================================

/**
 * @brief 选择默认片选线
 *
 * @param[in] inst  SPI 实例
 * @param[in] cs    片选掩码，可组合
 */
void spi_select_cs(spi_instance_t inst, spi_chip_select_t cs);

/**
 * @brief 手动拉低当前默认片选
 *
 * @param[in] inst SPI 实例
 */
void spi_assert_cs(spi_instance_t inst);

/**
 * @brief 手动释放当前默认片选
 *
 * @param[in] inst SPI 实例
 */
void spi_release_cs(spi_instance_t inst);

/**
 * @brief 配置非 batch / 连续传输的数据批次长度
 *
 * @param[in] inst   SPI 实例
 * @param[in] batch  批次数（1 ~ SPI_BATCH_MAX）
 * @return true: 配置成功, false: 参数非法
 */
bool spi_set_batch_size(spi_instance_t inst, uint32_t batch);

/**
 * @brief 配置 TX 延时寄存器
 *
 * @param[in] inst      SPI 实例
 * @param[in] delay_hclk 延时 HCLK 周期数
 */
void spi_set_tx_delay(spi_instance_t inst, uint32_t delay_hclk);

/**
 * @brief 配置手动 IO 输出使能
 *
 * @param[in] inst      SPI 实例
 * @param[in] outputs   输出掩码
 */
void spi_set_output_enable(spi_instance_t inst, spi_io_output_t outputs);

/**
 * @brief 设置交替字节值
 *
 * @param[in] inst   SPI 实例
 * @param[in] value  交替字节寄存器值
 */
void spi_set_alter_byte(spi_instance_t inst, uint32_t value);

/**
 * @brief 设置 CS 超时值
 *
 * @param[in] inst     SPI 实例
 * @param[in] timeout  超时计数值（16bit）
 */
void spi_set_cs_timeout(spi_instance_t inst, uint16_t timeout);

/**
 * @brief 清除"仅发送一次指令"状态
 *
 * @param[in] inst SPI 实例
 */
void spi_mem_clear_once_instruction(spi_instance_t inst);

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
void spi_sid_enable(spi_instance_t inst, bool enable);

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
void spi_sid_write_rgb(spi_instance_t inst, uint32_t rgb);

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
void spi_sid_set_reset_val(spi_instance_t inst, uint32_t hclk_cnt);

/**
 * @brief 注册 SPI 中断回调
 *
 * @param[in] inst      SPI 实例
 * @param[in] callback  回调函数，可为 NULL
 */
void spi_irq_register(spi_instance_t inst, spi_callback_t callback);

/**
 * @brief 使能 SPI 中断
 *
 * @param[in] inst  SPI 实例
 * @param[in] irq   中断掩码，可组合
 */
void spi_irq_enable(spi_instance_t inst, spi_irq_t irq);

/**
 * @brief 禁用 SPI 中断
 *
 * @param[in] inst  SPI 实例
 * @param[in] irq   中断掩码，可组合
 */
void spi_irq_disable(spi_instance_t inst, spi_irq_t irq);

//===========================================
// 第4层：控制与查询 API
//===========================================

/**
 * @brief 复位 TX FIFO
 *
 * @param[in] inst SPI 实例
 */
void spi_tx_fifo_reset(spi_instance_t inst);

/**
 * @brief 复位 RX FIFO
 *
 * @param[in] inst SPI 实例
 */
void spi_rx_fifo_reset(spi_instance_t inst);

/**
 * @brief 使能 TX
 *
 * @param[in] inst SPI 实例
 */
void spi_tx_enable(spi_instance_t inst);

/**
 * @brief 禁用 TX
 *
 * @param[in] inst SPI 实例
 */
void spi_tx_disable(spi_instance_t inst);

/**
 * @brief 使能 RX
 *
 * @param[in] inst SPI 实例
 */
void spi_rx_enable(spi_instance_t inst);

/**
 * @brief 禁用 RX
 *
 * @param[in] inst SPI 实例
 */
void spi_rx_disable(spi_instance_t inst);

/**
 * @brief 向 DAT 写入 1 字节
 *
 * @param[in] inst  SPI 实例
 * @param[in] data  数据字节
 */
void spi_write_data(spi_instance_t inst, uint8_t data);

/**
 * @brief 从 DAT 读取 1 字节
 *
 * @param[in] inst SPI 实例
 * @return 接收到的 8bit 数据
 */
uint8_t spi_read_data(spi_instance_t inst);

/**
 * @brief 获取 STATUS 寄存器
 *
 * @param[in] inst SPI 实例
 * @return STATUS 寄存器快照
 */
uint32_t spi_get_flags(spi_instance_t inst);

/**
 * @brief 判断指定状态标志是否置位
 *
 * @param[in] inst   SPI 实例
 * @param[in] flag   状态标志
 * @return true: 已置位, false: 未置位
 */
bool spi_flag_is_set(spi_instance_t inst, spi_flag_t flag);

/**
 * @brief 清除状态标志
 *
 * @param[in] inst   SPI 实例
 * @param[in] flags  要清除的标志掩码
 */
void spi_clear_flags(spi_instance_t inst, uint32_t flags);

/**
 * @brief 等待 SPI 空闲
 *
 * @param[in] inst        SPI 实例
 * @param[in] timeout_ms  超时时间，0 使用默认值
 * @return true: 空闲, false: 超时
 */
bool spi_wait_idle(spi_instance_t inst, uint32_t timeout_ms);

/**
 * @brief 阻塞式发送
 *
 * @param[in] inst        SPI 实例
 * @param[in] data        发送缓冲区
 * @param[in] len         发送长度
 * @param[in] timeout_ms  超时时间，0 使用默认值
 * @return true: 发送成功, false: 参数非法或超时
 */
bool spi_transmit(spi_instance_t inst, const uint8_t *data, size_t len, uint32_t timeout_ms);

/**
 * @brief 阻塞式接收
 *
 * @param[in] inst        SPI 实例
 * @param[out] data       接收缓冲区
 * @param[in] len         接收长度
 * @param[in] timeout_ms  超时时间，0 使用默认值
 * @return true: 接收成功, false: 参数非法或超时
 */
bool spi_receive(spi_instance_t inst, uint8_t *data, size_t len, uint32_t timeout_ms);

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
 * @note 推荐仅在 1 线模式下使用全双工；2/4 线模式硬件文档定义为半双工。
 */
bool spi_transceive(spi_instance_t inst, const uint8_t *tx_data, uint8_t *rx_data,
                    size_t len, uint32_t timeout_ms);

//===========================================
// 中断服务函数声明
//===========================================

/**
 * @brief QSPIx 中断服务函数
 *
 * 各 SPI 实例的中断入口，内部调用统一的中断处理函数。
 * 注意：中断名称使用 QSPI 前缀，与启动文件保持一致。
 *
 * @note 由中断向量表直接调用
 */
void QSPI1_IRQHandler(void);
void QSPI2_IRQHandler(void);
void QSPI3_IRQHandler(void);
void QSPI4_IRQHandler(void);
void QSPI7_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* _HARDWARE_SPI_H */