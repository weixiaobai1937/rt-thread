/**
 * @file lpuart.c
 * @brief ACM32P4 LPUART 低功耗串口驱动实现
 * @author ACM32P4 SDK Team
 * @version 1.1
 */

#include "hardware/lpuart.h"
#include "hardware/clocks.h"
#include "system.h"
#include "device/acm32p4xx.h"
#include <assert.h>

//===========================================
// 内部宏定义
//===========================================

#define LPUART_TIMEOUT_MS         10U         ///< 超时毫秒数
#define LPUART_SR_CLEAR_MASK      0x0003FFCFU ///< SR 可清除标志位掩码（bit0-5,8-9,16-17）

//===========================================
// 内部状态变量
//===========================================

static LPUART_TypeDef * const lpuart_instances[LPUART_COUNT] = {
    (LPUART_TypeDef *)LPUART1_BASE,
};

static const clock_periph_t lpuart_clock_periph[LPUART_COUNT] = {
    CLK_LPUART1,
};

static lpuart_callback_t lpuart_callbacks[LPUART_COUNT] = { NULL };

//===========================================
// 内部辅助函数
//===========================================

/**
 * @brief 获取 LPUART 寄存器指针
 */
static inline LPUART_TypeDef* lpuart_get_instance(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);
    return lpuart_instances[lpuart];
}

/**
 * @brief 根据校验模式计算每帧比特数
 */
static uint8_t lpuart_get_bits_per_frame(lpuart_instance_t lpuart)
{
    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    uint32_t lcr = lp->LCR;

    uint8_t bits = 1U;  // Start 位

    if (lcr & (1U << 4)) {  // WLEN=1: 7位
        bits += 7U;
    } else {
        bits += 8U;
    }

    if (lcr & (1U << 0)) {  // PEN: 校验使能
        bits += 1U;
    }

    if (lcr & (1U << 3)) {  // STP2=1: 2位停止位
        bits += 2U;
    } else {
        bits += 1U;
    }

    return bits;
}

/**
 * @brief 计算并写入波特率参数
 */
static void lpuart_apply_baudrate(lpuart_instance_t lpuart, uint32_t baud,
                                  uint32_t clk_hz, uint8_t bits_per_frame)
{
    uint8_t ibaud;
    uint16_t fbaud;
    uint8_t rxsam;

    lpuart_baudrate_calc(baud, clk_hz, bits_per_frame, &ibaud, &fbaud, &rxsam);
    lpuart_set_baudrate(lpuart, ibaud, fbaud, rxsam);
}

/**
 * @brief 配置 LCR 参数（数据字长/停止位/校验/极性）
 *
 * @note 不设置 WKCK（唤醒校验检查），由调用方通过 lpuart_set_wake_check() 控制
 */
static void lpuart_config_lcr(lpuart_instance_t lpuart, lpuart_wlen_t wlen,
                              lpuart_parity_t parity, lpuart_stop_t stop)
{
    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);

    lp->LCR_f.WLEN = (uint32_t)wlen;
    lp->LCR_f.STP2 = (uint32_t)stop;

    switch (parity) {
        case LPUART_PARITY_NONE:
            lp->LCR_f.PEN = 0U;
            break;
        case LPUART_PARITY_ODD:
            lp->LCR_f.PEN = 1U;
            lp->LCR_f.SPS = 0U;
            lp->LCR_f.EPS = 0U;
            break;
        case LPUART_PARITY_EVEN:
            lp->LCR_f.PEN = 1U;
            lp->LCR_f.SPS = 0U;
            lp->LCR_f.EPS = 1U;
            break;
        case LPUART_PARITY_0:
            lp->LCR_f.PEN = 1U;
            lp->LCR_f.SPS = 1U;
            lp->LCR_f.EPS = 1U;
            break;
        case LPUART_PARITY_1:
            lp->LCR_f.PEN = 1U;
            lp->LCR_f.SPS = 1U;
            lp->LCR_f.EPS = 0U;
            break;
    }
}

//===========================================
// 第1层：快速初始化 API
//===========================================

/**
 * @brief 快速初始化 LPUART 为 RCL 32.768KHz 时钟源的 9600 波特率通信
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] wlen   数据字长（7/8 位）
 * @param[in] parity 校验模式（无/奇/偶/0/1）
 * @param[in] stop   停止位（1/2 位）
 *
 * @note 时钟固定为 RCL 32.768KHz，IBAUD=2(3分频)，FBAUD=0x952
 */
