/**
 * @file tim.c
 * @brief ACM32P4 TIM驱动实现
 * 
 * 本文件实现了ACM32P4芯片的定时器驱动功能，支持：
 * - 高级定时器（TIM1/8/20）
 * - 通用定时器（TIM2-5/9-19/23-25）
 * - 基本定时器（TIM6/7/21/22）
 * 
 * 特性：
 * - 4层API架构
 * - 类型安全
 * - 中断/DMA支持
 * 
 * @author ACM32P4 SDK Team
 * @version 1.0
 * @date 2026-02-11
 */

#include "hardware/tim.h"
#include "hardware/clocks.h"
#include "device/acm32p4xx.h"
#include <string.h>

//===========================================
// 内部宏定义
//===========================================

// CR1寄存器位定义
#define TIM_CR1_CEN_Pos         (0U)
#define TIM_CR1_CEN_Msk         (0x1UL << TIM_CR1_CEN_Pos)
#define TIM_CR1_UDIS_Pos        (1U)
#define TIM_CR1_UDIS_Msk        (0x1UL << TIM_CR1_UDIS_Pos)
#define TIM_CR1_URS_Pos         (2U)
#define TIM_CR1_URS_Msk         (0x1UL << TIM_CR1_URS_Pos)
#define TIM_CR1_OPM_Pos         (3U)
#define TIM_CR1_OPM_Msk         (0x1UL << TIM_CR1_OPM_Pos)
#define TIM_CR1_DIR_Pos         (4U)
#define TIM_CR1_DIR_Msk         (0x1UL << TIM_CR1_DIR_Pos)
#define TIM_CR1_CMS_Pos         (5U)
#define TIM_CR1_CMS_Msk         (0x3UL << TIM_CR1_CMS_Pos)
#define TIM_CR1_ARPE_Pos        (7U)
#define TIM_CR1_ARPE_Msk        (0x1UL << TIM_CR1_ARPE_Pos)
#define TIM_CR1_CKD_Pos         (8U)
#define TIM_CR1_CKD_Msk         (0x3UL << TIM_CR1_CKD_Pos)

// CR2寄存器位定义
#define TIM_CR2_CCPC_Pos        (0U)
#define TIM_CR2_CCPC_Msk        (0x1UL << TIM_CR2_CCPC_Pos)
#define TIM_CR2_CCUS_Pos        (2U)
#define TIM_CR2_CCUS_Msk        (0x1UL << TIM_CR2_CCUS_Pos)
#define TIM_CR2_CCDS_Pos        (3U)
#define TIM_CR2_CCDS_Msk        (0x1UL << TIM_CR2_CCDS_Pos)
#define TIM_CR2_MMS_Pos         (4U)
#define TIM_CR2_MMS_Msk         (0x7UL << TIM_CR2_MMS_Pos)
#define TIM_CR2_TI1S_Pos        (7U)
#define TIM_CR2_TI1S_Msk        (0x1UL << TIM_CR2_TI1S_Pos)
#define TIM_CR2_OIS1_Pos        (8U)
#define TIM_CR2_OIS1_Msk        (0x1UL << TIM_CR2_OIS1_Pos)
#define TIM_CR2_OIS1N_Pos       (9U)
#define TIM_CR2_OIS1N_Msk       (0x1UL << TIM_CR2_OIS1N_Pos)
#define TIM_CR2_OIS2_Pos        (10U)
#define TIM_CR2_OIS2_Msk        (0x1UL << TIM_CR2_OIS2_Pos)
#define TIM_CR2_OIS2N_Pos       (11U)
#define TIM_CR2_OIS2N_Msk       (0x1UL << TIM_CR2_OIS2N_Pos)
#define TIM_CR2_OIS3_Pos        (12U)
#define TIM_CR2_OIS3_Msk        (0x1UL << TIM_CR2_OIS3_Pos)
#define TIM_CR2_OIS3N_Pos       (13U)
#define TIM_CR2_OIS3N_Msk       (0x1UL << TIM_CR2_OIS3N_Pos)
#define TIM_CR2_OIS4_Pos        (14U)
#define TIM_CR2_OIS4_Msk        (0x1UL << TIM_CR2_OIS4_Pos)
#define TIM_CR2_MMS2_Pos        (20U)
#define TIM_CR2_MMS2_Msk        (0x1FUL << TIM_CR2_MMS2_Pos)

// SMCR寄存器位定义
#define TIM_SMCR_SMS_Pos        (0U)
#define TIM_SMCR_SMS_Msk        (0x7UL << TIM_SMCR_SMS_Pos)
#define TIM_SMCR_TS_Pos         (4U)
#define TIM_SMCR_TS_Msk         (0x7UL << TIM_SMCR_TS_Pos)
#define TIM_SMCR_MSM_Pos        (7U)
#define TIM_SMCR_MSM_Msk        (0x1UL << TIM_SMCR_MSM_Pos)
#define TIM_SMCR_ETF_Pos        (8U)
#define TIM_SMCR_ETF_Msk        (0xFUL << TIM_SMCR_ETF_Pos)
#define TIM_SMCR_ETPS_Pos       (12U)
#define TIM_SMCR_ETPS_Msk       (0x3UL << TIM_SMCR_ETPS_Pos)
#define TIM_SMCR_ECE_Pos        (14U)
#define TIM_SMCR_ECE_Msk        (0x1UL << TIM_SMCR_ECE_Pos)
#define TIM_SMCR_ETP_Pos        (15U)
#define TIM_SMCR_ETP_Msk        (0x1UL << TIM_SMCR_ETP_Pos)

// DIER寄存器位定义
#define TIM_DIER_UIE_Pos        (0U)
#define TIM_DIER_UIE_Msk        (0x1UL << TIM_DIER_UIE_Pos)
#define TIM_DIER_CC1IE_Pos      (1U)
#define TIM_DIER_CC1IE_Msk      (0x1UL << TIM_DIER_CC1IE_Pos)
#define TIM_DIER_CC2IE_Pos      (2U)
#define TIM_DIER_CC2IE_Msk      (0x1UL << TIM_DIER_CC2IE_Pos)
#define TIM_DIER_CC3IE_Pos      (3U)
#define TIM_DIER_CC3IE_Msk      (0x1UL << TIM_DIER_CC3IE_Pos)
#define TIM_DIER_CC4IE_Pos      (4U)
#define TIM_DIER_CC4IE_Msk      (0x1UL << TIM_DIER_CC4IE_Pos)
#define TIM_DIER_COMIE_Pos      (5U)
#define TIM_DIER_COMIE_Msk      (0x1UL << TIM_DIER_COMIE_Pos)
#define TIM_DIER_TIE_Pos        (6U)
#define TIM_DIER_TIE_Msk        (0x1UL << TIM_DIER_TIE_Pos)
#define TIM_DIER_BIE_Pos        (7U)
#define TIM_DIER_BIE_Msk        (0x1UL << TIM_DIER_BIE_Pos)
#define TIM_DIER_UDE_Pos        (8U)
#define TIM_DIER_UDE_Msk        (0x1UL << TIM_DIER_UDE_Pos)
#define TIM_DIER_CC1DE_Pos      (9U)
#define TIM_DIER_CC1DE_Msk      (0x1UL << TIM_DIER_CC1DE_Pos)
#define TIM_DIER_CC2DE_Pos      (10U)
#define TIM_DIER_CC2DE_Msk      (0x1UL << TIM_DIER_CC2DE_Pos)
#define TIM_DIER_CC3DE_Pos      (11U)
#define TIM_DIER_CC3DE_Msk      (0x1UL << TIM_DIER_CC3DE_Pos)
#define TIM_DIER_CC4DE_Pos      (12U)
#define TIM_DIER_CC4DE_Msk      (0x1UL << TIM_DIER_CC4DE_Pos)
#define TIM_DIER_COMDE_Pos      (13U)
#define TIM_DIER_COMDE_Msk      (0x1UL << TIM_DIER_COMDE_Pos)
#define TIM_DIER_TDE_Pos        (14U)
#define TIM_DIER_TDE_Msk        (0x1UL << TIM_DIER_TDE_Pos)

// SR寄存器位定义
#define TIM_SR_UIF_Pos          (0U)
#define TIM_SR_UIF_Msk          (0x1UL << TIM_SR_UIF_Pos)
#define TIM_SR_CC1IF_Pos        (1U)
#define TIM_SR_CC1IF_Msk        (0x1UL << TIM_SR_CC1IF_Pos)
#define TIM_SR_CC2IF_Pos        (2U)
#define TIM_SR_CC2IF_Msk        (0x1UL << TIM_SR_CC2IF_Pos)
#define TIM_SR_CC3IF_Pos        (3U)
#define TIM_SR_CC3IF_Msk        (0x1UL << TIM_SR_CC3IF_Pos)
#define TIM_SR_CC4IF_Pos        (4U)
#define TIM_SR_CC4IF_Msk        (0x1UL << TIM_SR_CC4IF_Pos)
#define TIM_SR_COMIF_Pos        (5U)
#define TIM_SR_COMIF_Msk        (0x1UL << TIM_SR_COMIF_Pos)
#define TIM_SR_TIF_Pos          (6U)
#define TIM_SR_TIF_Msk          (0x1UL << TIM_SR_TIF_Pos)
#define TIM_SR_BIF_Pos          (7U)
#define TIM_SR_BIF_Msk          (0x1UL << TIM_SR_BIF_Pos)
#define TIM_SR_CC1OF_Pos        (9U)
#define TIM_SR_CC1OF_Msk        (0x1UL << TIM_SR_CC1OF_Pos)
#define TIM_SR_CC2OF_Pos        (10U)
#define TIM_SR_CC2OF_Msk        (0x1UL << TIM_SR_CC2OF_Pos)
#define TIM_SR_CC3OF_Pos        (11U)
#define TIM_SR_CC3OF_Msk        (0x1UL << TIM_SR_CC3OF_Pos)
#define TIM_SR_CC4OF_Pos        (12U)
#define TIM_SR_CC4OF_Msk        (0x1UL << TIM_SR_CC4OF_Pos)

