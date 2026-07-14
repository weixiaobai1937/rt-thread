# ACM32P4xx-Nucleo SPI Master DMA 设计（阶段 2）

## 1. 目标

在阶段 1（SPI1 Master 轮询 + 软 CS）之上，为 SPI1 增加 **可选 DMA 传输**：

- 长度 **≥ 32 字节** 的 **半双工** 大包走 DMA
- 短包（&lt; 32）与 **全双工**（同时有 send 与 recv 缓冲）保持 **轮询**
- 软 CS、`rt_spi` 同步 `xfer` 语义不变
- 不修改 HAL / RT-Thread SPI 框架源码

## 2. 背景与约束

### 2.1 阶段 1 基线

- 总线名 `"spi1"`，引脚 PE12/SCK、PE11/MOSI、PE10/MISO（AF3），软 CS 默认 PE13
- `drv_spi.c` + `spi_config.h` + `BSP_USING_SPI1`
- 环回测试 `spi1_loopback_test` 已通过

### 2.2 HAL 能力与限制

| 能力 | 状态 |
|------|------|
| `HAL_SPI_Transmit_DMA` | 有 |
| `HAL_SPI_Receive_DMA` | 有 |
| `HAL_SPI_TransmitReceive_DMA` | **无** |
| DMA 启动方式 | `HAL_DMA_Start`（非 `_IT`） |
| 完成判定 | SPI batch / `TxState`/`RxState` + 驱动侧等待 |

### 2.3 DMA 请求号（手册 / `hal_dma.h`）

- `DMA1/2_REQ_SPI1_TX = 1`
- `DMA1/2_REQ_SPI1_RX = 2`

### 2.4 硬约束

- 不修改 `hal_spi.c` / `hal_dma.c` / `components/drivers`
- 最小改动；`rtconfig.h` 仅由 `.config` + `scons --pyconfig-silent` 生成
- DMA 通道避开当前默认 UART 占用：
  - UART2：TX `DMA1_CH3`，RX `DMA2_CH0`
  - UART1 DMA（可选）：`DMA1_CH1/CH2`
  - UART4 RX（若启用）：`DMA1_CH0`

## 3. 架构

```
rt_spi xfer (同步)
    │
    ├─ length < 32 或 未开 DMA 或 full-duplex
    │       → 阶段 1 轮询路径
    │
    └─ half-duplex 且 length ≥ 32 且 BSP_USING_SPI1_DMA
            ├─ 仅 TX → HAL_SPI_Transmit_DMA + wait
            └─ 仅 RX → Receive_DMA + dummy 时钟源 + wait
```

### 3.1 文件

| 文件 | 变更 |
|------|------|
| `drivers/drv_spi.c` | DMA 初始化、阈值分支、完成等待 |
| `drivers/spi_config.h` | TX/RX DMA 实例、通道、ReqID、IRQ 宏 |
| `drivers/Kconfig` | `BSP_USING_SPI1_DMA` |
| `applications/main.c` | 扩展环回测试（长度参数或 `spi1_dma_test`） |

### 3.2 运行时扩展

```c
/* 在 struct acm32_spi 中（概念） */
#ifdef BSP_USING_SPI1_DMA
    DMA_HandleTypeDef dma_tx;
    DMA_HandleTypeDef dma_rx;
    rt_uint8_t        spi_dma_flag; /* bit0=RX, bit1=TX */
#endif
```

`SPI_HandleTypeDef` 的 `HDMA_Tx` / `HDMA_Rx` 指向上述 handle。

## 4. 配置

### 4.1 Kconfig

```
config BSP_USING_SPI1_DMA
    bool "Enable SPI1 DMA (TX: DMA2_CH1, RX: DMA2_CH3)"
    depends on BSP_USING_SPI1
    default n
    help
      Half-duplex transfers >= 32 bytes use DMA.
      Full-duplex always uses polling (no TransmitReceive_DMA in HAL).
      Do not enable together with BSP_USING_UART3_DMA RX path
      (UART3 RX may use DMA2_CH3).
```

### 4.2 通道分配

| 方向 | Instance | Channel | IRQ | ReqID |
|------|----------|---------|-----|-------|
| TX | `DMA2_Channel1` | 1 | `DMA2_CH1_IRQn` | `DMA2_REQ_SPI1_TX` (1) |
| RX | `DMA2_Channel3` | 3 | `DMA2_CH3_IRQn` | `DMA2_REQ_SPI1_RX` (2) |

与默认 `BSP_USING_UART2_DMA`（DMA1_CH3 + DMA2_CH0）无冲突。  
若同时启用 `BSP_USING_UART3_DMA`，其 RX 为 DMA2_CH3，与 SPI1 RX 冲突——**Kconfig help 禁止**；不强制 `depends on !UART3_DMA`（避免过度耦合），实现与文档双重提醒即可。

### 4.3 阈值

```c
#define SPI_DMA_MIN_SIZE  32
```

阶段 2 **固定** 32，不增加 Kconfig 项。

### 4.4 DMA 参数

| 项 | TX | RX |
|----|----|----|
| Mode | NORMAL | NORMAL |
| DataFlow | M2P | P2M |
| Width | BYTE | BYTE |
| Src/Dest Inc | 内存增 / 外设固定 | 外设固定 / 内存增 |

