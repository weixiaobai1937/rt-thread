# ACM32P4 Coreboard RT-Thread 移植设计文档

## 概述

将 ACM32P4 Coreboard 开发板移植到 RT-Thread 实时操作系统，使其成为一个完整的 BSP（Board Support Package）。

### 硬件参数

| 项目 | 规格 |
|------|------|
| CPU 核心 | Star-MC1（ARMv8-M 兼容，兼容 Cortex-M33 指令集） |
| 系统主频 | 180MHz（可配置至 220MHz） |
| Flash | ~1016KB（0x08002000 起，前 8KB 为 Bootloader） |
| SRAM | 128KB（0x20000000 - 0x20020000） |
| PSRAM | 8MB（0x80000000，通过 OSPI 接口） |

### 目标

- 最小可运行系统：RT-Thread 内核 + GPIO 驱动 + UART 控制台 + 板载 LED
- 支持 GCC（ARM None-Eabi）和 Keil MDK（ARM Compiler 6）编译
- 通过 UART 控制台输出启动信息，可使用 Finsh 命令行交互

## 技术方案

采用方案 A：**完全独立 BSP**，直接使用 ACM32P4 SDK 的硬件 API（`hardware/gpio.h`、`hardware/uart.h` 等），不依赖 HAL 兼容层。

### 关键设计决策

| 决策 | 选择 | 理由 |
|------|------|------|
| 系统初始化 | `system_init()` | ACM32P4 SDK 标准入口，初始化时钟/NVIC/TIM64B |
| RT-Thread Tick 源 | SysTick | RT-Thread 原生支持，无需额外移植 |
| GPIO 驱动 | `hardware/gpio.h` API | Pico SDK 风格，线性引脚编号（PA0=0） |
| UART 驱动 | `hardware/uart.h` API | 支持轮询 + 中断模式，内置空闲中断 |
| 链接脚本 | `libraries/acm32p4/linker/acm32p4xx_flash.ld` | SDK 自带，已提供 `__bss_end__` 符号 |
| CPU 标识 | `cortex-m33` | Star-MC1 指令集兼容 ARMv8-M |
| FPU | `fpv5-sp-d16`，hard-float | Star-MC1 内置单精度 FPU |

## 文件结构

```
bsp/acm32/acm32p4xx-coreboard/
├── .config                          # [新建] Kconfig 配置输出
├── applications/
│   ├── main.c                       # [新建] 用户入口
│   └── SConscript                   # [新建]
├── drivers/
│   ├── board.c                      # [新建] 板级初始化
│   ├── board.h                      # [新建] 板级配置（UART 引脚等）
│   ├── drv_gpio.c                   # [新建] GPIO 驱动
│   ├── drv_uart.c                   # [新建] UART 驱动
│   ├── uart_config.h                # [新建] UART 配置宏
│   ├── Kconfig                      # [新建] 驱动配置
│   ├── SConscript                   # [新建]
│   └── linker_scripts/
│       └── link.lds                 # [新建] 链接脚本引用
├── Kconfig                          # [新建] BSP 级配置
├── libraries/                       # [已有] ACM32P4 SDK（全部不动）
│   ├── acm32p4/
│   │   ├── include/
│   │   │   ├── acm32p4.h
│   │   │   ├── device/
│   │   │   └── hardware/
│   │   ├── linker/
│   │   │   ├── acm32p4xx_flash.ld
│   │   │   ├── acm32p4xx_sram.ld
│   │   │   └── acm32p4xx.sct
│   │   ├── src/                     # 29 个外设源文件
│   │   └── startup/
│   │       ├── startup_acm32p4xx.s
│   │       └── system_acm32p4xx.c
│   └── example/                     # [已有] 40 个范例工程
├── project.uvprojx                  # [新建] Keil MDK 工程
├── project.uvoptx                   # [新建] Keil 工程选项
├── README.md                        # [新建]
├── rtconfig.h                       # [新建]
├── rtconfig.py                      # [新建]
├── SConscript                       # [新建]
└── SConstruct                       # [新建]
```

## 各模块设计

### 1. 板级初始化（board.c）

```c
void rt_hw_board_init(void)
{
    // 1. ACM32P4 SDK 系统初始化
    system_init();

    // 2. 配置 SysTick（RT-Thread tick）
    sys_info_t info;
    system_get_info(&info);
    SysTick_Config(info.sysclk_hz / RT_TICK_PER_SECOND);

    // 3. UART 初始化 + 控制台
    rt_hw_uart_init();
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);

    // 4. 堆内存初始化
    rt_system_heap_init((void *)&__bss_end, (void *)(0x20000000 + 128*1024));

    // 5. 组件初始化
    rt_components_board_init();
}
```

