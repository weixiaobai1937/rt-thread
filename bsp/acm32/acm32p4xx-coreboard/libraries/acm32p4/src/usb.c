/**
 * @file usb.c
 * @brief ACM32P4 USB 设备控制器驱动实现
 *
 * 实现了 ACM32P4 芯片 USB 设备控制器的全部功能：
 * - 端点初始化与配置
 * - FIFO / Memory 两种数据收发模式
 * - 中断处理与回调分发
 * - 控制传输（SETUP/DATA/STATUS）
 */

#include "hardware/usb.h"
#include "hardware/clocks.h"
#include "device/acm32p4xx.h"
#include <string.h>

//===========================================
// 内部宏定义
//===========================================

/** 指向 USB 寄存器基址 */
#define USBx    ((USB_TypeDef *)USB_BASE)

/** 端点 CSR 寄存器数组（EP0~EP6），步长 1 个 uint32 */
#define EP_CSR(ep)  (*((volatile uint32_t *)((uint32_t)USBx + 0x40U + (uint32_t)(ep) * 4U)))

/** 端点 SENDBN 寄存器数组（EP0~EP6），步长 1 个 uint32 */
#define EP_SENDBN(ep) (*((volatile uint32_t *)((uint32_t)USBx + 0x80U + (uint32_t)(ep) * 4U)))

/** 端点 FIFO 寄存器数组（EP0~EP6），步长 1 个 uint32 */
#define EP_FIFO(ep)   (*((volatile uint32_t *)((uint32_t)USBx + 0x100U + (uint32_t)(ep) * 4U)))

/** EP0 FIFO 指针（SETUP 数据） */
#define EP0_FIFO    (USBx->EP0FIFO)

//===========================================
// EPxCSR 位域掩码（EP0~EP6 通用）
//===========================================

#define EP_CSR_RECEIVED_BYTE_MASK   0xFFU         ///< [7:0] 接收字节数
#define EP_CSR_EN_BIT               (1U << 8)     ///< [8]   端点使能
#define EP_CSR_FIFOCLR_BIT          (1U << 9)     ///< [9]   FIFO 清除
#define EP_CSR_DATA_START_BIT       (1U << 10)    ///< [10]  发送启动
#define EP_CSR_RECEIVED_DONE_BIT    (1U << 11)    ///< [11]  接收完成
#define EP_CSR_SEND_STALL_BIT       (1U << 12)    ///< [12]  发送 STALL
#define EP_CSR_STALL_DONE_BIT       (1U << 13)    ///< [13]  STALL 发送完成
#define EP_CSR_IN_TOGGLE_VAL_BIT    (1U << 14)    ///< [14]  IN Toggle 值
#define EP_CSR_IN_TOGGLE_CTRL_BIT   (1U << 15)    ///< [15]  IN Toggle 控制使能
#define EP_CSR_OUT_TOGGLE_VAL_BIT   (1U << 16)    ///< [16]  OUT Toggle 收到的值
#define EP_CSR_OUT_TOGGLE_WANT_BIT  (1U << 17)    ///< [17]  OUT Toggle 期望值
#define EP_CSR_OUT_TOGGLE_CTRL_BIT  (1U << 18)    ///< [18]  OUT Toggle 控制使能
#define EP_CSR_OUT_TOGGLE_STATE_BIT (1U << 19)    ///< [19]  OUT Toggle 错误标志
#define EP_CSR_OUT_VALID_BIT        (1U << 20)    ///< [20]  OUT 数据有效
#define EP_CSR_ISOCHRONOUS_BIT      (1U << 21)    ///< [21]  同步传输使能（EP1~EP6）
#define EP_CSR_RECEIVED_ACK_BIT     (1U << 24)    ///< [24]  收到 ACK
#define EP_CSR_RECEIVED_NAK_BIT     (1U << 25)    ///< [25]  收到 NAK
#define EP_CSR_RECEIVED_STALL_BIT   (1U << 26)    ///< [26]  收到 STALL
#define EP_CSR_SEND_HALT_BIT        (1U << 27)    ///< [27]  发送暂停

