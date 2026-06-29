/**
 * @file dac.c
 * @brief ACM32P4 DAC驱动实现
 *
 * 本文件实现了ACM32P4系列芯片的数模转换器驱动功能，支持：
 * - DAC1/DAC2独立模式
 * - 双通道独立或同步输出
 * - LFSR噪声 / 三角波 / 锯齿波生成
 * - DMA单数据和双数据模式
 * - 外部触发和软件触发
 * - 采样保持模式
 * - BUFFER偏差校准
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#include "hardware/dac.h"
#include "hardware/clocks.h"
#include "device/acm32p4xx.h"
#include <assert.h>

//===========================================
// 1. 寄存器位定义
//===========================================

// ---- DAC_CR 控制寄存器 ----
#define DAC_CR_EN1_Pos              (0U)
#define DAC_CR_EN1_Msk              (0x1UL << DAC_CR_EN1_Pos)
#define DAC_CR_TEN1_Pos             (1U)
#define DAC_CR_TEN1_Msk             (0x1UL << DAC_CR_TEN1_Pos)
#define DAC_CR_TSEL1_Pos            (2U)
#define DAC_CR_TSEL1_Msk            (0xFUL << DAC_CR_TSEL1_Pos)
#define DAC_CR_WAVE1_Pos            (6U)
#define DAC_CR_WAVE1_Msk            (0x3UL << DAC_CR_WAVE1_Pos)
#define DAC_CR_MAMP1_Pos            (8U)
#define DAC_CR_MAMP1_Msk            (0xFUL << DAC_CR_MAMP1_Pos)
#define DAC_CR_DMAEN1_Pos           (12U)
#define DAC_CR_DMAEN1_Msk           (0x1UL << DAC_CR_DMAEN1_Pos)
#define DAC_CR_DMAUDIE1_Pos         (13U)
#define DAC_CR_DMAUDIE1_Msk         (0x1UL << DAC_CR_DMAUDIE1_Pos)
#define DAC_CR_CEN1_Pos             (14U)
#define DAC_CR_CEN1_Msk             (0x1UL << DAC_CR_CEN1_Pos)
#define DAC_CR_EN2_Pos              (16U)
#define DAC_CR_EN2_Msk              (0x1UL << DAC_CR_EN2_Pos)
#define DAC_CR_TEN2_Pos             (17U)
#define DAC_CR_TEN2_Msk             (0x1UL << DAC_CR_TEN2_Pos)
#define DAC_CR_TSEL2_Pos            (18U)
#define DAC_CR_TSEL2_Msk            (0xFUL << DAC_CR_TSEL2_Pos)
#define DAC_CR_WAVE2_Pos            (22U)
#define DAC_CR_WAVE2_Msk            (0x3UL << DAC_CR_WAVE2_Pos)
#define DAC_CR_MAMP2_Pos            (24U)
#define DAC_CR_MAMP2_Msk            (0xFUL << DAC_CR_MAMP2_Pos)
#define DAC_CR_DMAEN2_Pos           (28U)
#define DAC_CR_DMAEN2_Msk           (0x1UL << DAC_CR_DMAEN2_Pos)
#define DAC_CR_DMAUDIE2_Pos         (29U)
#define DAC_CR_DMAUDIE2_Msk         (0x1UL << DAC_CR_DMAUDIE2_Pos)
#define DAC_CR_CEN2_Pos             (30U)
#define DAC_CR_CEN2_Msk             (0x1UL << DAC_CR_CEN2_Pos)

// ---- DAC_SR 状态寄存器 ----
#define DAC_SR_SAMOV1_Pos           (8U)
#define DAC_SR_SAMOV1_Msk           (0x1UL << DAC_SR_SAMOV1_Pos)
#define DAC_SR_DORSTAT1_Pos         (12U)
#define DAC_SR_DORSTAT1_Msk         (0x1UL << DAC_SR_DORSTAT1_Pos)
#define DAC_SR_DMAUDR1_Pos          (13U)
#define DAC_SR_DMAUDR1_Msk          (0x1UL << DAC_SR_DMAUDR1_Pos)
#define DAC_SR_CAL_FLAG1_Pos        (14U)
#define DAC_SR_CAL_FLAG1_Msk        (0x1UL << DAC_SR_CAL_FLAG1_Pos)
#define DAC_SR_SAMOV2_Pos           (24U)
#define DAC_SR_SAMOV2_Msk           (0x1UL << DAC_SR_SAMOV2_Pos)
#define DAC_SR_DORSTAT2_Pos         (28U)
#define DAC_SR_DORSTAT2_Msk         (0x1UL << DAC_SR_DORSTAT2_Pos)
#define DAC_SR_DMAUDR2_Pos          (29U)
#define DAC_SR_DMAUDR2_Msk          (0x1UL << DAC_SR_DMAUDR2_Pos)
#define DAC_SR_CAL_FLAG2_Pos        (30U)
#define DAC_SR_CAL_FLAG2_Msk        (0x1UL << DAC_SR_CAL_FLAG2_Pos)

// ---- DAC_CCR 校准控制寄存器 ----
#define DAC_CCR_OTRIM1_Pos          (0U)
#define DAC_CCR_OTRIM1_Msk          (0x1FUL << DAC_CCR_OTRIM1_Pos)
#define DAC_CCR_OTRIM2_Pos          (16U)
#define DAC_CCR_OTRIM2_Msk          (0x1FUL << DAC_CCR_OTRIM2_Pos)

// ---- DAC_MCR 模式控制寄存器 ----
#define DAC_MCR_MODE1_Pos           (0U)
#define DAC_MCR_MODE1_Msk           (0x7UL << DAC_MCR_MODE1_Pos)
#define DAC_MCR_DMADOUBLE1_Pos      (8U)
#define DAC_MCR_DMADOUBLE1_Msk      (0x1UL << DAC_MCR_DMADOUBLE1_Pos)
#define DAC_MCR_SINFORMAT1_Pos      (9U)
#define DAC_MCR_SINFORMAT1_Msk      (0x1UL << DAC_MCR_SINFORMAT1_Pos)
#define DAC_MCR_MODE2_Pos           (16U)
#define DAC_MCR_MODE2_Msk           (0x7UL << DAC_MCR_MODE2_Pos)
#define DAC_MCR_DMADOUBLE2_Pos      (24U)
#define DAC_MCR_DMADOUBLE2_Msk      (0x1UL << DAC_MCR_DMADOUBLE2_Pos)
#define DAC_MCR_SINFORMAT2_Pos      (25U)
#define DAC_MCR_SINFORMAT2_Msk      (0x1UL << DAC_MCR_SINFORMAT2_Pos)

// ---- DAC_SHSRx 采样时间寄存器 ----
#define DAC_SHSR_TSAMPLE_Pos        (0U)
#define DAC_SHSR_TSAMPLE_Msk        (0x3FFUL << DAC_SHSR_TSAMPLE_Pos)

// ---- DAC_SHHR 保持时间寄存器 ----
#define DAC_SHHR_THOLD1_Pos         (0U)
#define DAC_SHHR_THOLD1_Msk         (0x3FFUL << DAC_SHHR_THOLD1_Pos)
#define DAC_SHHR_THOLD2_Pos         (16U)
#define DAC_SHHR_THOLD2_Msk         (0x3FFUL << DAC_SHHR_THOLD2_Pos)

// ---- DAC_SHRR 刷新时间寄存器 ----
#define DAC_SHRR_TREFRESH1_Pos      (0U)
#define DAC_SHRR_TREFRESH1_Msk      (0xFFUL << DAC_SHRR_TREFRESH1_Pos)
#define DAC_SHRR_TREFRESH2_Pos      (16U)
#define DAC_SHRR_TREFRESH2_Msk      (0xFFUL << DAC_SHRR_TREFRESH2_Pos)

// ---- DAC_STRx 锯齿波寄存器 ----
#define DAC_STR_STRSTDATA_Pos       (0U)
#define DAC_STR_STRSTDATA_Msk       (0xFFFUL << DAC_STR_STRSTDATA_Pos)
#define DAC_STR_STDIR_Pos           (12U)
#define DAC_STR_STDIR_Msk           (0x1UL << DAC_STR_STDIR_Pos)
#define DAC_STR_STINCDATA_Pos       (16U)
#define DAC_STR_STINCDATA_Msk       (0xFFFFUL << DAC_STR_STINCDATA_Pos)

// ---- DAC_STMODR 锯齿波模式寄存器 ----
#define DAC_STMODR_STRSTTRIGSEL1_Pos   (0U)
#define DAC_STMODR_STRSTTRIGSEL1_Msk   (0xFUL << DAC_STMODR_STRSTTRIGSEL1_Pos)
#define DAC_STMODR_STINCTRIGSEL1_Pos   (8U)
#define DAC_STMODR_STINCTRIGSEL1_Msk   (0xFUL << DAC_STMODR_STINCTRIGSEL1_Pos)
#define DAC_STMODR_STRSTTRIGSEL2_Pos   (16U)
#define DAC_STMODR_STRSTTRIGSEL2_Msk   (0xFUL << DAC_STMODR_STRSTTRIGSEL2_Pos)
#define DAC_STMODR_STINCTRIGSEL2_Pos   (24U)
#define DAC_STMODR_STINCTRIGSEL2_Msk   (0xFUL << DAC_STMODR_STINCTRIGSEL2_Pos)

// ---- DAC_SWTRIGR 软件触发寄存器 ----
#define DAC_SWTRIGR_SWTRIG1_Pos     (0U)
#define DAC_SWTRIGR_SWTRIG1_Msk     (0x1UL << DAC_SWTRIGR_SWTRIG1_Pos)
#define DAC_SWTRIGR_SWTRIG2_Pos     (1U)
#define DAC_SWTRIGR_SWTRIG2_Msk     (0x1UL << DAC_SWTRIGR_SWTRIG2_Pos)
#define DAC_SWTRIGR_SWTRIGB1_Pos    (16U)
#define DAC_SWTRIGR_SWTRIGB1_Msk    (0x1UL << DAC_SWTRIGR_SWTRIGB1_Pos)
#define DAC_SWTRIGR_SWTRIGB2_Pos    (17U)
#define DAC_SWTRIGR_SWTRIGB2_Msk    (0x1UL << DAC_SWTRIGR_SWTRIGB2_Pos)

//===========================================
// 2. 内部数据表
//===========================================

/** @brief DAC基地址表（按dac_instance_t顺序） */
static DAC_TypeDef * const dac_base_table[DAC_COUNT] = {
    DAC1,
};

