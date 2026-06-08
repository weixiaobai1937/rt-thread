/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-06-04     AisinoChip   ACM32P4xx UART V2 driver
 *                              Support USART1-8 + LPUART1/2
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"
#include "uart_config.h"

#ifdef RT_USING_SERIAL_V2

enum { UART_TYPE_USART = 0, UART_TYPE_LPUART };

struct acm32_uart
{
    struct acm32_uart_config    *config;
    struct rt_serial_device     serial;
    union {
        UART_HandleTypeDef      usart;
        LPUART_HandleTypeDef    lpuart;
    } handle;
#ifdef RT_SERIAL_USING_DMA
    rt_uint16_t                 uart_dma_flag;
    struct
    {
        DMA_HandleTypeDef       handle;
        rt_size_t               last_index;
    } dma_rx;
    struct
    {
        DMA_HandleTypeDef       handle;
    } dma_tx;
#endif
};

#define raw_to_uart(raw) rt_container_of(raw, struct acm32_uart, serial)

/* ==================== Low-level register helpers ==================== */

rt_inline rt_uint32_t _flag_read(struct acm32_uart *uart)
{
    if (uart->config->uart_type == UART_TYPE_USART)
        return ((USART_TypeDef *)uart->config->Instance)->FR;
    else
        return ((LPUART_TypeDef *)uart->config->Instance)->SR;
}

rt_inline void _data_write(struct acm32_uart *uart, rt_uint8_t c)
{
    if (uart->config->uart_type == UART_TYPE_USART)
        ((USART_TypeDef *)uart->config->Instance)->DR = c;
    else
        ((LPUART_TypeDef *)uart->config->Instance)->TXDR = c;
}

rt_inline rt_uint8_t _data_read(struct acm32_uart *uart)
{
    if (uart->config->uart_type == UART_TYPE_USART)
        return ((USART_TypeDef *)uart->config->Instance)->DR & 0xFF;
    else
        return ((LPUART_TypeDef *)uart->config->Instance)->RXDR & 0xFF;
}

rt_inline void _ie_set(struct acm32_uart *uart, rt_uint32_t val)
{
    if (uart->config->uart_type == UART_TYPE_USART)
        ((USART_TypeDef *)uart->config->Instance)->IE = val;
    else
        ((LPUART_TypeDef *)uart->config->Instance)->IE = val;
}

rt_inline rt_uint32_t _ie_get(struct acm32_uart *uart)
{
    if (uart->config->uart_type == UART_TYPE_USART)
        return ((USART_TypeDef *)uart->config->Instance)->IE;
    else
        return ((LPUART_TypeDef *)uart->config->Instance)->IE;
}

rt_inline void _isr_clear(struct acm32_uart *uart, rt_uint32_t val)
{
    if (uart->config->uart_type == UART_TYPE_USART)
        ((USART_TypeDef *)uart->config->Instance)->ISR = val;
    else
        ((LPUART_TypeDef *)uart->config->Instance)->SR = val;
}

rt_inline rt_uint32_t _isr_read(struct acm32_uart *uart)
{
    if (uart->config->uart_type == UART_TYPE_USART)
        return ((USART_TypeDef *)uart->config->Instance)->ISR;
    else
        return ((LPUART_TypeDef *)uart->config->Instance)->SR;
}

/* LPUART status bits */
#define LPUART_SR_TXFF      (1 << 5)
#define LPUART_SR_BUSY      (1 << 9)
#define LPUART_SR_RXFE      (1 << 4)

/* LPUART interrupt bits */
#define LPUART_IE_TXI       (1 << 7)
#define LPUART_IE_RXI       (1 << 4)

/* ==================== HAL MspInit (USART 1-4 only; 5-8 add as needed) ==================== */

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef g = {0};

    if (huart->Instance == USART1)
    {
        __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_USART1_CLK_ENABLE();
        g.Pin = GPIO_PIN_9 | GPIO_PIN_10; g.Mode = GPIO_MODE_AF_PP;
        g.Pull = GPIO_PULLUP; g.Drive = GPIO_DRIVE_LEVEL3; g.Alternate = GPIO_FUNCTION_1;
        HAL_GPIO_Init(GPIOA, &g);
        NVIC_SetPriority(USART1_IRQn, 5); NVIC_EnableIRQ(USART1_IRQn);
    }