//===========================================
// 内部状态变量
//===========================================

/** 回调函数表 */
static usb_device_callbacks_t s_dev_cbs[USB_INSTANCE_COUNT];

/** OUT 端点接收缓冲区指针 */
static uint8_t *s_ep_rx_buf[USB_INSTANCE_COUNT][USB_MAX_EP_COUNT];

/** OUT 端点期望接收长度 */
static uint32_t s_ep_rx_len[USB_INSTANCE_COUNT][USB_MAX_EP_COUNT];

/** OUT 端点已接收长度 */
static uint32_t s_ep_rx_count[USB_INSTANCE_COUNT][USB_MAX_EP_COUNT];

/** IN 端点发送缓冲区指针 */
static const uint8_t *s_ep_tx_buf[USB_INSTANCE_COUNT][USB_MAX_EP_COUNT];

/** IN 端点计划发送长度 */
static uint32_t s_ep_tx_len[USB_INSTANCE_COUNT][USB_MAX_EP_COUNT];

/** IN 端点已发送长度 */
static uint32_t s_ep_tx_count[USB_INSTANCE_COUNT][USB_MAX_EP_COUNT];

/** 端点最大包长 */
static uint16_t s_ep_mps[USB_INSTANCE_COUNT][USB_MAX_EP_COUNT];

/** 端点类型 */
static uint8_t s_ep_type[USB_INSTANCE_COUNT][USB_MAX_EP_COUNT];

//===========================================
// 内部辅助函数
//===========================================

/**
 * @brief 向端点 FIFO 写入数据（32bit 对齐）
 */
static void usb_write_fifo(uint8_t ep_num, const uint8_t *src, uint32_t len)
{
    volatile uint32_t *fifo = &EP_FIFO(ep_num);
    uint32_t word_cnt = (len + 3U) / 4U;

    for (uint32_t i = 0U; i < word_cnt; i++) {
        uint32_t word = 0U;
        uint32_t bytes = (len - i * 4U) >= 4U ? 4U : (len - i * 4U);
        for (uint32_t b = 0U; b < bytes; b++) {
            word |= (uint32_t)src[i * 4U + b] << (b * 8U);
        }
        *fifo = word;
    }
}

/**
 * @brief 从端点 FIFO 读取数据（32bit 对齐）
 */
static void usb_read_fifo(uint8_t ep_num, uint8_t *dest, uint32_t len)
{
    volatile uint32_t *fifo = &EP_FIFO(ep_num);
    uint32_t word_cnt = (len + 3U) / 4U;

    for (uint32_t i = 0U; i < word_cnt; i++) {
        uint32_t word = *fifo;
        uint32_t bytes = (len - i * 4U) >= 4U ? 4U : (len - i * 4U);
        if (dest != NULL) {
            for (uint32_t b = 0U; b < bytes; b++) {
                dest[i * 4U + b] = (uint8_t)(word >> (b * 8U));
            }
        }
    }
}

/**
 * @brief IN 端点继续填充 TxFIFO（长数据分包发送）
 */
static void usb_ep_fill_tx_fifo(usb_inst_t inst, uint8_t ep_num)
{
    uint32_t idx = (uint32_t)inst;
    uint16_t mps = s_ep_mps[idx][ep_num];
    uint32_t remaining = s_ep_tx_len[idx][ep_num] - s_ep_tx_count[idx][ep_num];

    if (remaining == 0U) {
        return;
    }

    uint32_t chunk = (remaining > (uint32_t)mps) ? (uint32_t)mps : remaining;
    const uint8_t *ptr = &s_ep_tx_buf[idx][ep_num][s_ep_tx_count[idx][ep_num]];

    usb_write_fifo(ep_num, ptr, chunk);
    EP_SENDBN(ep_num) = chunk;
    EP_CSR(ep_num) |= EP_CSR_DATA_START_BIT;

    s_ep_tx_count[idx][ep_num] += chunk;
}

//===========================================
// 第1层：快速初始化 API
//===========================================

