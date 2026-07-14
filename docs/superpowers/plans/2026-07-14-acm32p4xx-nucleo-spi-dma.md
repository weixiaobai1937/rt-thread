# ACM32P4xx-Nucleo SPI Master DMA（阶段 2）Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为 SPI1 增加可选半双工 DMA（≥32 字节），全双工与短包保持轮询，软 CS 与同步 xfer 语义不变。

**Architecture:** Kconfig `BSP_USING_SPI1_DMA`；TX=`DMA2_CH1` Req1，RX=`DMA2_CH3` Req2；`configure` 链接 `HDMA_Tx/Rx`；`xfer` 按决策表分支；HAL `Transmit_DMA`/`Receive_DMA` + **SPI1 IRQ** 调 `HAL_SPI_IRQHandler` 完成状态机；xfer 内轮询 `GetTxState`/`GetRxState`。

**Tech Stack:** RT-Thread SPI、ACM32P4 HAL SPI/DMA、Keil MDK5

## Global Constraints

- Spec: `docs/superpowers/specs/2026-07-14-acm32p4xx-nucleo-spi-dma-design.md`
- 不修改 HAL 源码 / RT-Thread SPI 框架
- 最小改动；`rtconfig.h` 仅由 `.config` + `scons --pyconfig-silent` 生成
- Full-duplex（send&&recv）**一律轮询**；半双工 length≥32 才 DMA
- 阈值固定 `SPI_DMA_MIN_SIZE = 32`
- 通道：TX `DMA2_Channel1` / RX `DMA2_Channel3`；避开 UART2 默认 DMA
- Dummy RX：**禁止** 按 length 无界 `rt_malloc`；用静态可复用 0xFF 缓冲
- 阶段 1 轮回环测必须继续 PASS

## File Map

| 文件 | 动作 |
|------|------|
| `drivers/Kconfig` | 增加 `BSP_USING_SPI1_DMA` |
| `drivers/spi_config.h` | DMA 实例/通道/ReqID/IRQ 宏 |
| `drivers/drv_spi.c` | DMA init、IRQ、xfer 分支、等待 |
| `applications/main.c` | 扩展测试（长度参数 / dma 路径） |
| `.config` / `rtconfig.h` | 验证时打开 DMA 并生成 |

---

### Task 1: Kconfig + spi_config DMA 宏

**Files:**
- Modify: `bsp/acm32/acm32p4xx-nucleo/drivers/Kconfig`
- Modify: `bsp/acm32/acm32p4xx-nucleo/drivers/spi_config.h`

**Interfaces:**
- Produces: `BSP_USING_SPI1_DMA`；`SPI1_DMA_TX_*` / `SPI1_DMA_RX_*` 宏

- [ ] **Step 1: Kconfig**

在 `BSP_USING_SPI1` 之后插入：

```kconfig
    config BSP_USING_SPI1_DMA
        bool "Enable SPI1 DMA (TX: DMA2_CH1, RX: DMA2_CH3)"
        depends on BSP_USING_SPI1
        default n
        help
          Half-duplex transfers >= 32 bytes use DMA.
          Full-duplex always polls (HAL has no TransmitReceive_DMA).
          Avoid enabling with UART3 DMA RX (DMA2_CH3 conflict).
```

- [ ] **Step 2: spi_config.h 追加 DMA 宏**

在 `#endif /* BSP_USING_SPI1 */` 之前（仍在 `BSP_USING_SPI1` 内）或文件中增加：

```c
#ifdef BSP_USING_SPI1_DMA
#ifndef SPI1_DMA_TX_CONFIG
#define SPI1_DMA_TX_CONFIG \
    { .Instance = DMA2_Channel1, .Channel = 1, \
      .irq = DMA2_CH1_IRQn, .reqid = DMA2_REQ_SPI1_TX }
#endif
#ifndef SPI1_DMA_RX_CONFIG
#define SPI1_DMA_RX_CONFIG \
    { .Instance = DMA2_Channel3, .Channel = 3, \
      .irq = DMA2_CH3_IRQn, .reqid = DMA2_REQ_SPI1_RX }
#endif
#endif /* BSP_USING_SPI1_DMA */

#define SPI_DMA_MIN_SIZE    32
```

（宏结构字段名可在 Task 2 与 `struct` 对齐；Req 常量来自 `hal_dma.h`。）

- [ ] **Step 3: Commit**

