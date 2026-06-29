/**
 * @file i2s.c
 * @brief ACM32P4 I2S 椹卞姩瀹炵幇
 *
 * I2S锛圛nter-IC Sound锛夌敤浜庢暟瀛楅煶棰戣澶囬棿鐨勪覆琛屾暟鎹€氫俊銆? * P4 鏀寔 I2S1 鍜?I2S2 涓や釜瀹炰緥銆? */

#include "../include/acm32p4.h"

//=============================================================================
// 鍐呴儴杈呭姪
//=============================================================================

static I2S_TypeDef *const i2s_instances[] = {
    I2S1,  // I2S1
    I2S2,  // I2S2
};

#define I2S_INSTANCE_COUNT  (sizeof(i2s_instances) / sizeof(i2s_instances[0]))

static inline I2S_TypeDef *get_i2s_regs(i2s_instance_t i2s)
{
    if (i2s >= I2S_INSTANCE_COUNT) return NULL;
    return i2s_instances[i2s];
}

//=============================================================================
// API 瀹炵幇
//=============================================================================

bool i2s_init(i2s_instance_t i2s, const i2s_config_t *config)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL || config == NULL) return false;

    /* 鍏堢鐢?I2S */
    regs->CTL &= ~(1U << 0);

    /* 閰嶇疆宸ヤ綔妯″紡 */
    uint32_t ctl = regs->CTL;
    ctl &= ~((1U << 0) | (1U << 1) | (3U << 2) | (3U << 4));

    if (config->mode == I2S_MODE_MASTER) {
        ctl |= (1U << 1);  /* MST_MODE */
    }
    ctl |= (config->standard & 0x03U) << 2;
    ctl |= (config->data_width & 0x03U) << 4;
    regs->CTL = ctl;

    /* 閰嶇疆鏃堕挓鍒嗛锛堢畝鍖栵細浣跨敤榛樿鍒嗛锛?/
    if (config->sample_rate > 0) {
        uint32_t div = 24000000U / (config->sample_rate * 32U * 2U);
        if (div > 0xFF) div = 0xFF;
        if (div < 1) div = 1;
        regs->BAUD = div;
    }

    /* 閰嶇疆鍙戦€?鎺ユ敹 */
    uint32_t tx_ctl = regs->TX_CTL;
    uint32_t rx_ctl = regs->RX_CTL;
    if (config->tx_enable) {
        tx_ctl |= (1U << 0);  /* TX_EN */
    } else {
        tx_ctl &= ~(1U << 0);
    }
    if (config->rx_enable) {
        rx_ctl |= (1U << 0);  /* RX_EN */
    } else {
        rx_ctl &= ~(1U << 0);
    }
    regs->TX_CTL = tx_ctl;
    regs->RX_CTL = rx_ctl;

    return true;
}

void i2s_enable(i2s_instance_t i2s)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL) return;
    regs->CTL |= (1U << 0);
}

void i2s_disable(i2s_instance_t i2s)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL) return;
    regs->CTL &= ~(1U << 0);
}

bool i2s_send(i2s_instance_t i2s, uint32_t data)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL) return false;

    /* 绛夊緟TX FIFO涓嶆弧 */
    uint32_t timeout = 1000000U;
    while ((regs->STATUS & (1U << 3)) && timeout) {
        timeout--;
    }
    if (timeout == 0) return false;

    regs->DAT = data;
    return true;
}

bool i2s_receive(i2s_instance_t i2s, uint32_t *data)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL || data == NULL) return false;

    /* 绛夊緟RX FIFO涓嶇┖ */
    uint32_t timeout = 1000000U;
    while ((regs->STATUS & (1U << 4)) && timeout) {
        timeout--;
    }
    if (timeout == 0) return false;

    *data = regs->DAT;
    return true;
}

uint32_t i2s_get_tx_fifo_level(i2s_instance_t i2s)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL) return 0;

    if (regs->STATUS & (1U << 2)) return 0;  /* TX_FIFO_EMPTY */
    if (regs->STATUS & (1U << 3)) return 4;  /* TX_FIFO_FULL */
    return 2;  /* 鍗婃弧 */
}

uint32_t i2s_get_rx_fifo_level(i2s_instance_t i2s)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL) return 0;

    if (regs->STATUS & (1U << 4)) return 0;  /* RX_FIFO_EMPTY */
    if (regs->STATUS & (1U << 5)) return 4;  /* RX_FIFO_FULL */
    return 2;  /* 鍗婃弧 */
}

bool i2s_is_busy(i2s_instance_t i2s)
{
    I2S_TypeDef *regs = get_i2s_regs(i2s);
    if (regs == NULL) return false;
    return (regs->STATUS & (1U << 0)) ? true : false;
}