// EGR寄存器位定义
#define TIM_EGR_UG_Pos          (0U)
#define TIM_EGR_UG_Msk          (0x1UL << TIM_EGR_UG_Pos)
#define TIM_EGR_CC1G_Pos        (1U)
#define TIM_EGR_CC1G_Msk        (0x1UL << TIM_EGR_CC1G_Pos)
#define TIM_EGR_CC2G_Pos        (2U)
#define TIM_EGR_CC2G_Msk        (0x1UL << TIM_EGR_CC2G_Pos)
#define TIM_EGR_CC3G_Pos        (3U)
#define TIM_EGR_CC3G_Msk        (0x1UL << TIM_EGR_CC3G_Pos)
#define TIM_EGR_CC4G_Pos        (4U)
#define TIM_EGR_CC4G_Msk        (0x1UL << TIM_EGR_CC4G_Pos)
#define TIM_EGR_COMG_Pos        (5U)
#define TIM_EGR_COMG_Msk        (0x1UL << TIM_EGR_COMG_Pos)
#define TIM_EGR_TG_Pos          (6U)
#define TIM_EGR_TG_Msk          (0x1UL << TIM_EGR_TG_Pos)
#define TIM_EGR_BG_Pos          (7U)
#define TIM_EGR_BG_Msk          (0x1UL << TIM_EGR_BG_Pos)

// CCMR1 Output模式位定义
#define TIM_CCMR1_CC1S_Pos      (0U)
#define TIM_CCMR1_CC1S_Msk      (0x3UL << TIM_CCMR1_CC1S_Pos)
#define TIM_CCMR1_OC1FE_Pos     (2U)
#define TIM_CCMR1_OC1FE_Msk     (0x1UL << TIM_CCMR1_OC1FE_Pos)
#define TIM_CCMR1_OC1PE_Pos     (3U)
#define TIM_CCMR1_OC1PE_Msk     (0x1UL << TIM_CCMR1_OC1PE_Pos)
#define TIM_CCMR1_OC1M_Pos      (4U)
#define TIM_CCMR1_OC1M_Msk      (0x7UL << TIM_CCMR1_OC1M_Pos)
#define TIM_CCMR1_OC1CE_Pos     (7U)
#define TIM_CCMR1_OC1CE_Msk     (0x1UL << TIM_CCMR1_OC1CE_Pos)
#define TIM_CCMR1_CC2S_Pos      (8U)
#define TIM_CCMR1_CC2S_Msk      (0x3UL << TIM_CCMR1_CC2S_Pos)
#define TIM_CCMR1_OC2FE_Pos     (10U)
#define TIM_CCMR1_OC2FE_Msk     (0x1UL << TIM_CCMR1_OC2FE_Pos)
#define TIM_CCMR1_OC2PE_Pos     (11U)
#define TIM_CCMR1_OC2PE_Msk     (0x1UL << TIM_CCMR1_OC2PE_Pos)
#define TIM_CCMR1_OC2M_Pos      (12U)
#define TIM_CCMR1_OC2M_Msk      (0x7UL << TIM_CCMR1_OC2M_Pos)
#define TIM_CCMR1_OC2CE_Pos     (15U)
#define TIM_CCMR1_OC2CE_Msk     (0x1UL << TIM_CCMR1_OC2CE_Pos)

// CCMR1 Input模式位定义
#define TIM_CCMR1_IC1PSC_Pos    (2U)
#define TIM_CCMR1_IC1PSC_Msk    (0x3UL << TIM_CCMR1_IC1PSC_Pos)
#define TIM_CCMR1_IC1F_Pos      (4U)
#define TIM_CCMR1_IC1F_Msk      (0xFUL << TIM_CCMR1_IC1F_Pos)
#define TIM_CCMR1_IC2PSC_Pos    (10U)
#define TIM_CCMR1_IC2PSC_Msk    (0x3UL << TIM_CCMR1_IC2PSC_Pos)
#define TIM_CCMR1_IC2F_Pos      (12U)
#define TIM_CCMR1_IC2F_Msk      (0xFUL << TIM_CCMR1_IC2F_Pos)

// CCER寄存器位定义
#define TIM_CCER_CC1E_Pos       (0U)
#define TIM_CCER_CC1E_Msk       (0x1UL << TIM_CCER_CC1E_Pos)
#define TIM_CCER_CC1P_Pos       (1U)
#define TIM_CCER_CC1P_Msk       (0x1UL << TIM_CCER_CC1P_Pos)
#define TIM_CCER_CC1NE_Pos      (2U)
#define TIM_CCER_CC1NE_Msk      (0x1UL << TIM_CCER_CC1NE_Pos)
#define TIM_CCER_CC1NP_Pos      (3U)
#define TIM_CCER_CC1NP_Msk      (0x1UL << TIM_CCER_CC1NP_Pos)
#define TIM_CCER_CC2E_Pos       (4U)
#define TIM_CCER_CC2E_Msk       (0x1UL << TIM_CCER_CC2E_Pos)
#define TIM_CCER_CC2P_Pos       (5U)
#define TIM_CCER_CC2P_Msk       (0x1UL << TIM_CCER_CC2P_Pos)
#define TIM_CCER_CC2NE_Pos      (6U)
#define TIM_CCER_CC2NE_Msk      (0x1UL << TIM_CCER_CC2NE_Pos)
#define TIM_CCER_CC2NP_Pos      (7U)
#define TIM_CCER_CC2NP_Msk      (0x1UL << TIM_CCER_CC2NP_Pos)
#define TIM_CCER_CC3E_Pos       (8U)
#define TIM_CCER_CC3E_Msk       (0x1UL << TIM_CCER_CC3E_Pos)
#define TIM_CCER_CC3P_Pos       (9U)
#define TIM_CCER_CC3P_Msk       (0x1UL << TIM_CCER_CC3P_Pos)
#define TIM_CCER_CC3NE_Pos      (10U)
#define TIM_CCER_CC3NE_Msk      (0x1UL << TIM_CCER_CC3NE_Pos)
#define TIM_CCER_CC3NP_Pos      (11U)
#define TIM_CCER_CC3NP_Msk      (0x1UL << TIM_CCER_CC3NP_Pos)
#define TIM_CCER_CC4E_Pos       (12U)
#define TIM_CCER_CC4E_Msk       (0x1UL << TIM_CCER_CC4E_Pos)
#define TIM_CCER_CC4P_Pos       (13U)
#define TIM_CCER_CC4P_Msk       (0x1UL << TIM_CCER_CC4P_Pos)
#define TIM_CCER_CC4NP_Pos      (15U)
#define TIM_CCER_CC4NP_Msk      (0x1UL << TIM_CCER_CC4NP_Pos)

// BDTR寄存器位定义（高级定时器）
#define TIM_BDTR_DTG_Pos        (0U)
#define TIM_BDTR_DTG_Msk        (0xFFUL << TIM_BDTR_DTG_Pos)
#define TIM_BDTR_LOCK_Pos       (8U)
#define TIM_BDTR_LOCK_Msk       (0x3UL << TIM_BDTR_LOCK_Pos)
#define TIM_BDTR_OSSI_Pos       (10U)
#define TIM_BDTR_OSSI_Msk       (0x1UL << TIM_BDTR_OSSI_Pos)
#define TIM_BDTR_OSSR_Pos       (11U)
#define TIM_BDTR_OSSR_Msk       (0x1UL << TIM_BDTR_OSSR_Pos)
#define TIM_BDTR_BKE_Pos        (12U)
#define TIM_BDTR_BKE_Msk        (0x1UL << TIM_BDTR_BKE_Pos)
#define TIM_BDTR_BKP_Pos        (13U)
#define TIM_BDTR_BKP_Msk        (0x1UL << TIM_BDTR_BKP_Pos)
#define TIM_BDTR_AOE_Pos        (14U)
#define TIM_BDTR_AOE_Msk        (0x1UL << TIM_BDTR_AOE_Pos)
#define TIM_BDTR_MOE_Pos        (15U)
#define TIM_BDTR_MOE_Msk        (0x1UL << TIM_BDTR_MOE_Pos)
#define TIM_BDTR_BKF_Pos        (16U)
#define TIM_BDTR_BKF_Msk        (0xFUL << TIM_BDTR_BKF_Pos)
#define TIM_BDTR_BK2F_Pos       (20U)
#define TIM_BDTR_BK2F_Msk       (0xFUL << TIM_BDTR_BK2F_Pos)
#define TIM_BDTR_BK2E_Pos       (24U)
#define TIM_BDTR_BK2E_Msk       (0x1UL << TIM_BDTR_BK2E_Pos)
#define TIM_BDTR_BK2P_Pos       (25U)
#define TIM_BDTR_BK2P_Msk       (0x1UL << TIM_BDTR_BK2P_Pos)

// DCR寄存器位定义
#define TIM_DCR_DBA_Pos         (0U)
#define TIM_DCR_DBA_Msk         (0x1FUL << TIM_DCR_DBA_Pos)
#define TIM_DCR_DBL_Pos         (8U)
#define TIM_DCR_DBL_Msk         (0x1FUL << TIM_DCR_DBL_Pos)

//===========================================
// 内部类型定义
//===========================================

/**
 * @brief TIM实例基地址表
 */
static void* const tim_base_table[TIM_COUNT] = {
    TIM1,   TIM8,   // 高级定时器
    TIM2,   TIM5,   // 通用定时器32位
    TIM3,   TIM4,   // 通用定时器16位全功能
    TIM9,   TIM10,  // 通用定时器16位基础
    TIM6,   TIM7,   // 基本定时器
    TIM26           // 特殊定时器
};

/**
 * @brief TIM中断编号表
 */
static const IRQn_Type tim_irq_table[TIM_COUNT] = {
    TIM1_BRK_UP_TRG_COM_IRQn,  // TIM1
    TIM8_BRK_UP_TRG_COM_IRQn,  // TIM8
    TIM2_IRQn,                 // TIM2
    TIM5_IRQn,                 // TIM5
    TIM3_IRQn,                 // TIM3
    TIM4_IRQn,                 // TIM4
    TIM9_IRQn,                 // TIM9
    TIM10_IRQn,                // TIM10
    TIM6_IRQn,                 // TIM6
    TIM7_IRQn,                 // TIM7
    TIM26_IRQn                 // TIM26
};