#ifdef BSP_USING_UART2
    else if (huart->Instance == USART2)
    {
        __HAL_RCC_GPIOD_CLK_ENABLE(); __HAL_RCC_USART2_CLK_ENABLE();
        g.Pin = BSP_UART2_PIN_TX | BSP_UART2_PIN_RX; g.Mode = GPIO_MODE_AF_PP;
        g.Pull = GPIO_PULLUP; g.Drive = GPIO_DRIVE_LEVEL3; g.Alternate = BSP_UART2_AF;
        HAL_GPIO_Init(BSP_UART2_PORT, &g);
        NVIC_SetPriority(USART2_IRQn, 5); NVIC_EnableIRQ(USART2_IRQn);
    }
#endif
#ifdef BSP_USING_UART3
    else if (huart->Instance == USART3)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE(); __HAL_RCC_USART3_CLK_ENABLE();
        g.Pin = BSP_UART3_PIN_TX | BSP_UART3_PIN_RX; g.Mode = GPIO_MODE_AF_PP;
        g.Pull = GPIO_PULLUP; g.Drive = GPIO_DRIVE_LEVEL3; g.Alternate = BSP_UART3_AF;
        HAL_GPIO_Init(BSP_UART3_PORT, &g);
        NVIC_SetPriority(USART3_IRQn, 5); NVIC_EnableIRQ(USART3_IRQn);
    }
#endif
#ifdef BSP_USING_UART4
    else if (huart->Instance == USART4)
    {
        __HAL_RCC_GPIOC_CLK_ENABLE(); __HAL_RCC_USART4_CLK_ENABLE();
        g.Pin = BSP_UART4_PIN_TX | BSP_UART4_PIN_RX; g.Mode = GPIO_MODE_AF_PP;
        g.Pull = GPIO_PULLUP; g.Drive = GPIO_DRIVE_LEVEL3; g.Alternate = BSP_UART4_AF;
        HAL_GPIO_Init(BSP_UART4_PORT, &g);
        NVIC_SetPriority(USART4_IRQn, 5); NVIC_EnableIRQ(USART4_IRQn);
    }
#endif
}

void HAL_LPUART_MspInit(LPUART_HandleTypeDef *hlpuart)
{
    GPIO_InitTypeDef g = {0};

#ifdef BSP_USING_LPUART1
    if (hlpuart->Instance == LPUART1)
    {
        __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_LPUART1_CLK_ENABLE();
        g.Pin = BSP_LPUART1_PIN_TX | BSP_LPUART1_PIN_RX; g.Mode = GPIO_MODE_AF_PP;
        g.Pull = GPIO_PULLUP; g.Drive = GPIO_DRIVE_LEVEL3; g.Alternate = BSP_LPUART1_AF;
        HAL_GPIO_Init(BSP_LPUART1_PORT, &g);
        NVIC_SetPriority(LPUART1_IRQn, 5); NVIC_EnableIRQ(LPUART1_IRQn);
    }
#endif
#ifdef BSP_USING_LPUART2
    if (hlpuart->Instance == LPUART2)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE(); __HAL_RCC_LPUART2_CLK_ENABLE();
        g.Pin = BSP_LPUART2_PIN_TX | BSP_LPUART2_PIN_RX; g.Mode = GPIO_MODE_AF_PP;
        g.Pull = GPIO_PULLUP; g.Drive = GPIO_DRIVE_LEVEL3; g.Alternate = BSP_LPUART2_AF;
        HAL_GPIO_Init(BSP_LPUART2_PORT, &g);
        NVIC_SetPriority(LPUART2_IRQn, 5); NVIC_EnableIRQ(LPUART2_IRQn);
    }
#endif
}

/* ==================== V2 Ops ==================== */

static rt_err_t _uart_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
    struct acm32_uart *uart = raw_to_uart(serial);

    if (uart->config->uart_type == UART_TYPE_USART)
    {
        uart->handle.usart.Instance          = (USART_TypeDef *)uart->config->Instance;
        uart->handle.usart.Init.BaudRate     = cfg->baud_rate;
        uart->handle.usart.Init.WordLength   = (cfg->data_bits == DATA_BITS_9) ? UART_WORDLENGTH_9B : UART_WORDLENGTH_8B;
        uart->handle.usart.Init.StopBits     = (cfg->stop_bits == STOP_BITS_2) ? UART_STOPBITS_2 : UART_STOPBITS_1;
        if (cfg->parity == PARITY_ODD)       uart->handle.usart.Init.Parity = UART_PARITY_ODD;
        else if (cfg->parity == PARITY_EVEN) uart->handle.usart.Init.Parity = UART_PARITY_EVEN;
        else                                 uart->handle.usart.Init.Parity = UART_PARITY_NONE;
        uart->handle.usart.Init.Mode       = UART_MODE_TX_RX;
        uart->handle.usart.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
        HAL_UART_Init(&uart->handle.usart);
    }
    else
    {
        uart->handle.lpuart.Instance          = (LPUART_TypeDef *)uart->config->Instance;
        uart->handle.lpuart.Init.BaudRate     = cfg->baud_rate;
        uart->handle.lpuart.Init.WordLength   = (cfg->data_bits == DATA_BITS_9) ? LPUART_WORDLENGTH_8B : LPUART_WORDLENGTH_8B;
        uart->handle.lpuart.Init.StopBits     = (cfg->stop_bits == STOP_BITS_2) ? LPUART_STOPBITS_2B : LPUART_STOPBITS_1B;
        uart->handle.lpuart.Init.Parity       = LPUART_PARITY_NONE;
        if (cfg->parity == PARITY_ODD)       uart->handle.lpuart.Init.Parity = LPUART_PARITY_ODD;
        else if (cfg->parity == PARITY_EVEN) uart->handle.lpuart.Init.Parity = LPUART_PARITY_EVEN;
        uart->handle.lpuart.Init.Mode         = LPUART_MODE_TXRX;
        uart->handle.lpuart.Init.Polarity     = 0;
        uart->handle.lpuart.Init.ClockSource  = 0;
        HAL_LPUART_Init(&uart->handle.lpuart);
    }
    return RT_EOK;
}