void lpuart_init_9600(lpuart_instance_t lpuart, lpuart_wlen_t wlen,
                      lpuart_parity_t parity, lpuart_stop_t stop)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);

    lpuart_enable_clock(lpuart);

    lp->CR_f.TX_EN = 0U;
    lp->CR_f.RX_EN = 0U;

    lpuart_config_lcr(lpuart, wlen, parity, stop);
    lpuart_set_wake_check(lpuart, true);  // 默认检查校验位和 STOP 位

    // 32.768KHz → 9600 baud: ibaud=2 (3分频), fbaud=0x952
    lp->IBAUD_f.IBAUD = 2U;
    lp->IBAUD_f.RXSAM = 1U;
    lp->FBAUD_f.FBAUD = 0x952U;

    lp->IE = 0U;
    lp->SR = LPUART_SR_CLEAR_MASK;

    lp->CR_f.TX_EN = 1U;
    lp->CR_f.RX_EN = 1U;
}

/**
 * @brief 使用 PCLK 分频时钟初始化 LPUART，支持高速波特率
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] baud   目标波特率（如 115200）
 * @param[in] clk_hz LPUART 时钟源频率（Hz），需预先通过 LPUARTDIV 分频
 * @param[in] wlen   数据字长（7/8 位）
 * @param[in] parity 校验模式（无/奇/偶/0/1）
 * @param[in] stop   停止位（1/2 位）
 *
 * @note clk_hz 为 LPUARTDIV 分频后的频率，非原始 PCLK
 */
void lpuart_init_pclk(lpuart_instance_t lpuart, uint32_t baud, uint32_t clk_hz,
                      lpuart_wlen_t wlen, lpuart_parity_t parity, lpuart_stop_t stop)
{
    assert(lpuart < LPUART_COUNT);
    assert(baud > 0U);
    assert(clk_hz > 0U);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);

    lpuart_enable_clock(lpuart);

    lp->CR_f.TX_EN = 0U;
    lp->CR_f.RX_EN = 0U;

    lpuart_config_lcr(lpuart, wlen, parity, stop);
    lpuart_set_wake_check(lpuart, true);  // 默认检查校验位和 STOP 位

    uint8_t bits_per_frame = lpuart_get_bits_per_frame(lpuart);
    lpuart_apply_baudrate(lpuart, baud, clk_hz, bits_per_frame);

    lp->IE = 0U;
    lp->SR = LPUART_SR_CLEAR_MASK;

    lp->CR_f.TX_EN = 1U;
    lp->CR_f.RX_EN = 1U;
}

/**
 * @brief 初始化 LPUART 为 STOP 模式唤醒接收
 *
 * 配置 LPUART 在 STOP 模式下检测 START 位、接收完成或地址匹配时唤醒系统。
 *
 * @param[in] lpuart    LPUART 实例
 * @param[in] wake_mode 唤醒方式（START位/1字节完成/地址匹配/无唤醒）
 * @param[in] addr      地址匹配值（仅 wake_mode=LPUART_WAKE_MATCH 时有效）
 * @param[in] wlen      数据字长（7/8 位）
 * @param[in] parity    校验模式（无/奇/偶/0/1）
 * @param[in] stop      停止位（1/2 位）
 *
 */
void lpuart_init_wakeup(lpuart_instance_t lpuart, lpuart_wake_mode_t wake_mode, uint8_t addr,
                        lpuart_wlen_t wlen, lpuart_parity_t parity, lpuart_stop_t stop)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);

    lpuart_enable_clock(lpuart);

    lp->CR_f.TX_EN = 0U;
    lp->CR_f.RX_EN = 0U;

    lpuart_config_lcr(lpuart, wlen, parity, stop);
    lpuart_set_wake_mode(lpuart, wake_mode);
    lpuart_set_addr(lpuart, addr);

    lp->IE = 0U;
    lp->SR = LPUART_SR_CLEAR_MASK;

    lp->CR_f.TX_EN = 0U;
    lp->CR_f.RX_EN = 1U;
}

//===========================================
// 第2层：基础配置 API
//===========================================

