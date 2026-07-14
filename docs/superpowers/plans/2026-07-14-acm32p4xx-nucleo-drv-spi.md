# ACM32P4xx-Nucleo SPI 驱动（阶段 1）Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在 ACM32P4xx-Nucleo 上实现 SPI1 Master 轮询 + 软件 CS 的 RT-Thread 标准 SPI 总线驱动，并通过环回测试验收。

**Architecture:** `rt_spi_bus "spi1"` 注册 `rt_spi_ops`（`configure` + `xfer`）；GPIO AF 配置 SCK/MOSI/MISO；软 CS 由框架 `cs_pin` + 驱动 `xfer` 拉高低；HAL 仅调用不修改。

**Tech Stack:** RT-Thread 5.x SPI 框架、`HAL_SPI_*`、Keil MDK5 / `scons --target=mdk5`

## Global Constraints

- 不修改第三方代码（HAL、`components/drivers`、RT-Thread 核心）
- 最小改动；禁止无关重构
- 禁止手改 `rtconfig.h`：用 `.config` + `scons --pyconfig-silent` 生成
- 阶段 1 仅 SPI1、Master、8-bit、轮询、软 CS；无 DMA/Slave/QSPI
- 引脚：PE12=SCK AF3，PE11=MOSI AF3，PE10=MISO AF3，软 CS 默认 PE13
- Spec：`docs/superpowers/specs/2026-07-14-acm32p4xx-nucleo-drv-spi-design.md`

## File Map

| 文件 | 动作 | 职责 |
|------|------|------|
| `bsp/acm32/acm32p4xx-nucleo/drivers/Kconfig` | Modify | `BSP_USING_SPI1` |
| `bsp/acm32/acm32p4xx-nucleo/libraries/Device/acm32p4xx_hal_conf.h` | Modify | 打开 `HAL_SPI_MODULE_ENABLED` |
| `bsp/acm32/acm32p4xx-nucleo/drivers/spi_config.h` | Create | SPI1 总线/引脚宏 |
| `bsp/acm32/acm32p4xx-nucleo/drivers/drv_spi.c` | Create | 驱动主体 |
| `bsp/acm32/acm32p4xx-nucleo/drivers/drv_gpio.c` | Modify | 扩展 pin 表到 GPIOE（含 PE13） |
| `bsp/acm32/acm32p4xx-nucleo/applications/main.c` | Modify | `spi1_loopback_test` MSH |
| `bsp/acm32/acm32p4xx-nucleo/.config` | Modify | 打开 SPI1（验证时） |
| `bsp/acm32/acm32p4xx-nucleo/rtconfig.h` | Generate | 仅通过 scons 生成 |

---

### Task 1: Kconfig + HAL SPI 模块使能 + spi_config.h

**Files:**
- Modify: `bsp/acm32/acm32p4xx-nucleo/drivers/Kconfig`
- Modify: `bsp/acm32/acm32p4xx-nucleo/libraries/Device/acm32p4xx_hal_conf.h`
- Create: `bsp/acm32/acm32p4xx-nucleo/drivers/spi_config.h`

**Interfaces:**
- Produces: `BSP_USING_SPI1`；`HAL_SPI_MODULE_ENABLED`；`SPI1_BUS_CONFIG` 宏

- [ ] **Step 1: 在 Kconfig 的 Onboard Peripheral 菜单中加入 SPI1**

在 `BSP_USING_LPUART2` 配置块之后、`BSP_USING_GPIO1` 之前插入：

```kconfig
    config BSP_USING_SPI1
        bool "Enable SPI1 (PE12/PE11/PE10, soft CS PE13)"
        select RT_USING_SPI
        select RT_USING_PIN
        default n
```

- [ ] **Step 2: 打开 HAL SPI 模块**

将 `acm32p4xx_hal_conf.h` 中：

```c
//#define HAL_SPI_MODULE_ENABLED
```

改为：

```c
#define HAL_SPI_MODULE_ENABLED
```

（与 UART/ETH 一样为板级 HAL 裁剪开关；SPI 驱动依赖此宏编译 `hal_spi.c`。）

- [ ] **Step 3: 创建 spi_config.h**

路径：`bsp/acm32/acm32p4xx-nucleo/drivers/spi_config.h`

