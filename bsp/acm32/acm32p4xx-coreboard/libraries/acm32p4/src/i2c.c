/**
 * @file i2c.c
 * @brief ACM32P4 I2C 驱动实现
 *
 * 实现了 I2C 驱动的全部功能，包括：
 * - 主/从模式初始化（4层API）
 * - 阻塞式轮询收发（master_transmit/receive/write_read）
 * - 统一中断回调框架（14路中断源）
 * - SCL/SDA 滤波、SDA 跳变检测
 * - SMBus 2.0 超时配置
 * - DMA 使能配置
 * - 从机拉时钟修正、多主机仲裁、开漏模式
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#include "hardware/i2c.h"
#include "hardware/clocks.h"
#include "hardware/system.h"
#include "device/acm32p4xx.h"
#include <assert.h>
#include <stddef.h>

//===========================================
// 内部宏定义
//===========================================

/** I2C_CR 寄存器各位掩码 */
#define CR_MEN_MSK              (1UL << 0)
#define CR_NOSTRETCH_MSK        (1UL << 1)
#define CR_MASTER_MSK           (1UL << 2)
#define CR_TX_MSK               (1UL << 3)
#define CR_START_MSK            (1UL << 4)
#define CR_STOP_MSK             (1UL << 5)
#define CR_TACK_MSK             (1UL << 6)
#define CR_MTF_INT_EN_MSK       (1UL << 7)
#define CR_RX_ADDR1_INT_EN_MSK  (1UL << 8)
#define CR_TXE_INT_EN_MSK       (1UL << 9)
#define CR_RXNE_INT_EN_MSK      (1UL << 10)
#define CR_OVR_INT_EN_MSK       (1UL << 11)
#define CR_RX_ADDR2_INT_EN_MSK  (1UL << 12)
#define CR_DETR_INT_EN_MSK      (1UL << 13)
#define CR_TX_AUTO_EN_MSK       (1UL << 15)
#define CR_MARLO_INTEN_MSK      (1UL << 16)
#define CR_TXE_SEL_MSK          (1UL << 17)
#define CR_DMA_EN_MSK           (1UL << 18)
#define CR_RX_ADDR3_INTEN_MSK   (1UL << 19)
#define CR_STOPF_INTEN_MSK      (1UL << 20)
#define CR_NACKF_INTEN_MSK      (1UL << 21)
#define CR_TX_RX_FLAG_INTEN_MSK (1UL << 22)
#define CR_MARLO_SEL_MSK        (1UL << 23)
#define CR_SU_HD_FIXEN_MSK      (1UL << 24)
#define CR_STRETCH_FIXEN_MSK    (1UL << 25)
#define CR_HIGH_WAITEN_MSK      (1UL << 26)
#define CR_MULTIMA_EN_MSK       (1UL << 27)

/** I2C_TIMEOUT 寄存器各位掩码 */
#define TIMEOUT_TIMEOUTA_POS    (0U)
#define TIMEOUT_TIMEOUTA_MSK    (0xFFFUL << TIMEOUT_TIMEOUTA_POS)
#define TIMEOUT_TOUTA_INTEN_MSK (1UL << 14)
#define TIMEOUT_TIMOUTEN_MSK    (1UL << 15)
#define TIMEOUT_TIMEOUTB_POS    (16U)
#define TIMEOUT_TIMEOUTB_MSK    (0xFFFUL << TIMEOUT_TIMEOUTB_POS)
#define TIMEOUT_EXT_MODE_MSK    (1UL << 29)
#define TIMEOUT_TOUTB_INTEN_MSK (1UL << 30)
#define TIMEOUT_EXTEN_MSK       (1UL << 31)

/** I2C_CLK_DIV 寄存器各位掩码 */
#define CLK_DIV_DIV_POS         (0U)
#define CLK_DIV_DIV_MSK         (0xFFFUL << CLK_DIV_DIV_POS)
#define CLK_DIV_DLY_POS         (12U)
#define CLK_DIV_DLY_MSK         (0xFFFUL << CLK_DIV_DLY_POS)

/** I2C_SLAVE_ADDR1 寄存器 */
#define ADDR1_POS               (1U)

/** I2C_SLAVE_ADDR2_3 寄存器 */
#define ADDR2_EN_MSK            (1UL << 0)
#define ADDR2_POS               (1U)
#define ADDR2_MSK               (0x7FUL << ADDR2_POS)
#define ADDR3_EN_MSK            (1UL << 8)
#define ADDR3_POS               (9U)
#define ADDR3_MSK               (0x7FUL << ADDR3_POS)

/** 向 I2C_SR 写1清0（SR 在设备头文件中声明为只读，需要强制转换） */
#define SR_CLEAR(i2cx, mask)    (*(volatile uint32_t *)(&(i2cx)->SR) = (uint32_t)(mask))


//===========================================
// 内部类型与状态变量
//===========================================

/** 驱动内部状态（每实例一份） */
static struct {
    i2c_callback_t callback;  ///< 用户注册的中断回调
} s_state[I2C_COUNT];

/** I2C外设寄存器指针表 */
static I2C_TypeDef * const s_i2c_regs[I2C_COUNT] = {
    I2C1, I2C2,
};

/** I2C外设时钟标识符表 */
static const clock_periph_t s_i2c_clks[I2C_COUNT] = {
    CLK_I2C1, CLK_I2C2,
};