/**
 * @brief TIM回调函数管理器
 */
typedef struct {
    tim_callback_t    update_callback;
    tim_cc_callback_t cc_callbacks[6];  // CH1-CH6
    tim_callback_t    break_callback;
    tim_callback_t    trigger_callback;
    tim_callback_t    com_callback;
} tim_callback_manager_t;

static tim_callback_manager_t tim_callbacks[TIM_COUNT] = {0};

//===========================================
// 内部辅助函数
//===========================================

/**
 * @brief 获取TIM实例基地址
 */
static inline void* tim_get_base(tim_instance_t tim)
{
    if (tim >= TIM_COUNT) return NULL;
    return tim_base_table[tim];
}

/**
 * @brief TIM实例到时钟标识符映射表
 */
static const clock_periph_t tim_clock_table[TIM_COUNT] = {
    CLK_TIM1,   CLK_TIM8,   // 高级定时器
    CLK_TIM2,   CLK_TIM5,   // 通用定时器32位
    CLK_TIM3,   CLK_TIM4,   // 通用定时器16位全功能
    CLK_TIM9,   CLK_TIM10,  // 通用定时器16位基础
    CLK_TIM6,   CLK_TIM7,   // 基本定时器
    0                       // TIM26 无时钟标识
};

/**
 * @brief TIM实例到定时器编号映射表（用于获取时钟频率）
 * 
 * @note TIM编号与实例枚举对应关系：
 *       TIM1->1, TIM2->2, TIM3->3, ..., TIM26->26
 */
static const uint8_t tim_number_table[TIM_COUNT] = {
    1,   8,   // 高级定时器
    2,   5,   // 通用定时器32位
    3,   4,   // 通用定时器16位全功能
    9,   10,  // 通用定时器16位基础
    6,   7,   // 基本定时器
    26          // 特殊定时器
};

/**
 * @brief 使能TIM时钟
 * 
 * @param tim TIM实例
 * @return true 使能成功
 * @return false TIM实例无效
 * 
 * @note 自动根据TIM实例调用clock_periph_enable()使能对应的时钟
 */
static bool tim_enable_clock(tim_instance_t tim)
{
    if (tim >= TIM_COUNT) {
        return false;
    }
    
    // 使能TIM外设时钟
    clock_periph_enable(tim_clock_table[tim]);
    
    return true;
}

/**
 * @brief 获取TIM时钟频率
 * 
 * @param tim TIM实例
 * @return TIM时钟频率（Hz），失败返回0
 * 
 * @note TIM时钟频率规则（ACM32P4）：
 *       - 当APB预分频系数 = 1时：TIMx_CLK = PCLK
 *       - 当APB预分频系数 > 1时：TIMx_CLK = PCLK × 2
 *       
 *       TIM所在总线：
 *       - APB1: TIM2-7, TIM12-14
 *       - APB2: TIM1, TIM8-11, TIM15-25
 */
static uint32_t tim_get_clock_frequency(tim_instance_t tim)
{
    if (tim >= TIM_COUNT) {
        return 0;
    }
    
    // 获取定时器编号（1-26）
    uint8_t tim_num = tim_number_table[tim];
    
    // 调用clocks模块获取定时器时钟频率
    // clock_get_timer_hz()会自动处理APB分频倍增规则
    return clock_get_timer_hz(tim_num);
}

/**
 * @brief 读取TIM1类型寄存器（带BDTR）
 */
typedef struct {
    __IO uint32_t CR1;
    __IO uint32_t CR2;
    __IO uint32_t SMCR;
    __IO uint32_t DIER;
    __IO uint32_t SR;
    __IO uint32_t EGR;
    union {
        __IO uint32_t CCMR1_OUTPUT;
        __IO uint32_t CCMR1_INPUT;
    };
    union {
        __IO uint32_t CCMR2_OUTPUT;
        __IO uint32_t CCMR2_INPUT;
    };
    __IO uint32_t CCER;
    __IO uint32_t CNT;
    __IO uint32_t PSC;
    __IO uint32_t ARR;
    __IO uint32_t RCR;
    __IO uint32_t CCR1;
    __IO uint32_t CCR2;
    __IO uint32_t CCR3;
    __IO uint32_t CCR4;
    __IO uint32_t BDTR;
    __IO uint32_t DCR;
    __IO uint32_t DMAR;
    __IO uint32_t RESERVED0[3];
    union {
        __IO uint32_t CCMR3_OUTPUT;
        __IO uint32_t CCMR3_INPUT;
    };
    __IO uint32_t CCR5;
    __IO uint32_t CCR6;
    __IO uint32_t AF1;
    __IO uint32_t AF2;
    __IO uint32_t TISEL;
    __IO uint32_t DBER;
} TIM_Advanced_TypeDef;

/**
 * @brief 读取TIM6类型寄存器（基本定时器）
 */
typedef struct {
    __IO uint32_t CR1;
    __IO uint32_t CR2;
    __IO uint32_t RESERVED0;
    __IO uint32_t DIER;
    __IO uint32_t SR;
    __IO uint32_t EGR;
    __IO uint32_t RESERVED1[3];
    __IO uint32_t CNT;
    __IO uint32_t PSC;
    __IO uint32_t ARR;
} TIM_Basic_TypeDef;

//===========================================
// 能力查询函数
//===========================================

bool tim_is_32bit(tim_instance_t tim)
{
    return TIM_IS_32BIT(tim);
}

bool tim_is_advanced(tim_instance_t tim)
{
    return TIM_IS_ADVANCED(tim);
}

bool tim_has_complementary_output(tim_instance_t tim)
{
    return TIM_HAS_COMPLEMENTARY(tim);
}

bool tim_has_break(tim_instance_t tim)
{
    return TIM_HAS_BREAK(tim);
}

bool tim_has_repetition_counter(tim_instance_t tim)
{
    return TIM_HAS_REPETITION(tim);
}

bool tim_has_encoder(tim_instance_t tim)
{
    return TIM_HAS_ENCODER(tim);
}

uint8_t tim_get_channel_count(tim_instance_t tim)
{
    return TIM_GET_CHANNEL_COUNT(tim);
}

//===========================================
// 辅助计算函数
//===========================================

bool tim_calculate_frequency(uint32_t tim_clock, uint32_t target_freq,
                              uint16_t *prescaler, uint32_t *period, bool is_32bit)
{
    if (target_freq == 0 || prescaler == NULL || period == NULL) {
        return false;
    }
    
    // 计算最优PSC和ARR组合
    uint32_t max_period = is_32bit ? 0xFFFFFFFF : 0xFFFF;
    uint64_t target_counts = (uint64_t)tim_clock / target_freq;
    
    // 尝试PSC从0开始
    for (uint32_t psc = 0; psc <= 65535; psc++) {
        uint32_t arr = (uint32_t)(target_counts / (psc + 1)) - 1;
        if (arr <= max_period && arr > 0) {
            *prescaler = (uint16_t)psc;
            *period = arr;
            return true;
        }
    }
    
    return false;
}

bool tim_calculate_period_us(uint32_t tim_clock, uint32_t period_us,
                              uint16_t *prescaler, uint32_t *period, bool is_32bit)
{
    if (period_us == 0 || prescaler == NULL || period == NULL) {
        return false;
    }
    
    // 转换为频率计算
    uint32_t freq = 1000000 / period_us;
    return tim_calculate_frequency(tim_clock, freq, prescaler, period, is_32bit);
}

//===========================================
// 第4层：控制与查询 API
//===========================================

bool tim_start(tim_instance_t tim)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 使能计数器：CR1.CEN = 1
    tim_base->CR1 |= TIM_CR1_CEN_Msk;
    return true;
}

bool tim_stop(tim_instance_t tim)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 禁用计数器：CR1.CEN = 0
    tim_base->CR1 &= ~TIM_CR1_CEN_Msk;
    return true;
}

bool tim_enable_main_output(tim_instance_t tim)
{
    if (!TIM_HAS_BREAK(tim)) return false;
    
    TIM_Advanced_TypeDef *tim_base = (TIM_Advanced_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 使能主输出：BDTR.MOE = 1
    tim_base->BDTR |= TIM_BDTR_MOE_Msk;
    return true;
}

bool tim_disable_main_output(tim_instance_t tim)
{
    if (!TIM_HAS_BREAK(tim)) return false;
    
    TIM_Advanced_TypeDef *tim_base = (TIM_Advanced_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 禁用主输出：BDTR.MOE = 0
    tim_base->BDTR &= ~TIM_BDTR_MOE_Msk;
    return true;
}

bool tim_start_pwm(tim_instance_t tim, tim_channel_t channel)
{
    if (channel >= TIM_CH5) return false;  // 只支持CH1-CH4
    
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 使能通道：CCER.CCxE = 1
    tim_base->CCER |= (1 << (channel * 4));
    
    // 如果是高级定时器，使能主输出
    if (TIM_HAS_BREAK(tim)) {
        tim_enable_main_output(tim);
    }
    
    // 启动计数器
    return tim_start(tim);
}

bool tim_stop_pwm(tim_instance_t tim, tim_channel_t channel)
{
    if (channel >= TIM_CH5) return false;
    
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 禁用通道：CCER.CCxE = 0
    tim_base->CCER &= ~(1 << (channel * 4));
    return true;
}

bool tim_enable_channel(tim_instance_t tim, tim_channel_t channel, bool enable)
{
    if (channel >= TIM_CH5) return false;
    
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 设置CCER.CCxE
    if (enable) {
        tim_base->CCER |= (1 << (channel * 4));
    } else {
        tim_base->CCER &= ~(1 << (channel * 4));
    }
    return true;
}

bool tim_enable_complementary_channel(tim_instance_t tim, tim_channel_t channel, bool enable)
{
    if (!TIM_HAS_COMPLEMENTARY(tim) || channel >= TIM_CH4) return false;
    
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 设置CCER.CCxNE
    if (enable) {
        tim_base->CCER |= (TIM_CCER_CC1NE_Msk << (channel * 4));
    } else {
        tim_base->CCER &= ~(TIM_CCER_CC1NE_Msk << (channel * 4));
    }
    return true;
}

bool tim_set_counter(tim_instance_t tim, uint32_t value)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 写CNT寄存器
    tim_base->CNT = value;
    return true;
}

uint32_t tim_get_counter(tim_instance_t tim)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return 0;
    
    // 读CNT寄存器
    return tim_base->CNT;
}

