/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-08-23     AisinoChip   the first version
 * 2026-06-04     AisinoChip   add ACM32P4xx support
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"
#include "uart_config.h"

#ifdef RT_USING_SERIAL

/**
 * @brief  UART MSP Initialization
 *         This function configures the hardware resources used in this example:
 *           - Peripheral's clock enable
 *           - Peripheral's GPIO Configuration
 * @param  huart: UART handle pointer
 * @retval None
 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (huart->Instance == USART1)
    {
        /* Enable GPIO clocks */
        __HAL_RCC_GPIOA_CLK_ENABLE();
        /* Enable USART1 clock */
        __HAL_RCC_USART1_CLK_ENABLE();

        /* Configure UART1 TX pin: PA9 */
        GPIO_InitStruct.Pin       = GPIO_PIN_9;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_PULLUP;
        GPIO_InitStruct.Drive     = GPIO_DRIVE_LEVEL3;
        GPIO_InitStruct.Alternate = GPIO_FUNCTION_1;  /* AF1: USART1_TX */
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* Configure UART1 RX pin: PA10 */
        GPIO_InitStruct.Pin       = GPIO_PIN_10;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_PULLUP;
        GPIO_InitStruct.Drive     = GPIO_DRIVE_LEVEL3;
        GPIO_InitStruct.Alternate = GPIO_FUNCTION_1;  /* AF1: USART1_RX */
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

struct acm32_uart_config
{
    const char          *name;
    USART_TypeDef       *Instance;
    IRQn_Type           irq_type;

    GPIO_TypeDef        *tx_port;
    GPIO_TypeDef        *rx_port;
    uint32_t            tx_pin;
    uint32_t            rx_pin;
};

struct acm32_uart
{
    UART_HandleTypeDef          handle;
    struct acm32_uart_config    *config;
    struct rt_serial_device     serial;
};

static rt_err_t _uart_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
    struct acm32_uart *uart;

    RT_ASSERT(serial != RT_NULL);
    RT_ASSERT(cfg != RT_NULL);

    uart = rt_container_of(serial, struct acm32_uart, serial);

    uart->handle.Instance          = uart->config->Instance;
    uart->handle.Init.BaudRate     = cfg->baud_rate;

    if (cfg->data_bits == DATA_BITS_8)
    {
        uart->handle.Init.WordLength = UART_WORDLENGTH_8B;
    }
    else if (cfg->data_bits == DATA_BITS_9)
    {
        uart->handle.Init.WordLength = UART_WORDLENGTH_9B;
    }
    else
    {
        return -RT_EINVAL;
    }

    if (cfg->stop_bits == STOP_BITS_1)
    {
        uart->handle.Init.StopBits = UART_STOPBITS_1;
    }
    else if (cfg->stop_bits == STOP_BITS_2)
    {
        uart->handle.Init.StopBits = UART_STOPBITS_2;
    }
    else
    {
        return -RT_EINVAL;
    }

    if (cfg->parity == PARITY_NONE)
    {
        uart->handle.Init.Parity = UART_PARITY_NONE;
    }
    else if (cfg->parity == PARITY_ODD)
    {
        uart->handle.Init.Parity = UART_PARITY_ODD;
    }
    else if (cfg->parity == PARITY_EVEN)
    {
        uart->handle.Init.Parity = UART_PARITY_EVEN;
    }
    else
    {
        return -RT_EINVAL;
    }

    uart->handle.Init.Mode       = UART_MODE_TX_RX;
    uart->handle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;

    HAL_UART_Init(&uart->handle);

    return RT_EOK;
}

static rt_err_t _uart_control(struct rt_serial_device *serial, int cmd, void *arg)
{
    struct acm32_uart *uart;

    RT_ASSERT(serial != RT_NULL);

    uart = rt_container_of(serial, struct acm32_uart, serial);

    switch (cmd)
    {
    /* disable interrupt */
    case RT_DEVICE_CTRL_CLR_INT:
        NVIC_DisableIRQ(uart->config->irq_type);
        /* Disable RX interrupt */
        uart->handle.Instance->IE &= ~UART_IE_RXI;
        break;
    /* enable interrupt */
    case RT_DEVICE_CTRL_SET_INT:
        NVIC_EnableIRQ(uart->config->irq_type);
        /* Enable RX interrupt */
        uart->handle.Instance->IE |= UART_IE_RXI;
        break;
    }

    return RT_EOK;
}

