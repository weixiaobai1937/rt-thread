#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <drivers/dev_serial_v2.h>
#include "board.h"
#include "uart_config.h"

#ifdef RT_USING_SERIAL_V2

#define UART_FIFO_DEPTH     16      /* ACM32P4 UART FIFO 深度 */
#define UART_MAX_COUNT      8       /* UART1~UART8 = 8 个 UART */
#define UART_DMA_RX_BUF_SIZE  256   /* DMA 接收循环缓冲区大小 */

/* 自定义控制命令 */
#define RT_DEVICE_CTRL_GET_UART_CONFIG  0x10

/* DMA 接收缓冲区（按 UART 索引，每 UART 独立）。
 * 仅当至少一个 BSP_USING_UARTx_DMA 启用时才分配，节省 RAM。 */
#if defined(BSP_USING_UART1_DMA) || defined(BSP_USING_UART2_DMA) || \
    defined(BSP_USING_UART3_DMA) || defined(BSP_USING_UART4_DMA) || \
    defined(BSP_USING_UART5_DMA) || defined(BSP_USING_UART6_DMA) || \
    defined(BSP_USING_UART7_DMA) || defined(BSP_USING_UART8_DMA)
static rt_uint8_t uart_dma_rx_buf[UART_MAX_COUNT][UART_DMA_RX_BUF_SIZE] __attribute__((aligned(4)));
#endif

/* ==================== 运行时结构体 ==================== */

struct acm32_uart
{
    struct acm32_uart_config    *config;
    struct rt_serial_device     serial;

    /* 中断发送状态 */
    volatile const rt_uint8_t    *tx_buf;
    rt_size_t                   tx_size;
    rt_size_t                   tx_pos;

    /* 中断发送完成标记（TC 回调中设置） */
    volatile rt_bool_t          tx_done;

    /* 当前配置镜像 */
    struct serial_configure     shadow_config;

    /* 当前使能的中断掩码 */
    rt_uint32_t                 int_mask;

    /* DMA 接收缓冲区 */
    rt_uint8_t                 *rx_dma_buf;
    rt_uint16_t                 rx_dma_bufsz;
};

#define raw_to_uart(raw) rt_container_of(raw, struct acm32_uart, serial)

/* ==================== 回调函数指针（文件级全局，供 SDK 回调使用） ==================== */

/* SDK 回调签名固定为 (uart_num_t, ...)，需要通过 uart_num 找到对应的 acm32_uart */
static struct acm32_uart *g_uart_instances[UART_MAX_COUNT] = {NULL};

static struct acm32_uart *uart_num_to_obj(uart_num_t uart_num)
{
    if (uart_num >= UART_MAX_COUNT)
        return NULL;
    return g_uart_instances[uart_num];
}

/* ==================== SDK 回调实现 ==================== */

/* RX 回调：每收到一个字节调用一次 */
static void uart_rx_cb(uart_num_t uart_num, uint8_t data)
{
    struct acm32_uart *uart = uart_num_to_obj(uart_num);
    if (uart == NULL)
        return;

    /* 将数据放入 V2 框架的 ringbuffer */
    rt_hw_serial_control_isr(&uart->serial, RT_HW_SERIAL_CTRL_PUTC, &data);

    /* 通知框架有新数据 */
    rt_hw_serial_isr(&uart->serial, RT_SERIAL_EVENT_RX_IND);
}

/* TX 回调：TX FIFO 有空位时调用，填充下一个字节 */
static void uart_tx_cb(uart_num_t uart_num)
{
    struct acm32_uart *uart = uart_num_to_obj(uart_num);
    if (uart == NULL || uart->tx_buf == NULL)
        return;

    /* 尽量填充 FIFO */
    while (uart->tx_pos < uart->tx_size)
    {
        if (!uart_putc_try(uart->config->uart_num, uart->tx_buf[uart->tx_pos]))
            break;  /* FIFO 满，等下次中断 */
        uart->tx_pos++;
    }

    /* 全部填充完毕：禁用 TX 中断，等 TC 中断通知完成 */
    if (uart->tx_pos >= uart->tx_size)
    {
        uart_interrupt_disable(uart->config->uart_num, UART_INT_TX);
    }
}

/* TC 回调：移位寄存器完全空闲 */
static void uart_tc_cb(uart_num_t uart_num)
{
    struct acm32_uart *uart = uart_num_to_obj(uart_num);
    if (uart == NULL)
        return;

    uart_interrupt_disable(uart->config->uart_num, UART_INT_TC);
    uart->tx_buf = NULL;
    uart->tx_done = RT_TRUE;
    rt_hw_serial_isr(&uart->serial, RT_SERIAL_EVENT_TX_DONE);
}

