/**
 * @file uart.c
 * @brief ACM32P4 UART驱动实现
 * @author ACM32P4 SDK Team
 * @version 1.0
 * 
 * 本文件实现了ACM32P4芯片的UART驱动功能，提供了完整的串口控制接口。
 * 
 * 主要功能：
 * - 基础UART配置（波特率、字长、校验、停止位等）
 * - FIFO配置和管理
 * - 硬件流控支持
 * - DMA传输支持
 * - 中断管理和回调机制
 * - 数据收发操作
 * - 扩展功能（LIN、IrDA、智能卡、RS485、同步模式等）
 */

#include <assert.h>
#include <string.h>
#include "hardware/uart.h"
#include "hardware/clocks.h"
#include "system.h"
#include "device/acm32p4xx.h"

//===========================================
// 内部宏定义
//===========================================

#define UART_TX_TIMEOUT_MS      10U         ///< 发送超时毫秒数
#define UART_RX_TIMEOUT_MS      100U        ///< 接收超时毫秒数

//===========================================
// 内部类型定义
//===========================================

/**
 * @brief UART回调函数集合
 */
typedef struct {
    uart_rx_callback_t      rx_callback;     ///< 接收回调
    uart_tx_callback_t      tx_callback;     ///< 发送回调
    uart_error_callback_t   error_callback;  ///< 错误回调
    uart_idle_callback_t    idle_callback;   ///< 空闲回调
} uart_callbacks_t;

//===========================================
// 内部状态变量
//===========================================

/**
 * @brief UART回调函数表
 */
static uart_callbacks_t uart_callbacks[8] = {0};

/**
 * @brief UART寄存器基地址表
 */
static UART_TypeDef * const uart_instances[8] = {
    (UART_TypeDef *)UART1_BASE,
    (UART_TypeDef *)UART2_BASE,
    (UART_TypeDef *)UART3_BASE,
    (UART_TypeDef *)UART4_BASE,
    (UART_TypeDef *)UART5_BASE,
    (UART_TypeDef *)UART6_BASE,
    (UART_TypeDef *)UART7_BASE,
    (UART_TypeDef *)UART8_BASE
};

/**
 * @brief UART时钟使能位表（外设编号）
 */
static const clock_periph_t uart_clock_periph[8] = {
    CLK_UART1,   // UART1 on APB2
    CLK_UART2,   // UART2 on APB1
    CLK_UART3,   // UART3 on APB1
    CLK_UART4,   // UART4 on APB1
    CLK_UART5,   // UART5 on APB1
    CLK_UART6,   // UART6 on APB2
    CLK_UART7,   // UART7 on APB1
    CLK_UART8    // UART8 on APB1
};

//===========================================
// 内部辅助函数
//===========================================

/**
 * @brief 获取UART寄存器指针
 * @param uart UART外设编号
 * @return UART寄存器指针
 */
static inline UART_TypeDef* uart_get_instance(uart_num_t uart)
{
    assert(uart < 8);
    return uart_instances[uart];
}

/**
 * @brief 获取UART外设时钟频率
 * @param uart UART外设编号
 * @return 时钟频率（Hz）
 */
static uint32_t uart_get_clock_freq(uart_num_t uart)
{
    if (uart == UART_1 || uart == UART_6) {
        return clock_get_pclk2_hz();
    } else {
        return clock_get_pclk1_hz();
    }
}

/**
 * @brief 计算波特率寄存器值
 * @param uart UART外设编号
 * @param baudrate 目标波特率
 * @param ibaud 输出：整数部分
 * @param fbaud 输出：小数部分
 * @return 成功返回true，失败返回false
 */