/** @brief DAC IRQ号表 */
static const IRQn_Type dac_irq_table[DAC_COUNT] = {
    DAC_IRQn,
};

/** @brief DAC时钟外设ID表 */
static const clock_periph_t dac_clock_table[DAC_COUNT] = {
    CLK_DAC1,
};

//===========================================
// 3. 内部状态变量
//===========================================

/**
 * @brief DAC回调管理器
 */
typedef struct {
    dac_callback_t dma_underrun_callback;  ///< DMA下溢回调（两个通道共用）
} dac_callback_manager_t;

static dac_callback_manager_t dac_callback_manager[DAC_COUNT];

/** @brief DAC校准超时计数值 */
#define DAC_CAL_TIMEOUT  10000U

/** @brief DAC通道位偏移表（用于访问CR寄存器中通道对应的位域） */
static const uint32_t dac_ch_cr_shift[2] = { 0U, 16U };

//===========================================
// 4. 内部辅助函数
//===========================================

/**
 * @brief 获取DAC实例的寄存器基地址
 */
static inline DAC_TypeDef *dac_get_base(dac_instance_t dac)
{
    return dac_base_table[dac];
}

/**
 * @brief 计算通道在CR寄存器中的位偏移（通道1=0, 通道2=16）
 */
static inline uint32_t dac_ch_shift(dac_channel_t channel)
{
    return dac_ch_cr_shift[channel];
}

