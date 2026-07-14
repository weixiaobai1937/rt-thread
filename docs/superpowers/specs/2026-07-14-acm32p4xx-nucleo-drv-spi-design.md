# ACM32P4xx-Nucleo SPI 驱动设计（阶段 1）

## 1. 目标

为 `bsp/acm32/acm32p4xx-nucleo` 提供 RT-Thread 标准 SPI 总线驱动：

- **Master 轮询传输**
- **软件 GPIO CS**
- **仅 SPI1**
- 可编译、可注册总线、可 attach 从设备、可通过环回或外设冒烟验证

本阶段 **不实现**：DMA、中断异步、Slave、QSPI/OSPI、多总线、硬件 CS 自动控制。

## 2. 背景与约束

### 2.1 现状

- Nucleo BSP 已有 `drv_uart` / `drv_gpio` / `drv_eth`，**无** `drv_spi`
- HAL 提供 `hal_spi.c` / `hal_spi.h`（含 `HAL_SPI_Init`、`Transmit`/`Receive`/`TransmitReceive` 及 DMA API）
- `acm32p4xx_hal_conf.h` 中 `HAL_SPI_MODULE_ENABLED` 当前默认关闭
- 同系列参考：`bsp/acm32/acm32f0x0-nucleo/drivers/drv_spi.c`（F0 HAL，不可直接照搬）
- 框架参考：STM32 `drv_spi.c` 的 `rt_spi_ops` + `rt_hw_spi_device_attach`

### 2.2 硬约束

- 不修改第三方代码（HAL、RT-Thread 核心、`components/drivers`）
- 最小改动；不无关重构
- 禁止手改 `rtconfig.h`：通过 `.config` + `scons --menuconfig` / `scons --pyconfig-silent` 生成
- SPI 模块开关与现有 UART/ETH 一致：Kconfig + HAL conf 使能

### 2.3 硬件（用户板级走线）

| 信号 | 连接器 | 引脚 | AF | 阶段 1 用途 |
|------|--------|------|-----|-------------|
| SPI1_CLK | CN1-06 | PE12 | AF3 | SCK |
| SPI1_IO0 (MOSI) | CN1-04 | PE11 | AF3 | MOSI |
| SPI1_IO1 (MISO) | CN1-30 | PE10 | AF3 | MISO |
| SPI1_CS | CN1-12 | PE13 | — | **软件 GPIO CS**（输出，空闲高） |
| SPI1_IO2 | CN1-36 | PE14 | AF3 | 不初始化（留给 QSPI） |
| SPI1_IO3 | CN1-10 | PE15 | AF3 | 不初始化（留给 QSPI） |

## 3. 架构

```
应用 / 传感器驱动
    │  rt_spi_transfer / rt_spi_send_then_recv
    ▼
rt_spi_device  (软 CS：attach 绑定 pin)
    │
    ▼
rt_spi_bus "spi1"  ← drivers/drv_spi.c (rt_spi_ops)
    │  configure() / xfer()
    ▼
HAL_SPI_*  (libraries/HAL_Driver，只调用不修改)
    │
SPI1 + GPIO AF (PE10/11/12) + 软 CS GPIO (默认示例 PE13)
```

### 3.1 文件放置

| 文件 | 职责 |
|------|------|
| `drivers/drv_spi.c` | 总线注册、`rt_spi_ops`、MspInit、xfer |
| `drivers/spi_config.h` | SPI1 实例、引脚、AF、总线名宏 |
| `drivers/Kconfig` | `BSP_USING_SPI1` |
| HAL conf / 板级配置路径 | `HAL_SPI_MODULE_ENABLED` |
| `applications/main.c`（或等价） | 可选 MSH 环回测试命令 |

`drivers/SConscript` 已 `Glob('*.c')`，新增 `drv_spi.c` 自动编入。

### 3.2 运行时结构（示意）

