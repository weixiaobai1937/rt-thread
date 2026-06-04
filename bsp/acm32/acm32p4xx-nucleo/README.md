# ACM32P4xx-Nucleo BSP for RT-Thread

## Overview

This BSP supports the ACM32P4xx series MCU from AisinoChip, based on the Cortex-M33 core.

The ACM32P4xx is a high-performance 32-bit microcontroller featuring:
- ARM Cortex-M33 core with FPU (Star-MC1), up to 180MHz
- Up to 128KB SRAM
- Up to 1MB Flash
- Rich peripherals: UART, SPI, I2C, I2S, CAN-FD, Ethernet, USB, OSPI, etc.

## Supported Boards

- ACM32P4xx-Nucleo (ACM32P4xx Core Board)

## Quick Start

### Prerequisites

- RT-Thread source code
- GCC ARM Embedded toolchain (arm-none-eabi-gcc)
- Python 3.x with SCons

### Build

```bash
# Set the RTT_ROOT environment variable (optional)
export RTT_ROOT=/path/to/rt-thread

# Set the toolchain path
export RTT_EXEC_PATH=/path/to/gcc-arm-none-eabi/bin

# Build
scons
```

### Flash

```bash
# Using J-Link or ST-Link, flash the generated rtthread.bin
# The binary is located at: rtthread.bin
```

## Board Resources

| Resource | Configuration |
|----------|---------------|
| LED      | PA0           |
| UART1 TX | PA9           |
| UART1 RX | PA10          |

## Pin Map

| Pin Number | GPIO   | Pin Number | GPIO   |
|-----------|--------|-----------|--------|
| 0         | PA0    | 16        | PB0    |
| 1         | PA1    | 17        | PB1    |
| 2         | PA2    | 18        | PB2    |
| ...       | ...    | ...       | ...    |
| 15        | PA15   | 31        | PB15   |

## Reference

- [ACM32P4xx Datasheet](https://www.aisinochip.com)
- [RT-Thread Documentation](https://www.rt-thread.io/document/site/)