/**
 * i2c_irq_type_t 位索引 → CR 寄存器位位置映射表
 * 0xFF 表示该中断不在 CR 中（在 TIMEOUT 寄存器中）
 */
static const uint8_t s_irq_to_cr_pos[14U] = {
    7U,    ///< I2C_IRQ_MTF        → CR.MTF_INT_EN[7]
    8U,    ///< I2C_IRQ_RX_ADDR1   → CR.RX_ADDR1_INT_EN[8]
    9U,    ///< I2C_IRQ_TXE        → CR.TXE_INT_EN[9]
    10U,   ///< I2C_IRQ_RXNE       → CR.RXNE_INT_EN[10]
    11U,   ///< I2C_IRQ_OVR        → CR.OVR_INT_EN[11]
    12U,   ///< I2C_IRQ_RX_ADDR2   → CR.RX_ADDR2_INT_EN[12]
    13U,   ///< I2C_IRQ_DETR       → CR.DETR_INT_EN[13]
    19U,   ///< I2C_IRQ_RX_ADDR3   → CR.RX_ADDR3_INTEN[19]
    20U,   ///< I2C_IRQ_STOPF      → CR.STOPF_INTEN[20]
    0xFFU, ///< I2C_IRQ_TIMEOUTAF  → TIMEOUT.TOUTA_INTEN[14]（非CR）
     0xFFU, ///< I2C_IRQ_TIMEOUTBF  → TIMEOUT.TOUTB_INTEN[30]（非CR）
    21U,   ///< I2C_IRQ_NACKF      → CR.NACKF_INTEN[21]
    22U,   ///< I2C_IRQ_TX_RX_FLAG → CR.TX_RX_FLAG_INTEN[22]
    16U,   ///< I2C_IRQ_MARLO      → CR.MARLO_INTEN[16]
};

//===========================================
// 内部辅助函数
//===========================================

/**
 * @brief 获取指定实例的寄存器指针
 */
static inline I2C_TypeDef *get_i2c(i2c_instance_t inst)
{
    return s_i2c_regs[inst];
}

/**
 * @brief 轮询等待 SR 中指定标志位置位
 *
 * @param i2c         I2C寄存器指针
 * @param flag        等待的标志位掩码
 * @param timeout_ms  超时时间 (ms)，0 使用默认值
 * @return true: 标志已置位，false: 超时
 */
static bool wait_flag_set(I2C_TypeDef *i2c, uint32_t flag, uint32_t timeout_ms)
{
    if (timeout_ms == 0U) {
        timeout_ms = I2C_DEFAULT_TIMEOUT_MS;
    }
    uint64_t t0 = system_get_tick();
    while (!(i2c->SR & flag)) {
        if (system_elapsed(t0, timeout_ms)) {
            return false;
        }
    }
    return true;
}

/**
 * @brief 轮询等待 SR 中指定标志位清零
 *
 * @param i2c         I2C寄存器指针
 * @param flag        等待清零的标志位掩码
 * @param timeout_ms  超时时间 (ms)，0 使用默认值
 * @return true: 标志已清零，false: 超时
 */
static bool wait_flag_clear(I2C_TypeDef *i2c, uint32_t flag, uint32_t timeout_ms)
{
    if (timeout_ms == 0U) {
        timeout_ms = I2C_DEFAULT_TIMEOUT_MS;
    }
    uint64_t t0 = system_get_tick();
    while (i2c->SR & flag) {
        if (system_elapsed(t0, timeout_ms)) {
            return false;
        }
    }
    return true;
}

/**
 * @brief 计算时钟分频值
 *
 * 公式：clk_div = pclk_hz / (4 × scl_hz) - 1
 *
 * @param pclk_hz  APB时钟频率（Hz）
 * @param scl_hz   目标SCL时钟频率（Hz）
 * @return 分频值，0 表示参数无效
 */
static uint32_t calc_clk_div(uint32_t pclk_hz, uint32_t scl_hz)
{
    if (scl_hz == 0U || pclk_hz == 0U) {
        return 0U;
    }
    uint32_t div = pclk_hz / (4U * scl_hz);
    if (div < 1U) {
        return 0U;
    }
    div -= 1U;
    if (div < I2C_CLK_DIV_MIN) {
        return 0U;
    }
    if (div > I2C_CLK_DIV_MAX) {
        return 0U;
    }
    return div;
}

/**
 * @brief 设置或清除 CR/TIMEOUT 中的中断使能位（内部公共实现）
 *
 * @param i2c        I2C寄存器指针
 * @param irq_types  中断类型位掩码
 * @param enable     true=使能，false=禁用
 */
