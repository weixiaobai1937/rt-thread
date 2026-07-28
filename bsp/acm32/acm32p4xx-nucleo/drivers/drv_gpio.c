/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author            Notes
 * 2021-09-18     AisinoChip        first version
 * 2026-06-04     AisinoChip        add ACM32P4xx support
 */

#include <rthw.h>
#include <rtdevice.h>
#include "board.h"


/*
GPIO复用功能映射表
| 引脚名称 | AF0          | AF1          | AF2          | AF3          | AF4       | AF5        | AF6                             | AF7         | 附加功能                       |
| -------- | ------------ | ------------ | ------------ | ------------ | --------- | ---------- | ------------------------------- | ----------- | ------------------------------ |
| PA0      | LPTIM1_OUT   | UART2_CTS    | TIM2_CH1_ETR | UART4_TX     | TIM5_CH1  | TIM8_ETR   | ETH_MII_CRS                     | —           | ADC_INP16, RTC_TAMP2, WKUP0    |
| PA1      | SDMMC1_D6    | UART2_RTS_DE | TIM2_CH2     | UART4_RX     | TIM5_CH2  | OSPI2_DQS  | ETH_MII_RX_CLK/ETH_RMII_REF_CLK | I2C1_SDA    | ADC_INN16, ADC_INP17           |
| PA2      | SDMMC1_D7    | UART2_TX     | TIM2_CH3     | TIM9_CH1     | TIM5_CH3  | —          | ETH_MDIO                        | I2C1_SCL    | ADC_INP14, WKUP1               |
| PA3      | —            | UART2_RX     | TIM2_CH4     | TIM9_CH2     | TIM5_CH4  | OSPI2_CLK  | ETH_MII_COL                     | —           | ADC_INP19                      |
| PA4      | SPI1_CS      | TIM10_CH1    | —            | SPI3_CS      | TIM5_ETR  | UART2_CK   | —                               | I2S1_WS     | ADC_INP3, DAC1_OUT1            |
| PA5      | SPI1_SCK     | TIM1_CH4     | TIM2_CH1_ETR | —            | TIM8_CH1N | —          | —                               | I2S1_CK     | ADC_INN3, ADC_INP1, DAC1_OUT2  |
| PA6      | SPI1_MISO    | TIM3_CH1     | TIM1_BKIN    | SPI1_IO3     | TIM8_BKIN | OSPI2_IO3  | UART4_RX                        | I2S1_SDI    | ADC_INP9                       |
| PA7      | SPI1_MOSI    | TIM3_CH2     | TIM1_CH1N    | UART1_TX     | TIM8_CH1N | OSPI2_IO2  | ETH_MII_RX_DV/ETH_RMII_CRS_DV   | SDMMC1_D0   | ADC_INN9, ADC_INP7             |
| PA8      | MCO1         | —            | TIM1_CH1     | —            | SPI4_IO3  | UART7_RX   | UART1_CK                        | —           | —                              |
| PA9      | LPUART1_TX   | UART1_TX     | TIM1_CH2     | SPI2_SCK     | —         | UART4_CK   | ETH_TX_ER                       | —           | —                              |
| PA10     | LPUART1_RX   | UART1_RX     | TIM1_CH3     | SPI1_IO2     | TIM8_BKIN | —          | —                               | —           | —                              |
| PA11     | —            | UART1_CTS    | TIM1_CH4     | SPI2_CS      | CAN1_RX   | UART4_RX   | COMP1_OUT                       | I2S2_WS     | USBD_FS_DM                     |
| PA12     | —            | UART1_RTS    | TIM1_ETR     | SPI2_SCK     | CAN1_TX   | UART4_TX   | —                               | I2S2_CK     | USBD_FS_DP                     |
| PA13     | SWDIO_TMS    | UART1_RX     | —            | UART2_RTS    | I2C1_SCL  | —          | ETH_TX_ER                       | —           | —                              |
| PA14     | SWCLK_TCK    | UART1_TX     | —            | —            | I2C1_SDA  | —          | —                               | —           | —                              |
| PA15     | TDI          | UART4_RTS    | TIM2_CH1_ETR | SPI1_CS      | SPI3_CS   | OSPI2_DQS  | UART7_TX                        | I2S1_WS     | —                              |
| PB0      | —            | TIM3_CH3     | TIM1_CH2N    | UART2_RX     | TIM8_CH2N | OSPI2_IO1  | ETH_MII_RXD2                    | UART4_CTS   | ADC_INN5, ADC_INP18, COMP1_INP |
| PB1      | SPI4_MOSI    | TIM3_CH4     | TIM1_CH3N    | UART2_RTS    | TIM8_CH3N | OSPI2_IO0  | ETH_MII_RXD3                    | UART4_RTS   | ADC_INP5, COMP1_INM            |
| PB2      | OSPI2_DQS    | SPI4_SCK     | —            | UART2_CK     | SPI3_MOSI | OSPI2_CLK  | ETH_TX_ER                       | UART7_CK    | COMP1_INP                      |
| PB3      | TDO          | UART5_CK     | TIM2_CH2     | SPI3_SCK     | SPI1_SCK  | —          | UART7_RX                        | I2S1_CK     | —                              |
| PB4      | TRST         | TIM3_CH1     | SPI2_CS      | SPI3_MISO    | SPI1_MISO | OSPI2_CLK  | UART7_TX                        | I2S1_SDI    | —                              |
| PB5      | SPI1_MOSI    | TIM3_CH2     | —            | SPI3_MOSI    | UART5_RX  | OSPI2_NCLK | ETH_PPS_OUT                     | I2S1_SDO    | WKUP4                          |
| PB6      | LPUART1_TX   | UART1_TX     | TIM4_CH1     | SPI4_MISO    | UART5_TX  | OSPI2_NCS  | —                               | I2C1_SCL    | —                              |
| PB7      | LPUART1_RX   | UART1_RX     | TIM4_CH2     | —            | UART5_CK  | I2S2_MCK   | COMP1_OUT                       | I2C1_SDA    | —                              |
| PB8      | TIM10_CH1    | UART4_RX     | TIM4_CH3     | —            | CAN1_RX   | I2C1_SCL   | ETH_MII_TXD3                    | SDMMC1_D4   | —                              |
| PB9      | SDMMC1_CMD   | UART4_TX     | TIM4_CH4     | SPI2_CS      | CAN1_TX   | I2C1_SDA   | SDMMC1_D5                       | I2S2_WS     | —                              |
| PB10     | I2C2_SCL     | UART3_TX     | TIM2_CH3     | SPI2_SCK     | CAN1_STBY | OSPI2_NCS  | ETH_MII_RX_ER                   | I2S2_CK     | —                              |
| PB11     | I2C2_SDA     | UART3_RX     | TIM2_CH4     | —            | —         | —          | ETH_MII_TX_EN/ETH_RMII_TX_EN    | SDMMC1_D1   | —                              |
| PB12     | SDMMC1_D2    | UART3_CK     | TIM1_BKIN    | SPI2_CS      | UART5_RX  | OSPI2_NCLK | ETH_MII_TXD0/ETH_RMII_TXD0      | I2S2_WS     | —                              |
| PB13     | UART3_CTS    | SDMMC1_D3    | TIM1_CH1N    | SPI2_SCK     | UART5_TX  | —          | ETH_MII_TXD1/ETH_RMII_TXD1      | I2S2_CK     | —                              |
| PB14     | UART3_RTS_DE | UART4_RTS    | TIM1_CH2N    | SPI2_MISO    | TIM8_CH2N | SPI4_MOSI  | UART1_TX                        | I2S2_SDI    | —                              |
| PB15     | —            | UART4_CTS    | TIM1_CH3N    | SPI2_MOSI    | TIM8_CH3N | OSPI2_CLK  | UART1_RX                        | I2S2_SDO    | WKUP6                          |
| PC0      | LPUART2_TX   | —            | —            | —            | TIM9_CH1  | —          | OSPI2_IO7                       | —           | ADC_INP10                      |
| PC1      | LPUART2_RX   | SDMMC1_CK    | UART2_RX     | SPI2_MOSI    | —         | OSPI2_IO4  | ETH_MDC                         | I2S2_SDO    | ADC_INN10, ADC_INP11           |
| PC2      | TIM9_CH2     | —            | UART2_TX     | SPI2_MISO    | OSPI2_IO2 | OSPI2_IO5  | ETH_MII_TXD2                    | I2S2_SDI    | ADC_INP2                       |
| PC3      | —            | SPI1_MOSI    | UART2_TX     | SPI2_MOSI    | OSPI2_IO0 | OSPI2_IO6  | ETH_MII_TX_CLK                  | I2S2_SDO    | ADC_INN2, ADC_INP6             |
| PC4      | IR_OUT       | SDMMC1_D4    | UART6_CTS    | UART1_RTS    | —         | —          | ETH_MII_RXD0/ETH_RMII_RXD0      | I2S1_MCK    | ADC_INP4, COMP1_INM            |
| PC5      | IR_OUT       | SDMMC1_D5    | UART6_RTS    | UART1_CTS    | —         | OSPI2_DQS  | ETH_MII_RXD1/ETH_RMII_RXD1      | COMP1_OUT   | ADC_INN4, ADC_INP8, WKUP5      |
| PC6      | TIM3_CH1     | SPI2_IO3     | SDMMC1_D6    | UART6_TX     | TIM8_CH1  | OSPI2_IO5  | SPI2_CS                         | I2S2_MCK    | —                              |
| PC7      | TIM3_CH2     | SPI2_IO2     | SDMMC1_D7    | UART6_RX     | TIM8_CH2  | OSPI2_IO6  | SPI2_SCK                        | I2S1_MCK    | —                              |
| PC8      | TIM3_CH3     | SPI2_CS3     | UART5_RTS    | UART6_CK     | TIM8_CH3  | —          | SPI2_MISO                       | SDMMC1_D0   | —                              |
| PC9      | TIM3_CH4     | SPI2_CS4     | UART5_CTS    | MCO2         | TIM8_CH4  | OSPI2_IO0  | SPI2_MOSI                       | SDMMC1_D1   | —                              |
| PC10     | TIM5_CH1     | SPI1_IO2     | SPI3_SCK     | UART3_TX     | UART4_TX  | OSPI2_IO1  | SDMMC1_D2                       | I2S1_CK     | —                              |
| PC11     | TIM5_CH2     | IR_OUT       | SPI3_MISO    | UART3_RX     | UART4_RX  | OSPI2_NCS  | SDMMC1_D3                       | I2S1_SDI    | —                              |
| PC12     | TIM5_CH3     | IR_OUT       | SPI3_MOSI    | UART3_CK     | UART5_TX  | OSPI2_NCLK | SDMMC1_CK                       | I2S1_SDO    | —                              |
| PC13     | —            | RTC_OUT      | —            | —            | SPI4_CS   | —          | —                               | —           | RTC_TAMP1, RTC_TS, WKUP3       |
| PC14     | —            | —            | —            | —            | —         | —          | —                               | —           | OSC32_IN                       |
| PC15     | —            | —            | —            | —            | —         | —          | —                               | —           | OSC32_OUT                      |
| PD0      | —            | —            | SPI3_IO3     | UART4_RX     | CAN1_RX   | OSPI2_IO0  | —                               | —           | OSC2_IN                        |
| PD1      | —            | —            | —            | UART4_TX     | CAN1_TX   | OSPI2_IO1  | —                               | —           | OSC2_OUT                       |
| PD2      | TIM5_CH4     | TIM3_ETR     | UART5_RX     | UART6_CK     | CAN1_STBY | OSPI2_IO2  | SPI4_IO2                        | SDMMC1_CMD  | —                              |
| PD3      | —            | —            | SPI2_SCK     | UART2_CTS    | —         | OSPI2_IO3  | —                               | I2S2_CK     | —                              |
| PD4      | —            | —            | —            | UART2_RTS_DE | —         | OSPI2_IO4  | —                               | I2S2_WS     | —                              |
| PD5      | —            | —            | —            | UART2_TX     | —         | OSPI2_IO5  | —                               | —           | —                              |
| PD6      | —            | —            | SPI3_MOSI    | UART2_RX     | —         | OSPI2_IO6  | —                               | —           | —                              |
| PD7      | —            | —            | SPI1_MOSI    | UART2_CK     | —         | OSPI2_IO7  | —                               | I2S1_SDO    | —                              |
| PD8      | —            | —            | —            | UART3_TX     | —         | UART8_CK   | —                               | UART7_RX    | —                              |
| PD9      | —            | —            | —            | UART3_RX     | —         | OSPI2_DQS  | —                               | UART7_TX    | —                              |
| PD10     | —            | —            | TIM1_CH4N    | UART6_TX     | —         | UART3_CK   | —                               | UART7_RTS   | —                              |
| PD11     | —            | CAN2_RX      | SPI4_CS3     | UART3_CTS    | TIM8_CH4N | OSPI2_IO0  | —                               | UART7_CTS   | —                              |
| PD12     | LPTIM1_IN1   | CAN2_TX      | TIM4_CH1     | UART3_RTS_DE | SPI4_CS4  | OSPI2_IO1  | —                               | UART7_CK    | —                              |
| PD13     | LPTIM1_OUT   | CAN2_STBY    | TIM4_CH2     | SPI3_IO2     | —         | OSPI2_IO3  | —                               | —           | —                              |
| PD14     | —            | UART8_CTS    | TIM4_CH3     | UART6_RX     | SPI4_IO3  | —          | —                               | —           | —                              |
| PD15     | —            | UART8_RTS    | TIM4_CH4     | SPI3_IO2     | —         | —          | —                               | —           | —                              |
| PE0      | LPTIM1_ETR   | UART8_RX     | TIM4_ETR     | SPI1_SCK     | —         | —          | —                               | I2C2_SDA    | —                              |
| PE1      | LPTIM1_IN2   | UART8_TX     | —            | SPI1_CS      | SPI4_IO2  | —          | —                               | I2C2_SCL    | —                              |
| PE2      | —            | UART8_CK     | —            | SPI1_CS3     | SPI4_SCK  | OSPI2_IO2  | ETH_MII_TXD3                    | —           | ANA_OUT                        |
| PE3      | LPUART2_TX   | TIM10_CH1    | SPI2_SCK     | SPI1_CS4     | SPI4_IO3  | —          | —                               | —           | —                              |
| PE4      | LPUART2_RX   | —            | —            | SPI3_IO3     | SPI4_CS   | —          | —                               | —           | —                              |
| PE5      | —            | CAN2_RX      | TIM9_CH1     | —            | SPI4_MISO | —          | —                               | —           | —                              |
| PE6      | —            | CAN2_TX      | TIM9_CH2     | —            | SPI4_MOSI | —          | —                               | UART7_CK    | WKUP2                          |
| PE7      | —            | TIM1_ETR     | —            | —            | —         | OSPI2_IO4  | UART4_CTS                       | UART7_RX    | —                              |
| PE8      | —            | TIM1_CH1N    | —            | —            | —         | OSPI2_IO5  | UART4_RTS                       | UART7_TX    | —                              |
| PE9      | —            | TIM1_CH1     | —            | SPI3_IO3     | —         | OSPI2_IO6  | UART4_TX                        | UART7_RTS   | —                              |
| PE10     | —            | TIM1_CH2N    | —            | SPI1_MISO    | —         | OSPI2_IO7  | UART4_RX                        | UART7_CTS   | —                              |
| PE11     | —            | TIM1_CH2     | —            | SPI1_MOSI    | SPI4_CS   | OSPI2_NCS  | —                               | —           | —                              |
| PE12     | —            | TIM1_CH3N    | —            | SPI1_SCK     | SPI4_SCK  | —          | —                               | COMP1_OUT   | —                              |
| PE13     | —            | TIM1_CH3     | —            | SPI1_CS      | SPI4_MISO | —          | —                               | —           | —                              |
| PE14     | —            | TIM1_CH4     | —            | SPI1_IO2     | SPI4_MOSI | —          | —                               | SDMMC1_CKIN | —                              |
| PE15     | —            | TIM1_BKIN    | TIM1_CH4N    | SPI1_IO3     | SPI4_IO2  | —          | —                               | SDMMC1_CDIR | —                              |
| PF0      | —            | —            | —            | SPI2_MOSI    | —         | —          | —                               | —           | —                              |
| PF1      | TIM2_CH1_ETR | —            | —            | —            | TIM8_CH1N | —          | OSPI2_IO0                       | SPI7_MOSI   | —                              |
| PF2      | —            | —            | —            | SPI2_IO2     | —         | —          | OSPI2_CLK                       | SPI7_SCK    | —                              |
| PF3      | TIM9_CH2     | —            | —            | —            | TIM8_CH2N | —          | OSPI2_IO3                       | SPI7_IO3    | —                              |
| PF4      | —            | —            | —            | SPI2_MISO    | —         | —          | —                               | —           | —                              |
| PF5      | —            | —            | —            | —            | —         | UART7_CK   | —                               | —           | —                              |
| PF6      | TIM9_CH1     | —            | —            | SPI2_CS      | —         | —          | —                               | —           | —                              |
| PF7      | TIM10_CH1    | —            | —            | —            | UART7_RTS | —          | OSPI2_IO2                       | SPI7_IO2    | —                              |
| PF8      | —            | —            | —            | SPI2_SCK     | —         | —          | —                               | —           | WKUP7                          |
| PF9      | TIM2_CH2     | —            | —            | —            | UART7_CTS | —          | —                               | —           | —                              |
| PF10     | —            | —            | —            | SPI2_IO3     | —         | —          | OSPI2_IO1                       | SPI7_MISO   | —                              |
| PF11     | —            | —            | —            | —            | I2C1_SCL  | —          | OSPI2_NCS                       | SPI7_CS     | —                              |
| PF12     | —            | —            | —            | —            | I2C1_SDA  | —          | —                               | —           | —                              |
| PF13     | —            | —            | —            | —            | UART7_RX  | —          | —                               | —           | —                              |
| PF14     | —            | —            | —            | —            | UART7_TX  | —          | —                               | —           | —                              |
| PF15     | —            | —            | —            | SPI3_IO3     | —         | —          | —                               | —           | —                              |
| PG0      | —            | —            | —            | SPI3_SCK     | —         | —          | —                               | —           | —                              |
| PG1      | —            | —            | —            | —            | TIM8_CH3N | —          | —                               | —           | —                              |
| PG2      | —            | —            | —            | SPI3_MOSI    | —         | —          | —                               | —           | —                              |
| PG3      | —            | —            | —            | —            | UART5_RTS | —          | —                               | —           | —                              |
| PG4      | —            | OSPI1_IO0    | OSPI1_NCS    | SPI3_CS      | —         | —          | OSPI1_NCLK                      | OSPI1_DQS   | —                              |
| PG5      | —            | OSPI1_IO1    | OSPI1_IO0    | —            | UART5_CTS | OSPI1_NCS  | OSPI1_DQS                       | OSPI1_IO7   | —                              |
| PG6      | —            | OSPI1_IO2    | OSPI1_IO1    | SPI3_MISO    | —         | OSPI1_CLK  | OSPI1_IO7                       | OSPI1_IO6   | —                              |
| PG7      | —            | OSPI1_IO3    | OSPI1_IO2    | —            | UART5_CK  | OSPI1_DQS  | OSPI1_IO6                       | OSPI1_IO5   | —                              |
| PG8      | —            | OSPI1_IO1    | OSPI1_IO3    | SPI3_IO2     | —         | OSPI1_IO7  | OSPI1_IO5                       | OSPI1_IO4   | —                              |
| PG9      | —            | OSPI1_IO5    | OSPI1_NCS    | —            | UART5_RX  | OSPI1_IO6  | OSPI1_IO4                       | OSPI1_CLK   | —                              |
| PG10     | I2C2_SCL     | OSPI1_IO6    | —            | —            | TIM8_CH1  | OSPI1_IO5  | OSPI1_CLK                       | OSPI1_NCS   | —                              |
| PG11     | I2C2_SDA     | OSPI1_IO7    | —            | —            | TIM8_CH2  | OSPI1_IO4  | OSPI1_NCS                       | OSPI1_IO3   | —                              |
| PG12     | —            | OSPI1_DQS    | OSPI1_IO5    | —            | TIM8_CH3  | OSPI1_IO3  | OSPI1_IO1                       | OSPI1_IO2   | —                              |
| PG13     | —            | OSPI1_CLK    | OSPI1_IO6    | —            | TIM8_CH4  | OSPI1_IO2  | —                               | OSPI1_IO1   | —                              |
| PG14     | —            | OSPI1_NCS    | OSPI1_IO7    | —            | UART5_TX  | OSPI1_IO1  | —                               | OSPI1_IO0   | —                              |
| PG15     | TIM2_CH3     | OSPI1_IO3    | OSPI1_DQS    | —            | TIM8_BKIN | OSPI1_IO0  | —                               | —           | —                              |
| PH0      | —            | —            | —            | —            | —         | —          | —                               | —           | OSC_IN                         |
| PH1      | —            | —            | —            | —            | —         | —          | —                               | —           | OSC_OUT                        |
| PH2      | —            | OSPI2_IO3    | SPI7_IO3     | —            | —         | —          | OSPI2_IO0                       | SPI7_MOSI   | —                              |
| PH3      | —            | OSPI2_IO1    | SPI7_MISO    | —            | —         | —          | OSPI2_CLK                       | SPI7_SCK    | —                              |
| PH4      | —            | OSPI2_IO0    | SPI7_MOSI    | —            | —         | —          | OSPI2_IO3                       | SPI7_IO3    | —                              |
| PH5      | —            | OSPI2_NCS    | SPI7_CS      | —            | —         | —          | OSPI2_IO2                       | SPI7_IO2    | —                              |
| PH6      | —            | OSPI2_CLK    | SPI7_SCK     | —            | —         | —          | OSPI2_IO1                       | SPI7_MISO   | —                              |
| PH7      | —            | OSPI2_IO2    | SPI7_IO2     | —            | —         | —          | OSPI2_NCS                       | SPI7_CS     | —                              |
*/





