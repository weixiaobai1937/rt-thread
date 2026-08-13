/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-06-04     AisinoChip   ACM32P4xx UART V2 driver
 *                              Support USART1-4 + LPUART1/2
 * 2026-07-06     AisinoChip   rewrite: HAL init + custom ISR, FIFO,
 *                              interrupt TX/RX, DMA RX with IDLE,
 *                              DMA TX, V2 ringbuffer integration
 * 2026-07-23     AisinoChip   pin/DMA from uart_config (Kconfig groups)
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <drivers/dev_serial_v2.h>
#include "board.h"
#include "uart_config.h"
#include "system_accelerate.h"

#ifdef RT_USING_SERIAL_V2

/* ==================== Constants ==================== */

#define UART_FIFO_DEPTH         16
#define UART_MAX_COUNT          6       /* USART1-4 + LPUART1-2 */

/* UART type */
enum { UART_TYPE_USART = 0, UART_TYPE_LPUART };

/* ==================== Runtime structure ==================== */

struct acm32_uart
{
    struct acm32_uart_config    *config;
    struct rt_serial_device     serial;

    /* HAL handle (used only for HAL_UART_Init, not in ISR) */
    union {
        UART_HandleTypeDef      usart;
        LPUART_HandleTypeDef    lpuart;
    } handle;

    /* Interrupt TX state */
    volatile const rt_uint8_t  *tx_buf;
    rt_size_t                   tx_size;
    rt_size_t                   tx_pos;
    volatile rt_bool_t          tx_done;

    /* Currently enabled interrupt mask */
    rt_uint32_t                 int_mask;

    /* DMA capability flags (set at registration; control maps BLOCKING to DMA/INT) */
    rt_uint16_t                 uart_dma_flag;

    /* DMA RX buffer */
    rt_uint8_t                  *rx_dma_ping_buf;
    rt_uint16_t                 rx_dma_bufsz;
    volatile rt_uint16_t        rx_dma_last_pos;

#ifdef HAL_DMA_MODULE_ENABLED
    DMA_HandleTypeDef           dma_tx;
    DMA_HandleTypeDef           dma_rx;
    rt_bool_t                   dma_tx_ready;
    volatile rt_bool_t          dma_tx_busy;
#endif
};

#define raw_to_uart(raw) rt_container_of(raw, struct acm32_uart, serial)

/* ==================== DMA RX callback forward declarations ==================== */

#ifdef HAL_DMA_MODULE_ENABLED
static void _dma_rx_half_cplt(DMA_HandleTypeDef *hdma);
static void _dma_rx_cplt(DMA_HandleTypeDef *hdma);
static void _dma_rx_err(DMA_HandleTypeDef *hdma);
static void _dma_tx_cplt(DMA_HandleTypeDef *hdma);
#endif

/* ==================== Global lookup table (ISR reverse mapping, DMA buffer index) ==================== */

static struct acm32_uart *g_uart_instances[UART_MAX_COUNT] = {NULL};

static struct acm32_uart *uart_find(void *instance)
{
    for (int i = 0; i < UART_MAX_COUNT; i++)
    {
        if (g_uart_instances[i] &&
            g_uart_instances[i]->config->Instance == instance)
            return g_uart_instances[i];
    }
    return NULL;
}

/* ==================== Register helpers (inline functions) ==================== */

rt_inline rt_uint32_t uart_reg_fr(void *inst, int type)
{
    if (type == UART_TYPE_USART)
        return ((USART_TypeDef *)inst)->FR;
    else
        return ((LPUART_TypeDef *)inst)->SR;
}

rt_inline void uart_reg_dr_write(void *inst, int type, rt_uint8_t c)
{
    if (type == UART_TYPE_USART)
        ((USART_TypeDef *)inst)->DR = c;
    else
        ((LPUART_TypeDef *)inst)->TXDR = c;
}

rt_inline rt_uint8_t uart_reg_dr_read(void *inst, int type)
{
    if (type == UART_TYPE_USART)
        return ((USART_TypeDef *)inst)->DR & 0xFF;
    else
        return ((LPUART_TypeDef *)inst)->RXDR & 0xFF;
}

rt_inline rt_uint32_t uart_reg_ie(void *inst, int type)
{
    if (type == UART_TYPE_USART)
        return ((USART_TypeDef *)inst)->IE;
    else
        return ((LPUART_TypeDef *)inst)->IE;
}

rt_inline void uart_reg_ie_set(void *inst, int type, rt_uint32_t val)
{
    if (type == UART_TYPE_USART)
        ((USART_TypeDef *)inst)->IE = val;
    else
        ((LPUART_TypeDef *)inst)->IE = val;
}

rt_inline void uart_reg_isr_clear(void *inst, int type, rt_uint32_t val)
{
    if (type == UART_TYPE_USART)
        ((USART_TypeDef *)inst)->ISR = val;
    else
        ((LPUART_TypeDef *)inst)->SR = val;
}

rt_inline rt_uint32_t uart_reg_isr(void *inst, int type)
{
    if (type == UART_TYPE_USART)
        return ((USART_TypeDef *)inst)->ISR;
    else
        return ((LPUART_TypeDef *)inst)->SR;
}

/* USART flag/interrupt/ISR bits */
#define U_FR_TXFF    USART_FR_TXFF
#define U_FR_RXFE    USART_FR_RXFE

#define U_IE_RXI     USART_IE_RXI
#define U_IE_TXI     USART_IE_TXI
#define U_IE_TCI     USART_IE_TCI
#define U_IE_IDLEI   USART_IE_IDLEI

#define U_ISR_RXI    USART_ISR_RXI
#define U_ISR_TXI    USART_ISR_TXI
#define U_ISR_TCI    USART_ISR_TCI
#define U_ISR_IDLEI  USART_ISR_IDLEI

/*
 * LPUART uses the official bit definitions (do not use hand-written magic numbers).
 * Note that the FR semantics are opposite to USART polarity:
 *   USART TXFF=1 full / RXFE=1 empty
 *   LPUART TXE=1 writable / RXF=1 data available
 */