bool lpuart_config(lpuart_instance_t lpuart, const lpuart_config_t *config)
{
    assert(config != NULL);
    if (lpuart >= LPUART_COUNT) return false;
    if (config == NULL) return false;

    if (config->wlen > LPUART_WLEN_7) return false;
    if (config->stop > LPUART_STOP_2) return false;
    if (config->parity > LPUART_PARITY_1) return false;
    if (config->wake_mode > LPUART_WAKE_NONE) return false;
    if (config->bcnt_value > LPUART_BCNT_VALUE_MAX) return false;
    if (config->addr > LPUART_ADDR_MAX) return false;

    lpuart_config_lcr(lpuart, config->wlen, config->parity, config->stop);
    lpuart_set_tx_polarity(lpuart, config->txpol);
    lpuart_set_rx_polarity(lpuart, config->rxpol);
    lpuart_set_wake_mode(lpuart, config->wake_mode);
    lpuart_set_wake_check(lpuart, config->wake_check);
    lpuart_set_auto_start(lpuart, config->auto_start);
    lpuart_set_bcnt_value(lpuart, config->bcnt_value);
    lpuart_set_addr(lpuart, config->addr);

    return true;
}

//===========================================
// 第3层：高级功能 API
//===========================================

// --- 数据收发 ---

void lpuart_send_byte(lpuart_instance_t lpuart, uint8_t data)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);

    uint64_t start = system_get_tick();
    while (!lp->SR_f.TXEIF && !system_elapsed(start, LPUART_TIMEOUT_MS)) {
    }

    lp->TXDR = data;
}

uint8_t lpuart_read_byte(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);

    return (uint8_t)(lp->RXDR & 0xFFU);
}

void lpuart_send_buffer(lpuart_instance_t lpuart, const uint8_t *data, size_t len)
{
    assert(lpuart < LPUART_COUNT);
    assert(data != NULL);

    for (size_t i = 0U; i < len; i++) {
        lpuart_send_byte(lpuart, data[i]);
    }
}

size_t lpuart_read_buffer(lpuart_instance_t lpuart, uint8_t *data, size_t len, uint32_t timeout)
{
    assert(lpuart < LPUART_COUNT);
    assert(data != NULL);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    size_t count = 0U;
    uint64_t start = system_get_tick();

    while (count < len) {
        if (lp->SR_f.RXF) {
            data[count++] = (uint8_t)(lp->RXDR & 0xFFU);
        } else {
            if (timeout > 0U && system_elapsed(start, timeout)) {
                break;
            }
        }
    }

    return count;
}

void lpuart_send_string(lpuart_instance_t lpuart, const char *str)
{
    assert(lpuart < LPUART_COUNT);
    assert(str != NULL);

    while (*str != '\0') {
        lpuart_send_byte(lpuart, (uint8_t)*str);
        str++;
    }
}

bool lpuart_flush_tx(lpuart_instance_t lpuart, uint32_t timeout)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    uint64_t start = system_get_tick();

    while (!lp->SR_f.TCIF) {
        if (timeout > 0U && system_elapsed(start, timeout)) {
            return false;
        }
    }

    return true;
}

// --- 波特率 ---

void lpuart_set_baudrate(lpuart_instance_t lpuart, uint8_t ibaud, uint16_t fbaud, uint8_t rxsam)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);

    lp->IBAUD_f.IBAUD = ibaud;
    lp->IBAUD_f.RXSAM = rxsam;
    lp->FBAUD_f.FBAUD = (fbaud & LPUART_FBAUD_MASK);
}

void lpuart_calculate_baudrate(lpuart_instance_t lpuart, uint32_t baud, uint32_t clk_hz,
                               uint8_t bits_per_frame)
{
    assert(lpuart < LPUART_COUNT);

    uint8_t ibaud;
    uint16_t fbaud;
    uint8_t rxsam;

    lpuart_baudrate_calc(baud, clk_hz, bits_per_frame, &ibaud, &fbaud, &rxsam);
    lpuart_set_baudrate(lpuart, ibaud, fbaud, rxsam);
}

// --- DMA ---

void lpuart_dma_enable(lpuart_instance_t lpuart, bool enable)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    lp->CR_f.DMA_EN = enable ? 1U : 0U;
}

bool lpuart_dma_is_enabled(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    return (lp->CR_f.DMA_EN != 0U);
}

uint32_t lpuart_get_rxdr_addr(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    return (uint32_t)&lp->RXDR;
}