//===========================================
// 第1层：快速初始化 API
//===========================================

bool dac_init_channel_basic(dac_instance_t dac, dac_channel_t channel,
                             dac_data_align_t align)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);

    DAC_TypeDef *dac_reg = dac_get_base(dac);
    uint32_t shift = dac_ch_shift(channel);

    // 使能DAC时钟
    clock_periph_enable(dac_clock_table[dac]);

    // 配置输出模式为正常模式（Buffer使能，输出到PAD）
    uint32_t mode_mask = (channel == DAC_CHANNEL_1) ? DAC_MCR_MODE1_Msk : DAC_MCR_MODE2_Msk;
    dac_reg->MCR = (dac_reg->MCR & ~mode_mask)
                 | (((uint32_t)DAC_MODE_NORMAL_BUF_PAD) << ((channel == DAC_CHANNEL_1)
                    ? DAC_MCR_MODE1_Pos : DAC_MCR_MODE2_Pos));

    // 使能通道
    dac_reg->CR |= (DAC_CR_EN1_Msk << shift);

    (void)align;
    return true;
}

bool dac_init_channel_triggered(dac_instance_t dac, dac_channel_t channel,
                                 dac_trigger_source_t trigger, dac_data_align_t align)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);
    assert(trigger <= DAC_TRIG_TIM3_TRGO);

    DAC_TypeDef *dac_reg = dac_get_base(dac);
    uint32_t shift = dac_ch_shift(channel);

    // 使能DAC时钟
    clock_periph_enable(dac_clock_table[dac]);

    // 配置输出模式
    uint32_t mode_mask = (channel == DAC_CHANNEL_1) ? DAC_MCR_MODE1_Msk : DAC_MCR_MODE2_Msk;
    uint32_t mode_pos = (channel == DAC_CHANNEL_1) ? DAC_MCR_MODE1_Pos : DAC_MCR_MODE2_Pos;
    dac_reg->MCR = (dac_reg->MCR & ~mode_mask)
                 | (((uint32_t)DAC_MODE_NORMAL_BUF_PAD) << mode_pos);

    // 配置触发源和触发使能
    dac_reg->CR = (dac_reg->CR & ~(DAC_CR_TSEL1_Msk << shift))
                | (((uint32_t)trigger << DAC_CR_TSEL1_Pos) << shift)
                | (DAC_CR_TEN1_Msk << shift);

    // 使能通道
    dac_reg->CR |= (DAC_CR_EN1_Msk << shift);

    (void)align;
    return true;
}