bool tim_set_autoreload(tim_instance_t tim, uint32_t value)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 写ARR寄存器
    tim_base->ARR = value;
    return true;
}

uint32_t tim_get_autoreload(tim_instance_t tim)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return 0;
    
    // 读ARR寄存器
    return tim_base->ARR;
}

bool tim_set_prescaler(tim_instance_t tim, uint16_t prescaler)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 写PSC寄存器
    tim_base->PSC = prescaler;
    return true;
}

uint16_t tim_get_prescaler(tim_instance_t tim)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return 0;
    
    // 读PSC寄存器
    return (uint16_t)tim_base->PSC;
}

bool tim_set_compare(tim_instance_t tim, tim_channel_t channel, uint32_t value)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL || channel >= TIM_CH5) return false;
    
    // 写CCRx寄存器
    switch (channel) {
        case TIM_CH1: tim_base->CCR1 = value; break;
        case TIM_CH2: tim_base->CCR2 = value; break;
        case TIM_CH3: tim_base->CCR3 = value; break;
        case TIM_CH4: tim_base->CCR4 = value; break;
        default: return false;
    }
    return true;
}

uint32_t tim_get_compare(tim_instance_t tim, tim_channel_t channel)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL || channel >= TIM_CH5) return 0;
    
    // 读CCRx寄存器
    switch (channel) {
        case TIM_CH1: return tim_base->CCR1;
        case TIM_CH2: return tim_base->CCR2;
        case TIM_CH3: return tim_base->CCR3;
        case TIM_CH4: return tim_base->CCR4;
        default: return 0;
    }
}

uint32_t tim_get_capture(tim_instance_t tim, tim_channel_t channel)
{
    // 捕获值和比较值使用同一个寄存器
    return tim_get_compare(tim, channel);
}

bool tim_set_duty(tim_instance_t tim, tim_channel_t channel, uint8_t duty_percent)
{
    if (duty_percent > 100) return false;
    
    uint32_t arr = tim_get_autoreload(tim);
    uint32_t ccr = (arr + 1) * duty_percent / 100;
    
    return tim_set_compare(tim, channel, ccr);
}

uint8_t tim_get_duty(tim_instance_t tim, tim_channel_t channel)
{
    uint32_t arr = tim_get_autoreload(tim);
    uint32_t ccr = tim_get_compare(tim, channel);
    
    if (arr == 0) return 0;
    return (uint8_t)((ccr * 100) / (arr + 1));
}

bool tim_set_pwm_frequency(tim_instance_t tim, uint32_t frequency_hz)
{
    uint32_t tim_clock = tim_get_clock_frequency(tim);
    uint16_t psc;
    uint32_t arr;
    
    // 保存当前占空比
    uint8_t duty_ch1 = tim_get_duty(tim, TIM_CH1);
    
    // 计算新的PSC和ARR
    if (!tim_calculate_frequency(tim_clock, frequency_hz, &psc, &arr, tim_is_32bit(tim))) {
        return false;
    }
    
    // 设置新值
    tim_set_prescaler(tim, psc);
    tim_set_autoreload(tim, arr);
    
    // 恢复占空比
    tim_set_duty(tim, TIM_CH1, duty_ch1);
    
    // 生成更新事件
    tim_generate_update_event(tim);
    
    return true;
}

uint32_t tim_get_pwm_frequency(tim_instance_t tim)
{
    uint32_t tim_clock = tim_get_clock_frequency(tim);
    uint32_t psc = tim_get_prescaler(tim);
    uint32_t arr = tim_get_autoreload(tim);
    
    if (arr == 0) return 0;
    
    return tim_clock / ((psc + 1) * (arr + 1));
}

bool tim_enable_interrupt(tim_instance_t tim, uint32_t interrupt)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 设置DIER中的中断使能位
    tim_base->DIER |= interrupt;
    
    // 使能NVIC中断
    NVIC_EnableIRQ(tim_irq_table[tim]);
    
    return true;
}

bool tim_disable_interrupt(tim_instance_t tim, uint32_t interrupt)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 清除DIER中的中断使能位
    tim_base->DIER &= ~interrupt;
    return true;
}

bool tim_get_interrupt_status(tim_instance_t tim, uint32_t interrupt)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 检查SR寄存器
    return (tim_base->SR & interrupt) != 0;
}

bool tim_clear_interrupt_flag(tim_instance_t tim, uint32_t interrupt)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 写0清除SR寄存器标志
    tim_base->SR &= ~interrupt;
    return true;
}

bool tim_enable_dma_request(tim_instance_t tim, uint32_t dma_request)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 设置DIER中的DMA使能位
    tim_base->DIER |= dma_request;
    return true;
}

bool tim_disable_dma_request(tim_instance_t tim, uint32_t dma_request)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 清除DIER中的DMA使能位
    tim_base->DIER &= ~dma_request;
    return true;
}

volatile uint32_t* tim_get_dma_burst_address(tim_instance_t tim)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return NULL;
    
    // 返回DMAR寄存器地址
    return &(tim_base->DMAR);
}

bool tim_generate_update_event(tim_instance_t tim)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 设置EGR.UG
    tim_base->EGR = TIM_EGR_UG_Msk;
    return true;
}

bool tim_generate_cc_event(tim_instance_t tim, tim_channel_t channel)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL || channel >= TIM_CH5) return false;
    
    // 设置EGR.CCxG
    tim_base->EGR = (1 << (channel + 1));
    return true;
}

bool tim_generate_com_event(tim_instance_t tim)
{
    if (!TIM_IS_ADVANCED(tim)) return false;
    
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 设置EGR.COMG
    tim_base->EGR = TIM_EGR_COMG_Msk;
    return true;
}

bool tim_generate_trigger_event(tim_instance_t tim)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 设置EGR.TG
    tim_base->EGR = TIM_EGR_TG_Msk;
    return true;
}

bool tim_generate_break_event(tim_instance_t tim, uint8_t break_input)
{
    if (!TIM_HAS_BREAK(tim) || break_input < 1 || break_input > 2) return false;
    
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 设置EGR.BG或B2G
    if (break_input == 1) {
        tim_base->EGR = TIM_EGR_BG_Msk;
    } else {
        // B2G位在EGR[8]
        tim_base->EGR = (1 << 8);
    }
    return true;
}

bool tim_is_running(tim_instance_t tim)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 检查CR1.CEN
    return (tim_base->CR1 & TIM_CR1_CEN_Msk) != 0;
}

bool tim_get_direction(tim_instance_t tim)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 检查CR1.DIR
    return (tim_base->CR1 & TIM_CR1_DIR_Msk) != 0;
}

bool tim_get_capture_overflow_flag(tim_instance_t tim, tim_channel_t channel)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL || channel >= TIM_CH5) return false;
    
    // 检查SR.CCxOF
    return (tim_base->SR & (TIM_SR_CC1OF_Msk << channel)) != 0;
}

bool tim_clear_capture_overflow_flag(tim_instance_t tim, tim_channel_t channel)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL || channel >= TIM_CH5) return false;
    
    // 清除SR.CCxOF
    tim_base->SR &= ~(TIM_SR_CC1OF_Msk << channel);
    return true;
}

bool tim_get_break_flag(tim_instance_t tim, uint8_t break_input)
{
    if (!TIM_HAS_BREAK(tim) || break_input < 1 || break_input > 2) return false;
    
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 检查SR.BIF或B2IF
    if (break_input == 1) {
        return (tim_base->SR & TIM_SR_BIF_Msk) != 0;
    } else {
        return (tim_base->SR & (1 << 20)) != 0;  // B2IF在SR[20]
    }
}

bool tim_clear_break_flag(tim_instance_t tim, uint8_t break_input)
{
    if (!TIM_HAS_BREAK(tim) || break_input < 1 || break_input > 2) return false;
    
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 清除SR.BIF或B2IF
    if (break_input == 1) {
        tim_base->SR &= ~TIM_SR_BIF_Msk;
    } else {
        tim_base->SR &= ~(1 << 20);
    }
    return true;
}

//===========================================
// 第3层：高级功能 API
//===========================================

bool tim_set_dead_time(tim_instance_t tim, uint8_t dead_time)
{
    if (!TIM_HAS_COMPLEMENTARY(tim)) return false;
    
    TIM_Advanced_TypeDef *tim_base = (TIM_Advanced_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 设置BDTR.DTG
    uint32_t bdtr = tim_base->BDTR;
    bdtr &= ~TIM_BDTR_DTG_Msk;
    bdtr |= (dead_time << TIM_BDTR_DTG_Pos);
    tim_base->BDTR = bdtr;
    
    return true;
}

bool tim_set_repetition_counter(tim_instance_t tim, uint8_t repetition)
{
    if (!TIM_HAS_REPETITION(tim)) return false;
    
    TIM_Advanced_TypeDef *tim_base = (TIM_Advanced_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 写RCR寄存器
    tim_base->RCR = repetition;
    return true;
}

bool tim_set_lock_level(tim_instance_t tim, tim_lock_level_t lock_level)
{
    if (!TIM_HAS_BREAK(tim)) return false;
    
    TIM_Advanced_TypeDef *tim_base = (TIM_Advanced_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 设置BDTR.LOCK
    uint32_t bdtr = tim_base->BDTR;
    bdtr &= ~TIM_BDTR_LOCK_Msk;
    bdtr |= (lock_level << TIM_BDTR_LOCK_Pos);
    tim_base->BDTR = bdtr;
    
    return true;
}

bool tim_enable_autoreload_preload(tim_instance_t tim, bool enable)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 设置CR1.ARPE
    if (enable) {
        tim_base->CR1 |= TIM_CR1_ARPE_Msk;
    } else {
        tim_base->CR1 &= ~TIM_CR1_ARPE_Msk;
    }
    return true;
}

bool tim_enable_one_pulse_mode(tim_instance_t tim, bool enable)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 设置CR1.OPM
    if (enable) {
        tim_base->CR1 |= TIM_CR1_OPM_Msk;
    } else {
        tim_base->CR1 &= ~TIM_CR1_OPM_Msk;
    }
    return true;
}

bool tim_enable_update_event(tim_instance_t tim, bool enable)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 设置CR1.UDIS (注意：UDIS=1时禁用更新事件)
    if (enable) {
        tim_base->CR1 &= ~TIM_CR1_UDIS_Msk;
    } else {
        tim_base->CR1 |= TIM_CR1_UDIS_Msk;
    }
    return true;
}