#define L_IE_RXI     LPUART_IE_RXIE
#define L_IE_TXI     LPUART_IE_TXEIE
#define L_IE_TCI     LPUART_IE_TCIE
#define L_IE_IDLEI   LPUART_IE_IDLEIE

#define L_ISR_RXI    LPUART_SR_RXIF
#define L_ISR_TXI    LPUART_SR_TXEIF
#define L_ISR_TCI    LPUART_SR_TCIF
#define L_ISR_IDLEI  LPUART_SR_IDLEIF

/* Select bit by type (only for same-polarity bits such as IE/ISR) */
#define _BIT(type, usart_bit, lpuart_bit) \
    ((type) == UART_TYPE_USART ? (usart_bit) : (lpuart_bit))

/* Error bit mask (USART ISR / LPUART SR) */
#define U_ERR_MASK   (USART_ISR_OEI | USART_ISR_BEI | USART_ISR_PEI | USART_ISR_FEI)
#define L_ERR_MASK   (LPUART_SR_RXOVIF | LPUART_SR_FEIF | LPUART_SR_PEIF)

/* FR semantics helper: USART and LPUART have opposite polarity, bit mask cannot be shared */
rt_inline rt_bool_t uart_tx_full(void *inst, int type)
{
    if (type == UART_TYPE_USART)
        return (uart_reg_fr(inst, type) & U_FR_TXFF) ? RT_TRUE : RT_FALSE;
    /* LPUART: TXE=1 means writable */
    return (uart_reg_fr(inst, type) & LPUART_SR_TXE) ? RT_FALSE : RT_TRUE;
}

rt_inline rt_bool_t uart_rx_empty(void *inst, int type)
{
    if (type == UART_TYPE_USART)
        return (uart_reg_fr(inst, type) & U_FR_RXFE) ? RT_TRUE : RT_FALSE;
    /* LPUART: RXF=1 means data available */
    return (uart_reg_fr(inst, type) & LPUART_SR_RXF) ? RT_FALSE : RT_TRUE;
}

/* ==================== Pins / clocks (from uart_config Kconfig group) ==================== */

static void acm32_uart_gpio_clk_enable(GPIO_TypeDef *port)
{
    if (port == GPIOA)
        __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (port == GPIOB)
        __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (port == GPIOC)
        __HAL_RCC_GPIOC_CLK_ENABLE();
    else if (port == GPIOD)
        __HAL_RCC_GPIOD_CLK_ENABLE();
    else if (port == GPIOE)
        __HAL_RCC_GPIOE_CLK_ENABLE();
    else if (port == GPIOF)
        __HAL_RCC_GPIOF_CLK_ENABLE();
    else if (port == GPIOG)
        __HAL_RCC_GPIOG_CLK_ENABLE();
}

static void acm32_uart_periph_clk_enable(void *inst)
{
    if (inst == USART1)
        __HAL_RCC_USART1_CLK_ENABLE();
    else if (inst == USART2)
        __HAL_RCC_USART2_CLK_ENABLE();
    else if (inst == USART3)
        __HAL_RCC_USART3_CLK_ENABLE();
    else if (inst == USART4)
        __HAL_RCC_USART4_CLK_ENABLE();
    else if (inst == LPUART1)
        __HAL_RCC_LPUART1_CLK_ENABLE();
    else if (inst == LPUART2)
        __HAL_RCC_LPUART2_CLK_ENABLE();
}

static void acm32_uart_msp_pins(struct acm32_uart_config *c)
{
    GPIO_InitTypeDef g = {0};

    if (c == NULL)
        return;

    acm32_uart_periph_clk_enable(c->Instance);
    acm32_uart_gpio_clk_enable(c->tx_port);
    acm32_uart_gpio_clk_enable(c->rx_port);

    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_PULLUP;
    g.Drive = GPIO_DRIVE_LEVEL3;

    if (c->tx_pin)
    {
        g.Pin = c->tx_pin;
        g.Alternate = c->tx_af;
        HAL_GPIO_Init(c->tx_port, &g);
    }
    if (c->rx_pin)
    {
        g.Pin = c->rx_pin;
        g.Alternate = c->rx_af;
        HAL_GPIO_Init(c->rx_port, &g);
    }
}

/* ==================== MspInit (HAL callback: GPIO + clock + NVIC) ==================== */

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    struct acm32_uart *uart = uart_find(huart->Instance);

    if (uart == NULL || uart->config == NULL)
        return;

    acm32_uart_msp_pins(uart->config);
}

void HAL_LPUART_MspInit(LPUART_HandleTypeDef *hlpuart)
{
    struct acm32_uart *uart = uart_find(hlpuart->Instance);

    if (uart == NULL || uart->config == NULL)
        return;

    acm32_uart_msp_pins(uart->config);
}

/* ==================== DMA start/stop (V2: started at control CONFIG) ==================== */

#ifdef HAL_DMA_MODULE_ENABLED
static void _dma_clk_enable(DMA_Channel_TypeDef *ch)
{
    if ((rt_uint32_t)ch < (rt_uint32_t)DMA2_Channel0)
        __HAL_RCC_DMA1_CLK_ENABLE();
    else
        __HAL_RCC_DMA2_CLK_ENABLE();
}

/* Shared by HTC/TC/IDLE: report RX_DMADONE by HW current position (must be atomic to prevent double reporting) */
static void _dma_rx_report_tail(struct acm32_uart *uart)
{
    rt_uint16_t cur_pos;
    rt_uint16_t tail;
    rt_base_t level;

    if (uart->dma_rx.Instance == NULL || uart->rx_dma_bufsz == 0)
        return;

    level = rt_hw_interrupt_disable();

    cur_pos = uart->rx_dma_bufsz -
        (rt_uint16_t)__HAL_DMA_GET_TRANSFER_SIZE(&uart->dma_rx);
    __DSB();

    if (cur_pos == uart->rx_dma_last_pos)
    {
        rt_hw_interrupt_enable(level);
        return;
    }

    System_InvalidateDAccelerate_by_Addr((volatile void *)uart->rx_dma_ping_buf,
                                         (int32_t)uart->rx_dma_bufsz);

    if (cur_pos > uart->rx_dma_last_pos)
        tail = cur_pos - uart->rx_dma_last_pos;
    else
        tail = (uart->rx_dma_bufsz - uart->rx_dma_last_pos) + cur_pos;

    uart->rx_dma_last_pos = cur_pos;
    rt_hw_interrupt_enable(level);

    rt_hw_serial_isr(&uart->serial,
        RT_SERIAL_EVENT_RX_DMADONE | ((rt_uint32_t)tail << 8));
}