/* IDLE 回调：总线空闲，用于 DMA 接收帧边界检测 */
static void uart_idle_cb(uart_num_t uart_num)
{
    struct acm32_uart *uart = uart_num_to_obj(uart_num);
    if (uart == NULL)
        return;

    struct acm32_uart_config *c = uart->config;
    uint32_t received = 0;

    /*
     * 计算 DMA 已接收的字节数。
     * 前提：dma_get_transfer_progress 返回 DMA 控制器中剩余的传输次数
     * （即 配置的 transfer_size - 已传输字节数）。
     * 因此 已接收 = rx_dma_bufsz - dma_get_transfer_progress(...)。
     * 注意：不同 DMA 控制器实现可能返回已传输字节数而非剩余字节数，
     * 移植到其他平台时需要确认此 API 的返回语义。
     */
    if (c->rx_dma_channel != DMA_CHANNEL_NONE)
        received = uart->rx_dma_bufsz - dma_get_transfer_progress(c->dma_instance, c->rx_dma_channel);

    /* 读取 IDLE 中断时残留在 FIFO 中的数据，补充到 DMA 缓冲区尾部 */
    while (!uart_is_rx_fifo_empty(c->uart_num))
    {
        if (received < uart->rx_dma_bufsz)
            uart->rx_dma_buf[received++] = uart_getc(c->uart_num);
        else
            (void)uart_getc(c->uart_num);  /* 缓冲区满，丢弃 */
    }

    /* 重新配置 DMA 接收（循环模式下需要重新启动） */
    if (c->rx_dma_channel != DMA_CHANNEL_NONE)
    {
        dma_stop_channel(c->dma_instance, c->rx_dma_channel);
        dma_start_channel(c->dma_instance, c->rx_dma_channel);
    }

    /*
     * 内存屏障：确保 ISR 中对 rx_dma_buf 的写入对后续 task context
     * 中的读取可见。Cortex-M 默认无 Cache 时 __DSB() 为 nop，不增加开销；
     * 开启 Cache 时保证写入已到达内存。
     */
    __DSB();

    /* V2 框架通过事件高 8 位获取 rx_length，必须编码实际接收字节数 */
    rt_hw_serial_isr(&uart->serial, RT_SERIAL_EVENT_RX_DMADONE | ((int)received << 8));
}

/* DMA 通道 → UART 反向映射表（避免 O(n) 扫描） */
static struct acm32_uart *dma_ch_to_uart[2][DMA_CHANNEL_COUNT];  /* DMA1 + DMA2 */

/* DMA TC 回调：DMA 传输完成 */
static void dma_tx_tc_cb(dma_instance_t instance, dma_channel_t channel)
{
    struct acm32_uart *uart = dma_ch_to_uart[instance][channel];
    if (uart == NULL)
        return;

    dma_stop_channel(instance, channel);
    uart_config_dma(uart->config->uart_num,
        &(uart_dma_config_t){.tx_dma_enable = false, .rx_dma_enable = false});
    dma_ch_to_uart[instance][channel] = NULL;
    rt_hw_serial_isr(&uart->serial, RT_SERIAL_EVENT_TX_DMADONE);
}

/* 将 RT-Thread data_bits 映射为 SDK word_length */
static uart_word_length_t cfg_to_word_length(int data_bits)
{
    switch (data_bits)
    {
    case DATA_BITS_5: return UART_WORD_LENGTH_5BIT;
    case DATA_BITS_6: return UART_WORD_LENGTH_6BIT;
    case DATA_BITS_7: return UART_WORD_LENGTH_7BIT;
    case DATA_BITS_9: return UART_WORD_LENGTH_9BIT;
    default:          return UART_WORD_LENGTH_8BIT;
    }
}

/* ==================== OPS 实现 ==================== */

