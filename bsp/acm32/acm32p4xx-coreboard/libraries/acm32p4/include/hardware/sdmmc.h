/**
 * @file sdmmc.h
 * @brief ACM32P4 SDMMC 驱动头文件
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#ifndef _HARDWARE_SDMMC_H
#define _HARDWARE_SDMMC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//===========================================
// 宏定义
//===========================================

#define SDMMC_FIFO_DEPTH              16U
#define SDMMC_DEFAULT_TIMEOUT_MS      1000U
#define SDMMC_CLK_DIV_MIN             2U
#define SDMMC_CLK_DIV_MAX             510U
#define SDMMC_BLOCK_SIZE_MAX          65536U
#define SDMMC_IDMAC_DESC_ALIGN        4U
#define SDMMC_CARD_COUNT              2U

//===========================================
// 枚举类型定义
//===========================================

/** @brief SDMMC 卡类型 */
typedef enum {
    SDMMC_CARD_TYPE_SD      = 0,  ///< SD 存储卡
    SDMMC_CARD_TYPE_SDIO    = 1,  ///< SDIO 卡
    SDMMC_CARD_TYPE_MMC     = 2,  ///< MMC/eMMC 卡
    SDMMC_CARD_TYPE_CEATA   = 3,  ///< CE-ATA 卡
} sdmmc_card_type_t;

/** @brief SDMMC 总线宽度 */
typedef enum {
    SDMMC_BUS_WIDTH_1BIT    = 0,  ///< 1 位数据总线
    SDMMC_BUS_WIDTH_4BIT    = 1,  ///< 4 位数据总线
    SDMMC_BUS_WIDTH_8BIT    = 2,  ///< 8 位数据总线（仅 MMC）
} sdmmc_bus_width_t;

/** @brief SDMMC 响应类型 */
typedef enum {
    SDMMC_RESPONSE_NONE         = 0,  ///< 无响应（如 CMD0）
    SDMMC_RESPONSE_SHORT        = 1,  ///< 短响应 48 位（如 CMD3）
    SDMMC_RESPONSE_LONG         = 2,  ///< 长响应 136 位（如 CMD2）
    SDMMC_RESPONSE_SHORT_BUSY   = 3,  ///< 短响应 + 忙检测（如 CMD7）
} sdmmc_response_type_t;

/** @brief SDMMC 传输模式 */
typedef enum {
    SDMMC_TRANSFER_MODE_POLLING   = 0,  ///< 轮询 FIFO
    SDMMC_TRANSFER_MODE_INTERRUPT = 1,  ///< 中断驱动
    SDMMC_TRANSFER_MODE_IDMA      = 2,  ///< 内部 DMA
} sdmmc_transfer_mode_t;

/** @brief SDMMC 传输方向 */
typedef enum {
    SDMMC_TRANSFER_DIR_NONE    = 0,  ///< 无数据传输
    SDMMC_TRANSFER_DIR_READ    = 1,  ///< 卡 → 主机
    SDMMC_TRANSFER_DIR_WRITE   = 2,  ///< 主机 → 卡
} sdmmc_transfer_dir_t;