bool dac_init_dual_channel(dac_instance_t dac, dac_data_align_t align)
{
    assert(dac < DAC_COUNT);

    DAC_TypeDef *dac_reg = dac_get_base(dac);

    // 使能DAC时钟
    clock_periph_enable(dac_clock_table[dac]);

    // 配置两个通道的输出模式
    dac_reg->MCR = (dac_reg->MCR & ~(DAC_MCR_MODE1_Msk | DAC_MCR_MODE2_Msk))
                 | (((uint32_t)DAC_MODE_NORMAL_BUF_PAD) << DAC_MCR_MODE1_Pos)
                 | (((uint32_t)DAC_MODE_NORMAL_BUF_PAD) << DAC_MCR_MODE2_Pos);

    // 使能两个通道
    dac_reg->CR |= DAC_CR_EN1_Msk | DAC_CR_EN2_Msk;

    (void)align;
    return true;
}

//===========================================
// 第2层：基础配置 API
//===========================================

bool dac_config_channel(dac_instance_t dac, dac_channel_t channel,
                         const dac_channel_config_t *config)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);
    assert(config != NULL);

    DAC_TypeDef *dac_reg = dac_get_base(dac);
    uint32_t shift = dac_ch_shift(channel);

    // --- CR寄存器：TENx, TSELx, WAVEx, MAMPx, DMAENx ---
    uint32_t cr_val = dac_reg->CR;
    uint32_t cr_mask = (DAC_CR_TEN1_Msk | DAC_CR_TSEL1_Msk | DAC_CR_WAVE1_Msk
                        | DAC_CR_MAMP1_Msk | DAC_CR_DMAEN1_Msk) << shift;

    uint32_t cr_new = 0;
    if (config->trigger_enable) {
        cr_new |= (DAC_CR_TEN1_Msk << shift);
    }
    cr_new |= (((uint32_t)config->trigger_source << DAC_CR_TSEL1_Pos) << shift);
    cr_new |= (((uint32_t)config->waveform << DAC_CR_WAVE1_Pos) << shift);
    cr_new |= (((uint32_t)config->mamp << DAC_CR_MAMP1_Pos) << shift);
    if (config->dma_enable) {
        cr_new |= (DAC_CR_DMAEN1_Msk << shift);
    }

    dac_reg->CR = (cr_val & ~cr_mask) | cr_new;

    // --- MCR寄存器：MODEx, DMADOUBLEx, SINFORMATx ---
    uint32_t mcr_mask;
    uint32_t mcr_new_val;
    if (channel == DAC_CHANNEL_1) {
        mcr_mask = DAC_MCR_MODE1_Msk | DAC_MCR_DMADOUBLE1_Msk | DAC_MCR_SINFORMAT1_Msk;
        mcr_new_val = (((uint32_t)config->output_mode << DAC_MCR_MODE1_Pos)
                     | ((uint32_t)config->dma_double_data << DAC_MCR_DMADOUBLE1_Pos)
                     | ((uint32_t)config->signed_format << DAC_MCR_SINFORMAT1_Pos));
    } else {
        mcr_mask = DAC_MCR_MODE2_Msk | DAC_MCR_DMADOUBLE2_Msk | DAC_MCR_SINFORMAT2_Msk;
        mcr_new_val = (((uint32_t)config->output_mode << DAC_MCR_MODE2_Pos)
                     | ((uint32_t)config->dma_double_data << DAC_MCR_DMADOUBLE2_Pos)
                     | ((uint32_t)config->signed_format << DAC_MCR_SINFORMAT2_Pos));
    }
    dac_reg->MCR = (dac_reg->MCR & ~mcr_mask) | mcr_new_val;

    return true;
}

bool dac_config_trigger(dac_instance_t dac, dac_channel_t channel,
                         dac_trigger_source_t trigger, bool enable)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);
    assert(trigger <= DAC_TRIG_TIM3_TRGO);

    DAC_TypeDef *dac_reg = dac_get_base(dac);
    uint32_t shift = dac_ch_shift(channel);

    uint32_t mask = (DAC_CR_TEN1_Msk | DAC_CR_TSEL1_Msk) << shift;
    uint32_t val = ((uint32_t)trigger << DAC_CR_TSEL1_Pos) << shift;
    if (enable) {
        val |= (DAC_CR_TEN1_Msk << shift);
    }

    dac_reg->CR = (dac_reg->CR & ~mask) | val;
    return true;
}