bool tim_set_update_request_source(tim_instance_t tim, bool only_overflow)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 设置CR1.URS
    if (only_overflow) {
        tim_base->CR1 |= TIM_CR1_URS_Msk;
    } else {
        tim_base->CR1 &= ~TIM_CR1_URS_Msk;
    }
    return true;
}

bool tim_set_ti1_selection(tim_instance_t tim, bool xor_enable)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 设置CR2.TI1S
    if (xor_enable) {
        tim_base->CR2 |= TIM_CR2_TI1S_Msk;
    } else {
        tim_base->CR2 &= ~TIM_CR2_TI1S_Msk;
    }
    return true;
}

bool tim_set_output_idle_state(tim_instance_t tim, tim_channel_t channel,
                                tim_idle_state_t idle_state, tim_idle_state_t nidle_state)
{
    if (!TIM_IS_ADVANCED(tim) || channel >= TIM_CH5) return false;
    
    TIM_Advanced_TypeDef *tim_base = (TIM_Advanced_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 设置CR2.OISx和OISxN
    uint32_t cr2 = tim_base->CR2;
    uint32_t ois_pos = TIM_CR2_OIS1_Pos + (channel * 2);
    uint32_t oisn_pos = TIM_CR2_OIS1N_Pos + (channel * 2);
    
    if (idle_state == TIM_IDLE_SET) {
        cr2 |= (1 << ois_pos);
    } else {
        cr2 &= ~(1 << ois_pos);
    }
    
    if (nidle_state == TIM_IDLE_SET) {
        cr2 |= (1 << oisn_pos);
    } else {
        cr2 &= ~(1 << oisn_pos);
    }
    
    tim_base->CR2 = cr2;
    return true;
}

bool tim_set_ocref_clear(tim_instance_t tim, tim_channel_t channel, bool enable)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL || channel >= TIM_CH5) return false;
    
    // 设置CCMRx.OCxCE
    if (channel < TIM_CH3) {
        // CCMR1
        uint32_t shift = (channel == TIM_CH1) ? 7 : 15;
        if (enable) {
            tim_base->CCMR1_OUTPUT |= (1 << shift);
        } else {
            tim_base->CCMR1_OUTPUT &= ~(1 << shift);
        }
    } else {
        // CCMR2
        uint32_t shift = (channel == TIM_CH3) ? 7 : 15;
        if (enable) {
            tim_base->CCMR2_OUTPUT |= (1 << shift);
        } else {
            tim_base->CCMR2_OUTPUT &= ~(1 << shift);
        }
    }
    return true;
}

bool tim_set_input_filter(tim_instance_t tim, tim_channel_t channel, uint8_t filter)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL || channel >= TIM_CH5 || filter > 15) return false;
    
    // 设置CCMRx.ICxF
    if (channel < TIM_CH3) {
        // CCMR1
        uint32_t shift = (channel == TIM_CH1) ? TIM_CCMR1_IC1F_Pos : TIM_CCMR1_IC2F_Pos;
        uint32_t mask = (channel == TIM_CH1) ? TIM_CCMR1_IC1F_Msk : TIM_CCMR1_IC2F_Msk;
        tim_base->CCMR1_INPUT &= ~mask;
        tim_base->CCMR1_INPUT |= (filter << shift);
    } else {
        // CCMR2
        uint32_t shift = (channel == TIM_CH3) ? 4 : 12;
        uint32_t mask = 0xF << shift;
        tim_base->CCMR2_INPUT &= ~mask;
        tim_base->CCMR2_INPUT |= (filter << shift);
    }
    return true;
}

bool tim_set_input_prescaler(tim_instance_t tim, tim_channel_t channel,
                              tim_ic_prescaler_t prescaler)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL || channel >= TIM_CH5) return false;
    
    // 设置CCMRx.ICxPSC
    if (channel < TIM_CH3) {
        uint32_t shift = (channel == TIM_CH1) ? TIM_CCMR1_IC1PSC_Pos : TIM_CCMR1_IC2PSC_Pos;
        uint32_t mask = (channel == TIM_CH1) ? TIM_CCMR1_IC1PSC_Msk : TIM_CCMR1_IC2PSC_Msk;
        tim_base->CCMR1_INPUT &= ~mask;
        tim_base->CCMR1_INPUT |= (prescaler << shift);
    } else {
        uint32_t shift = (channel == TIM_CH3) ? 2 : 10;
        uint32_t mask = 0x3 << shift;
        tim_base->CCMR2_INPUT &= ~mask;
        tim_base->CCMR2_INPUT |= (prescaler << shift);
    }
    return true;
}

bool tim_set_channel_input_selection(tim_instance_t tim, tim_channel_t channel,
                                      tim_ic_selection_t selection)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL || channel >= TIM_CH5) return false;
    
    // 设置CCMRx.CCxS
    if (channel < TIM_CH3) {
        uint32_t shift = (channel == TIM_CH1) ? TIM_CCMR1_CC1S_Pos : TIM_CCMR1_CC2S_Pos;
        uint32_t mask = (channel == TIM_CH1) ? TIM_CCMR1_CC1S_Msk : TIM_CCMR1_CC2S_Msk;
        tim_base->CCMR1_INPUT &= ~mask;
        tim_base->CCMR1_INPUT |= (selection << shift);
    } else {
        uint32_t shift = (channel == TIM_CH3) ? 0 : 8;
        uint32_t mask = 0x3 << shift;
        tim_base->CCMR2_INPUT &= ~mask;
        tim_base->CCMR2_INPUT |= (selection << shift);
    }
    return true;
}

bool tim_set_channel_remap(tim_instance_t tim, tim_channel_t channel, uint8_t remap)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL || channel >= TIM_CH5) return false;
    
    // 设置TISEL寄存器
    uint32_t shift = channel * 8;
    uint32_t mask = 0xFF << shift;
    tim_base->TISEL &= ~mask;
    tim_base->TISEL |= (remap << shift);
    
    return true;
}

