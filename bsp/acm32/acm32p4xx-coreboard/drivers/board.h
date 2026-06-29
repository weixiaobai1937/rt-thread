#ifndef __BOARD_H__
#define __BOARD_H__

#include <rtconfig.h>
#include "acm32p4.h"

/* UART1: TX=PA9(AF1), RX=PA10(AF1) - console */
#if defined(BSP_USING_UART1)
    #define UART1_TX_PIN            PA9
    #define UART1_RX_PIN            PA10
    #define UART1_AF                GPIO_AF_1
#endif

/* Coreboard LED: PA0 */
#define LED_PIN                  PA0

/* SOC memory config (overrides rtconfig.h defaults) */
#undef  SOC_SRAM_START_ADDR
#define SOC_SRAM_START_ADDR      (0x20000000)
#undef  SOC_SRAM_SIZE
#define SOC_SRAM_SIZE            (128)   /* KB */

void rt_hw_board_init(void);

#endif /* __BOARD_H__ */