**注意：** ACM32P4 SDK 的 `system_init()` 使用 TIM64B 作为时基，不占用 SysTick。RT-Thread 可以自由使用 SysTick。

### 2. GPIO 驱动（drv_gpio.c）

基于 ACM32P4 的线性引脚编号系统（PA0=0, PB0=16, ...），实现 `rt_pin_ops`：

| RT-Thread PIN API | ACM32P4 实现 |
|---|---|
| `pin_mode(pin, OUTPUT)` | `gpio_init(pin); gpio_set_dir(pin, GPIO_OUT)` |
| `pin_mode(pin, INPUT)` | `gpio_init(pin); gpio_set_dir(pin, GPIO_IN)` |
| `pin_mode(pin, INPUT_PULLUP)` | `gpio_pull_up(pin)` |
| `pin_mode(pin, OUTPUT_OD)` | `gpio_set_output_type(pin, GPIO_OTYPE_OD)` |
| `pin_write(pin, HIGH)` | `gpio_put(pin, true)` |
| `pin_read(pin)` | `gpio_get(pin)` |

### 3. UART 驱动（drv_uart.c）

实现 RT-Thread 串口设备框架。使用 ACM32P4 UART API：

- **初始化**：`uart_init_default(uart_num, baudrate)` + GPIO AF 配置
- **发送**：轮询模式 `uart_putc()`，中断模式 `uart_write()` + 回调
- **接收**：中断模式，使用 `uart_register_rx_callback()` + `uart_register_idle_callback()` 实现帧边界检测
- **IRQ**：每个 UART 的 ISR 调用 `uart_irq_handler(uart_num)`

UART 引脚在 `board.h` 中通过宏配置，与范例代码风格一致。

### 4. 链接脚本（linker_scripts/link.lds）

直接引用 SDK 自带的链接脚本：

```
INCLUDE libraries/acm32p4/linker/acm32p4xx_flash.ld
```

该链接脚本已提供 RT-Thread 需要的符号：`__bss_end__`（用作堆起始地址）。

### 5. 工具链配置（rtconfig.py）

```python
ARCH = 'arm'
CPU = 'cortex-m33'        # Star-MC1 兼容 ARMv8-M
CROSS_TOOL = 'gcc'

# GCC flags
DEVICE = ' -mcpu=' + CPU + ' -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard -ffunction-sections -fdata-sections'
LFLAGS = DEVICE + ' -Wl,--gc-sections,-Map=rtthread.map,-cref,-u,Reset_Handler -T drivers/linker_scripts/link.lds'
```

### 6. Keil MDK 工程

- **Device**：`ARMCM33_DSP_FP`
- **ARM Compiler**：V6 (ARMCLANG)
- **Scatter File**：`libraries/acm32p4/linker/acm32p4xx.sct`
- **启动文件**：`libraries/acm32p4/startup/startup_acm32p4xx.s`
- 支持通过 `scons --target=mdk5` 自动生成

### 7. 应用入口（applications/main.c）

```c
int main(void)
{
    // system_init() 已在 rt_hw_board_init() 中调用
    // 此处放置用户应用代码
    
    return 0;
}
```

## 驱动支持计划

| 驱动 | 本阶段 | 备注 |
|------|--------|------|
| GPIO | ✅ | PIN 设备框架 |
| UART | ✅ | 串口设备框架 + 控制台 |
| LED | ✅ | 通过 GPIO PIN 框架控制 |
| SPI | 后续 | 硬件 API 已就绪 |
| I2C | 后续 | 硬件 API 已就绪 |
| ADC | 后续 | 硬件 API 已就绪 |
| PWM | 后续 | TIM/TIM64B 硬件 API 已就绪 |
| WDT | 后续 | 硬件 API 已就绪 |

## 风险与注意事项

1. **PendSV_Handler 冲突**：启动文件中的 `PendSV_Handler` 默认为弱符号（WEAK），RT-Thread 的 `cpuport.c` 会提供实现，需确认链接顺序
2. **HardFault_Handler 覆盖**：RT-Thread 需要接管 HardFault 进行异常诊断
3. **Star-MC1 与 Cortex-M33 差异**：虽然在 ARMv8-M 指令集层面兼容，但仍需关注可能的运行时差异
4. **PSRAM 初始化**：`system_init()` 默认会初始化 OSPI PSRAM，这会占用部分引脚（PG5-PG15, PF10），需在 BSP 文档中说明
