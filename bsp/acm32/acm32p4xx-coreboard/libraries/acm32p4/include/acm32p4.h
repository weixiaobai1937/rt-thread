/**
 * @file acm32p4.h
 * @brief ACM32P4 SDK - 主头文件
 *
 * �?acm32p4 SDK 风格一致的精简 API
 */

#ifndef _ACM32P4_H
#define _ACM32P4_H

// ========================================
// CMSIS Device（会自动包含 core_starmc1.h�?// ========================================
#include "device/acm32p4xx.h"
#include "device/system_acm32p4xx.h"

// ========================================
// Hardware APIs
// ========================================
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/comp.h"
#include "hardware/dac.h"
#include "hardware/syscfg.h"
#include "hardware/gpio.h"
#include "hardware/exti.h"
#include "hardware/tim.h"
#include "hardware/pmu.h"
#include "hardware/dma.h"
#include "hardware/tim64b.h"
#include "hardware/i2c.h"
#include "hardware/lptim.h"
#include "hardware/lpuart.h"
#include "hardware/uart.h"
#include "hardware/wdt.h"
#include "hardware/iwdt.h"
#include "hardware/spi.h"
#include "hardware/ospi.h"
#include "hardware/i2s.h"
#include "hardware/system.h"

#endif // _ACM32P4_H
