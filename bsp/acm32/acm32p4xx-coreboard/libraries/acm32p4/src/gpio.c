/**
 * @file gpio.c
 * @brief ACM32P4 GPIO驱动实现
 *
 * 本文件实现了ACM32P4芯片的GPIO驱动功能，提供了完整的引脚控制接口。
 *
 * 主要功能：
 * - 基础GPIO输入/输出操作
 * - 复用功能配置
 * - 输出类型和驱动能力配置
 * - 模拟模式配置
 * - 引脚锁定保护
 * - 端口批量操作
 * - 引脚状态查询
 *
 * @author ACM32 SDK Team
 * @version 1.0
 */

#include <assert.h>
#include "device/acm32p4xx.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"

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

//===========================================
// 内部辅助函数
//===========================================

/**
 * @brief 从引脚编号获取GPIO端口基地址
 * @param pin GPIO引脚编号
 * @return GPIO端口寄存器基地址指针
 */
static inline GPIO_TypeDef *gpio_get_port_base(gpio_pin_t pin)
{
    uint8_t port = pin / 16;
    const uint32_t port_bases[] = {
        GPIOA_BASE, GPIOB_BASE, GPIOC_BASE, GPIOD_BASE,
        GPIOE_BASE, GPIOF_BASE, GPIOG_BASE, GPIOH_BASE};
    return (GPIO_TypeDef *)port_bases[port];
}

/**
 * @brief 从引脚编号提取引脚号
 * @param pin GPIO引脚编号
 * @return 引脚号 (0-15)
 */
static inline uint8_t gpio_get_pin_num(gpio_pin_t pin)
{
    return pin % 16;
}

/**
 * @brief 修改寄存器中特定引脚的位域
 * @param reg 寄存器地址指针
 * @param pin_num 引脚号 (0-15)
 * @param bits_per_pin 每个引脚占用的位数
 * @param value 要写入的值
 */
static inline void gpio_modify_register(volatile uint32_t *reg,
                                        uint8_t pin_num,
                                        uint8_t bits_per_pin,
                                        uint32_t value)
{
    uint32_t shift = pin_num * bits_per_pin;
    uint32_t mask = ((1U << bits_per_pin) - 1);
    uint32_t temp = *reg;
    temp &= ~(mask << shift);
    temp |= (value << shift);
    *reg = temp;
}

//===========================================
// 第1层：快速初始化API
//===========================================

/**
 * @brief 初始化GPIO引脚为默认配置
 *
 * 将引脚配置为输入模式，浮空，禁用施密特触发器。
 * 自动使能对应GPIO端口的时钟。
 *
 * @param pin GPIO引脚编号
 *
 * @note 自动使能对应GPIO端口的时钟
 * @note 覆盖寄存器：GPIOx_MD, GPIOx_PUPD, GPIOx_SMIT, GPIOx_OTYP, GPIOx_DS0/DS1, GPIOx_AF0/AF1, GPIOx_AIEN
 *
 * 使用示例：
 * @code
 * // 初始化PA5引脚
 * gpio_init(PA5);
 *
 * // 初始化多个引脚
 * gpio_init(PB0);
 * gpio_init(PC13);
 * @endcode
 */
void gpio_init(gpio_pin_t pin)
{
    assert(GPIO_PIN_VALID(pin));

    // 1. 自动使能GPIO时钟
    uint8_t port = pin / 16;
    clock_periph_enable(CLK_GPIOA + port);

    // 2. 获取端口和引脚号
    GPIO_TypeDef *gpio = gpio_get_port_base(pin);
    uint8_t pin_num = gpio_get_pin_num(pin);

    // 3. 配置为输入模式
    gpio_modify_register(&gpio->MD, pin_num, 2, GPIO_MODE_INPUT);

    // 4. 禁用上下拉（浮空）
    gpio_modify_register(&gpio->PUPD, pin_num, 2, GPIO_PULL_NONE);

    // 5. 禁用施密特触发器
    gpio->SMIT &= ~(1U << pin_num);

    // 6. 复位输出类型为推挽
    gpio->OTYP &= ~(1U << pin_num);

    // 7. 复位驱动能力
    if (pin_num < 8) {
        gpio_modify_register(&gpio->DS0, pin_num, 4, GPIO_DRIVE_2MA);
    } else {
        gpio_modify_register(&gpio->DS1, pin_num - 8, 4, GPIO_DRIVE_2MA);
    }

    // 8. 复位复用功能
    if (pin_num < 8) {
        gpio_modify_register(&gpio->AF0, pin_num, 4, 0);
    } else {
        gpio_modify_register(&gpio->AF1, pin_num - 8, 4, 0);
    }

    // 9. 禁能模拟开关
    gpio->AIEN &= ~(1U << pin_num);
}