```c
struct acm32_spi_config {
    const char     *bus_name;     /* "spi1" */
    SPI_TypeDef    *Instance;
    IRQn_Type       irq_type;     /* 阶段1 可不用，预留 */
    /* SCK / MOSI / MISO: port, pin, af */
};

struct acm32_spi {
    SPI_HandleTypeDef           handle;
    struct acm32_spi_config    *config;
    struct rt_spi_configuration *cfg;   /* 最近一次 configure 缓存，可选 */
    struct rt_spi_bus           spi_bus;
};
```

阶段 2 再在结构中增加 DMA handle / flag，本阶段不预埋复杂 DMA 逻辑。

## 4. 配置与时钟

### 4.1 Kconfig

```
config BSP_USING_SPI1
    bool "Enable SPI1 (PE12/PE11/PE10, soft CS PE13)"
    select RT_USING_SPI
    default n
```

### 4.2 HAL 模块

使能 `HAL_SPI_MODULE_ENABLED`，使 `hal_spi.c` 参与编译（方式与现有外设模块一致，不手改无关 conf 大段）。

### 4.3 时钟与 GPIO（`HAL_SPI_MspInit`）

当 `hspi->Instance == SPI1`：

1. `__HAL_RCC_GPIOE_CLK_ENABLE()`
2. `__HAL_RCC_SPI1_CLK_ENABLE()`
3. PE12/PE11/PE10：`GPIO_MODE_AF_PP`，`GPIO_FUNCTION_3`（AF3），上下拉按板级习惯（建议上拉或无上下拉，实现时与现有 GPIO 风格一致）
4. **不**在 MspInit 中配置 CS

### 4.4 波特率

- 输入：`struct rt_spi_configuration::max_hz`
- 输出：`SPI_InitTypeDef::BaudRate_Prescaler`  
  可选：`2, 4, 6, 8, 16, 32, 64, 128, 254`（HAL 定义）
- 基准时钟：SPI1 挂在 AHB1，实现时使用板级可知的 SPI/AHB 时钟（`SystemCoreClock` 或 RCC 查询/板级宏），选择 **不大于** `max_hz` 的最大分频结果（即分频后频率 ≤ 请求频率；若均过高则用最大分频 254）

### 4.5 SPI 工作参数映射

| RT-Thread cfg | HAL |
|---------------|-----|
| Master（仅此） | `SPI_MODE_MASTER` |
| Mode 0/1/2/3 | `SPI_WORK_MODE_0..3`（由 CPOL/CPHA 组合） |
| MSB / LSB | `SPI_FIRSTBIT_MSB` / `LSB` |
| 数据线 | `SPI_1X_MODE`（标准单线双向数据对） |
| 数据宽度 | 阶段 1 以 **8-bit** 为主路径；若框架传入 16-bit，要么明确支持并按 16-bit 传长度语义，要么返回错误。**默认实现 8-bit；非 8-bit 返回 `-RT_EINVAL` 或 configure 失败**，避免静默错传 |

## 5. API 与数据流

### 5.1 `rt_spi_ops`

#### `configure(device, cfg)`

1. 从 `device->bus` 取 `struct acm32_spi`
2. 填充 `SPI_HandleTypeDef::Init`（Master、Work_Mode、First_Bit、Prescaler、`X_Mode=1X`）
3. `HAL_SPI_Init`；失败返回 `-RT_EIO`
4. 成功返回 `RT_EOK`

#### `xfer(device, message)`

1. 取 bus / handle；取 CS（见 5.2）
2. `message->cs_take` 且非 `RT_SPI_NO_CS`：CS 拉低
3. 传输（轮询，timeout 默认 1000 ms，或 `max(1000, f(length))`）：
   - `send_buf && recv_buf` → `HAL_SPI_TransmitReceive`
   - 仅 `send_buf` → `HAL_SPI_Transmit`
   - 仅 `recv_buf` → Master 收数需时钟：使用 **dummy 0xFF 的 TransmitReceive**，或 HAL `Receive`（若其内部已产生时钟）。**优先 TransmitReceive(dummy, recv)**，行为明确
4. `message->cs_release` 且非 `RT_SPI_NO_CS`：CS 拉高
5. HAL 失败：返回 `0`；成功：返回 `message->length`

### 5.2 软件 CS

- 导出：

```c
rt_err_t rt_hw_spi_device_attach(const char *bus_name,
                                 const char *device_name,
                                 rt_base_t cs_pin);
```

