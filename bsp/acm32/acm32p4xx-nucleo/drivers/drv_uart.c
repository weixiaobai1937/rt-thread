/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-06-04     AisinoChip   ACM32P4xx UART V2 driver
 *                              Support USART1-8 + LPUART1/2
 * 2026-07-06     AisinoChip   rewrite: HAL init + custom ISR, FIFO,
 *                              interrupt TX/RX, DMA RX with IDLE,
 *                              DMA TX, V2 ringbuffer integration
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <drivers/dev_serial_v2.h>
#include "board.h"
#include "uart_config.h"

#ifdef RT_USING_SERIAL_V2

/* ==================== 常量 ==================== */

#define UART_FIFO_DEPTH         16
#define UART_MAX_COUNT          10      /* USART1-8 + LPUART1-2 */

/* UART 类型 */
enum { UART_TYPE_USART = 0, UART_TYPE_LPUART };

/* ==================== 运行时结构体 ==================== */

struct acm32_uart
{
    struct acm32_uart_config    *config;
    struct rt_serial_device     serial;

    /* HAL handle（仅用于 HAL_UART_Init 初始化，不用于 ISR） */
    union {
        UART_HandleTypeDef      usart;
        LPUART_HandleTypeDef    lpuart;
    } handle;

    /* 中断发送状态 */
    volatile const rt_uint8_t  *tx_buf;
    rt_size_t                   tx_size;
    rt_size_t                   tx_pos;
    volatile rt_bool_t          tx_done;

    /* 当前使能的中断掩码 */
    rt_uint32_t                 int_mask;

    /* DMA 能力标志（注册时设置，control 中映射 BLOCKING→DMA/INT） */
    rt_uint16_t                 uart_dma_flag;

    /* DMA 接收缓冲区 */
    rt_uint8_t                  *rx_dma_ping_buf;
    rt_uint16_t                 rx_dma_bufsz;
    volatile rt_uint16_t        rx_dma_last_pos;

#ifdef HAL_DMA_MODULE_ENABLED
    DMA_HandleTypeDef           dma_tx;
    DMA_HandleTypeDef           dma_rx;
    rt_bool_t                   dma_tx_ready;
#endif
};

#define raw_to_uart(raw) rt_container_of(raw, struct acm32_uart, serial)

/* ==================== DMA RX 回调前向声明 ==================== */

#ifdef HAL_DMA_MODULE_ENABLED
static void _dma_rx_half_cplt(DMA_HandleTypeDef *hdma);
static void _dma_rx_cplt(DMA_HandleTypeDef *hdma);
static void _dma_rx_err(DMA_HandleTypeDef *hdma);
static void _dma_tx_cplt(DMA_HandleTypeDef *hdma);
#endif

/* ==================== 全局查找表（ISR 反向映射、DMA 缓冲区索引） ==================== */

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

/* ==================== 寄存器辅助（内联函数） ==================== */

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

/* USART 标志/中断/ISR 位（LPUART 对应值见下文） */
#define U_FR_TXFF    USART_FR_TXFF
#define U_FR_RXFE    USART_FR_RXFE
#define U_FR_BUSY    USART_FR_BUSY

#define U_IE_RXI     USART_IE_RXI
#define U_IE_TXI     USART_IE_TXI
#define U_IE_TCI     USART_IE_TCI
#define U_IE_IDLEI   USART_IE_IDLEI

#define U_ISR_RXI    USART_ISR_RXI
#define U_ISR_TXI    USART_ISR_TXI
#define U_ISR_TCI    USART_ISR_TCI
#define U_ISR_IDLEI  USART_ISR_IDLEI

/* LPUART 位（与 USART 不同的定义） */
#define L_FR_TXFF    (1 << 5)
#define L_FR_RXFE    (1 << 4)
#define L_FR_BUSY    (1 << 9)

#define L_IE_RXI     (1 << 4)
#define L_IE_TXI     (1 << 7)
#define L_IE_TCI     (1 << 8)
#define L_IE_IDLEI   (1 << 3)

#define L_ISR_RXI    (1 << 4)
#define L_ISR_TXI    (1 << 7)
#define L_ISR_TCI    (1 << 8)
#define L_ISR_IDLEI  (1 << 3)

