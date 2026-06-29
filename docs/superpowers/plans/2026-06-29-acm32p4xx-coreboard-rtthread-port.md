# ACM32P4 Coreboard RT-Thread 移植实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将 `bsp/acm32/acm32p4xx-coreboard` 建成为完整的 RT-Thread BSP，支持 GCC 编译、GPIO 驱动、UART 控制台和 Keil MDK 工程

**Architecture:** 以 `acm32p4xx-nucleo` BSP 为模板复制 BSP 骨架，使用 coreboard 已有的 ACM32P4 SDK（`libraries/acm32p4/`，硬件 API 风格，不含 HAL 层），重写 `drivers/` 驱动适配 SDK API，修改 `rtconfig.py`/`rtconfig.h`/`board.c`/`board.h` 适配 coreboard 硬件参数

**Tech Stack:** C, ARM GCC (arm-none-eabi-), SCons, Keil MDK (ARM Compiler 6), RT-Thread 5.x, ACM32P4 SDK (hardware API)

**参考文档：**
- 官方移植手册：`C:\Users\22206\Desktop\acm32xxx-mcu\rt-thread-standard-doc\programming-manual\porting\porting.md`
- UART V2 驱动文档：`C:\Users\22206\Desktop\acm32xxx-mcu\rt-thread-standard-doc\programming-manual\device\uart\uart_v2\uart.md`
- PIN 驱动文档：`C:\Users\22206\Desktop\acm32xxx-mcu\rt-thread-standard-doc\programming-manual\device\pin\pin.md`
- 已有 HAL 风格 BSP：`bsp/acm32/acm32p4xx-nucleo/`（参考 BSP 骨架）
- 范例代码：`bsp/acm32/acm32p4xx-coreboard/libraries/example/`（SDK 使用范例）
- 设计文档：`docs/superpowers/specs/2026-06-29-acm32p4xx-coreboard-rtthread-port-design.md`

## Global Constraints

- 必须使用 coreboard 已有的 `libraries/acm32p4/` SDK（不可删除或替换，不可引入 HAL 层）
- CPU 标识 `cortex-m33`，FPU=`fpv5-sp-d16`，hard-float ABI
- Flash 起始 `0x08002000`，长度 `0xFE000`；SRAM 起始 `0x20000000`，长度 `0x20000`（128KB）
- GCC 为主要编译器，Keil MDK ARM Compiler 6 也需支持
- RT-Thread 内核 tick 使用 SysTick
- 使用 RT-Thread 串口框架 V2（`RT_USING_SERIAL_V2`）
- BSP 必须可编译通过（`scons` 产生 `rtthread.elf`/`rtthread.bin`）
- 启动文件 SDK 自带Keil语法版本，需编写 GCC 版本

---

### Task 1: 创建 BSP 骨架文件（SConstruct / SConscript / Kconfig / rtconfig.py / rtconfig.h）

**Files:**
- Create: `bsp/acm32/acm32p4xx-coreboard/SConstruct`
- Create: `bsp/acm32/acm32p4xx-coreboard/SConscript`
- Create: `bsp/acm32/acm32p4xx-coreboard/Kconfig`
- Create: `bsp/acm32/acm32p4xx-coreboard/rtconfig.py`
- Create: `bsp/acm32/acm32p4xx-coreboard/rtconfig.h`

**Interfaces:**
- Produces: 构建系统入口/配置/工具链/内核配置

- [ ] **Step 1: 创建 SConstruct（构建入口）**

```python
import os
import sys
import rtconfig

if os.getenv('RTT_ROOT'):
    RTT_ROOT = os.getenv('RTT_ROOT')
else:
    RTT_ROOT = os.path.normpath(os.getcwd() + '/../../..')

sys.path = sys.path + [os.path.join(RTT_ROOT, 'tools')]
from building import *

TARGET = 'rtthread_acm32p4xx.' + rtconfig.TARGET_EXT

env = Environment(tools = ['mingw'],
    AS = rtconfig.AS, ASFLAGS = rtconfig.AFLAGS,
    CC = rtconfig.CC, CFLAGS = rtconfig.CFLAGS,
    AR = rtconfig.AR, ARFLAGS = '-rc',
    LINK = rtconfig.LINK, LINKFLAGS = rtconfig.LFLAGS)
env.PrependENVPath('PATH', rtconfig.EXEC_PATH)

if rtconfig.PLATFORM in ['iccarm']:
    env.Replace(CCCOM = ['$CC $CFLAGS $CPPFLAGS $_CPPDEFFLAGS $_CPPINCFLAGS -o $TARGET $SOURCES'])
    env.Replace(ARFLAGS = [''])
    env.Replace(LINKCOM = ['$LINK $SOURCES $LINKFLAGS -o $TARGET --map project.map'])

Export('RTT_ROOT')
Export('rtconfig')

objs = PrepareBuilding(env, RTT_ROOT, has_libcpu=False)
DoBuilding(TARGET, objs)
```

