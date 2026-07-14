# ACM32P4xx-Nucleo SPI 多实例（阶段 3）Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将 `drv_spi` 从 SPI1 单例升级为表驱动 SPI1+SPI2，共用轮询/半双工 TX DMA 策略，SPI2 可选独立 DMA 通道。

**Architecture:** 条件编译展开 `spi_config[]` / `spi_bus_obj[]`；每实例持有 `config` 与可选 `dma_tx_cfg`/`dma_rx_cfg` 指针；MspInit/IRQ 按 Instance 分发；xfer 只看 `spi_dma_flag` 与 message 形态。

**Tech Stack:** RT-Thread SPI、ACM32P4 HAL SPI/DMA、Keil MDK5

## Global Constraints

- Spec: `docs/superpowers/specs/2026-07-14-acm32p4xx-nucleo-spi-multi-design.md`
- 不修改 HAL / `components/drivers`
- 最小改动；`rtconfig.h` 仅由 `.config` + `scons --pyconfig-silent` 生成
- SPI2 引脚：SCK PB13 AF3、MOSI PB15 AF3、MISO PB14 AF3、软 CS PB12（pin index **28**）
- SPI2 DMA：TX `DMA1_Channel0` Req `DMA1_REQ_SPI2_TX`(3)，RX `DMA1_Channel1` Req `DMA1_REQ_SPI2_RX`(4)
- 传输策略与阶段 2 相同：full-duplex 轮询；半双工 TX≥32 DMA；RX 静态 dummy 分块
- SPI1 回归：`spi1_loopback_test` / `spi1_dma_test` 必须仍 PASS
- 默认 `.config` **不必**打开 SPI2（default n）；验证时可临时打开
- 文件外层编译条件：`#if defined(BSP_USING_SPI1) || defined(BSP_USING_SPI2)`（当前错误地只用 `BSP_USING_SPI1`）

## File Map

| 文件 | 动作 |
|------|------|
| `drivers/Kconfig` | SPI2 + SPI2_DMA |
| `drivers/spi_config.h` | SPI2 bus/DMA/CS 宏 |
| `drivers/drv_spi.c` | 多实例重构核心 |
| `applications/main.c` | 可选 spi2 环回测试 |
| `.config` / `rtconfig.h` | 仅验证任务需要时打开 SPI2 |

---

### Task 1: Kconfig + spi_config.h 增加 SPI2

**Files:**
- Modify: `bsp/acm32/acm32p4xx-nucleo/drivers/Kconfig`
- Modify: `bsp/acm32/acm32p4xx-nucleo/drivers/spi_config.h`

**Interfaces:**
- Produces: `BSP_USING_SPI2`、`BSP_USING_SPI2_DMA`；`SPI2_BUS_CONFIG`；`SPI2_DMA_TX_CONFIG` / `SPI2_DMA_RX_CONFIG`；`SPI2_DEFAULT_CS_PIN_INDEX 28`

- [ ] **Step 1: Kconfig（SPI1_DMA 块之后）**

```kconfig
    config BSP_USING_SPI2
        bool "Enable SPI2 (PB13/PB15/PB14, soft CS PB12)"
        select RT_USING_SPI
        select RT_USING_PIN
        default n

    config BSP_USING_SPI2_DMA
        bool "Enable SPI2 DMA (TX: DMA1_CH0, RX: DMA1_CH1)"
        depends on BSP_USING_SPI2
        default n
        help
          Same policy as SPI1 DMA: half-duplex TX >= 32 bytes.
          TX DMA1_CH0 Req SPI2_TX(3), RX DMA1_CH1 Req SPI2_RX(4).
          Avoid other DMA1_CH0/CH1 users (e.g. UART4 RX if using DMA1_CH0).
```

- [ ] **Step 2: spi_config.h 追加 SPI2**

在文件中 `SPI1` 块之后、`SPI_DMA_MIN_SIZE` 之前增加：

```c
#ifdef BSP_USING_SPI2
#ifndef SPI2_BUS_CONFIG
#define SPI2_BUS_CONFIG                                                 \
    {                                                                   \
        .bus_name = "spi2",                                             \
        .Instance = SPI2,                                               \
        .irq_type = SPI2_IRQn,                                          \
        .sck_port = GPIOB,  .sck_pin = GPIO_PIN_13, .sck_af = GPIO_FUNCTION_3, \
        .mosi_port = GPIOB, .mosi_pin = GPIO_PIN_15, .mosi_af = GPIO_FUNCTION_3, \
        .miso_port = GPIOB, .miso_pin = GPIO_PIN_14, .miso_af = GPIO_FUNCTION_3, \
    }
#endif

#ifdef BSP_USING_SPI2_DMA
#ifndef SPI2_DMA_TX_CONFIG
#define SPI2_DMA_TX_CONFIG \
    { .Instance = DMA1_Channel0, .Channel = 0, \
      .irq = DMA1_CH0_IRQn, .reqid = DMA1_REQ_SPI2_TX }
#endif
#ifndef SPI2_DMA_RX_CONFIG
#define SPI2_DMA_RX_CONFIG \
    { .Instance = DMA1_Channel1, .Channel = 1, \
      .irq = DMA1_CH1_IRQn, .reqid = DMA1_REQ_SPI2_RX }
#endif
#endif /* BSP_USING_SPI2_DMA */
#endif /* BSP_USING_SPI2 */

/* PB12: PortB base=16, pin12 → 28 */
#define SPI2_DEFAULT_CS_PIN_INDEX   28
```