bool dac_config_waveform(dac_instance_t dac, dac_channel_t channel,
                          dac_waveform_t waveform, dac_mamp_t mamp)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);
    assert(waveform <= DAC_WAVE_SAWTOOTH);
    assert(mamp <= DAC_MAMP_4095);

    DAC_TypeDef *dac_reg = dac_get_base(dac);
    uint32_t shift = dac_ch_shift(channel);

    uint32_t mask = (DAC_CR_WAVE1_Msk | DAC_CR_MAMP1_Msk) << shift;
    uint32_t val = (((uint32_t)waveform << DAC_CR_WAVE1_Pos)
                  | ((uint32_t)mamp << DAC_CR_MAMP1_Pos)) << shift;

    dac_reg->CR = (dac_reg->CR & ~mask) | val;
    return true;
}

bool dac_config_output_mode(dac_instance_t dac, dac_channel_t channel,
                             dac_output_mode_t mode)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);
    assert(mode <= DAC_MODE_SH_NOBUF_OPA);

    DAC_TypeDef *dac_reg = dac_get_base(dac);

    if (channel == DAC_CHANNEL_1) {
        dac_reg->MCR = (dac_reg->MCR & ~DAC_MCR_MODE1_Msk)
                     | ((uint32_t)mode << DAC_MCR_MODE1_Pos);
    } else {
        dac_reg->MCR = (dac_reg->MCR & ~DAC_MCR_MODE2_Msk)
                     | ((uint32_t)mode << DAC_MCR_MODE2_Pos);
    }
    return true;
}

bool dac_config_signed_format(dac_instance_t dac, dac_channel_t channel,
                               dac_signed_format_t format)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);

    DAC_TypeDef *dac_reg = dac_get_base(dac);

    if (channel == DAC_CHANNEL_1) {
        dac_reg->MCR = (dac_reg->MCR & ~DAC_MCR_SINFORMAT1_Msk)
                     | ((uint32_t)format << DAC_MCR_SINFORMAT1_Pos);
    } else {
        dac_reg->MCR = (dac_reg->MCR & ~DAC_MCR_SINFORMAT2_Msk)
                     | ((uint32_t)format << DAC_MCR_SINFORMAT2_Pos);
    }
    return true;
}

//===========================================
// 第3层：高级功能 API
//===========================================

bool dac_config_sawtooth(dac_instance_t dac, dac_channel_t channel,
                          const dac_sawtooth_config_t *config)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);
    assert(config != NULL);

    DAC_TypeDef *dac_reg = dac_get_base(dac);

    // 配置STRx寄存器
    uint32_t str_val = ((uint32_t)(config->reset_value & 0xFFF) << DAC_STR_STRSTDATA_Pos)
                     | ((uint32_t)config->direction << DAC_STR_STDIR_Pos)
                     | ((uint32_t)config->increment << DAC_STR_STINCDATA_Pos);

    if (channel == DAC_CHANNEL_1) {
        dac_reg->STR1 = str_val;

        // 配置STMODR寄存器
        dac_reg->STMODR = (dac_reg->STMODR
                          & ~(DAC_STMODR_STRSTTRIGSEL1_Msk | DAC_STMODR_STINCTRIGSEL1_Msk))
                        | ((uint32_t)config->reset_trigger << DAC_STMODR_STRSTTRIGSEL1_Pos)
                        | ((uint32_t)config->inc_trigger << DAC_STMODR_STINCTRIGSEL1_Pos);
    } else {
        dac_reg->STR2 = str_val;

        dac_reg->STMODR = (dac_reg->STMODR
                          & ~(DAC_STMODR_STRSTTRIGSEL2_Msk | DAC_STMODR_STINCTRIGSEL2_Msk))
                        | ((uint32_t)config->reset_trigger << DAC_STMODR_STRSTTRIGSEL2_Pos)
                        | ((uint32_t)config->inc_trigger << DAC_STMODR_STINCTRIGSEL2_Pos);
    }
    return true;
}

bool dac_config_sample_hold(dac_instance_t dac, dac_channel_t channel,
                             const dac_sh_config_t *config)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);

    DAC_TypeDef *dac_reg = dac_get_base(dac);

    if (config == NULL) {
        if (channel == DAC_CHANNEL_1) {
            dac_reg->SHSR1 = 0;
            dac_reg->SHHR = dac_reg->SHHR & ~DAC_SHHR_THOLD1_Msk;
            dac_reg->SHRR = dac_reg->SHRR & ~DAC_SHRR_TREFRESH1_Msk;
        } else {
            dac_reg->SHSR2 = 0;
            dac_reg->SHHR = dac_reg->SHHR & ~DAC_SHHR_THOLD2_Msk;
            dac_reg->SHRR = dac_reg->SHRR & ~DAC_SHRR_TREFRESH2_Msk;
        }
        return true;
    }

    if (channel == DAC_CHANNEL_1) {
        dac_reg->SHSR1 = ((uint32_t)config->sample_time & 0x3FF) << DAC_SHSR_TSAMPLE_Pos;
        dac_reg->SHHR = (dac_reg->SHHR & ~DAC_SHHR_THOLD1_Msk)
                      | ((uint32_t)config->hold_time << DAC_SHHR_THOLD1_Pos);
        dac_reg->SHRR = (dac_reg->SHRR & ~DAC_SHRR_TREFRESH1_Msk)
                      | ((uint32_t)config->refresh_time << DAC_SHRR_TREFRESH1_Pos);
    } else {
        dac_reg->SHSR2 = ((uint32_t)config->sample_time & 0x3FF) << DAC_SHSR_TSAMPLE_Pos;
        dac_reg->SHHR = (dac_reg->SHHR & ~DAC_SHHR_THOLD2_Msk)
                      | ((uint32_t)config->hold_time << DAC_SHHR_THOLD2_Pos);
        dac_reg->SHRR = (dac_reg->SHRR & ~DAC_SHRR_TREFRESH2_Msk)
                      | ((uint32_t)config->refresh_time << DAC_SHRR_TREFRESH2_Pos);
    }
    return true;
}