void usb_init_device(usb_inst_t inst, const usb_device_config_t *config)
{
    (void)inst;

    /* 使能 USB 外设时钟 */
    clock_periph_enable(CLK_USB1);

    /* 工作模式：全速、总线自动复位 */
    uint32_t wm = 0U;
    wm |= (1U << 0);   /* SPEED_MODE = 1: 全速 */
    wm |= (1U << 3);   /* USB_BUS_AUTO_RST_EN */
    USBx->WORKING_MODE = wm;

    /* 软复位控制器 */
    USBx->WORKING_MODE_f.USB_FORCE_RST = 1U;

    /* 清除所有中断状态（写1清零） */
    USBx->INT_ISR    = 0xFFFFFFFFU;
    USBx->EPINT_ISR  = 0xFFFFFFFFU;
    USBx->EPINT_ISR2 = 0xFFFFFFFFU;

    /* 禁用所有中断 */
    USBx->INT_EN    = 0U;
    USBx->EPINT_EN  = 0U;
    USBx->EPINT_EN2 = 0U;

    /* 清除端点状态 */
    for (uint32_t ep = 0U; ep < USB_MAX_EP_COUNT; ep++) {
        EP_CSR(ep) = 0U;
        EP_SENDBN(ep) = 0U;
    }

    /* 清除内部状态 */
    uint32_t idx = (uint32_t)inst;
    memset(&s_dev_cbs[idx], 0, sizeof(usb_device_callbacks_t));
    memset(s_ep_rx_buf[idx], 0, sizeof(s_ep_rx_buf[idx]));
    memset(s_ep_rx_len[idx], 0, sizeof(s_ep_rx_len[idx]));
    memset(s_ep_rx_count[idx], 0, sizeof(s_ep_rx_count[idx]));
    memset(s_ep_tx_buf[idx], 0, sizeof(s_ep_tx_buf[idx]));
    memset(s_ep_tx_len[idx], 0, sizeof(s_ep_tx_len[idx]));
    memset(s_ep_tx_count[idx], 0, sizeof(s_ep_tx_count[idx]));
    memset(s_ep_mps[idx], 0, sizeof(s_ep_mps[idx]));
    memset(s_ep_type[idx], 0, sizeof(s_ep_type[idx]));

    /* EP0 默认最大包长 64 */
    s_ep_mps[idx][0] = USB_MAX_PACKET_SIZE;
    s_ep_type[idx][0] = (uint8_t)USB_EP_TYPE_CTRL;
}

void usb_deinit(usb_inst_t inst)
{
    (void)inst;

    /* 禁用所有中断 */
    USBx->INT_EN    = 0U;
    USBx->EPINT_EN  = 0U;
    USBx->EPINT_EN2 = 0U;

    /* 清除中断状态 */
    USBx->INT_ISR    = 0xFFFFFFFFU;
    USBx->EPINT_ISR  = 0xFFFFFFFFU;
    USBx->EPINT_ISR2 = 0xFFFFFFFFU;

    /* 软断开 */
    usb_device_disconnect(inst);

    /* 禁用 USB 时钟 */
    clock_periph_disable(CLK_USB1);

    /* 清除内部状态 */
    uint32_t idx = (uint32_t)inst;
    memset(&s_dev_cbs[idx], 0, sizeof(usb_device_callbacks_t));
    memset(s_ep_rx_buf[idx], 0, sizeof(s_ep_rx_buf[idx]));
    memset(s_ep_rx_len[idx], 0, sizeof(s_ep_rx_len[idx]));
    memset(s_ep_rx_count[idx], 0, sizeof(s_ep_rx_count[idx]));
    memset(s_ep_tx_buf[idx], 0, sizeof(s_ep_tx_buf[idx]));
    memset(s_ep_tx_len[idx], 0, sizeof(s_ep_tx_len[idx]));
    memset(s_ep_tx_count[idx], 0, sizeof(s_ep_tx_count[idx]));
    memset(s_ep_mps[idx], 0, sizeof(s_ep_mps[idx]));
    memset(s_ep_type[idx], 0, sizeof(s_ep_type[idx]));
}

//===========================================
// 第2层：基础配置 API
//===========================================