static bool uart_calc_baudrate(uart_num_t uart, uint32_t baudrate, 
                               uint16_t *ibaud, uint8_t *fbaud)
{
    uint32_t pclk = uart_get_clock_freq(uart);
    
    if (baudrate == 0 || pclk == 0) {
        return false;
    }
    
    // 根据文档公式：
    // UART_IBAUD = integer(FPCLK/(16*BAUD))
    // UART_FBAUD = integer(badf*64 + 0.5)，badf为小数部分
    
    uint32_t divisor = baudrate << 4;  // 16 * baudrate
    
    // 计算整数部分
    *ibaud = (uint16_t)(pclk / divisor);
    
    // 计算小数部分（四舍五入）
    uint32_t remainder = pclk % divisor;
    // fbaud = (remainder * 64 + divisor/2) / divisor，加divisor/2实现四舍五入
    *fbaud = (uint8_t)(((uint64_t)remainder * 64 + (divisor >> 1)) / divisor);
    
    // 处理进位情况
    if (*fbaud >= 64) {
        *fbaud = 0;
        (*ibaud)++;
    }
    
    return (*ibaud > 0 && *ibaud <= 0xFFFFU);
}

//===========================================
// 第1层：快速初始化API
//===========================================

bool uart_init_default(uart_num_t uart, uint32_t baudrate)
{
    assert(uart < 8);
    
    uart_config_t config = {
        .baudrate = baudrate,
        .word_length = UART_WORD_LENGTH_8BIT,
        .stop_bits = UART_STOP_BITS_1,
        .parity = UART_PARITY_NONE,
        .endian = UART_ENDIAN_LSB_FIRST,
        .fifo_enable = false
    };
    
    return uart_init_custom(uart, &config);
}

bool uart_init_custom(uart_num_t uart, const uart_config_t *config)
{
    assert(uart < 8);
    assert(config != NULL);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    uart_enable_clock(uart);
    
    uart_disable(uart);
    
    if (!uart_config_baudrate(uart, config->baudrate)) {
        return false;
    }
    
    uart_config_word_length(uart, config->word_length);
    uart_config_stop_bits(uart, config->stop_bits);
    uart_config_parity(uart, config->parity);
    uart_config_endian(uart, config->endian);
    
    /* 根据配置使能或禁用 FIFO */
    if (config->fifo_enable) {
        uart_base->CR3 |= (1 << 5);   /* FEN = 1，使能 FIFO */
    } else {
        uart_base->CR3 &= ~(1 << 5);  /* FEN = 0，禁用 FIFO */
    }
    
    uart_enable(uart);
    uart_tx_enable(uart);
    uart_rx_enable(uart);
    
    return true;
}

void uart_deinit(uart_num_t uart)
{
    assert(uart < 8);
    
    uart_disable(uart);
    
    uart_callbacks[uart].rx_callback = NULL;
    uart_callbacks[uart].tx_callback = NULL;
    uart_callbacks[uart].error_callback = NULL;
    uart_callbacks[uart].idle_callback = NULL;
    
    clock_periph_disable(uart_clock_periph[uart]);
}

//===========================================
// 第2层：基础配置API
//===========================================

bool uart_config_baudrate(uart_num_t uart, uint32_t baudrate)
{
    assert(uart < 8);
    
    uint16_t ibaud;
    uint8_t fbaud;
    
    if (!uart_calc_baudrate(uart, baudrate, &ibaud, &fbaud)) {
        return false;
    }
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    /* 手册：BRR bit[21:6]=IBAUD, bit[5:0]=FBAUD，bit[31:22]保留 */
    uart_base->BRR = ((uint32_t)ibaud << 6) | (fbaud & 0x3FU);
    
    return true;
}

void uart_config_word_length(uart_num_t uart, uart_word_length_t word_length)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    uart_base->CR3 &= ~(0x7 << 6);
    uart_base->CR3 |= ((uint32_t)word_length << 6);
}

void uart_config_stop_bits(uart_num_t uart, uart_stop_bits_t stop_bits)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    if (stop_bits == UART_STOP_BITS_1) {
        uart_base->CR3 &= ~(1 << 3);
    } else {
        uart_base->CR3 |= (1 << 3);
    }
}