bool dac_enable_dma(dac_instance_t dac, dac_channel_t channel, bool enable)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);

    DAC_TypeDef *dac_reg = dac_get_base(dac);
    uint32_t shift = dac_ch_shift(channel);

    if (enable) {
        dac_reg->CR |= (DAC_CR_DMAEN1_Msk << shift);
    } else {
        dac_reg->CR &= ~(DAC_CR_DMAEN1_Msk << shift);
    }
    return true;
}

bool dac_enable_dma_double_data(dac_instance_t dac, dac_channel_t channel,
                                 bool enable)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);

    DAC_TypeDef *dac_reg = dac_get_base(dac);

    if (channel == DAC_CHANNEL_1) {
        if (enable) {
            dac_reg->MCR |= DAC_MCR_DMADOUBLE1_Msk;
        } else {
            dac_reg->MCR &= ~DAC_MCR_DMADOUBLE1_Msk;
        }
    } else {
        if (enable) {
            dac_reg->MCR |= DAC_MCR_DMADOUBLE2_Msk;
        } else {
            dac_reg->MCR &= ~DAC_MCR_DMADOUBLE2_Msk;
        }
    }
    return true;
}

bool dac_calibrate_offset(dac_instance_t dac, dac_channel_t channel)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);

    DAC_TypeDef *dac_reg = dac_get_base(dac);
    uint32_t shift = dac_ch_shift(channel);
    uint32_t cal_flag_mask = (channel == DAC_CHANNEL_1)
                             ? DAC_SR_CAL_FLAG1_Msk : DAC_SR_CAL_FLAG2_Msk;

    // 使能校准
    dac_reg->CR |= (DAC_CR_CEN1_Msk << shift);

    // 等待校准完成
    uint32_t timeout = DAC_CAL_TIMEOUT;
    while (!(dac_reg->SR & cal_flag_mask)) {
        if (--timeout == 0) {
            dac_reg->CR &= ~(DAC_CR_CEN1_Msk << shift);
            return false;
        }
    }

    // 关闭校准使能
    dac_reg->CR &= ~(DAC_CR_CEN1_Msk << shift);
    return true;
}

//===========================================
// 第4层：控制与查询 API
//===========================================

bool dac_set_data_12r(dac_instance_t dac, dac_channel_t channel, uint16_t data)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);

    DAC_TypeDef *dac_reg = dac_get_base(dac);

    if (channel == DAC_CHANNEL_1) {
        dac_reg->DHR12R1 = (data & 0xFFFU);
    } else {
        dac_reg->DHR12R2 = (data & 0xFFFU);
    }
    return true;
}

bool dac_set_data_12l(dac_instance_t dac, dac_channel_t channel, uint16_t data)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);

    DAC_TypeDef *dac_reg = dac_get_base(dac);

    if (channel == DAC_CHANNEL_1) {
        dac_reg->DHR12L1 = (uint32_t)(data & 0xFFF0U);
    } else {
        dac_reg->DHR12L2 = (uint32_t)(data & 0xFFF0U);
    }
    return true;
}

bool dac_set_data_8r(dac_instance_t dac, dac_channel_t channel, uint8_t data)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);

    DAC_TypeDef *dac_reg = dac_get_base(dac);

    if (channel == DAC_CHANNEL_1) {
        dac_reg->DHR8R1 = (uint32_t)data;
    } else {
        dac_reg->DHR8R2 = (uint32_t)data;
    }
    return true;
}

bool dac_set_dual_data_12r(dac_instance_t dac, uint16_t data1, uint16_t data2)
{
    assert(dac < DAC_COUNT);

    DAC_TypeDef *dac_reg = dac_get_base(dac);

    dac_reg->DHR12RD = ((uint32_t)(data2 & 0xFFFU) << 16)
                     | ((uint32_t)(data1 & 0xFFFU));
    return true;
}