时钟：`__HAL_RCC_DMA2_CLK_ENABLE()`。

## 5. 传输策略（定稿）

### 5.1 决策表

| 条件 | 路径 |
|------|------|
| `!BSP_USING_SPI1_DMA` | 全部轮询 |
| `send_buf && recv_buf`（全双工） | **一律轮询**（任意长度） |
| `length < SPI_DMA_MIN_SIZE` | 轮询 |
| 仅 `send_buf`，`length ≥ 32` | `HAL_SPI_Transmit_DMA` + 等待完成 |
| 仅 `recv_buf`，`length ≥ 32` | DMA RX + 提供时钟（见 5.2） |

### 5.2 仅 RX 大包

Master 收数需要 SCK。阶段 2 采用：

1. 驱动内 **静态可复用** `0xFF` dummy 缓冲（固定大小，例如 64 或 256 字节）
2. 按块：对每块启动 **TX DMA（dummy）+ RX DMA（用户缓冲）**，或  
   - 若 HAL 状态机不允许并行两路：则 **仅 RX 大包回退轮询**，并在实现注释与测试中标明  
3. **禁止** 按 `message->length` 无界 `rt_malloc`

推荐优先实现 **可工作的并行双 DMA**；若板测不稳定，回退到「仅 RX 轮询」并记入 commit 说明（仍满足「TX 大包 DMA」主目标）。

### 5.3 完成等待

在 `xfer` 内同步等待（保持 `rt_spi` 阻塞语义）：

- 轮询 `HAL_SPI_GetTxState` / `HAL_SPI_GetRxState` 至 IDLE，和/或 SPI STATUS batch done 与 DMA 通道 EN 清除
- 超时：`HAL_DMA_Abort`，清 SPI DMA 请求位（若可从 HAL 侧安全清理），返回 `0`
- 超时上界：按 length 与波特率估算 spin 上限，避免误用「毫秒」命名（阶段 1 已发现 HAL Timeout 为忙等计数）

### 5.4 软 CS

与阶段 1 相同：`cs_take` / `cs_release` 基于 `device->cs_pin`，在 DMA 启动前 take、等待结束后 release。

### 5.5 初始化时机

在 `configure()` → `HAL_SPI_Init` 成功后：

1. 若 `BSP_USING_SPI1_DMA`：初始化 `dma_tx`/`dma_rx`，`hspi->HDMA_Tx`/`HDMA_Rx` 赋值  
2. 使能 NVIC 仅当后续改用 `HAL_DMA_Start_IT` 时需要；阶段 2 以 HAL 的 `HAL_DMA_Start` 为准时，**可不注册 DMA IRQ**，除非等待逻辑依赖 TC 中断  

若发现必须用 `_IT` 才能可靠完成，允许驱动侧对 DMA 使用 `HAL_DMA_Start_IT` + 简单 flag，**仍不修改** `hal_spi.c` 内部实现（在调用 HAL_SPI_*_DMA 前自建 DMA 完成路径时需评估与 HAL 的 `HAL_DMA_Start` 是否冲突——**优先完全沿用 HAL_SPI_*_DMA + 状态轮询**）。

## 6. 错误处理

| 场景 | 行为 |
|------|------|
| DMA init 失败 | configure 返回 `-RT_EIO`，回退不可用 DMA 标志 |
| DMA 传输超时/失败 | xfer 返回 `0` |
| 通道被占用（运行时） | Abort + 返回 `0` |
| Full-duplex | 永不进 DMA 分支 |

## 7. 测试与验收

1. `.config` 打开 `BSP_USING_SPI1_DMA=y`，`scons --pyconfig-silent`，`scons --target=mdk5`，Keil **0 Error**
2. 短包环回（&lt;32）仍 `PASS`（轮询）
3. 长包半双工：MSH 测试发送 ≥32（建议 256）字节环回或 TX-only 不卡死
4. 全双工 `rt_spi_transfer` 长包正确（轮询路径）
5. 默认与 `BSP_USING_UART2_DMA` 同时开启可链接、无通道静态冲突
6. 测试命令：扩展 `spi1_loopback_test [len]` 或新增 `spi1_dma_test`

## 8. 明确不做（阶段 2）

- Full-duplex DMA
- 异步 `rt_completion` / 非阻塞 SPI
- Slave、QSPI/OSPI、SPI2+ 多实例
- 修改 HAL 源码增加 `TransmitReceive_DMA`
- 与 UART3_DMA 的硬 Kconfig 互斥（仅 help 说明）

## 9. 后续

| 阶段 | 内容 |
|------|------|
| 3 | SPI2+ 多实例 |
| 4 | Slave |
| 5 | QSPI/OSPI |
| 可选改进 | Full-duplex 双 DMA、真实 AHB 时钟查表、DMA 完成中断减忙等 |

## 10. 成功标准（阶段 2 Done）

- [ ] `BSP_USING_SPI1_DMA` 可选且默认可关
- [ ] 半双工 length≥32 走 DMA 且可完成
- [ ] 短包与全双工行为与阶段 1 一致
- [ ] Keil 0 Error；通道与 UART2 默认 DMA 无冲突
- [ ] 未修改 HAL / 内核；无界 malloc 不用于 DMA dummy