```bash
git add bsp/acm32/acm32p4xx-nucleo/drivers/Kconfig \
        bsp/acm32/acm32p4xx-nucleo/drivers/spi_config.h
git commit -m "feat(spi): add SPI1 DMA Kconfig and channel macros"
```

---

### Task 2: DMA 初始化 + SPI1 IRQ（完成状态机）

**背景（必读）：**  
`HAL_SPI_Transmit_DMA` / `Receive_DMA` 置 `TxState`/`RxState` 为 busy，并 **使能 SPI batch-done 中断**。状态回到 `SPI_TX_STATE_IDLE` / `SPI_RX_STATE_IDLE` 发生在 **`HAL_SPI_IRQHandler`** 内。阶段 1 未启用 SPI1 NVIC——DMA 路径 **必须** 增加：

```c
void SPI1_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_SPI_IRQHandler(&spi_bus_obj[SPI1_INDEX].handle);
    rt_interrupt_leave();
}
```

并在 DMA 配置路径 `NVIC_EnableIRQ(SPI1_IRQn)`。

**Files:**
- Modify: `bsp/acm32/acm32p4xx-nucleo/drivers/drv_spi.c`

**Interfaces:**
- Consumes: `SPI1_DMA_TX_CONFIG` / `SPI1_DMA_RX_CONFIG` / `SPI_DMA_MIN_SIZE`
- Produces: `acm32_spi_dma_init()`；`spi_drv->handle.HDMA_Tx/Rx` 有效

- [ ] **Step 1: 扩展结构体**

```c
#define SPI_USING_RX_DMA_FLAG   (1U << 0)
#define SPI_USING_TX_DMA_FLAG   (1U << 1)

struct acm32_spi_dma_config
{
    DMA_Channel_TypeDef *Instance;
    rt_uint32_t          Channel;
    IRQn_Type            irq;
    rt_uint32_t          reqid;
};

struct acm32_spi
{
    SPI_HandleTypeDef            handle;
    struct acm32_spi_config     *config;
    struct rt_spi_configuration *cfg;
    struct rt_spi_bus            spi_bus;
#ifdef BSP_USING_SPI1_DMA
    DMA_HandleTypeDef            dma_tx;
    DMA_HandleTypeDef            dma_rx;
    rt_uint8_t                   spi_dma_flag;
#endif
};
```

`spi_config` 表可继续只用总线宏；DMA 通道用 `SPI1_DMA_TX_CONFIG` 静态常量：

```c
#ifdef BSP_USING_SPI1_DMA
static const struct acm32_spi_dma_config spi1_dma_tx = SPI1_DMA_TX_CONFIG;
static const struct acm32_spi_dma_config spi1_dma_rx = SPI1_DMA_RX_CONFIG;
#endif
```

- [ ] **Step 2: 实现 `acm32_spi_dma_config_channel` + `acm32_spi_dma_init`**

```c
#ifdef BSP_USING_SPI1_DMA
#include "hal_dma.h"

static void acm32_spi_dma_fill(DMA_HandleTypeDef *hdma,
                               const struct acm32_spi_dma_config *cfg,
                               rt_uint32_t dataflow)
{
    hdma->Instance = cfg->Instance;
    hdma->Channel  = cfg->Channel;
    hdma->Init.Mode       = DMA_MODE_NORMAL;
    hdma->Init.DataFlow   = dataflow;
    hdma->Init.ReqID      = cfg->reqid;
    hdma->Init.SrcIncDec  = (dataflow == DMA_DATAFLOW_M2P) ?
                            DMA_SRCINCDEC_INC : DMA_SRCINCDEC_DISABLE;
    hdma->Init.DestIncDec = (dataflow == DMA_DATAFLOW_M2P) ?
                            DMA_DESTINCDEC_DISABLE : DMA_DESTINCDEC_INC;
    hdma->Init.SrcWidth   = DMA_SRCWIDTH_BYTE;
    hdma->Init.DestWidth  = DMA_DESTWIDTH_BYTE;
    hdma->Init.SrcBurst   = DMA_SRCBURST_1;
    hdma->Init.DestBurst  = DMA_DESTBURST_1;
    hdma->Init.SrcMaster  = DMA_SRCMASTER_1;
    hdma->Init.DestMaster = DMA_DESTMASTER_1;
    hdma->Init.Lock       = 0;
    hdma->Init.NextMaster = 0;
}

static rt_err_t acm32_spi_dma_init(struct acm32_spi *spi_drv)
{
    __HAL_RCC_DMA2_CLK_ENABLE();

    acm32_spi_dma_fill(&spi_drv->dma_tx, &spi1_dma_tx, DMA_DATAFLOW_M2P);
    if (HAL_DMA_Init(&spi_drv->dma_tx) != HAL_OK)
        return -RT_EIO;

    acm32_spi_dma_fill(&spi_drv->dma_rx, &spi1_dma_rx, DMA_DATAFLOW_P2M);
    if (HAL_DMA_Init(&spi_drv->dma_rx) != HAL_OK)
        return -RT_EIO;

    spi_drv->handle.HDMA_Tx = &spi_drv->dma_tx;
    spi_drv->handle.HDMA_Rx = &spi_drv->dma_rx;
    spi_drv->spi_dma_flag = SPI_USING_TX_DMA_FLAG | SPI_USING_RX_DMA_FLAG;

    /* SPI batch-done IRQ required for HAL DMA completion path */
    NVIC_SetPriority(spi_drv->config->irq_type, 2);
    NVIC_EnableIRQ(spi_drv->config->irq_type);

    return RT_EOK;
}
#endif
```

