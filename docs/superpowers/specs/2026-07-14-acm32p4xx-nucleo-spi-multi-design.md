# ACM32P4xx-Nucleo SPI 多实例设计（阶段 3）

## 1. 目标

将 SPI 驱动从 **SPI1 单例** 升级为 **表驱动多实例**，在阶段 1/2 能力之上支持：

- **SPI1 + SPI2** 总线（Kconfig 分别开关）
- 共用 `configure` / `xfer`（轮询 + 半双工 TX DMA 决策）
- **SPI2 可选 DMA**（与 SPI1 同策略，独立通道）
- 软 CS + `rt_hw_spi_device_attach` 不变

**不实现：** SPI3/4/7、Slave、QSPI/OSPI、修改 HAL / SPI 框架。

## 2. 背景

### 2.1 阶段 1/2 基线

- SPI1：PE12/SCK、PE11/MOSI、PE10/MISO、软 CS PE13；Master 轮询；半双工 TX≥32 DMA（DMA2_CH1/CH3）
- 环回 / DMA 测试已通过

### 2.2 本阶段动机

- 板级需要第二路 SPI（传感器/Flash 等）
- 避免为每个 SPI 复制 `drv_spi.c` 逻辑
- 为后续 SPI3/4 只加 config 宏预留模型

## 3. 架构

```
spi_config[]          // 编译期按 BSP_USING_SPIx 展开
    │
spi_bus_obj[i]        // struct acm32_spi
    ├─ handle (SPI_HandleTypeDef)
    ├─ config *
    ├─ dma_tx / dma_rx (optional)
    └─ rt_spi_bus "spi1" | "spi2"
            │
            └─ 共用 rt_spi_ops (configure, xfer)
```

### 3.1 文件

| 文件 | 变更 |
|------|------|
| `drivers/spi_config.h` | 增加 `SPI2_BUS_CONFIG`、`SPI2_DMA_*`、`SPI2_DEFAULT_CS_PIN_INDEX` |
| `drivers/drv_spi.c` | 多实例表、MspInit 分发、IRQ 分发、DMA 配置 per-instance |
| `drivers/Kconfig` | `BSP_USING_SPI2`、`BSP_USING_SPI2_DMA` |
| `applications/main.c` | 可选 `spi2_loopback_test`（`#ifdef BSP_USING_SPI2`） |

### 3.2 运行时结构（概念）

```c
struct acm32_spi_dma_config {
    DMA_Channel_TypeDef *Instance;
    rt_uint32_t          Channel;
    IRQn_Type            irq;
    rt_uint32_t          reqid;
};

struct acm32_spi_config {
    const char      *bus_name;
    SPI_TypeDef     *Instance;
    IRQn_Type        irq_type;
    /* SCK/MOSI/MISO port+pin+af */
#ifdef BSP_USING_SPI1_DMA || BSP_USING_SPI2_DMA  /* 或始终保留指针可为 NULL */
    const struct acm32_spi_dma_config *dma_tx;
    const struct acm32_spi_dma_config *dma_rx;
#endif
};
```

实现时可用更简洁方式：每总线在 `spi_config.h` 定义 DMA 宏，在 `drv_spi.c` 用 `switch(Instance)` 或并行数组选 DMA 配置；**禁止** 只认 SPI1 的硬编码 `spi1_dma_tx`。

## 4. 引脚与片选

| 总线 | SCK | MOSI | MISO | 默认软 CS | AF | 说明 |
|------|-----|------|------|-----------|-----|------|
| SPI1 | PE12 | PE11 | PE10 | PE13（pin **77**） | AF3 | 阶段 1 既有 |
| SPI2 | PB13 | PB15 | PB14 | PB12（pin **28**） | AF3 | 本阶段新增 |

软 CS：GPIO 输出，空闲高；由 `rt_hw_spi_device_attach(bus, name, cs_pin)` 绑定。

时钟：

- SPI1：`__HAL_RCC_SPI1_CLK_ENABLE()` + `__HAL_RCC_GPIOE_CLK_ENABLE()`
- SPI2：`__HAL_RCC_SPI2_CLK_ENABLE()` + `__HAL_RCC_GPIOB_CLK_ENABLE()`