/**
 * @brief 设置GPIO引脚方向
 *
 * 配置引脚为输入或输出模式。输出模式默认配置为推挽输出、2mA驱动能力。
 *
 * @param pin GPIO引脚编号
 * @param dir 方向：GPIO_IN(输入) 或 GPIO_OUT(输出)
 *
 * @note 输出模式默认配置为推挽输出、2mA驱动能力
 * @note 覆盖寄存器：GPIOx_MD, GPIOx_OTYP, GPIOx_DS0/DS1
 *
 * 使用示例：
 * @code
 * // 配置PA5为输出（LED）
 * gpio_init(PA5);
 * gpio_set_dir(PA5, GPIO_OUT);
 *
 * // 配置PC13为输入（按键）
 * gpio_init(PC13);
 * gpio_set_dir(PC13, GPIO_IN);
 * @endcode
 */
void gpio_set_dir(gpio_pin_t pin, gpio_dir_t dir)
{
    assert(GPIO_PIN_VALID(pin));

    GPIO_TypeDef *gpio = gpio_get_port_base(pin);
    uint8_t pin_num = gpio_get_pin_num(pin);

    if (dir == GPIO_OUT) {
        // 配置为输出模式
        gpio_modify_register(&gpio->MD, pin_num, 2, GPIO_MODE_OUTPUT);

        // 默认配置：推挽输出
        gpio->OTYP &= ~(1U << pin_num);

        // 默认驱动能力：2mA（PIN0-7→DS0，PIN8-15→DS1）
        if (pin_num < 8) {
            gpio_modify_register(&gpio->DS0, pin_num, 4, GPIO_DRIVE_2MA);
        } else {
            gpio_modify_register(&gpio->DS1, pin_num - 8, 4, GPIO_DRIVE_2MA);
        }
    } else {
        // 配置为输入模式
        gpio_modify_register(&gpio->MD, pin_num, 2, GPIO_MODE_INPUT);
    }
}

/**
 * @brief 设置GPIO引脚输出电平
 *
 * 使用BSC寄存器实现原子操作，适合在中断中使用。
 *
 * @param pin GPIO引脚编号
 * @param value 输出电平：true=高电平，false=低电平
 *
 * @note 使用BSC寄存器实现原子操作
 * @note 覆盖寄存器：GPIOx_BSC
 *
 * 使用示例：
 * @code
 * // 点亮LED
 * gpio_put(PA5, true);
 *
 * // 熄灭LED
 * gpio_put(PA5, false);
 *
 * // LED闪烁
 * for (int i = 0; i < 10; i++) {
 *     gpio_put(PA5, true);
 *     delay_ms(500);
 *     gpio_put(PA5, false);
 *     delay_ms(500);
 * }
 * @endcode
 */
void gpio_put(gpio_pin_t pin, bool value)
{
    assert(GPIO_PIN_VALID(pin));

    GPIO_TypeDef *gpio = gpio_get_port_base(pin);
    uint8_t pin_num = gpio_get_pin_num(pin);

    // 使用BSC寄存器实现原子操作
    if (value) {
        gpio->BSC = (1U << pin_num); // 置位（低16位）
    } else {
        gpio->BSC = (1U << (pin_num + 16)); // 清零（高16位）
    }
}

/**
 * @brief 读取GPIO引脚输入电平
 *
 * 读取引脚的当前电平状态。
 *
 * @param pin GPIO引脚编号
 * @return 引脚电平：true=高电平，false=低电平
 *
 * 使用示例：
 * @code
 * // 读取按键状态
 * if (gpio_get(PC13)) {
 *     printf("Button pressed\n");
 * }
 *
 * // 等待按键按下
 * while (!gpio_get(PC13)) {
 *     // 等待
 * }
 * @endcode
 */
bool gpio_get(gpio_pin_t pin)
{
    assert(GPIO_PIN_VALID(pin));

    GPIO_TypeDef *gpio = gpio_get_port_base(pin);
    uint8_t pin_num = gpio_get_pin_num(pin);

    return (gpio->IDATA & (1U << pin_num)) != 0;
}

/**
 * @brief 翻转GPIO引脚输出电平
 *
 * 使用 BSC 寄存器置位/清零，单次写操作原子；但内部需读 ODATA 判断当前电平，
 * 非严格原子，不建议中断与主循环并发操作同一端口的其他引脚。
 *
 * @param pin GPIO引脚编号
 *
 * @note BSC 写操作原子，读-改-写过程非原子
 * @note 覆盖寄存器：GPIOx_BSC
 *
 * 使用示例：
 * @code
 * // LED闪烁（使用翻转）
 * for (int i = 0; i < 20; i++) {
 *     gpio_toggle(PA5);
 *     delay_ms(500);
 * }
 *
 * // 在定时器中断中翻转LED
 * void TIM1_IRQHandler(void) {
 *     gpio_toggle(PA5);
 * }
 * @endcode
 */
void gpio_toggle(gpio_pin_t pin)
{
    assert(GPIO_PIN_VALID(pin));

    GPIO_TypeDef *gpio = gpio_get_port_base(pin);
    uint8_t pin_num = gpio_get_pin_num(pin);

    // BSC 寄存器写操作是原子的；但内部读 ODATA 再判断的过程非原子，
    // 不建议在中断与主循环并发操作同一端口的其他引脚。
    uint32_t odata = gpio->ODATA;
    if (odata & (1U << pin_num)) {
        gpio->BSC = (1U << (pin_num + 16)); // 清零
    } else {
        gpio->BSC = (1U << pin_num); // 置位
    }
}