#ifdef RT_USING_PIN

#define __ACM32_PIN(index, gpio, gpio_index)                                \
    {                                                                       \
        index, GPIO##gpio, GPIO_PIN_##gpio_index                            \
    }

#define __ACM32_PIN_RESERVE                                                 \
    {                                                                       \
        -1, RT_NULL, 0                                                      \
    }

/* ACM32 GPIO driver */
struct pin_index
{
    int             index;
    GPIO_TypeDef    *gpio;
    uint32_t        pin;
};

struct pin_irq_map
{
    rt_uint16_t     line;
    GPIO_TypeDef    *gpio;
};

static const struct pin_index pins[] =
{
#if defined(BSP_USING_GPIO1)
    __ACM32_PIN(0,   A, 0),
    __ACM32_PIN(1,   A, 1),
    __ACM32_PIN(2,   A, 2),
    __ACM32_PIN(3,   A, 3),
    __ACM32_PIN(4,   A, 4),
    __ACM32_PIN(5,   A, 5),
    __ACM32_PIN(6,   A, 6),
    __ACM32_PIN(7,   A, 7),
    __ACM32_PIN(8,   A, 8),
    __ACM32_PIN(9,   A, 9),
    __ACM32_PIN(10,  A, 10),
    __ACM32_PIN(11,  A, 11),
    __ACM32_PIN(12,  A, 12),
    __ACM32_PIN(13,  A, 13),
    __ACM32_PIN(14,  A, 14),
    __ACM32_PIN(15,  A, 15),
    __ACM32_PIN(16,  B, 0),
    __ACM32_PIN(17,  B, 1),
    __ACM32_PIN(18,  B, 2),
    __ACM32_PIN(19,  B, 3),
    __ACM32_PIN(20,  B, 4),
    __ACM32_PIN(21,  B, 5),
    __ACM32_PIN(22,  B, 6),
    __ACM32_PIN(23,  B, 7),
    __ACM32_PIN(24,  B, 8),
    __ACM32_PIN(25,  B, 9),
    __ACM32_PIN(26,  B, 10),
    __ACM32_PIN(27,  B, 11),
    __ACM32_PIN(28,  B, 12),
    __ACM32_PIN(29,  B, 13),
    __ACM32_PIN(30,  B, 14),
    __ACM32_PIN(31,  B, 15),
    __ACM32_PIN(32,  C, 0),
    __ACM32_PIN(33,  C, 1),
    __ACM32_PIN(34,  C, 2),
    __ACM32_PIN(35,  C, 3),
    __ACM32_PIN(36,  C, 4),
    __ACM32_PIN(37,  C, 5),
    __ACM32_PIN(38,  C, 6),
    __ACM32_PIN(39,  C, 7),
    __ACM32_PIN(40,  C, 8),
    __ACM32_PIN(41,  C, 9),
    __ACM32_PIN(42,  C, 10),
    __ACM32_PIN(43,  C, 11),
    __ACM32_PIN(44,  C, 12),
    __ACM32_PIN(45,  C, 13),
    __ACM32_PIN(46,  C, 14),
    __ACM32_PIN(47,  C, 15),
    __ACM32_PIN(48,  D, 0),
    __ACM32_PIN(49,  D, 1),
    __ACM32_PIN(50,  D, 2),
    __ACM32_PIN(51,  D, 3),
    __ACM32_PIN(52,  D, 4),
    __ACM32_PIN(53,  D, 5),
    __ACM32_PIN(54,  D, 6),
    __ACM32_PIN(55,  D, 7),
    __ACM32_PIN(56,  D, 8),
    __ACM32_PIN(57,  D, 9),
    __ACM32_PIN(58,  D, 10),
    __ACM32_PIN(59,  D, 11),
    __ACM32_PIN(60,  D, 12),
    __ACM32_PIN(61,  D, 13),
    __ACM32_PIN(62,  D, 14),
    __ACM32_PIN(63,  D, 15),
    __ACM32_PIN(64,  E, 0),
    __ACM32_PIN(65,  E, 1),
    __ACM32_PIN(66,  E, 2),
    __ACM32_PIN(67,  E, 3),
    __ACM32_PIN(68,  E, 4),
    __ACM32_PIN(69,  E, 5),
    __ACM32_PIN(70,  E, 6),
    __ACM32_PIN(71,  E, 7),
    __ACM32_PIN(72,  E, 8),
    __ACM32_PIN(73,  E, 9),
    __ACM32_PIN(74,  E, 10),
    __ACM32_PIN(75,  E, 11),
    __ACM32_PIN(76,  E, 12),
    __ACM32_PIN(77,  E, 13),
    __ACM32_PIN(78,  E, 14),
    __ACM32_PIN(79,  E, 15),
#endif /* defined(BSP_USING_GPIO1) */
};