- [ ] **Step 2: 创建 SConscript**

```python
import os
from building import *

cwd  = GetCurrentDir()
objs = []
list = os.listdir(cwd)
for d in list:
    path = os.path.join(cwd, d)
    if os.path.isfile(os.path.join(path, 'SConscript')):
        objs = objs + SConscript(os.path.join(d, 'SConscript'))
Return('objs')
```

- [ ] **Step 3: 创建 Kconfig**

```kconfig
mainmenu "RT-Thread Project Configuration"
BSP_DIR := .
RTT_DIR := ../../..
PKGS_DIR := packages
source "$(RTT_DIR)/Kconfig"
osource "$PKGS_DIR/Kconfig"
source "$(BSP_DIR)/drivers/Kconfig"
```

- [ ] **Step 4: 创建 rtconfig.py**

```python
import os

ARCH='arm'
CPU='cortex-m33'
CROSS_TOOL='gcc'
BSP_LIBRARY_TYPE = None

if os.getenv('RTT_CC'):
    CROSS_TOOL = os.getenv('RTT_CC')
if os.getenv('RTT_ROOT'):
    RTT_ROOT = os.getenv('RTT_ROOT')

if  CROSS_TOOL == 'gcc':
    PLATFORM     = 'gcc'
    EXEC_PATH     = r'/usr/bin'
elif CROSS_TOOL == 'keil':
    PLATFORM    = 'armclang'
    EXEC_PATH   = r'C:/Keil_v5/ARM/ARMCLANG/bin'
elif CROSS_TOOL == 'iar':
    PLATFORM    = 'iccarm'
    EXEC_PATH   = r'C:/Program Files (x86)/IAR Systems/Embedded Workbench 8.3'

if os.getenv('RTT_EXEC_PATH'):
    EXEC_PATH = os.getenv('RTT_EXEC_PATH')

BUILD = 'debug'

if PLATFORM == 'gcc':
    PREFIX = 'arm-none-eabi-'
    CC = PREFIX + 'gcc'
    AS = PREFIX + 'gcc'
    AR = PREFIX + 'ar'
    CXX = PREFIX + 'g++'
    LINK = PREFIX + 'gcc'
    TARGET_EXT = 'elf'
    SIZE = PREFIX + 'size'
    OBJDUMP = PREFIX + 'objdump'
    OBJCPY = PREFIX + 'objcopy'

    DEVICE = ' -mcpu=' + CPU + ' -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard -ffunction-sections -fdata-sections'
    CFLAGS = DEVICE
    AFLAGS = ' -c' + DEVICE + ' -x assembler-with-cpp -Wa,-mimplicit-it=thumb '
    LFLAGS = DEVICE + ' -Wl,--gc-sections,-Map=rtthread.map,-cref,-u,Reset_Handler -T drivers/linker_scripts/link.lds'

    CPATH = ''
    LPATH = ''
    if BUILD == 'debug':
        CFLAGS += ' -O0 -gdwarf-2 -g'
        AFLAGS += ' -gdwarf-2'
    else:
        CFLAGS += ' -O2'
    CXXFLAGS = CFLAGS
    POST_ACTION = OBJCPY + ' -O binary $TARGET rtthread.bin\n' + SIZE + ' $TARGET \n'

elif PLATFORM == 'armclang':
    CC = 'armclang'
    AS = 'armasm'
    AR = 'armar'
    LINK = 'armlink'
    TARGET_EXT = 'axf'
    CFLAGS = ' -xc -std=c99 --target=arm-arm-none-eabi -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -c'
    CFLAGS += ' -fno-rtti -funsigned-char -fshort-enums -fshort-wchar -D__MICROLIB -mlittle-endian -ffunction-sections'
    AFLAGS = ' --cpu=Cortex-M33 --fpu=FPv5-SP --li'
    LFLAGS = ' --cpu=Cortex-M33 --info sizes --info totals --info unused --info veneers --list ./build/acm32p4xx.map --scatter ./build/acm32p4xx.sct'
    CXXFLAGS = CFLAGS
    POST_ACTION = 'fromelf --bincombined $TARGET --output rtthread.bin\nfromelf -z $TARGET'

elif PLATFORM == 'iccarm':
    CC = 'iccarm'
    CXX = 'iccarm'
    AS = 'iasmarm'
    AR = 'iarchive'
    LINK = 'ilinkarm'
    TARGET_EXT = 'out'
    CFLAGS = '--endian=little --cpu=Cortex-M33 --fpu=FPv5-SP -e'
    AFLAGS = '--cpu Cortex-M33 --fpu FPv5-SP -s+ -M<> -w+ -pl'
    LFLAGS = '--config drivers/linker_scripts/link.icf'
    CXXFLAGS = CFLAGS
```