/**
 * @brief 使能GPIO引脚上拉电阻
 *
 * 通常用于按键输入、I2C等应用。
 *
 * @param pin GPIO引脚编号
 *
 * 使用示例：
 * @code
 * // 配置按键输入（上拉）
 * gpio_init(PC13);
 * gpio_set_dir(PC13, GPIO_IN);
 * gpio_pull_up(PC13);
 *
 * // I2C引脚配置
 * gpio_pull_up(PB6);  // SCL
 * gpio_pull_up(PB7);  // SDA
 * @endcode
 */
void gpio_pull_up(gpio_pin_t pin)
{
    assert(GPIO_PIN_VALID(pin));

    GPIO_TypeDef *gpio = gpio_get_port_base(pin);
    uint8_t pin_num = gpio_get_pin_num(pin);

    gpio_modify_register(&gpio->PUPD, pin_num, 2, GPIO_PULL_UP);
}

/**
 * @brief 使能GPIO引脚下拉电阻
 *
 * 通常用于某些特殊的输入应用。
 *
 * @param pin GPIO引脚编号
 *
 * 使用示例：
 * @code
 * // 配置下拉输入
 * gpio_init(PA0);
 * gpio_set_dir(PA0, GPIO_IN);
 * gpio_pull_down(PA0);
 * @endcode
 */
void gpio_pull_down(gpio_pin_t pin)
{
    assert(GPIO_PIN_VALID(pin));

    GPIO_TypeDef *gpio = gpio_get_port_base(pin);
    uint8_t pin_num = gpio_get_pin_num(pin);

    gpio_modify_register(&gpio->PUPD, pin_num, 2, GPIO_PULL_DOWN);
}

/**
 * @brief 禁用GPIO引脚上下拉电阻(浮空模式)
 *
 * 将引脚配置为高阻态（浮空）。
 *
 * @param pin GPIO引脚编号
 *
 * 使用示例：
 * @code
 * // 配置浮空输入
 * gpio_init(PA1);
 * gpio_set_dir(PA1, GPIO_IN);
 * gpio_disable_pulls(PA1);
 * @endcode
 */
void gpio_disable_pulls(gpio_pin_t pin)
{
    assert(GPIO_PIN_VALID(pin));

    GPIO_TypeDef *gpio = gpio_get_port_base(pin);
    uint8_t pin_num = gpio_get_pin_num(pin);

    gpio_modify_register(&gpio->PUPD, pin_num, 2, GPIO_PULL_NONE);
}

/**
 * @brief 设置GPIO引脚上下拉配置
 *
 * 统一接口设置上下拉模式。
 *
 * @param pin GPIO引脚编号
 * @param pull 上下拉模式：GPIO_PULL_NONE(浮空)、GPIO_PULL_UP(上拉)、GPIO_PULL_DOWN(下拉)
 *
 * 使用示例：
 * @code
 * // 根据需要动态切换上下拉
 * gpio_set_pulls(PA0, GPIO_PULL_UP);
 *
 * // 禁用上下拉
 * gpio_set_pulls(PA0, GPIO_PULL_NONE);
 * @endcode
 */
void gpio_set_pulls(gpio_pin_t pin, gpio_pull_t pull)
{
    assert(GPIO_PIN_VALID(pin));

    GPIO_TypeDef *gpio = gpio_get_port_base(pin);
    uint8_t pin_num = gpio_get_pin_num(pin);

    gpio_modify_register(&gpio->PUPD, pin_num, 2, pull);
}

//===========================================
// 第2层：基础配置API
//===========================================
/**
 * @brief 设置GPIO引脚复用功能
 *
 * 将引脚配置为复用功能模式，用于UART、SPI、I2C等外设。
 * 自动配置为复用功能模式，默认推挽输出、无上下拉、2mA驱动。
 *
 * @param pin GPIO引脚编号
 * @param af 复用功能编号 (0-15)
 *
 * @note 自动配置为复用功能模式，默认推挽输出、无上下拉、2mA驱动
 * @note 覆盖寄存器：GPIOx_MD, GPIOx_AF0/AF1, GPIOx_OTYP, GPIOx_PUPD, GPIOx_DS0/DS1
 *
 * 使用示例：
 * @code
 * // 配置UART1引脚 (PA9=TX, PA10=RX)
 * gpio_set_function(PA9, 7);   // USART1_TX (AF7)
 * gpio_set_function(PA10, 7);  // USART1_RX (AF7)
 *
 * // 配置SPI1引脚
 * gpio_set_function(PA5, 5);   // SPI1_SCK (AF5)
 * gpio_set_function(PA6, 5);   // SPI1_MISO (AF5)
 * gpio_set_function(PA7, 5);   // SPI1_MOSI (AF5)
 * @endcode
 */