static struct pin_irq_map pin_irq_map[] =
{
    {EXTI_LINE_0,  RT_NULL},
    {EXTI_LINE_1,  RT_NULL},
    {EXTI_LINE_2,  RT_NULL},
    {EXTI_LINE_3,  RT_NULL},
    {EXTI_LINE_4,  RT_NULL},
    {EXTI_LINE_5,  RT_NULL},
    {EXTI_LINE_6,  RT_NULL},
    {EXTI_LINE_7,  RT_NULL},
    {EXTI_LINE_8,  RT_NULL},
    {EXTI_LINE_9,  RT_NULL},
    {EXTI_LINE_10, RT_NULL},
    {EXTI_LINE_11, RT_NULL},
    {EXTI_LINE_12, RT_NULL},
    {EXTI_LINE_13, RT_NULL},
    {EXTI_LINE_14, RT_NULL},
    {EXTI_LINE_15, RT_NULL},
};

static volatile struct rt_pin_irq_hdr pin_irq_hdr_tab[] =
{
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
};
static uint32_t pin_irq_enable_mask = 0;

#define ITEM_NUM(items) sizeof(items) / sizeof(items[0])
static const struct pin_index *get_pin(uint8_t pin)
{
    const struct pin_index *index;

    if (pin < ITEM_NUM(pins))
    {
        index = &pins[pin];
        if (index->index == -1)
            index = RT_NULL;
    }
    else
    {
        index = RT_NULL;
    }

    return index;
};

