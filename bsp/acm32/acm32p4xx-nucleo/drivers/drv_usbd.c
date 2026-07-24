/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   ACM32P4xx USB Device driver skeleton
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"
#include "hal_fsusb.h"

/*
 * WARNING: This is a skeleton driver with all ops as stubs (return RT_EOK).
 * Do NOT enable BSP_USING_FSUSB in production until the implementation is complete.
 * The driver registers as RT_Device_Class_USBDevice but provides no actual USB functionality.
 * TODO: Implement all udcd_ops and FSUSB_IRQHandler before using this driver.
 */

#if defined(RT_USING_USB_DEVICE) && defined(BSP_USING_FSUSB)

#define DBG_TAG "drv.usbd"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define USBD_NAME "usbd"

/* ==================== 端点池 ==================== */

/*
 * TODO: 当驱动实现完成后，将 endpoint_pool 改为静态分配:
 * static struct ep_id endpoint_pool[] = { ... };
 * 当前使用 rt_malloc 是因为驱动是骨架，未来会重构。
 */

static struct ep_id endpoint_pool[] =
{
    {0x0,  USB_EP_ATTR_CONTROL,  USB_DIR_INOUT, 64, ID_ASSIGNED  },
    {0x1,  USB_EP_ATTR_BULK,     USB_DIR_IN,    64, ID_UNASSIGNED},
    {0x1,  USB_EP_ATTR_BULK,     USB_DIR_OUT,   64, ID_UNASSIGNED},
    {0x2,  USB_EP_ATTR_INT,      USB_DIR_IN,    64, ID_UNASSIGNED},
    {0x2,  USB_EP_ATTR_INT,      USB_DIR_OUT,   64, ID_UNASSIGNED},
    {0xFF, USB_EP_ATTR_TYPE_MASK, USB_DIR_MASK,  0,  ID_ASSIGNED  },
};

/* ==================== udcd_ops 占位实现 ==================== */

static rt_err_t _set_address(rt_uint8_t address)
{
    /* TODO: 通过 USBCTRL->USB_ADDR 设置设备地址 */
    return RT_EOK;
}

static rt_err_t _set_config(rt_uint8_t address)
{
    /* TODO: 配置 USB 设备 */
    return RT_EOK;
}

static rt_err_t _ep_set_stall(rt_uint8_t address)
{
    /* TODO: usb_send_stall(ep_idx, dir) */
    return RT_EOK;
}

static rt_err_t _ep_clear_stall(rt_uint8_t address)
{
    /* TODO: usb_clear_stall(ep_idx, dir) */
    return RT_EOK;
}

static rt_err_t _ep_enable(uep_t ep)
{
    RT_ASSERT(ep != RT_NULL);
    RT_ASSERT(ep->ep_desc != RT_NULL);

    /* TODO: HAL_FSUSB 端点使能 */
    return RT_EOK;
}

static rt_err_t _ep_disable(uep_t ep)
{
    RT_ASSERT(ep != RT_NULL);
    RT_ASSERT(ep->ep_desc != RT_NULL);

    /* TODO: HAL_FSUSB 端点禁用 */
    return RT_EOK;
}

static rt_ssize_t _ep_read_prepare(rt_uint8_t address, void *buffer, rt_size_t size)
{
    /* TODO: HAL_FSUSB_Receive_Data 准备接收 */
    return size;
}

static rt_ssize_t _ep_read(rt_uint8_t address, void *buffer)
{
    /* TODO: 从 FIFO 读取数据 */
    return 0;
}

static rt_ssize_t _ep_write(rt_uint8_t address, void *buffer, rt_size_t size)
{
    /* TODO: HAL_FSUSB_Send_Data 发送数据 */
    return size;
}

static rt_err_t _ep0_send_status(void)
{
    /* TODO: HAL_FSUSB_EP0_Send_Empty_Packet */
    return RT_EOK;
}

static rt_err_t _suspend(void)
{
    /* TODO: HAL_FSUSB_Suspend */
    return RT_EOK;
}

static rt_err_t _wakeup(void)
{
    /* TODO: HAL_FSUSB_Resume */
    return RT_EOK;
}

static const struct udcd_ops usbd_ops =
{
    _set_address,
    _set_config,
    _ep_set_stall,
    _ep_clear_stall,
    _ep_enable,
    _ep_disable,
    _ep_read_prepare,
    _ep_read,
    _ep_write,
    _ep0_send_status,
    _suspend,
    _wakeup,
};

/* ==================== USB 中断处理 ==================== */