```c
/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-14     AisinoChip   ACM32P4xx-Nucleo SPI1 config
 */

#ifndef __SPI_CONFIG_H__
#define __SPI_CONFIG_H__

#include "board.h"
#include "acm32p4xx_hal_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BSP_USING_SPI1
#ifndef SPI1_BUS_CONFIG
#define SPI1_BUS_CONFIG                                                 \
    {                                                                   \
        .bus_name = "spi1",                                             \
        .Instance = SPI1,                                               \
        .irq_type = SPI1_IRQn,                                          \
        .sck_port = GPIOE,  .sck_pin = GPIO_PIN_12, .sck_af = GPIO_FUNCTION_3, \
        .mosi_port = GPIOE, .mosi_pin = GPIO_PIN_11, .mosi_af = GPIO_FUNCTION_3, \
        .miso_port = GPIOE, .miso_pin = GPIO_PIN_10, .miso_af = GPIO_FUNCTION_3, \
    }
#endif
#endif /* BSP_USING_SPI1 */

/* 默认软 CS：PE13 在 pin 表中的 index（Task 2 扩展后：PortE base=64, pin13 → 77） */
#define SPI1_DEFAULT_CS_PIN_INDEX   77

#ifdef __cplusplus
}
#endif

#endif /* __SPI_CONFIG_H__ */
```

- [ ] **Step 4: Commit**

```bash
git add bsp/acm32/acm32p4xx-nucleo/drivers/Kconfig \
        bsp/acm32/acm32p4xx-nucleo/libraries/Device/acm32p4xx_hal_conf.h \
        bsp/acm32/acm32p4xx-nucleo/drivers/spi_config.h
git commit -m "feat(spi): add SPI1 Kconfig, HAL enable, and spi_config"
```

---

### Task 2: 扩展 GPIO pin 表到 GPIOE（软 CS PE13）

**背景：** 当前 `drv_gpio.c` 的 `pins[]` 仅有 PA0–PB15（index 0–31）。`rt_pin_mode(77)` / PE13 不存在会失败。软 CS 与环回测试依赖 PE13。

**Files:**
- Modify: `bsp/acm32/acm32p4xx-nucleo/drivers/drv_gpio.c`

**Interfaces:**
- Produces: pin index 映射  
  - PortA: 0–15，PortB: 16–31，PortC: 32–47，PortD: 48–63，PortE: 64–79  
  - **PE13 = 64 + 13 = 77**（与 `SPI1_DEFAULT_CS_PIN_INDEX` 一致）

- [ ] **Step 1: 扩展 pins[]**

将 `pins[]` 在现有 PB15 之后、`#endif /* defined(BSP_USING_GPIO1) */` 之前追加 C/D/E（保持 index 连续）：

```c
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
```

- [ ] **Step 2: 确认时钟使能覆盖 GPIOE**

在 `drv_gpio.c` 的时钟使能分支中，若尚无 `GPIOE` 分支则补上（与已有 GPIOA/B 风格一致）：

```c
    else if (index->gpio == GPIOE)
    {
        __HAL_RCC_GPIOE_CLK_ENABLE();
    }
```

（若已有完整 switch/if 链，按现有模式补 C/D/E，避免漏时钟。）

- [ ] **Step 3: Commit**

```bash
git add bsp/acm32/acm32p4xx-nucleo/drivers/drv_gpio.c
git commit -m "fix(gpio): extend pin table to GPIOC/D/E for SPI soft CS"
```

---

### Task 3: 实现 drv_spi.c（configure + xfer + 注册 + attach）

**Files:**
- Create: `bsp/acm32/acm32p4xx-nucleo/drivers/drv_spi.c`

**Interfaces:**
- Consumes: `SPI1_BUS_CONFIG` from `spi_config.h`；`HAL_SPI_*`；`rt_spi_bus_register`；`rt_spi_bus_attach_device_cspin`
- Produces:
  - `int rt_hw_spi_init(void)`（`INIT_BOARD_EXPORT`）
  - `rt_err_t rt_hw_spi_device_attach(const char *bus_name, const char *device_name, rt_base_t cs_pin)`

- [ ] **Step 1: 创建 drv_spi.c 骨架与结构体**