/* 按类型选择位 */
#define _BIT(type, usart_bit, lpuart_bit) \
    ((type) == UART_TYPE_USART ? (usart_bit) : (lpuart_bit))

/* 错误位掩码（USART ISR / LPUART SR） */
#define U_ERR_MASK   (USART_ISR_OEI | USART_ISR_BEI | USART_ISR_PEI | USART_ISR_FEI)
#define L_ERR_MASK   (LPUART_SR_RXOVIF | LPUART_SR_FEIF | LPUART_SR_PEIF)

/* ==================== 引脚映射表（MspInit 驱动） ==================== */

typedef struct {
    void         *instance;
    GPIO_TypeDef *tx_port;   rt_uint32_t tx_pin;
    GPIO_TypeDef *rx_port;   rt_uint32_t rx_pin;
    rt_uint32_t   af;
} uart_pin_t;

static const uart_pin_t g_pin_map[] = {
#ifdef BSP_USING_UART1
    /* USART1: PA9(TX) PA10(RX) AF1 */
    { USART1, GPIOA, GPIO_PIN_9,  GPIOA, GPIO_PIN_10, GPIO_FUNCTION_1 },
#endif
#ifdef BSP_USING_UART2
    /* USART2: PD5(TX) PD6(RX) AF3 */
    { USART2, GPIOD, GPIO_PIN_5,  GPIOD, GPIO_PIN_6,  GPIO_FUNCTION_3 },
#endif
#ifdef BSP_USING_UART3
    /* USART3: PB10(TX) PB11(RX) AF1 */
    { USART3, GPIOB, GPIO_PIN_10, GPIOB, GPIO_PIN_11, GPIO_FUNCTION_1 },
#endif
#ifdef BSP_USING_UART4
    /* USART4: PC10(TX) PC11(RX) AF4 */
    { USART4, GPIOC, GPIO_PIN_10, GPIOC, GPIO_PIN_11, GPIO_FUNCTION_4 },
#endif
#ifdef BSP_USING_UART5
    { USART5, GPIOA, GPIO_PIN_0,  GPIOA, GPIO_PIN_1,  GPIO_FUNCTION_1 },
#endif
#ifdef BSP_USING_UART6
    { USART6, GPIOB, GPIO_PIN_0,  GPIOB, GPIO_PIN_1,  GPIO_FUNCTION_1 },
#endif
#ifdef BSP_USING_UART7
    { USART7, GPIOC, GPIO_PIN_0,  GPIOC, GPIO_PIN_1,  GPIO_FUNCTION_1 },
#endif
#ifdef BSP_USING_UART8
    { USART8, GPIOD, GPIO_PIN_0,  GPIOD, GPIO_PIN_1,  GPIO_FUNCTION_1 },
#endif
#ifdef BSP_USING_LPUART1
    /* LPUART1: PA2(TX) PA3(RX) AF3 */
    { LPUART1, GPIOA, GPIO_PIN_2,  GPIOA, GPIO_PIN_3,  GPIO_FUNCTION_3 },
#endif
#ifdef BSP_USING_LPUART2
    /* LPUART2: PB10(TX) PB11(RX) AF3 */
    { LPUART2, GPIOB, GPIO_PIN_10, GPIOB, GPIO_PIN_11, GPIO_FUNCTION_3 },
#endif
};

static const uart_pin_t *pin_find(void *instance)
{
    int n = sizeof(g_pin_map) / sizeof(g_pin_map[0]);
    for (int i = 0; i < n; i++)
    {
        if (g_pin_map[i].instance == instance)
            return &g_pin_map[i];
    }
    return NULL;
}

/* 时钟使能辅助函数 */
static void rcc_enable_by_instance(void *inst)
{
    if (inst == USART1) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_USART1_CLK_ENABLE();
    }
    else if (inst == USART2) {
        __HAL_RCC_GPIOD_CLK_ENABLE();
        __HAL_RCC_USART2_CLK_ENABLE();
    }
    else if (inst == USART3) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_USART3_CLK_ENABLE();
    }
    else if (inst == USART4) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_USART4_CLK_ENABLE();
    }
    else if (inst == USART5) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_USART5_CLK_ENABLE();
    }
    else if (inst == USART6) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_USART6_CLK_ENABLE();
    }
    else if (inst == USART7) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_USART7_CLK_ENABLE();
    }
    else if (inst == USART8) {
        __HAL_RCC_GPIOD_CLK_ENABLE();
        __HAL_RCC_USART8_CLK_ENABLE();
    }
    else if (inst == LPUART1) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_LPUART1_CLK_ENABLE();
    }
    else if (inst == LPUART2) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_LPUART2_CLK_ENABLE();
    }
}

