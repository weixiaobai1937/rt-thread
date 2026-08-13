/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-28     AisinoChip   FSUSB (USB FS device) CDC VCOM driver
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"
#include "hal_fsusb.h"

#ifdef BSP_USING_FSUSB
#include <drivers/usb_device.h>

#define DBG_TAG "drv.fsusb"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/* ep_pool: EP0 + EP1-6 IN/OUT, 64B max packet */
static struct ep_id _ep_pool[] =
{
    {0x00, USB_EP_ATTR_CONTROL, USB_DIR_OUT, 64, 0},
    {0x81, USB_EP_ATTR_INT, USB_DIR_IN, 64, 0},
    {0x01, USB_EP_ATTR_BULK, USB_DIR_OUT, 64, 0},
    {0x82, USB_EP_ATTR_BULK, USB_DIR_IN, 64, 0},
    {0x02, USB_EP_ATTR_BULK, USB_DIR_OUT, 64, 0},
    {0x83, USB_EP_ATTR_BULK, USB_DIR_IN, 64, 0},
    {0x03, USB_EP_ATTR_INT, USB_DIR_OUT, 64, 0},
    {0x84, USB_EP_ATTR_INT, USB_DIR_IN, 64, 0},
    {0x04, USB_EP_ATTR_BULK, USB_DIR_OUT, 64, 0},
    {0x85, USB_EP_ATTR_BULK, USB_DIR_IN, 64, 0},
    {0x05, USB_EP_ATTR_INT, USB_DIR_OUT, 64, 0},
    {0x86, USB_EP_ATTR_INT, USB_DIR_IN, 64, 0},
    {0x06, USB_EP_ATTR_BULK, USB_DIR_OUT, 64, 0},
};

struct acm32_udc
{
    struct udcd parent;
    rt_uint8_t ep0_buf[64];
    rt_uint8_t *rx_buf[7];
    rt_uint16_t rx_len[7];
    rt_uint16_t last_tx_size[7];
};
static struct acm32_udc _udc;

/* ---- udcd ops: filled in Task 3; compile-ready stubs now ---- */
static rt_err_t _set_address(rt_uint8_t address)
{
    USBCTRL->USB_ADDR = address;
    return RT_EOK;
}

static rt_err_t _set_config(rt_uint8_t address)
{
    return RT_EOK;
}

static rt_err_t _ep_set_stall(rt_uint8_t address)
{
    return RT_EOK;
}

static rt_err_t _ep_clear_stall(rt_uint8_t address)
{
    return RT_EOK;
}

static rt_err_t _ep_enable(struct uendpoint *ep)
{
    return RT_EOK;
}

static rt_err_t _ep_disable(struct uendpoint *ep)
{
    return RT_EOK;
}

static rt_ssize_t _ep_read_prepare(rt_uint8_t address, void *buffer, rt_size_t size)
{
    return 0;
}

static rt_ssize_t _ep_read(rt_uint8_t address, void *buffer)
{
    return 0;
}

static rt_ssize_t _ep_write(rt_uint8_t address, void *buffer, rt_size_t size)
{
    return 0;
}

static rt_err_t _ep0_send_status(void)
{
    return RT_EOK;
}

static rt_err_t _suspend(void)
{
    return RT_EOK;
}

static rt_err_t _wakeup(void)
{
    return RT_EOK;
}

static const struct udcd_ops _udc_ops =
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

/* MSP: USB1 clock + reset release (weak in hal_fsusb.c, uint32_t return) */
uint32_t HAL_FSUSB_MSP_Init(void)
{
    __HAL_RCC_USB1_CLK_ENABLE();
    __HAL_RCC_USB1_RESET();
    __HAL_RCC_USB1_CLK_ENABLE();
    return HAL_OK;
}

/* Controller init: HAL_FSUSB_Init (incl. connect) + NVIC */
static rt_err_t _init(rt_device_t dev)
{
    HAL_FSUSB_MSP_Init();
    HAL_FSUSB_Init();
    NVIC_SetPriority(FSUSB_IRQn, 2);
    NVIC_EnableIRQ(FSUSB_IRQn);
    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops _ops =
{
    _init,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
};
#endif

int fsusb_udc_register(void)
{
    rt_memset(&_udc, 0, sizeof(_udc));
    _udc.parent.parent.type = RT_Device_Class_USBDevice;
#ifdef RT_USING_DEVICE_OPS
    _udc.parent.parent.ops = &_ops;
#else
    _udc.parent.parent.init = _init;
#endif
    _udc.parent.ops = &_udc_ops;
    _udc.parent.ep_pool = _ep_pool;
    _udc.parent.ep0.id = &_ep_pool[0];
    rt_device_register(&_udc.parent.parent, "usbd", 0);
    rt_usb_device_init();
    return RT_EOK;
}
INIT_DEVICE_EXPORT(fsusb_udc_register);

/* ISR: filled in Task 3; empty now so the symbol exists */
void FSUSB_IRQHandler(void)
{
    rt_interrupt_enter();
    rt_interrupt_leave();
}

#endif /* BSP_USING_FSUSB */