static rt_err_t _uart_control(struct rt_serial_device *serial, int cmd, void *arg)
{
    struct acm32_uart *uart = raw_to_uart(serial);

    switch (cmd)
    {
    case RT_DEVICE_CTRL_CLR_INT:
        NVIC_DisableIRQ(uart->config->irq_type);
        _ie_set(uart, _ie_get(uart) & ~(UART_IE_RXI));
        break;
    case RT_DEVICE_CTRL_SET_INT:
        NVIC_EnableIRQ(uart->config->irq_type);
        _ie_set(uart, _ie_get(uart) | UART_IE_RXI);
        break;
    }
    return RT_EOK;
}

static int _uart_putc(struct rt_serial_device *serial, char c)
{
    struct acm32_uart *uart = raw_to_uart(serial);
    rt_uint32_t tx_mask = (uart->config->uart_type == UART_TYPE_USART) ? UART_FR_TXFF : LPUART_SR_TXFF;
    rt_uint32_t busy_mask = (uart->config->uart_type == UART_TYPE_USART) ? UART_FR_BUSY : LPUART_SR_BUSY;

    while (_flag_read(uart) & tx_mask);
    _data_write(uart, c);
    while (_flag_read(uart) & busy_mask);
    return 1;
}

static int _uart_getc(struct rt_serial_device *serial)
{
    struct acm32_uart *uart = raw_to_uart(serial);
    rt_uint32_t rx_mask = (uart->config->uart_type == UART_TYPE_USART) ? UART_FR_RXFE : LPUART_SR_RXFE;
    int ch = -1;

    if (!(_flag_read(uart) & rx_mask))
        ch = _data_read(uart);
    return ch;
}

#ifdef RT_SERIAL_USING_DMA
static rt_ssize_t _uart_dma_transmit(struct rt_serial_device *serial, rt_uint8_t *buf, rt_size_t size, rt_uint32_t tx_flag)
{
    struct acm32_uart *uart = raw_to_uart(serial);
    RT_UNUSED(tx_flag);
    if (size == 0) return 0;

    if (uart->config->uart_type == UART_TYPE_USART)
        return (HAL_UART_Transmit_DMA(&uart->handle.usart, buf, size) == HAL_OK) ? size : 0;
    else
        return (HAL_LPUART_Transmit_DMA(&uart->handle.lpuart, buf, size) == HAL_OK) ? size : 0;
}
#endif

/* Always provide a transmit function - V2 requires it */
static rt_ssize_t _uart_transmit(struct rt_serial_device *serial, rt_uint8_t *buf, rt_size_t size, rt_uint32_t tx_flag)
{
#ifdef RT_SERIAL_USING_DMA
    return _uart_dma_transmit(serial, buf, size, tx_flag);
#else
    /* Fallback: polled transmit */
    rt_size_t i;
    for (i = 0; i < size; i++)
        _uart_putc(serial, buf[i]);
    return size;
#endif
}

static const struct rt_uart_ops acm32_uart_ops =
{
    .configure = _uart_configure,
    .control   = _uart_control,
    .putc      = _uart_putc,
    .getc      = _uart_getc,
    .transmit  = _uart_transmit,
};

/* ==================== ISR ==================== */

