#ifndef __UART_CONFIG_H__
#define __UART_CONFIG_H__

#include "board.h"
#include "acm32p4.h"
#include "hardware/uart.h"
#include "hardware/dma.h"

#define DMA_INSTANCE_NONE  ((dma_instance_t)(-1))
#define DMA_CHANNEL_NONE   ((dma_channel_t)(-1))

struct acm32_uart_config
{
    const char          *name;
    uart_num_t          uart_num;
    IRQn_Type           irq_type;
    gpio_pin_t          tx_pin;
    gpio_pin_t          rx_pin;
    gpio_af_t           af;

    /* DMA 配置（不使用 DMA 时填 DMA_INSTANCE_NONE / DMA_CHANNEL_NONE） */
    dma_instance_t      dma_instance;
    dma_channel_t       tx_dma_channel;
    dma_channel_t       rx_dma_channel;
    uint8_t             tx_periph_id;
    uint8_t             rx_periph_id;
};

#ifdef BSP_USING_UART1
#ifndef BSP_USING_UART1_DMA
#define UART1_CONFIG \
    { .name = "uart1", .uart_num = UART_1, .irq_type = UART1_IRQn, \
      .tx_pin = PA9, .rx_pin = PA10, .af = GPIO_AF_1, \
      .dma_instance = DMA_INSTANCE_NONE, \
      .tx_dma_channel = DMA_CHANNEL_NONE, \
      .rx_dma_channel = DMA_CHANNEL_NONE, \
      .tx_periph_id = 0, .rx_periph_id = 0 }
#else
#define UART1_CONFIG \
    { .name = "uart1", .uart_num = UART_1, .irq_type = UART1_IRQn, \
      .tx_pin = PA9, .rx_pin = PA10, .af = GPIO_AF_1, \
      .dma_instance = DMA_1, \
      .tx_dma_channel = DMA_CHANNEL_0, \
      .rx_dma_channel = DMA_CHANNEL_1, \
      .tx_periph_id = 5, .rx_periph_id = 6 }
#endif
#endif

#endif