/* ==================== MspInit（HAL 回调：GPIO + 时钟 + NVIC） ==================== */

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    const uart_pin_t *pin = pin_find(huart->Instance);
    if (pin == NULL) return;

    rcc_enable_by_instance(huart->Instance);

    GPIO_InitTypeDef g = {0};
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_PULLUP;
    g.Drive = GPIO_DRIVE_LEVEL3;
    g.Alternate = pin->af;

    if (pin->tx_pin)
    {
        g.Pin = pin->tx_pin;
        HAL_GPIO_Init(pin->tx_port, &g);
    }
    if (pin->rx_pin)
    {
        g.Pin = pin->rx_pin;
        HAL_GPIO_Init(pin->rx_port, &g);
    }
}

void HAL_LPUART_MspInit(LPUART_HandleTypeDef *hlpuart)
{
    const uart_pin_t *pin = pin_find(hlpuart->Instance);
    if (pin == NULL) return;

    rcc_enable_by_instance(hlpuart->Instance);

    GPIO_InitTypeDef g = {0};
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_PULLUP;
    g.Drive = GPIO_DRIVE_LEVEL3;
    g.Alternate = pin->af;

    if (pin->tx_pin)
    {
        g.Pin = pin->tx_pin;
        HAL_GPIO_Init(pin->tx_port, &g);
    }
    if (pin->rx_pin)
    {
        g.Pin = pin->rx_pin;
        HAL_GPIO_Init(pin->rx_port, &g);
    }
}

/* ==================== DMA 启停（V2: control CONFIG 时启动） ==================== */

#ifdef HAL_DMA_MODULE_ENABLED
static void _dma_clk_enable(DMA_Channel_TypeDef *ch)
{
    if ((rt_uint32_t)ch < (rt_uint32_t)DMA2_Channel0)
        __HAL_RCC_DMA1_CLK_ENABLE();
    else
        __HAL_RCC_DMA2_CLK_ENABLE();
}

static void _uart_dma_rx_stop(struct acm32_uart *uart)
{
    struct acm32_uart_config *c = uart->config;
    void *inst = c->Instance;
    int type = c->uart_type;

    if (uart->dma_rx.Instance == NULL)
        return;

    uart_reg_ie_set(inst, type,
        uart_reg_ie(inst, type) & ~_BIT(type, U_IE_IDLEI, L_IE_IDLEI));
    uart->int_mask &= ~_BIT(type, U_IE_IDLEI, L_IE_IDLEI);

    if (type == UART_TYPE_USART)
        CLEAR_BIT(((USART_TypeDef *)inst)->CR1, USART_CR1_RXDMAE);
    else
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
    /* LPUART DMA_EN 共享，仅在 RX 也停时由 _uart_dma_rx_stop 清除 */

    if (c->tx_dma_instance != UART_DMA_NONE)
        NVIC_DisableIRQ(c->tx_dma_irq);

    if (uart->dma_tx.Instance)
    {
        HAL_DMA_Abort(&uart->dma_tx);
        HAL_DMA_DeInit(&uart->dma_tx);
        uart->dma_tx.Instance = NULL;
    }
    uart->dma_tx_ready = RT_FALSE;
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

    /* 关闭逐字节 RXI，改用 IDLE 尾处理 */
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

    /* 仅做硬件参数初始化；DMA 在 control(CONFIG) 且 serial_rx 就绪后启动 */
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
        HAL_UART_Init(&uart->handle.usart);
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
        HAL_LPUART_Init(&uart->handle.lpuart);
    }

    if (type == UART_TYPE_USART)
    {
        MODIFY_REG(((USART_TypeDef *)inst)->CR3,
                   USART_CR3_RXIFLSEL_Msk | USART_CR3_TXIFLSEL_Msk,
                   USART_RX_FIFO_1_16 | USART_TX_FIFO_1_16);
    }

    /* 默认 INT RX；DMA 路径在 CONFIG 时关闭 RXI、打开 IDLE */
    uart->int_mask = _BIT(type, U_IE_RXI, L_IE_RXI);
    uart_reg_ie_set(inst, type,
        uart_reg_ie(inst, type) | _BIT(type, U_IE_RXI, L_IE_RXI));

    NVIC_SetPriority(c->irq_type, 2);
    NVIC_EnableIRQ(c->irq_type);

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

    /* V2: BLOCKING/NON_BLOCKING → DMA 或 INT（按注册能力） */
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
        NVIC_SetPriority(c->irq_type, 2);
        NVIC_EnableIRQ(c->irq_type);
        if (ctrl_arg == RT_DEVICE_FLAG_INT_RX)
        {
            uart_reg_ie_set(inst, type,
                uart_reg_ie(inst, type) | _BIT(type, U_IE_RXI, L_IE_RXI));
            uart->int_mask |= _BIT(type, U_IE_RXI, L_IE_RXI);
        }
        else if (ctrl_arg == RT_DEVICE_FLAG_INT_TX)
        {
            uart_reg_ie_set(inst, type,
                uart_reg_ie(inst, type)
                | _BIT(type, U_IE_TXI, L_IE_TXI)
                | _BIT(type, U_IE_TCI, L_IE_TCI));
            uart->int_mask |= _BIT(type, U_IE_TXI, L_IE_TXI)
                           |  _BIT(type, U_IE_TCI, L_IE_TCI);
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

    rt_uint32_t txff = _BIT(type, U_FR_TXFF, L_FR_TXFF);
    while (uart_reg_fr(inst, type) & txff);
    uart_reg_dr_write(inst, type, (rt_uint8_t)ch);
    return 1;
}