/** @brief SDMMC 中断事件（位掩码，对应 RINTSTS 寄存器） */
typedef enum {
    SDMMC_IRQ_CDET         = (1UL << 0),   ///< 卡检测
    SDMMC_IRQ_RE           = (1UL << 1),   ///< 响应错误
    SDMMC_IRQ_CDONE        = (1UL << 2),   ///< 命令完成
    SDMMC_IRQ_DTO          = (1UL << 3),   ///< 数据超时
    SDMMC_IRQ_TXDR         = (1UL << 4),   ///< TX FIFO 数据请求
    SDMMC_IRQ_RXDR         = (1UL << 5),   ///< RX FIFO 数据请求
    SDMMC_IRQ_RCRC         = (1UL << 6),   ///< 响应 CRC 错误
    SDMMC_IRQ_DCRC         = (1UL << 7),   ///< 数据 CRC 错误
    SDMMC_IRQ_RTO          = (1UL << 8),   ///< 响应超时
    SDMMC_IRQ_DRTO         = (1UL << 9),   ///< 数据读超时
    SDMMC_IRQ_HTO          = (1UL << 10),  ///< 数据饥饿超时
    SDMMC_IRQ_FRUN         = (1UL << 11),  ///< FIFO 运行中
    SDMMC_IRQ_HLE          = (1UL << 12),  ///< 硬件锁定错误
    SDMMC_IRQ_SBE          = (1UL << 13),  ///< 起始位错误
    SDMMC_IRQ_ACD          = (1UL << 14),  ///< 自动命令完成
    SDMMC_IRQ_EBE          = (1UL << 15),  ///< 结束位错误
    SDMMC_IRQ_SDIO0        = (1UL << 16),  ///< SDIO 卡中断 0
    SDMMC_IRQ_SDIO1        = (1UL << 17),  ///< SDIO 卡中断 1
    SDMMC_IRQ_ALL          = 0x3FFFFUL,    ///< 所有中断
    SDMMC_IRQ_ERROR        = (SDMMC_IRQ_RE | SDMMC_IRQ_RCRC | SDMMC_IRQ_DCRC |
                               SDMMC_IRQ_RTO | SDMMC_IRQ_DRTO | SDMMC_IRQ_HTO |
                               SDMMC_IRQ_EBE | SDMMC_IRQ_SBE | SDMMC_IRQ_HLE),
} sdmmc_irq_t;

/** @brief SDMMC 卡状态（对应 R1 响应 bit[12:9]） */
typedef enum {
    SDMMC_CARD_STATE_IDLE     = 0,  ///< 空闲
    SDMMC_CARD_STATE_READY    = 1,  ///< 就绪
    SDMMC_CARD_STATE_IDENT    = 2,  ///< 识别
    SDMMC_CARD_STATE_STBY     = 3,  ///< 待机
    SDMMC_CARD_STATE_TRAN     = 4,  ///< 传输
    SDMMC_CARD_STATE_DATA     = 5,  ///< 发送数据
    SDMMC_CARD_STATE_RCV      = 6,  ///< 接收数据
    SDMMC_CARD_STATE_PRG      = 7,  ///< 编程
    SDMMC_CARD_STATE_DIS      = 8,  ///< 断开
} sdmmc_card_state_t;

/** @brief SDMMC 卡速度等级 */
typedef enum {
    SDMMC_CARD_SPEED_DEFAULT   = 0,  ///< 默认速度
    SDMMC_CARD_SPEED_HIGH      = 1,  ///< 高速
    SDMMC_CARD_SPEED_SDR12     = 2,  ///< SDR12
    SDMMC_CARD_SPEED_SDR25     = 3,  ///< SDR25
    SDMMC_CARD_SPEED_SDR50     = 4,  ///< SDR50
    SDMMC_CARD_SPEED_SDR104    = 5,  ///< SDR104
    SDMMC_CARD_SPEED_DDR50     = 6,  ///< DDR50
    SDMMC_CARD_SPEED_HS200     = 7,  ///< HS200
} sdmmc_card_speed_t;

/** @brief MMC CMD6 SWITCH 访问模式（对应 CMD6 ARG bit[25:24]） */
typedef enum {
    SDMMC_MMC_ACCESS_CMD_SET      = 0,  ///< 命令集切换
    SDMMC_MMC_ACCESS_SET_BITS     = 1,  ///< 置位（Set Bits）
    SDMMC_MMC_ACCESS_CLEAR_BITS   = 2,  ///< 清零（Clear Bits）
    SDMMC_MMC_ACCESS_WRITE_BYTE   = 3,  ///< 写字节（Write Byte）
} sdmmc_mmc_access_mode_t;

//===========================================
// 回调函数类型
//===========================================

/** @brief SDMMC 中断事件回调类型 */
typedef void (*sdmmc_irq_callback_t)(void);

//===========================================
// 结构体类型定义
//===========================================

/**
 * @brief SDMMC 传输描述符（统一轮询/中断/IDMA）
 */
typedef struct {
    sdmmc_transfer_mode_t mode;          ///< 传输模式
    sdmmc_transfer_dir_t  dir;           ///< 传输方向
    uint32_t             *buffer;        ///< 数据缓冲区（必须 4 字节对齐）
    uint32_t              block_size;    ///< 块大小（1-65536）
    uint32_t              block_count;   ///< 块数量
    void                (*callback)(bool success); ///< 传输完成回调（中断/IDMA 模式）
} sdmmc_transfer_t;