static void irq_set_bits(I2C_TypeDef *i2c, i2c_irq_type_t irq_types, bool enable)
{
     uint32_t cr_mask      = 0U;
     uint32_t timeout_mask = 0U;
 
     for (uint32_t i = 0U; i < 14U; i++) {
         if (!((uint32_t)irq_types & (1U << i))) {
             continue;
         }
         uint8_t pos = s_irq_to_cr_pos[i];
         if (pos == 0xFFU) {
             /* TIMEOUTAF → TIMEOUT.TOUTA_INTEN[14]
                TIMEOUTBF → TIMEOUT.TOUTB_INTEN[30] */
             if (i == 9U) {
                 timeout_mask |= TIMEOUT_TOUTA_INTEN_MSK;
             } else {
                 timeout_mask |= TIMEOUT_TOUTB_INTEN_MSK;
             }
         } else {
             cr_mask |= (1UL << pos);
         }
     }
 
     if (enable) {
         i2c->CR      |= cr_mask;
         i2c->TIMEOUT |= timeout_mask;
     } else {
         i2c->CR      &= ~cr_mask;
         i2c->TIMEOUT &= ~timeout_mask;
     }
 }
 
 /**
  * @brief 主机发送流程公共部分：发 START → 写地址 → 发送数据
  *
  * @param i2c         寄存器指针
  * @param slave_addr  7位从机地址（不含R/W位）
  * @param data        待发送数据
  * @param len         字节数
  * @param timeout_ms  超时时间 (ms)
  * @param gen_stop    true=最后发送STOP，false=不发STOP（用于write_read中的写阶段）
  * @return true: 成功，false: NACK/超时
  */
static bool master_tx_phase(I2C_TypeDef *i2c, uint8_t slave_addr,
                            const uint8_t *data, size_t len,
                            uint32_t timeout_ms, bool gen_stop)
 {
     /* 产生 START（MASTER=1 保持，TX_AUTO_EN=1 自动处理方向） */
     i2c->CR |= CR_MASTER_MSK | CR_START_MSK;
 
     /* 写地址字节（写方向：LSB=0），写DR同时清除STARTF */
     i2c->DR = (uint32_t)((slave_addr << 1) | 0U);
 
     /* 等待地址字节传输完成（MTF=1） */
     if (!wait_flag_set(i2c, (uint32_t)I2C_FLAG_MTF, timeout_ms)) {
         i2c->CR |= CR_STOP_MSK;
         return false;
     }
     /* RACK=1 表示从机发送了 NACK */
     if (i2c->SR & (uint32_t)I2C_FLAG_RACK) {
         SR_CLEAR(i2c, (uint32_t)I2C_FLAG_MTF);
         i2c->CR |= CR_STOP_MSK;
         (void)wait_flag_clear(i2c, (uint32_t)I2C_FLAG_BUS_BUSY, timeout_ms);
         return false;
     }
     SR_CLEAR(i2c, (uint32_t)I2C_FLAG_MTF);

     /* 逐字节发送数据 */
     for (size_t i = 0U; i < len; i++) {
         /* 等待 DR 被移位寄存器取走（TXE=1） */
         if (!wait_flag_set(i2c, (uint32_t)I2C_FLAG_TXE, timeout_ms)) {
             i2c->CR |= CR_STOP_MSK;
             return false;
         }
         i2c->DR = data[i];  /* 写 DR，硬件清除 TXE */

         /* 等待字节（含ACK周期）传输完成 */
         if (!wait_flag_set(i2c, (uint32_t)I2C_FLAG_MTF, timeout_ms)) {
             i2c->CR |= CR_STOP_MSK;
             return false;
         }
         /* 检查是否收到 NACK */
         if (i2c->SR & (uint32_t)I2C_FLAG_NACKF) {
             SR_CLEAR(i2c, (uint32_t)I2C_FLAG_MTF | (uint32_t)I2C_FLAG_NACKF);
             i2c->CR |= CR_STOP_MSK;
             (void)wait_flag_clear(i2c, (uint32_t)I2C_FLAG_BUS_BUSY, timeout_ms);
             return false;
         }
         SR_CLEAR(i2c, (uint32_t)I2C_FLAG_MTF);
     }

     if (gen_stop) {
         i2c->CR |= CR_STOP_MSK;
         (void)wait_flag_clear(i2c, (uint32_t)I2C_FLAG_BUS_BUSY, timeout_ms);
     }
     return true;
 }
 
 /**
 * @brief 主机接收流程公共部分：写读地址 → 清MTF/TX_RX_FLAG → 接收数据
 *
 * 调用前需已产生 START，此函数从写读地址开始接管。
 * 逻辑如下：
 *   - 中间字节：等RXNE -> 等MTF（ACK已发完）-> 清MTF -> 读DR
 *   - 倒数第二字节：原子地读DR + 置TACK=NACK + 置STOP（防中断打断TACK窗口）
 *   - 最后一字节：等RXNE -> 读DR -> 等MTF（NACK+STOP完成）-> 清MTF
 *
 * @param i2c         寄存器指针
 * @param slave_addr  7位从机地址（不含R/W位）
 * @param data        接收缓冲区
 * @param len         字节数
 * @param timeout_ms  超时时间 (ms)
 * @return true: 成功，false: NACK/超时
 */