- [ ] **Step 5: 创建 rtconfig.h**

```c
#ifndef RT_CONFIG_H__
#define RT_CONFIG_H__

/* RT-Thread Kernel */
#define RT_NAME_MAX 12
#define RT_CPUS_NR 1
#define RT_ALIGN_SIZE 8
#define RT_THREAD_PRIORITY_32
#define RT_THREAD_PRIORITY_MAX 32
#define RT_TICK_PER_SECOND 1000
#define RT_USING_OVERFLOW_CHECK
#define RT_USING_HOOK
#define RT_HOOK_USING_FUNC_PTR
#define RT_USING_IDLE_HOOK
#define RT_IDLE_HOOK_LIST_SIZE 4
#define IDLE_THREAD_STACK_SIZE 256
#define RT_USING_TIMER_SOFT
#define RT_TIMER_THREAD_PRIO 4
#define RT_TIMER_THREAD_STACK_SIZE 512

#define RT_USING_DEBUG
#define RT_DEBUGING_ASSERT
#define RT_DEBUGING_COLOR
#define RT_DEBUGING_CONTEXT

#define RT_USING_SEMAPHORE
#define RT_USING_MUTEX
#define RT_USING_EVENT
#define RT_USING_MAILBOX
#define RT_USING_MESSAGEQUEUE

#define RT_USING_MEMPOOL
#define RT_USING_SMALL_MEM
#define RT_USING_SMALL_MEM_AS_HEAP
#define RT_USING_HEAP

#define RT_USING_DEVICE
#define RT_USING_DEVICE_OPS
#define RT_USING_CONSOLE
#define RT_CONSOLEBUF_SIZE 128
#define RT_CONSOLE_DEVICE_NAME "uart1"

#define RT_USING_COMPONENTS_INIT
#define RT_USING_USER_MAIN
#define RT_MAIN_THREAD_STACK_SIZE 2048
#define RT_MAIN_THREAD_PRIORITY 10
#define RT_USING_MSH
#define RT_USING_FINSH
#define FINSH_USING_MSH
#define FINSH_THREAD_NAME "tshell"
#define FINSH_THREAD_PRIORITY 20
#define FINSH_THREAD_STACK_SIZE 4096
#define FINSH_USING_HISTORY
#define FINSH_HISTORY_LINES 5
#define FINSH_USING_SYMTAB
#define FINSH_CMD_SIZE 80
#define MSH_USING_BUILT_IN_COMMANDS
#define FINSH_USING_DESCRIPTION
#define FINSH_ARG_MAX 10

#define RT_USING_DEVICE_IPC
#define RT_USING_SERIAL_V2
#define RT_USING_PIN

/* Hardware Drivers Config */
#define SOC_ACM32P40N128CCT
#define SOC_SRAM_START_ADDR 0x20000000
#define SOC_SRAM_SIZE 0x80
#define SOC_FLASH_START_ADDR 0x08002000
#define SOC_FLASH_SIZE 0xFE0
#define BSP_USING_UART1

#endif
```

---

### Task 2: 创建链接脚本引用 + drivers SConscript

**Files:**
- Create: `bsp/acm32/acm32p4xx-coreboard/drivers/linker_scripts/link.lds`
- Create: `bsp/acm32/acm32p4xx-coreboard/drivers/SConscript`

- [ ] **Step 1: link.lds**