static void _uart_dma_rx_stop(struct acm32_uart *uart)
{
    struct acm32_uart_config *c = uart->config;
    void *inst = c->Instance;
    int type = c->uart_type;

    if (uart->dma_rx.Instance == NULL)
        return;

    /* Report the remaining tail before stopping to avoid losing trailing data */
    _dma_rx_report_tail(uart);

    uart_reg_ie_set(inst, type,
        uart_reg_ie(inst, type) & ~_BIT(type, U_IE_IDLEI, L_IE_IDLEI));
    uart->int_mask &= ~_BIT(type, U_IE_IDLEI, L_IE_IDLEI);

    if (type == UART_TYPE_USART)
        CLEAR_BIT(((USART_TypeDef *)inst)->CR1, USART_CR1_RXDMAE);
    else if (!uart->dma_tx_busy)
        /* LPUART DMA_EN is shared: do not clear while TX is in flight */
        CLEAR_BIT(((LPUART_TypeDef *)inst)->CR, LPUART_CR_DMA_EN);

    NVIC_DisableIRQ(c->rx_dma_irq);
    HAL_DMA_Abort(&uart->dma_rx);
    HAL_DMA_DeInit(&uart->dma_rx);
    uart->dma_rx.Instance = NULL;
    uart->rx_dma_ping_buf = NULL;
    uart->rx_dma_bufsz = 0;
    uart->rx_dma_last_pos = 0;
}

static void _uart_dma_tx_stop(struct acm32_uart *uart)
{
    struct acm32_uart_config *c = uart->config;
    void *inst = c->Instance;
    int type = c->uart_type;

    if (uart->dma_tx.Instance == NULL && !uart->dma_tx_ready)
        return;

    if (type == UART_TYPE_USART)
        CLEAR_BIT(((USART_TypeDef *)inst)->CR1, USART_CR1_TXDMAE);
    /* LPUART DMA_EN is shared; cleared by _uart_dma_rx_stop / CLOSE only when RX is also stopped */

    if (c->tx_dma_instance != UART_DMA_NONE)
        NVIC_DisableIRQ(c->tx_dma_irq);

    if (uart->dma_tx.Instance)
    {
        HAL_DMA_Abort(&uart->dma_tx);
        HAL_DMA_DeInit(&uart->dma_tx);
        uart->dma_tx.Instance = NULL;
    }
    uart->dma_tx_ready = RT_FALSE;
    uart->dma_tx_busy = RT_FALSE;
}

static rt_err_t _uart_dma_rx_start(struct acm32_uart *uart)
{
    struct acm32_uart_config *c = uart->config;
    void *inst = c->Instance;
    int type = c->uart_type;
    rt_uint8_t *rx_dma_buf = NULL;
    rt_uint16_t rx_dma_bufsz;

    if (c->rx_dma_instance == UART_DMA_NONE)
        return -RT_EINVAL;

    if (uart->dma_rx.Instance)
        _uart_dma_rx_stop(uart);

    rt_hw_serial_control_isr(&uart->serial,
        RT_HW_SERIAL_CTRL_GET_DMA_PING_BUF, &rx_dma_buf);
    if (rx_dma_buf == NULL)
        return -RT_ERROR;

    rx_dma_bufsz = uart->serial.config.dma_ping_bufsz;
    if (rx_dma_bufsz == 0)
        return -RT_EINVAL;

    _dma_clk_enable(c->rx_dma_instance);

    uart->dma_rx.Instance     = c->rx_dma_instance;
    uart->dma_rx.Channel      = c->rx_dma_channel;
    uart->dma_rx.Init.Mode        = DMA_MODE_CIRCULAR;
    uart->dma_rx.Init.DataFlow    = DMA_DATAFLOW_P2M;
    uart->dma_rx.Init.ReqID       = c->rx_dma_reqid;
    uart->dma_rx.Init.SrcIncDec   = DMA_SRCINCDEC_DISABLE;
    uart->dma_rx.Init.DestIncDec  = DMA_DESTINCDEC_INC;
    uart->dma_rx.Init.SrcWidth    = DMA_SRCWIDTH_BYTE;
    uart->dma_rx.Init.DestWidth   = DMA_DESTWIDTH_BYTE;
    uart->dma_rx.Init.SrcBurst    = DMA_SRCBURST_1;
    uart->dma_rx.Init.DestBurst   = DMA_DESTBURST_1;
    uart->dma_rx.Init.SrcMaster   = DMA_SRCMASTER_1;
    uart->dma_rx.Init.DestMaster  = DMA_DESTMASTER_1;
    uart->dma_rx.Init.Lock        = 0;
    uart->dma_rx.Init.NextMaster  = 0;
    if (HAL_DMA_Init(&uart->dma_rx) != HAL_OK)
    {
        uart->dma_rx.Instance = NULL;
        return -RT_ERROR;
    }

    uart->dma_rx.Parent = uart;
    uart->dma_rx.XferHalfCpltCallback = _dma_rx_half_cplt;
    uart->dma_rx.XferCpltCallback     = _dma_rx_cplt;
    uart->dma_rx.XferErrorCallback    = _dma_rx_err;
    uart->rx_dma_ping_buf = rx_dma_buf;
    uart->rx_dma_bufsz = rx_dma_bufsz;
    uart->rx_dma_last_pos = 0;

    if (type == UART_TYPE_USART)
        SET_BIT(((USART_TypeDef *)inst)->CR1, USART_CR1_RXDMAE);
    else
        SET_BIT(((LPUART_TypeDef *)inst)->CR, LPUART_CR_DMA_EN);

    if (HAL_DMA_Start_IT(&uart->dma_rx,
            (rt_uint32_t)(type == UART_TYPE_USART ?
                &((USART_TypeDef *)inst)->DR :
                &((LPUART_TypeDef *)inst)->RXDR),
            (rt_uint32_t)rx_dma_buf,
            rx_dma_bufsz) != HAL_OK)
    {
        _uart_dma_rx_stop(uart);
        return -RT_ERROR;
    }

    NVIC_SetPriority(c->rx_dma_irq, 2);
    NVIC_EnableIRQ(c->rx_dma_irq);

    /* Disable byte-by-byte RXI, switch to IDLE tail processing */
    uart_reg_ie_set(inst, type,
        uart_reg_ie(inst, type) & ~_BIT(type, U_IE_RXI, L_IE_RXI));
    uart->int_mask &= ~_BIT(type, U_IE_RXI, L_IE_RXI);
    uart_reg_ie_set(inst, type,
        uart_reg_ie(inst, type) | _BIT(type, U_IE_IDLEI, L_IE_IDLEI));
    uart->int_mask |= _BIT(type, U_IE_IDLEI, L_IE_IDLEI);

    NVIC_SetPriority(c->irq_type, 2);
    NVIC_EnableIRQ(c->irq_type);
    return RT_EOK;
}