static rt_err_t _uart_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
    struct acm32_uart *uart = raw_to_uart(serial);
    struct acm32_uart_config *c = uart->config;

    /* GPIO 初始化 */
    gpio_init(c->tx_pin);
    gpio_set_function(c->tx_pin, c->af);
    gpio_init(c->rx_pin);
    gpio_set_function(c->rx_pin, c->af);
    gpio_pull_up(c->rx_pin);

    /* UART 初始化（自定义配置，使能 FIFO） */
    uart_config_t uart_init_cfg = {
        .baudrate = cfg->baud_rate,
        .word_length = UART_WORD_LENGTH_8BIT,
        .stop_bits = UART_STOP_BITS_1,
        .parity = UART_PARITY_NONE,
        .endian = UART_ENDIAN_LSB_FIRST,
        .fifo_enable = false  /* 先禁用，下面手动配置 */
    };
    uart_init_custom(c->uart_num, &uart_init_cfg);

    /* 配置 FIFO：TX 变空时触发中断，RX 1 字节阈值（Finsh 交互需要即时响应） */
    uart_fifo_config_t fifo_cfg = {
        .enable = true,
        .rx_threshold = UART_RX_FIFO_THRESHOLD_1_BYTE,
        .tx_threshold = UART_TX_FIFO_THRESHOLD_EMPTY
    };
    uart_config_fifo(c->uart_num, &fifo_cfg);

    /* 保存配置镜像 */
    uart->shadow_config = *cfg;

    /* 注册 SDK 回调（一次性注册，transmit 时不再重复注册） */
    uart_register_rx_callback(c->uart_num, uart_rx_cb);
    uart_register_tx_callback(c->uart_num, uart_tx_cb);
    uart_register_tc_callback(c->uart_num, uart_tc_cb);
    uart_register_error_callback(c->uart_num, NULL);

    /* 使能接收中断（RX 总是使能，TX/TC 在 transmit 时按需使能） */
    uart->int_mask = UART_INT_RX;
    uart_interrupt_enable(c->uart_num, UART_INT_RX);

    /* DMA 接收初始化 */
    if (c->rx_dma_channel != DMA_CHANNEL_NONE)
    {
        /* 分配 DMA 接收缓冲区（全局静态数组，按 UART 索引） */
        if (uart->rx_dma_buf == NULL)
        {
#if defined(BSP_USING_UART1_DMA) || defined(BSP_USING_UART2_DMA) || \
    defined(BSP_USING_UART3_DMA) || defined(BSP_USING_UART4_DMA) || \
    defined(BSP_USING_UART5_DMA) || defined(BSP_USING_UART6_DMA) || \
    defined(BSP_USING_UART7_DMA) || defined(BSP_USING_UART8_DMA)
            uart->rx_dma_buf = uart_dma_rx_buf[c->uart_num];
#endif
            uart->rx_dma_bufsz = UART_DMA_RX_BUF_SIZE;
        }

        /* 使能 DMA 控制器 */
        dma_enable(c->dma_instance);

        /* 使能 UART DMA 请求 */
        uart_dma_config_t dma_cfg = {
            .tx_dma_enable = false,
            .rx_dma_enable = true,
            .dma_disable_on_error = false
        };
        uart_config_dma(c->uart_num, &dma_cfg);

        /* 配置 DMA 循环接收 */
        dma_channel_config_t rx_dma_cfg = {
            .direction          = DMA_PERIPH_TO_MEM,
            .src_address        = uart_get_base_address(c->uart_num),
            .dest_address       = (rt_uint32_t)uart->rx_dma_buf,
            .src_addr_mode      = DMA_ADDR_FIXED,
            .dest_addr_mode     = DMA_ADDR_INCREMENT,
            .src_width          = DMA_WIDTH_BYTE,
            .dest_width         = DMA_WIDTH_BYTE,
            .src_burst          = DMA_BURST_1,
            .dest_burst         = DMA_BURST_1,
            .transfer_size      = uart->rx_dma_bufsz,
            .src_periph_id      = c->rx_periph_id,
            .dest_periph_id     = 0,
            .src_master         = DMA_MASTER_1,
            .dest_master        = DMA_MASTER_1,
            .bus_lock           = false,
            .tc_interrupt_enable = false,   /* 不用 TC 中断，用 IDLE 中断检测帧边界 */
            .ht_interrupt_enable = false,
            .error_interrupt_enable = false
        };
        dma_config_channel(c->dma_instance, c->rx_dma_channel, &rx_dma_cfg);
        dma_start_channel(c->dma_instance, c->rx_dma_channel);

        /* DMA 模式下禁用 RX 逐字节中断，仅保留 IDLE 中断 */
        uart_interrupt_disable(c->uart_num, UART_INT_RX);
        uart->int_mask &= ~UART_INT_RX;

        /* 使能 IDLE 中断（帧边界检测） */
        uart_register_idle_callback(c->uart_num, uart_idle_cb);
        uart->int_mask |= UART_INT_IDLE;
        uart_interrupt_enable(c->uart_num, UART_INT_IDLE);
    }

    NVIC_SetPriority(c->irq_type, 2);
    NVIC_EnableIRQ(c->irq_type);

    return RT_EOK;
}

