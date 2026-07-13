# ACM32P4xx-Nucleo UART DMA RX 重设计

## 目标
将 Nucleo BSP 的 UART DMA RX 从「DMA 停转 IDLE + last_pos 追踪 + HAL_DMA_Start 手动重启」改为「DMA 循环永不停止 + HTC/TC + IDLE 尾处理 + 使用 V2 dma_ping_rb + RX_DMADONE 事件」。

## 约束
- 使用 `dev_serial_v2`（`RT_USING_SERIAL_V2`）
- 使用 ACM32P4 HAL Driver（`hal_uart.c` / `hal_dma.c`）
- DMA 控制器：DMA1 4 通道 + DMA2 4 通道，仅 CH0~3 可用
- 不修改第三方代码（lwIP、RT-Thread 核心组件、HAL 库）
- UART1 作为 console 时可以不开 DMA（中断模式更稳定），但技术上支持 DMA
- **不丢数据**：连续流不能因 DMA 缓冲区回绕而丢失

## 架构设计

### 数据流

```
UART RX FIFO
    │
    ▼  DMA (circular, P2M)
dma_ping_rb 缓冲区 ← 由 V2 框架分配和管理
    │
    ▼  RX_DMADONE 事件触发框架批量搬数据
V2 主 ringbuffer
    │
    ▼  rt_device_read()
应用层
```

### DMA 配置

| 参数 | 值 |
|------|-----|
| 模式 | `DMA_MODE_CIRCULAR` |
| 数据流 | `DMA_DATAFLOW_P2M` |
| 传输宽度 | 字节 |
| 源地址 | `USART_TypeDef::DR` 或 `LPUART_TypeDef::RXDR`，不递增 |
| 目的地址 | `dma_ping_rb.buffer_ptr`，递增 |
| 缓冲区大小 | `dma_ping_bufsz` = 512 |
| 中断 | `HAL_DMA_Start_IT`：TC + HTC + Error |
| LLI 回绕 | 硬件自动，无需手动重启 |

### 运行时结构体

```c
struct acm32_uart {
    struct acm32_uart_config    *config;
    struct rt_serial_device      serial;
    volatile const rt_uint8_t   *tx_buf;
    rt_size_t                    tx_size;
    rt_size_t                    tx_pos;
    volatile rt_bool_t           tx_done;
    rt_uint32_t                  int_mask;

    /* DMA RX 位置追踪（IDLE 尾处理用） */
    rt_uint16_t                  rx_dma_last_pos;

    /* DMA handle */
    DMA_HandleTypeDef            dma_tx;
    DMA_HandleTypeDef            dma_rx;
};
```

**不再需要 `rx_dma_buf` / `rx_dma_bufsz`**——改用框架分配的 `dma_ping_rb`。

### configure 流程（DMA RX 部分）

```
_uart_configure()
    ├── HAL_UART_Init()           // GPIO + 时钟 + UART 参数
    ├── FIFO 阈值配置
    ├── 使能 RXI（逐字节，DMA 时后续禁用）
    │
    ├── if (rx_dma_instance != UART_DMA_NONE)
    │   ├── __HAL_RCC_DMAx_CLK_ENABLE()
    │   ├── USART_CR1_RXDMAE = 1
    │   ├── HAL_DMA_Init(&uart->dma_rx)
    │   │
    │   ├── 获取 dma_ping_rb 缓冲区:
    │   │   rt_hw_serial_control_isr(serial,
    │   │       RT_HW_SERIAL_CTRL_GET_DMA_PING_BUF, &dma_buf_info)
    │   │   rx_dma_buf = dma_buf_info.buffer_ptr
    │   │   rx_dma_bufsz = dma_buf_info.buffer_size
    │   │
    │   ├── 注册回调 + 启动 DMA (IT):
    │   │   dma_rx.Parent = uart
    │   │   dma_rx.XferHalfCpltCallback = _dma_rx_half_cplt
    │   │   dma_rx.XferCpltCallback     = _dma_rx_cplt
    │   │   dma_rx.XferErrorCallback    = _dma_rx_err
    │   │   rx_dma_last_pos = 0
    │   │   HAL_DMA_Start_IT(&dma_rx, addr, buf, bufsz)
    │   │
    │   ├── NVIC_SetPriority + NVIC_EnableIRQ (DMA 中断)
    │   │
    │   ├── 禁用 RXI → 使能 IDLEI
    │   └── NVIC 使能 UART IRQ
```

### 中断处理

#### DMA RX HTC 回调（半传输完成）