## 5. Kconfig

```
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
      Avoid conflict with other users of DMA1_CH0/CH1 (e.g. UART4 RX if remapped).
```

默认产品配置可继续 **只开 SPI1**；SPI2 需显式 menuconfig。

## 6. DMA 通道规划

| 外设 | TX | RX | 备注 |
|------|----|----|------|
| UART2（默认） | DMA1_CH3 | DMA2_CH0 | 已用 |
| SPI1 DMA | DMA2_CH1 | DMA2_CH3 | 阶段 2 |
| SPI2 DMA | **DMA1_CH0** | **DMA1_CH1** | 本阶段 |
| UART4 DMA（若启用） | DMA2_CH1 | DMA1_CH0 | 与 SPI1 TX / SPI2 RX 潜在冲突 → help 说明 |

ReqID：`DMA1_REQ_SPI2_TX = 3`，`DMA1_REQ_SPI2_RX = 4`。

策略与阶段 2 相同：

- Full-duplex：轮询  
- 半双工 TX，`length ≥ SPI_DMA_MIN_SIZE(32)`：`HAL_SPI_Transmit_DMA` + `HAL_SPI_WaitTxTimeout`  
- 仅 RX：静态 dummy 分块轮询  
- DMA 前 `System_CleanDAccelerate_by_Addr`  
- 完成依赖 **该实例 SPI IRQn** + `HAL_SPI_IRQHandler`

## 7. 驱动重构要点

1. **枚举索引**：`SPI1_INDEX` / `SPI2_INDEX` 条件编译进 `SPI_MAX_INDEX`  
2. **spi_config[] / spi_bus_obj[]**：按启用实例填充  
3. **MspInit**：按 `hspi->Instance` 匹配 config，配置对应 GPIO/时钟  
4. **DMA init**：读取该实例的 DMA 配置；无 DMA 的总线 `spi_dma_flag = 0`  
5. **IRQ**：

```c
#ifdef BSP_USING_SPI1
void SPI1_IRQHandler(void) { ... HAL_SPI_IRQHandler(&spi_bus_obj[SPI1_INDEX].handle); }
#endif
#ifdef BSP_USING_SPI2
void SPI2_IRQHandler(void) { ... HAL_SPI_IRQHandler(&spi_bus_obj[SPI2_INDEX].handle); }
#endif
```

6. **xfer**：不按总线名分支，只看 `spi_dma_flag` 与 message 形态  
7. **attach**：仍为全局 `rt_hw_spi_device_attach`；测试用 `SPI2_DEFAULT_CS_PIN_INDEX 28`

## 8. 测试与验收

| 场景 | 期望 |
|------|------|
| 仅 SPI1 | `spi1_loopback_test` / `spi1_dma_test` 回归 PASS |
| SPI1+SPI2 | `list_device` 可见 `spi1`、`spi2` |
| SPI2 环回 | 短接 PB15–PB14，`spi2_loopback_test` PASS（可选硬件） |
| SPI2_DMA=y | Keil **0 Error**；与 UART2+SPI1 默认 DMA 无链接级通道冲突 |
| SPI2 default n | 默认固件体积/行为不强制打开 SPI2 |

## 9. 明确不做

- SPI3 / SPI4 / SPI7  
- Slave / QSPI / OSPI  
- Full-duplex DMA  
- 修改 `hal_spi.c` / 内核 SPI 组件  
- 强制 `.config` 打开 SPI2（除非验证任务临时打开）

## 10. 成功标准

- [ ] 表驱动可同时注册 `spi1` 与 `spi2`  
- [ ] SPI2 引脚/时钟/软 CS 正确  
- [ ] SPI2 可选 DMA 通道独立且策略与 SPI1 一致  
- [ ] SPI1 阶段 2 回归通过  
- [ ] Keil 0 Error；未改 HAL  
- [ ] 后续加 SPI3 仅需 config + Kconfig + IRQ 三行级扩展（模型验证）