static void _pin_write(rt_device_t dev, rt_base_t pin, rt_uint8_t value)
{
    const struct pin_index *index;

    index = get_pin(pin);
    if (index == RT_NULL)
    {
        return;
    }

    HAL_GPIO_WritePin(index->gpio, index->pin, (GPIO_PinState)value);
}

static rt_ssize_t _pin_read(rt_device_t dev, rt_base_t pin)
{
    int value;
    const struct pin_index *index;

    value = PIN_LOW;

    index = get_pin(pin);
    if (index == RT_NULL)
    {
        return -RT_EINVAL;
    }

    value = HAL_GPIO_ReadPin(index->gpio, index->pin);

    return value;
}

static void _pin_mode(rt_device_t dev, rt_base_t pin, rt_uint8_t mode)
{
    const struct pin_index *index;
    GPIO_InitTypeDef GPIO_InitStruct;

    index = get_pin(pin);
    if (index == RT_NULL)
    {
        return;
    }

    /* Enable GPIO clock */
    if (index->gpio == GPIOA)
        __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (index->gpio == GPIOB)
        __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (index->gpio == GPIOC)
        __HAL_RCC_GPIOC_CLK_ENABLE();
    else if (index->gpio == GPIOD)
        __HAL_RCC_GPIOD_CLK_ENABLE();
    else if (index->gpio == GPIOE)
        __HAL_RCC_GPIOE_CLK_ENABLE();
    else if (index->gpio == GPIOF)
        __HAL_RCC_GPIOF_CLK_ENABLE();
    else if (index->gpio == GPIOG)
        __HAL_RCC_GPIOG_CLK_ENABLE();
    else if (index->gpio == GPIOH)
        __HAL_RCC_GPIOH_CLK_ENABLE();

    /* Configure GPIO_InitStructure */
    GPIO_InitStruct.Pin = index->pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Drive = GPIO_DRIVE_LEVEL3;
    GPIO_InitStruct.Alternate = GPIO_FUNCTION_0;

    if (mode == PIN_MODE_OUTPUT)
    {
        /* output setting */
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
    }
    else if (mode == PIN_MODE_INPUT)
    {
        /* input setting: not pull. */
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
    }
    else if (mode == PIN_MODE_INPUT_PULLUP)
    {
        /* input setting: pull up. */
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
    }
    else if (mode == PIN_MODE_INPUT_PULLDOWN)
    {
        /* input setting: pull down. */
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    }
    else if (mode == PIN_MODE_OUTPUT_OD)
    {
        /* output setting: od. */
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
    }

    HAL_GPIO_Init(index->gpio, &GPIO_InitStruct);
}