static bool master_rx_phase(I2C_TypeDef *i2c, uint8_t slave_addr,
                             uint8_t *data, size_t len, uint32_t timeout_ms)
{
    uint32_t primask;

    /* 写地址字节（读方向：LSB=1） */
    i2c->DR = (uint32_t)((slave_addr << 1) | 1U);

    /* 等待地址字节+ACK完成（MTF=1） */
    if (!wait_flag_set(i2c, (uint32_t)I2C_FLAG_MTF, timeout_ms)) {
        i2c->CR |= CR_STOP_MSK;
        return false;
    }
    if (i2c->SR & (uint32_t)I2C_FLAG_RACK) {
        SR_CLEAR(i2c, (uint32_t)I2C_FLAG_MTF | (uint32_t)I2C_FLAG_TX_RX_FLAG);
        i2c->CR |= CR_STOP_MSK;
        (void)wait_flag_clear(i2c, (uint32_t)I2C_FLAG_BUS_BUSY, timeout_ms);
        return false;
    }

    /*
     * TX_AUTO_EN=1 时，清除 MTF 触发硬件自动完成 TX->RX 切换并同时清 TX_RX_FLAG；
     * 两者在同一次写操作中一起清除，消除 W1C 竞争窗口。
     */
    SR_CLEAR(i2c, (uint32_t)I2C_FLAG_MTF | (uint32_t)I2C_FLAG_TX_RX_FLAG);

    if (len == 1U) {
        /*
         * 单字节接收：TX_RX_FLAG 已清（硬件开始产生接收 SCL），
         * 原子地置 TACK+STOP，确保在第一个字节的 ACK 时序前 TACK=NACK 已生效。
         */
        __asm volatile("mrs %0, primask" : "=r"(primask));
        __asm volatile("cpsid i");
        i2c->CR |= CR_TACK_MSK | CR_STOP_MSK;
        __asm volatile("msr primask, %0" :: "r"(primask));

        if (!wait_flag_set(i2c, (uint32_t)I2C_FLAG_RXNE, timeout_ms)) {
            return false;
        }
        data[0] = (uint8_t)(i2c->DR & 0xFFU);  /* 读DR（触发NACK时序） */

        /* 等待 NACK + STOP 完成（MTF） */
        if (!wait_flag_set(i2c, (uint32_t)I2C_FLAG_MTF, timeout_ms)) {
            return false;
        }
        SR_CLEAR(i2c, (uint32_t)I2C_FLAG_MTF);
    } else {
        /*
         * 多字节接收：确保 TACK=0（ACK模式）。
         * 硬件时序：RXNE 置位时字节已在 DR，ACK 时序尚未发出（SCL 被拉低）；
         * 等 MTF 确认 ACK 已发完后再读 DR，此后修改 TACK 不影响当前字节。
         */
        i2c->CR &= ~CR_TACK_MSK;

        /* 循环接收前 len-1 字节（不含最后一字节） */
        for (size_t i = 0U; i < len - 1U; i++) {
            if (!wait_flag_set(i2c, (uint32_t)I2C_FLAG_RXNE, timeout_ms)) {
                i2c->CR |= CR_STOP_MSK;
                return false;
            }
            /* 等 ACK 时序完成（MTF=1），确认当前字节 ACK 已发出，再安全读DR/改TACK */
            if (!wait_flag_set(i2c, (uint32_t)I2C_FLAG_MTF, timeout_ms)) {
                i2c->CR |= CR_STOP_MSK;
                return false;
            }
            SR_CLEAR(i2c, (uint32_t)I2C_FLAG_MTF);

            if (i == len - 2U) {
                /*
                 * 倒数第二字节：原子地读 DR 并置 TACK=NACK + STOP。
                 * 读 DR 清 RXNE，触发硬件开始接收最后一字节；
                 * 关中断防止 TACK 设置被延迟（TACK 须在最后字节 ACK 前生效）。
                 */
                __asm volatile("mrs %0, primask" : "=r"(primask));
                __asm volatile("cpsid i");
                data[i] = (uint8_t)(i2c->DR & 0xFFU);
                i2c->CR |= CR_TACK_MSK | CR_STOP_MSK;
                __asm volatile("msr primask, %0" :: "r"(primask));
            } else {
                data[i] = (uint8_t)(i2c->DR & 0xFFU);
            }
        }

        /* 最后一字节：等RXNE，读DR（触发NACK），等MTF（NACK+STOP完成），清MTF */
        if (!wait_flag_set(i2c, (uint32_t)I2C_FLAG_RXNE, timeout_ms)) {
            i2c->CR |= CR_STOP_MSK;
            return false;
        }
        data[len - 1U] = (uint8_t)(i2c->DR & 0xFFU);

        if (!wait_flag_set(i2c, (uint32_t)I2C_FLAG_MTF, timeout_ms)) {
            return false;
        }
        SR_CLEAR(i2c, (uint32_t)I2C_FLAG_MTF);
    }

    /* 等待总线释放 */
    (void)wait_flag_clear(i2c, (uint32_t)I2C_FLAG_BUS_BUSY, timeout_ms);

    /* 恢复 TACK=ACK */
    i2c->CR &= ~CR_TACK_MSK;

    return true;
}
 
 /**
  * @brief 所有实例共用的 IRQ 处理核心
  *
  * 读取 SR 快照，调用用户回调。标志清除由用户回调负责。
  */
 static void irq_common(i2c_instance_t inst)
 {
     I2C_TypeDef *i2c = get_i2c(inst);
     i2c_callback_t cb = s_state[inst].callback;
 
     if (cb != NULL) {
         i2c_irq_context_t ctx = {
             .inst  = inst,
             .flags = i2c->SR,
         };
         cb(&ctx);
     }
 }
 
//===========================================
 // 第1层：快速初始化 API
