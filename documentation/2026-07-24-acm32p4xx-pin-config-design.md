# BSP ACM32P4xx 单引脚 Kconfig 配置方案

**日期:** 2026-07-24  
**状态:** 设计已确认，待实施

## 动机

现有引脚配置采用"预定义组"模式（如 UART1 ∈ {PA9,PA10} 或 {PB6,PB7}），缺点：
1. 不能跨组混搭引脚（如 SCL 用 PB6，SDA 用 PF12）
2. 每个新组需要手写一套 Kconfig + 头文件 + rtconfig.py 映射
3. 用户无法看到所有可用引脚选项

改为**每个信号线独立选择**：每根引脚一个 Kconfig `choice`，列出 AF 表允许的所有备选。

## 总体架构

```
Kconfig (choice per signal)
  ↓ 生成 rtconfig.h 宏 (BSP_UART1_TX_PA9 等)
config 头文件 (#ifdef → PORT/PIN/AF 三元组)
  ↓ 驱动引用 UART1_TX_PORT/PIN/AF
drv_*.c (代码不碰 #ifdef，直接用宏)
  ↑ 引脚参与冲突检测
rtconfig.py (动态匹配选中引脚)
```

## Layer 1: Kconfig

每个外设实例的每根信号线，一个 `choice`：

```kconfig
choice
    prompt "UART1 TX pin"
    depends on BSP_USING_UART1
    default BSP_UART1_TX_PA9

    config BSP_UART1_TX_PA9
        bool "PA9 AF1 (default)"
    config BSP_UART1_TX_PB6
        bool "PB6 AF1"
    config BSP_UART1_TX_PA14
        bool "PA14 AF1 (SWCLK, conflicts with debug)"
endchoice
```

**规则：**
- 每个 choice 有一个 `default` 指向当前在用引脚
- 选项名格式：`BSP_<外设>_<信号>_<端口><引脚号>`
- 选项描述包含 AF 号和对冲突的提示

## Layer 2: Config 头文件

`uart_config.h` / `spi_config.h` / `i2c_config.h` 等：

```c
/* UART1 TX */
#if defined(BSP_UART1_TX_PA9)
  #define UART1_TX_PORT     GPIOA
  #define UART1_TX_PIN      GPIO_PIN_9
  #define UART1_TX_AF       GPIO_FUNCTION_1
#elif defined(BSP_UART1_TX_PB6)
  #define UART1_TX_PORT     GPIOB
  #define UART1_TX_PIN      GPIO_PIN_6
  #define UART1_TX_AF       GPIO_FUNCTION_1
#elif defined(BSP_UART1_TX_PA14)
  ...
#else
  /* fallback default */
  #define UART1_TX_PORT     GPIOA
  #define UART1_TX_PIN      GPIO_PIN_9
  #define UART1_TX_AF       GPIO_FUNCTION_1
#endif
```

**规则：**
- 每根信号线独立一个 `#if/#elif/#else/#endif` 块
- 最后 `#else` 提供 fallback default（与 Kconfig default 一致）
- 驱动里直接 `gpio.Pin = UART1_TX_PIN;` 不需要碰 `#ifdef`

## Layer 3: 冲突检测

`rtconfig.py` 中 `_bsp_check_pins()` 不需要改逻辑，只改数据：

```python
# 所有可能出现的引脚（含选择键 → 引脚映射）
_BSP_UART1_TX_PINS = {
    'BSP_UART1_TX_PA9':  'PA9',
    'BSP_UART1_TX_PB6':  'PB6',
    'BSP_UART1_TX_PA14': 'PA14',
}
```

遍历所有外设的所有信号线，检查 cfg 中哪个宏被选中，收集对应引脚做冲突检测。

## 覆盖范围

| 外设 | 实例数 | 改法 | 说明 |
|------|:---:|------|------|
| UART | 4 | 独立引脚 | TX/RX/RTS/CTS 各有 Kconfig choice |
| LPUART | 2 | 独立引脚 | TX/RX |
| SPI | 4 | 独立引脚 | SCK/MOSI/MISO/CS |
| I2C | 2 | 独立引脚 | SCL/SDA |
| FDCAN | 2 | 独立引脚 | TX/RX |
| I2S | 1 | 独立引脚 | WS/CK/SDI/SDO/MCK |
| PWM | 4 | 独立引脚 | TIM1/2/3/10 每 CH 有备选引脚 |
| ETH | 1 | 保持现状 | RMII 引脚固定 |
| SDMMC | 1 | 保持现状 | 引脚固定 |
| USB | 1 | 保持现状 | PA11/PA12 固定 |
| ADC | 1 | 保持现状 | 模拟引脚 |
| DAC | 1 | 保持现状 | PA4/PA5 固定 |
| Pulse Encoder | 2 | 保持现状 | 唯一引脚 |
| Input Capture | 2 | 保持现状 | 唯一引脚 |

## 引脚数据来源

所有备选引脚严格来自 `drv_gpio.c` 中的 AF 功能映射表（芯片手册已验证）。

## 文件改动清单

| 文件 | 改动 |
|------|------|
| `drivers/Kconfig` | 所有外设引脚从组 choice 改为单信号 choice |
| `drivers/uart_config.h` | 从组宏 → 单引脚 `#ifdef` |
| `drivers/spi_config.h` | 同上 |
| `drivers/i2c_config.h` | 同上 |
| `drivers/tim_config.h` | 同上（PWM 部分） |
| `drivers/drv_can.h` | 同上 |
| `drivers/drv_i2s.c` | 同上（内联引脚宏） |
| `rtconfig.py` | 引脚映射表改为单引脚索引 |
| `applications/main.c` | 启动提示可能需更新 |

## 不变的部分

- 驱动 `.c` 文件：只引用 `PORT/PIN/AF` 宏，不碰 Kconfig 宏
- HAL 模块使能：`acm32p4xx_hal_conf.h` 不涉及
- 测试代码：不涉及
- DMA 冲突检测：不涉及