static rt_err_t _uart_dma_tx_prepare(struct acm32_uart *uart)
{
    struct acm32_uart_config *c = uart->config;

    if (c->tx_dma_instance == UART_DMA_NONE)
        return -RT_EINVAL;

    if (uart->dma_tx_ready)
        return RT_EOK;

    _dma_clk_enable(c->tx_dma_instance);

    uart->dma_tx.Instance     = c->tx_dma_instance;
    uart->dma_tx.Channel      = c->tx_dma_channel;
    uart->dma_tx.Init.Mode        = DMA_MODE_NORMAL;
    uart->dma_tx.Init.DataFlow    = DMA_DATAFLOW_M2P;
    uart->dma_tx.Init.ReqID       = c->tx_dma_reqid;
    uart->dma_tx.Init.SrcIncDec   = DMA_SRCINCDEC_INC;
    uart->dma_tx.Init.DestIncDec  = DMA_DESTINCDEC_DISABLE;
    uart->dma_tx.Init.SrcWidth    = DMA_SRCWIDTH_BYTE;
    uart->dma_tx.Init.DestWidth   = DMA_DESTWIDTH_BYTE;
    uart->dma_tx.Init.SrcBurst    = DMA_SRCBURST_1;
    uart->dma_tx.Init.DestBurst   = DMA_DESTBURST_1;
    uart->dma_tx.Init.SrcMaster   = DMA_SRCMASTER_1;
    uart->dma_tx.Init.DestMaster  = DMA_DESTMASTER_1;
    uart->dma_tx.Init.Lock        = 0;
    uart->dma_tx.Init.NextMaster  = 0;
    if (HAL_DMA_Init(&uart->dma_tx) != HAL_OK)
    {
        uart->dma_tx.Instance = NULL;
        return -RT_ERROR;
    }

    uart->dma_tx.Parent = uart;
    uart->dma_tx.XferCpltCallback = _dma_tx_cplt;
    NVIC_SetPriority(c->tx_dma_irq, 2);
    NVIC_EnableIRQ(c->tx_dma_irq);
    uart->dma_tx_ready = RT_TRUE;
    return RT_EOK;
}
#endif /* HAL_DMA_MODULE_ENABLED */

/* ==================== OPS: configure ==================== */

static rt_err_t _uart_configure(struct rt_serial_device *serial,
                                 struct serial_configure *cfg)
{
    struct acm32_uart *uart = raw_to_uart(serial);
    struct acm32_uart_config *c = uart->config;
    void *inst = c->Instance;
    int type = c->uart_type;
#ifdef HAL_DMA_MODULE_ENABLED
    rt_bool_t dma_rx_was_active = (uart->dma_rx.Instance != NULL) ? RT_TRUE : RT_FALSE;
    rt_bool_t dma_tx_was_ready = uart->dma_tx_ready;
#endif

#ifdef HAL_DMA_MODULE_ENABLED
    /* Runtime reconfigure: stop DMA first, then restore the previous mode after HAL resets the peripheral */
    if (dma_rx_was_active)
        _uart_dma_rx_stop(uart);
    if (dma_tx_was_ready)
        _uart_dma_tx_stop(uart);
#endif

    /* Only hardware parameter init; DMA is started at control(CONFIG) once serial_rx is ready */
    if (type == UART_TYPE_USART)
    {
        uart->handle.usart.Instance          = (USART_TypeDef *)inst;
        uart->handle.usart.Init.BaudRate     = cfg->baud_rate;
        uart->handle.usart.Init.WordLength   =
            (cfg->data_bits == DATA_BITS_9) ? UART_WORDLENGTH_9B : UART_WORDLENGTH_8B;
        uart->handle.usart.Init.StopBits     =
            (cfg->stop_bits == STOP_BITS_2) ? UART_STOPBITS_2 : UART_STOPBITS_1;
        if (cfg->parity == PARITY_ODD)
            uart->handle.usart.Init.Parity = UART_PARITY_ODD;
        else if (cfg->parity == PARITY_EVEN)
            uart->handle.usart.Init.Parity = UART_PARITY_EVEN;
        else
            uart->handle.usart.Init.Parity = UART_PARITY_NONE;
        uart->handle.usart.Init.Mode       = UART_MODE_TX_RX;
        uart->handle.usart.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
        if (HAL_UART_Init(&uart->handle.usart) != HAL_OK)
            return -RT_ERROR;
    }
    else
    {
        uart->handle.lpuart.Instance          = (LPUART_TypeDef *)inst;
        uart->handle.lpuart.Init.BaudRate     = cfg->baud_rate;
        uart->handle.lpuart.Init.WordLength   = LPUART_WORDLENGTH_8B;
        uart->handle.lpuart.Init.StopBits     =
            (cfg->stop_bits == STOP_BITS_2) ? LPUART_STOPBITS_2B : LPUART_STOPBITS_1B;
        uart->handle.lpuart.Init.Parity       = LPUART_PARITY_NONE;
        if (cfg->parity == PARITY_ODD)
            uart->handle.lpuart.Init.Parity = LPUART_PARITY_ODD;
        else if (cfg->parity == PARITY_EVEN)
            uart->handle.lpuart.Init.Parity = LPUART_PARITY_EVEN;
        uart->handle.lpuart.Init.Mode         = LPUART_MODE_TXRX;
        uart->handle.lpuart.Init.Polarity     = 0;
        uart->handle.lpuart.Init.ClockSource  = 0;
        if (HAL_LPUART_Init(&uart->handle.lpuart) != HAL_OK)
            return -RT_ERROR;
    }