void usb_config_ep_activate(usb_inst_t inst, const usb_ep_config_t *config)
{
    uint32_t idx = (uint32_t)inst;
    uint8_t epn = config->ep_num;

    s_ep_mps[idx][epn]  = config->max_packet_size;
    s_ep_type[idx][epn] = (uint8_t)config->type;

    /* 使能端点 */
    EP_CSR(epn) |= EP_CSR_EN_BIT;

    /* 同步传输使能 */
    if (config->type == USB_EP_TYPE_ISOC) {
        EP_CSR(epn) |= EP_CSR_ISOCHRONOUS_BIT;
    }

    /* 配置端点地址 */
    if (epn >= 1U && epn <= 6U) {
        uint32_t shift = (uint32_t)(epn - 1U) * 4U;
        USBx->EP_ADDR = (USBx->EP_ADDR & ~(0xFU << shift))
                       | ((uint32_t)epn << shift);
    }
}

void usb_config_ep_deactivate(usb_inst_t inst, uint8_t ep_num, usb_ep_dir_t dir)
{
    uint32_t idx = (uint32_t)inst;

    /* 禁用端点 */
    EP_CSR(ep_num) &= ~EP_CSR_EN_BIT;

    /* 禁用端点中断 */
    uint32_t bit_pos = (uint32_t)ep_num * 4U;
    if (dir == USB_EP_DIR_IN) {
        USBx->EPINT_EN &= ~(1U << bit_pos);
    } else {
        USBx->EPINT_EN &= ~(1U << (bit_pos + 1U));
    }

    /* 清除内部状态 */
    s_ep_rx_buf[idx][ep_num]    = NULL;
    s_ep_rx_len[idx][ep_num]    = 0U;
    s_ep_rx_count[idx][ep_num]  = 0U;
    s_ep_tx_buf[idx][ep_num]    = NULL;
    s_ep_tx_len[idx][ep_num]    = 0U;
    s_ep_tx_count[idx][ep_num]  = 0U;
    s_ep_mps[idx][ep_num]       = 0U;
    s_ep_type[idx][ep_num]      = 0U;
}

//===========================================
// 第3层：高级功能 API
//===========================================

void usb_ep_int_enable(usb_inst_t inst, uint8_t ep_addr)
{
    (void)inst;
    uint8_t ep_num = ep_addr & 0x0FU;
    uint32_t bit_pos = (uint32_t)ep_num * 4U;

    if ((ep_addr & 0x80U) != 0U) {
        /* IN 端点：使能 IN + ACK + HANDSHAKE_ERR */
        USBx->EPINT_EN |= (1U << bit_pos);           // EPx_IN_EN
        USBx->EPINT_EN |= (1U << (bit_pos + 2U));    // EPx_ACK_EN
        USBx->EPINT_EN |= (1U << (bit_pos + 3U));    // EPx_IN_HANDSHAKE_ERR_EN
    } else {
        /* OUT 端点：使能 OUT + ACK */
        USBx->EPINT_EN |= (1U << (bit_pos + 1U));    // EPx_OUT_EN
        USBx->EPINT_EN |= (1U << (bit_pos + 2U));    // EPx_ACK_EN
    }
}

void usb_ep_int_disable(usb_inst_t inst, uint8_t ep_addr)
{
    (void)inst;
    uint8_t ep_num = ep_addr & 0x0FU;
    uint32_t bit_pos = (uint32_t)ep_num * 4U;

    if ((ep_addr & 0x80U) != 0U) {
        /* IN 端点：禁用 IN + ACK + HANDSHAKE_ERR */
        USBx->EPINT_EN &= ~(1U << bit_pos);           // EPx_IN_EN
        USBx->EPINT_EN &= ~(1U << (bit_pos + 2U));    // EPx_ACK_EN
        USBx->EPINT_EN &= ~(1U << (bit_pos + 3U));    // EPx_IN_HANDSHAKE_ERR_EN
    } else {
        /* OUT 端点：禁用 OUT + ACK */
        USBx->EPINT_EN &= ~(1U << (bit_pos + 1U));    // EPx_OUT_EN
        USBx->EPINT_EN &= ~(1U << (bit_pos + 2U));    // EPx_ACK_EN
    }
}