static int _uart_getc(struct rt_serial_device *serial)
{
    struct acm32_uart *uart = raw_to_uart(serial);
    void *inst = uart->config->Instance;
    int type = uart->config->uart_type;

    rt_uint32_t rxfe = _BIT(type, U_FR_RXFE, L_FR_RXFE);
    if (!(uart_reg_fr(inst, type) & rxfe))
        return uart_reg_dr_read(inst, type);
    return -RT_EEMPTY;
}

/* ==================== OPS: transmit（中断 + DMA） ==================== */

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
    /* DMA TX：能力由 uart_dma_flag 决定，NVIC 在 CONFIG 时已使能 */
    if ((uart->uart_dma_flag & RT_DEVICE_FLAG_DMA_TX) &&
        c->tx_dma_instance != UART_DMA_NONE)
    {
        if (_uart_dma_tx_prepare(uart) != RT_EOK)
            return 0;

        if (type == UART_TYPE_USART)
            SET_BIT(((USART_TypeDef *)inst)->CR1, USART_CR1_TXDMAE);
        else
            SET_BIT(((LPUART_TypeDef *)inst)->CR, LPUART_CR_DMA_EN);

        if (HAL_DMA_Start_IT(&uart->dma_tx,
                (rt_uint32_t)buf,
                (rt_uint32_t)(type == UART_TYPE_USART ?
                    &((USART_TypeDef *)inst)->DR :
                    &((LPUART_TypeDef *)inst)->TXDR),
                size) != HAL_OK)
        {
            return 0;
        }

        return size;
    }
#endif

    /* 中断 TX 模式 */
    uart->tx_buf  = buf;
    uart->tx_size = size;
    uart->tx_pos  = 0;
    uart->tx_done = RT_FALSE;

    /* 预填充 FIFO */
    rt_uint32_t txff = _BIT(type, U_FR_TXFF, L_FR_TXFF);
    while (uart->tx_pos < uart->tx_size)
    {
        if (uart_reg_fr(inst, type) & txff) break;
        uart_reg_dr_write(inst, type, buf[uart->tx_pos++]);
    }

    /* 使能 TXI + TCI */
    uart->int_mask |= _BIT(type, U_IE_TXI, L_IE_TXI)
                   |  _BIT(type, U_IE_TCI, L_IE_TCI);
    uart_reg_ie_set(inst, type,
        uart_reg_ie(inst, type)
        | _BIT(type, U_IE_TXI, L_IE_TXI)
        | _BIT(type, U_IE_TCI, L_IE_TCI));

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

/* ==================== 统一 ISR ==================== */