```ld
/* ACM32P4xx Coreboard linker script */
INCLUDE libraries/acm32p4/linker/acm32p4xx_flash.ld
```

- [ ] **Step 2: drivers/SConscript**

```python
Import('RTT_ROOT')
Import('rtconfig')
from building import *

cwd = GetCurrentDir()
src = Glob('*.c')
CPPPATH = [cwd]

group = DefineGroup('Drivers', src, depend = [''], CPPPATH = CPPPATH)
Return('group')
```

---

### Task 3: 创建 libraries SConscript（编译 SDK）

**Files:**
- Create: `bsp/acm32/acm32p4xx-coreboard/libraries/SConscript`

- [ ] **Step 1: libraries/SConscript**

```python
import rtconfig
Import('RTT_ROOT')
from building import *

cwd = GetCurrentDir()

# ACM32P4 SDK 源文件
src = Glob('acm32p4/src/*.c')
src += ['acm32p4/startup/system_acm32p4xx.c']

# 按编译器选择启动文件
if rtconfig.PLATFORM in ['gcc']:
    src += ['acm32p4/startup/startup_acm32p4xx_gcc.S']
elif rtconfig.PLATFORM in ['armcc', 'armclang']:
    src += ['acm32p4/startup/startup_acm32p4xx.s']
elif rtconfig.PLATFORM in ['iccarm']:
    src += ['acm32p4/startup/startup_acm32p4xx.s']

path = [
    cwd + '/acm32p4/include',
    cwd + '/acm32p4/include/device',
    cwd + '/acm32p4/include/hardware',
]

group = DefineGroup('ACM32P4_SDK', src, depend = [''], CPPPATH = path)
Return('group')
```

---

### Task 4: 创建 drivers 配置（Kconfig / board.h / board.c）

**Files:**
- Create: `bsp/acm32/acm32p4xx-coreboard/drivers/Kconfig`
- Create: `bsp/acm32/acm32p4xx-coreboard/drivers/board.h`
- Create: `bsp/acm32/acm32p4xx-coreboard/drivers/board.c`

- [ ] **Step 1: drivers/Kconfig**

```kconfig
menu "Hardware Drivers Config"

choice
    prompt "select chip type"
    default SOC_ACM32P40N128CCT

    config SOC_ACM32P40N128CCT
    bool "SOC_ACM32P40N128CCT"
    select RT_USING_COMPONENTS_INIT
    select RT_USING_USER_MAIN
    help
        ACM32P40N128CCT, Star-MC1 core, 128KB SRAM, 1MB Flash
endchoice

menu "ACM32P4xx Chip Configuration"
    depends on SOC_ACM32P40N128CCT
    config SOC_SRAM_START_ADDR
        hex "sram start address"
        default 0x20000000
    config SOC_SRAM_SIZE
        hex "sram size(KBytes)"
        default 0x80
    config SOC_FLASH_START_ADDR
        hex "Flash Start Address"
        default 0x08002000
    config SOC_FLASH_SIZE
        hex "Flash Size(KBytes)"
        default 0xFE0
endmenu

menu "On-chip Peripheral Drivers"
    config BSP_USING_UART1
        bool "Enable UART1 (PA9/PA10)"
        select RT_USING_SERIAL_V2
        default y
    config BSP_USING_GPIO1
        bool "Enable GPIO (PA-PB)"
        select RT_USING_PIN
        default y
endmenu

endmenu
```

- [ ] **Step 2: drivers/board.h**

```c
#ifndef __BOARD_H__
#define __BOARD_H__

#include <rtconfig.h>
#include "acm32p4.h"

/* UART1: TX=PA9(AF1), RX=PA10(AF1) — 控制台 */
#if defined(BSP_USING_UART1)
    #define UART1_TX_PIN            PA9
    #define UART1_RX_PIN            PA10
    #define UART1_AF                GPIO_AF_1
#endif

/* Coreboard LED: PA0 */
#define LED_PIN                  PA0

/* SOC 内存配置 */
#define SOC_SRAM_START_ADDR      (0x20000000)
#define SOC_SRAM_SIZE            (128)   /* KB */

void rt_hw_board_init(void);

#endif /* __BOARD_H__ */
```

- [ ] **Step 3: drivers/board.c**