/**
  * @brief  FSUSB 中断处理函数
  * @note   覆盖 startup 中的弱定义 Default_Handler
  *         TODO: 实现完整的中断分发逻辑，参考 hal_fsusb.c 中 HAL_FSUSB_Init 使能的中断源
  */
void FSUSB_IRQHandler(void)
{
    rt_interrupt_enter();

    /* TODO: 实现 USB 全局中断处理
     * 1. 读取 USBINT->INT_STAT_RAW 获取全局中断状态
     * 2. 复位 (USB_BUS_RESET)    → rt_usbd_reset_handler(udcd)
     * 3. 挂起 (USB_SUSPEND)      → rt_usbd_disconnect_handler(udcd)
     * 4. 唤醒 (USB_RESUME)       → rt_usbd_connect_handler(udcd)
     * 5. SOF  (USB_SOF)          → rt_usbd_sof_handler(udcd)
     * 6. SETUP (USB_SETUPTOK/EP0_SETUP_PACKET) → 解析DEVICE_REQUEST
     *    → rt_usbd_ep0_setup_handler(udcd, &req)
     * 7. 读取 USBINT->EP_INT_STAT_RAW 获取端点中断状态
     * 8. EP0 IN ACK  → rt_usbd_ep0_in_handler(udcd)
     * 9. EP0 OUT     → rt_usbd_ep0_out_handler(udcd, len)
     * 10. EPx IN     → rt_usbd_ep_in_handler(udcd, 0x80|ep, len)
     * 11. EPx OUT    → rt_usbd_ep_out_handler(udcd, ep, len)
     * 12. 清除相应的中断标志 HAL_FSUSB_Clear_Global_Interrupt / HAL_FSUSB_Clear_EP_Interrupt
     *
     * 注册的 udcd 指针可通过 static 变量保存后在 ISR 中使用。
     */

    rt_interrupt_leave();
}

/* ==================== MSP 初始化 ==================== */

/**
  * @brief  FSUSB MSP 初始化：时钟 + GPIO (PA11=DM, PA12=DP) + NVIC
  */
static void fsusb_msp_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    /* 使能 USB 时钟 */
    __HAL_RCC_USB1_CLK_ENABLE();

    /* 使能 GPIOA 时钟 */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA11=USB_DM, PA12=USB_DP (AF10, 无上下拉, DRIVE_LEVEL4) */
    gpio_init.Pin       = GPIO_PIN_11 | GPIO_PIN_12;
    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_NOPULL;
    gpio_init.Drive     = GPIO_DRIVE_LEVEL4;
    gpio_init.Alternate = GPIO_FUNCTION_10;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    /* 配置 NVIC，优先级 2 */
    NVIC_SetPriority(FSUSB_IRQn, 2);
    NVIC_EnableIRQ(FSUSB_IRQn);
}

/* ==================== 设备注册 ==================== */

static udcd_t _udcd = RT_NULL;

/**
  * @brief  ACM32P4xx USB Device 控制器注册
  */
int acm32_usbd_register(void)
{
    rt_err_t result;

    _udcd = (udcd_t)rt_malloc(sizeof(struct udcd));
    if (_udcd == RT_NULL)
    {
        LOG_E("udcd malloc failed");
        return -RT_ERROR;
    }
    rt_memset((void *)_udcd, 0, sizeof(struct udcd));

    _udcd->parent.type = RT_Device_Class_USBDevice;
    _udcd->ops = &usbd_ops;
    _udcd->ep_pool = endpoint_pool;
    _udcd->ep0.id = &endpoint_pool[0];

    result = rt_device_register((rt_device_t)_udcd, USBD_NAME, 0);
    if (result != RT_EOK)
    {
        LOG_E("usbd register failed (%d)", result);
        rt_free(_udcd);
        _udcd = RT_NULL;
        return result;
    }

    /* MSP 初始化: 时钟 + GPIO + NVIC */
    fsusb_msp_init();

    /* 初始化 FSUSB 控制器 (PHY, EP, 中断使能, connect) */
    (void)HAL_FSUSB_Init();

    /* 初始化 RT-Thread USB Device 框架 */
    result = rt_usb_device_init();
    if (result != RT_EOK)
    {
        LOG_E("usb device init failed (%d)", result);
        rt_device_unregister((rt_device_t)_udcd);
        rt_free(_udcd);
        _udcd = RT_NULL;
        return result;
    }

    LOG_I("ACM32P4xx USB Device driver registered (%s)", USBD_NAME);
    return RT_EOK;
}
INIT_DEVICE_EXPORT(acm32_usbd_register);

#endif /* RT_USING_USB_DEVICE && BSP_USING_FSUSB */