static rt_err_t _uart_control(struct rt_serial_device *serial, int cmd, void *arg)
{
    struct acm32_uart *uart = raw_to_uart(serial);
    struct acm32_uart_config *c = uart->config;

    switch (cmd)
    {
    case RT_DEVICE_CTRL_CLR_INT:
        /* 禁用当前已使能的中断 */
        if (uart->int_mask)
        {
            uart_interrupt_disable(c->uart_num, uart->int_mask);
            NVIC_DisableIRQ(c->irq_type);
        }
        break;

    case RT_DEVICE_CTRL_SET_INT:
        /* 重新使能中断 */
        if (uart->int_mask)
        {
            uart_interrupt_enable(c->uart_num, uart->int_mask);
            NVIC_EnableIRQ(c->irq_type);
        }
        break;

    case RT_DEVICE_CTRL_CONFIG:
    {
        if (arg == NULL)
            return -RT_EINVAL;
        struct serial_configure *cfg = (struct serial_configure *)arg;

        /* 禁用 UART */
        uart_disable(c->uart_num);

        /* 逐项重配置 */
        uart_config_baudrate(c->uart_num, cfg->baud_rate);
        uart_config_word_length(c->uart_num, cfg_to_word_length(cfg->data_bits));

        uart_config_stop_bits(c->uart_num,
            (cfg->stop_bits == STOP_BITS_2) ? UART_STOP_BITS_2 : UART_STOP_BITS_1);

        uart_config_parity(c->uart_num,
            (cfg->parity == PARITY_ODD)  ? UART_PARITY_ODD :
            (cfg->parity == PARITY_EVEN) ? UART_PARITY_EVEN : UART_PARITY_NONE);

        uart_config_endian(c->uart_num,
            (cfg->bit_order == BIT_ORDER_MSB) ? UART_ENDIAN_MSB_FIRST : UART_ENDIAN_LSB_FIRST);

        /* 重新使能 */
        uart_enable(c->uart_num);
        uart_tx_enable(c->uart_num);
        uart_rx_enable(c->uart_num);

        /* DMA 接收通道需要重新启动（重配置期间 UART 被禁用，DMA 通道可能停止） */
        if (c->rx_dma_channel != DMA_CHANNEL_NONE)
        {
            dma_stop_channel(c->dma_instance, c->rx_dma_channel);
            dma_start_channel(c->dma_instance, c->rx_dma_channel);
        }

        /* 更新镜像 */
        uart->shadow_config = *cfg;
        break;
    }

    case RT_DEVICE_CTRL_GET_UART_CONFIG:
    {
        if (arg == NULL)
            return -RT_EINVAL;
        rt_memcpy(arg, &uart->shadow_config, sizeof(struct serial_configure));
        break;
    }

    case RT_DEVICE_CTRL_CLOSE:
        /* 关闭设备：禁用所有中断，注销回调 */
        uart_interrupt_disable(c->uart_num, uart->int_mask);
        NVIC_DisableIRQ(c->irq_type);
        uart_register_rx_callback(c->uart_num, NULL);
        uart_register_tx_callback(c->uart_num, NULL);
        uart_register_tc_callback(c->uart_num, NULL);
        uart_register_idle_callback(c->uart_num, NULL);

        /* 停止 DMA */
        if (c->rx_dma_channel != DMA_CHANNEL_NONE)
        {
            dma_stop_channel(c->dma_instance, c->rx_dma_channel);
            uart_config_dma(c->uart_num,
                &(uart_dma_config_t){.tx_dma_enable = false, .rx_dma_enable = false});
        }
        break;
    }

    return RT_EOK;
}

static int _uart_putc(struct rt_serial_device *serial, char c)
{
    struct acm32_uart *uart = raw_to_uart(serial);
    uart_putc(uart->config->uart_num, (rt_uint8_t)c);
    return 1;
}

static int _uart_getc(struct rt_serial_device *serial)
{
    struct acm32_uart *uart = raw_to_uart(serial);
    if (!uart_is_rx_fifo_empty(uart->config->uart_num))
        return (int)uart_getc(uart->config->uart_num);
    return -RT_EEMPTY;
}