bool tim_config_break_input_source(tim_instance_t tim, uint8_t break_input, uint32_t source_mask)
{
    if (!TIM_HAS_BREAK(tim) || break_input < 1 || break_input > 2) return false;
    
    TIM_Advanced_TypeDef *tim_base = (TIM_Advanced_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 设置AF1或AF2寄存器
    if (break_input == 1) {
        tim_base->AF1 = source_mask;
    } else {
        tim_base->AF2 = source_mask;
    }
    
    return true;
}

bool tim_enable_break_bidirectional(tim_instance_t tim, uint8_t break_input, bool enable)
{
    if (!TIM_HAS_BREAK(tim) || break_input < 1 || break_input > 2) return false;
    
    TIM_Advanced_TypeDef *tim_base = (TIM_Advanced_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 设置AF1.BKBID或AF2.BK2BID
    if (break_input == 1) {
        if (enable) {
            tim_base->AF1 |= (1 << 28);  // BKBID位
        } else {
            tim_base->AF1 &= ~(1 << 28);
        }
    } else {
        if (enable) {
            tim_base->AF2 |= (1 << 28);  // BK2BID位
        } else {
            tim_base->AF2 &= ~(1 << 28);
        }
    }
    
    return true;
}

bool tim_set_break_input_disable(tim_instance_t tim, uint8_t break_input, uint32_t disable_mask)
{
    if (!TIM_HAS_BREAK(tim) || break_input < 1 || break_input > 2) return false;
    
    TIM_Advanced_TypeDef *tim_base = (TIM_Advanced_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 设置AF1或AF2的禁用位
    if (break_input == 1) {
        tim_base->AF1 |= disable_mask;
    } else {
        tim_base->AF2 |= disable_mask;
    }
    
    return true;
}

bool tim_enable_external_clock_mode2(tim_instance_t tim, bool enable)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 设置SMCR.ECE
    if (enable) {
        tim_base->SMCR |= TIM_SMCR_ECE_Msk;
    } else {
        tim_base->SMCR &= ~TIM_SMCR_ECE_Msk;
    }
    return true;
}

bool tim_config_external_trigger(tim_instance_t tim, const tim_etr_config_t *config)
{
    if (config == NULL) return false;
    
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 配置SMCR.ETP, ETPS, ETF
    uint32_t smcr = tim_base->SMCR;
    smcr &= ~(TIM_SMCR_ETP_Msk | TIM_SMCR_ETPS_Msk | TIM_SMCR_ETF_Msk);
    smcr |= (config->polarity << TIM_SMCR_ETP_Pos);
    smcr |= (config->prescaler << TIM_SMCR_ETPS_Pos);
    smcr |= (config->filter << TIM_SMCR_ETF_Pos);
    tim_base->SMCR = smcr;
    
    return true;
}

bool tim_config_dma_burst(tim_instance_t tim, const tim_dma_burst_config_t *config)
{
    if (config == NULL) return false;
    
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 配置DCR.DBA和DBL
    uint32_t dcr = 0;
    dcr |= (config->base_address << TIM_DCR_DBA_Pos);
    dcr |= (config->burst_length << TIM_DCR_DBL_Pos);
    tim_base->DCR = dcr;
    
    return true;
}

bool tim_config_break(tim_instance_t tim, const tim_break_config_t *config)
{
    if (!TIM_HAS_BREAK(tim) || config == NULL) return false;

    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 步骤1：先配置CR1寄存器的刹车滤波器 (bits 13:10)
    uint32_t cr1 = tim_base->CR1;
    cr1 &= ~(0xF << 10);  // 清除BKF位
    cr1 |= (config->brk_filter << 10);
    tim_base->CR1 = cr1;
    
    // 步骤2：配置AF1寄存器：刹车源使能和极性
    // 极性计算公式：
    // - BRK引脚最终极性 = BKINP XOR BKP
    // - 比较器x最终极性 = (BKINP XOR BKP) XOR BKCMP_xP
    // 策略：以BRK引脚极性为基准，BKINP=0, BKP=brk_pin_polarity
    //      比较器极性通过 BKCMP_xP 相对于基准极性调整
    
    uint32_t af1 = tim_base->AF1 & ~0x3F1F;  // 清除bits[0-4,9-13]
    
    // 刹车源使能 (bits 0-4: BKINE, BKCMP1E-4E)
    if (config->brk_pin_enable)  af1 |= (1 << 0);
    if (config->comp1_enable)    af1 |= (1 << 1);
    if (config->comp2_enable)    af1 |= (1 << 2);
    if (config->comp3_enable)    af1 |= (1 << 3);
    if (config->comp4_enable)    af1 |= (1 << 4);
    
    // 刹车源极性配置 (bits 9-13: BKINP, BKCMP1P-4P)
    // BKINP保持为0（bit 9）
    // 比较器极性：如果与BRK引脚极性不同，需要置位BKCMP_xP
    uint8_t base_pol = config->brk_pin_polarity;  // 基准极性
    if (config->comp1_polarity != base_pol)  af1 |= (1 << 10);
    if (config->comp2_polarity != base_pol)  af1 |= (1 << 11);
    if (config->comp3_polarity != base_pol)  af1 |= (1 << 12);
    if (config->comp4_polarity != base_pol)  af1 |= (1 << 13);
    
    tim_base->AF1 = af1;
    
    // 步骤3：最后配置BDTR寄存器（包含LOCK位，一旦写入将冻结其他寄存器）
    uint32_t bdtr = tim_base->BDTR & 0xFF;  // 保留死区时间设置(bits 7:0)
    
    // Off-state选择
    if (config->off_state_idle == TIM_OFF_STATE_OIS) {
        bdtr |= TIM_BDTR_OSSI_Msk;  // bit 10: 输出OIS电平
    }
    if (config->off_state_run == TIM_OFF_STATE_OIS) {
        bdtr |= TIM_BDTR_OSSR_Msk;  // bit 11: 输出OIS电平
    }
    
    // 刹车功能使能 (bit 12: BKE)
    if (config->brk_pin_enable || config->comp1_enable || config->comp2_enable ||
        config->comp3_enable || config->comp4_enable) {
        bdtr |= TIM_BDTR_BKE_Msk;
    }
    
    // BKP设置为BRK引脚的期望极性 (bit 13: BKP)
    // 最终：BRK引脚极性 = BKINP XOR BKP = 0 XOR BKP = BKP
    //      比较器x极性 = (BKINP XOR BKP) XOR BKCMP_xP = BKP XOR BKCMP_xP
    if (config->brk_pin_polarity == TIM_BREAK_POL_HIGH) {
        bdtr |= TIM_BDTR_BKP_Msk;
    }
    
    // 自动输出使能 (bit 14)
    if (config->automatic_output) {
        bdtr |= TIM_BDTR_AOE_Msk;
    }
    
    // 锁定级别 (bits 9:8) - 最后配置，写入后将冻结DTG、BKE、BKP、AOE、AF1、CR1[BKF]等
    bdtr |= (config->lock_level << TIM_BDTR_LOCK_Pos);
    
    // 写入BDTR寄存器（包含LOCK位，必须在所有其他寄存器配置完成后）
    tim_base->BDTR = bdtr;
    
    return true;
}

bool tim_config_complementary_output(tim_instance_t tim, tim_channel_t channel,
                                      const tim_complementary_config_t *config)
{
    if (!TIM_HAS_COMPLEMENTARY(tim) || channel >= TIM_CH4 || config == NULL) return false;
    
    TIM_Advanced_TypeDef *tim_base = (TIM_Advanced_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 配置死区时间
    tim_set_dead_time(tim, config->dead_time);
    
    // 配置互补输出极性
    uint32_t ccer = tim_base->CCER;
    if (config->n_polarity == TIM_POLARITY_LOW) {
        ccer |= (TIM_CCER_CC1NP_Msk << (channel * 4));
    } else {
        ccer &= ~(TIM_CCER_CC1NP_Msk << (channel * 4));
    }
    tim_base->CCER = ccer;
    
    // 配置空闲状态
    tim_set_output_idle_state(tim, channel, config->idle_state, config->nidle_state);
    
    // 配置自动输出
    if (config->automatic_output) {
        tim_base->BDTR |= TIM_BDTR_AOE_Msk;
    }
    
    return true;
}

//===========================================
// 第2层：基础配置 API
//===========================================

bool tim_config_timebase(tim_instance_t tim, const tim_timebase_config_t *config)
{
    if (config == NULL) return false;
    
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 使能时钟
    tim_enable_clock(tim);
    
    // 配置PSC
    tim_base->PSC = config->prescaler;
    
    // 配置ARR
    tim_base->ARR = config->period;
    
    // 配置计数模式和时钟分频
    uint32_t cr1 = tim_base->CR1;
    cr1 &= ~(TIM_CR1_CMS_Msk | TIM_CR1_DIR_Msk | TIM_CR1_CKD_Msk);
    
    // 计数模式
    if (config->count_mode == TIM_COUNT_DOWN) {
        cr1 |= TIM_CR1_DIR_Msk;
    } else if (config->count_mode >= TIM_COUNT_CENTER_1) {
        uint32_t cms = config->count_mode - TIM_COUNT_CENTER_1 + 1;
        cr1 |= (cms << TIM_CR1_CMS_Pos);
    }
    
    // 时钟分频
    cr1 |= (config->clock_division << TIM_CR1_CKD_Pos);
    tim_base->CR1 = cr1;
    
    // 配置重复计数器（如果支持）
    if (TIM_HAS_REPETITION(tim)) {
        TIM_Advanced_TypeDef *adv_base = (TIM_Advanced_TypeDef*)tim_base;
        adv_base->RCR = config->repetition_counter;
    }
    
    // 生成更新事件以加载配置
    tim_generate_update_event(tim);
    
    return true;
}

bool tim_config_channel_pwm(tim_instance_t tim, tim_channel_t channel,
                             const tim_pwm_config_t *config)
{
    if (config == NULL || channel >= TIM_CH5) return false;
    
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 配置输出比较模式 (CCMRx)
    if (channel < TIM_CH3) {
        // CCMR1
        uint32_t shift = (channel == TIM_CH1) ? 4 : 12;
        uint32_t ccmr1 = tim_base->CCMR1_OUTPUT;
        
        // 清除CCxS (配置为输出)
        ccmr1 &= ~(0x3 << (shift - 4));
        
        // OCxM (输出比较模式)
        ccmr1 &= ~(0x7 << shift);
        ccmr1 |= (config->mode << shift);
        
        // OCxPE (预装载)
        if (config->preload) {
            ccmr1 |= (1 << (shift - 1));
        } else {
            ccmr1 &= ~(1 << (shift - 1));
        }
        
        // OCxFE (快速模式)
        if (config->fast_mode) {
            ccmr1 |= (1 << (shift - 2));
        } else {
            ccmr1 &= ~(1 << (shift - 2));
        }
        
        tim_base->CCMR1_OUTPUT = ccmr1;
    } else {
        // CCMR2 (类似处理)
        uint32_t shift = (channel == TIM_CH3) ? 4 : 12;
        uint32_t ccmr2 = tim_base->CCMR2_OUTPUT;
        ccmr2 &= ~(0x3 << (shift - 4));
        ccmr2 &= ~(0x7 << shift);
        ccmr2 |= (config->mode << shift);
        if (config->preload) ccmr2 |= (1 << (shift - 1));
        if (config->fast_mode) ccmr2 |= (1 << (shift - 2));
        tim_base->CCMR2_OUTPUT = ccmr2;
    }
    
    // 配置输出极性 (CCER)
    uint32_t ccer = tim_base->CCER;
    uint32_t ch_shift = channel * 4;
    
    // CCxP (主输出极性)
    if (config->polarity == TIM_POLARITY_LOW) {
        ccer |= (TIM_CCER_CC1P_Msk << ch_shift);
    } else {
        ccer &= ~(TIM_CCER_CC1P_Msk << ch_shift);
    }
    
    // CCxNP (互补输出极性)
    if (TIM_HAS_COMPLEMENTARY(tim) && channel < TIM_CH4) {
        if (config->npolarity == TIM_POLARITY_LOW) {
            ccer |= (TIM_CCER_CC1NP_Msk << ch_shift);
        } else {
            ccer &= ~(TIM_CCER_CC1NP_Msk << ch_shift);
        }
    }
    
    tim_base->CCER = ccer;
    
    // 设置脉宽值
    tim_set_compare(tim, channel, config->pulse);
    
    // 配置空闲状态（高级定时器）
    if (TIM_IS_ADVANCED(tim)) {
        tim_set_output_idle_state(tim, channel, config->idle_state, config->nidle_state);
    }
    
    return true;
}

bool tim_config_channel_input_capture(tim_instance_t tim, tim_channel_t channel,
                                       const tim_ic_config_t *config)
{
    if (config == NULL || channel >= TIM_CH5) return false;
    
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 配置输入捕获模式 (CCMRx)
    if (channel < TIM_CH3) {
        uint32_t shift_base = (channel == TIM_CH1) ? 0 : 8;
        uint32_t ccmr1 = tim_base->CCMR1_INPUT;
        
        // CCxS (输入选择)
        ccmr1 &= ~(0x3 << shift_base);
        ccmr1 |= (config->selection << shift_base);
        
        // ICxPSC (输入预分频)
        ccmr1 &= ~(0x3 << (shift_base + 2));
        ccmr1 |= (config->prescaler << (shift_base + 2));
        
        // ICxF (输入滤波器)
        ccmr1 &= ~(0xF << (shift_base + 4));
        ccmr1 |= (config->filter << (shift_base + 4));
        
        tim_base->CCMR1_INPUT = ccmr1;
    } else {
        uint32_t shift_base = (channel == TIM_CH3) ? 0 : 8;
        uint32_t ccmr2 = tim_base->CCMR2_INPUT;
        ccmr2 &= ~(0x3 << shift_base);
        ccmr2 |= (config->selection << shift_base);
        ccmr2 &= ~(0x3 << (shift_base + 2));
        ccmr2 |= (config->prescaler << (shift_base + 2));
        ccmr2 &= ~(0xF << (shift_base + 4));
        ccmr2 |= (config->filter << (shift_base + 4));
        tim_base->CCMR2_INPUT = ccmr2;
    }
    
    // 配置捕获极性 (CCER)
    uint32_t ccer = tim_base->CCER;
    uint32_t ch_shift = channel * 4;
    
    // 清除CCxP和CCxNP
    ccer &= ~((TIM_CCER_CC1P_Msk | TIM_CCER_CC1NP_Msk) << ch_shift);
    
    // 设置极性
    if (config->polarity == TIM_IC_FALLING) {
        ccer |= (TIM_CCER_CC1P_Msk << ch_shift);
    } else if (config->polarity == TIM_IC_BOTH) {
        ccer |= ((TIM_CCER_CC1P_Msk | TIM_CCER_CC1NP_Msk) << ch_shift);
    }
    
    tim_base->CCER = ccer;
    
    return true;
}

bool tim_config_channel_output_compare(tim_instance_t tim, tim_channel_t channel,
                                        const tim_oc_config_t *config)
{
    // 输出比较配置与PWM类似
    tim_pwm_config_t pwm_config = {
        .mode = config->mode,
        .pulse = config->pulse,
        .polarity = config->polarity,
        .npolarity = config->npolarity,
        .idle_state = config->idle_state,
        .nidle_state = config->nidle_state,
        .fast_mode = config->fast_mode,
        .preload = config->preload
    };
    
    return tim_config_channel_pwm(tim, channel, &pwm_config);
}

bool tim_config_encoder(tim_instance_t tim, const tim_encoder_config_t *config)
{
    if (!TIM_HAS_ENCODER(tim) || config == NULL) return false;
    
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 配置TI1
    tim_ic_config_t ic1_config = {
        .polarity = config->ic1_polarity,
        .selection = config->ic1_selection,
        .prescaler = config->ic1_prescaler,
        .filter = config->ic1_filter
    };
    tim_config_channel_input_capture(tim, TIM_CH1, &ic1_config);
    
    // 配置TI2
    tim_ic_config_t ic2_config = {
        .polarity = config->ic2_polarity,
        .selection = config->ic2_selection,
        .prescaler = config->ic2_prescaler,
        .filter = config->ic2_filter
    };
    tim_config_channel_input_capture(tim, TIM_CH2, &ic2_config);
    
    // 配置编码器模式 (SMCR.SMS)
    uint32_t smcr = tim_base->SMCR;
    smcr &= ~TIM_SMCR_SMS_Msk;
    smcr |= (config->mode << TIM_SMCR_SMS_Pos);
    tim_base->SMCR = smcr;
    
    return true;
}

bool tim_config_one_pulse_mode(tim_instance_t tim, const tim_opm_config_t *config)
{
    if (config == NULL) return false;
    
    // 配置通道1为输出比较
    tim_oc_config_t oc_config = {
        .mode = config->oc_mode,
        .pulse = config->pulse,
        .polarity = config->oc_polarity,
        .preload = true
    };
    tim_config_channel_output_compare(tim, TIM_CH1, &oc_config);
    
    // 配置通道2为输入捕获
    tim_ic_config_t ic_config = {
        .polarity = config->ic_polarity,
        .selection = config->ic_selection,
        .prescaler = TIM_IC_PSC_DIV1,
        .filter = config->ic_filter
    };
    tim_config_channel_input_capture(tim, TIM_CH2, &ic_config);
    
    // 使能单脉冲模式
    tim_enable_one_pulse_mode(tim, true);
    
    return true;
}

bool tim_config_master_slave(tim_instance_t tim, const tim_master_slave_config_t *config)
{
    if (config == NULL) return false;
    
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return false;
    
    // 配置SMCR
    uint32_t smcr = tim_base->SMCR;
    
    // SMS (从模式)
    smcr &= ~TIM_SMCR_SMS_Msk;
    smcr |= (config->slave_mode << TIM_SMCR_SMS_Pos);
    
    // TS (触发源)
    smcr &= ~TIM_SMCR_TS_Msk;
    smcr |= (config->trigger_source << TIM_SMCR_TS_Pos);
    
    // MSM (主从模式)
    if (config->msm_enable) {
        smcr |= TIM_SMCR_MSM_Msk;
    } else {
        smcr &= ~TIM_SMCR_MSM_Msk;
    }
    
    tim_base->SMCR = smcr;
    
    // 配置CR2 (主模式)
    uint32_t cr2 = tim_base->CR2;
    
    // MMS
    cr2 &= ~TIM_CR2_MMS_Msk;
    cr2 |= (config->master_mode << TIM_CR2_MMS_Pos);
    
    // MMS2 (仅高级定时器)
    if (TIM_IS_ADVANCED(tim)) {
        cr2 &= ~TIM_CR2_MMS2_Msk;
        cr2 |= (config->master_mode2 << TIM_CR2_MMS2_Pos);
    }
    
    tim_base->CR2 = cr2;
    
    return true;
}

bool tim_config_hall_sensor(tim_instance_t tim, const tim_hall_sensor_config_t *config)
{
    if (config == NULL) return false;
    
    // 使能TI1 XOR功能
    tim_set_ti1_selection(tim, true);
    
    // 配置TI1捕获
    tim_ic_config_t ic_config = {
        .polarity = config->ic1_polarity,
        .selection = TIM_IC_SELECT_TRC,
        .prescaler = config->ic1_prescaler,
        .filter = config->ic1_filter
    };
    tim_config_channel_input_capture(tim, TIM_CH1, &ic_config);
    
    // 配置从模式为复位模式
    tim_master_slave_config_t ms_config = {
        .slave_mode = TIM_SLAVE_RESET,
        .trigger_source = TIM_TS_TI1F_ED,
        .master_mode = TIM_TRGO_OC2REF,
        .msm_enable = false
    };
    tim_config_master_slave(tim, &ms_config);
    
    // 设置换向延迟
    tim_set_compare(tim, TIM_CH2, config->commutation_delay);
    
    return true;
}

//===========================================
// 第1层：快速初始化 API
//===========================================

bool tim_init_pwm(tim_instance_t tim, tim_channel_t channel,
                  uint32_t frequency_hz, uint8_t duty_percent)
{
    if (channel >= TIM_CH5 || duty_percent > 100) return false;
    
    // 使能时钟
    tim_enable_clock(tim);
    
    // 计算PSC和ARR
    uint32_t tim_clock = tim_get_clock_frequency(tim);
    uint16_t psc;
    uint32_t arr;
    if (!tim_calculate_frequency(tim_clock, frequency_hz, &psc, &arr, tim_is_32bit(tim))) {
        return false;
    }
    
    // 配置时基
    tim_timebase_config_t timebase = {
        .prescaler = psc,
        .period = arr,
        .count_mode = TIM_COUNT_UP,
        .clock_division = TIM_CKD_DIV1,
        .repetition_counter = 0
    };
    tim_config_timebase(tim, &timebase);
    
    // 配置PWM
    uint32_t pulse = (arr + 1) * duty_percent / 100;
    tim_pwm_config_t pwm = {
        .mode = TIM_OC_PWM1,
        .pulse = pulse,
        .polarity = TIM_POLARITY_HIGH,
        .npolarity = TIM_POLARITY_HIGH,
        .idle_state = TIM_IDLE_RESET,
        .nidle_state = TIM_IDLE_RESET,
        .fast_mode = false,
        .preload = true
    };
    tim_config_channel_pwm(tim, channel, &pwm);
    
    // 启动PWM
    tim_start_pwm(tim, channel);
    
    return true;
}

bool tim_init_input_capture(tim_instance_t tim, tim_channel_t channel,
                             tim_ic_polarity_t polarity, uint8_t filter)
{
    if (channel >= TIM_CH5) return false;
    
    // 使能时钟
    tim_enable_clock(tim);
    
    // 配置时基（最大计数范围）
    tim_timebase_config_t timebase = {
        .prescaler = 0,
        .period = tim_is_32bit(tim) ? 0xFFFFFFFF : 0xFFFF,
        .count_mode = TIM_COUNT_UP,
        .clock_division = TIM_CKD_DIV1,
        .repetition_counter = 0
    };
    tim_config_timebase(tim, &timebase);
    
    // 配置输入捕获
    tim_ic_config_t ic = {
        .polarity = polarity,
        .selection = TIM_IC_SELECT_DIRECT,
        .prescaler = TIM_IC_PSC_DIV1,
        .filter = filter
    };
    tim_config_channel_input_capture(tim, channel, &ic);
    
    // 使能通道
    tim_enable_channel(tim, channel, true);
    
    // 启动定时器
    tim_start(tim);
    
    return true;
}

bool tim_init_encoder(tim_instance_t tim, tim_encoder_mode_t mode)
{
    if (!TIM_HAS_ENCODER(tim)) return false;
    
    // 使能时钟
    tim_enable_clock(tim);
    
    // 配置编码器
    tim_encoder_config_t enc = {
        .mode = mode,
        .ic1_polarity = TIM_IC_RISING,
        .ic1_selection = TIM_IC_SELECT_DIRECT,
        .ic1_prescaler = TIM_IC_PSC_DIV1,
        .ic1_filter = 8,
        .ic2_polarity = TIM_IC_RISING,
        .ic2_selection = TIM_IC_SELECT_DIRECT,
        .ic2_prescaler = TIM_IC_PSC_DIV1,
        .ic2_filter = 8
    };
    tim_config_encoder(tim, &enc);
    
    // 设置最大计数值
    tim_set_autoreload(tim, tim_is_32bit(tim) ? 0xFFFFFFFF : 0xFFFF);
    
    // 启动定时器
    tim_start(tim);
    
    return true;
}

bool tim_init_periodic(tim_instance_t tim, uint32_t period_us)
{
    // 使能时钟
    tim_enable_clock(tim);
    
    // 计算PSC和ARR
    uint32_t tim_clock = tim_get_clock_frequency(tim);
    uint16_t psc;
    uint32_t arr;
    if (!tim_calculate_period_us(tim_clock, period_us, &psc, &arr, tim_is_32bit(tim))) {
        return false;
    }
    
    // 配置时基
    tim_timebase_config_t timebase = {
        .prescaler = psc,
        .period = arr,
        .count_mode = TIM_COUNT_UP,
        .clock_division = TIM_CKD_DIV1,
        .repetition_counter = 0
    };
    tim_config_timebase(tim, &timebase);
    
    // 使能更新中断
    tim_enable_interrupt(tim, TIM_INT_UPDATE);
    
    return true;
}

//===========================================
// 回调函数注册
//===========================================

bool tim_register_update_callback(tim_instance_t tim, tim_callback_t callback)
{
    if (tim >= TIM_COUNT) return false;
    tim_callbacks[tim].update_callback = callback;
    return true;
}

bool tim_register_cc_callback(tim_instance_t tim, tim_channel_t channel,
                               tim_cc_callback_t callback)
{
    if (tim >= TIM_COUNT || channel >= TIM_CH6) return false;
    tim_callbacks[tim].cc_callbacks[channel] = callback;
    return true;
}

bool tim_register_break_callback(tim_instance_t tim, tim_callback_t callback)
{
    if (tim >= TIM_COUNT) return false;
    tim_callbacks[tim].break_callback = callback;
    return true;
}

bool tim_register_trigger_callback(tim_instance_t tim, tim_callback_t callback)
{
    if (tim >= TIM_COUNT) return false;
    tim_callbacks[tim].trigger_callback = callback;
    return true;
}

bool tim_register_com_callback(tim_instance_t tim, tim_callback_t callback)
{
    if (tim >= TIM_COUNT) return false;
    tim_callbacks[tim].com_callback = callback;
    return true;
}

//===========================================
// 中断处理函数
//===========================================

/**
 * @brief TIM通用中断处理函数
 * @param tim TIM实例
 * @note 根据定时器类型处理不同的中断
 */
static void tim_irq_handler_common(tim_instance_t tim)
{
    TIM1_TypeDef *tim_base = (TIM1_TypeDef*)tim_get_base(tim);
    if (tim_base == NULL) return;
    
    uint32_t sr = tim_base->SR;
    uint32_t dier = tim_base->DIER;
    
    // 更新中断（所有定时器都支持）
    if ((sr & TIM_SR_UIF_Msk) && (dier & TIM_DIER_UIE_Msk)) {
        tim_clear_interrupt_flag(tim, TIM_INT_UPDATE);
        if (tim_callbacks[tim].update_callback) {
            tim_callbacks[tim].update_callback(tim);
        }
    }
    
    // 根据定时器类型处理其他中断
    switch (tim) {
        // 高级定时器：TIM1/8
        case TIM_1:
        case TIM_8:
            // 捕获/比较中断（4个通道）
            for (uint8_t ch = 0; ch < 4; ch++) {
                uint32_t cc_flag = TIM_SR_CC1IF_Msk << ch;
                uint32_t cc_ie = TIM_DIER_CC1IE_Msk << ch;
                if ((sr & cc_flag) && (dier & cc_ie)) {
                    tim_clear_interrupt_flag(tim, cc_flag);
                    if (tim_callbacks[tim].cc_callbacks[ch]) {
                        tim_callbacks[tim].cc_callbacks[ch](tim, (tim_channel_t)ch);
                    }
                }
            }
            // 刹车中断
            if ((sr & TIM_SR_BIF_Msk) && (dier & TIM_DIER_BIE_Msk)) {
                tim_clear_interrupt_flag(tim, TIM_INT_BREAK);
                if (tim_callbacks[tim].break_callback) {
                    tim_callbacks[tim].break_callback(tim);
                }
            }
            // 触发中断
            if ((sr & TIM_SR_TIF_Msk) && (dier & TIM_DIER_TIE_Msk)) {
                tim_clear_interrupt_flag(tim, TIM_INT_TRIGGER);
                if (tim_callbacks[tim].trigger_callback) {
                    tim_callbacks[tim].trigger_callback(tim);
                }
            }
            // COM中断
            if ((sr & TIM_SR_COMIF_Msk) && (dier & TIM_DIER_COMIE_Msk)) {
                tim_clear_interrupt_flag(tim, TIM_INT_COM);
                if (tim_callbacks[tim].com_callback) {
                    tim_callbacks[tim].com_callback(tim);
                }
            }
            break;
            
        // 通用定时器（32位4通道）：TIM2/5
        case TIM_2:
        case TIM_5:
        // 通用定时器（16位4通道）：TIM3/4
        case TIM_3:
        case TIM_4:
            // 捕获/比较中断（4个通道）
            for (uint8_t ch = 0; ch < 4; ch++) {
                uint32_t cc_flag = TIM_SR_CC1IF_Msk << ch;
                uint32_t cc_ie = TIM_DIER_CC1IE_Msk << ch;
                if ((sr & cc_flag) && (dier & cc_ie)) {
                    tim_clear_interrupt_flag(tim, cc_flag);
                    if (tim_callbacks[tim].cc_callbacks[ch]) {
                        tim_callbacks[tim].cc_callbacks[ch](tim, (tim_channel_t)ch);
                    }
                }
            }
            // 触发中断
            if ((sr & TIM_SR_TIF_Msk) && (dier & TIM_DIER_TIE_Msk)) {
                tim_clear_interrupt_flag(tim, TIM_INT_TRIGGER);
                if (tim_callbacks[tim].trigger_callback) {
                    tim_callbacks[tim].trigger_callback(tim);
                }
            }
            break;
            
        // 通用定时器（2通道）：TIM9
        case TIM_9:
            // 捕获/比较中断（2个通道）
            for (uint8_t ch = 0; ch < 2; ch++) {
                uint32_t cc_flag = TIM_SR_CC1IF_Msk << ch;
                uint32_t cc_ie = TIM_DIER_CC1IE_Msk << ch;
                if ((sr & cc_flag) && (dier & cc_ie)) {
                    tim_clear_interrupt_flag(tim, cc_flag);
                    if (tim_callbacks[tim].cc_callbacks[ch]) {
                        tim_callbacks[tim].cc_callbacks[ch](tim, (tim_channel_t)ch);
                    }
                }
            }
            // 触发中断
            if ((sr & TIM_SR_TIF_Msk) && (dier & TIM_DIER_TIE_Msk)) {
                tim_clear_interrupt_flag(tim, TIM_INT_TRIGGER);
                if (tim_callbacks[tim].trigger_callback) {
                    tim_callbacks[tim].trigger_callback(tim);
                }
            }
            break;
            
        // 通用定时器（1通道）：TIM10
        case TIM_10:
            // 捕获/比较中断（1个通道）
            if ((sr & TIM_SR_CC1IF_Msk) && (dier & TIM_DIER_CC1IE_Msk)) {
                tim_clear_interrupt_flag(tim, TIM_SR_CC1IF_Msk);
                if (tim_callbacks[tim].cc_callbacks[0]) {
                    tim_callbacks[tim].cc_callbacks[0](tim, TIM_CH1);
                }
            }
            // 触发中断
            if ((sr & TIM_SR_TIF_Msk) && (dier & TIM_DIER_TIE_Msk)) {
                tim_clear_interrupt_flag(tim, TIM_INT_TRIGGER);
                if (tim_callbacks[tim].trigger_callback) {
                    tim_callbacks[tim].trigger_callback(tim);
                }
            }
            break;
            
        // 基本定时器：TIM6/7（只有更新中断，已在上面处理）
        case TIM_6:
        case TIM_7:
            // 只有更新中断，已处理
            break;
            
        default:
            break;
    }
}

//===========================================
// 中断服务函数
//===========================================

/**
 * @brief TIM1刹车、更新、触发、COM中断服务函数
 */
void TIM1_BRK_UP_TRG_COM_IRQHandler(void)
{
    tim_irq_handler_common(TIM_1);
}

/**
 * @brief TIM1捕获/比较中断服务函数
 */
void TIM1_CC_IRQHandler(void)
{
    tim_irq_handler_common(TIM_1);
}

/**
 * @brief TIM2中断服务函数
 */
void TIM2_IRQHandler(void)
{
    tim_irq_handler_common(TIM_2);
}

/**
 * @brief TIM3中断服务函数
 */
void TIM3_IRQHandler(void)
{
    tim_irq_handler_common(TIM_3);
}

/**
 * @brief TIM4中断服务函数
 */
void TIM4_IRQHandler(void)
{
    tim_irq_handler_common(TIM_4);
}

/**
 * @brief TIM5中断服务函数
 */
void TIM5_IRQHandler(void)
{
    tim_irq_handler_common(TIM_5);
}

/**
 * @brief TIM6中断服务函数
 */
void TIM6_IRQHandler(void)
{
    tim_irq_handler_common(TIM_6);
}

/**
 * @brief TIM7中断服务函数
 */
void TIM7_IRQHandler(void)
{
    tim_irq_handler_common(TIM_7);
}

/**
 * @brief TIM8刹车、更新、触发、COM中断服务函数
 */
void TIM8_BRK_UP_TRG_COM_IRQHandler(void)
{
    tim_irq_handler_common(TIM_8);
}

/**
 * @brief TIM8捕获/比较中断服务函数
 */
void TIM8_CC_IRQHandler(void)
{
    tim_irq_handler_common(TIM_8);
}

/**
 * @brief TIM9中断服务函数
 */
void TIM9_IRQHandler(void)
{
    tim_irq_handler_common(TIM_9);
}

/**
 * @brief TIM10中断服务函数
 */
void TIM10_IRQHandler(void)
{
    tim_irq_handler_common(TIM_10);
}
