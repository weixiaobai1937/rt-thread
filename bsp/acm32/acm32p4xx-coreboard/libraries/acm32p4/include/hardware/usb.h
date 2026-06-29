/**
 * @file usb.h
 * @brief ACM32P4 USB 设备控制器驱动（Device 模式）
 *
 * ACM32P4 集成了 1 路 USB 2.0 全速设备控制器，支持：
 * - EP0 控制端点 + EP1~EP6 通用双向端点
 * - 端点最大包长 64 字节
 * - FIFO（32bit）和 Memory（8/16/32bit）两种访问模式
 * - 挂起、唤醒和远程唤醒
 * - Toggle 硬件比对与软件控制
 * - LPM（Link Power Management）
 * - 同步传输（EP1~EP6）
 * - 可选 CRC/NAK/EOP 错误回复
 *
 * @code
 * usb_device_config_t cfg = {
 *     .phy         = { .type = USB_PHY_EMBEDDED },
 *     .speed       = USB_SPEED_FULL,
 *     .ep_count    = 3,
 *     .dma_enable  = false,
 * };
 * usb_device_callback_set(USB_1, &my_cbs);
 * usb_init_device(USB_1, &cfg);
 * usb_device_connect(USB_1);
 * @endcode
 *
 * @note 中断架构：
 *   - USB_IRQn (13) — USB 主中断
 *   - USB_WKUP_IRQn (37) — USB 唤醒中断
 */

#ifndef _HARDWARE_USB_H
#define _HARDWARE_USB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// 宏定义
//=============================================================================

/** USB 实例数量（ACM32P4 仅 1 路） */
#define USB_INSTANCE_COUNT      1U

/** 最大端点数（EP0~EP6） */
#define USB_MAX_EP_COUNT        7U

/** 端点最大包长（字节） */
#define USB_MAX_PACKET_SIZE     64U

//=============================================================================
// 枚举类型定义
//=============================================================================

/**
 * @brief USB 外设实例
 */
typedef enum {
    USB_1 = 0   ///< USB 设备控制器，基地址 0x40040000
} usb_inst_t;

/**
 * @brief USB PHY 类型
 */
typedef enum {
    USB_PHY_EMBEDDED = 0   ///< 片上内置 PHY
} usb_phy_t;

/**
 * @brief USB PHY 接口配置（ACM32P4 无实际意义，保留兼容） */
typedef enum {
    USB_PHY_IF_16BIT_30MHZ = 0,
    USB_PHY_IF_8BIT_60MHZ  = 1
} usb_phy_if_t;

/**
 * @brief USB 速度
 */
typedef enum {
    USB_SPEED_FULL = 1,   ///< 全速（12Mbps）
    USB_SPEED_LOW  = 2    ///< 低速（1.5Mbps）
} usb_speed_t;

/**
 * @brief USB 端点传输类型
 */
typedef enum {
    USB_EP_TYPE_CTRL = 0,   ///< 控制传输
    USB_EP_TYPE_ISOC = 1,   ///< 同步传输
    USB_EP_TYPE_BULK = 2,   ///< 批量传输
    USB_EP_TYPE_INTR = 3    ///< 中断传输
} usb_ep_type_t;

/**
 * @brief USB 端点方向
 */
typedef enum {
    USB_EP_DIR_IN  = 0,   ///< IN 端点（设备→主机）
    USB_EP_DIR_OUT = 1    ///< OUT 端点（主机→设备）
} usb_ep_dir_t;

//=============================================================================
// 配置结构体
//=============================================================================

/**
 * @brief PHY 配置（ACM32P4 内置 PHY，仅保留兼容字段）
 */
typedef struct {
    usb_phy_t    type;        ///< PHY 类型
    usb_phy_if_t interface;   ///< 接口配置（保留）
    bool         refclk_12m;  ///< 参考时钟（保留）
    bool         pll_en;      ///< PLL 使能（保留）
} usb_phy_config_t;