保留现有 `SPI1_*` 与 `SPI_DMA_MIN_SIZE`、`SPI1_DEFAULT_CS_PIN_INDEX`。

- [ ] **Step 3: Commit**

```bash
git add bsp/acm32/acm32p4xx-nucleo/drivers/Kconfig \
        bsp/acm32/acm32p4xx-nucleo/drivers/spi_config.h
git commit -m "feat(spi): add SPI2 Kconfig and pin/DMA macros"
```

---

### Task 2: drv_spi 多实例表 + MspInit/时钟 + IRQ

**Files:**
- Modify: `bsp/acm32/acm32p4xx-nucleo/drivers/drv_spi.c`

**Interfaces:**
- Consumes: `SPI1_BUS_CONFIG` / `SPI2_BUS_CONFIG`
- Produces: 可注册 `spi1` 与/或 `spi2`；MspInit 支持 SPI2；SPI2_IRQHandler

- [ ] **Step 1: 放宽编译门控**

将文件顶部与底部：

```c
#ifdef BSP_USING_SPI1
...
#endif /* BSP_USING_SPI1 */
```

改为：

```c
#if defined(BSP_USING_SPI1) || defined(BSP_USING_SPI2)
...
#endif /* BSP_USING_SPI1 || BSP_USING_SPI2 */
```

`#include "hal_dma.h"` 条件改为：

```c
#if defined(BSP_USING_SPI1_DMA) || defined(BSP_USING_SPI2_DMA)
#include "hal_dma.h"
#endif
```

同理所有 `BSP_USING_SPI1_DMA` 仅用于「本文件是否有 DMA 代码」处，改为：

```c
#if defined(BSP_USING_SPI1_DMA) || defined(BSP_USING_SPI2_DMA)
#define ACM32_SPI_USING_DMA
#endif
```

后续 DMA 结构体/函数用 `#ifdef ACM32_SPI_USING_DMA`。

- [ ] **Step 2: 索引与 config 表**

```c
enum
{
#ifdef BSP_USING_SPI1
    SPI1_INDEX,
#endif
#ifdef BSP_USING_SPI2
    SPI2_INDEX,
#endif
    SPI_MAX_INDEX
};

static struct acm32_spi_config spi_config[] =
{
#ifdef BSP_USING_SPI1
    SPI1_BUS_CONFIG,
#endif
#ifdef BSP_USING_SPI2
    SPI2_BUS_CONFIG,
#endif
};
```

- [ ] **Step 3: MspInit 支持 SPI2**

将：

```c
    if (hspi->Instance == SPI1)
    {
        __HAL_RCC_GPIOE_CLK_ENABLE();
        __HAL_RCC_SPI1_CLK_ENABLE();
    }
    else
    {
        return;
    }
```

替换为：

```c
    if (hspi->Instance == SPI1)
    {
        __HAL_RCC_GPIOE_CLK_ENABLE();
        __HAL_RCC_SPI1_CLK_ENABLE();
    }
#ifdef BSP_USING_SPI2
    else if (hspi->Instance == SPI2)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_SPI2_CLK_ENABLE();
    }
#endif
    else
    {
        return;
    }
```

GPIO 初始化仍用 `c->sck/mosi/miso_*`（已按 config 填好）。

- [ ] **Step 4: SPI2_IRQHandler**

```c
#ifdef BSP_USING_SPI1
void SPI1_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_SPI_IRQHandler(&spi_bus_obj[SPI1_INDEX].handle);
    rt_interrupt_leave();
}
#endif

#ifdef BSP_USING_SPI2
void SPI2_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_SPI_IRQHandler(&spi_bus_obj[SPI2_INDEX].handle);
    rt_interrupt_leave();
}
#endif
```

删除旧的「仅 SPI1」重复 `#ifdef` 包裹错误（当前文件末尾 `SPI1_IRQHandler` 在 `#ifdef BSP_USING_SPI1` 内且外层又是 SPI1——整理为上述）。

- [ ] **Step 5: 静态检查**

- 仅 `BSP_USING_SPI1` 时编译语义与改前一致  
- 仅 `BSP_USING_SPI2`（无 SPI1）也应能编译（索引/表非空）