#define     PIN2INDEX(pin)      ((pin) % 16)

static rt_err_t _pin_attach_irq(struct rt_device *device, rt_base_t pin,
                                     rt_uint8_t mode, void (*hdr)(void *args), void *args)
{
    const struct pin_index *index;
    rt_base_t level;
    rt_int32_t irqindex = -1;

    index = get_pin(pin);
    if (index == RT_NULL)
    {
        return -RT_EINVAL;
    }

    irqindex = PIN2INDEX(pin);

    level = rt_hw_interrupt_disable();
    if (pin_irq_hdr_tab[irqindex].pin == pin &&
            pin_irq_hdr_tab[irqindex].hdr == hdr &&
            pin_irq_hdr_tab[irqindex].mode == mode &&
            pin_irq_hdr_tab[irqindex].args == args)
    {
        rt_hw_interrupt_enable(level);
        return RT_EOK;
    }

    if (pin_irq_hdr_tab[irqindex].pin != -1)
    {
        rt_hw_interrupt_enable(level);
        return -RT_EBUSY;
    }

    pin_irq_hdr_tab[irqindex].pin = pin;
    pin_irq_hdr_tab[irqindex].hdr = hdr;
    pin_irq_hdr_tab[irqindex].mode = mode;
    pin_irq_hdr_tab[irqindex].args = args;
    rt_hw_interrupt_enable(level);

    return RT_EOK;
}