/**
 * @brief SDMMC 卡信息结构体
 */
typedef struct {
    sdmmc_card_type_t  card_type;        ///< 卡类型
    uint32_t           rca;              ///< 相对地址
    uint32_t           ocr;              ///< 操作条件寄存器
    uint32_t           cid[4];           ///< 卡 ID（128 位）
    uint32_t           csd[4];           ///< 卡特定数据（128 位）
    uint32_t           scr[2];           ///< SD 配置寄存器（64 位）
    uint32_t           capacity;         ///< 容量（块数）
    uint32_t           block_size;       ///< 块大小（字节）
    bool               high_capacity;    ///< 是否高容量卡（SDHC/SDXC）
    sdmmc_card_speed_t card_speed;       ///< 卡速度等级
} sdmmc_card_info_t;

/**
 * @brief SDMMC 中断回调注册表
 *
 * @note 每个成员对应一个中断源，NULL 表示不注册
 * @note ISR 中按位检查并使能的中断源才触发对应回调
 */
typedef struct {
    sdmmc_irq_callback_t card_detect;     ///< 卡检测中断（CDET）
    sdmmc_irq_callback_t cmd_done;        ///< 命令完成中断（CDONE）
    sdmmc_irq_callback_t tx_data_request; ///< TX FIFO 数据请求（TXDR）
    sdmmc_irq_callback_t rx_data_request; ///< RX FIFO 数据请求（RXDR）
    sdmmc_irq_callback_t data_timeout;    ///< 数据超时（DTO）
    sdmmc_irq_callback_t resp_timeout;    ///< 响应超时（RTO）
    sdmmc_irq_callback_t resp_crc_error;  ///< 响应 CRC 错误（RCRC）
    sdmmc_irq_callback_t data_crc_error;  ///< 数据 CRC 错误（DCRC）
    sdmmc_irq_callback_t data_read_timeout; ///< 数据读超时（DRTO）
    sdmmc_irq_callback_t data_starve_timeout; ///< 数据饥饿超时（HTO）
    sdmmc_irq_callback_t fifo_run;        ///< FIFO 运行中（FRUN）
    sdmmc_irq_callback_t hw_lock_error;   ///< 硬件锁定错误（HLE）
    sdmmc_irq_callback_t start_bit_error; ///< 起始位错误（SBE）
    sdmmc_irq_callback_t end_bit_error;   ///< 结束位错误（EBE）
    sdmmc_irq_callback_t auto_cmd_done;   ///< 自动命令完成（ACD）
    sdmmc_irq_callback_t sdio_interrupt0; ///< SDIO 卡中断 0（SDIO0）
    sdmmc_irq_callback_t sdio_interrupt1; ///< SDIO 卡中断 1（SDIO1）
} sdmmc_irq_callbacks_t;

/**
 * @brief SDMMC 初始化配置结构体
 */
typedef struct {
    uint32_t              clk_div;           ///< 时钟分频值（0=旁路, 1-510=分频）
    sdmmc_bus_width_t     bus_width;         ///< 总线宽度
    sdmmc_transfer_mode_t transfer_mode;     ///< 默认传输模式
    bool                  enable_ddr;        ///< 使能 DDR 模式
    bool                  enable_hw_reset;   ///< 使能硬件复位
    const sdmmc_irq_callbacks_t *irq_cbs;   ///< 中断回调注册表（NULL = 不使用中断）
} sdmmc_init_config_t;

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
 */
bool sdmmc_init(const sdmmc_init_config_t *config);

/**
 * @brief 反初始化 SDMMC 主机控制器
 *
 * @note 覆盖寄存器：CTRL, CLKENA, PWREN, INTMASK
 * @note 关闭时钟、禁用电源、清除中断
 */
void sdmmc_deinit(void);

//===========================================
// 第2层：基础配置 API
//===========================================