//===========================================
 
 bool i2c_init_master(i2c_instance_t inst, uint32_t pclk_hz, uint32_t scl_hz)
 {
     if (inst >= I2C_COUNT) {
         return false;
     }
 
     uint32_t clk_div = calc_clk_div(pclk_hz, scl_hz);
     if (clk_div == 0U) {
         return false;
     }
 
     clock_periph_enable(s_i2c_clks[inst]);
 
     I2C_TypeDef *i2c = get_i2c(inst);
 
     /* 先停止并清空 CR，再写入主模式配置 */
     i2c->CR = 0U;
 
     i2c->CLK_DIV = (i2c->CLK_DIV & ~CLK_DIV_DIV_MSK)
                  | ((clk_div << CLK_DIV_DIV_POS) & CLK_DIV_DIV_MSK);
 
     /* 主模式：MEN + MASTER + TX_AUTO_EN + HIGH_WAITEN + MULTIMA_EN */
     i2c->CR = CR_MEN_MSK
             | CR_MASTER_MSK
             | CR_TX_AUTO_EN_MSK
             | CR_HIGH_WAITEN_MSK
             | CR_MULTIMA_EN_MSK;
 
     return true;
 }
 
 bool i2c_init_slave(i2c_instance_t inst, uint8_t addr)
 {
     if (inst >= I2C_COUNT) {
         return false;
     }
 
     clock_periph_enable(s_i2c_clks[inst]);
 
     I2C_TypeDef *i2c = get_i2c(inst);
 
     i2c->CR = 0U;
 
     /* 写从机地址1（bits[7:1]） */
     i2c->SLAVE_ADDR1 = (uint32_t)addr << ADDR1_POS;
 
     /* 从模式：MEN + TX_AUTO_EN */
     i2c->CR = CR_MEN_MSK | CR_TX_AUTO_EN_MSK;
 
     return true;
 }
 
//===========================================
 // 第2层：基础配置 API