static void uart_isr(struct acm32_uart *uart)
{
    void *inst = uart->config->Instance;
    int type = uart->config->uart_type;

    rt_uint32_t isr = uart_reg_isr(inst, type);
    rt_uint32_t ie  = uart_reg_ie(inst, type);

    /* ---- 清除错误标志（溢出/帧/奇偶 等），防止持续中断 ---- */
    rt_uint32_t err_mask = _BIT(type, U_ERR_MASK, L_ERR_MASK);
    if (isr & err_mask)
    {
        uart_reg_isr_clear(inst, type, isr & err_mask);
    }

    /* ---- RXI: 将硬件 FIFO 数据喂入 V2 ringbuffer ---- */
    if ((ie & _BIT(type, U_IE_RXI, L_IE_RXI)) &&
        (isr & _BIT(type, U_ISR_RXI, L_ISR_RXI)))
    {
        uart_reg_isr_clear(inst, type, _BIT(type, U_ISR_RXI, L_ISR_RXI));

        rt_uint32_t rxfe = _BIT(type, U_FR_RXFE, L_FR_RXFE);
        while (!(uart_reg_fr(inst, type) & rxfe))
        {
            rt_uint8_t ch = uart_reg_dr_read(inst, type);
            rt_hw_serial_control_isr(&uart->serial, RT_HW_SERIAL_CTRL_PUTC, &ch);
        }
        rt_hw_serial_isr(&uart->serial, RT_SERIAL_EVENT_RX_IND);
    }

    /* ---- TXI: 填充 TX FIFO ---- */
    if ((ie & _BIT(type, U_IE_TXI, L_IE_TXI)) &&
        (isr & _BIT(type, U_ISR_TXI, L_ISR_TXI)))
    {
        uart_reg_isr_clear(inst, type, _BIT(type, U_ISR_TXI, L_ISR_TXI));

        rt_uint32_t txff = _BIT(type, U_FR_TXFF, L_FR_TXFF);
        while (uart->tx_buf && uart->tx_pos < uart->tx_size)
        {
            if (uart_reg_fr(inst, type) & txff) break;
            uart_reg_dr_write(inst, type, uart->tx_buf[uart->tx_pos++]);
        }

        /* 全部填充完毕：禁用 TXI */
        if (uart->tx_pos >= uart->tx_size)
        {
            uart_reg_ie_set(inst, type,
                ie & ~_BIT(type, U_IE_TXI, L_IE_TXI));
            uart->int_mask &= ~_BIT(type, U_IE_TXI, L_IE_TXI);
        }
    }

    /* ---- TCI: 发送完成 ---- */
    if ((ie & _BIT(type, U_IE_TCI, L_IE_TCI)) &&
        (isr & _BIT(type, U_ISR_TCI, L_ISR_TCI)))
    {
        uart_reg_isr_clear(inst, type, _BIT(type, U_ISR_TCI, L_ISR_TCI));
        uart_reg_ie_set(inst, type,
            ie & ~_BIT(type, U_IE_TCI, L_IE_TCI));
        uart->int_mask &= ~_BIT(type, U_IE_TCI, L_IE_TCI);

        uart->tx_buf = NULL;
        uart->tx_done = RT_TRUE;
        rt_hw_serial_isr(&uart->serial, RT_SERIAL_EVENT_TX_DONE);
    }

    /* ---- IDLEI: DMA RX 帧尾处理 ---- */
    if ((ie & _BIT(type, U_IE_IDLEI, L_IE_IDLEI)) &&
        (isr & _BIT(type, U_ISR_IDLEI, L_ISR_IDLEI)))
    {
        uart_reg_isr_clear(inst, type, _BIT(type, U_ISR_IDLEI, L_ISR_IDLEI));

#ifdef HAL_DMA_MODULE_ENABLED
        /* circular DMA 运行中：只按 HW 当前位置上报 tail，禁止 CPU 写 ping buffer */
        if (uart->dma_rx.Instance && uart->rx_dma_bufsz)
        {
            rt_uint16_t cur_pos = uart->rx_dma_bufsz -
                (rt_uint16_t)__HAL_DMA_GET_TRANSFER_SIZE(&uart->dma_rx);

            __DSB();

            if (cur_pos != uart->rx_dma_last_pos)
            {
                rt_uint16_t tail;
                if (cur_pos > uart->rx_dma_last_pos)
                    tail = cur_pos - uart->rx_dma_last_pos;
                else
                    tail = (uart->rx_dma_bufsz - uart->rx_dma_last_pos) + cur_pos;

                rt_hw_serial_isr(&uart->serial,
                    RT_SERIAL_EVENT_RX_DMADONE | ((rt_uint32_t)tail << 8));
                uart->rx_dma_last_pos = cur_pos;
            }
        }
#endif
    }
}