static rt_err_t _pin_dettach_irq(struct rt_device *device, rt_base_t pin)
{
    const struct pin_index *index;
    rt_base_t level;
    rt_int32_t irqindex = -1;

    index = get_pin(pin);
    if (index == RT_NULL)
    {
        return -RT_EINVAL;
    }

    irqindex = PIN2INDEX(pin);

    level = rt_hw_interrupt_disable();
    if (pin_irq_hdr_tab[irqindex].pin == -1)
    {
        rt_hw_interrupt_enable(level);
        return RT_EOK;
    }
    pin_irq_hdr_tab[irqindex].pin = -1;
    pin_irq_hdr_tab[irqindex].hdr = RT_NULL;
    pin_irq_hdr_tab[irqindex].mode = 0;
    pin_irq_hdr_tab[irqindex].args = RT_NULL;
    rt_hw_interrupt_enable(level);

    return RT_EOK;
}

static rt_err_t _pin_irq_enable(struct rt_device *device, rt_base_t pin,
                                     rt_uint8_t enabled)
{
    const struct pin_index *index;
    rt_base_t level;
    rt_int32_t irqindex = -1;
    uint32_t exti_mode;

    index = get_pin(pin);
    if (index == RT_NULL)
    {
        return -RT_EINVAL;
    }

    irqindex = PIN2INDEX(pin);

    if (enabled == PIN_IRQ_ENABLE)
    {
        level = rt_hw_interrupt_disable();

        if (pin_irq_hdr_tab[irqindex].pin == -1)
        {
            rt_hw_interrupt_enable(level);
            return -RT_EPERM;  /* 未 attach 中断回调 */
        }

        pin_irq_map[irqindex].gpio = index->gpio;

        switch (pin_irq_hdr_tab[irqindex].mode)
        {
        case PIN_IRQ_MODE_RISING:
            exti_mode = EXTI_MODE_IT_RISING;
            break;
        case PIN_IRQ_MODE_FALLING:
            exti_mode = EXTI_MODE_IT_FALLING;
            break;
        case PIN_IRQ_MODE_RISING_FALLING:
            exti_mode = EXTI_MODE_IT_RISING_FALLING;
            break;
        default:
            rt_hw_interrupt_enable(level);
            return -RT_EINVAL;
        }

        HAL_EXTI_SetConfigLine(index->gpio, pin_irq_map[irqindex].line, exti_mode);

        pin_irq_enable_mask |= 1 << irqindex;

        /* 使能对应 EXTI NVIC 通道 */
        if (irqindex <= 4)
            NVIC_EnableIRQ((IRQn_Type)(EXTI0_IRQn + irqindex));
        else if (irqindex <= 9)
            NVIC_EnableIRQ(EXTI9_5_IRQn);
        else
            NVIC_EnableIRQ(EXTI15_10_IRQn);

        rt_hw_interrupt_enable(level);
    }
    else if (enabled == PIN_IRQ_DISABLE)
    {
        if ((pin_irq_enable_mask & (1 << irqindex)) == 0)
        {
            return -RT_EPERM;  /* 中断未使能，不能禁用 */
        }

        level = rt_hw_interrupt_disable();

        HAL_EXTI_ClearConfigLine(pin_irq_map[irqindex].line);

        pin_irq_enable_mask &= ~(1 << irqindex);

        /* 若该共享 IRQ 组内无其他使能的线，禁能 NVIC */
        if (irqindex <= 4)
        {
            NVIC_DisableIRQ((IRQn_Type)(EXTI0_IRQn + irqindex));
        }
        else if (irqindex <= 9)
        {
            if ((pin_irq_enable_mask & 0x03E0) == 0)  /* bit 5-9 */
                NVIC_DisableIRQ(EXTI9_5_IRQn);
        }
        else
        {
            if ((pin_irq_enable_mask & 0xFC00) == 0)  /* bit 10-15 */
                NVIC_DisableIRQ(EXTI15_10_IRQn);
        }

        rt_hw_interrupt_enable(level);
    }
    else
    {
        return -RT_EINVAL;
    }

    return RT_EOK;
}