void uart_config_parity(uart_num_t uart, uart_parity_t parity)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    switch (parity) {
        case UART_PARITY_NONE:
            uart_base->CR3 &= ~(1 << 1);
            break;
        
        case UART_PARITY_EVEN:
            uart_base->CR3 |= (1 << 1);
            uart_base->CR3 &= ~(1 << 9);
            uart_base->CR3 |= (1 << 2);
            break;
        
        case UART_PARITY_ODD:
            uart_base->CR3 |= (1 << 1);
            uart_base->CR3 &= ~(1 << 9);
            uart_base->CR3 &= ~(1 << 2);
            break;
        
        case UART_PARITY_ZERO:
            uart_base->CR3 |= (1 << 1);
            uart_base->CR3 |= (1 << 9);
            uart_base->CR3 |= (1 << 2);
            break;
        
        case UART_PARITY_ONE:
            uart_base->CR3 |= (1 << 1);
            uart_base->CR3 |= (1 << 9);
            uart_base->CR3 &= ~(1 << 2);
            break;
    }
}

void uart_config_endian(uart_num_t uart, uart_endian_t endian)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    if (endian == UART_ENDIAN_LSB_FIRST) {
        uart_base->CR1 &= ~(1 << 16);
    } else {
        uart_base->CR1 |= (1 << 16);
    }
}

void uart_config_fifo(uart_num_t uart, const uart_fifo_config_t *config)
{
    assert(uart < 8);
    assert(config != NULL);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    if (config->enable) {
        uart_base->CR3 |= (1 << 5);
        
        /* 手册：UART5/7/8 仅支持 4 字节 FIFO，阈值不能超过 4 字节 */
        uint8_t max_threshold = 0x05;  /* 默认最大阈值索引 */
        if (uart == UART_5 || uart == UART_7 || uart == UART_8) {
            max_threshold = 0x01;  /* UART5/7/8 最大 1/4 满（4字节） */
        }
        
        uint8_t rx_thresh = (uint8_t)config->rx_threshold;
        if (rx_thresh > max_threshold) {
            rx_thresh = max_threshold;
        }
        uart_base->CR3 &= ~(0x7 << 13);
        uart_base->CR3 |= ((uint32_t)rx_thresh << 13);
        
        uint8_t tx_thresh = (uint8_t)config->tx_threshold;
        if (tx_thresh > max_threshold) {
            tx_thresh = max_threshold;
        }
        uart_base->CR3 &= ~(0x7 << 10);
        uart_base->CR3 |= ((uint32_t)tx_thresh << 10);
    } else {
        uart_base->CR3 &= ~(1 << 5);
    }
}

void uart_config_flow_control(uart_num_t uart, const uart_flow_control_config_t *config)
{
    assert(uart < 8);
    assert(config != NULL);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    if (config->cts_enable) {
        uart_base->CR1 |= (1 << 15);
    } else {
        uart_base->CR1 &= ~(1 << 15);
    }
    
    if (config->rts_enable) {
        uart_base->CR1 |= (1 << 14);
    } else {
        uart_base->CR1 &= ~(1 << 14);
    }
}

void uart_config_lin(uart_num_t uart, const uart_lin_config_t *config)
{
    assert(uart < 8);
    assert(config != NULL);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    if (config->enable) {
        /* 只清除 BCNT_VALUE(bit[23:0]) 和 LBD_CNT(bit[31:28])，保留 RS485 的 DEM/DEP/AUTO_START_EN */
        uart_base->BCNT &= ~(0xFU << 28);         /* 清除 LBD_CNT */
        uart_base->BCNT &= ~0x00FFFFFFU;           /* 清除 BCNT_VALUE */
        uart_base->BCNT |= (config->break_length & 0x00FFFFFFU);
        uart_base->BCNT |= ((config->lbd_cnt & 0xFU) << 28);
    } else {
        uart_base->BCNT &= ~0x00FFFFFFU;           /* 清除 BCNT_VALUE */
    }
}

void uart_config_irda(uart_num_t uart, const uart_irda_config_t *config)
{
    assert(uart < 8);
    assert(config != NULL);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    if (config->enable) {
        uart_base->CR1 |= (1 << 1);
        
        if (config->low_power) {
            uart_base->CR1 |= (1 << 2);
        } else {
            uart_base->CR1 &= ~(1 << 2);
        }
        
        /* GTPR.PSC 为 bit[7:0]，IrDA 模式下 PSC = integer(Fsys/(16*ILPBAUD)) */
        uart_base->GTPR &= ~0xFFU;
        uart_base->GTPR |= (config->prescaler & 0xFFU);
    } else {
        uart_base->CR1 &= ~(1 << 1);
    }
}