```c
#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"

#define SOC_SRAM_END_ADDR   (SOC_SRAM_START_ADDR + SOC_SRAM_SIZE * 1024)

extern int __bss_end;
extern void rt_hw_uart_init(void);

void SysTick_Handler(void)
{
    rt_interrupt_enter();
    rt_tick_increase();
    rt_interrupt_leave();
}

void rt_hw_board_init(void)
{
    /* 1. ACM32P4 SDK 系统初始化（时钟/NVIC/TIM64B/PSRAM） */
    system_init();

    /* 2. 配置 SysTick */
    sys_info_t info;
    system_get_info(&info);
    SysTick_Config(info.sysclk_hz / RT_TICK_PER_SECOND);

    /* 3. UART + 控制台 */
    rt_hw_uart_init();
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);

    /* 4. 堆内存 */
#ifdef RT_USING_HEAP
    rt_system_heap_init((void *)&__bss_end, (void *)SOC_SRAM_END_ADDR);
#endif

    /* 5. 组件初始化 */
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif
}
```

---

### Task 5: 编写 GCC 启动文件

**Files:**
- Create: `bsp/acm32/acm32p4xx-coreboard/libraries/acm32p4/startup/startup_acm32p4xx_gcc.S`

- [ ] **Step 1: 编写 GCC 启动文件**

参照 Keil 版 `startup_acm32p4xx.s` 的向量表，转为 GCC 汇编语法。关键点：

- `.section .isr_vector, "a"` — 匹配链接脚本
- 所有 Handler 用 `.weak` + `.thumb_set` 指向 `Default_Handler`
- `Reset_Handler` 中调用 `system_init()` 而非 `SystemInit`
- `SysTick_Handler` 和 `PendSV_Handler` 为弱符号，RT-Thread 提供强符号覆盖

```asm
.syntax unified
.cpu cortex-m33
.thumb

.equ Stack_Size, 0x00004000
.equ Heap_Size,  0x00001000

.section .isr_vector, "a"
.align 2
.globl __Vectors
__Vectors:
    .long   __initial_sp
    .long   Reset_Handler
    .long   NMI_Handler
    .long   HardFault_Handler
    .long   MemManage_Handler
    .long   BusFault_Handler
    .long   UsageFault_Handler
    .long   0
    .long   0
    .long   0
    .long   0
    .long   SVC_Handler
    .long   DebugMon_Handler
    .long   0
    .long   PendSV_Handler
    .long   SysTick_Handler
    /* External Interrupts — 从 startup_acm32p4xx.s 完整复制向量表 */
    .long   WDT_IRQHandler
    /* ... 所有外设中断 ... */
    .long   FMAC_IRQHandler
    .size __Vectors, . - __Vectors

.weak Reset_Handler
.type Reset_Handler, %function
Reset_Handler:
    ldr r0, =_sidata
    ldr r1, =_sdata
    ldr r2, =_edata
    b LoopCopyDataInit
CopyDataInit:
    ldr r3, [r0], #4
    str r3, [r1], #4
LoopCopyDataInit:
    cmp r1, r2
    bcc CopyDataInit

    ldr r0, =_sbss
    ldr r1, =_ebss
    movs r2, #0
    b LoopFillZerobss
FillZerobss:
    str r2, [r0], #4
LoopFillZerobss:
    cmp r0, r1
    bcc FillZerobss

    bl system_init          /* ACM32P4 SDK 初始化 */
    bl __libc_init_array
    bl main
    b .

/* 默认异常处理 */
.weak NMI_Handler
.thumb_set NMI_Handler, Default_Handler
.weak HardFault_Handler
.thumb_set HardFault_Handler, Default_Handler
.weak MemManage_Handler
.thumb_set MemManage_Handler, Default_Handler
.weak BusFault_Handler
.thumb_set BusFault_Handler, Default_Handler
.weak UsageFault_Handler
.thumb_set UsageFault_Handler, Default_Handler
.weak SVC_Handler
.thumb_set SVC_Handler, Default_Handler
.weak DebugMon_Handler
.thumb_set DebugMon_Handler, Default_Handler
.weak PendSV_Handler
.thumb_set PendSV_Handler, Default_Handler
.weak SysTick_Handler
.thumb_set SysTick_Handler, Default_Handler

/* 所有外设中断 */
.weak WDT_IRQHandler
.thumb_set WDT_IRQHandler, Default_Handler
/* ... 全部外设 ... */
.weak FMAC_IRQHandler
.thumb_set FMAC_IRQHandler, Default_Handler

Default_Handler:
    b .

.align 3
.section .stack, "aw", %nobits
.globl __initial_sp
Stack_Mem: .space Stack_Size
__initial_sp:

.section .heap, "aw", %nobits
.globl __heap_start__
.globl __heap_end__
__heap_start__: .space Heap_Size
__heap_end__:
```