```c
/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-14     AisinoChip   ACM32P4xx-Nucleo SPI1 master poll + soft CS
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"
#include "spi_config.h"

#ifdef BSP_USING_SPI1

#include "hal_spi.h"
#include "hal_gpio.h"
#include "hal_rcc.h"

#define DBG_TAG "drv.spi"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define SPI_XFER_TIMEOUT_MS    1000U

struct acm32_spi_config
{
    const char      *bus_name;
    SPI_TypeDef     *Instance;
    IRQn_Type        irq_type;
    GPIO_TypeDef    *sck_port;
    rt_uint32_t      sck_pin;
    rt_uint32_t      sck_af;
    GPIO_TypeDef    *mosi_port;
    rt_uint32_t      mosi_pin;
    rt_uint32_t      mosi_af;
    GPIO_TypeDef    *miso_port;
    rt_uint32_t      miso_pin;
    rt_uint32_t      miso_af;
};

struct acm32_spi
{
    SPI_HandleTypeDef            handle;
    struct acm32_spi_config     *config;
    struct rt_spi_configuration *cfg;
    struct rt_spi_bus            spi_bus;
};

enum
{
    SPI1_INDEX,
    SPI_MAX_INDEX
};

static struct acm32_spi_config spi_config[] =
{
    SPI1_BUS_CONFIG,
};

static struct acm32_spi spi_bus_obj[SPI_MAX_INDEX] = {0};
```

- [ ] **Step 2: 实现 HAL_SPI_MspInit（仅 SPI1 引脚+时钟）**

```c
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef gpio = {0};
    struct acm32_spi *spi;
    struct acm32_spi_config *c;
    int i;

    if (hspi == RT_NULL)
        return;

    spi = RT_NULL;
    for (i = 0; i < SPI_MAX_INDEX; i++)
    {
        if (spi_bus_obj[i].handle.Instance == hspi->Instance ||
            spi_config[i].Instance == hspi->Instance)
        {
            spi = &spi_bus_obj[i];
            break;
        }
    }
    if (spi == RT_NULL)
        return;

    c = spi->config ? spi->config : &spi_config[i];

    if (hspi->Instance == SPI1)
    {
        __HAL_RCC_GPIOE_CLK_ENABLE();
        __HAL_RCC_SPI1_CLK_ENABLE();
    }
    else
    {
        return;
    }

    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Drive = GPIO_DRIVE_LEVEL3;

    gpio.Pin = c->sck_pin;
    gpio.Alternate = c->sck_af;
    HAL_GPIO_Init(c->sck_port, &gpio);

    gpio.Pin = c->mosi_pin;
    gpio.Alternate = c->mosi_af;
    HAL_GPIO_Init(c->mosi_port, &gpio);

    gpio.Pin = c->miso_pin;
    gpio.Alternate = c->miso_af;
    HAL_GPIO_Init(c->miso_port, &gpio);
}
```

注意：`spi_bus_obj[i].config` 在 `rt_hw_spi_init` 赋值后再 `HAL_SPI_Init`；MspInit 被 `HAL_SPI_Init` 调用时 config 应已设置。实现时保证 init 顺序：先填 `config` 与 `handle.Instance`，再在 `configure` 里 `HAL_SPI_Init`。

- [ ] **Step 3: 实现波特率分频与 configure**