在 `acm32_spi_init` 末尾 `HAL_SPI_Init` 成功后：

```c
#ifdef BSP_USING_SPI1_DMA
    if (acm32_spi_dma_init(spi_drv) != RT_EOK)
        return -RT_EIO;
#endif
```

- [ ] **Step 3: SPI1_IRQHandler**

```c
#ifdef BSP_USING_SPI1
void SPI1_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_SPI_IRQHandler(&spi_bus_obj[SPI1_INDEX].handle);
    rt_interrupt_leave();
}
#endif
```

（放在 `#ifdef BSP_USING_SPI1` 内；无 DMA 时也可启用 IRQ 无害，或仅 `#ifdef BSP_USING_SPI1_DMA` 注册——**推荐仅 DMA 时 EnableIRQ**，但 handler 符号可始终提供以免 weak 空处理。）

- [ ] **Step 4: Commit**

```bash
git add bsp/acm32/acm32p4xx-nucleo/drivers/drv_spi.c
git commit -m "feat(spi): init SPI1 DMA handles and SPI batch IRQ"
```

---

### Task 3: xfer DMA 分支 + 完成等待 + RX dummy

**Files:**
- Modify: `bsp/acm32/acm32p4xx-nucleo/drivers/drv_spi.c`（`spixfer`）

**Interfaces:**
- Consumes: `HAL_SPI_Transmit_DMA` / `HAL_SPI_Receive_DMA`；`HAL_SPI_GetTxState` / `GetRxState`；`SPI_TX_STATE_IDLE` / `SPI_RX_STATE_IDLE`
- Produces: 半双工 ≥32 字节 DMA 路径

- [ ] **Step 1: 等待与超时辅助**

```c
#ifdef BSP_USING_SPI1_DMA
/* HAL Timeout 参数是忙等计数，不是 ms；按 length 放大 spin 上限 */
static rt_bool_t acm32_spi_wait_tx_idle(SPI_HandleTypeDef *hspi, rt_uint32_t spins)
{
    while (spins--)
    {
        if (HAL_SPI_GetTxState(hspi) == SPI_TX_STATE_IDLE)
            return RT_TRUE;
    }
    return RT_FALSE;
}

static rt_bool_t acm32_spi_wait_rx_idle(SPI_HandleTypeDef *hspi, rt_uint32_t spins)
{
    while (spins--)
    {
        if (HAL_SPI_GetRxState(hspi) == SPI_RX_STATE_IDLE)
            return RT_TRUE;
    }
    return RT_FALSE;
}

static rt_uint32_t acm32_spi_dma_spins(rt_size_t length)
{
    /* 粗算：每字节大量循环 + 底数，避免极短超时 */
    rt_uint32_t s = (rt_uint32_t)length * 4096U + 100000U;
    return s;
}

/* 静态 0xFF dummy，禁止按 length malloc */
#define SPI_DMA_DUMMY_CHUNK  64
static rt_uint8_t spi_dma_dummy_ff[SPI_DMA_DUMMY_CHUNK];

static void acm32_spi_dummy_ff_init_once(void)
{
    static rt_uint8_t inited;
    if (!inited)
    {
        rt_memset(spi_dma_dummy_ff, 0xFF, sizeof(spi_dma_dummy_ff));
        inited = 1;
    }
}
#endif
```