void uart_config_smartcard(uart_num_t uart, const uart_smartcard_config_t *config)
{
    assert(uart < 8);
    assert(config != NULL);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    if (config->enable) {
        uart_base->CR2 |= (1 << 5);
        
        uart_base->CR2 |= (1 << 0);
        
        uart_base->CR3 |= (1 << 1);
        
        uart_base->CR2 |= (1 << 7);
        
        if (config->nack_enable) {
            uart_base->CR2 |= (1 << 6);
        } else {
            uart_base->CR2 &= ~(1 << 6);
        }
        
        uart_base->GTPR &= ~0x1F;
        uart_base->GTPR |= (config->prescaler & 0x1F);
        
        uart_base->GTPR &= ~(0xF << 8);
        uart_base->GTPR |= ((config->guard_time & 0xF) << 8);
        
        /* 手册：智能卡重发机制需要关闭发送 FIFO，检测到 FE 时软件重发 */
        if (config->auto_retry) {
            uart_base->CR3 &= ~(1 << 5);  /* 关闭 FIFO（FEN=0） */
        }
        
        if (config->block_timeout > 0) {
            uart_base->BCNT &= ~0x00FFFFFFU;
            uart_base->BCNT |= (config->block_timeout & 0x00FFFFFFU);
            uart_base->BCNT |= (1 << 25);
        }
    } else {
        uart_base->CR2 &= ~((1 << 0) | (1 << 5) | (1 << 6) | (1 << 7));
        uart_base->CR3 &= ~(1 << 1);
    }
}

void uart_config_half_duplex(uart_num_t uart, bool enable)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    if (enable) {
        uart_base->CR2 |= (1 << 0);
    } else {
        uart_base->CR2 &= ~(1 << 0);
    }
}

void uart_config_multiprocessor(uart_num_t uart, const uart_multiprocessor_config_t *config)
{
    assert(uart < 8);
    assert(config != NULL);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    if (config->enable) {
        if (config->wakeup_method == UART_WAKEUP_IDLE_LINE) {
            uart_base->CR2 &= ~(1 << 3);
        } else {
            uart_base->CR2 |= (1 << 3);
        }
        
        if (config->address_mode == UART_ADDRESS_MODE_4BIT) {
            uart_base->CR2 &= ~(1 << 18);
        } else {
            uart_base->CR2 |= (1 << 18);
        }
        
        uart_base->CR2 &= ~(0x7F << 11);
        uart_base->CR2 |= ((config->address & 0x7F) << 11);
    } else {
        uart_base->CR2 &= ~((1 << 3) | (1 << 18));
    }
}

void uart_config_sync(uart_num_t uart, const uart_sync_config_t *config)
{
    assert(uart < 8);
    assert(config != NULL);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    if (config->enable) {
        uart_base->CR2 |= (1 << 7);
        
        uart_base->CR1 &= ~(1 << 1);
        uart_base->CR2 &= ~(1 << 0);
        uart_base->CR2 &= ~(1 << 5);
        
        if (config->clock_polarity == UART_CLOCK_POLARITY_HIGH) {
            uart_base->CR2 |= (1 << 10);
        } else {
            uart_base->CR2 &= ~(1 << 10);
        }
        
        if (config->clock_phase == UART_CLOCK_PHASE_2ND_EDGE) {
            uart_base->CR2 |= (1 << 9);
        } else {
            uart_base->CR2 &= ~(1 << 9);
        }
        
        if (config->last_bit_clock) {
            uart_base->CR2 |= (1 << 8);
        } else {
            uart_base->CR2 &= ~(1 << 8);
        }
    } else {
        uart_base->CR2 &= ~(1 << 7);
    }
}