void usb_device_callback_set(usb_inst_t inst, const usb_device_callbacks_t *callbacks)
{
    uint32_t idx = (uint32_t)inst;
    s_dev_cbs[idx] = *callbacks;
}

//===========================================
// 第4层：控制与查询 API
//===========================================

void usb_device_connect(usb_inst_t inst)
{
    (void)inst;
    USBx->WORKING_MODE_f.USB_DPPU    = 1U;
    USBx->WORKING_MODE_f.USB_DPPU_LO = 1U;
}

void usb_device_disconnect(usb_inst_t inst)
{
    (void)inst;
    USBx->WORKING_MODE_f.USB_DPPU    = 0U;
    USBx->WORKING_MODE_f.USB_DPPU_LO = 0U;
}

void usb_device_set_address(usb_inst_t inst, uint8_t addr)
{
    (void)inst;
    USBx->USB_ADDR_f.USB_ADDR = addr & 0x7FU;
}

void usb_activate_setup(usb_inst_t inst)
{
    (void)inst;
    /* 使能 EP0 */
    EP_CSR(0U) |= EP_CSR_EN_BIT;
    /* 清除 FIFO 并通知接收完成，准备接收 SETUP */
    EP_CSR(0U) |= EP_CSR_FIFOCLR_BIT;
    EP_CSR(0U) |= EP_CSR_RECEIVED_DONE_BIT;
}

void usb_ep0_out_start(usb_inst_t inst)
{
    (void)inst;
    /* 清除 EP0 FIFO */
    EP_CSR(0U) |= EP_CSR_FIFOCLR_BIT;
    /* 通知接收完成，准备接收 SETUP */
    EP_CSR(0U) |= EP_CSR_RECEIVED_DONE_BIT;
}

void usb_ep_stall_set(usb_inst_t inst, uint8_t ep_num, usb_ep_dir_t dir)
{
    (void)inst;
    (void)dir;
    EP_CSR(ep_num) |= EP_CSR_SEND_STALL_BIT;
}

void usb_ep_stall_clear(usb_inst_t inst, uint8_t ep_num, usb_ep_dir_t dir)
{
    (void)inst;
    (void)dir;
    if (ep_num == 0U) {
        /* EP0：通过 DATA_START 清除 STALL（手册 30.5.12） */
        EP_CSR(0U) |= EP_CSR_DATA_START_BIT;
    } else {
        /* EP1~EP6：直接清除 SEND_STALL 位（手册 30.5.13-18） */
        EP_CSR(ep_num) &= ~EP_CSR_SEND_STALL_BIT;
    }
}

bool usb_ep_stall_is(usb_inst_t inst, uint8_t ep_num, usb_ep_dir_t dir)
{
    (void)inst;
    (void)dir;
    return (EP_CSR(ep_num) & EP_CSR_SEND_STALL_BIT) != 0U;
}

void usb_ep_write(usb_inst_t inst, uint8_t ep_num,
                  const uint8_t *data, uint32_t len)
{
    uint32_t idx = (uint32_t)inst;
    uint16_t mps = s_ep_mps[idx][ep_num];

    if (mps == 0U) {
        mps = USB_MAX_PACKET_SIZE;
    }

    /* 清除 FIFO */
    EP_CSR(ep_num) |= EP_CSR_FIFOCLR_BIT;

    if (len == 0U || data == NULL) {
        /* 零长度包 */
        EP_SENDBN(ep_num) = 0U;
        EP_CSR(ep_num) |= EP_CSR_DATA_START_BIT;
        return;
    }

    /* 计算首包长度 */
    uint32_t first = (len > (uint32_t)mps) ? (uint32_t)mps : len;

    usb_write_fifo(ep_num, data, first);
    EP_SENDBN(ep_num) = first;
    EP_CSR(ep_num) |= EP_CSR_DATA_START_BIT;

    /* 如果数据超过一个包，保存状态等待 IN 中断继续发送 */
    if (len > first) {
        s_ep_tx_buf[idx][ep_num]    = data;
        s_ep_tx_len[idx][ep_num]    = len;
        s_ep_tx_count[idx][ep_num]  = first;
    } else {
        s_ep_tx_buf[idx][ep_num]    = NULL;
        s_ep_tx_len[idx][ep_num]    = 0U;
        s_ep_tx_count[idx][ep_num]  = 0U;
    }
}