void gpio_set_function(gpio_pin_t pin, gpio_af_t af)
{
    assert(GPIO_PIN_VALID(pin));

    GPIO_TypeDef *gpio = gpio_get_port_base(pin);
    uint8_t pin_num = gpio_get_pin_num(pin);

    // 1. 配置为复用功能模式
    gpio_modify_register(&gpio->MD, pin_num, 2, GPIO_MODE_AF);

    // 2. 配置复用功能编号
    if (pin_num < 8) {
        // PIN0-7: 使用AF0寄存器
        gpio_modify_register(&gpio->AF0, pin_num, 4, af);
    } else {
        // PIN8-15: 使用AF1寄存器
        gpio_modify_register(&gpio->AF1, pin_num - 8, 4, af);
    }

    // 3. 默认配置：推挽输出、无上下拉、2mA驱动
    gpio->OTYP &= ~(1U << pin_num); // 推挽
    gpio_modify_register(&gpio->PUPD, pin_num, 2, GPIO_PULL_NONE);

    // 驱动能力配置（PIN0-7使用DS0，PIN8-15使用DS1）
    if (pin_num < 8) {
        gpio_modify_register(&gpio->DS0, pin_num, 4, GPIO_DRIVE_2MA);
    } else {
        gpio_modify_register(&gpio->DS1, pin_num - 8, 4, GPIO_DRIVE_2MA);
    }
}

/**
 * @brief 获取GPIO引脚当前的复用功能编号
 *
 * 读取引脚当前配置的复用功能编号。
 *
 * @param pin GPIO引脚编号
 * @return 复用功能编号 (0-15)
 *
 * 使用示例：
 * @code
 * // 检查引脚的复用功能
 * gpio_af_t af = gpio_get_function(PA9);
 * printf("PA9 AF = %d\n", af);
 * @endcode
 */
gpio_af_t gpio_get_function(gpio_pin_t pin)
{
    assert(GPIO_PIN_VALID(pin));

    GPIO_TypeDef *gpio = gpio_get_port_base(pin);
    uint8_t pin_num = gpio_get_pin_num(pin);

    uint32_t af_reg;
    uint8_t shift;

    if (pin_num < 8) {
        af_reg = gpio->AF0;
        shift = pin_num * 4;
    } else {
        af_reg = gpio->AF1;
        shift = (pin_num - 8) * 4;
    }

    return (gpio_af_t)((af_reg >> shift) & 0x0F);
}

//===========================================
// 第3层：高级功能API
// 包括：复用功能、输出配置、模拟模式、锁定保护
//===========================================

/**
 * @brief 设置GPIO引脚输出类型
 *
 * 配置引脚为推挽输出或开漏输出。
 *
 * @param pin GPIO引脚编号
 * @param otype 输出类型：GPIO_OTYPE_PP(推挽) 或 GPIO_OTYPE_OD(开漏)
 *
 * 使用示例：
 * @code
 * // 配置I2C引脚为开漏输出
 * gpio_init(PB6);
 * gpio_set_dir(PB6, GPIO_OUT);
 * gpio_set_output_type(PB6, GPIO_OTYPE_OD);
 * gpio_pull_up(PB6);
 *
 * // 配置普通LED为推挽输出
 * gpio_set_output_type(PA5, GPIO_OTYPE_PP);
 * @endcode
 */
void gpio_set_output_type(gpio_pin_t pin, gpio_otype_t otype)
{
    assert(GPIO_PIN_VALID(pin));

    GPIO_TypeDef *gpio = gpio_get_port_base(pin);
    uint8_t pin_num = gpio_get_pin_num(pin);

    if (otype == GPIO_OTYPE_OD) {
        gpio->OTYP |= (1U << pin_num); // 开漏
    } else {
        gpio->OTYP &= ~(1U << pin_num); // 推挽
    }
}

/**
 * @brief 设置GPIO引脚驱动能力
 *
 * 配置引脚的输出驱动能力（2mA - 16mA）。
 *
 * @param pin GPIO引脚编号
 * @param drive 驱动能力：GPIO_DRIVE_2MA ~ GPIO_DRIVE_16MA
 *
 * 使用示例：
 * @code
 * // SPI高速信号需要更强驱动能力
 * gpio_set_function(PA5, 5);  // SPI_SCK
 * gpio_set_drive_strength(PA5, GPIO_DRIVE_8MA);
 *
 * // 普通LED使用默认驱动
 * gpio_set_drive_strength(PA5, GPIO_DRIVE_2MA);
 * @endcode
 */
void gpio_set_drive_strength(gpio_pin_t pin, gpio_drive_t drive)
{
    assert(GPIO_PIN_VALID(pin));

    GPIO_TypeDef *gpio = gpio_get_port_base(pin);
    uint8_t pin_num = gpio_get_pin_num(pin);

    // PIN0-7使用DS0寄存器，PIN8-15使用DS1寄存器
    if (pin_num < 8) {
        gpio_modify_register(&gpio->DS0, pin_num, 4, drive);
    } else {
        gpio_modify_register(&gpio->DS1, pin_num - 8, 4, drive);
    }
}