bool dac_set_dual_data_12l(dac_instance_t dac, uint16_t data1, uint16_t data2)
{
    assert(dac < DAC_COUNT);

    DAC_TypeDef *dac_reg = dac_get_base(dac);

    dac_reg->DHR12LD = ((uint32_t)(data2 & 0xFFF0U) << 16)
                     | ((uint32_t)(data1 & 0xFFF0U));
    return true;
}

bool dac_set_dual_data_8r(dac_instance_t dac, uint8_t data1, uint8_t data2)
{
    assert(dac < DAC_COUNT);

    DAC_TypeDef *dac_reg = dac_get_base(dac);

    dac_reg->DHR8RD = ((uint32_t)data2 << 16) | (uint32_t)data1;
    return true;
}

bool dac_set_data_double(dac_instance_t dac, dac_channel_t channel,
                          dac_data_align_t align, uint16_t data_a, uint16_t data_b)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);

    DAC_TypeDef *dac_reg = dac_get_base(dac);

    switch (align) {
        case DAC_ALIGN_12B_RIGHT:
            if (channel == DAC_CHANNEL_1) {
                dac_reg->DHR12R1 = ((uint32_t)(data_b & 0xFFFU) << 16)
                                 | ((uint32_t)(data_a & 0xFFFU));
            } else {
                dac_reg->DHR12R2 = ((uint32_t)(data_b & 0xFFFU) << 16)
                                 | ((uint32_t)(data_a & 0xFFFU));
            }
            break;
        case DAC_ALIGN_12B_LEFT:
            if (channel == DAC_CHANNEL_1) {
                dac_reg->DHR12L1 = ((uint32_t)(data_b & 0xFFF0U) << 16)
                                 | ((uint32_t)(data_a & 0xFFF0U));
            } else {
                dac_reg->DHR12L2 = ((uint32_t)(data_b & 0xFFF0U) << 16)
                                 | ((uint32_t)(data_a & 0xFFF0U));
            }
            break;
        case DAC_ALIGN_8B_RIGHT:
            if (channel == DAC_CHANNEL_1) {
                dac_reg->DHR8R1 = ((uint32_t)(data_b & 0xFFU) << 8)
                                | ((uint32_t)(data_a & 0xFFU));
            } else {
                dac_reg->DHR8R2 = ((uint32_t)(data_b & 0xFFU) << 8)
                                | ((uint32_t)(data_a & 0xFFU));
            }
            break;
        default:
            return false;
    }
    return true;
}

bool dac_software_trigger(dac_instance_t dac, dac_channel_t channel)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);

    DAC_TypeDef *dac_reg = dac_get_base(dac);

    if (channel == DAC_CHANNEL_1) {
        dac_reg->SWTRIGR |= DAC_SWTRIGR_SWTRIG1_Msk;
    } else {
        dac_reg->SWTRIGR |= DAC_SWTRIGR_SWTRIG2_Msk;
    }
    return true;
}

bool dac_software_trigger_swtrigb(dac_instance_t dac, dac_channel_t channel)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);

    DAC_TypeDef *dac_reg = dac_get_base(dac);

    if (channel == DAC_CHANNEL_1) {
        dac_reg->SWTRIGR |= DAC_SWTRIGR_SWTRIGB1_Msk;
    } else {
        dac_reg->SWTRIGR |= DAC_SWTRIGR_SWTRIGB2_Msk;
    }
    return true;
}

bool dac_enable(dac_instance_t dac, dac_channel_t channel)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);

    DAC_TypeDef *dac_reg = dac_get_base(dac);
    uint32_t shift = dac_ch_shift(channel);

    // 先使能时钟
    clock_periph_enable(dac_clock_table[dac]);

    dac_reg->CR |= (DAC_CR_EN1_Msk << shift);
    return true;
}

bool dac_disable(dac_instance_t dac, dac_channel_t channel)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);

    DAC_TypeDef *dac_reg = dac_get_base(dac);
    uint32_t shift = dac_ch_shift(channel);

    dac_reg->CR &= ~(DAC_CR_EN1_Msk << shift);
    return true;
}

uint16_t dac_get_output_data(dac_instance_t dac, dac_channel_t channel)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);

    DAC_TypeDef *dac_reg = dac_get_base(dac);

    if (channel == DAC_CHANNEL_1) {
        return (uint16_t)(dac_reg->DOR1 & 0xFFFU);
    } else {
        return (uint16_t)(dac_reg->DOR2 & 0xFFFU);
    }
}

uint16_t dac_get_output_data_b(dac_instance_t dac, dac_channel_t channel)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);

    DAC_TypeDef *dac_reg = dac_get_base(dac);

    if (channel == DAC_CHANNEL_1) {
        return (uint16_t)((dac_reg->DOR1 >> 16) & 0xFFFU);
    } else {
        return (uint16_t)((dac_reg->DOR2 >> 16) & 0xFFFU);
    }
}