    if (type == UART_TYPE_USART)
    {
        MODIFY_REG(((USART_TypeDef *)inst)->CR3,
                   USART_CR3_RXIFLSEL_Msk | USART_CR3_TXIFLSEL_Msk,
                   USART_RX_FIFO_1_16 | USART_TX_FIFO_1_16);
    }

#ifdef HAL_DMA_MODULE_ENABLED
    if (dma_rx_was_active)
    {
        /* Restore DMA RX (no forced RXI, to avoid a dual path with DMA) */
        if (_uart_dma_rx_start(uart) != RT_EOK)
            return -RT_ERROR;
    }
    else
#endif
    {
        /* INT RX: enable RXI only when the device is open (serial_rx exists) */
        if (serial->serial_rx != RT_NULL)
        {
            uart->int_mask = _BIT(type, U_IE_RXI, L_IE_RXI);
            uart_reg_ie_set(inst, type,
                uart_reg_ie(inst, type) | _BIT(type, U_IE_RXI, L_IE_RXI));
            NVIC_SetPriority(c->irq_type, 2);
            NVIC_EnableIRQ(c->irq_type);
        }
        else
        {
            uart->int_mask = 0;
        }
    }

#ifdef HAL_DMA_MODULE_ENABLED
    if (dma_tx_was_ready)
    {
        if (_uart_dma_tx_prepare(uart) != RT_EOK)
            return -RT_ERROR;
    }
#endif

    return RT_EOK;
}

/* ==================== OPS: control ==================== */

static rt_err_t _uart_control(struct rt_serial_device *serial,
                               int cmd, void *arg)
{
    struct acm32_uart *uart = raw_to_uart(serial);
    struct acm32_uart_config *c = uart->config;
    void *inst = c->Instance;
    int type = c->uart_type;
    rt_ubase_t ctrl_arg = (rt_ubase_t)arg;

    /* V2: BLOCKING/NON_BLOCKING -> DMA or INT (per registered capability) */
    if (ctrl_arg & (RT_DEVICE_FLAG_RX_BLOCKING | RT_DEVICE_FLAG_RX_NON_BLOCKING))
    {
        if (uart->uart_dma_flag & RT_DEVICE_FLAG_DMA_RX)
            ctrl_arg = RT_DEVICE_FLAG_DMA_RX;
        else
            ctrl_arg = RT_DEVICE_FLAG_INT_RX;
    }
    else if (ctrl_arg & (RT_DEVICE_FLAG_TX_BLOCKING | RT_DEVICE_FLAG_TX_NON_BLOCKING))
    {
        if (uart->uart_dma_flag & RT_DEVICE_FLAG_DMA_TX)
            ctrl_arg = RT_DEVICE_FLAG_DMA_TX;
        else
            ctrl_arg = RT_DEVICE_FLAG_INT_TX;
    }

    switch (cmd)
    {
    case RT_DEVICE_CTRL_CLR_INT:
        if (ctrl_arg == RT_DEVICE_FLAG_INT_RX)
        {
            uart_reg_ie_set(inst, type,
                uart_reg_ie(inst, type) & ~_BIT(type, U_IE_RXI, L_IE_RXI));
            uart->int_mask &= ~_BIT(type, U_IE_RXI, L_IE_RXI);
        }
        else if (ctrl_arg == RT_DEVICE_FLAG_INT_TX)
        {
            uart_reg_ie_set(inst, type,
                uart_reg_ie(inst, type)
                & ~(_BIT(type, U_IE_TXI, L_IE_TXI) | _BIT(type, U_IE_TCI, L_IE_TCI)));
            uart->int_mask &= ~(_BIT(type, U_IE_TXI, L_IE_TXI) | _BIT(type, U_IE_TCI, L_IE_TCI));
        }
#ifdef HAL_DMA_MODULE_ENABLED
        else if (ctrl_arg == RT_DEVICE_FLAG_DMA_RX)
        {
            _uart_dma_rx_stop(uart);
        }
        else if (ctrl_arg == RT_DEVICE_FLAG_DMA_TX)
        {
            _uart_dma_tx_stop(uart);
        }
#endif
        break;

    case RT_DEVICE_CTRL_SET_INT:
        if (ctrl_arg == RT_DEVICE_FLAG_INT_RX)
        {
            NVIC_SetPriority(c->irq_type, 2);
            NVIC_EnableIRQ(c->irq_type);
            uart_reg_ie_set(inst, type,
                uart_reg_ie(inst, type) | _BIT(type, U_IE_RXI, L_IE_RXI));
            uart->int_mask |= _BIT(type, U_IE_RXI, L_IE_RXI);
        }
        else if (ctrl_arg == RT_DEVICE_FLAG_INT_TX)
        {
            NVIC_SetPriority(c->irq_type, 2);
            NVIC_EnableIRQ(c->irq_type);
            uart_reg_ie_set(inst, type,
                uart_reg_ie(inst, type)
                | _BIT(type, U_IE_TXI, L_IE_TXI)
                | _BIT(type, U_IE_TCI, L_IE_TCI));
            uart->int_mask |= _BIT(type, U_IE_TXI, L_IE_TXI)
                           |  _BIT(type, U_IE_TCI, L_IE_TCI);
        }
        else
        {
            return -RT_EINVAL;
        }
        break;

    case RT_DEVICE_CTRL_CONFIG:
#ifdef HAL_DMA_MODULE_ENABLED
        if (ctrl_arg == RT_DEVICE_FLAG_DMA_RX)
            return _uart_dma_rx_start(uart);
        if (ctrl_arg == RT_DEVICE_FLAG_DMA_TX)
            return _uart_dma_tx_prepare(uart);
#endif
        return _uart_control(serial, RT_DEVICE_CTRL_SET_INT, (void *)ctrl_arg);

    case RT_DEVICE_CHECK_OPTMODE:
        if (ctrl_arg & RT_DEVICE_FLAG_DMA_TX)
            return RT_SERIAL_TX_BLOCKING_NO_BUFFER;
        return RT_SERIAL_TX_BLOCKING_BUFFER;

    case RT_DEVICE_CTRL_CLOSE:
        if (uart->int_mask)
        {
            uart_reg_ie_set(inst, type,
                uart_reg_ie(inst, type) & ~uart->int_mask);
            uart->int_mask = 0;
        }
        NVIC_DisableIRQ(c->irq_type);
#ifdef HAL_DMA_MODULE_ENABLED
        _uart_dma_rx_stop(uart);
        _uart_dma_tx_stop(uart);
        if (type == UART_TYPE_LPUART)
            CLEAR_BIT(((LPUART_TypeDef *)inst)->CR, LPUART_CR_DMA_EN);
#endif
        break;
    }
    return RT_EOK;
}

