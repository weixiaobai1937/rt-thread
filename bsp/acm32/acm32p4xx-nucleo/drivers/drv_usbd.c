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
    /* RX path: buffer registered by _ep_read_prepare, filled by ISR */
    rt_uint8_t *rx_buf[7];
    rt_uint16_t rx_len[7];
    /* last TX length per EP, reported on IN completion */
    rt_uint16_t last_tx_size[7];
};
static struct acm32_udc _udc;

/* ==================== Endpoint helpers ==================== */

/* EP address (0x00 / 0x01..0x06 / 0x81..0x86) -> physical index (0..6) */
static int _ep_index(rt_uint8_t addr)
{
    return addr & 0x7F;
}

static rt_bool_t _ep_is_in(rt_uint8_t addr)
{
    return (addr & 0x80) ? RT_TRUE : RT_FALSE;
}

/* Enable / disable the per-EP interrupt for the given endpoint */
static void _ep_enable_ie(rt_uint8_t addr, rt_bool_t on)
{
    int ep = _ep_index(addr);
    rt_uint32_t mask = _ep_is_in(addr) ? MASK_EPX_IN(ep) : MASK_EPX_OUT(ep);

    if (on)
        HAL_FSUSB_Enable_EP_Interrupt(mask);
    else
        HAL_FSUSB_Disable_EP_Interrupt(mask);
}

/* ==================== udcd ops (register level, non-blocking) ==================== */

static rt_err_t _set_address(rt_uint8_t address)
{
    USBCTRL->USB_ADDR = address;
    return RT_EOK;
}

static rt_err_t _set_config(rt_uint8_t address)
{
    /* FSUSB has no extra per-config action */
    return RT_EOK;
}

static rt_err_t _ep_set_stall(rt_uint8_t address)
{
    USBCTRL->EPxCSR[_ep_index(address)] |= (1 << 12);
    return RT_EOK;
}

static rt_err_t _ep_clear_stall(rt_uint8_t address)
{
    /* Clear in/out toggle + stall (same pattern as vendor usb_clear_stall) */
    USBCTRL->EPxCSR[_ep_index(address)] = 0x2100;
    return RT_EOK;
}

static rt_err_t _ep_enable(struct uendpoint *ep)
{
    int idx;

    RT_ASSERT(ep != RT_NULL);
    RT_ASSERT(ep->ep_desc != RT_NULL);

    idx = _ep_index(ep->ep_desc->bEndpointAddress);
    USBCTRL->EPxCSR[idx] |= (1 << 8);   /* EP_ENABLE */
    _ep_enable_ie(ep->ep_desc->bEndpointAddress, RT_TRUE);
    return RT_EOK;
}

static rt_err_t _ep_disable(struct uendpoint *ep)
{
    int idx;

    RT_ASSERT(ep != RT_NULL);
    RT_ASSERT(ep->ep_desc != RT_NULL);

    idx = _ep_index(ep->ep_desc->bEndpointAddress);
    _ep_enable_ie(ep->ep_desc->bEndpointAddress, RT_FALSE);
    USBCTRL->EPxCSR[idx] &= ~(1 << 8);  /* clear EP_ENABLE */
    return RT_EOK;
}

static rt_ssize_t _ep_read(rt_uint8_t address, void *buffer)
{
    /* Data was copied from FIFO into the prepare buffer by the ISR */
    return 0;
}

static rt_ssize_t _ep_read_prepare(rt_uint8_t address, void *buffer, rt_size_t size)
{
    int ep = _ep_index(address);

    _udc.rx_buf[ep] = buffer;
    _udc.rx_len[ep] = 0;
    USBCTRL->EPxCSR[ep] |= (1 << 11);   /* RECEIVE_READY */
    _ep_enable_ie(address, RT_TRUE);
    return size;
}

static rt_ssize_t _ep_write(rt_uint8_t address, void *buffer, rt_size_t size)
{
    int ep = _ep_index(address);

    if (size == 0)
    {
        /* Zero-length packet */
        USBCTRL->EPxSENDBN[ep] = 0;
        USBCTRL->EPxCSR[ep] |= (1 << 10);
        return 0;
    }

    /* Load the EP FIFO (64B per EP; stack splits by maxpacket anyway) */
    HAL_FSUSB_Write_EP_MEM8((rt_uint8_t *)buffer, (uint32_t)size, 0, (uint8_t)ep);

    USBCTRL->EPxSENDBN[ep] = (uint32_t)size;
    USBCTRL->EPxCSR[ep] |= (1 << 10);   /* SEND_ENABLE: data ready for TX */
    _udc.last_tx_size[ep] = (rt_uint16_t)size;
    _ep_enable_ie(address, RT_TRUE);
    return size;
}

static rt_err_t _ep0_send_status(void)
{
    HAL_FSUSB_EP0_Send_Empty_Packet();
    return RT_EOK;
}

static rt_err_t _suspend(void)
{
    HAL_FSUSB_PHY_Suspend();
    return RT_EOK;
}

static rt_err_t _wakeup(void)
{
    HAL_FSUSB_PHY_Resume();
    HAL_FSUSB_Send_Remote_Wakeup_Signal();
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

/* ==================== ISR ==================== */

void FSUSB_IRQHandler(void)
{
    struct acm32_udc *udc = &_udc;
    rt_uint32_t gisr, eisr;
    int ep;

    rt_interrupt_enter();

    gisr = USBINT->INT_STAT_RAW;
    eisr = USBINT->EP_INT_STAT_RAW;
    USBINT->INT_STAT_RAW = gisr;        /* write 1 to clear */
    USBINT->EP_INT_STAT_RAW = eisr;

    /* Bus reset */
    if (gisr & USB_BUS_RESET)
    {
        rt_usbd_reset_handler(&udc->parent);
    }

    /* EP0 SETUP packet (8 bytes; urequest layout matches DEVICE_REQUEST) */
    if (gisr & (USB_SETUPTOK | USB_EP0_SETUP_PACKET))
    {
        struct urequest req;

        HAL_FSUSB_Read_EP_MEM8((rt_uint8_t *)&req, 8, 0, USB_EP0);
        rt_usbd_ep0_setup_handler(&udc->parent, &req);
    }

    /* Per-EP interrupts */
    for (ep = 0; ep <= 6; ep++)
    {
        if (eisr & MASK_EPX_IN(ep))
        {
            if (ep == 0)
                rt_usbd_ep0_in_handler(&udc->parent);
            else
                rt_usbd_ep_in_handler(&udc->parent, 0x80 | ep, udc->last_tx_size[ep]);
        }

        if (eisr & MASK_EPX_OUT(ep))
        {
            rt_uint16_t len = HAL_FSUSB_Get_FIFO_Length((uint8_t)ep);

            if (ep == 0)
            {
                rt_uint8_t *dst = udc->rx_buf[0];

                if (dst && len)
                    HAL_FSUSB_Read_EP_MEM8(dst, len, 0, USB_EP0);
                udc->rx_len[0] = len;
                rt_usbd_ep0_out_handler(&udc->parent, len);
            }
            else
            {
                rt_uint8_t *dst = udc->rx_buf[ep];

                if (dst && len)
                    HAL_FSUSB_Read_EP_MEM8(dst, len, 0, (uint8_t)ep);
                udc->rx_len[ep] = len;
                rt_usbd_ep_out_handler(&udc->parent, 0x00 | ep, len);
                USBCTRL->EPxCSR[ep] |= (1 << 11);   /* re-arm RECEIVE_READY */
            }
        }
    }

    rt_interrupt_leave();
}

#endif /* BSP_USING_FSUSB */