```c
static rt_uint32_t acm32_spi_get_src_clk(void)
{
    /* SPI1 on AHB1; stage-1 uses SystemCoreClock as source estimate */
    extern volatile uint32_t SystemCoreClock;
    return SystemCoreClock;
}

static rt_uint32_t acm32_spi_baud_prescaler(rt_uint32_t max_hz)
{
    static const rt_uint32_t table[] = {2, 4, 6, 8, 16, 32, 64, 128, 254};
    rt_uint32_t src = acm32_spi_get_src_clk();
    rt_size_t i;

    if (max_hz == 0)
        max_hz = 1;

    for (i = 0; i < sizeof(table) / sizeof(table[0]); i++)
    {
        if ((src / table[i]) <= max_hz)
            return table[i];
    }
    return 254;
}

static rt_err_t acm32_spi_init(struct acm32_spi *spi_drv, struct rt_spi_configuration *cfg)
{
    SPI_HandleTypeDef *hspi = &spi_drv->handle;
    rt_uint32_t mode = cfg->mode;

    if (cfg->data_width != 8)
        return -RT_EINVAL;

    if (mode & RT_SPI_SLAVE)
        return -RT_EINVAL;

    hspi->Instance = spi_drv->config->Instance;
    hspi->CSx = SPI_CS_CS0; /* HAL_SPI_Init asserts CSx; HW CS pin unused, soft CS separate */
    hspi->Init.SPI_Mode = SPI_MODE_MASTER;
    hspi->Init.X_Mode = SPI_1X_MODE;
    hspi->Init.Slave_SofteCs_En = SPI_SLAVE_SOFT_CS_DISABLE;
    hspi->Init.Master_SShift = SPI_MASTER_SSHIFT_NONE;
    hspi->Init.Sid_Rst_Val = 0;
    hspi->Init.BaudRate_Prescaler = acm32_spi_baud_prescaler(cfg->max_hz);

    if (mode & RT_SPI_MSB)
        hspi->Init.First_Bit = SPI_FIRSTBIT_MSB;
    else
        hspi->Init.First_Bit = SPI_FIRSTBIT_LSB;

    switch (mode & (RT_SPI_CPOL | RT_SPI_CPHA))
    {
    case RT_SPI_MODE_0:
        hspi->Init.SPI_Work_Mode = SPI_WORK_MODE_0;
        break;
    case RT_SPI_MODE_1:
        hspi->Init.SPI_Work_Mode = SPI_WORK_MODE_1;
        break;
    case RT_SPI_MODE_2:
        hspi->Init.SPI_Work_Mode = SPI_WORK_MODE_2;
        break;
    case RT_SPI_MODE_3:
    default:
        hspi->Init.SPI_Work_Mode = SPI_WORK_MODE_3;
        break;
    }

    if (HAL_SPI_Init(hspi) != HAL_OK)
        return -RT_EIO;

    spi_drv->cfg = cfg;
    return RT_EOK;
}

static rt_err_t spi_configure(struct rt_spi_device *device,
                              struct rt_spi_configuration *configuration)
{
    struct acm32_spi *spi_drv;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(configuration != RT_NULL);

    spi_drv = rt_container_of(device->bus, struct acm32_spi, spi_bus);
    return acm32_spi_init(spi_drv, configuration);
}
```

- [ ] **Step 4: 实现 xfer（软 CS + 轮询）**

```c
static rt_ssize_t spixfer(struct rt_spi_device *device, struct rt_spi_message *message)
{
    struct acm32_spi *spi_drv;
    SPI_HandleTypeDef *hspi;
    HAL_StatusTypeDef state = HAL_OK;
    rt_uint8_t *recv_buf;
    const rt_uint8_t *send_buf;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->bus != RT_NULL);
    RT_ASSERT(message != RT_NULL);

    spi_drv = rt_container_of(device->bus, struct acm32_spi, spi_bus);
    hspi = &spi_drv->handle;

    if (message->cs_take && !(device->config.mode & RT_SPI_NO_CS) &&
        (device->cs_pin != PIN_NONE))
    {
        if (device->config.mode & RT_SPI_CS_HIGH)
            rt_pin_write(device->cs_pin, PIN_HIGH);
        else
            rt_pin_write(device->cs_pin, PIN_LOW);
    }

    recv_buf = message->recv_buf;
    send_buf = message->send_buf;

    if (message->length > 0)
    {
        if (send_buf && recv_buf)
        {
            state = HAL_SPI_TransmitReceive(hspi, (uint8_t *)send_buf, recv_buf,
                                            message->length, SPI_XFER_TIMEOUT_MS);
        }
        else if (send_buf)
        {
            state = HAL_SPI_Transmit(hspi, (uint8_t *)send_buf,
                                     message->length, SPI_XFER_TIMEOUT_MS);
        }
        else if (recv_buf)
        {
            /* Master RX needs clocks: full-duplex with 0xFF dummy TX */
            rt_uint8_t *dummy = RT_NULL;
            dummy = (rt_uint8_t *)rt_malloc(message->length);
            if (dummy == RT_NULL)
            {
                state = HAL_ERROR;
            }
            else
            {
                rt_memset(dummy, 0xFF, message->length);
                state = HAL_SPI_TransmitReceive(hspi, dummy, recv_buf,
                                                message->length, SPI_XFER_TIMEOUT_MS);
                rt_free(dummy);
            }
        }
    }

    if (message->cs_release && !(device->config.mode & RT_SPI_NO_CS) &&
        (device->cs_pin != PIN_NONE))
    {
        if (device->config.mode & RT_SPI_CS_HIGH)
            rt_pin_write(device->cs_pin, PIN_LOW);
        else
            rt_pin_write(device->cs_pin, PIN_HIGH);
    }

    if (state != HAL_OK)
        return 0;

    return message->length;
}
```