static int _uart_putc(struct rt_serial_device *serial, char c)
{
    struct acm32_uart *uart;

    RT_ASSERT(serial != RT_NULL);

    uart = rt_container_of(serial, struct acm32_uart, serial);

    while (uart->handle.Instance->FR & UART_FR_TXFF);    /* wait Tx FIFO not full */
    uart->handle.Instance->DR = c;
    while ((uart->handle.Instance->FR & UART_FR_BUSY));   /* wait TX Complete */

    return 1;
}

static int _uart_getc(struct rt_serial_device *serial)
{
    struct acm32_uart *uart;
    int ch;

    RT_ASSERT(serial != RT_NULL);

    uart = rt_container_of(serial, struct acm32_uart, serial);

    ch = -1;
    if (!(uart->handle.Instance->FR & UART_FR_RXFE))   /* Rx FIFO not empty */
    {
        ch = uart->handle.Instance->DR & 0xff;
    }

    return ch;
}

static const struct rt_uart_ops acm32_uart_ops =
{
    _uart_configure,
    _uart_control,
    _uart_putc,
    _uart_getc,
};

enum
{
#ifdef BSP_USING_UART1
    UART1_INDEX,
#endif
    UART_MAX_INDEX,
};

static struct acm32_uart_config uart_config[] =
{
#ifdef BSP_USING_UART1
    UART1_CONFIG,
#endif
};

static struct acm32_uart uart_obj[sizeof(uart_config) / sizeof(uart_config[0])] = {0};

rt_err_t rt_hw_uart_init(void)
{
    rt_size_t obj_num = sizeof(uart_obj) / sizeof(struct acm32_uart);
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    rt_err_t rc = RT_EOK;

    for (int i = 0; i < obj_num; i++)
    {
        uart_obj[i].config          = &uart_config[i];

        uart_obj[i].serial.ops      = &acm32_uart_ops;
        uart_obj[i].serial.config   = config;

        /* register UART device */
        rc = rt_hw_serial_register(&uart_obj[i].serial, uart_obj[i].config->name,
                                   RT_DEVICE_FLAG_RDWR
                                   | RT_DEVICE_FLAG_INT_RX
                                   | RT_DEVICE_FLAG_INT_TX
                                   , NULL);
        RT_ASSERT(rc == RT_EOK);
    }

    return rc;
}

static void uart_isr(struct rt_serial_device *serial)
{
    struct acm32_uart *uart = rt_container_of(serial, struct acm32_uart, serial);

    RT_ASSERT(serial != RT_NULL);

    /* UART in mode Receiver ---------------------------------------------------*/
    if ((uart->handle.Instance->IE & UART_IE_RXI) &&
        (uart->handle.Instance->ISR & UART_ISR_RXI))
    {
        rt_hw_serial_isr(serial, RT_SERIAL_EVENT_RX_IND);
        /* Clear RX interrupt flag by writing to ISR */
        uart->handle.Instance->ISR = UART_ISR_RXI;
    }

    /* UART in mode Transmitter ------------------------------------------------*/
    if ((uart->handle.Instance->IE & UART_IE_TXI) &&
        (uart->handle.Instance->ISR & UART_ISR_TXI))
    {
        /* Clear TX interrupt flag by writing to ISR */
        uart->handle.Instance->ISR = UART_ISR_TXI;

        if (serial->parent.open_flag & RT_DEVICE_FLAG_INT_TX)
        {
            rt_hw_serial_isr(serial, RT_SERIAL_EVENT_TX_DONE);
        }
        /* Disable TX interrupt */
        uart->handle.Instance->IE &= ~UART_IE_TXI;
    }
}

#if defined(BSP_USING_UART1)
void USART1_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    uart_isr(&uart_obj[UART1_INDEX].serial);

    /* leave interrupt */
    rt_interrupt_leave();
}
#endif /* BSP_USING_UART1 */

#endif /* RT_USING_SERIAL */