//===========================================
 
 bool i2c_config_master(i2c_instance_t inst, const i2c_master_config_t *config)
 {
     if (inst >= I2C_COUNT) {
         return false;
     }
     assert(config != NULL);
 
     if (config->clk_div < I2C_CLK_DIV_MIN || config->clk_div > I2C_CLK_DIV_MAX) {
         return false;
     }
 
     I2C_TypeDef *i2c = get_i2c(inst);
 
     i2c->CLK_DIV = (i2c->CLK_DIV & ~CLK_DIV_DIV_MSK)
                  | (((uint32_t)config->clk_div << CLK_DIV_DIV_POS) & CLK_DIV_DIV_MSK);
 
     uint32_t cr = CR_MASTER_MSK | CR_MEN_MSK;
     if (config->tx_auto_en)      { cr |= CR_TX_AUTO_EN_MSK; }
     if (config->multi_master_en) { cr |= CR_MULTIMA_EN_MSK; }
     if (config->high_wait_en)    { cr |= CR_HIGH_WAITEN_MSK; }
     if (config->marlo_sel)       { cr |= CR_MARLO_SEL_MSK; }
 
     /* 保留中断使能位和高级功能位，仅更新控制位 */
     uint32_t keep_mask = CR_MTF_INT_EN_MSK   | CR_RX_ADDR1_INT_EN_MSK
                        | CR_TXE_INT_EN_MSK   | CR_RXNE_INT_EN_MSK
                        | CR_OVR_INT_EN_MSK   | CR_RX_ADDR2_INT_EN_MSK
                         | CR_DETR_INT_EN_MSK
                         | CR_MARLO_INTEN_MSK  | CR_TXE_SEL_MSK
                        | CR_DMA_EN_MSK       | CR_RX_ADDR3_INTEN_MSK
                        | CR_STOPF_INTEN_MSK  | CR_NACKF_INTEN_MSK
                        | CR_TX_RX_FLAG_INTEN_MSK
                        | CR_SU_HD_FIXEN_MSK  | CR_STRETCH_FIXEN_MSK
                        | CR_NOSTRETCH_MSK;
 
     i2c->CR = (i2c->CR & keep_mask) | cr;
     return true;
 }
 
 bool i2c_config_slave(i2c_instance_t inst, const i2c_slave_config_t *config)
 {
     if (inst >= I2C_COUNT) {
         return false;
     }
     assert(config != NULL);
 
     I2C_TypeDef *i2c = get_i2c(inst);
 
     /* 从机地址1 */
     i2c->SLAVE_ADDR1 = (uint32_t)config->addr1 << ADDR1_POS;
 
     /* 从机地址2和3 */
     uint32_t addr23 = ((uint32_t)config->addr2 << ADDR2_POS) & ADDR2_MSK;
     if (config->addr2_en) {
         addr23 |= ADDR2_EN_MSK;
     }
     addr23 |= ((uint32_t)config->addr3 << ADDR3_POS) & ADDR3_MSK;
     if (config->addr3_en) {
         addr23 |= ADDR3_EN_MSK;
     }
     i2c->SLAVE_ADDR2_3 = addr23;
 
     /* 保留中断使能位，重新配置控制位 */
     uint32_t keep_mask = CR_MTF_INT_EN_MSK   | CR_RX_ADDR1_INT_EN_MSK
                        | CR_TXE_INT_EN_MSK   | CR_RXNE_INT_EN_MSK
                        | CR_OVR_INT_EN_MSK   | CR_RX_ADDR2_INT_EN_MSK
                         | CR_DETR_INT_EN_MSK
                         | CR_MARLO_INTEN_MSK  | CR_RX_ADDR3_INTEN_MSK
                        | CR_STOPF_INTEN_MSK  | CR_NACKF_INTEN_MSK
                        | CR_TX_RX_FLAG_INTEN_MSK
                        | CR_SU_HD_FIXEN_MSK  | CR_STRETCH_FIXEN_MSK
                        | CR_MULTIMA_EN_MSK   | CR_HIGH_WAITEN_MSK;
 
     uint32_t cr = CR_MEN_MSK;
     if (config->tx_auto_en) { cr |= CR_TX_AUTO_EN_MSK; }
     if (config->nostretch)  { cr |= CR_NOSTRETCH_MSK; }
     if (config->txe_sel)    { cr |= CR_TXE_SEL_MSK; }
 
     i2c->CR = (i2c->CR & keep_mask) | cr;
     return true;
 }
 
 void i2c_config_clock_div(i2c_instance_t inst, uint16_t clk_div)
 {
     I2C_TypeDef *i2c = get_i2c(inst);
     i2c->CLK_DIV = (i2c->CLK_DIV & ~CLK_DIV_DIV_MSK)
                  | (((uint32_t)clk_div << CLK_DIV_DIV_POS) & CLK_DIV_DIV_MSK);
 }
 
 void i2c_config_filter(i2c_instance_t inst, const i2c_filter_config_t *config)
 {
     assert(config != NULL);
     I2C_TypeDef *i2c = get_i2c(inst);
 
     /* FITER 寄存器：SCL_FITER[4:0]，SDA_IN_DELAY[12:8] */
     i2c->FILTER = ((uint32_t)config->scl_filter   & 0x1FU)
                | (((uint32_t)config->sda_in_delay & 0x1FU) << 8);
 }
 
 void i2c_config_slave_addr1(i2c_instance_t inst, uint8_t addr)
 {
     get_i2c(inst)->SLAVE_ADDR1 = (uint32_t)addr << ADDR1_POS;
 }
 
 void i2c_config_slave_addr2(i2c_instance_t inst, uint8_t addr, bool enable)
 {
     I2C_TypeDef *i2c = get_i2c(inst);
     uint32_t val = i2c->SLAVE_ADDR2_3;
     val &= ~(ADDR2_MSK | ADDR2_EN_MSK);
     val |= ((uint32_t)addr << ADDR2_POS) & ADDR2_MSK;
     if (enable) {
         val |= ADDR2_EN_MSK;
     }
     i2c->SLAVE_ADDR2_3 = val;
 }
 
 void i2c_config_slave_addr3(i2c_instance_t inst, uint8_t addr, bool enable)
 {
     I2C_TypeDef *i2c = get_i2c(inst);
     uint32_t val = i2c->SLAVE_ADDR2_3;
     val &= ~(ADDR3_MSK | ADDR3_EN_MSK);
     val |= ((uint32_t)addr << ADDR3_POS) & ADDR3_MSK;
     if (enable) {
         val |= ADDR3_EN_MSK;
     }
     i2c->SLAVE_ADDR2_3 = val;
 }
 
 void i2c_config_stretch_fix(i2c_instance_t inst, const i2c_stretch_fix_config_t *config)
 {
     assert(config != NULL);
     I2C_TypeDef *i2c = get_i2c(inst);
 
     /* 更新 CLK_DIV.DLY_TIME[23:12] */
     i2c->CLK_DIV = (i2c->CLK_DIV & ~CLK_DIV_DLY_MSK)
                  | (((uint32_t)config->dly_time << CLK_DIV_DLY_POS) & CLK_DIV_DLY_MSK);
 
     /* 更新 CR.STRETCH_FIXEN 和 CR.SU_HD_FIXEN */
     uint32_t cr = i2c->CR;
     cr &= ~(CR_STRETCH_FIXEN_MSK | CR_SU_HD_FIXEN_MSK);
     if (config->stretch_fix_en) { cr |= CR_STRETCH_FIXEN_MSK; }
     if (config->su_hd_fix_en)   { cr |= CR_SU_HD_FIXEN_MSK; }
     i2c->CR = cr;
 }
 
 bool i2c_config_smbus_timeout(i2c_instance_t inst, const i2c_smbus_timeout_config_t *config)
 {
     if (inst >= I2C_COUNT) {
         return false;
     }
     assert(config != NULL);
 
     I2C_TypeDef *i2c = get_i2c(inst);
 
     uint32_t val = 0U;
 
     /* TimeoutA */
     val |= ((uint32_t)config->timeouta << TIMEOUT_TIMEOUTA_POS) & TIMEOUT_TIMEOUTA_MSK;
     if (config->timeouta_en)     { val |= TIMEOUT_TIMOUTEN_MSK; }
     if (config->timeouta_irq_en) { val |= TIMEOUT_TOUTA_INTEN_MSK; }
 
     /* TimeoutB */
     val |= ((uint32_t)config->timeoutb << TIMEOUT_TIMEOUTB_POS) & TIMEOUT_TIMEOUTB_MSK;
     if (config->timeoutb_en)     { val |= TIMEOUT_EXTEN_MSK; }
     if (config->timeoutb_irq_en) { val |= TIMEOUT_TOUTB_INTEN_MSK; }
 
     /* EXT_MODE */
     if (config->ext_mode) { val |= TIMEOUT_EXT_MODE_MSK; }
 
     i2c->TIMEOUT = val;
     return true;
 }
 