const static struct rt_pin_ops _acm32_pin_ops =
{
    _pin_mode,
    _pin_write,
    _pin_read,
    _pin_attach_irq,
    _pin_dettach_irq,
    _pin_irq_enable,
    RT_NULL,
};

int rt_hw_pin_init(void)
{
    __HAL_RCC_EXTI_CLK_ENABLE();

    rt_device_pin_register("pin", &_acm32_pin_ops, RT_NULL);

    return 0;
}
INIT_BOARD_EXPORT(rt_hw_pin_init);

/* EXTI IRQ handlers for line 0-4 (individual) */
#if defined(EXTI0_IRQn)
void EXTI0_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_EXTI_IRQHandler(EXTI_LINE_0);
    if (pin_irq_hdr_tab[0].hdr)
    {
        pin_irq_hdr_tab[0].hdr(pin_irq_hdr_tab[0].args);
    }
    rt_interrupt_leave();
}
#endif

#if defined(EXTI1_IRQn)
void EXTI1_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_EXTI_IRQHandler(EXTI_LINE_1);
    if (pin_irq_hdr_tab[1].hdr)
    {
        pin_irq_hdr_tab[1].hdr(pin_irq_hdr_tab[1].args);
    }
    rt_interrupt_leave();
}
#endif

#if defined(EXTI2_IRQn)
void EXTI2_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_EXTI_IRQHandler(EXTI_LINE_2);
    if (pin_irq_hdr_tab[2].hdr)
    {
        pin_irq_hdr_tab[2].hdr(pin_irq_hdr_tab[2].args);
    }
    rt_interrupt_leave();
}
#endif

