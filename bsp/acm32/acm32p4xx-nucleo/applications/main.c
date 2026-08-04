/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-09-17     AisinoChip   the first version
 * 2026-06-04     AisinoChip   add ACM32P4xx support
 * 2026-07-24     AisinoChip   add I2C/ADC/DAC/RTC/WDT/Timer test prompts
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"

#define LED_PIN_NUM    0     /* PA0 - LED on ACM32P4xx Nucleo board */

int main(void)
{
    rt_kprintf("ACM32P4xx-Nucleo BSP boot success!\n");
#ifdef BSP_USING_UART1
    rt_kprintf("Run 'uart_test' / 'uart_test info' / 'uart_test echo uart2'\n");
#endif
#if defined(BSP_USING_SPI1) || defined(BSP_USING_SPI2) || \
    defined(BSP_USING_SPI3) || defined(BSP_USING_SPI4)
    rt_kprintf("Run 'spi_test' / 'spi_test info' / 'spi_test loopback 1' (short MOSI-MISO)\n");
#endif
#if defined(BSP_USING_I2C1) || defined(BSP_USING_I2C2)
    rt_kprintf("Run 'i2c_test' / 'i2c_test scan i2c1'\n");
#endif
#ifdef BSP_USING_ADC
    rt_kprintf("Run 'adc_test' / 'adc_test ch 3 10'\n");
#endif
#ifdef BSP_USING_DAC
    rt_kprintf("Run 'dac_test info' / 'dac_test out 1 1650' / 'dac_test sweep 1'\n");
#endif
#ifdef BSP_USING_RTC
    rt_kprintf("Run 'rtc_test' / 'rtc_test set 2026-07-24 15:30:00'\n");
#endif
#ifdef RT_USING_WDT
    rt_kprintf("Run 'wdt_test info' / 'wdt_test start 5' / 'wdt_test feed'\n");
#endif
#ifdef RT_USING_CLOCK_TIME
    rt_kprintf("Run 'timer_test' / 'timer_test info timer1'\n");
#endif
#ifdef RT_USING_PWM
    rt_kprintf("Run 'pwm_test' / 'pwm_test set pwm1.1 1000 50'\n");
#endif
#ifdef RT_USING_CAN
    rt_kprintf("Run 'can_test' / 'can_test send 123 11 22' / 'can_test loopback fdcan1'\n");
#endif
#ifdef BSP_USING_HWCRYPTO
    rt_kprintf("Run 'hwrng_test' for HW random numbers\n");
#endif
#if defined(RT_USING_PULSE_ENCODER)
    rt_kprintf("Run 'enc_test' / 'enc_test count pulse2' / 'enc_test clear pulse2'\n");
#endif
#if defined(RT_USING_INPUT_CAPTURE)
    rt_kprintf("Run 'cap_test' / 'cap_test read capture2'\n");
#endif
#ifdef BSP_USING_LPTIMER
    rt_kprintf("Run 'lptim_test' for LPTIM1 info\n");
#endif
#ifdef BSP_USING_I2S1
    rt_kprintf("Run 'audio_test' for I2S sound0 info\n");
#endif
#ifdef BSP_USING_SDMMC1
    rt_kprintf("Run 'sd_test' / 'sd_test read 0' (insert SD card)\n");
#endif
#ifdef DATA_IN_ExtSRAM
    rt_kprintf("Run 'psram_info' / 'psram_test' / 'psram_speed' for OSPI PSRAM\n");
#endif
#ifdef BSP_USING_ETH
    rt_kprintf("Run 'eth_test' / 'eth_test iperf -s' for Ethernet; eth_ifconfig\n");
#endif

    rt_pin_mode(LED_PIN_NUM, PIN_MODE_OUTPUT);
    rt_pin_write(LED_PIN_NUM, PIN_LOW);

    while (1)
    {
        rt_pin_write(LED_PIN_NUM, PIN_HIGH);
        rt_thread_mdelay(500);
        rt_pin_write(LED_PIN_NUM, PIN_LOW);
        rt_thread_mdelay(500);
    }
}