**HAL 注意：** `HAL_SPI_Transmit`/`Receive` 内部会操作硬件 CS 寄存器位。软 CS 用 GPIO；`hspi->CSx = SPI_CS_CS0` 满足 HAL 断言。若板级未接硬件 CS 脚，硬件 CS 位翻转无外部影响。勿改 HAL。

- [ ] **Step 5: ops 表、总线注册、attach API**

```c
static const struct rt_spi_ops acm32_spi_ops =
{
    .configure = spi_configure,
    .xfer = spixfer,
};

rt_err_t rt_hw_spi_device_attach(const char *bus_name, const char *device_name, rt_base_t cs_pin)
{
    struct rt_spi_device *spi_device;
    rt_err_t result;

    RT_ASSERT(bus_name != RT_NULL);
    RT_ASSERT(device_name != RT_NULL);

    spi_device = (struct rt_spi_device *)rt_malloc(sizeof(struct rt_spi_device));
    if (spi_device == RT_NULL)
        return -RT_ENOMEM;
    rt_memset(spi_device, 0, sizeof(struct rt_spi_device));

    result = rt_spi_bus_attach_device_cspin(spi_device, device_name, bus_name, cs_pin, RT_NULL);
    if (result != RT_EOK)
    {
        rt_free(spi_device);
        LOG_E("%s attach to %s failed: %d", device_name, bus_name, result);
        return result;
    }

    /* Idle CS high for active-low devices (default) */
    if (cs_pin != PIN_NONE)
        rt_pin_write(cs_pin, PIN_HIGH);

    LOG_D("%s attach to %s, cs=%d", device_name, bus_name, (int)cs_pin);
    return RT_EOK;
}

int rt_hw_spi_init(void)
{
    rt_err_t result;
    rt_size_t i;

    for (i = 0; i < sizeof(spi_config) / sizeof(spi_config[0]); i++)
    {
        spi_bus_obj[i].config = &spi_config[i];
        spi_bus_obj[i].handle.Instance = spi_config[i].Instance;
        spi_bus_obj[i].spi_bus.parent.user_data = &spi_config[i];

        result = rt_spi_bus_register(&spi_bus_obj[i].spi_bus,
                                     spi_config[i].bus_name,
                                     &acm32_spi_ops);
        RT_ASSERT(result == RT_EOK);
        LOG_D("%s bus register done", spi_config[i].bus_name);
    }
    return RT_EOK;
}
INIT_BOARD_EXPORT(rt_hw_spi_init);

#endif /* BSP_USING_SPI1 */
```

- [ ] **Step 6: 静态检查**

确认：
- `#ifdef BSP_USING_SPI1` 包住全部实现
- 无未使用的 DMA 代码
- `GPIO_FUNCTION_3` / `GPIO_DRIVE_LEVEL3` 与 `drv_uart` 一致

- [ ] **Step 7: Commit**

```bash
git add bsp/acm32/acm32p4xx-nucleo/drivers/drv_spi.c
git commit -m "feat(spi): add SPI1 master poll driver with soft CS"
```

---

### Task 4: 环回 MSH 测试 + 打开配置并编译验证

**Files:**
- Modify: `bsp/acm32/acm32p4xx-nucleo/applications/main.c`
- Modify: `bsp/acm32/acm32p4xx-nucleo/.config`（仅验证用）
- Generate: `bsp/acm32/acm32p4xx-nucleo/rtconfig.h`

**Interfaces:**
- Consumes: `rt_hw_spi_device_attach`；`SPI1_DEFAULT_CS_PIN_INDEX`（或字面量 77）；`rt_spi_transfer` / `rt_spi_configure`

- [ ] **Step 1: 添加 spi1_loopback_test**

在 `main.c` 中增加（保留现有 uart2 测试）：

