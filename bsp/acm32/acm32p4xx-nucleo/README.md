# ACM32P4xx-Nucleo BSP for RT-Thread

## Overview

This BSP targets the **ACM32P4xx-Nucleo** board (AisinoChip Cortex-M33).

| Item | Value |
|------|--------|
| Core | ARM Cortex-M33 (Star-MC1) + FPU, up to 180 MHz |
| RAM | DTCM 64 KB + SRAM1 64 KB (128 KB total) |
| Flash | App image at `0x08002000` (~1016 KB usable of 1 MB) |
| Ext. mem | OSPI PSRAM 8 MB @ `0x80000000` (`DATA_IN_ExtSRAM`) |

> Not the same as `acm32p4xx-coreboard` (minimal UART-only BSP).

## Default-enabled features (`rtconfig.h`)

| Feature | Detail |
|---------|--------|
| Console | UART1 PA9/PA10 @ `uart1` |
| UART2 + DMA | PD5/PD6, TX DMA1_CH3 / RX DMA2_CH0 |
| SPI1 + DMA | PE12/11/10/13, soft CS |
| Ethernet | RMII + LAN8720A-class PHY, zero-copy RX in PSRAM |
| lwIP | Static IP default `192.168.16.50` (see menuconfig) |
| PSRAM | First 2 MB = `psram` memheap (ETH DMA); rest free for tests |

Other drivers (I2C, TIM/PWM, CAN, ADC/DAC, RTC, WDT, SDMMC, I2S, …) are available via **menuconfig**.

## Memory map

| Region | Address | Use |
|--------|---------|-----|
| DTCM | `0x20000000`–`0x2000FFFF` | `.data` / `.bss` + MSP stack |
| SRAM1 | `0x20010000`–`0x2001FFFF` | RT-Thread system heap |
| PSRAM | `0x80000000`–`0x801FFFFF` (2 MB heap) | ETH DMA + optional app |
| PSRAM rest | `0x80200000`–`0x807FFFFF` | `psram_test` region |

## Quick Start

### Prerequisites

- This RT-Thread tree
- `arm-none-eabi-gcc` + Python 3 + SCons  
  or Keil MDK (`template.uvprojx` / `project.uvprojx`)

### Build (GCC / SCons)

```bash
cd bsp/acm32/acm32p4xx-nucleo

# Windows example (MSYS2 toolchain)
set RTT_EXEC_PATH=C:/msys64/ucrt64/bin

# Linux / macOS
# export RTT_EXEC_PATH=/path/to/gcc-arm-none-eabi/bin

scons -j8
```

Outputs: `rtthread_acm32p4xx.elf`, `rtthread.bin`.

### Configure

```bash
scons --menuconfig
# then regenerate: source env setup if needed, or use ENV tool
```

SCons runs a **DMA/pin resource check** and appends a map into `rtconfig.h` (`BEGIN_BSP_RESOURCE_CHECK`).

### Flash

Program `rtthread.bin` with J-Link / OpenOCD / vendor tools.  
Vector table / app base: **`0x08002000`**.

## Board resources (default)

| Resource | Pins / notes |
|----------|----------------|
| LED | PA0 |
| UART1 console | PA9 TX, PA10 RX |
| UART2 | PD5 TX, PD6 RX (+ DMA) |
| SPI1 | PE12 SCK, PE11 MOSI, PE10 MISO, PE13 CS |
| ETH RMII | PA1/2/7, PB11/12/13, PC1/4/5 |
| PHY nRST | default PB14 (`BSP_ETH_PHY_RST_PIN=30`) |
| OSPI PSRAM | board OSPI1 wiring (see `system_acm32p4xx.c`) |

## MSH smoke tests

| Command | Purpose |
|---------|---------|
| `uart_test` / `spi_test` | Serial / SPI |
| `eth_ifconfig` / `eth_test` / `dping` | Ethernet |
| `psram_info` / `psram_test` / `psram_speed` | OSPI PSRAM |
| `list_device` | Registered devices |

When more peripherals are enabled: `i2c_test`, `can_test`, `adc_test`, `pwm_test`, `timer_test`, `wdt_test`, etc. (prompts also print from `main`).

## Important notes

1. **ETH requires working OSPI PSRAM.** Descriptors / bounce / RX pool allocate from `psram` memheap.
2. **Do not** run `psram_info reinit` while ETH is active (needs `force` and idle network).
3. **USB**: chip HAL sources exist, but this BSP has **no** RT-Thread USB device/host driver yet; FSUSB module is disabled in `acm32p4xx_hal_conf.h`.
4. **I2S vs SDMMC / DAC**: Kconfig mutual exclusion / alternate pins — see `drivers/Kconfig`.
5. **LPTIM**: hardware init only; PM tickless not integrated.

## Pin index map (GPIO)

`rt_pin` index: `PXn = port * 16 + n` (PA0=0 … PE15=79, PF0=80 … PG15=111).

## Reference

- [AisinoChip](https://www.aisinochip.com)
- [RT-Thread docs](https://www.rt-thread.io/document/site/)