/* ==================== DMA RX 回调 ==================== */

#ifdef HAL_DMA_MODULE_ENABLED
static void _dma_rx_half_cplt(DMA_HandleTypeDef *hdma)
{
    struct acm32_uart *uart = (struct acm32_uart *)hdma->Parent;

    rt_uint16_t cur_pos = uart->rx_dma_bufsz -
        (rt_uint16_t)__HAL_DMA_GET_TRANSFER_SIZE(&uart->dma_rx);

    __DSB();

    if (cur_pos != uart->rx_dma_last_pos)
    {
        rt_uint16_t tail;
        if (cur_pos > uart->rx_dma_last_pos)
            tail = cur_pos - uart->rx_dma_last_pos;
        else
            tail = (uart->rx_dma_bufsz - uart->rx_dma_last_pos) + cur_pos;

        rt_hw_serial_isr(&uart->serial,
            RT_SERIAL_EVENT_RX_DMADONE | ((rt_uint32_t)tail << 8));
        uart->rx_dma_last_pos = cur_pos;
    }
}

static void _dma_rx_cplt(DMA_HandleTypeDef *hdma)
{
    struct acm32_uart *uart = (struct acm32_uart *)hdma->Parent;

    rt_uint16_t cur_pos = uart->rx_dma_bufsz -
        (rt_uint16_t)__HAL_DMA_GET_TRANSFER_SIZE(&uart->dma_rx);

    __DSB();

    if (cur_pos != uart->rx_dma_last_pos)
    {
        rt_uint16_t tail;
        if (cur_pos > uart->rx_dma_last_pos)
            tail = cur_pos - uart->rx_dma_last_pos;
        else
            tail = (uart->rx_dma_bufsz - uart->rx_dma_last_pos) + cur_pos;

        rt_hw_serial_isr(&uart->serial,
            RT_SERIAL_EVENT_RX_DMADONE | ((rt_uint32_t)tail << 8));
        uart->rx_dma_last_pos = cur_pos;
    }
}

static void _dma_rx_err(DMA_HandleTypeDef *hdma)
{
    struct acm32_uart *uart = (struct acm32_uart *)hdma->Parent;
    void *inst = uart->config->Instance;
    int type = uart->config->uart_type;
    rt_uint32_t src;

    if (uart->rx_dma_ping_buf == NULL || uart->rx_dma_bufsz == 0)
        return;

    /* 清 UART 错误标志，abort 后重启 circular RX */
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
            (rt_uint32_t)uart->rx_dma_ping_buf,
            uart->rx_dma_bufsz) == HAL_OK)
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
    /* LPUART: DMA_EN 是共享位（TX+RX），TX 完成时不关闭 */

    rt_hw_serial_isr(&uart->serial, RT_SERIAL_EVENT_TX_DMADONE);
}
#endif

/* ==================== ISR 入口 ==================== */

#define UART_IRQ_HANDLER(irq_name, uart_obj_ptr)     \
    void irq_name##_IRQHandler(void)                 \
    {                                                \
        rt_interrupt_enter();                        \
        uart_isr(uart_obj_ptr);                      \
        rt_interrupt_leave();                        \
    }

/* ==================== 设备索引枚举 ==================== */

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
#ifdef BSP_USING_UART5
    UART5_INDEX,
#endif
#ifdef BSP_USING_UART6
    UART6_INDEX,
#endif
#ifdef BSP_USING_UART7
    UART7_INDEX,
#endif
#ifdef BSP_USING_UART8
    UART8_INDEX,
#endif
#ifdef BSP_USING_LPUART1
    LPUART1_INDEX,
#endif
#ifdef BSP_USING_LPUART2
    LPUART2_INDEX,
#endif
    UART_MAX_INDEX,
};

/* ==================== 配置表 ==================== */

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
#ifdef BSP_USING_UART5
    UART5_CONFIG,
