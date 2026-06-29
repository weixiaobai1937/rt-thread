# ACM32P4xx Coreboard BSP for RT-Thread

## Overview

This BSP supports the ACM32P4xx Coreboard development board from AisinoChip.

### Hardware Resources

| Resource | Configuration |
|----------|---------------|
| CPU      | Star-MC1 (ARMv8-M compatible), up to 180MHz |
| SRAM     | 128KB (0x20000000) |
| Flash    | 1MB (0x08002000, 8KB reserved for bootloader) |
| PSRAM    | 8MB (0x80000000, via OSPI) |
| LED      | PA0 |
| UART1 TX | PA9 |
| UART1 RX | PA10 |

## Quick Start

### Prerequisites

- RT-Thread source code
- GCC ARM Embedded toolchain (arm-none-eabi-gcc)
- Python 3.x with SCons

### Build

```bash
# Set RTT_ROOT (optional)
export RTT_ROOT=/path/to/rt-thread

# Set toolchain path
export RTT_EXEC_PATH=/path/to/gcc-arm-none-eabi/bin

# Build
scons
```

### Flash

Flash the generated `rtthread.bin` using J-Link, ST-Link, or your preferred debugger.

## Driver Support

| Driver | Status | Notes |
|--------|--------|-------|
| GPIO   | ✅ | PIN device framework, PA0-PH15 (linear numbering 0-127) |
| UART   | ✅ | Serial V2 framework, UART1 as console (115200 8N1) |
| LED    | ✅ | PA0, toggles at 1Hz in default application |
| SPI    | Planned | SDK API available |
| I2C    | Planned | SDK API available |
| ADC    | Planned | SDK API available |

## Pin Map

| RT-Thread PIN | GPIO  | RT-Thread PIN | GPIO  |
|---------------|-------|---------------|-------|
| 0             | PA0   | 16            | PB0   |
| 1             | PA1   | 17            | PB1   |
| 2             | PA2   | 18            | PB2   |
| ...           | ...   | ...           | ...   |
| 15            | PA15  | 31            | PB15  |

Pin numbering: `pin = port_index * 16 + pin_number`. Example: PC13 = 2*16 + 13 = 45.

## Notes

- The ACM32P4 SDK initializes OSPI PSRAM in `system_init()` by default. This uses pins PG5-PG15 and PF10. Avoid using these pins if PSRAM is enabled.
- `system_init()` uses TIM64B (TIM26) as the hardware timebase, not SysTick. RT-Thread uses SysTick for the kernel tick independently.
- The startup file enables DTCM before calling `system_init()`.

## Reference

- [ACM32P4xx Datasheet](https://www.aisinochip.com)
- [RT-Thread Documentation](https://www.rt-thread.io/document/site/)