static void uart_isr(struct rt_serial_device *serial)
{
    struct acm32_uart *uart = raw_to_uart(serial);
    rt_uint32_t isr = _isr_read(uart);
    rt_uint32_t ie  = _ie_get(uart);

    /* RX */
    if ((ie & UART_IE_RXI) && (isr & UART_ISR_RXI))
    {
        rt_hw_serial_isr(serial, RT_SERIAL_EVENT_RX_IND);
        _isr_clear(uart, UART_ISR_RXI);
    }

    /* TX done */
    if ((ie & UART_IE_TXI) && (isr & UART_ISR_TXI))
    {
        _isr_clear(uart, UART_ISR_TXI);
        _ie_set(uart, ie & ~UART_IE_TXI);
        if (serial->parent.open_flag & RT_DEVICE_FLAG_INT_TX)
            rt_hw_serial_isr(serial, RT_SERIAL_EVENT_TX_DONE);
    }
}

/* ==================== ISR Handlers ==================== */

#define UART_ISR(irq_name, index)                    \
    void irq_name##_IRQHandler(void)                 \
    {                                                \
        rt_interrupt_enter();                        \
        uart_isr(&uart_obj[index].serial);           \
        rt_interrupt_leave();                        \
    }

/* ==================== Instances ==================== */

enum {
#ifdef BSP_USING_UART1
    UART1_INDEX,
#endif
#ifdef BSP_USING_UART2
    UART2_INDEX,
#endif
#ifdef BSP_USING_UART3
    UART3_INDEX,
#endif
#ifdef BSP_USING_UART4
    UART4_INDEX,
#endif
#ifdef BSP_USING_UART5
    UART5_INDEX,
#endif
#ifdef BSP_USING_UART6
    UART6_INDEX,
#endif
#ifdef BSP_USING_UART7
    UART7_INDEX,
#endif
#ifdef BSP_USING_UART8
    UART8_INDEX,
#endif
#ifdef BSP_USING_LPUART1
    LPUART1_INDEX,
#endif
#ifdef BSP_USING_LPUART2
    LPUART2_INDEX,
#endif
    UART_MAX_INDEX,
};

static struct acm32_uart_config uart_config[] = {
#ifdef BSP_USING_UART1
    UART1_CONFIG,
#endif
#ifdef BSP_USING_UART2
    UART2_CONFIG,
#endif
#ifdef BSP_USING_UART3
    UART3_CONFIG,
#endif
#ifdef BSP_USING_UART4
    UART4_CONFIG,
#endif
#ifdef BSP_USING_UART5
    UART5_CONFIG,
#endif
#ifdef BSP_USING_UART6
    UART6_CONFIG,
#endif
#ifdef BSP_USING_UART7
    UART7_CONFIG,
#endif
#ifdef BSP_USING_UART8
    UART8_CONFIG,
#endif
#ifdef BSP_USING_LPUART1
    LPUART1_CONFIG,
#endif
#ifdef BSP_USING_LPUART2
    LPUART2_CONFIG,
#endif
};

static struct acm32_uart uart_obj[UART_MAX_INDEX] = {0};

rt_err_t rt_hw_uart_init(void)
{
    rt_size_t n = sizeof(uart_obj) / sizeof(struct acm32_uart);
    struct serial_configure c = RT_SERIAL_CONFIG_DEFAULT;

    for (int i = 0; i < n; i++)
    {
        uart_obj[i].config        = &uart_config[i];
        uart_obj[i].serial.ops    = &acm32_uart_ops;
        uart_obj[i].serial.config = c;
        rt_err_t rc = rt_hw_serial_register(&uart_obj[i].serial,
            uart_obj[i].config->name,
            RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_INT_TX,
            NULL);
        RT_ASSERT(rc == RT_EOK);
    }
    return RT_EOK;
}

/* ISR instantiations */
#ifdef BSP_USING_UART1
UART_ISR(USART1, UART1_INDEX)
#endif
#ifdef BSP_USING_UART2
UART_ISR(USART2, UART2_INDEX)
#endif
#ifdef BSP_USING_UART3
UART_ISR(USART3, UART3_INDEX)
#endif
#ifdef BSP_USING_UART4
UART_ISR(USART4, UART4_INDEX)
#endif
#ifdef BSP_USING_UART5
UART_ISR(USART5, UART5_INDEX)
#endif
#ifdef BSP_USING_UART6
UART_ISR(USART6, UART6_INDEX)
#endif
#ifdef BSP_USING_UART7
UART_ISR(USART7, UART7_INDEX)
#endif
#ifdef BSP_USING_UART8
UART_ISR(USART8, UART8_INDEX)
#endif
#ifdef BSP_USING_LPUART1
UART_ISR(LPUART1, LPUART1_INDEX)
#endif
#ifdef BSP_USING_LPUART2
UART_ISR(LPUART2, LPUART2_INDEX)
#endif

#endif /* RT_USING_SERIAL_V2 */