bool sdmmc_config_clock(uint32_t clk_div);
bool sdmmc_config_bus_width(sdmmc_bus_width_t width);
bool sdmmc_config_timeout(uint32_t data_timeout, uint8_t resp_timeout);
bool sdmmc_config_block_size(uint32_t block_size);
bool sdmmc_config_fifo_threshold(uint8_t tx_level, uint8_t rx_level);
bool sdmmc_config_card_detect(uint32_t debounce_count);

//===========================================
// 第3层：高级功能 API
//===========================================

/* --- 复位 --- */
void sdmmc_reset_controller(void);
void sdmmc_reset_fifo(void);
void sdmmc_reset_dma(void);

/* --- 电源与时钟 --- */
void sdmmc_power_on(void);
void sdmmc_power_off(void);
void sdmmc_clock_enable(void);
void sdmmc_clock_disable(void);

/* --- FIFO 操作 --- */
void sdmmc_write_fifo(const uint32_t *data, uint32_t word_count);
void sdmmc_read_fifo(uint32_t *data, uint32_t word_count);

/* --- 中断 --- */
void sdmmc_irq_enable(sdmmc_irq_t mask);
void sdmmc_irq_disable(sdmmc_irq_t mask);
void sdmmc_irq_clear(sdmmc_irq_t mask);
void sdmmc_irq_register(const sdmmc_irq_callbacks_t *cbs);
void sdmmc_irq_set_callback(sdmmc_irq_t irq, sdmmc_irq_callback_t cb);

/* --- IDMAC --- */
bool sdmmc_dma_config(uint32_t desc_base_addr, uint8_t pbl);
void sdmmc_dma_start(void);
void sdmmc_dma_stop(void);
bool sdmmc_dma_resume(void);

/* --- DDR / UHS --- */
void sdmmc_ddr_enable(bool enable);
void sdmmc_hw_reset_assert(void);
void sdmmc_hw_reset_deassert(void);

/* --- 初始化序列 --- */
void sdmmc_send_init_sequence(void);

//===========================================
// 第4层：控制与查询 API
//===========================================

sdmmc_irq_t  sdmmc_get_raw_irq_status(void);
sdmmc_irq_t  sdmmc_get_masked_irq_status(void);
uint32_t     sdmmc_get_status(void);
bool         sdmmc_is_card_inserted(void);
uint32_t     sdmmc_get_tcbcnt(void);
uint32_t     sdmmc_get_tbbcnt(void);
bool         sdmmc_is_fifo_full(void);
bool         sdmmc_is_fifo_empty(void);
uint32_t     sdmmc_get_fifo_count(void);
uint32_t     sdmmc_dma_get_status(void);
uint32_t     sdmmc_dma_get_current_desc(void);
uint32_t     sdmmc_dma_get_current_buffer(void);
uint32_t     sdmmc_get_response(uint8_t index);

//===========================================
// 通用卡命令层 API
//===========================================

bool sdmmc_send_cmd(uint8_t cmd_index, uint32_t cmd_arg,
                    sdmmc_response_type_t resp_type,
                    const sdmmc_transfer_t *transfer,
                    uint32_t timeout_ms);

bool sdmmc_cmd0_go_idle(void);
bool sdmmc_cmd2_all_send_cid(uint32_t *cid);
bool sdmmc_cmd3_send_relative_addr(uint32_t *rca);
bool sdmmc_cmd7_select_card(uint32_t rca);
bool sdmmc_cmd7_deselect_card(void);
bool sdmmc_cmd8_send_if_cond(uint8_t vhs, uint8_t pattern, uint32_t *response);
bool sdmmc_cmd9_send_csd(uint32_t *csd);
bool sdmmc_cmd10_send_cid(uint32_t rca, uint32_t *cid);
bool sdmmc_cmd12_stop_transmission(void);
bool sdmmc_cmd13_send_status(uint32_t rca, uint32_t *status);
bool sdmmc_cmd15_go_inactive(uint32_t rca);
bool sdmmc_cmd16_set_block_len(uint32_t block_len);
bool sdmmc_cmd17_read_single_block(uint32_t addr, uint32_t *buffer, uint32_t block_size);
bool sdmmc_cmd18_read_multiple_block(uint32_t addr, uint32_t *buffer,
                                     uint32_t block_size, uint32_t block_count);