- [ ] **Step 6: Commit**

```bash
git add bsp/acm32/acm32p4xx-nucleo/drivers/drv_spi.c
git commit -m "refactor(spi): table-driven multi-instance SPI1/SPI2 bus"
```

---

### Task 3: 每实例 DMA 配置（去硬编码 spi1_dma_*）

**Files:**
- Modify: `bsp/acm32/acm32p4xx-nucleo/drivers/drv_spi.c`

**Interfaces:**
- Consumes: `SPI1_DMA_TX_CONFIG` / `SPI2_DMA_TX_CONFIG` 等
- Produces: `acm32_spi_dma_init` 按 `spi_drv->config->Instance` 选通道与 DMA1/2 时钟

- [ ] **Step 1: 扩展 config 或旁路表**

**推荐（改动小）：** 在 `struct acm32_spi` 增加：

```c
#ifdef ACM32_SPI_USING_DMA
    const struct acm32_spi_dma_config *dma_tx_cfg;
    const struct acm32_spi_dma_config *dma_rx_cfg;
    DMA_HandleTypeDef            dma_tx;
    DMA_HandleTypeDef            dma_rx;
    rt_uint8_t                   spi_dma_flag;
#endif
```

删除全局 `static const ... spi1_dma_tx/rx` 仅 SPI1 绑定。

在 `rt_hw_spi_init` 循环中赋值：

```c
#ifdef ACM32_SPI_USING_DMA
        spi_bus_obj[i].dma_tx_cfg = RT_NULL;
        spi_bus_obj[i].dma_rx_cfg = RT_NULL;
        spi_bus_obj[i].spi_dma_flag = 0;
#ifdef BSP_USING_SPI1_DMA
        if (spi_config[i].Instance == SPI1)
        {
            static const struct acm32_spi_dma_config s1tx = SPI1_DMA_TX_CONFIG;
            static const struct acm32_spi_dma_config s1rx = SPI1_DMA_RX_CONFIG;
            spi_bus_obj[i].dma_tx_cfg = &s1tx;
            spi_bus_obj[i].dma_rx_cfg = &s1rx;
        }
#endif
#ifdef BSP_USING_SPI2_DMA
        if (spi_config[i].Instance == SPI2)
        {
            static const struct acm32_spi_dma_config s2tx = SPI2_DMA_TX_CONFIG;
            static const struct acm32_spi_dma_config s2rx = SPI2_DMA_RX_CONFIG;
            spi_bus_obj[i].dma_tx_cfg = &s2tx;
            spi_bus_obj[i].dma_rx_cfg = &s2rx;
        }
#endif
#endif
```

（`static const` 放函数内需 C99；若编译器不喜，改为文件作用域 `spi1_dma_tx`/`spi2_dma_tx` 两套常量。）

- [ ] **Step 2: 改写 acm32_spi_dma_init**

```c
static rt_err_t acm32_spi_dma_init(struct acm32_spi *spi_drv)
{
    if (spi_drv->dma_tx_cfg == RT_NULL || spi_drv->dma_rx_cfg == RT_NULL)
        return RT_EOK; /* 本总线未开 DMA */

    if (spi_drv->spi_dma_flag & (SPI_USING_TX_DMA_FLAG | SPI_USING_RX_DMA_FLAG))
        return RT_EOK;

    /* SPI1 DMA 在 DMA2；SPI2 DMA 在 DMA1 */
    if ((rt_uint32_t)spi_drv->dma_tx_cfg->Instance < (rt_uint32_t)DMA2_Channel0)
        __HAL_RCC_DMA1_CLK_ENABLE();
    else
        __HAL_RCC_DMA2_CLK_ENABLE();

    acm32_spi_dma_fill(&spi_drv->dma_tx, spi_drv->dma_tx_cfg, DMA_DATAFLOW_M2P);
    if (HAL_DMA_Init(&spi_drv->dma_tx) != HAL_OK)
        return -RT_EIO;

    acm32_spi_dma_fill(&spi_drv->dma_rx, spi_drv->dma_rx_cfg, DMA_DATAFLOW_P2M);
    if (HAL_DMA_Init(&spi_drv->dma_rx) != HAL_OK)
        return -RT_EIO;

    spi_drv->handle.HDMA_Tx = &spi_drv->dma_tx;
    spi_drv->handle.HDMA_Rx = &spi_drv->dma_rx;
    spi_drv->spi_dma_flag = SPI_USING_TX_DMA_FLAG | SPI_USING_RX_DMA_FLAG;

    NVIC_SetPriority(spi_drv->config->irq_type, 2);
    NVIC_EnableIRQ(spi_drv->config->irq_type);
    return RT_EOK;
}
```

`acm32_spi_init` 末尾：