**注意：** 完整向量表需从 `startup_acm32p4xx.s` 逐项转换（100+ 个外设中断入口），上面为示意结构。

---

### Task 6: 创建 UART V2 驱动

**Files:**
- Create: `bsp/acm32/acm32p4xx-coreboard/drivers/uart_config.h`
- Create: `bsp/acm32/acm32p4xx-coreboard/drivers/drv_uart.c`

- [ ] **Step 1: uart_config.h**

```c
#ifndef __UART_CONFIG_H__
#define __UART_CONFIG_H__

#include "board.h"
#include "acm32p4.h"
#include "hardware/uart.h"

struct acm32_uart_config
{
    const char          *name;
    uart_num_t          uart_num;
    IRQn_Type           irq_type;
    gpio_pin_t          tx_pin;
    gpio_pin_t          rx_pin;
    gpio_af_t           af;
};

#ifdef BSP_USING_UART1
#define UART1_CONFIG \
    { .name = "uart1", .uart_num = UART_1, .irq_type = USART1_IRQn, \
      .tx_pin = PA9, .rx_pin = PA10, .af = GPIO_AF_1 }
#endif

#endif
```

- [ ] **Step 2: drv_uart.c**

参照 `acm32p4xx-nucleo/drivers/drv_uart.c` 的 V2 框架，核心映射：
- `HAL_UART_Init()` → `uart_init_default(uart_num, baudrate)` + GPIO AF
- `_uart_putc` → `uart_putc(uart_num, c)`
- `_uart_getc` → `uart_is_readable()` ? `uart_getc()` : -1
- ISR → `uart_irq_handler(uart_num)` 在用户 ISR 中调用，`uart_is_readable()` 驱动 RT-Thread RX 事件

```c
#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"
#include "uart_config.h"

#ifdef RT_USING_SERIAL_V2

struct acm32_uart
{
    struct acm32_uart_config    *config;
    struct rt_serial_device     serial;
};
#define raw_to_uart(raw) rt_container_of(raw, struct acm32_uart, serial)

static rt_err_t _uart_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
    struct acm32_uart *uart = raw_to_uart(serial);

    gpio_init(uart->config->tx_pin);
    gpio_set_function(uart->config->tx_pin, uart->config->af);
    gpio_init(uart->config->rx_pin);
    gpio_set_function(uart->config->rx_pin, uart->config->af);
    gpio_pull_up(uart->config->rx_pin);

    uart_init_default(uart->config->uart_num, cfg->baud_rate);
    return RT_EOK;
}

static rt_err_t _uart_control(struct rt_serial_device *serial, int cmd, void *arg)
{
    struct acm32_uart *uart = raw_to_uart(serial);
    switch (cmd)
    {
    case RT_DEVICE_CTRL_CLR_INT:
        NVIC_DisableIRQ(uart->config->irq_type);
        break;
    case RT_DEVICE_CTRL_SET_INT:
        NVIC_EnableIRQ(uart->config->irq_type);
        break;
    }
    return RT_EOK;
}

static int _uart_putc(struct rt_serial_device *serial, char c)
{
    struct acm32_uart *uart = raw_to_uart(serial);
    uart_putc(uart->config->uart_num, (uint8_t)c);
    return 1;
}

static int _uart_getc(struct rt_serial_device *serial)
{
    struct acm32_uart *uart = raw_to_uart(serial);
    if (uart_is_readable(uart->config->uart_num))
        return (int)uart_getc(uart->config->uart_num);
    return -1;
}

static rt_ssize_t _uart_transmit(struct rt_serial_device *serial, rt_uint8_t *buf, rt_size_t size, rt_uint32_t tx_flag)
{
    struct acm32_uart *uart = raw_to_uart(serial);
    rt_size_t i;
    for (i = 0; i < size; i++)
        uart_putc(uart->config->uart_num, buf[i]);
    return size;
}

static const struct rt_uart_ops acm32_uart_ops =
{
    .configure = _uart_configure,
    .control   = _uart_control,
    .putc      = _uart_putc,
    .getc      = _uart_getc,
    .transmit  = _uart_transmit,
};

static void uart_isr(struct rt_serial_device *serial)
{
    struct acm32_uart *uart = raw_to_uart(serial);
    if (uart_is_readable(uart->config->uart_num))
        rt_hw_serial_isr(serial, RT_SERIAL_EVENT_RX_IND);
}

enum {
#ifdef BSP_USING_UART1
    UART1_INDEX,
#endif
    UART_MAX_INDEX,
};

static struct acm32_uart_config uart_config[] = {
#ifdef BSP_USING_UART1
    UART1_CONFIG,
#endif
};

static struct acm32_uart uart_obj[UART_MAX_INDEX] = {0};

rt_err_t rt_hw_uart_init(void)
{
    rt_size_t n = sizeof(uart_obj) / sizeof(struct acm32_uart);
    struct serial_configure c = RT_SERIAL_CONFIG_DEFAULT;
    for (int i = 0; i < n; i++)
    {
        uart_obj[i].config = &uart_config[i];
        uart_obj[i].serial.ops = &acm32_uart_ops;
        uart_obj[i].serial.config = c;
        rt_err_t rc = rt_hw_serial_register(&uart_obj[i].serial,
            uart_obj[i].config->name,
            RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_INT_TX,
            NULL);
        RT_ASSERT(rc == RT_EOK);
    }
    return RT_EOK;
}

#ifdef BSP_USING_UART1
void USART1_IRQHandler(void)
{
    rt_interrupt_enter();
    uart_irq_handler(UART_1);
    uart_isr(&uart_obj[UART1_INDEX].serial);
    rt_interrupt_leave();
}
#endif

#endif /* RT_USING_SERIAL_V2 */
```