/**
 * @brief 设置GPIO引脚大驱动能力
 *
 * 配置支持大驱动能力的引脚（20mA - 40mA）。
 *
 * @param pin GPIO引脚编号
 * @param drive 大驱动能力：GPIO_DRIVE_HIGH_20MA ~ GPIO_DRIVE_HIGH_40MA
 *
 * @note 只有部分引脚支持大驱动能力，请参考芯片手册
 *
 * 使用示例：
 * @code
 * // 驱动大功率LED
 * gpio_init(PA5);
 * gpio_set_dir(PA5, GPIO_OUT);
 * gpio_set_drive_strength_high(PA5, GPIO_DRIVE_HIGH_30MA);
 * @endcode
 */
void gpio_set_drive_strength_high(gpio_pin_t pin, gpio_drive_high_t drive)
{
    assert(GPIO_PIN_VALID(pin));

    GPIO_TypeDef *gpio = gpio_get_port_base(pin);
    uint8_t pin_num = gpio_get_pin_num(pin);

    // 大驱动能力配置（与标准驱动能力使用相同寄存器）
    if (pin_num < 8) {
        gpio_modify_register(&gpio->DS0, pin_num, 4, drive);
    } else {
        gpio_modify_register(&gpio->DS1, pin_num - 8, 4, drive);
    }
}

/**
 * @brief 设置GPIO引脚施密特触发器
 *
 * 使能或禁用施密特触发器，用于抑制输入信号抖动。
 *
 * @param pin GPIO引脚编号
 * @param enable true=使能，false=禁用
 *
 * 使用示例：
 * @code
 * // 按键输入使能施密特触发器
 * gpio_init(PC13);
 * gpio_set_dir(PC13, GPIO_IN);
 * gpio_set_schmitt(PC13, true);
 *
 * // 高速信号禁用施密特触发器
 * gpio_set_schmitt(PA5, false);
 * @endcode
 */
void gpio_set_schmitt(gpio_pin_t pin, bool enable)
{
    assert(GPIO_PIN_VALID(pin));

    GPIO_TypeDef *gpio = gpio_get_port_base(pin);
    uint8_t pin_num = gpio_get_pin_num(pin);

    if (enable) {
        gpio->SMIT |= (1U << pin_num);
    } else {
        gpio->SMIT &= ~(1U << pin_num);
    }
}

//===========================================
// 第4层：控制与查询API
//===========================================

/**
 * @brief 设置GPIO引脚为模拟模式
 *
 * 将引脚配置为模拟模式，用于ADC、DAC等模拟外设。
 * 自动禁能上下拉电阻和施密特触发器（手册要求）。
 *
 * @param pin GPIO引脚编号
 *
 * @note 覆盖寄存器：GPIOx_MD, GPIOx_PUPD, GPIOx_SMIT
 *
 * 使用示例：
 * @code
 * // 配置ADC输入引脚
 * gpio_init(PA0);
 * gpio_set_analog_mode(PA0);
 * gpio_set_analog_switch(PA0, true);
 *
 * // 配置DAC输出引脚
 * gpio_set_analog_mode(PA4);
 * gpio_set_analog_switch(PA4, true);
 * @endcode
 */
void gpio_set_analog_mode(gpio_pin_t pin)
{
    assert(GPIO_PIN_VALID(pin));

    GPIO_TypeDef *gpio = gpio_get_port_base(pin);
    uint8_t pin_num = gpio_get_pin_num(pin);

    // 配置为模拟模式
    gpio_modify_register(&gpio->MD, pin_num, 2, GPIO_MODE_ANALOG);

    // 手册要求模拟模式下须禁能上下拉电阻
    gpio_modify_register(&gpio->PUPD, pin_num, 2, GPIO_PULL_NONE);

    // 禁能施密特触发器
    gpio->SMIT &= ~(1U << pin_num);
}

/**
 * @brief 设置GPIO引脚模拟开关
 *
 * 控制模拟开关的开合状态。
 *
 * @param pin GPIO引脚编号
 * @param enable true=闭合模拟开关，false=打开模拟开关
 *
 * 使用示例：
 * @code
 * // ADC通道配置
 * gpio_set_analog_mode(PA0);
 * gpio_set_analog_switch(PA0, true);  // 闭合开关连接ADC
 *
 * // 禁用ADC通道
 * gpio_set_analog_switch(PA0, false); // 打开开关断开ADC
 * @endcode
 */
void gpio_set_analog_switch(gpio_pin_t pin, bool enable)
{
    assert(GPIO_PIN_VALID(pin));

    GPIO_TypeDef *gpio = gpio_get_port_base(pin);
    uint8_t pin_num = gpio_get_pin_num(pin);

    if (enable) {
        gpio->AIEN |= (1U << pin_num); // 闭合模拟开关
    } else {
        gpio->AIEN &= ~(1U << pin_num); // 打开模拟开关
    }
}