- [ ] **Step 2: 半双工 TX DMA**

```c
static HAL_StatusTypeDef acm32_spi_dma_tx(struct acm32_spi *spi_drv,
                                          const rt_uint8_t *buf, rt_size_t len)
{
    SPI_HandleTypeDef *hspi = &spi_drv->handle;
    if (HAL_SPI_Transmit_DMA(hspi, (uint8_t *)buf, len) != HAL_OK)
        return HAL_ERROR;
    if (!acm32_spi_wait_tx_idle(hspi, acm32_spi_dma_spins(len)))
    {
        if (hspi->HDMA_Tx)
            HAL_DMA_Abort(hspi->HDMA_Tx);
        hspi->TxState = SPI_TX_STATE_IDLE;
        return HAL_TIMEOUT;
    }
    return HAL_OK;
}
```

- [ ] **Step 3: 半双工 RX DMA（分块 dummy TX）**

HAL 的 `Transmit_DMA` 会关 RX，`Receive_DMA` 只开 RX。Master 收数需要时钟。阶段 2 **优先** 策略：

**策略 A（首选实现）：** 对 length 分块（≤`SPI_DMA_DUMMY_CHUNK`），每块：
1. 无法真正并行 HAL 两路时：**回退该方向到轮询** `HAL_SPI_TransmitReceive(dummy, recv, chunk)`  
2. 若板测确认可先 `Receive_DMA` 再靠 BATCH 产钟：用 `HAL_SPI_Receive_DMA` + `wait_rx_idle`  

**策略 B（规格允许的明确回退）：** 仅 RX 无论长度 **全部轮询**（用静态 dummy 分块 `TransmitReceive`，去掉 malloc）；**仅 TX ≥32 走 DMA**。

实现顺序：
1. 先落地 **TX-only DMA**（策略清晰）
2. RX-only：用 **静态 dummy 分块轮询** 替换 `rt_malloc`（同时修阶段 1 Minor）
3. 可选尝试 RX DMA；失败则保持 RX 轮询并在注释写明

最小必达（spec 成功标准）：

```c
/* 在 spixfer 中，CS take 之后： */

#ifdef BSP_USING_SPI1_DMA
    if ((spi_drv->spi_dma_flag & SPI_USING_TX_DMA_FLAG) &&
        send_buf && !recv_buf && message->length >= SPI_DMA_MIN_SIZE)
    {
        state = acm32_spi_dma_tx(spi_drv, send_buf, message->length);
    }
    else
#endif
    if (send_buf && recv_buf)
    {
        /* full-duplex: always poll */
        state = HAL_SPI_TransmitReceive(...);
    }
    else if (send_buf)
    {
        state = HAL_SPI_Transmit(...); /* length < 32 or no DMA */
    }
    else if (recv_buf)
    {
        /* RX: chunked static dummy + TransmitReceive, no malloc */
        acm32_spi_dummy_ff_init_once();
        {
            rt_size_t left = message->length;
            rt_uint8_t *p = recv_buf;
            state = HAL_OK;
            while (left && state == HAL_OK)
            {
                rt_size_t n = left > SPI_DMA_DUMMY_CHUNK ? SPI_DMA_DUMMY_CHUNK : left;
                state = HAL_SPI_TransmitReceive(hspi, spi_dma_dummy_ff, p, n,
                                                SPI_XFER_TIMEOUT_MS);
                p += n;
                left -= n;
            }
        }
    }
```

若时间允许且双 DMA 可行，再增加 RX DMA 路径；**不得**阻塞 Task 4 验收（TX DMA + 无 malloc RX 即可宣称阶段 2 主路径完成，RX DMA 作为同 commit 可选增强）。

- [ ] **Step 4: Commit**

```bash
git add bsp/acm32/acm32p4xx-nucleo/drivers/drv_spi.c
git commit -m "feat(spi): half-duplex TX DMA path and malloc-free RX poll"
```

---

### Task 4: 测试、打开 DMA 配置、Keil 0 Error