---

### Task 7: 创建 GPIO 驱动

**Files:**
- Create: `bsp/acm32/acm32p4xx-coreboard/drivers/drv_gpio.c`

- [ ] **Step 1: drv_gpio.c**

ACM32P4 线性引脚编号（PA0=0, PB0=16, ...），与 RT-Thread PIN 框架天然兼容。

```c
#include <rthw.h>
#include <rtdevice.h>
#include "board.h"

#ifdef RT_USING_PIN

#define ACM32_PIN_MAX   128

static void _pin_write(rt_device_t dev, rt_base_t pin, rt_uint8_t value)
{
    if (pin >= ACM32_PIN_MAX) return;
    gpio_put((gpio_pin_t)pin, value != PIN_LOW);
}

static rt_ssize_t _pin_read(rt_device_t dev, rt_base_t pin)
{
    if (pin >= ACM32_PIN_MAX) return -RT_EINVAL;
    return gpio_get((gpio_pin_t)pin) ? PIN_HIGH : PIN_LOW;
}

static void _pin_mode(rt_device_t dev, rt_base_t pin, rt_uint8_t mode)
{
    if (pin >= ACM32_PIN_MAX) return;
    gpio_pin_t p = (gpio_pin_t)pin;

    switch (mode)
    {
    case PIN_MODE_OUTPUT:
        gpio_init(p);
        gpio_set_dir(p, GPIO_OUT);
        break;
    case PIN_MODE_INPUT:
        gpio_init(p);
        gpio_set_dir(p, GPIO_IN);
        break;
    case PIN_MODE_INPUT_PULLUP:
        gpio_init(p);
        gpio_set_dir(p, GPIO_IN);
        gpio_pull_up(p);
        break;
    case PIN_MODE_INPUT_PULLDOWN:
        gpio_init(p);
        gpio_set_dir(p, GPIO_IN);
        gpio_pull_down(p);
        break;
    case PIN_MODE_OUTPUT_OD:
        gpio_init(p);
        gpio_set_dir(p, GPIO_OUT);
        gpio_set_output_type(p, GPIO_OTYPE_OD);
        break;
    default:
        break;
    }
}

static rt_err_t _pin_attach_irq(struct rt_device *device, rt_base_t pin,
                                 rt_uint8_t mode, void (*hdr)(void *args), void *args)
{ return -RT_ENOSYS; }

static rt_err_t _pin_dettach_irq(struct rt_device *device, rt_base_t pin)
{ return -RT_ENOSYS; }

static rt_err_t _pin_irq_enable(struct rt_device *device, rt_base_t pin, rt_uint8_t enabled)
{ return -RT_ENOSYS; }

static const struct rt_pin_ops _acm32_pin_ops =
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
    rt_device_pin_register("pin", &_acm32_pin_ops, RT_NULL);
    return 0;
}
INIT_BOARD_EXPORT(rt_hw_pin_init);

#endif /* RT_USING_PIN */
```