void uart_config_rs485(uart_num_t uart, const uart_rs485_config_t *config)
{
    assert(uart < 8);
    assert(config != NULL);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    if (config->enable) {
        uart_base->BCNT |= (1 << 27);
        
        if (config->de_polarity == UART_DE_POLARITY_LOW) {
            uart_base->BCNT |= (1 << 26);
        } else {
            uart_base->BCNT &= ~(1 << 26);
        }
        
        /* 手册：RS485 模式下 BCNT_VALUE[7:4]=DEAT, [3:0]=DEDT，清除全部 24 位 */
        uart_base->BCNT &= ~0x00FFFFFFU;
        uint32_t timing = ((uint32_t)(config->de_assert & 0xFU) << 4) | (config->de_deassert & 0xFU);
        uart_base->BCNT |= timing;
    } else {
        uart_base->BCNT &= ~(1 << 27);
    }
}

//===========================================
// 第3层：高级功能API（DMA、中断）
//===========================================

void uart_config_dma(uart_num_t uart, const uart_dma_config_t *config)
{
    assert(uart < 8);
    assert(config != NULL);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    if (config->tx_dma_enable) {
        uart_base->CR1 |= (1 << 4);
    } else {
        uart_base->CR1 &= ~(1 << 4);
    }
    
    if (config->rx_dma_enable) {
        uart_base->CR1 |= (1 << 3);
    } else {
        uart_base->CR1 &= ~(1 << 3);
    }
    
    if (config->dma_disable_on_error) {
        uart_base->CR1 |= (1 << 5);
    } else {
        uart_base->CR1 &= ~(1 << 5);
    }
}

void uart_interrupt_enable(uart_num_t uart, uint32_t interrupt_mask)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    uart_base->IE |= interrupt_mask;
}

void uart_interrupt_disable(uart_num_t uart, uint32_t interrupt_mask)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    uart_base->IE &= ~interrupt_mask;
}

void uart_register_rx_callback(uart_num_t uart, uart_rx_callback_t callback)
{
    assert(uart < 8);
    
    uart_callbacks[uart].rx_callback = callback;
}

void uart_register_tx_callback(uart_num_t uart, uart_tx_callback_t callback)
{
    assert(uart < 8);
    
    uart_callbacks[uart].tx_callback = callback;
}

void uart_register_error_callback(uart_num_t uart, uart_error_callback_t callback)
{
    assert(uart < 8);
    
    uart_callbacks[uart].error_callback = callback;
}

void uart_register_idle_callback(uart_num_t uart, uart_idle_callback_t callback)
{
    assert(uart < 8);
    
    uart_callbacks[uart].idle_callback = callback;
}

//===========================================
// 第4层：控制与查询API
//===========================================


void uart_enable_clock(uart_num_t uart)
{
    assert(uart < 8);
    
    clock_periph_enable(uart_clock_periph[uart]);
}

void uart_disable_clock(uart_num_t uart)
{
    assert(uart < 8);
    
    clock_periph_disable(uart_clock_periph[uart]);
}

void uart_enable(uart_num_t uart)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    uart_base->CR1 |= (1 << 0);
}

void uart_disable(uart_num_t uart)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    uart_base->CR1 &= ~(1 << 0);
}

void uart_tx_enable(uart_num_t uart)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    uart_base->CR1 |= (1 << 8);
}

void uart_tx_disable(uart_num_t uart)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    uart_base->CR1 &= ~(1 << 8);
}

void uart_rx_enable(uart_num_t uart)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    uart_base->CR1 |= (1 << 9);
}

void uart_rx_disable(uart_num_t uart)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    uart_base->CR1 &= ~(1 << 9);
}

void uart_set_rts(uart_num_t uart, bool state)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    if (state) {
        uart_base->CR1 |= (1 << 11);
    } else {
        uart_base->CR1 &= ~(1 << 11);
    }
}

bool uart_get_cts(uart_num_t uart)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    return (uart_base->FR & (1 << 8)) ? true : false;
}

bool uart_putc(uart_num_t uart, uint8_t data)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    uint64_t start = system_get_tick();
    while ((uart_base->FR & (1 << 5)) && !system_elapsed(start, UART_TX_TIMEOUT_MS)) {
    }
    
    if (uart_base->FR & (1 << 5)) {
        return false;
    }

    uart_base->DR = data;
    return true;
}