**Files:**
- Modify: `bsp/acm32/acm32p4xx-nucleo/applications/main.c`
- Modify: `bsp/acm32/acm32p4xx-nucleo/.config`
- Generate: `rtconfig.h`
- Possibly: `libraries/SConscript` 已含 `hal_spi.c`（阶段 1 已修）

- [ ] **Step 1: 扩展 spi1_loopback_test**

支持可选长度参数（默认 16 测轮询，≥32 测长包/DMA TX 需半双工 API）：

环回验证 full-duplex 仍用 `rt_spi_transfer`（走轮询）。  
另增：

```c
/* 半双工大包：仅发送 length 字节 0x5A 图案（DMA TX），再 transfer 环回校验 */
int spi1_dma_test(int argc, char **argv)
{
    /* 1) attach spi10 if needed
       2) configure 1MHz mode0
       3) len = (argc>=2) ? atoi(argv[1]) : 256; if len < 32 len = 256;
       4) rt_spi_transfer 环回 len 字节（full-duplex 轮询路径仍应 PASS）
       5) 使用 rt_spi_send 发送 len 字节（half-duplex TX，触发 DMA）
          — 环回时 send 数据会出现在 MISO，可再用 transfer 验证短包
       6) 打印 PASS/FAIL
    */
}
MSH_CMD_EXPORT(spi1_dma_test, "SPI1 long xfer test [len]");
```

更稳妥的板测组合：
1. `spi1_loopback_test` — 16 字节 full-duplex（回归）
2. `spi1_loopback_test` 改为接受 `len`：`rt_spi_transfer` 对 len 字节——验证长包 full-duplex 轮询
3. 日志打印是否编译了 `BSP_USING_SPI1_DMA`（`#ifdef` 提示 “DMA enabled, half-duplex TX>=32 uses DMA”）

推荐最小改 `spi1_loopback_test`：

```c
int len = 16;
if (argc >= 2)
    len = atoi(argv[1]);
if (len <= 0 || len > 512)
    len = 16;
/* 栈上或静态 tx[512]/rx[512] */
```

- [ ] **Step 2: .config**

```
CONFIG_BSP_USING_SPI1=y
CONFIG_BSP_USING_SPI1_DMA=y
```

- [ ] **Step 3: 生成与编译**

```powershell
chcp 65001 | Out-Null
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
cd bsp/acm32/acm32p4xx-nucleo
scons --pyconfig-silent
scons --target=mdk5
# Keil UV4 -b project.uvprojx
```

Expected: `rtconfig.h` 含 `BSP_USING_SPI1_DMA`；Keil **0 Error**。

- [ ] **Step 4: 板测清单**

| 命令 | 期望 |
|------|------|
| `spi1_loopback_test` | PASS（16B 轮询） |
| `spi1_loopback_test 64` | PASS（64B full-duplex 轮询） |
| 半双工 DMA | 无卡死（可用逻辑分析仪看 CS/SCK） |

- [ ] **Step 5: Commit**

```bash
git add bsp/acm32/acm32p4xx-nucleo/applications/main.c \
        bsp/acm32/acm32p4xx-nucleo/.config \
        bsp/acm32/acm32p4xx-nucleo/rtconfig.h
git commit -m "test(spi): enable SPI1 DMA and extend loopback length"
```

---

## Plan Self-Review

| Spec 要求 | Task |
|-----------|------|
| `BSP_USING_SPI1_DMA` | 1 |
| DMA2_CH1 / CH3 + Req 1/2 | 1–2 |
| 阈值 32 | 1, 3 |
| Full-duplex 轮询 | 3 |
| Half-duplex TX DMA | 3 |
| 无界 malloc 禁止 | 3 |
| SPI IRQ 完成路径 | 2（关键，spec 完成等待依赖） |
| 软 CS 不变 | 3（沿用） |
| 测试 + Keil | 4 |
| 不改 HAL | 全局 |

**HAL 风险备忘：** DMA 完成依赖 `HAL_SPI_IRQHandler`；未开 SPI1 NVIC 会导致 `GetTxState` 永不 IDLE。

**Placeholder scan：** 无 TBD；RX DMA 可选增强已标明优先级。

---

## 成功标准

- [ ] DMA 可选、默认可关
- [ ] TX half-duplex ≥32 走 DMA 且完成
- [ ] 短包 / full-duplex 与阶段 1 一致
- [ ] Keil 0 Error；与 UART2 DMA 通道无冲突
- [ ] RX dummy 无 `rt_malloc(length)`