void usb_ep_read_start(usb_inst_t inst, uint8_t ep_num,
                       uint8_t *buf, uint32_t len)
{
    uint32_t idx = (uint32_t)inst;

    s_ep_rx_buf[idx][ep_num]   = buf;
    s_ep_rx_len[idx][ep_num]   = len;
    s_ep_rx_count[idx][ep_num] = 0U;

    /* 通知接收完成，准备接收下一笔数据 */
    EP_CSR(ep_num) |= EP_CSR_RECEIVED_DONE_BIT;
}

uint32_t usb_ep_get_rx_count(usb_inst_t inst, uint8_t ep_num)
{
    uint32_t idx = (uint32_t)inst;
    return s_ep_rx_count[idx][ep_num];
}

uint32_t usb_get_frame_number(usb_inst_t inst)
{
    (void)inst;
    return (uint32_t)USBx->CURRENT_FRAME_NUMBER_f.CURRENT_FRAME_NUMBER;
}

//===========================================
// 中断处理函数
//===========================================

/**
 * @brief 处理全局中断
 */
static void usb_handle_global_irq(usb_inst_t inst)
{
    uint32_t isr = USBx->INT_ISR;

    /* 总线复位 */
    if ((isr & (1U << 0)) != 0U) {
        USBx->INT_ISR = (1U << 0);

        /* 复位内部状态 */
        uint32_t idx = (uint32_t)inst;
        for (uint32_t ep = 0U; ep < USB_MAX_EP_COUNT; ep++) {
            s_ep_rx_buf[idx][ep]    = NULL;
            s_ep_rx_len[idx][ep]    = 0U;
            s_ep_rx_count[idx][ep]  = 0U;
            s_ep_tx_buf[idx][ep]    = NULL;
            s_ep_tx_len[idx][ep]    = 0U;
            s_ep_tx_count[idx][ep]  = 0U;
        }

        if (s_dev_cbs[(uint32_t)inst].reset != NULL) {
            s_dev_cbs[(uint32_t)inst].reset(inst);
        }
    }

    /* 挂起 */
    if ((isr & (1U << 1)) != 0U) {
        USBx->INT_ISR = (1U << 1);
        if (s_dev_cbs[(uint32_t)inst].suspend != NULL) {
            s_dev_cbs[(uint32_t)inst].suspend(inst);
        }
    }

    /* 恢复 */
    if ((isr & (1U << 2)) != 0U) {
        USBx->INT_ISR = (1U << 2);
        if (s_dev_cbs[(uint32_t)inst].resume != NULL) {
            s_dev_cbs[(uint32_t)inst].resume(inst);
        }
    }

    /* SOF */
    if ((isr & (1U << 3)) != 0U) {
        USBx->INT_ISR = (1U << 3);
        if (s_dev_cbs[(uint32_t)inst].sof != NULL) {
            s_dev_cbs[(uint32_t)inst].sof(inst);
        }
    }

    /* SET_ADDRESS 完成 */
    if ((isr & (1U << 22)) != 0U) {
        USBx->INT_ISR = (1U << 22);
        if (s_dev_cbs[(uint32_t)inst].enum_done != NULL) {
            s_dev_cbs[(uint32_t)inst].enum_done(inst, USB_SPEED_FULL);
        }
    }
}

/**
 * @brief 处理端点中断
 */