uint8_t uart_getc(uart_num_t uart)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);

    uint64_t start = system_get_tick();
    while ((uart_base->FR & (1 << 4)) && !system_elapsed(start, UART_RX_TIMEOUT_MS)) {
    }
    
    if (uart_base->FR & (1 << 4)) {
        return 0;
    }

    return (uint8_t)(uart_base->DR & 0x1FF);
}

void uart_puts(uart_num_t uart, const char *str)
{
    assert(uart < 8);
    assert(str != NULL);
    
    while (*str) {
        uart_putc(uart, (uint8_t)*str);
        str++;
    }
}

uint32_t uart_write(uart_num_t uart, const uint8_t *data, uint32_t length)
{
    assert(uart < 8);
    assert(data != NULL);
    
    uint32_t count = 0;
    
    for (uint32_t i = 0; i < length; i++) {
        uart_putc(uart, data[i]);
        count++;
    }
    
    return count;
}

uint32_t uart_read(uart_num_t uart, uint8_t *buffer, uint32_t length)
{
    assert(uart < 8);
    assert(buffer != NULL);
    
    uint32_t count = 0;
    
    for (uint32_t i = 0; i < length; i++) {
        if (!uart_is_rx_fifo_empty(uart)) {
            buffer[i] = uart_getc(uart);
            count++;
        } else {
            break;
        }
    }
    
    return count;
}

bool uart_putc_try(uart_num_t uart, uint8_t data)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    if (uart_base->FR & (1 << 5)) {
        return false;
    }
    
    uart_base->DR = data;
    return true;
}

bool uart_getc_try(uart_num_t uart, uint8_t *data)
{
    assert(uart < 8);
    assert(data != NULL);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    if (uart_base->FR & (1 << 4)) {
        return false;
    }
    
    *data = (uint8_t)(uart_base->DR & 0x1FF);
    return true;
}

bool uart_is_tx_fifo_empty(uart_num_t uart)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    return (uart_base->FR & (1 << 7)) ? true : false;
}

bool uart_is_tx_fifo_full(uart_num_t uart)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    return (uart_base->FR & (1 << 5)) ? true : false;
}

bool uart_is_rx_fifo_empty(uart_num_t uart)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    return (uart_base->FR & (1 << 4)) ? true : false;
}

bool uart_is_rx_fifo_full(uart_num_t uart)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    return (uart_base->FR & (1 << 6)) ? true : false;
}

bool uart_is_busy(uart_num_t uart)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    return (uart_base->FR & (1 << 9)) ? true : false;
}

uint32_t uart_get_interrupt_status(uart_num_t uart)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    return uart_base->ISR;
}

void uart_clear_interrupt(uart_num_t uart, uint32_t interrupt_mask)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    /* ISR 寄存器为 RCW1 类型，写 1 的位清零，写 0 的位无影响 */
    uart_base->ISR = interrupt_mask;
}

uart_error_t uart_get_error(uart_num_t uart)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    uart_error_t error = UART_ERROR_NONE;
    
    if (uart_base->FR & (1 << 0)) {
        error |= UART_ERROR_FE;
    }
    if (uart_base->FR & (1 << 1)) {
        error |= UART_ERROR_PE;
    }
    if (uart_base->FR & (1 << 2)) {
        error |= UART_ERROR_BE;
    }
    if (uart_base->FR & (1 << 3)) {
        error |= UART_ERROR_OE;
    }
    
    return error;
}

void uart_clear_error(uart_num_t uart)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    /* 手册：FR 中的 FE/PE/BE/OE 标志通过读 DR 清除 */
    (void)uart_base->DR;
    /* ISR 中的 FEI/PEI/BEI/OEI 中断标志通过写 1 清除（RCW1） */
    uart_base->ISR = (UART_INT_FE | UART_INT_PE | UART_INT_BE | UART_INT_OE);
}

void uart_autobaud_start(uart_num_t uart)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    uart_base->CR2 |= (1 << 4);
}

void uart_autobaud_stop(uart_num_t uart)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    uart_base->CR2 &= ~(1 << 4);
}