---

### Task 8: 创建 applications 入口

**Files:**
- Create: `bsp/acm32/acm32p4xx-coreboard/applications/SConscript`
- Create: `bsp/acm32/acm32p4xx-coreboard/applications/main.c`

- [ ] **Step 1: applications/SConscript**

```python
from building import *
cwd = GetCurrentDir()
src = Glob('*.c')
CPPPATH = [cwd]
group = DefineGroup('Applications', src, depend = [''], CPPPATH = CPPPATH)
Return('group')
```

- [ ] **Step 2: applications/main.c**

```c
#include <rthw.h>
#include <rtthread.h>
#include "board.h"

#define LED_PIN  PA0

int main(void)
{
    rt_pin_mode(LED_PIN, PIN_MODE_OUTPUT);
    while (1)
    {
        rt_pin_write(LED_PIN, PIN_LOW);
        rt_thread_mdelay(500);
        rt_pin_write(LED_PIN, PIN_HIGH);
        rt_thread_mdelay(500);
    }
    return 0;
}
```

---

### Task 9: 编译验证

- [ ] **Step 1: 尝试编译**

```bash
cd bsp/acm32/acm32p4xx-coreboard
scons -c
scons -j4
```

- [ ] **Step 2: 修复预期问题**

| 预期问题 | 修复 |
|---------|------|
| arm-none-eabi-gcc 未找到 | `export RTT_EXEC_PATH=/path/to/gcc-arm-none-eabi/bin` |
| `uart_is_readable` 不存在 | 用实际函数名替换（如 `!uart_is_rx_fifo_empty()`） |
| `gpio_pull_down` 不存在 | 检查 `hardware/gpio.h` 中是否为此名，可能是 `gpio_pull()` |
| 链接时 `__bss_end` 未定义 | SDK 的 ld 脚本中符号名不同，改为 `__bss_end__` 或 `_end` |
| PendSV_Handler 多重定义 | GCC 启动文件用 `.weak` 声明，RT-Thread `context_gcc.S` 用 `.global` 覆盖 ✅ |

- [ ] **Step 3: 检查产物**

```bash
arm-none-eabi-size rtthread.elf
```

---

### Task 10: Keil MDK 工程 + README

- [ ] **Step 1: 创建 Keil 工程文件**（`project.uvprojx`、`project.uvoptx`）

基于 `acm32f4xx-nucleo/template.uvprojx`，核心差异：
- Device: `ARMCM33_DSP_FP`
- Scatter: `.\libraries\acm32p4\linker\acm32p4xx.sct`
- 启动文件: `libraries\acm32p4\startup\startup_acm32p4xx.s`（Keil 语法版）

- [ ] **Step 2: 创建 README.md**

```markdown
# ACM32P4xx Coreboard BSP for RT-Thread

## 概述
基于 ACM32P4xx (Star-MC1/Cortex-M33) 的 Coreboard 开发板 BSP。

## 硬件资源
| 资源 | 配置 |
|------|------|
| CPU  | Star-MC1, 180MHz |
| SRAM | 128KB |
| Flash| 1MB (Bootloader 占用 8KB) |
| LED  | PA0 |
| UART1| PA9(TX)/PA10(RX), 115200 |

## 编译
```bash
scons
```

## 驱动支持
| 驱动 | 状态 |
|------|------|
| GPIO | ✅ |
| UART | ✅ |
| LED  | ✅ |
```

---

### 自审查

1. **Spec coverage:** 设计文档每个模块都有对应任务 ✅
2. **Placeholder scan:** GCC 启动文件需从 Keil 版补全完整向量表（约 100 个 IRQ），已在 Task 5 步骤中标注；API 函数名（`uart_is_readable` 等）以 SDK 实际头文件为准，在 Task 9 预期问题表中已覆盖
3. **Type consistency:** `uart_config.h` → `drv_uart.c` 类型一致；`board.h` → `board.c` 接口匹配