/* ==================== OPS: putc / getc ==================== */

static int _uart_putc(struct rt_serial_device *serial, char ch)
{
    struct acm32_uart *uart = raw_to_uart(serial);
    void *inst = uart->config->Instance;
    int type = uart->config->uart_type;
    rt_uint32_t spins = 100000U;

    while (uart_tx_full(inst, type))
    {
        if (--spins == 0)
            return -RT_ETIMEOUT;
    }
    uart_reg_dr_write(inst, type, (rt_uint8_t)ch);
    return 1;
}

static int _uart_getc(struct rt_serial_device *serial)
{
    struct acm32_uart *uart = raw_to_uart(serial);
    void *inst = uart->config->Instance;
    int type = uart->config->uart_type;

    if (!uart_rx_empty(inst, type))
        return uart_reg_dr_read(inst, type);
    return -RT_EEMPTY;
}

/* ==================== OPS: transmit (interrupt + DMA) ==================== */

static rt_ssize_t _uart_transmit(struct rt_serial_device *serial,
                                  rt_uint8_t *buf, rt_size_t size,
                                  rt_uint32_t tx_flag)
{
    struct acm32_uart *uart = raw_to_uart(serial);
    struct acm32_uart_config *c = uart->config;
    void *inst = c->Instance;
    int type = c->uart_type;

    if (size == 0) return 0;

#ifdef HAL_DMA_MODULE_ENABLED
    /* DMA TX: capability determined by uart_dma_flag; NVIC enabled at CONFIG time */
    if ((uart->uart_dma_flag & RT_DEVICE_FLAG_DMA_TX) &&
        c->tx_dma_instance != UART_DMA_NONE)
    {
        if (uart->dma_tx_busy)
            return -RT_EBUSY;

        if (_uart_dma_tx_prepare(uart) != RT_EOK)
            return -RT_EIO;

        /* If the channel is still EN, abort before starting to avoid cutting a partial packet */
        if (uart->dma_tx.Instance &&
            (uart->dma_tx.Instance->CXCONFIG & DMA_CXCONFIG_EN))
        {
            HAL_DMA_Abort(&uart->dma_tx);
        }

        if (type == UART_TYPE_USART)
            SET_BIT(((USART_TypeDef *)inst)->CR1, USART_CR1_TXDMAE);
        else
            SET_BIT(((LPUART_TypeDef *)inst)->CR, LPUART_CR_DMA_EN);

        uart->dma_tx_busy = RT_TRUE;
        System_CleanDAccelerate_by_Addr((volatile void *)buf, (int32_t)size);
        if (HAL_DMA_Start_IT(&uart->dma_tx,
                (rt_uint32_t)buf,
                (rt_uint32_t)(type == UART_TYPE_USART ?
                    &((USART_TypeDef *)inst)->DR :
                    &((LPUART_TypeDef *)inst)->TXDR),
                size) != HAL_OK)
        {
            uart->dma_tx_busy = RT_FALSE;
            return -RT_EIO;
        }

        return size;
    }
#endif

    /* Interrupt TX mode */
    if (uart->tx_buf != RT_NULL && !uart->tx_done)
        return -RT_EBUSY;

    uart->tx_buf  = buf;
    uart->tx_size = size;
    uart->tx_pos  = 0;
    uart->tx_done = RT_FALSE;

    /* Prefill the FIFO */
    while (uart->tx_pos < uart->tx_size)
    {
        if (uart_tx_full(inst, type))
            break;
        uart_reg_dr_write(inst, type, buf[uart->tx_pos++]);
    }

    /* Enable TXI + TCI */
    uart->int_mask |= _BIT(type, U_IE_TXI, L_IE_TXI)
                   |  _BIT(type, U_IE_TCI, L_IE_TCI);
    uart_reg_ie_set(inst, type,
        uart_reg_ie(inst, type)
        | _BIT(type, U_IE_TXI, L_IE_TXI)
        | _BIT(type, U_IE_TCI, L_IE_TCI));

    return size;
}

/* ==================== OPS table ==================== */

static const struct rt_uart_ops acm32_uart_ops =
{
    .configure    = _uart_configure,
    .control      = _uart_control,
    .putc         = _uart_putc,
    .getc         = _uart_getc,
    .transmit     = _uart_transmit,
};

/* ==================== Unified ISR ==================== */