/**
 * @brief Device 模式初始化配置
 */
typedef struct {
    usb_phy_config_t phy;         ///< PHY 配置
    usb_speed_t      speed;       ///< 目标速度
    uint8_t          ep_count;    ///< 使用的端点数（不含 EP0，1~6）
    bool             lpm_enable;  ///< LPM 使能
    bool             dma_enable;  ///< DMA 使能（ACM32P4 不支持，忽略）
} usb_device_config_t;

/**
 * @brief 端点配置
 */
typedef struct {
    uint8_t       ep_num;           ///< 端点编号（1~6）
    usb_ep_dir_t  direction;        ///< IN 或 OUT
    usb_ep_type_t type;             ///< 传输类型
    uint16_t      max_packet_size;  ///< 最大包长（字节，≤64）
} usb_ep_config_t;

//=============================================================================
// 回调函数类型定义
//=============================================================================

/** USB 复位完成回调 */
typedef void (*usb_dev_reset_cb_t)(usb_inst_t inst);

/** 枚举完成回调（ACM32P4 无枚举完成中断，复位后即视为完成） */
typedef void (*usb_dev_enum_done_cb_t)(usb_inst_t inst, usb_speed_t speed);

/** SETUP 阶段完成回调（buf = 8 字节 SETUP 数据） */
typedef void (*usb_dev_setup_stage_cb_t)(usb_inst_t inst, const uint8_t *buf);

/** IN 数据发送完成回调 */
typedef void (*usb_dev_data_in_cb_t)(usb_inst_t inst, uint8_t ep_num);

/** OUT 数据接收完成回调 */
typedef void (*usb_dev_data_out_cb_t)(usb_inst_t inst, uint8_t ep_num);

/** SOF 帧起始回调 */
typedef void (*usb_dev_sof_cb_t)(usb_inst_t inst);

/** 挂起回调 */
typedef void (*usb_dev_suspend_cb_t)(usb_inst_t inst);

/** 恢复回调 */
typedef void (*usb_dev_resume_cb_t)(usb_inst_t inst);

/** 连接回调 */
typedef void (*usb_dev_connect_cb_t)(usb_inst_t inst);

/** 断开连接回调 */
typedef void (*usb_dev_disconnect_cb_t)(usb_inst_t inst);

/**
 * @brief Device 模式回调集合
 */
typedef struct {
    usb_dev_reset_cb_t       reset;
    usb_dev_enum_done_cb_t   enum_done;
    usb_dev_setup_stage_cb_t setup_stage;
    usb_dev_data_in_cb_t     data_in;
    usb_dev_data_out_cb_t    data_out;
    usb_dev_sof_cb_t         sof;
    usb_dev_suspend_cb_t     suspend;
    usb_dev_resume_cb_t      resume;
    usb_dev_connect_cb_t     connect;
    usb_dev_disconnect_cb_t  disconnect;
} usb_device_callbacks_t;

//=============================================================================
// 第1层：快速初始化 API
//=============================================================================

/**
 * @brief Device 模式快速初始化
 * @param inst   USB 实例
 * @param config 设备模式配置
 */
void usb_init_device(usb_inst_t inst, const usb_device_config_t *config);

/**
 * @brief 去初始化 USB 控制器
 * @param inst USB 实例
 */
void usb_deinit(usb_inst_t inst);

//=============================================================================
// 第2层：基础配置 API
//=============================================================================

/**
 * @brief 激活端点
 * @param inst   USB 实例
 * @param config 端点配置
 */
void usb_config_ep_activate(usb_inst_t inst, const usb_ep_config_t *config);

/**
 * @brief 停用端点
 * @param inst   USB 实例
 * @param ep_num 端点编号（1~6）
 * @param dir    IN 或 OUT
 */
void usb_config_ep_deactivate(usb_inst_t inst, uint8_t ep_num, usb_ep_dir_t dir);

//=============================================================================
// 第3层：高级功能 API
//=============================================================================