//===========================================
 // 第3层：高级功能 API
//===========================================
 
 void i2c_config_dma(i2c_instance_t inst, bool enable)
 {
     I2C_TypeDef *i2c = get_i2c(inst);
     if (enable) {
         i2c->CR |= CR_DMA_EN_MSK;
     } else {
         i2c->CR &= ~CR_DMA_EN_MSK;
     }
 }
 
 void i2c_config_txe_sel(i2c_instance_t inst, bool set_on_addr_match)
 {
     I2C_TypeDef *i2c = get_i2c(inst);
     if (set_on_addr_match) {
         i2c->CR |= CR_TXE_SEL_MSK;
     } else {
         i2c->CR &= ~CR_TXE_SEL_MSK;
     }
 }
 
 void i2c_config_nostretch(i2c_instance_t inst, bool disable)
 {
     I2C_TypeDef *i2c = get_i2c(inst);
     if (disable) {
         i2c->CR |= CR_NOSTRETCH_MSK;
     } else {
         i2c->CR &= ~CR_NOSTRETCH_MSK;
     }
 }
 
 void i2c_config_tx_auto_en(i2c_instance_t inst, bool enable)
 {
     I2C_TypeDef *i2c = get_i2c(inst);
     if (enable) {
         i2c->CR |= CR_TX_AUTO_EN_MSK;
     } else {
         i2c->CR &= ~CR_TX_AUTO_EN_MSK;
     }
 }
 
 void i2c_config_multi_master(i2c_instance_t inst, bool enable)
 {
     I2C_TypeDef *i2c = get_i2c(inst);
     if (enable) {
         i2c->CR |= CR_MULTIMA_EN_MSK;
     } else {
         i2c->CR &= ~CR_MULTIMA_EN_MSK;
     }
 }
 
 void i2c_config_high_wait(i2c_instance_t inst, bool enable)
 {
     I2C_TypeDef *i2c = get_i2c(inst);
     if (enable) {
         i2c->CR |= CR_HIGH_WAITEN_MSK;
     } else {
         i2c->CR &= ~CR_HIGH_WAITEN_MSK;
     }
 }
 
 void i2c_config_marlo_sel(i2c_instance_t inst, bool auto_switch)
 {
     I2C_TypeDef *i2c = get_i2c(inst);
     if (auto_switch) {
         i2c->CR |= CR_MARLO_SEL_MSK;
     } else {
         i2c->CR &= ~CR_MARLO_SEL_MSK;
     }
 }
 
 void i2c_config_sda_detect(i2c_instance_t inst, uint16_t threshold)
 {
     get_i2c(inst)->DET = threshold;
 }

bool i2c_master_transmit(i2c_instance_t inst, uint8_t slave_addr,
                         const uint8_t *data, size_t len, uint32_t timeout_ms)
{
    if (inst >= I2C_COUNT || data == NULL || len == 0U) {
        return false;
    }

    I2C_TypeDef *i2c = get_i2c(inst);

    if (!wait_flag_clear(i2c, (uint32_t)I2C_FLAG_BUS_BUSY, timeout_ms)) {
        return false;
    }

    return master_tx_phase(i2c, slave_addr, data, len, timeout_ms, true);
}

bool i2c_master_receive(i2c_instance_t inst, uint8_t slave_addr,
                        uint8_t *data, size_t len, uint32_t timeout_ms)
{
    if (inst >= I2C_COUNT || data == NULL || len == 0U) {
        return false;
    }

    I2C_TypeDef *i2c = get_i2c(inst);

    if (!wait_flag_clear(i2c, (uint32_t)I2C_FLAG_BUS_BUSY, timeout_ms)) {
        return false;
    }

    /* 产生 START */
    i2c->CR |= CR_MASTER_MSK | CR_START_MSK;

    return master_rx_phase(i2c, slave_addr, data, len, timeout_ms);
}

//===========================================
// 第4层：控制与查询 API
//===========================================

/* ── 使能控制 ── */

void i2c_enable(i2c_instance_t inst)
{
    get_i2c(inst)->CR |= CR_MEN_MSK;
}

void i2c_disable(i2c_instance_t inst)
{
    get_i2c(inst)->CR &= ~CR_MEN_MSK;
}

bool i2c_is_enabled(i2c_instance_t inst)
{
    return (get_i2c(inst)->CR & CR_MEN_MSK) != 0U;
}

/* ── 总线操作 ── */

void i2c_start(i2c_instance_t inst)
{
    get_i2c(inst)->CR |= CR_START_MSK;
}

void i2c_stop(i2c_instance_t inst)
{
    get_i2c(inst)->CR |= CR_STOP_MSK;
}

void i2c_set_tack(i2c_instance_t inst, i2c_ack_t ack)
{
    I2C_TypeDef *i2c = get_i2c(inst);
    if (ack == I2C_NACK) {
        i2c->CR |= CR_TACK_MSK;
    } else {
        i2c->CR &= ~CR_TACK_MSK;
    }
}

void i2c_set_tx_mode(i2c_instance_t inst, bool tx)
{
    I2C_TypeDef *i2c = get_i2c(inst);
    if (tx) {
        i2c->CR |= CR_TX_MSK;
    } else {
        i2c->CR &= ~CR_TX_MSK;
    }
}