- 实现优先调用框架 `rt_spi_bus_attach_device_cspin`（与 STM32 一致），由框架管理 CS GPIO
- 若当前树 SPI 框架版本 API 名称不同，则等价：attach device + 将 CS 设为输出空闲高，并在 xfer 中 take/release
- **默认测试 CS**：`GET_PIN(E, 13)` / 板级 pin 号对应 PE13
- 不强制启动时自动 attach；测试命令或应用显式 attach

### 5.3 总线注册

- `rt_hw_spi_init` / `INIT_BOARD_EXPORT`：在 `BSP_USING_SPI1` 下注册总线名 `"spi1"`
- 并发：依赖 `rt_spi_bus` 的 take/release，驱动内不加第二把锁

### 5.4 数据流

```
app: rt_spi_transfer
  → bus take
  → ops->configure（配置变化时由框架触发）
  → ops->xfer: CS↓ → HAL 轮询 → CS↑
  → bus release
```

## 6. 错误处理

| 场景 | 行为 |
|------|------|
| `HAL_SPI_Init` 失败 | `configure` 返回 `-RT_EIO` |
| 传输 HAL 非 OK | `xfer` 返回 `0` |
| 非法 data_width（非 8-bit） | `configure` 失败（`-RT_EINVAL`） |
| 空 message / 空 bus | `RT_ASSERT` 或返回 0（与仓库 SPI 驱动风格一致） |
| 第三方 HAL bug | 不修改 HAL；在驱动侧规避或记录限制 |

## 7. 测试与验收

### 7.1 编译

1. `.config` 打开 `BSP_USING_SPI1=y`（及 `RT_USING_SPI`）
2. `scons --pyconfig-silent` 生成 `rtconfig.h`
3. `scons --target=mdk5` / Keil：**0 Error**（Warning 尽量 0）

### 7.2 功能

1. `list_device`（或启动日志）可见 `spi1`
2. `rt_hw_spi_device_attach("spi1", "spi10", pe13_pin)` 成功
3. **环回**：MOSI 短接 MISO，MSH `spi1_loopback_test` 发送固定图案并校验读回
4. CS：空闲高，传输期间低（逻辑分析仪或示波器可选）
5. Mode0–3 / 不同 `max_hz` 切换不崩溃

### 7.3 测试命令

- 名称：`spi1_loopback_test`
- 位置：`applications/main.c` 或独立测试文件（与 `uart2_echo_test` 风格一致）
- **不**在 `main()` 中强制死循环占用

## 8. 明确不做（阶段 1）

- DMA TX/RX、IT 异步完成
- SPI Slave
- QSPI/OSPI / `SPI_2X_MODE` / `SPI_4X_MODE` / XIP
- SPI2/SPI3/SPI4/SPI7 多实例（表结构可扩展，但不实现）
- 硬件 `SPI1_CS` 外设自动控制
- 修改 `hal_spi.c` 或 RT-Thread SPI 框架源码

## 9. 后续路线图（仅规划）

| 阶段 | 内容 |
|------|------|
| 2 | SPI Master DMA TX/RX（通道分配避开 UART DMA 冲突） |
| 3 | SPI2+ 多实例 + Kconfig |
| 4 | Slave 模式 |
| 5 | QSPI/OSPI（启用 PE14/PE15 四线，可能独立 `drv_qspi`） |

## 10. 实现顺序建议

1. Kconfig + HAL_SPI 使能 + `spi_config.h` 引脚宏  
2. `drv_spi.c`：MspInit、configure、总线注册  
3. xfer + 软 CS attach API  
4. 环回 MSH 测试  
5. `scons --pyconfig-silent` + `scons --target=mdk5` 验证  

## 11. 成功标准（阶段 1 Done）

- [ ] SPI1 总线可注册为 `"spi1"`
- [ ] 软 CS attach 可用（默认示例 PE13）
- [ ] 8-bit Master 轮询收发正确（环回或外设）
- [ ] 工程可干净编译
- [ ] 未修改 HAL / 内核；`rtconfig.h` 由配置生成