/**
 * @brief 使能端点中断
 * @param inst    USB 实例
 * @param ep_addr 端点地址：bit[3:0]=端点号，bit[7]=1 表示 IN
 */
void usb_ep_int_enable(usb_inst_t inst, uint8_t ep_addr);

/**
 * @brief 禁用端点中断
 * @param inst    USB 实例
 * @param ep_addr 端点地址
 */
void usb_ep_int_disable(usb_inst_t inst, uint8_t ep_addr);

/**
 * @brief 注册 Device 模式回调集合
 * @param inst      USB 实例
 * @param callbacks 回调函数集合
 */
void usb_device_callback_set(usb_inst_t inst, const usb_device_callbacks_t *callbacks);

//=============================================================================
// 第4层：控制与查询 API
//=============================================================================

/**
 * @brief 使能 DP 上拉，软连接
 * @param inst USB 实例
 */
void usb_device_connect(usb_inst_t inst);

/**
 * @brief 禁用 DP 上拉，软断开
 * @param inst USB 实例
 */
void usb_device_disconnect(usb_inst_t inst);

/**
 * @brief 设置设备地址
 * @param inst USB 实例
 * @param addr 设备地址（0~127）
 */
void usb_device_set_address(usb_inst_t inst, uint8_t addr);

/**
 * @brief 激活 EP0 接收 SETUP 事务
 * @param inst USB 实例
 */
void usb_activate_setup(usb_inst_t inst);

/**
 * @brief 准备 EP0 OUT 的 SETUP 接收
 * @param inst USB 实例
 */
void usb_ep0_out_start(usb_inst_t inst);

/**
 * @brief 置位端点 STALL
 * @param inst   USB 实例
 * @param ep_num 端点编号
 * @param dir    端点方向
 */
void usb_ep_stall_set(usb_inst_t inst, uint8_t ep_num, usb_ep_dir_t dir);

/**
 * @brief 清除端点 STALL
 * @param inst   USB 实例
 * @param ep_num 端点编号
 * @param dir    端点方向
 */
void usb_ep_stall_clear(usb_inst_t inst, uint8_t ep_num, usb_ep_dir_t dir);

/**
 * @brief 查询端点 STALL 状态
 * @param inst   USB 实例
 * @param ep_num 端点编号
 * @param dir    端点方向
 * @return true = STALL
 */
bool usb_ep_stall_is(usb_inst_t inst, uint8_t ep_num, usb_ep_dir_t dir);

/**
 * @brief IN 端点发送数据
 * @param inst   USB 实例
 * @param ep_num IN 端点编号（0~6）
 * @param data   数据缓冲区（NULL 表示零长度包）
 * @param len    数据长度（字节）
 */
void usb_ep_write(usb_inst_t inst, uint8_t ep_num,
                  const uint8_t *data, uint32_t len);

/**
 * @brief 准备 OUT 端点接收数据
 * @param inst   USB 实例
 * @param ep_num OUT 端点编号（0~6）
 * @param buf    接收缓冲区
 * @param len    期望接收长度
 */
void usb_ep_read_start(usb_inst_t inst, uint8_t ep_num,
                       uint8_t *buf, uint32_t len);

/**
 * @brief 获取 OUT 端点已接收字节数
 * @param inst   USB 实例
 * @param ep_num OUT 端点编号（0~6）
 * @return 本次实际收到的字节数
 */
uint32_t usb_ep_get_rx_count(usb_inst_t inst, uint8_t ep_num);

/**
 * @brief 获取当前帧编号
 * @param inst USB 实例
 * @return 11 位帧编号
 */
uint32_t usb_get_frame_number(usb_inst_t inst);

/**
 * @brief USB 中断统一处理入口
 * @param inst USB 实例
 */
void usb_irq_handler(usb_inst_t inst);

#ifdef __cplusplus
}
#endif

#endif /* _HARDWARE_USB_H */