```c
#include "spi_config.h"

#ifdef BSP_USING_SPI1
int spi1_loopback_test(int argc, char **argv)
{
    struct rt_spi_device *dev;
    struct rt_spi_configuration cfg;
    rt_uint8_t tx[16];
    rt_uint8_t rx[16];
    rt_err_t ret;
    int i;
    int pass = 1;

    RT_UNUSED(argc);
    RT_UNUSED(argv);

    ret = rt_hw_spi_device_attach("spi1", "spi10", SPI1_DEFAULT_CS_PIN_INDEX);
    if (ret != RT_EOK)
    {
        /* 可能已 attach：继续 find */
        rt_kprintf("attach ret=%d (may already exist)\n", ret);
    }

    dev = (struct rt_spi_device *)rt_device_find("spi10");
    if (dev == RT_NULL)
    {
        rt_kprintf("spi10 not found\n");
        return -1;
    }

    cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB;
    cfg.data_width = 8;
    cfg.max_hz = 1000000;
    ret = rt_spi_configure(dev, &cfg);
    if (ret != RT_EOK)
    {
        rt_kprintf("spi configure failed: %d\n", ret);
        return -1;
    }

    for (i = 0; i < 16; i++)
        tx[i] = (rt_uint8_t)(0xA0 + i);
    rt_memset(rx, 0, sizeof(rx));

    rt_kprintf("SPI1 loopback: short MOSI(PE11)-MISO(PE10), then check pattern\n");
    if (rt_spi_transfer(dev, tx, rx, 16) != 16)
    {
        rt_kprintf("transfer failed\n");
        return -1;
    }

    for (i = 0; i < 16; i++)
    {
        if (rx[i] != tx[i])
        {
            pass = 0;
            rt_kprintf("mismatch @%d tx=%02X rx=%02X\n", i, tx[i], rx[i]);
        }
    }

    rt_kprintf("SPI1 loopback %s\n", pass ? "PASS" : "FAIL");
    return pass ? 0 : -1;
}
MSH_CMD_EXPORT(spi1_loopback_test, "SPI1 MOSI-MISO loopback test");
#endif
```

在 `main()` 打印一行提示：`Run 'spi1_loopback_test' after shorting MOSI-MISO`。

- [ ] **Step 2: 打开 .config 中的 SPI1**

在 `bsp/acm32/acm32p4xx-nucleo/.config` 中设置（或 menuconfig）：

```
CONFIG_BSP_USING_SPI1=y
```

并确保 `CONFIG_RT_USING_SPI=y`（Kconfig select 通常会带上）。

- [ ] **Step 3: 生成 rtconfig.h 并生成 Keil 工程**

```powershell
chcp 65001 | Out-Null
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
cd bsp/acm32/acm32p4xx-nucleo
scons --pyconfig-silent
scons --target=mdk5
```

Expected：
- `rtconfig.h` 含 `#define BSP_USING_SPI1` 与 `#define RT_USING_SPI`
- `scons --target=mdk5` 成功（`Keil-MDK project has generated successfully`）

- [ ] **Step 4: Keil 全量编译（或本机等价）**

在 Keil 中 Build；Expected：**0 Error**。

若链接缺 `HAL_SPI_*`：确认 `hal_spi.c` 在工程中且 `HAL_SPI_MODULE_ENABLED` 已定义。

- [ ] **Step 5: 板级环回（可选硬件）**

1. 短接 PE11(MOSI)–PE10(MISO)
2. 烧录后 MSH：`spi1_loopback_test`
3. Expected：`SPI1 loopback PASS`

- [ ] **Step 6: Commit**

```bash
git add bsp/acm32/acm32p4xx-nucleo/applications/main.c \
        bsp/acm32/acm32p4xx-nucleo/.config \
        bsp/acm32/acm32p4xx-nucleo/rtconfig.h
git commit -m "test(spi): add spi1 loopback MSH and enable BSP_USING_SPI1"
```

不要提交无关 `project.uvprojx` 噪音除非团队惯例要求。

---

## Plan Self-Review

| Spec 要求 | 对应 Task |
|-----------|-----------|
| SPI1 Master 轮询 | Task 3 |
| 软 CS + attach | Task 3 + Task 2（PE13 pin） |
| 引脚 PE10/11/12 AF3 | Task 1 config + Task 3 MspInit |
| Kconfig / HAL 使能 | Task 1 |
| 8-bit only | Task 3 configure `-RT_EINVAL` |
| 环回测试 | Task 4 |
| 不改 HAL/内核 | 全局约束 |
| rtconfig 生成 | Task 4 Step 3 |
| 无 DMA/Slave/QSPI | 未安排任务 |

**风险备忘：**
1. HAL 内部硬件 CS 位：软 CS 用 GPIO；`CSx=SPI_CS_CS0` 仅满足 HAL
2. GPIO 表必须含 PortE，否则 CS 失败
3. RX-only 用 dummy `TransmitReceive`，避免 Master 无时钟

**Placeholder scan：** 无 TBD；关键代码均已给出。