uint32_t lpuart_get_txdr_addr(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    return (uint32_t)&lp->TXDR;
}

//===========================================
// 第4层：控制与查询 API
//===========================================

// --- CR 控制寄存器 ---

void lpuart_tx_enable(lpuart_instance_t lpuart, bool enable)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    lp->CR_f.TX_EN = enable ? 1U : 0U;
}

void lpuart_rx_enable(lpuart_instance_t lpuart, bool enable)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    lp->CR_f.RX_EN = enable ? 1U : 0U;
}

bool lpuart_tx_is_enabled(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    return (lp->CR_f.TX_EN != 0U);
}

bool lpuart_rx_is_enabled(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    return (lp->CR_f.RX_EN != 0U);
}

// --- LCR 线控寄存器 ---

void lpuart_set_wlen(lpuart_instance_t lpuart, lpuart_wlen_t wlen)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    lp->LCR_f.WLEN = (uint32_t)wlen;
}

lpuart_wlen_t lpuart_get_wlen(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    return (lpuart_wlen_t)lp->LCR_f.WLEN;
}

void lpuart_set_stop(lpuart_instance_t lpuart, lpuart_stop_t stop)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    lp->LCR_f.STP2 = (uint32_t)stop;
}

lpuart_stop_t lpuart_get_stop(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    return (lpuart_stop_t)lp->LCR_f.STP2;
}

void lpuart_set_parity(lpuart_instance_t lpuart, lpuart_parity_t parity)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);

    switch (parity) {
        case LPUART_PARITY_NONE:
            lp->LCR_f.PEN = 0U;
            break;
        case LPUART_PARITY_ODD:
            lp->LCR_f.PEN = 1U;
            lp->LCR_f.SPS = 0U;
            lp->LCR_f.EPS = 0U;
            break;
        case LPUART_PARITY_EVEN:
            lp->LCR_f.PEN = 1U;
            lp->LCR_f.SPS = 0U;
            lp->LCR_f.EPS = 1U;
            break;
        case LPUART_PARITY_0:
            lp->LCR_f.PEN = 1U;
            lp->LCR_f.SPS = 1U;
            lp->LCR_f.EPS = 1U;
            break;
        case LPUART_PARITY_1:
            lp->LCR_f.PEN = 1U;
            lp->LCR_f.SPS = 1U;
            lp->LCR_f.EPS = 0U;
            break;
    }
}

lpuart_parity_t lpuart_get_parity(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);

    if (lp->LCR_f.PEN == 0U) {
        return LPUART_PARITY_NONE;
    }

    if (lp->LCR_f.SPS == 0U) {
        return (lp->LCR_f.EPS == 0U) ? LPUART_PARITY_ODD : LPUART_PARITY_EVEN;
    } else {
        return (lp->LCR_f.EPS == 0U) ? LPUART_PARITY_1 : LPUART_PARITY_0;
    }
}

void lpuart_set_tx_polarity(lpuart_instance_t lpuart, bool invert)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    lp->LCR_f.TXPOL = invert ? 1U : 0U;
}

bool lpuart_get_tx_polarity(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    return (lp->LCR_f.TXPOL != 0U);
}

void lpuart_set_rx_polarity(lpuart_instance_t lpuart, bool invert)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    lp->LCR_f.RXPOL = invert ? 1U : 0U;
}

bool lpuart_get_rx_polarity(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    return (lp->LCR_f.RXPOL != 0U);
}

void lpuart_set_wake_mode(lpuart_instance_t lpuart, lpuart_wake_mode_t mode)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    lp->LCR_f.RXWKS = (uint32_t)mode;
}

lpuart_wake_mode_t lpuart_get_wake_mode(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    return (lpuart_wake_mode_t)lp->LCR_f.RXWKS;
}

void lpuart_set_wake_check(lpuart_instance_t lpuart, bool check)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    lp->LCR_f.WKCK = check ? 1U : 0U;
}

bool lpuart_get_wake_check(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    return (lp->LCR_f.WKCK != 0U);
}

void lpuart_set_auto_start(lpuart_instance_t lpuart, bool enable)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    lp->LCR_f.AUTO_START_EN = enable ? 1U : 0U;
}

bool lpuart_get_auto_start(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    return (lp->LCR_f.AUTO_START_EN != 0U);
}