static void usb_handle_ep_irq(usb_inst_t inst)
{
    uint32_t ep_isr  = USBx->EPINT_ISR;
    uint32_t ep_isr2 = USBx->EPINT_ISR2;
    uint32_t idx = (uint32_t)inst;

    for (uint32_t ep = 0U; ep < USB_MAX_EP_COUNT; ep++) {
        uint32_t base = ep * 4U;

        /* EPx IN 中断（IN 令牌包到达） */
        if ((ep_isr & (1U << base)) != 0U) {
            USBx->EPINT_ISR = (1U << base);

            /* 继续发送剩余数据 */
            if (s_ep_tx_buf[idx][ep] != NULL) {
                usb_ep_fill_tx_fifo(inst, (uint8_t)ep);
            }

            /* EP0 SETUP 阶段 */
            if (ep == 0U && s_dev_cbs[idx].setup_stage != NULL) {
                uint8_t setup[8];
                uint32_t d0 = USBx->SETUP_0_3_DATA;
                uint32_t d1 = USBx->SETUP_4_7_DATA;
                setup[0] = (uint8_t)(d0 & 0xFFU);
                setup[1] = (uint8_t)((d0 >> 8) & 0xFFU);
                setup[2] = (uint8_t)((d0 >> 16) & 0xFFU);
                setup[3] = (uint8_t)((d0 >> 24) & 0xFFU);
                setup[4] = (uint8_t)(d1 & 0xFFU);
                setup[5] = (uint8_t)((d1 >> 8) & 0xFFU);
                setup[6] = (uint8_t)((d1 >> 16) & 0xFFU);
                setup[7] = (uint8_t)((d1 >> 24) & 0xFFU);
                s_dev_cbs[idx].setup_stage(inst, setup);
            } else if (ep != 0U && s_dev_cbs[idx].data_in != NULL) {
                s_dev_cbs[idx].data_in(inst, (uint8_t)ep);
            }
        }

        /* EPx OUT 中断（收到数据） */
        if ((ep_isr & (1U << (base + 1U))) != 0U) {
            USBx->EPINT_ISR = (1U << (base + 1U));

            /* 读取接收字节数 */
            uint32_t rx_byte = EP_CSR(ep) & EP_CSR_RECEIVED_BYTE_MASK;

            if (ep == 0U) {
                /* EP0 OUT：SETUP 数据已在上面的 IN 处理中读取 */
                /* 或 Data Phase 的 OUT 数据 */
                if (s_ep_rx_buf[idx][0] != NULL && rx_byte > 0U) {
                    usb_read_fifo(0U, s_ep_rx_buf[idx][0], rx_byte);
                    s_ep_rx_count[idx][0] = rx_byte;
                }
                if (s_dev_cbs[idx].data_out != NULL) {
                    s_dev_cbs[idx].data_out(inst, 0U);
                }
            } else {
                /* 通用 EP OUT */
                if (s_ep_rx_buf[idx][ep] != NULL && rx_byte > 0U) {
                    uint32_t to_copy = rx_byte;
                    if (to_copy > s_ep_rx_len[idx][ep]) {
                        to_copy = s_ep_rx_len[idx][ep];
                    }
                    usb_read_fifo((uint8_t)ep, s_ep_rx_buf[idx][ep], to_copy);
                    s_ep_rx_count[idx][ep] = to_copy;
                }

                if (s_dev_cbs[idx].data_out != NULL) {
                    s_dev_cbs[idx].data_out(inst, (uint8_t)ep);
                }
            }
        }

        /* EPx ACK 中断 */
        if ((ep_isr & (1U << (base + 2U))) != 0U) {
            USBx->EPINT_ISR = (1U << (base + 2U));
        }

        /* EPx IN HANDSHAKE ERROR 中断 */
        if ((ep_isr & (1U << (base + 3U))) != 0U) {
            USBx->EPINT_ISR = (1U << (base + 3U));
        }
    }

    /* EPx IN DATA 中断（同步传输完成） */
    for (uint32_t ep = 1U; ep <= 6U; ep++) {
        if ((ep_isr2 & (1U << ep)) != 0U) {
            USBx->EPINT_ISR2 = (1U << ep);
        }
    }
}

void usb_irq_handler(usb_inst_t inst)
{
    usb_handle_global_irq(inst);
    usb_handle_ep_irq(inst);
}
