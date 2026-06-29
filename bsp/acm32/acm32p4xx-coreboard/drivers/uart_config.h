#ifndef __UART_CONFIG_H__
#define __UART_CONFIG_H__

#include "board.h"
#include "acm32p4.h"
#include "hardware/uart.h"

struct acm32_uart_config
{
    const char          *name;
    uart_num_t          uart_num;
    IRQn_Type           irq_type;
    gpio_pin_t          tx_pin;
    gpio_pin_t          rx_pin;
    gpio_af_t           af;
};

#ifdef BSP_USING_UART1
#define UART1_CONFIG \
    { .name = "uart1", .uart_num = UART_1, .irq_type = UART1_IRQn, \
      .tx_pin = PA9, .rx_pin = PA10, .af = GPIO_AF_1 }
#endif

#endif