static void uart_isr(struct acm32_uart *uart)
{
    void *inst = uart->config->Instance;
    int type = uart->config->uart_type;

    rt_uint32_t isr = uart_reg_isr(inst, type);
    rt_uint32_t ie  = uart_reg_ie(inst, type);

    /* ---- Clear error flags (overrun/frame/parity, etc.) to prevent continuous interrupts ---- */
    rt_uint32_t err_mask = _BIT(type, U_ERR_MASK, L_ERR_MASK);
    if (isr & err_mask)
    {
        uart_reg_isr_clear(inst, type, isr & err_mask);
    }

    /* ---- RXI: feed hardware FIFO data into the V2 ringbuffer ---- */
    if ((ie & _BIT(type, U_IE_RXI, L_IE_RXI)) &&
        (isr & _BIT(type, U_ISR_RXI, L_ISR_RXI)))
    {
        uart_reg_isr_clear(inst, type, _BIT(type, U_ISR_RXI, L_ISR_RXI));

        while (!uart_rx_empty(inst, type))
        {
            rt_uint8_t ch = uart_reg_dr_read(inst, type);
            rt_hw_serial_control_isr(&uart->serial, RT_HW_SERIAL_CTRL_PUTC, &ch);
        }
        rt_hw_serial_isr(&uart->serial, RT_SERIAL_EVENT_RX_IND);
    }

    /* ---- TXI: fill the TX FIFO ---- */
    if ((ie & _BIT(type, U_IE_TXI, L_IE_TXI)) &&
        (isr & _BIT(type, U_ISR_TXI, L_ISR_TXI)))
    {
        uart_reg_isr_clear(inst, type, _BIT(type, U_ISR_TXI, L_ISR_TXI));

        while (uart->tx_buf && uart->tx_pos < uart->tx_size)
        {
            if (uart_tx_full(inst, type))
                break;
            uart_reg_dr_write(inst, type, uart->tx_buf[uart->tx_pos++]);
        }

        /* All data filled: disable TXI */
        if (uart->tx_pos >= uart->tx_size)
        {
            uart_reg_ie_set(inst, type,
                ie & ~_BIT(type, U_IE_TXI, L_IE_TXI));
            uart->int_mask &= ~_BIT(type, U_IE_TXI, L_IE_TXI);
            ie &= ~_BIT(type, U_IE_TXI, L_IE_TXI);
        }
    }

    /* ---- TCI: transmission complete ---- */
    if ((ie & _BIT(type, U_IE_TCI, L_IE_TCI)) &&
        (isr & _BIT(type, U_ISR_TCI, L_ISR_TCI)))
    {
        uart_reg_isr_clear(inst, type, _BIT(type, U_ISR_TCI, L_ISR_TCI));
        uart_reg_ie_set(inst, type,
            ie & ~_BIT(type, U_IE_TCI, L_IE_TCI));
        uart->int_mask &= ~_BIT(type, U_IE_TCI, L_IE_TCI);
        ie &= ~_BIT(type, U_IE_TCI, L_IE_TCI);

        uart->tx_buf = NULL;
        uart->tx_done = RT_TRUE;
        rt_hw_serial_isr(&uart->serial, RT_SERIAL_EVENT_TX_DONE);
    }

    /* ---- IDLEI: DMA RX frame-tail processing ---- */
    if ((ie & _BIT(type, U_IE_IDLEI, L_IE_IDLEI)) &&
        (isr & _BIT(type, U_ISR_IDLEI, L_ISR_IDLEI)))
    {
        uart_reg_isr_clear(inst, type, _BIT(type, U_ISR_IDLEI, L_ISR_IDLEI));

#ifdef HAL_DMA_MODULE_ENABLED
        /* circular DMA: report tail by HW position only; CPU must not write the ping buffer */
        _dma_rx_report_tail(uart);
#endif
    }
}

/* ==================== DMA RX callbacks ==================== */

#ifdef HAL_DMA_MODULE_ENABLED
static void _dma_rx_half_cplt(DMA_HandleTypeDef *hdma)
{
    _dma_rx_report_tail((struct acm32_uart *)hdma->Parent);
}

static void _dma_rx_cplt(DMA_HandleTypeDef *hdma)
{
    _dma_rx_report_tail((struct acm32_uart *)hdma->Parent);
}

static void _dma_rx_err(DMA_HandleTypeDef *hdma)
{
    struct acm32_uart *uart = (struct acm32_uart *)hdma->Parent;
    void *inst = uart->config->Instance;
    int type = uart->config->uart_type;
    rt_uint8_t *ping_buf;
    rt_uint16_t ping_sz;
    rt_uint32_t src;

    if (uart->rx_dma_ping_buf == NULL || uart->rx_dma_bufsz == 0)
        return;

    /* Report the written-but-unconsumed tail before aborting */
    _dma_rx_report_tail(uart);

    ping_buf = uart->rx_dma_ping_buf;
    ping_sz = uart->rx_dma_bufsz;

    uart_reg_isr_clear(inst, type, _BIT(type, U_ERR_MASK, L_ERR_MASK));
    HAL_DMA_Abort(&uart->dma_rx);

    src = (rt_uint32_t)(type == UART_TYPE_USART ?
        &((USART_TypeDef *)inst)->DR :
        &((LPUART_TypeDef *)inst)->RXDR);

    if (type == UART_TYPE_USART)
        SET_BIT(((USART_TypeDef *)inst)->CR1, USART_CR1_RXDMAE);
    else
        SET_BIT(((LPUART_TypeDef *)inst)->CR, LPUART_CR_DMA_EN);

    if (HAL_DMA_Start_IT(&uart->dma_rx, src,
            (rt_uint32_t)ping_buf, ping_sz) == HAL_OK)
    {
        uart->rx_dma_last_pos = 0;
    }
}

static void _dma_tx_cplt(DMA_HandleTypeDef *hdma)
{
    struct acm32_uart *uart = (struct acm32_uart *)hdma->Parent;
    void *inst = uart->config->Instance;
    int type = uart->config->uart_type;

    if (type == UART_TYPE_USART)
        CLEAR_BIT(((USART_TypeDef *)inst)->CR1, USART_CR1_TXDMAE);
    /* LPUART: DMA_EN is a shared bit (TX+RX), not cleared on TX completion */

    uart->dma_tx_busy = RT_FALSE;
    rt_hw_serial_isr(&uart->serial, RT_SERIAL_EVENT_TX_DMADONE);
}
#endif

/* ==================== ISR entry ==================== */

#define UART_IRQ_HANDLER(irq_name, uart_obj_ptr)     \
    void irq_name##_IRQHandler(void)                 \
    {                                                \
        rt_interrupt_enter();                        \
        uart_isr(uart_obj_ptr);                      \
        rt_interrupt_leave();                        \
    }

/* ==================== Device index enumeration ==================== */