bool uart_is_autobaud_complete(uart_num_t uart)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    return (uart_base->ISR & (1 << 14)) ? true : false;
}

void uart_enter_mute_mode(uart_num_t uart)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    uart_base->CR2 |= (1 << 2);
}

void uart_exit_mute_mode(uart_num_t uart)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    uart_base->CR2 &= ~(1 << 2);
}

void uart_send_lin_break(uart_num_t uart, uint32_t length)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    uart_base->BCNT &= ~0x00FFFFFFU;
    uart_base->BCNT |= (length & 0x00FFFFFFU);
    
    uart_base->BCNT |= (1 << 24);
    
    uart_base->CR3 |= (1 << 0);
    
    uint64_t start = system_get_tick();
    while (!(uart_base->ISR & (1 << 12)) && !system_elapsed(start, UART_TX_TIMEOUT_MS)) {
    }
    
    uart_base->ISR = UART_INT_BCNT;
    uart_base->CR3 &= ~(1 << 0);
}

void uart_bcnt_auto_start_enable(uart_num_t uart, bool enable)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    
    if (enable) {
        uart_base->BCNT |= (1 << 25);
    } else {
        uart_base->BCNT &= ~(1 << 25);
    }
}

uint32_t uart_get_base_address(uart_num_t uart)
{
    assert(uart < 8);
    
    return (uint32_t)uart_instances[uart];
}

//===========================================
// 中断处理函数（供用户在中断服务函数中调用）
//===========================================

/**
 * @brief UART中断处理函数
 * 
 * @param uart UART外设编号
 * 
 * @note 用户需要在对应的UART中断服务函数中调用此函数
 */
void uart_irq_handler(uart_num_t uart)
{
    assert(uart < 8);
    
    UART_TypeDef *uart_base = uart_get_instance(uart);
    uint32_t isr = uart_base->ISR;
    uint32_t ie = uart_base->IE;
    
    if ((isr & UART_INT_RX) && (ie & UART_INT_RX)) {
        if (uart_callbacks[uart].rx_callback != NULL) {
            /* 限制单次中断最多读取 256 字节，防止死循环 */
            uint32_t rx_limit = 256;
            while (!(uart_base->FR & (1 << 4)) && rx_limit--) {
                uint8_t data = (uint8_t)(uart_base->DR & 0x1FF);
                uart_callbacks[uart].rx_callback(uart, data);
            }
        }
        uart_base->ISR = UART_INT_RX;
    }
    
    if ((isr & UART_INT_TX) && (ie & UART_INT_TX)) {
        if (uart_callbacks[uart].tx_callback != NULL) {
            uart_callbacks[uart].tx_callback(uart);
        }
        uart_base->ISR = UART_INT_TX;
    }
    
    if ((isr & (UART_INT_FE | UART_INT_PE | UART_INT_BE | UART_INT_OE))
        && (ie & (UART_INT_FE | UART_INT_PE | UART_INT_BE | UART_INT_OE))
        && uart_callbacks[uart].error_callback != NULL) {
        uart_error_t error = uart_get_error(uart);
        uart_callbacks[uart].error_callback(uart, error);
        uart_base->ISR =
            (UART_INT_FE | UART_INT_PE | UART_INT_BE | UART_INT_OE);
    }
    
    if ((isr & UART_INT_IDLE) && (ie & UART_INT_IDLE)) {
        if (uart_callbacks[uart].idle_callback != NULL) {
            uart_callbacks[uart].idle_callback(uart);
        }
        uart_base->ISR = UART_INT_IDLE;
    }
    
    if ((isr & UART_INT_TC) && (ie & UART_INT_TC)) {
        uart_base->ISR = UART_INT_TC;
    }
    
    if ((isr & UART_INT_LBD) && (ie & UART_INT_LBD)) {
        uart_base->ISR = UART_INT_LBD;
    }
    
    if ((isr & UART_INT_BCNT) && (ie & UART_INT_BCNT)) {
        uart_base->ISR = UART_INT_BCNT;
    }
    
    if ((isr & UART_INT_ABR) && (ie & UART_INT_ABR)) {
        uart_base->ISR = UART_INT_ABR;
    }
}