void lpuart_set_bcnt_value(lpuart_instance_t lpuart, uint16_t bcnt)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    lp->LCR_f.BCNT_VALUE = (uint32_t)bcnt;
}

uint16_t lpuart_get_bcnt_value(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    return (uint16_t)lp->LCR_f.BCNT_VALUE;
}

// --- IBAUD/FBAUD ---

void lpuart_set_ibaud(lpuart_instance_t lpuart, uint8_t ibaud)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    lp->IBAUD_f.IBAUD = ibaud;
}

uint8_t lpuart_get_ibaud(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    return (uint8_t)lp->IBAUD_f.IBAUD;
}

void lpuart_set_fbaud(lpuart_instance_t lpuart, uint16_t fbaud)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    lp->FBAUD_f.FBAUD = (fbaud & LPUART_FBAUD_MASK);
}

uint16_t lpuart_get_fbaud(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    return (uint16_t)lp->FBAUD_f.FBAUD;
}

void lpuart_set_rxsam(lpuart_instance_t lpuart, uint8_t rxsam)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    lp->IBAUD_f.RXSAM = rxsam;
}

uint8_t lpuart_get_rxsam(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    return (uint8_t)lp->IBAUD_f.RXSAM;
}

// --- ADDR ---

void lpuart_set_addr(lpuart_instance_t lpuart, uint8_t addr)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    lp->ADDR_f.ADDR = addr;
}

uint8_t lpuart_get_addr(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    return (uint8_t)lp->ADDR_f.ADDR;
}

// --- IE 中断使能 ---

void lpuart_irq_enable(lpuart_instance_t lpuart, lpuart_irq_t irq, bool enable)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);

    if (enable) {
        lp->IE |= (uint32_t)irq;
    } else {
        lp->IE &= ~((uint32_t)irq);
    }
}

bool lpuart_irq_is_enabled(lpuart_instance_t lpuart, lpuart_irq_t irq)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    return ((lp->IE & (uint32_t)irq) != 0U);
}

void lpuart_irq_disable_all(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    lp->IE = 0U;
}

// --- SR 状态查询与清除 ---

bool lpuart_get_flag(lpuart_instance_t lpuart, lpuart_flag_t flag)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    return ((lp->SR & (uint32_t)flag) != 0U);
}

void lpuart_clear_flag(lpuart_instance_t lpuart, lpuart_flag_t flag)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);

    if (flag == LPUART_FLAG_RXF) {
        (void)lp->RXDR;
        return;
    }
    if (flag == LPUART_FLAG_TXEIF) {
        return;
    }

    lp->SR = (uint32_t)flag;
}

uint32_t lpuart_get_status(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    return lp->SR;
}

bool lpuart_rx_available(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    return (lp->SR_f.RXF != 0U);
}

bool lpuart_tx_empty(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    return (lp->SR_f.TXEIF != 0U);
}

bool lpuart_tx_done(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    return (lp->SR_f.TCIF != 0U);
}

// --- 中断回调 ---

void lpuart_register_callback(lpuart_instance_t lpuart, lpuart_callback_t callback)
{
    assert(lpuart < LPUART_COUNT);

    lpuart_callbacks[lpuart] = callback;
}

// --- 时钟 ---

void lpuart_enable_clock(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    clock_periph_enable(lpuart_clock_periph[lpuart]);
}

void lpuart_disable_clock(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    clock_periph_disable(lpuart_clock_periph[lpuart]);
}

//===========================================
// 辅助计算函数
//===========================================

/**
 * @brief 计算 16 位整数中置位的位数（popcount）
 */
static uint8_t popcount16(uint16_t val)
{
    uint8_t count = 0U;
    while (val != 0U) {
        count += (uint8_t)(val & 1U);
        val >>= 1U;
    }
    return count;
}

