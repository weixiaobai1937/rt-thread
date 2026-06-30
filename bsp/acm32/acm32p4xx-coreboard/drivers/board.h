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

/* ETH RMII: LAN8720A PHY, all AF6 */
#if defined(BSP_USING_ETH)
    #define ETH_RST_PIN             PC0
    #define ETH_REF_CLK_PIN         PA1
    #define ETH_REF_CLK_AF          GPIO_AF_6
    #define ETH_MDIO_PIN            PA2
    #define ETH_MDIO_AF             GPIO_AF_6
    #define ETH_CRS_DV_PIN          PA7
    #define ETH_CRS_DV_AF           GPIO_AF_6
    #define ETH_MDC_PIN             PC1
    #define ETH_MDC_AF              GPIO_AF_6
    #define ETH_RXD0_PIN            PC4
    #define ETH_RXD0_AF             GPIO_AF_6
    #define ETH_RXD1_PIN            PC5
    #define ETH_RXD1_AF             GPIO_AF_6
    #define ETH_TX_EN_PIN           PB11
    #define ETH_TX_EN_AF            GPIO_AF_6
    #define ETH_TXD0_PIN            PB12
    #define ETH_TXD0_AF             GPIO_AF_6
    #define ETH_TXD1_PIN            PB13
    #define ETH_TXD1_AF             GPIO_AF_6
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