bool sdmmc_cmd19_send_tuning_block(uint32_t *buffer);
bool sdmmc_cmd24_write_single_block(uint32_t addr, const uint32_t *buffer, uint32_t block_size);
bool sdmmc_cmd25_write_multiple_block(uint32_t addr, const uint32_t *buffer,
                                      uint32_t block_size, uint32_t block_count);

sdmmc_card_state_t sdmmc_get_card_state(uint32_t status);
bool               sdmmc_is_card_locked(uint32_t status);
bool               sdmmc_is_app_cmd(uint32_t status);
bool               sdmmc_is_card_ready(uint32_t status);

//===========================================
// SD 协议层 API
//===========================================

bool sdmmc_sd_init(sdmmc_card_info_t *card);
bool sdmmc_acmd6_set_bus_width(uint32_t rca, sdmmc_bus_width_t width);
bool sdmmc_acmd13_sd_status(uint32_t *status);
bool sdmmc_acmd41_send_op_cond(bool hcs, uint32_t *ocr);
bool sdmmc_acmd51_send_scr(uint32_t *scr);

bool sdmmc_sd_read_blocks(uint32_t start_block, uint32_t *buffer, uint32_t block_count);
bool sdmmc_sd_write_blocks(uint32_t start_block, const uint32_t *buffer, uint32_t block_count);
bool sdmmc_sd_erase(uint32_t start_block, uint32_t end_block);

bool     sdmmc_sd_get_card_info(sdmmc_card_info_t *card);
uint64_t sdmmc_sd_get_capacity(void);

//===========================================
// SDIO 协议层 API
//===========================================

bool sdmmc_sdio_init(sdmmc_card_info_t *card);
bool sdmmc_sdio_read_direct(uint8_t func, uint32_t addr, uint8_t *data);
bool sdmmc_sdio_write_direct(uint8_t func, uint32_t addr, uint8_t data, uint8_t *resp);
bool sdmmc_sdio_read_extended(uint8_t func, uint32_t addr,
                              bool block_mode, bool op_code,
                              uint32_t *buffer, uint32_t count);
bool sdmmc_sdio_write_extended(uint8_t func, uint32_t addr,
                               bool block_mode, bool op_code,
                               const uint32_t *buffer, uint32_t count);
bool sdmmc_sdio_read_byte(uint8_t func, uint32_t addr, uint8_t *data);
bool sdmmc_sdio_write_byte(uint8_t func, uint32_t addr, uint8_t data);
bool sdmmc_sdio_read_bytes(uint8_t func, uint32_t addr, uint8_t *data, uint32_t len);
bool sdmmc_sdio_write_bytes(uint8_t func, uint32_t addr, const uint8_t *data, uint32_t len);
bool sdmmc_sdio_enable_irq(void);
bool sdmmc_sdio_disable_irq(void);

//===========================================
// MMC 协议层 API
//===========================================

bool sdmmc_mmc_init(sdmmc_card_info_t *card);
bool sdmmc_cmd1_send_op_cond(uint32_t *ocr);
bool sdmmc_cmd6_switch(uint8_t index, uint8_t value, uint32_t *status);

bool sdmmc_mmc_read_blocks(uint32_t start_block, uint32_t *buffer, uint32_t block_count);
bool sdmmc_mmc_write_blocks(uint32_t start_block, const uint32_t *buffer, uint32_t block_count);
bool sdmmc_mmc_erase(uint32_t start_block, uint32_t end_block);

bool sdmmc_emmc_set_ddr(bool enable);
bool sdmmc_emmc_set_hs200(void);

//===========================================
// CE-ATA 协议层 API
//===========================================

bool sdmmc_ceata_init(sdmmc_card_info_t *card);
bool sdmmc_ceata_send_command(uint16_t cmd, uint8_t features,
                              uint32_t sectors, uint8_t device);
bool sdmmc_ceata_read_sectors(uint32_t lba, uint32_t *buffer, uint32_t sector_count);
bool sdmmc_ceata_write_sectors(uint32_t lba, const uint32_t *buffer, uint32_t sector_count);

#ifdef __cplusplus
}
#endif
#endif /* _HARDWARE_SDMMC_H */