/* 发送（中断模式或 DMA 模式，根据 open_flag 判断） */
static rt_ssize_t _uart_transmit(struct rt_serial_device *serial, rt_uint8_t *buf, rt_size_t size, rt_uint32_t tx_flag)
{
    struct acm32_uart *uart = raw_to_uart(serial);
    struct acm32_uart_config *c = uart->config;

    if (size == 0)
        return 0;

#ifdef RT_SERIAL_USING_DMA
    /* DMA 发送模式 */
    if (serial->parent.open_flag & RT_DEVICE_FLAG_DMA_TX)
    {
        if (c->tx_dma_channel == DMA_CHANNEL_NONE)
            return 0;

        /* 使能 DMA 控制器 */
        dma_enable(c->dma_instance);

        /* 使能 UART TX DMA */
        uart_dma_config_t uart_dma_cfg = {
            .tx_dma_enable = true,
            .rx_dma_enable = false,
            .dma_disable_on_error = false
        };
        uart_config_dma(c->uart_num, &uart_dma_cfg);

        /* 配置 DMA 通道 */
        rt_uint16_t actual_size = (rt_uint16_t)(size > 65535 ? 65535 : size);
        dma_channel_config_t dma_cfg = {
            .direction          = DMA_MEM_TO_PERIPH,
            .src_address        = (rt_uint32_t)buf,
            .dest_address       = uart_get_base_address(c->uart_num),
            .src_addr_mode      = DMA_ADDR_INCREMENT,
            .dest_addr_mode     = DMA_ADDR_FIXED,
            .src_width          = DMA_WIDTH_BYTE,
            .dest_width         = DMA_WIDTH_BYTE,
            .src_burst          = DMA_BURST_1,
            .dest_burst         = DMA_BURST_1,
            .transfer_size      = actual_size,
            .src_periph_id      = 0,
            .dest_periph_id     = c->tx_periph_id,
            .src_master         = DMA_MASTER_1,
            .dest_master        = DMA_MASTER_1,
            .bus_lock           = false,
            .tc_interrupt_enable = true,
            .ht_interrupt_enable = false,
            .error_interrupt_enable = false
        };
        dma_config_channel(c->dma_instance, c->tx_dma_channel, &dma_cfg);

        /* 注册 DMA TC 回调 + 反向映射表 */
        dma_register_tc_callback(c->dma_instance, c->tx_dma_channel, dma_tx_tc_cb);
        dma_ch_to_uart[c->dma_instance][c->tx_dma_channel] = uart;

        /* 启动 DMA */
        dma_start_channel(c->dma_instance, c->tx_dma_channel);

        return actual_size;
    }
#endif /* RT_SERIAL_USING_DMA */

    /* 中断发送模式（回调已在 _uart_configure 中注册） */
    uart->tx_buf  = buf;
    uart->tx_size = size;
    uart->tx_pos  = 0;
    uart->tx_done = RT_FALSE;

    /* 填充首批数据到 FIFO，触发后续 TX 中断 */
    while (uart->tx_pos < uart->tx_size)
    {
        if (!uart_putc_try(c->uart_num, buf[uart->tx_pos]))
            break;
        uart->tx_pos++;
    }

    /* 使能 TX 和 TC 中断（回调已注册，直接使能中断即可） */
    uart_interrupt_enable(c->uart_num, UART_INT_TX | UART_INT_TC);

    return size;
}

/* ==================== OPS 表 ==================== */

static const struct rt_uart_ops acm32_uart_ops =
{
    .configure    = _uart_configure,
    .control      = _uart_control,
    .putc         = _uart_putc,
    .getc         = _uart_getc,
    .transmit     = _uart_transmit,
};

/* ==================== 设备索引枚举 ==================== */

enum {
#ifdef BSP_USING_UART1
    UART1_INDEX,
#endif
    UART_MAX_INDEX,
};

/* ==================== 配置表 ==================== */

static struct acm32_uart_config uart_config[] = {
#ifdef BSP_USING_UART1
    UART1_CONFIG,
#endif
};

static struct acm32_uart uart_obj[UART_MAX_INDEX] = {0};

/* ==================== 初始化 ==================== */

rt_err_t rt_hw_uart_init(void)
{
    int n = sizeof(uart_obj) / sizeof(struct acm32_uart);
    struct serial_configure c = RT_SERIAL_CONFIG_DEFAULT;

    for (int i = 0; i < n; i++)
    {
        uart_obj[i].config = &uart_config[i];
        uart_obj[i].serial.ops = &acm32_uart_ops;
        uart_obj[i].serial.config = c;
        uart_obj[i].shadow_config = c;

        /* 注册到全局查找表（供 SDK 回调使用） */
        g_uart_instances[uart_config[i].uart_num] = &uart_obj[i];

        rt_err_t rc = rt_hw_serial_register(&uart_obj[i].serial,
            uart_obj[i].config->name,
            RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_INT_TX,
            NULL);
        RT_ASSERT(rc == RT_EOK);
    }
    return RT_EOK;
}

#endif /* RT_USING_SERIAL_V2 */