```c
#ifdef ACM32_SPI_USING_DMA
    if (acm32_spi_dma_init(spi_drv) != RT_EOK)
        return -RT_EIO;
#endif
```

`spixfer` 中 DMA 分支条件保持：

```c
#ifdef ACM32_SPI_USING_DMA
        if ((spi_drv->spi_dma_flag & SPI_USING_TX_DMA_FLAG) &&
            send_buf && !recv_buf && message->length >= SPI_DMA_MIN_SIZE)
```

- [ ] **Step 3: Commit**

```bash
git add bsp/acm32/acm32p4xx-nucleo/drivers/drv_spi.c
git commit -m "feat(spi): per-instance DMA config for SPI1 and SPI2"
```

---

### Task 4: 测试 + 可选打开 SPI2 编译验证

**Files:**
- Modify: `bsp/acm32/acm32p4xx-nucleo/applications/main.c`
- Optionally: `.config` / `rtconfig.h`（验证 SPI2 时）

**Interfaces:**
- Consumes: `SPI2_DEFAULT_CS_PIN_INDEX`；`rt_hw_spi_device_attach`

- [ ] **Step 1: spi2_loopback_test（镜像 spi1，短接 PB15–PB14）**

```c
#ifdef BSP_USING_SPI2
int spi2_loopback_test(int argc, char **argv)
{
    /* 与 spi1_loopback_test 相同逻辑：
       attach("spi2","spi20", SPI2_DEFAULT_CS_PIN_INDEX)
       configure mode0 1MHz
       len default 16, max 512
       rt_spi_transfer 校验
    */
}
MSH_CMD_EXPORT(spi2_loopback_test, "SPI2 MOSI-MISO loopback [len]");
#endif
```

`main()` 提示：

```c
#ifdef BSP_USING_SPI2
    rt_kprintf("Run 'spi2_loopback_test' after shorting MOSI(PB15)-MISO(PB14)\n");
#endif
```

- [ ] **Step 2: 编译矩阵（最低）**

**矩阵 A（默认回归，必做）：** 保持现有 `.config`（SPI1 + SPI1_DMA，无 SPI2）

```powershell
cd bsp/acm32/acm32p4xx-nucleo
scons --pyconfig-silent
scons --target=mdk5
# Keil build → 0 Error
```

板测：`spi1_loopback_test`、`spi1_dma_test` → PASS

**矩阵 B（SPI2 冒烟，推荐做一次）：** 临时 `.config`：

```
CONFIG_BSP_USING_SPI2=y
# CONFIG_BSP_USING_SPI2_DMA is not set   # 或 =y 再编一次
```

```powershell
scons --pyconfig-silent
scons --target=mdk5
# Keil 0 Error
```

若有硬件：`spi2_loopback_test`

验证后可将 `.config` 恢复为「仅 SPI1」再生成 `rtconfig.h`，避免默认固件强制 SPI2——**或** 保留 SPI2=n 的提交，矩阵 B 仅本地验证不提交 `.config` 变更。

**计划约定：** 提交 `main.c` 中的 spi2 测试代码；**默认不提交** `BSP_USING_SPI2=y` 到 `.config`（保持 default n）。若你希望仓库默认开 SPI2，在 Task 4 明确改 `.config`。

- [ ] **Step 3: Commit**

```bash
git add bsp/acm32/acm32p4xx-nucleo/applications/main.c
# 仅当故意默认启用 SPI2 时再 add .config rtconfig.h
git commit -m "test(spi): add spi2 loopback MSH command"
```

---

## Plan Self-Review

| Spec 要求 | Task |
|-----------|------|
| 表驱动 SPI1+SPI2 | 2 |
| SPI2 引脚 PB13/15/14 CS28 | 1 |
| SPI2 Kconfig default n | 1 |
| SPI2 DMA DMA1_CH0/1 | 1, 3 |
| 去硬编码 spi1_dma | 3 |
| MspInit SPI2 时钟 | 2 |
| SPI2_IRQHandler | 2 |
| xfer 策略不变 | 3（不改决策表） |
| SPI1 回归 | 4 |
| spi2 可选测试 | 4 |
| 不改 HAL | 全局 |

**风险：**
1. 外层 `#ifdef BSP_USING_SPI1` 漏改会导致开 SPI2 无驱动  
2. DMA 时钟：SPI2 必须 `DMA1` 而非写死 `DMA2`  
3. `SPI2_INDEX` 在仅 SPI2 时为 0，IRQ 用 `SPI2_INDEX` 正确  

**Placeholder scan：** 无 TBD；代码块完整。

---

## 成功标准

- [ ] SPI1  alone：行为与阶段 2 一致  
- [ ] SPI1+SPI2：两总线可注册  
- [ ] SPI2 DMA 可选且通道独立  
- [ ] Keil 0 Error  
- [ ] 未修改 HAL  
