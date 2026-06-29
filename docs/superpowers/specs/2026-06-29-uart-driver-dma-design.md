# drv_uart.c 完善设计文档

## 目标
将 `drv_uart.c` 从基础轮询收发升级为全功能 UART 驱动：支持中断非阻塞发送、DMA 收发、运行时重配置。

## 约束
- 使用 `dev_serial_v2`（`RT_USING_SERIAL_V2`）
- 使用 ACM32P4 SDK 的 `uart_*` / `dma_*` API
- SDK 已内置所有 UART ISR（`USART1_IRQHandler`~`USART8_IRQHandler`）和 DMA ISR（`DMA1_CH0~DMA2_CH7`），`drv_uart.c` 不重复定义 ISR
- SDK 回调机制：`rx_callback`（逐字节）、`tx_callback`（FIFO 空）、`tc_callback`（发送完成）、`idle_callback`（总线空闲）、DMA `tc_callback`（传输完成）

## 架构设计

### 数据结构

#### uart_config.h 扩展
```c
struct acm32_uart_config {
    // 原有字段
    const char      *name;
    uart_num_t       uart_num;
    IRQn_Type        irq_type;
    gpio_pin_t       tx_pin;
    gpio_pin_t       rx_pin;
    gpio_af_t        af;

    // 新增 DMA 字段
    dma_instance_t   dma_instance;      // DMA_1 或 DMA_2（不使用 DMA 时填 DMA_INSTANCE_NONE）
    dma_channel_t    tx_dma_channel;    // TX DMA 通道
    dma_channel_t    rx_dma_channel;    // RX DMA 通道
    uint8_t          tx_periph_id;      // 外设请求ID（如 UART1_TX=5）
    uint8_t          rx_periph_id;      // 外设请求ID（如 UART1_RX=6）
};
```

#### drv_uart.c 运行时结构体
```c
struct acm32_uart {
    struct acm32_uart_config *config;
    struct rt_serial_device   serial;

    // 中断发送状态
    const rt_uint8_t         *tx_buf;
    rt_size_t                 tx_size;
    rt_size_t                 tx_pos;

    // 当前配置镜像（GET_UART_CONFIG 用）
    struct serial_configure   shadow_config;

    // 当前使能的中断掩码
    rt_uint32_t               int_mask;
};
```

### OPS 回调定义

| op | 函数 | 说明 |
|----|------|------|
| `.configure` | `_uart_configure` | 初始化 GPIO + UART，注册 rx/tx/tc/idle 回调 |
| `.control` | `_uart_control` | CONFIG/GET_CONFIG/SET_INT/CLR_INT/CLOSE |
| `.putc` | `_uart_putc` | 单字节轮询发送（控制台用） |
| `.getc` | `_uart_getc` | 单字节轮询接收（控制台用） |
| `.transmit` | `_uart_transmit` | 中断非阻塞发送 |
| `.dma_transmit` | `_uart_dma_transmit` | DMA 非阻塞发送 |

### SDK 回调 → RT-Thread 事件映射

| SDK 回调 | 注册时机 | 触发后调用 |
|----------|---------|-----------|
| `rx_callback` | `_uart_configure` | `rt_hw_serial_isr(serial, RT_SERIAL_EVENT_RX_IND)` |
| `tx_callback` | `_uart_transmit` 启动时 | 填充 FIFO；发完最后一字节后禁用 TX 中断、使能 TC 中断 |
| `tc_callback` | `_uart_transmit` 启动时 | `rt_hw_serial_isr(serial, RT_SERIAL_EVENT_TX_DONE)`；禁用 TC 中断 |
| `idle_callback` | `_uart_configure`（DMA RX 时） | `rt_hw_serial_isr(serial, RT_SERIAL_EVENT_RX_DMADONE)` |
| DMA `tc_callback` | `_uart_dma_transmit` | 清理 DMA → `rt_hw_serial_isr(serial, RT_SERIAL_EVENT_TX_DMADONE)` |

### `_uart_control` 命令处理