#endif
#ifdef BSP_USING_UART6
    UART6_CONFIG,
#endif
#ifdef BSP_USING_UART7
    UART7_CONFIG,
#endif
#ifdef BSP_USING_UART8
    UART8_CONFIG,
#endif
#ifdef BSP_USING_LPUART1
    LPUART1_CONFIG,
#endif
#ifdef BSP_USING_LPUART2
    LPUART2_CONFIG,
#endif
};

static struct acm32_uart uart_obj[UART_MAX_INDEX] = {0};

/* ==================== 初始化 ==================== */

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
            uart_obj[i].serial.config.rx_bufsz = 1024;
            uart_obj[i].serial.config.dma_ping_bufsz = 512;
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

/* ==================== ISR 实例化 ==================== */

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
#ifdef BSP_USING_UART5
UART_IRQ_HANDLER(USART5, &uart_obj[UART5_INDEX])
#endif
#ifdef BSP_USING_UART6
UART_IRQ_HANDLER(USART6, &uart_obj[UART6_INDEX])
#endif
#ifdef BSP_USING_UART7
UART_IRQ_HANDLER(USART7, &uart_obj[UART7_INDEX])
#endif
#ifdef BSP_USING_UART8
UART_IRQ_HANDLER(USART8, &uart_obj[UART8_INDEX])
#endif
#ifdef BSP_USING_LPUART1
UART_IRQ_HANDLER(LPUART1, &uart_obj[LPUART1_INDEX])
#endif
#ifdef BSP_USING_LPUART2
UART_IRQ_HANDLER(LPUART2, &uart_obj[LPUART2_INDEX])
#endif

/* ==================== DMA RX IRQ 实例化 ==================== */

#ifdef HAL_DMA_MODULE_ENABLED
#define DMA_RX_IRQ_HANDLER(irq_name)                   \
    void irq_name##_IRQHandler(void)                    \
    {                                                   \
        rt_interrupt_enter();                           \
        for (int _i = 0; _i < UART_MAX_INDEX; _i++)  \
        {                                               \
            if (uart_obj[_i].dma_rx.DMA &&             \
                uart_obj[_i].dma_rx.DMA->INTSTATUS &   \
                (1UL << uart_obj[_i].dma_rx.Channel))  \
            {                                           \
                HAL_DMA_IRQHandler(&uart_obj[_i].dma_rx);\
            }                                           \
        }                                               \
        rt_interrupt_leave();                           \
    }

#ifdef BSP_USING_UART1_DMA
DMA_RX_IRQ_HANDLER(DMA1_CH2)
#endif
#ifdef BSP_USING_UART2_DMA
DMA_RX_IRQ_HANDLER(DMA2_CH0)
#endif
#ifdef BSP_USING_UART3_DMA
DMA_RX_IRQ_HANDLER(DMA2_CH3)
#endif
#ifdef BSP_USING_UART4_DMA
DMA_RX_IRQ_HANDLER(DMA1_CH0)
#endif

#define DMA_TX_IRQ_HANDLER(irq_name)                   \
    void irq_name##_IRQHandler(void)                    \
    {                                                   \
        rt_interrupt_enter();                           \
        for (int _i = 0; _i < UART_MAX_INDEX; _i++)  \
        {                                               \
            if (uart_obj[_i].dma_tx.DMA &&             \
                uart_obj[_i].dma_tx.DMA->INTSTATUS &   \
                (1UL << uart_obj[_i].dma_tx.Channel))  \
            {                                           \
                HAL_DMA_IRQHandler(&uart_obj[_i].dma_tx);\
            }                                           \
        }                                               \
        rt_interrupt_leave();                           \
    }

#ifdef BSP_USING_UART1_DMA
DMA_TX_IRQ_HANDLER(DMA1_CH1)
#endif
#ifdef BSP_USING_UART2_DMA
DMA_TX_IRQ_HANDLER(DMA1_CH3)
#endif
#ifdef BSP_USING_UART3_DMA
DMA_TX_IRQ_HANDLER(DMA2_CH2)
#endif
#ifdef BSP_USING_UART4_DMA
DMA_TX_IRQ_HANDLER(DMA2_CH1)
#endif
#endif /* HAL_DMA_MODULE_ENABLED */

#endif /* RT_USING_SERIAL_V2 */