void lpuart_baudrate_calc(uint32_t baud, uint32_t clk_hz, uint8_t bits_per_frame,
                          uint8_t *ibaud, uint16_t *fbaud, uint8_t *rxsam)
{
    if (baud == 0U || clk_hz == 0U || bits_per_frame == 0U) {
        *ibaud = 2U;
        *fbaud = 0U;
        *rxsam = 1U;
        return;
    }

    // target_div = clk_hz / baud
    // ibaud = floor(target_div) - 1, min = 2
    // fraction = target_div - (ibaud + 1)
    // fbaud_ones = round(fraction * bits_per_frame)
    // distribute evenly across bits_per_frame bits

    uint32_t target_mul = clk_hz * 1000U;  // 放大 1000 倍提高精度
    uint32_t div = baud;
    uint32_t target_div_int = clk_hz / baud;

    uint8_t ibaud_val = (uint8_t)(target_div_int - 1U);
    if (ibaud_val < LPUART_IBAUD_MIN) {
        ibaud_val = LPUART_IBAUD_MIN;
    }
    if (ibaud_val > LPUART_IBAUD_MAX) {
        ibaud_val = LPUART_IBAUD_MAX;
    }

    // fraction = target_div - (ibaud + 1)，放大 1000 倍
    uint32_t frac_mul = ((target_mul / baud) - ((uint32_t)(ibaud_val + 1U) * 1000U));

    // fbaud_ones = round(fraction * bits_per_frame)（含四舍五入）
    uint8_t fbaud_ones = (uint8_t)(((frac_mul * bits_per_frame) + 500U) / 1000U);
    if (fbaud_ones > bits_per_frame) {
        fbaud_ones = bits_per_frame;
    }

    // 均匀分布：使用 Bresenham 算法
    uint16_t fbaud_val = 0U;
    int16_t error = (int16_t)(0 - (int16_t)fbaud_ones);
    for (uint8_t i = 0U; i < bits_per_frame && i < 12U; i++) {
        error += (int16_t)fbaud_ones;
        if (error >= 0) {
            fbaud_val |= (uint16_t)(1U << i);
            error -= (int16_t)bits_per_frame;
        }
    }

    uint8_t rxsam_val = (uint8_t)(ibaud_val >> 1U);
    if (rxsam_val < 1U) {
        rxsam_val = 1U;
    }

    *ibaud = ibaud_val;
    *fbaud = fbaud_val;
    *rxsam = rxsam_val;
}

uint32_t lpuart_get_actual_baudrate(uint32_t clk_hz, uint8_t ibaud,
                                    uint16_t fbaud, uint8_t bits_per_frame)
{
    if (bits_per_frame == 0U) {
        return 0U;
    }

    uint8_t ones = popcount16(fbaud & ((uint16_t)(1U << bits_per_frame) - 1U));

    // actual_divisor = ibaud + 1 + ones / bits_per_frame
    // actual_baud = clk_hz / actual_divisor
    uint32_t divisor_mul = (uint32_t)(ibaud + 1U) * bits_per_frame + (uint32_t)ones;
    return (clk_hz * bits_per_frame) / divisor_mul;
}

//===========================================
// 中断处理函数
//===========================================

void lpuart_irq_handler(lpuart_instance_t lpuart)
{
    assert(lpuart < LPUART_COUNT);

    LPUART_TypeDef *lp = lpuart_get_instance(lpuart);
    uint32_t sr = lp->SR;
    uint32_t ie = lp->IE;

    if (lpuart_callbacks[lpuart] != NULL) {
        lpuart_callbacks[lpuart](lpuart, sr);
    }

    // 清除已处理的中断标志
    uint32_t clear_mask = sr & ie & LPUART_SR_CLEAR_MASK;
    if (clear_mask & (uint32_t)LPUART_FLAG_RXIF) {
        lp->SR = (uint32_t)LPUART_FLAG_RXIF;
    }
    if (clear_mask & (uint32_t)LPUART_FLAG_TCIF) {
        lp->SR = (uint32_t)LPUART_FLAG_TCIF;
    }
    if (clear_mask & (uint32_t)LPUART_FLAG_PEIF) {
        lp->SR = (uint32_t)LPUART_FLAG_PEIF;
    }
    if (clear_mask & (uint32_t)LPUART_FLAG_FEIF) {
        lp->SR = (uint32_t)LPUART_FLAG_FEIF;
    }
    if (clear_mask & (uint32_t)LPUART_FLAG_RXOVIF) {
        lp->SR = (uint32_t)LPUART_FLAG_RXOVIF;
    }
    if (clear_mask & (uint32_t)LPUART_FLAG_TXOVF) {
        lp->SR = (uint32_t)LPUART_FLAG_TXOVF;
    }
}

void LPUART1_IRQHandler(void)
{
    lpuart_irq_handler(LPUART_1);
}