/**
 * @brief 使用配置结构体配置GPIO引脚
 *
 * 一次性配置引脚的所有参数。
 *
 * @param pin GPIO引脚编号
 * @param config 配置结构体指针
 *
 * 使用示例：
 * @code
 * // 配置I2C引脚
 * gpio_config_t i2c_cfg = {
 *     .mode = GPIO_MODE_AF,
 *     .output_type = GPIO_OTYPE_OD,
 *     .pull = GPIO_PULL_UP,
 *     .drive = GPIO_DRIVE_4MA,
 *     .af = 4,
 *     .schmitt = true
 * };
 * gpio_configure(PB6, &i2c_cfg);  // SCL
 * gpio_configure(PB7, &i2c_cfg);  // SDA
 *
 * // 配置高速SPI引脚
 * gpio_config_t spi_cfg = {
 *     .mode = GPIO_MODE_AF,
 *     .output_type = GPIO_OTYPE_PP,
 *     .pull = GPIO_PULL_NONE,
 *     .drive = GPIO_DRIVE_8MA,
 *     .af = 5,
 *     .schmitt = false
 * };
 * gpio_configure(PA5, &spi_cfg);  // SCK
 * gpio_configure(PA7, &spi_cfg);  // MOSI
 * @endcode
 */
void gpio_configure(gpio_pin_t pin, const gpio_config_t *config)
{
    assert(GPIO_PIN_VALID(pin));
    assert(config != NULL);

    GPIO_TypeDef *gpio = gpio_get_port_base(pin);
    uint8_t pin_num = gpio_get_pin_num(pin);

    // 1. 配置工作模式
    gpio_modify_register(&gpio->MD, pin_num, 2, config->mode);

    // 2. 配置输出类型
    if (config->output_type == GPIO_OTYPE_OD) {
        gpio->OTYP |= (1U << pin_num);
    } else {
        gpio->OTYP &= ~(1U << pin_num);
    }

    // 3. 配置上下拉
    gpio_modify_register(&gpio->PUPD, pin_num, 2, config->pull);

    // 4. 配置驱动能力
    if (pin_num < 8) {
        gpio_modify_register(&gpio->DS0, pin_num, 4, config->drive);
    } else {
        gpio_modify_register(&gpio->DS1, pin_num - 8, 4, config->drive);
    }

    // 5. 配置复用功能（如果是复用模式）
    if (config->mode == GPIO_MODE_AF) {
        if (pin_num < 8) {
            gpio_modify_register(&gpio->AF0, pin_num, 4, config->af);
        } else {
            gpio_modify_register(&gpio->AF1, pin_num - 8, 4, config->af);
        }
    }

    // 6. 配置施密特触发器
    if (config->schmitt) {
        gpio->SMIT |= (1U << pin_num);
    } else {
        gpio->SMIT &= ~(1U << pin_num);
    }
}


/**
 * @brief 锁定GPIO引脚配置
 *
 * 锁定引脚配置，防止意外修改。锁定后只能通过复位解锁。
 *
 * @param pin GPIO引脚编号
 *
 * @note 锁定后只能通过系统复位解锁
 *
 * 使用示例：
 * @code
 * // 配置关键引脚并锁定
 * gpio_init(PA5);
 * gpio_set_dir(PA5, GPIO_OUT);
 * gpio_lock(PA5);  // 锁定配置
 *
 * // 尝试修改配置将无效
 * gpio_set_dir(PA5, GPIO_IN);  // 此操作无效
 * @endcode
 */
void gpio_lock(gpio_pin_t pin)
{
    assert(GPIO_PIN_VALID(pin));

    GPIO_TypeDef *gpio = gpio_get_port_base(pin);
    uint8_t pin_num = gpio_get_pin_num(pin);

    // 锁定键写入序列：写1 -> 写0 -> 写1 -> 读0 -> 读1
    uint32_t lock_value = (1U << 16) | (1U << pin_num); // LOCK_KEY=1, LOCK_EN[pin]=1

    gpio->LOCK = lock_value;      // 写1
    gpio->LOCK = (1U << pin_num); // 写0 (LOCK_KEY=0)
    gpio->LOCK = lock_value;      // 写1

    volatile uint32_t temp;
    temp = gpio->LOCK; // 读0（验证LOCK_KEY=1）
    temp = gpio->LOCK; // 读1（可选，但建议读取以确认）
    (void)temp;        // 避免编译器警告
}

/**
 * @brief 检查GPIO引脚是否被锁定
 *
 * 检查引脚配置是否处于锁定状态。
 *
 * @param pin GPIO引脚编号
 * @return true=已锁定，false=未锁定
 *
 * 使用示例：
 * @code
 * // 检查引脚锁定状态
 * if (gpio_is_locked(PA5)) {
 *     printf("PA5 is locked\n");
 * }
 * @endcode
 */