bool dac_is_dma_underrun(dac_instance_t dac, dac_channel_t channel)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);

    DAC_TypeDef *dac_reg = dac_get_base(dac);

    if (channel == DAC_CHANNEL_1) {
        return (dac_reg->SR & DAC_SR_DMAUDR1_Msk) != 0;
    } else {
        return (dac_reg->SR & DAC_SR_DMAUDR2_Msk) != 0;
    }
}

bool dac_is_calibration_done(dac_instance_t dac, dac_channel_t channel)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);

    DAC_TypeDef *dac_reg = dac_get_base(dac);

    if (channel == DAC_CHANNEL_1) {
        return (dac_reg->SR & DAC_SR_CAL_FLAG1_Msk) != 0;
    } else {
        return (dac_reg->SR & DAC_SR_CAL_FLAG2_Msk) != 0;
    }
}

bool dac_get_dorstat(dac_instance_t dac, dac_channel_t channel)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);

    DAC_TypeDef *dac_reg = dac_get_base(dac);

    if (channel == DAC_CHANNEL_1) {
        return (dac_reg->SR & DAC_SR_DORSTAT1_Msk) != 0;
    } else {
        return (dac_reg->SR & DAC_SR_DORSTAT2_Msk) != 0;
    }
}

bool dac_clear_dma_underrun(dac_instance_t dac, dac_channel_t channel)
{
    assert(dac < DAC_COUNT);
    assert(channel <= DAC_CHANNEL_2);

    DAC_TypeDef *dac_reg = dac_get_base(dac);

    // 写1清零DMAUDRx标志
    if (channel == DAC_CHANNEL_1) {
        dac_reg->SR = DAC_SR_DMAUDR1_Msk;
    } else {
        dac_reg->SR = DAC_SR_DMAUDR2_Msk;
    }
    return true;
}

bool dac_enable_interrupt(dac_instance_t dac, uint32_t interrupt)
{
    assert(dac < DAC_COUNT);

    DAC_TypeDef *dac_reg = dac_get_base(dac);

    if (interrupt & DAC_INT_DMA_UNDERRUN1) {
        dac_reg->CR |= DAC_CR_DMAUDIE1_Msk;
    }
    if (interrupt & DAC_INT_DMA_UNDERRUN2) {
        dac_reg->CR |= DAC_CR_DMAUDIE2_Msk;
    }

    // 使能NVIC中断
    NVIC_EnableIRQ(dac_irq_table[dac]);
    return true;
}

bool dac_disable_interrupt(dac_instance_t dac, uint32_t interrupt)
{
    assert(dac < DAC_COUNT);

    DAC_TypeDef *dac_reg = dac_get_base(dac);

    if (interrupt & DAC_INT_DMA_UNDERRUN1) {
        dac_reg->CR &= ~DAC_CR_DMAUDIE1_Msk;
    }
    if (interrupt & DAC_INT_DMA_UNDERRUN2) {
        dac_reg->CR &= ~DAC_CR_DMAUDIE2_Msk;
    }
    return true;
}

//===========================================
// 9. 回调函数注册
//===========================================

bool dac_register_dma_underrun_callback(dac_instance_t dac, dac_callback_t callback)
{
    assert(dac < DAC_COUNT);

    dac_callback_manager[dac].dma_underrun_callback = callback;
    return true;
}

//===========================================
// 10. 中断处理函数
//===========================================

/**
 * @brief DAC中断处理通用函数
 */
static void dac_irq_handler_common(dac_instance_t dac)
{
    DAC_TypeDef *dac_reg = dac_get_base(dac);
    uint32_t sr = dac_reg->SR;
    dac_callback_manager_t *mgr = &dac_callback_manager[dac];

    // DMA下溢通道1
    if ((sr & DAC_SR_DMAUDR1_Msk) && (dac_reg->CR & DAC_CR_DMAUDIE1_Msk)) {
        if (mgr->dma_underrun_callback) {
            mgr->dma_underrun_callback(dac, DAC_CHANNEL_1);
        }
        dac_reg->SR = DAC_SR_DMAUDR1_Msk;  // 写1清零
    }

    // DMA下溢通道2
    if ((sr & DAC_SR_DMAUDR2_Msk) && (dac_reg->CR & DAC_CR_DMAUDIE2_Msk)) {
        if (mgr->dma_underrun_callback) {
            mgr->dma_underrun_callback(dac, DAC_CHANNEL_2);
        }
        dac_reg->SR = DAC_SR_DMAUDR2_Msk;  // 写1清零
    }
}

void DAC1_IRQHandler(void)
{
    dac_irq_handler_common(DAC_1);
}