void i2c_set_master_mode(i2c_instance_t inst, bool master)
{
    I2C_TypeDef *i2c = get_i2c(inst);
    if (master) {
        i2c->CR |= CR_MASTER_MSK;
    } else {
        i2c->CR &= ~CR_MASTER_MSK;
    }
}

/* ── 数据读写 ── */

void i2c_write_dr(i2c_instance_t inst, uint8_t data)
{
    get_i2c(inst)->DR = data;
}

uint8_t i2c_read_dr(i2c_instance_t inst)
{
    return (uint8_t)(get_i2c(inst)->DR & 0xFFU);
}

/* ── 状态查询 ── */

bool i2c_flag_is_set(i2c_instance_t inst, i2c_flag_t flag)
{
    return (get_i2c(inst)->SR & (uint32_t)flag) != 0U;
}

void i2c_flag_clear(i2c_instance_t inst, i2c_flag_t flag)
{
    SR_CLEAR(get_i2c(inst), (uint32_t)flag);
}

uint32_t i2c_get_sr(i2c_instance_t inst)
{
    return get_i2c(inst)->SR;
}

bool i2c_wait_bus_free(i2c_instance_t inst, uint32_t timeout_ms)
{
    return wait_flag_clear(get_i2c(inst), (uint32_t)I2C_FLAG_BUS_BUSY, timeout_ms);
}

/* ── 中断管理 ── */

void i2c_irq_enable(i2c_instance_t inst, i2c_irq_type_t irq_types)
{
    irq_set_bits(get_i2c(inst), irq_types, true);
}

void i2c_irq_disable(i2c_instance_t inst, i2c_irq_type_t irq_types)
{
    irq_set_bits(get_i2c(inst), irq_types, false);
}

void i2c_irq_register(i2c_instance_t inst, i2c_callback_t callback)
{
    if (inst < I2C_COUNT) {
        s_state[inst].callback = callback;
    }
}

void i2c_irq_unregister(i2c_instance_t inst)
{
    if (inst < I2C_COUNT) {
        s_state[inst].callback = NULL;
    }
}

/* ── 寄存器读取与综合操作 ── */

uint32_t i2c_get_clk_div(i2c_instance_t inst)
{
    return get_i2c(inst)->CLK_DIV;
}

uint8_t i2c_get_slave_addr1(i2c_instance_t inst)
{
    return (uint8_t)((get_i2c(inst)->SLAVE_ADDR1 >> ADDR1_POS) & 0x7FU);
}

uint8_t i2c_get_slave_addr2(i2c_instance_t inst)
{
    return (uint8_t)((get_i2c(inst)->SLAVE_ADDR2_3 >> ADDR2_POS) & 0x7FU);
}

uint8_t i2c_get_slave_addr3(i2c_instance_t inst)
{
    return (uint8_t)((get_i2c(inst)->SLAVE_ADDR2_3 >> ADDR3_POS) & 0x7FU);
}

uint32_t i2c_get_cr(i2c_instance_t inst)
{
    return get_i2c(inst)->CR;
}

bool i2c_smbus_timeout_is_set(i2c_instance_t inst, bool timeout_a)
{
    uint32_t flag = timeout_a ? (uint32_t)I2C_FLAG_TIMEOUTAF
                              : (uint32_t)I2C_FLAG_TIMEOUTBF;
    return (get_i2c(inst)->SR & flag) != 0U;
}

void i2c_smbus_timeout_clear(i2c_instance_t inst, bool timeout_a)
{
    uint32_t flag = timeout_a ? (uint32_t)I2C_FLAG_TIMEOUTAF
                              : (uint32_t)I2C_FLAG_TIMEOUTBF;
    SR_CLEAR(get_i2c(inst), flag);
}

bool i2c_master_write_read(i2c_instance_t inst, uint8_t slave_addr,
                           const uint8_t *write_data, size_t write_len,
                           uint8_t *read_data,  size_t read_len,
                           uint32_t timeout_ms)
{
    if (inst >= I2C_COUNT
        || write_data == NULL || write_len == 0U
        || read_data  == NULL || read_len  == 0U) {
        return false;
    }

    I2C_TypeDef *i2c = get_i2c(inst);

    /* 等待总线空闲 */
    if (!wait_flag_clear(i2c, (uint32_t)I2C_FLAG_BUS_BUSY, timeout_ms)) {
        return false;
    }

    /* ── 写阶段：START + 地址(写方向) + 数据，不发 STOP ── */
    if (!master_tx_phase(i2c, slave_addr, write_data, write_len, timeout_ms, false)) {
        return false;
    }

    /* ── 重复 START + 读阶段 ── */
    i2c->CR |= CR_START_MSK;  /* 产生重复START，写DR之前硬件保持SCL低 */

    return master_rx_phase(i2c, slave_addr, read_data, read_len, timeout_ms);
}

//===========================================
// 中断处理函数
//===========================================

/**
 * @brief I2C1 中断服务函数
 * @note  用户不可重定义，请通过 i2c_irq_register(I2C_1, cb) 注册回调
 */
void I2C1_IRQHandler(void)
{
    irq_common(I2C_1);
}

/**
 * @brief I2C2 中断服务函数
 */
void I2C2_IRQHandler(void)
{
    irq_common(I2C_2);
}