bool gpio_is_locked(gpio_pin_t pin)
{
    assert(GPIO_PIN_VALID(pin));

    GPIO_TypeDef *gpio = gpio_get_port_base(pin);
    uint8_t pin_num = gpio_get_pin_num(pin);

    // 检查LOCK_KEY位是否被激活
    if (gpio->LOCK & (1U << 16)) {
        // 检查该引脚是否被锁定
        return (gpio->LOCK & (1U << pin_num)) != 0;
    }

    return false;
}


/**
 * @brief 批量置位GPIO端口引脚
 *
 * 原子操作批量置位指定端口的多个引脚。
 *
 * @param port 端口号 (0=GPIOA, 1=GPIOB, ...)
 * @param mask 引脚掩码 (bit0-15对应PIN0-15)
 *
 * 使用示例：
 * @code
 * // 同时置位GPIOA的PIN0、PIN1、PIN5
 * gpio_port_set_masked(0, 0x0023);  // 0x0023 = bit0 + bit1 + bit5
 *
 * // 批量操作LED
 * gpio_port_set_masked(0, 0xFF00);  // 置位PA8-PA15
 * @endcode
 */
void gpio_port_set_masked(uint8_t port, uint16_t mask)
{
    assert(port < 8); // 0-7 (GPIOA-GPIOH)

    const uint32_t port_bases[] = {
        GPIOA_BASE, GPIOB_BASE, GPIOC_BASE, GPIOD_BASE,
        GPIOE_BASE, GPIOF_BASE, GPIOG_BASE, GPIOH_BASE};

    GPIO_TypeDef *gpio = (GPIO_TypeDef *)port_bases[port];

    // 使用BSC寄存器的低16位置位
    gpio->BSC = mask;
}

/**
 * @brief 批量清零GPIO端口引脚
 *
 * 原子操作批量清零指定端口的多个引脚。
 *
 * @param port 端口号 (0=GPIOA, 1=GPIOB, ...)
 * @param mask 引脚掩码 (bit0-15对应PIN0-15)
 *
 * 使用示例：
 * @code
 * // 同时清零GPIOA的PIN0、PIN1、PIN5
 * gpio_port_clear_masked(0, 0x0023);
 *
 * // 关闭所有LED
 * gpio_port_clear_masked(0, 0xFFFF);  // 清零PA0-PA15
 * @endcode
 */
void gpio_port_clear_masked(uint8_t port, uint16_t mask)
{
    assert(port < 8);

    const uint32_t port_bases[] = {
        GPIOA_BASE, GPIOB_BASE, GPIOC_BASE, GPIOD_BASE,
        GPIOE_BASE, GPIOF_BASE, GPIOG_BASE, GPIOH_BASE};

    GPIO_TypeDef *gpio = (GPIO_TypeDef *)port_bases[port];

    // 使用BSC寄存器的高16位清零
    gpio->BSC = ((uint32_t)mask << 16);
}

/**
 * @brief 读取GPIO端口所有引脚状态
 *
 * 一次性读取整个端口16个引脚的输入状态。
 *
 * @param port 端口号 (0=GPIOA, 1=GPIOB, ...)
 * @return 16位引脚状态 (bit0-15对应PIN0-15)
 *
 * 使用示例：
 * @code
 * // 读取GPIOA所有引脚状态
 * uint16_t pins = gpio_port_get(0);
 * if (pins & 0x0001) {
 *     printf("PA0 is HIGH\n");
 * }
 *
 * // 读取并处理多个按键
 * uint16_t buttons = gpio_port_get(2);  // GPIOC
 * if (buttons & 0x2000) {  // PC13
 *     printf("Button 1 pressed\n");
 * }
 * @endcode
 */
uint16_t gpio_port_get(uint8_t port)
{
    assert(port < 8);

    const uint32_t port_bases[] = {
        GPIOA_BASE, GPIOB_BASE, GPIOC_BASE, GPIOD_BASE,
        GPIOE_BASE, GPIOF_BASE, GPIOG_BASE, GPIOH_BASE};

    GPIO_TypeDef *gpio = (GPIO_TypeDef *)port_bases[port];

    return (uint16_t)(gpio->IDATA & 0xFFFF);
}

/**
 * @brief 设置GPIO端口所有引脚输出值
 *
 * 一次性设置整个端口16个引脚的输出状态。
 *
 * @param port 端口号 (0=GPIOA, 1=GPIOB, ...)
 * @param value 16位输出值 (bit0-15对应PIN0-15)
 *
 * 使用示例：
 * @code
 * // 设置GPIOA输出值
 * gpio_port_put(0, 0x00FF);  // PA0-PA7高电平，PA8-PA15低电平
 *
 * // LED流水灯
 * for (int i = 0; i < 16; i++) {
 *     gpio_port_put(0, 1 << i);
 *     delay_ms(100);
 * }
 * @endcode
 */
void gpio_port_put(uint8_t port, uint16_t value)
{
    assert(port < 8);

    const uint32_t port_bases[] = {
        GPIOA_BASE, GPIOB_BASE, GPIOC_BASE, GPIOD_BASE,
        GPIOE_BASE, GPIOF_BASE, GPIOG_BASE, GPIOH_BASE};

    GPIO_TypeDef *gpio = (GPIO_TypeDef *)port_bases[port];

    gpio->ODATA = value;
}