enum {
#ifdef BSP_USING_UART1
    UART1_INDEX,
#endif
#ifdef BSP_USING_UART2
    UART2_INDEX,
#endif
#ifdef BSP_USING_UART3
    UART3_INDEX,
#endif
#ifdef BSP_USING_UART4
    UART4_INDEX,
#endif
#ifdef BSP_USING_LPUART1
    LPUART1_INDEX,
#endif
#ifdef BSP_USING_LPUART2
    LPUART2_INDEX,
#endif
    UART_MAX_INDEX,
};

/* ==================== Config table ==================== */

static struct acm32_uart_config uart_config[] = {
#ifdef BSP_USING_UART1
    UART1_CONFIG,
#endif
#ifdef BSP_USING_UART2
    UART2_CONFIG,
#endif
#ifdef BSP_USING_UART3
    UART3_CONFIG,
#endif
#ifdef BSP_USING_UART4
    UART4_CONFIG,
#endif
#ifdef BSP_USING_LPUART1
    LPUART1_CONFIG,
#endif
#ifdef BSP_USING_LPUART2
    LPUART2_CONFIG,
#endif
};

static struct acm32_uart uart_obj[UART_MAX_INDEX] = {0};

/* ==================== Initialization ==================== */

rt_err_t rt_hw_uart_init(void)
{
    int n = sizeof(uart_obj) / sizeof(struct acm32_uart);
    struct serial_configure cfg = RT_SERIAL_CONFIG_DEFAULT;

    for (int i = 0; i < n; i++)
    {
        uart_obj[i].config = &uart_config[i];
        uart_obj[i].serial.ops = &acm32_uart_ops;
        uart_obj[i].serial.config = cfg;

        rt_uint32_t flags = RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_INT_TX;
        uart_obj[i].uart_dma_flag = 0;
        if (uart_obj[i].config->rx_dma_instance != UART_DMA_NONE)
        {
            flags |= RT_DEVICE_FLAG_DMA_RX;
            uart_obj[i].uart_dma_flag |= RT_DEVICE_FLAG_DMA_RX;
#ifdef BSP_UART_RX_BUFSZ
            uart_obj[i].serial.config.rx_bufsz = BSP_UART_RX_BUFSZ;
#else
            uart_obj[i].serial.config.rx_bufsz = 1024;
#endif
#ifdef BSP_UART_DMA_PING_BUFSZ
            uart_obj[i].serial.config.dma_ping_bufsz = BSP_UART_DMA_PING_BUFSZ;
#else
            uart_obj[i].serial.config.dma_ping_bufsz = 512;
#endif
        }
        if (uart_obj[i].config->tx_dma_instance != UART_DMA_NONE)
        {
            flags |= RT_DEVICE_FLAG_DMA_TX;
            uart_obj[i].uart_dma_flag |= RT_DEVICE_FLAG_DMA_TX;
        }

        g_uart_instances[i] = &uart_obj[i];

        rt_err_t rc = rt_hw_serial_register(&uart_obj[i].serial,
            uart_obj[i].config->name, flags, NULL);
        RT_ASSERT(rc == RT_EOK);
    }
    return RT_EOK;
}

/* ==================== ISR instantiation ==================== */

#ifdef BSP_USING_UART1
UART_IRQ_HANDLER(USART1, &uart_obj[UART1_INDEX])
#endif
#ifdef BSP_USING_UART2
UART_IRQ_HANDLER(USART2, &uart_obj[UART2_INDEX])
#endif
#ifdef BSP_USING_UART3
UART_IRQ_HANDLER(USART3, &uart_obj[UART3_INDEX])
#endif
#ifdef BSP_USING_UART4
UART_IRQ_HANDLER(USART4, &uart_obj[UART4_INDEX])
#endif
#ifdef BSP_USING_LPUART1
UART_IRQ_HANDLER(LPUART1, &uart_obj[LPUART1_INDEX])
#endif
#ifdef BSP_USING_LPUART2
UART_IRQ_HANDLER(LPUART2, &uart_obj[LPUART2_INDEX])
#endif

/* ==================== DMA IRQ instantiation (all channels covered, compatible with any Kconfig preset) ==================== */

#ifdef HAL_DMA_MODULE_ENABLED
/*
 * Each DMA channel IRQ handler searches all uart_obj entries,
 * and calls HAL_DMA_IRQHandler once the RX or TX DMA instance matches.
 * Thus the correct IRQ handler is always defined regardless of the Kconfig DMA preset.
 */
#define ACM32_DMA_IRQ_HANDLER(irq_name)                              \
    void irq_name##_IRQHandler(void)                                 \
    {                                                                \
        rt_interrupt_enter();                                      \
        for (int _i = 0; _i < UART_MAX_INDEX; _i++)                \
        {                                                            \
            if (uart_obj[_i].dma_rx.DMA &&                          \
                uart_obj[_i].dma_rx.DMA->INTSTATUS &                \
                (1UL << uart_obj[_i].dma_rx.Channel))               \
            {                                                        \
                HAL_DMA_IRQHandler(&uart_obj[_i].dma_rx);           \
            }                                                        \
            if (uart_obj[_i].dma_tx.DMA &&                          \
                uart_obj[_i].dma_tx.DMA->INTSTATUS &                \
                (1UL << uart_obj[_i].dma_tx.Channel))               \
            {                                                        \
                HAL_DMA_IRQHandler(&uart_obj[_i].dma_tx);           \
            }                                                        \
        }                                                            \
        rt_interrupt_leave();                                        \
    }

/* DMA1 channels 0-3 */
#ifndef BSP_USING_I2S1
ACM32_DMA_IRQ_HANDLER(DMA1_CH0)
#endif
ACM32_DMA_IRQ_HANDLER(DMA1_CH1)
ACM32_DMA_IRQ_HANDLER(DMA1_CH2)
ACM32_DMA_IRQ_HANDLER(DMA1_CH3)
/* DMA2 channels 0-3 */
ACM32_DMA_IRQ_HANDLER(DMA2_CH0)
ACM32_DMA_IRQ_HANDLER(DMA2_CH1)
ACM32_DMA_IRQ_HANDLER(DMA2_CH2)
ACM32_DMA_IRQ_HANDLER(DMA2_CH3)
#endif /* HAL_DMA_MODULE_ENABLED */

#endif /* RT_USING_SERIAL_V2 */