```c
_dma_rx_half_cplt(hdma)
    uart = hdma->Parent
    // DMA 已完成前 256 字节写入 dma_ping_rb
    // 通知框架 256 字节可用
    rt_hw_serial_isr(&uart->serial,
        RT_SERIAL_EVENT_RX_DMADONE | ((rx_dma_bufsz/2) << 8))
    uart->rx_dma_last_pos = rx_dma_bufsz / 2
```

#### DMA RX TC 回调（传输完成）

```c
_dma_rx_cplt(hdma)
    uart = hdma->Parent
    // DMA 已完成后 256 字节写入 dma_ping_rb
    rt_hw_serial_isr(&uart->serial,
        RT_SERIAL_EVENT_RX_DMADONE | ((rx_dma_bufsz/2) << 8))
    uart->rx_dma_last_pos = 0
```

#### UART IDLE 回调（尾处理）

```c
if (IDLEI)
    clear IDLEI flag
    if (rx_dma_buf && dma_rx.Instance)
        cur_pos = rx_dma_bufsz - __HAL_DMA_GET_TRANSFER_SIZE(&dma_rx)
        __DSB()

        // FIFO 残留追加到 DMA 缓冲区
        while (FIFO not empty)
            if (cur_pos < rx_dma_bufsz)
                rx_dma_buf[cur_pos++] = read DR
            else
                (void)read DR

        if (cur_pos != rx_dma_last_pos)
            if (cur_pos > rx_dma_last_pos)
                tail = cur_pos - rx_dma_last_pos
            else
                tail = (rx_dma_bufsz - rx_dma_last_pos) + cur_pos
            rt_hw_serial_isr(&uart->serial,
                RT_SERIAL_EVENT_RX_DMADONE | (tail << 8))
            rx_dma_last_pos = cur_pos
```

#### DMA RX Error 回调

```c
_dma_rx_err(hdma)
    // 出错时重启 DMA
    HAL_DMA_Start_IT(...)
    清零 rx_dma_last_pos
```

### TX DMA

```c
_uart_transmit()
    if (DMA_TX mode)
        HAL_DMA_Init(&dma_tx)     // NORMAL, M2P
        dma_tx.Parent = uart
        dma_tx.XferCpltCallback = _dma_tx_cplt
        USART_CR1_TXDMAE = 1
        HAL_DMA_Start_IT(&dma_tx, buf, DR, size)
        return size

_dma_tx_cplt(hdma)
    uart = hdma->Parent
    USART_CR1_TXDMAE = 0
    rt_hw_serial_isr(&uart->serial, RT_SERIAL_EVENT_TX_DMADONE)
```

### V2 框架集成

#### 注册标志

```c
// 非 DMA UART：
RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_INT_TX

// DMA UART（条件加）：
RT_DEVICE_FLAG_DMA_RX | RT_DEVICE_FLAG_DMA_TX
```

#### dma_ping_bufsz

```c
uart_obj[i].serial.config.dma_ping_bufsz = 512;
```

框架在 open(DMA_RX) 时据此分配 `dma_ping_rb`，configure 中通过 `GET_DMA_PING_BUF` 获取地址。

### 关键保证

| 场景 | 保证 |
|------|------|
| 连续流（1500+ 字节） | HTC 每 256 字节触发一次消费，缓冲区不会回绕覆盖 |
| 间歇 AT 响应 | IDLE 触发尾处理 |
| DMA 错误 | Error 回调自动重启 DMA |
| TX 完成通知 | DMA TX TC 回调通知框架 |
| FIFO 残留 | IDLE 时 drained |

### 不涉及

- 运行中重配置（`RT_DEVICE_CTRL_CONFIG`）：保持现有支持
- LPUART：支持，DMA 配置与 USART 相同
- 低功耗：不涉及

## 修改文件

| 文件 | 改动 |
|------|------|
| `drivers/drv_uart.c` | struct 增减字段；configure 改为 HAL_DMA_Start_IT + GET_DMA_PING_BUF；新增 HTC/TC/Error 回调；IDLE 改为 RX_DMADONE；TX 改为 TX_DMADONE；注册标志加 DMA；增加 DMA IRQ Handler |
| `drivers/uart_config.h` | 删除 UART_DMA_RX_BUF_SIZE（改用框架缓冲区） |
| `drivers/.config` | 启用 BSP_USING_UART1_DMA / UART2_DMA |
| `drivers/Kconfig` | 补 UART2/3/4 DMA 选项（已做） |

## 验证

- Keil MDK 编译通过（0 错误 0 警告）
- UART2（4G 模组）连续流 1500+378 字节不丢数据
- UART1 console 键入长字符串不回显错乱（或关闭 UART1 DMA 保持中断模式）