/**
 * @brief 获取GPIO引脚工作模式
 *
 * 读取引脚当前的工作模式。
 *
 * @param pin GPIO引脚编号
 * @return 工作模式：GPIO_MODE_INPUT/OUTPUT/AF/ANALOG
 *
 * 使用示例：
 * @code
 * // 检查引脚模式
 * gpio_mode_t mode = gpio_get_mode(PA5);
 * if (mode == GPIO_MODE_OUTPUT) {
 *     printf("PA5 is in output mode\n");
 * }
 * @endcode
 */
gpio_mode_t gpio_get_mode(gpio_pin_t pin)
{
    assert(GPIO_PIN_VALID(pin));

    GPIO_TypeDef *gpio = gpio_get_port_base(pin);
    uint8_t pin_num = gpio_get_pin_num(pin);

    uint32_t shift = pin_num * 2;
    return (gpio_mode_t)((gpio->MD >> shift) & 0x03);
}

/**
 * @brief 获取GPIO引脚方向
 *
 * 读取引脚当前的方向（输入/输出）。
 * 仅当模式为GPIO_MODE_OUTPUT时返回GPIO_OUT，其他模式（含AF/模拟）均返回GPIO_IN。
 *
 * @param pin GPIO引脚编号
 * @return GPIO_IN 或 GPIO_OUT
 *
 * 使用示例：
 * @code
 * // 检查引脚方向
 * if (gpio_get_dir(PA5) == GPIO_OUT) {
 *     printf("PA5 is output\n");
 * }
 * @endcode
 */
gpio_dir_t gpio_get_dir(gpio_pin_t pin)
{
    gpio_mode_t mode = gpio_get_mode(pin);
    return (mode == GPIO_MODE_OUTPUT) ? GPIO_OUT : GPIO_IN;
}

/**
 * @brief 获取GPIO引脚输出类型
 *
 * 读取引脚的输出类型配置。
 *
 * @param pin GPIO引脚编号
 * @return GPIO_OTYPE_PP(推挽) 或 GPIO_OTYPE_OD(开漏)
 *
 * 使用示例：
 * @code
 * // 检查输出类型
 * if (gpio_get_output_type(PB6) == GPIO_OTYPE_OD) {
 *     printf("PB6 is open-drain\n");
 * }
 * @endcode
 */
gpio_otype_t gpio_get_output_type(gpio_pin_t pin)
{
    assert(GPIO_PIN_VALID(pin));

    GPIO_TypeDef *gpio = gpio_get_port_base(pin);
    uint8_t pin_num = gpio_get_pin_num(pin);

    return (gpio->OTYP & (1U << pin_num)) ? GPIO_OTYPE_OD : GPIO_OTYPE_PP;
}

/**
 * @brief 获取GPIO引脚上下拉配置
 *
 * 读取引脚的上下拉电阻配置。
 *
 * @param pin GPIO引脚编号
 * @return GPIO_PULL_NONE/UP/DOWN
 *
 * 使用示例：
 * @code
 * // 检查上下拉配置
 * gpio_pull_t pull = gpio_get_pull(PC13);
 * if (pull == GPIO_PULL_UP) {
 *     printf("PC13 has pull-up enabled\n");
 * }
 * @endcode
 */
gpio_pull_t gpio_get_pull(gpio_pin_t pin)
{
    assert(GPIO_PIN_VALID(pin));

    GPIO_TypeDef *gpio = gpio_get_port_base(pin);
    uint8_t pin_num = gpio_get_pin_num(pin);

    uint32_t shift = pin_num * 2;
    return (gpio_pull_t)((gpio->PUPD >> shift) & 0x03);
}

/**
 * @brief 获取GPIO引脚驱动能力
 *
 * 读取引脚的驱动能力配置。
 *
 * @param pin GPIO引脚编号
 * @return GPIO_DRIVE_2MA ~ GPIO_DRIVE_16MA
 *
 * 使用示例：
 * @code
 * // 检查驱动能力
 * gpio_drive_t drive = gpio_get_drive_strength(PA5);
 * printf("PA5 drive strength: %d\n", drive);
 * @endcode
 */
gpio_drive_t gpio_get_drive_strength(gpio_pin_t pin)
{
    assert(GPIO_PIN_VALID(pin));

    GPIO_TypeDef *gpio = gpio_get_port_base(pin);
    uint8_t pin_num = gpio_get_pin_num(pin);

    uint32_t ds_reg;
    uint8_t shift;

    // PIN0-7使用DS0寄存器，PIN8-15使用DS1寄存器
    if (pin_num < 8) {
        ds_reg = gpio->DS0;
        shift = pin_num * 4;
    } else {
        ds_reg = gpio->DS1;
        shift = (pin_num - 8) * 4;
    }

    return (gpio_drive_t)((ds_reg >> shift) & 0x0F);
}