| cmd | 行为 |
|-----|------|
| `RT_DEVICE_CTRL_CONFIG` | 禁用 UART → 逐项配置 baudrate/word_length/stop_bits/parity/endian → 更新 shadow_config → 重新使能 UART |
| `RT_DEVICE_CTRL_GET_UART_CONFIG` | 返回 shadow_config 副本 |
| `RT_DEVICE_CTRL_SET_INT` | 重新使能 int_mask 中已记录的中断 |
| `RT_DEVICE_CTRL_CLR_INT` | 禁用 int_mask 中已记录的中断 |
| `RT_DEVICE_CTRL_CLOSE` | 禁用所有 UART 中断、注销 SDK 回调 |

### 中断发送流程

```
_uart_transmit(serial, buf, size, tx_flag)
    1. 保存 tx_buf, tx_size, tx_pos = 0
    2. 填充首字节到 UART_DR 触发发送
    3. 使能 UART_INT_TX + UART_INT_TC
    4. 注册 tx_callback、tc_callback
    5. 返回 size

tx_callback 被 uart_irq_handler() 调用：
    if (tx_pos < tx_size)
        uart_putc_try(UART, tx_buf[tx_pos++])
    if (tx_pos == tx_size)
        uart_interrupt_disable(UART, UART_INT_TX)  // 发完，禁 TX 中断
        // TC 中断会处理 DONE 通知

tc_callback 被 uart_irq_handler() 调用：
    // 移位寄存器完全空闲
    uart_interrupt_disable(UART, UART_INT_TC)
    rt_hw_serial_isr(serial, RT_SERIAL_EVENT_TX_DONE)
```

### DMA 发送流程

```
_uart_dma_transmit(serial, buf, size, tx_flag)
    1. 配置 DMA 通道：MEM→PERIPH，dest_periph_id=TX_ID
    2. 注册 DMA tc_callback
    3. uart_config_dma(UART, {tx_dma_enable=true})
    4. dma_start_channel()
    5. 返回 size

DMA tc_callback：
    dma_stop_channel()
    uart_config_dma(UART, {tx_dma_enable=false})
    rt_hw_serial_isr(serial, RT_SERIAL_EVENT_TX_DMADONE)
```

### DMA 接收流程

DMA 接收由框架 `dev_serial_v2.c` 驱动。驱动层只提供 `idle_callback`：

```
_uart_configure() 中（如果 config->rx_dma_channel 有效）：
    1. 配置 DMA 循环接收（PERIPH→MEM, src_periph_id=RX_ID, 循环模式）
    2. 使能 UART_INT_IDLE
    3. rx_callback 置空（DMA 模式下不用逐字节回调）

idle_callback 被 uart_irq_handler() 调用：
    // 一帧结束，计算 DMA 已接收字节数
    received = buffer_size - dma_get_transfer_progress()
    rt_hw_serial_isr(serial, RT_SERIAL_EVENT_RX_DMADONE)
    // 框架自动处理数据拷贝和 FIFO 管理
```

### DMA 接收缓冲区管理

- `rx_dma_buf` 和 `rx_dma_bufsz` 移到 `struct acm32_uart` 运行时结构体中，由 `_uart_configure` 动态分配
- DMA 配置为循环模式（`DMA_ADDR_INCREMENT`），持续接收
- 每次 IDLE 中断时计算已接收字节数，通知框架

### 保持不变的部分

- `_uart_putc` / `_uart_getc`：轮询模式，保持不变（控制台依赖）
- `rt_hw_uart_init()`：注册设备流程不变
- `uart_config.h` 中的 `UART1_CONFIG` 宏：扩展 DMA 字段

## 修改文件

| 文件 | 改动说明 |
|------|---------|
| `drivers/uart_config.h` | 扩展配置结构体 + 更新 UART1_CONFIG 宏 |
| `drivers/drv_uart.c` | 重写：新增中断发送/DMA 收发/CTRL_CONFIG/sdk 回调注册 |
| `Kconfig` | 新增 BSP_USING_UART1_DMA 等选项 |
| `rtconfig.h` | 如需 DMA 则定义 BSP_USING_UART1_DMA |

## 验证

- Keil MDK 编译通过（0 错误 0 警告）