#if defined(EXTI3_IRQn)
void EXTI3_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_EXTI_IRQHandler(EXTI_LINE_3);
    if (pin_irq_hdr_tab[3].hdr)
    {
        pin_irq_hdr_tab[3].hdr(pin_irq_hdr_tab[3].args);
    }
    rt_interrupt_leave();
}
#endif

#if defined(EXTI4_IRQn)
void EXTI4_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_EXTI_IRQHandler(EXTI_LINE_4);
    if (pin_irq_hdr_tab[4].hdr)
    {
        pin_irq_hdr_tab[4].hdr(pin_irq_hdr_tab[4].args);
    }
    rt_interrupt_leave();
}
#endif

/* EXTI IRQ handler for lines 5-9 */
#if defined(EXTI9_5_IRQn)
void EXTI9_5_IRQHandler(void)
{
    rt_interrupt_enter();
    uint32_t pdr = EXTI->PDR;  /* 在 HAL 清除前捕获 PDR 状态 */
    HAL_EXTI_IRQHandler(EXTI_LINE_5);
    HAL_EXTI_IRQHandler(EXTI_LINE_6);
    HAL_EXTI_IRQHandler(EXTI_LINE_7);
    HAL_EXTI_IRQHandler(EXTI_LINE_8);
    HAL_EXTI_IRQHandler(EXTI_LINE_9);
    if ((pdr & (1UL << 5)) && pin_irq_hdr_tab[5].hdr)  pin_irq_hdr_tab[5].hdr(pin_irq_hdr_tab[5].args);
    if ((pdr & (1UL << 6)) && pin_irq_hdr_tab[6].hdr)  pin_irq_hdr_tab[6].hdr(pin_irq_hdr_tab[6].args);
    if ((pdr & (1UL << 7)) && pin_irq_hdr_tab[7].hdr)  pin_irq_hdr_tab[7].hdr(pin_irq_hdr_tab[7].args);
    if ((pdr & (1UL << 8)) && pin_irq_hdr_tab[8].hdr)  pin_irq_hdr_tab[8].hdr(pin_irq_hdr_tab[8].args);
    if ((pdr & (1UL << 9)) && pin_irq_hdr_tab[9].hdr)  pin_irq_hdr_tab[9].hdr(pin_irq_hdr_tab[9].args);
    rt_interrupt_leave();
}
#endif

/* EXTI IRQ handler for lines 10-15 */
#if defined(EXTI15_10_IRQn)
void EXTI15_10_IRQHandler(void)
{
    rt_interrupt_enter();
    uint32_t pdr = EXTI->PDR;  /* 在 HAL 清除前捕获 PDR 状态 */
    HAL_EXTI_IRQHandler(EXTI_LINE_10);
    HAL_EXTI_IRQHandler(EXTI_LINE_11);
    HAL_EXTI_IRQHandler(EXTI_LINE_12);
    HAL_EXTI_IRQHandler(EXTI_LINE_13);
    HAL_EXTI_IRQHandler(EXTI_LINE_14);
    HAL_EXTI_IRQHandler(EXTI_LINE_15);
    if ((pdr & (1UL << 10)) && pin_irq_hdr_tab[10].hdr) pin_irq_hdr_tab[10].hdr(pin_irq_hdr_tab[10].args);
    if ((pdr & (1UL << 11)) && pin_irq_hdr_tab[11].hdr) pin_irq_hdr_tab[11].hdr(pin_irq_hdr_tab[11].args);
    if ((pdr & (1UL << 12)) && pin_irq_hdr_tab[12].hdr) pin_irq_hdr_tab[12].hdr(pin_irq_hdr_tab[12].args);
    if ((pdr & (1UL << 13)) && pin_irq_hdr_tab[13].hdr) pin_irq_hdr_tab[13].hdr(pin_irq_hdr_tab[13].args);
    if ((pdr & (1UL << 14)) && pin_irq_hdr_tab[14].hdr) pin_irq_hdr_tab[14].hdr(pin_irq_hdr_tab[14].args);
    if ((pdr & (1UL << 15)) && pin_irq_hdr_tab[15].hdr) pin_irq_hdr_tab[15].hdr(pin_irq_hdr_tab[15].args);
    rt_interrupt_leave();
}
#endif

#endif /* RT_USING_PIN */
