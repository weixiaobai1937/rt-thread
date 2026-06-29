/**
 * @file adc.c
 * @brief ACM32P4 ADC驱动实现
 *
 * @details
 * 本文件实现了ACM32P4芯片的ADC模块功能：
 * - 单次/连续转换模式
 * - 规则通道组（最多16个）和注入通道组（最多4个）
 * - 模拟看门狗
 * - 过采样与偏移补偿
 * - 中断和DMA支持
 * - 自校准
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "device/acm32p4xx.h"
#include <assert.h>
#include <string.h>

//===========================================
// 内部宏定义
//===========================================

#define ADC_READY_TIMEOUT   100000  ///< ADC就绪超时
#define ADC_CAL_TIMEOUT     100000  ///< 校准超时
#define ADC_REGULATOR_DELAY_US  20  ///< 稳压器稳定延时 (us)

//===========================================
// 内部状态变量
//===========================================

static adc_callback_t eoc_callbacks[ADC_COUNT];
static adc_callback_t eog_callbacks[ADC_COUNT];
static adc_callback_t jeoc_callbacks[ADC_COUNT];
static adc_callback_t jeog_callbacks[ADC_COUNT];
static adc_callback_t awd_callbacks[ADC_COUNT];
static adc_callback_t overflow_callbacks[ADC_COUNT];

//===========================================
// 内部辅助函数
//===========================================

/**
 * @brief 获取ADC实例的寄存器基地址
 */
static ADC_TypeDef* adc_get_regs(adc_instance_t inst)
{
    return (ADC_TypeDef*)ADC_BASE;
}

/**
 * @brief 简单延时（微秒级估算）
 */
static void adc_delay_us(uint32_t us)
{
    volatile uint32_t count = us * (SystemCoreClock / 1000000U / 4);
    while (count--) {
        __NOP();
    }
}

//===========================================
// 第1层：快速初始化 API
//===========================================

bool adc_init_single_channel(adc_instance_t inst, adc_channel_t channel,
                             adc_sample_time_t sample_time)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);

    // 使能ADC时钟
    clock_periph_enable(CLK_ADC);

    // 使能稳压器
    adc_enable_regulator(inst);
    adc_delay_us(ADC_REGULATOR_DELAY_US);

    // 配置规则序列：1个通道
    adc_regular_config_t reg = {
        .length = 1,
        .sequence = {channel},
    };
    adc_config_regular_sequence(inst, &reg);

    // 配置采样时间
    adc_sampling_config_t smp = {0};
    smp.smp[channel] = sample_time;
    adc_config_sampling_time(inst, &smp);

    // 单次转换模式
    adc_enable_continuous_mode(inst, false);

    // 使能ADC
    adc_enable(inst);

    // 等待ADC就绪
    uint32_t timeout = ADC_READY_TIMEOUT;
    while (!adc_is_ready(inst)) {
        if (--timeout == 0) return false;
    }

    return true;
}

bool adc_init_continuous_channel(adc_instance_t inst, adc_channel_t channel,
                                adc_sample_time_t sample_time)
{
    if (!adc_init_single_channel(inst, channel, sample_time)) {
        return false;
    }

    ADC_TypeDef *adc = adc_get_regs(inst);
    adc_enable_continuous_mode(inst, true);

    return true;
}

bool adc_init_multi_channel_dma(adc_instance_t inst, const adc_channel_t* channels,
                                uint8_t channel_count, adc_sample_time_t sample_time,
                                uint16_t* dma_buffer)
{
    (void)dma_buffer;
    if (inst >= ADC_COUNT || channels == NULL || channel_count == 0 || channel_count > 16) {
        return false;
    }

    ADC_TypeDef *adc = adc_get_regs(inst);

    clock_periph_enable(CLK_ADC);
    adc_enable_regulator(inst);
    adc_delay_us(ADC_REGULATOR_DELAY_US);

    // 配置规则序列
    adc_regular_config_t reg = {
        .length = channel_count,
    };
    for (uint8_t i = 0; i < channel_count; i++) {
        reg.sequence[i] = channels[i];
    }
    adc_config_regular_sequence(inst, &reg);

    // 统一采样时间
    adc_sampling_config_t smp = {0};
    for (uint8_t i = 0; i < channel_count; i++) {
        smp.smp[channels[i]] = sample_time;
    }
    adc_config_sampling_time(inst, &smp);

    // 使能DMA
    adc_enable_dma(inst);

    // 使能ADC
    adc_enable(inst);
    uint32_t timeout = ADC_READY_TIMEOUT;
    while (!adc_is_ready(inst)) {
        if (--timeout == 0) return false;
    }

    return true;
}

bool adc_init_internal_channel(adc_instance_t inst, adc_internal_channel_t channel,
                               adc_sample_time_t sample_time)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);

    // 使能内部通道
    adc_enable_internal_channel(inst, channel, true);

    // 根据内部通道选择实际ADC通道
    adc_channel_t adc_ch;
    switch (channel) {
        case ADC_INTERNAL_VREFINT:    adc_ch = ADC_CHANNEL_12; break;
        case ADC_INTERNAL_TEMPSENSOR: adc_ch = ADC_CHANNEL_15; break;
        case ADC_INTERNAL_VBAT:       adc_ch = ADC_CHANNEL_13; break;
        default: return false;
    }

    return adc_init_single_channel(inst, adc_ch, sample_time);
}

//===========================================
// 第2层：基础配置 API
//===========================================

bool adc_config_clock(adc_instance_t inst, const adc_clock_config_t* config)
{
    if (inst >= ADC_COUNT || config == NULL) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);

    // 配置时钟模式和分频
    uint32_t ccr = adc->CCR;
    ccr &= ~(1U << 13);           // CKMODE
    ccr &= ~(0x3F << 16);         // ADCDIV
    ccr |= ((uint32_t)config->divider << 16);

    if (config->source == ADC_CLK_SRC_PLLQCLK) {
        ccr |= (1U << 13);        // CKMODE = 1 (ADC_KER_CLK)
        clock_adc_set_source(ADC_CLK_PLLQCLK);
    } else {
        ccr &= ~(1U << 13);       // CKMODE = 0 (HCLK)
        clock_adc_set_source(ADC_CLK_RCH);
    }

    adc->CCR = ccr;
    return true;
}

bool adc_config_resolution(adc_instance_t inst, adc_resolution_t resolution)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    uint32_t cr2 = adc->CR2;
    cr2 &= ~(0x3 << 4);
    cr2 |= ((uint32_t)resolution << 4);
    adc->CR2 = cr2;
    return true;
}

bool adc_config_regular_sequence(adc_instance_t inst, const adc_regular_config_t* config)
{
    if (inst >= ADC_COUNT || config == NULL) return false;
    if (config->length == 0 || config->length > 16) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);

    // SQR1: L[3:0] + SQ1-SQ5
    uint32_t sqr1 = ((uint32_t)(config->length - 1) & 0xF);
    for (uint8_t i = 0; i < 5 && i < config->length; i++) {
        sqr1 |= (((uint32_t)config->sequence[i] & 0x1F) << (5 * (i + 1)));
    }
    adc->SQR1 = sqr1;

    // SQR2: SQ6-SQ11
    uint32_t sqr2 = 0;
    for (uint8_t i = 5; i < 11 && i < config->length; i++) {
        sqr2 |= (((uint32_t)config->sequence[i] & 0x1F) << (5 * (i - 5)));
    }
    adc->SQR2 = sqr2;

    // SQR3: SQ12-SQ16
    uint32_t sqr3 = 0;
    for (uint8_t i = 11; i < 16 && i < config->length; i++) {
        sqr3 |= (((uint32_t)config->sequence[i] & 0x1F) << (5 * (i - 11)));
    }
    adc->SQR3 = sqr3;

    return true;
}

bool adc_config_injected_sequence(adc_instance_t inst, const adc_injected_config_t* config)
{
    if (inst >= ADC_COUNT || config == NULL) return false;
    if (config->length == 0 || config->length > 4) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);

    uint32_t jsqr = ((uint32_t)(config->length - 1) & 0x3);
    for (uint8_t i = 0; i < config->length; i++) {
        jsqr |= (((uint32_t)config->sequence[i] & 0x1F) << (5 * (i + 1)));
    }
    adc->JSQR = jsqr;

    return true;
}

bool adc_config_sampling_time(adc_instance_t inst, const adc_sampling_config_t* config)
{
    if (inst >= ADC_COUNT || config == NULL) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);

    // SMPR1: CH0-CH7
    uint32_t smpr1 = 0;
    for (uint8_t i = 0; i < 8; i++) {
        smpr1 |= (((uint32_t)config->smp[i] & 0xF) << (4 * i));
    }
    adc->SMPR1 = smpr1;

    // SMPR2: CH8-CH15
    uint32_t smpr2 = 0;
    for (uint8_t i = 0; i < 8; i++) {
        smpr2 |= (((uint32_t)config->smp[i + 8] & 0xF) << (4 * i));
    }
    adc->SMPR2 = smpr2;

    // SMPR3: CH16-CH18
    uint32_t smpr3 = 0;
    for (uint8_t i = 0; i < 3; i++) {
        smpr3 |= (((uint32_t)config->smp[i + 16] & 0xF) << (4 * i));
    }
    adc->SMPR3 = smpr3;

    return true;
}

bool adc_config_regular_trigger(adc_instance_t inst, adc_regular_trigger_t trigger,
                                adc_trigger_polarity_t polarity)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    uint32_t cr1 = adc->CR1;
    cr1 &= ~(0x1F << 16);  // EXTSEL
    cr1 |= (((uint32_t)trigger & 0x1F) << 16);
    cr1 &= ~(0x3 << 14);   // EXTEN
    cr1 |= (((uint32_t)polarity & 0x3) << 14);
    adc->CR1 = cr1;
    return true;
}

bool adc_config_injected_trigger(adc_instance_t inst, adc_injected_trigger_t trigger,
                                 adc_trigger_polarity_t polarity)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    uint32_t jsqr = adc->JSQR;
    jsqr &= ~(0x1F << 27);  // JEXTSEL
    jsqr |= (((uint32_t)trigger & 0x1F) << 27);
    jsqr &= ~(0x3 << 25);   // JEXTEN
    jsqr |= (((uint32_t)polarity & 0x3) << 25);
    adc->JSQR = jsqr;
    return true;
}

bool adc_config_watchdog(adc_instance_t inst, const adc_watchdog_config_t* config)
{
    if (inst >= ADC_COUNT || config == NULL) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);

    // 配置阈值
    adc->HTR = ((uint32_t)config->high_threshold_diff << 16) | (config->high_threshold & 0xFFF);
    adc->LTR = ((uint32_t)config->low_threshold_diff << 16) | (config->low_threshold & 0xFFF);

    // 配置CR1中的看门狗控制位
    uint32_t cr1 = adc->CR1;
    cr1 &= ~((0x1F << 0) | (0x1F << 5) | (1U << 10) | (1U << 11) | (1U << 12));

    switch (config->mode) {
        case ADC_AWD_DISABLED:        break;
        case ADC_AWD_ALL_REGULAR:     cr1 |= (1U << 12); break;
        case ADC_AWD_ALL_INJECTED:    cr1 |= (1U << 11); break;
        case ADC_AWD_ALL:             cr1 |= (1U << 12) | (1U << 11); break;
        case ADC_AWD_SINGLE_REGULAR:  cr1 |= (1U << 12) | (1U << 10); cr1 |= ((uint32_t)config->channel & 0x1F); break;
        case ADC_AWD_SINGLE_INJECTED: cr1 |= (1U << 11) | (1U << 10); cr1 |= (((uint32_t)config->channel & 0x1F) << 5); break;
        case ADC_AWD_SINGLE_ALL:      cr1 |= (1U << 12) | (1U << 11) | (1U << 10); cr1 |= ((uint32_t)config->channel & 0x1F); break;
    }
    adc->CR1 = cr1;

    // 中断使能
    if (config->enable_interrupt) {
        adc->IE |= (1U << 7);  // AWDIE
    }

    return true;
}

//===========================================
// 第3层：高级功能 API
//===========================================

bool adc_config_oversampling(adc_instance_t inst, const adc_oversampling_config_t* config)
{
    if (inst >= ADC_COUNT || config == NULL) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    uint32_t cr2 = adc->CR2;

    cr2 &= ~((1U << 16) | (0x7 << 17) | (0xF << 20) | (1U << 24) | (1U << 25));

    if (config->enable_regular) {
        cr2 |= (1U << 16);  // OVSE
    }
    if (config->enable_injected) {
        cr2 |= (1U << 25);  // JOVSE
    }
    cr2 |= (((uint32_t)config->rate & 0x7) << 17);
    cr2 |= (((uint32_t)config->shift & 0xF) << 20);
    cr2 |= (((uint32_t)config->trigger_mode & 0x1) << 24);

    adc->CR2 = cr2;
    return true;
}

bool adc_config_offset(adc_instance_t inst, const adc_offset_config_t* config)
{
    if (inst >= ADC_COUNT || config == NULL) return false;
    if (config->index > 3) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    volatile uint32_t *ofr = &adc->OFR1 + config->index;

    uint32_t val = 0;
    val |= ((uint32_t)config->offset_value & 0xFFF);
    val |= (((uint32_t)config->mode & 0x1) << 24);
    val |= (((uint32_t)config->format & 0x1) << 25);
    val |= (((uint32_t)config->channel & 0x1F) << 26);
    val |= (1U << 31);  // OFFSETx_EN

    *ofr = val;
    return true;
}

bool adc_config_channel_input_mode(adc_instance_t inst, adc_channel_t channel,
                                   adc_input_mode_t mode)
{
    if (inst >= ADC_COUNT) return false;
    if (channel > 18) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    uint32_t difsel = adc->DIFSEL;

    if (mode == ADC_INPUT_DIFFERENTIAL) {
        difsel |= (1U << channel);
    } else {
        difsel &= ~(1U << channel);
    }

    adc->DIFSEL = difsel;
    return true;
}

bool adc_config_data_alignment(adc_instance_t inst, adc_data_align_t align)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    if (align == ADC_ALIGN_LEFT) {
        adc->CR2 |= (1U << 3);
    } else {
        adc->CR2 &= ~(1U << 3);
    }
    return true;
}

bool adc_config_overflow_mode(adc_instance_t inst, adc_overflow_mode_t mode)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    if (mode == ADC_OVERFLOW_OVERWRITE) {
        adc->CR2 |= (1U << 1);
    } else {
        adc->CR2 &= ~(1U << 1);
    }
    return true;
}

bool adc_config_discontinuous_mode(adc_instance_t inst, bool regular_enable,
                                   bool injected_enable, adc_discnum_t discnum)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    uint32_t cr1 = adc->CR1;

    if (regular_enable) {
        cr1 |= (1U << 26);   // DISCEN
        cr1 &= ~(0x7 << 28);
        cr1 |= (((uint32_t)discnum & 0x7) << 28);
    } else {
        cr1 &= ~(1U << 26);
    }

    if (injected_enable) {
        cr1 |= (1U << 27);   // JDISCEN
    } else {
        cr1 &= ~(1U << 27);
    }

    adc->CR1 = cr1;
    return true;
}

bool adc_enable_auto_injected(adc_instance_t inst, bool enable)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    if (enable) {
        adc->CR1 |= (1U << 25);  // JAUTO
    } else {
        adc->CR1 &= ~(1U << 25);
    }
    return true;
}

bool adc_config_dual_mode(const adc_dual_config_t* config)
{
    (void)config;
    return false;  // P4 双ADC模式待确认
}

bool adc_enable_internal_channel(adc_instance_t inst, adc_internal_channel_t channel,
                                 bool enable)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    uint32_t ccr = adc->CCR;

    switch (channel) {
        case ADC_INTERNAL_VREFINT:
            if (enable) ccr |= (1U << 23); else ccr &= ~(1U << 23);
            break;
        case ADC_INTERNAL_TEMPSENSOR:
            if (enable) ccr |= (1U << 22); else ccr &= ~(1U << 22);
            break;
        case ADC_INTERNAL_VBAT:
            if (enable) ccr |= (1U << 24); else ccr &= ~(1U << 24);
            break;
        default:
            return false;
    }

    adc->CCR = ccr;
    return true;
}

bool adc_config_trigger_polarity(adc_instance_t inst, adc_trigger_polarity_t polarity)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    uint32_t cr1 = adc->CR1;
    cr1 &= ~(0x3 << 14);
    cr1 |= (((uint32_t)polarity & 0x3) << 14);
    adc->CR1 = cr1;
    return true;
}

bool adc_enable_continuous_mode(adc_instance_t inst, bool enable)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    if (enable) {
        adc->CR1 |= (1U << 24);  // CONT
    } else {
        adc->CR1 &= ~(1U << 24);
    }
    return true;
}

//===========================================
// 第4层：控制与查询 API
//===========================================

bool adc_start_regular_conversion(adc_instance_t inst)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    adc->CR1 |= (1U << 23);  // SWSTART
    return true;
}

bool adc_start_injected_conversion(adc_instance_t inst)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    adc->CR1 |= (1U << 22);  // JSWSTART
    return true;
}

bool adc_stop_conversion(adc_instance_t inst)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    adc->CR2 |= (1U << 2);  // ADC_STP
    return true;
}

bool adc_enable(adc_instance_t inst)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    adc->CR2 |= (1U << 0);  // ADC_EN
    return true;
}

bool adc_disable(adc_instance_t inst)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    adc->CR2 &= ~(1U << 0);  // ADC_EN
    return true;
}

uint16_t adc_get_result(adc_instance_t inst)
{
    if (inst >= ADC_COUNT) return 0;

    ADC_TypeDef *adc = adc_get_regs(inst);
    return (uint16_t)(adc->DR & 0xFFFF);
}

uint16_t adc_get_result_with_channel(adc_instance_t inst, adc_channel_t* channel)
{
    if (inst >= ADC_COUNT) return 0;

    ADC_TypeDef *adc = adc_get_regs(inst);
    uint32_t dr = adc->DR;
    if (channel) {
        *channel = (adc_channel_t)((dr >> 16) & 0x1F);
    }
    return (uint16_t)(dr & 0xFFFF);
}

uint16_t adc_get_injected_result(adc_instance_t inst, uint8_t index)
{
    if (inst >= ADC_COUNT || index > 3) return 0;

    ADC_TypeDef *adc = adc_get_regs(inst);
    const volatile uint32_t *jdr = &adc->JDR1 + index;
    return (uint16_t)(*jdr & 0xFFFF);
}

uint32_t adc_get_dual_result(void)
{
    return 0;  // P4 双ADC模式待确认
}

uint16_t adc_get_channel_data(adc_instance_t inst, adc_channel_t channel)
{
    (void)channel;
    return adc_get_result(inst);
}

bool adc_is_ready(adc_instance_t inst)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    return (adc->SR & (1U << 0)) != 0;  // ADRDY
}

bool adc_is_eoc(adc_instance_t inst)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    return (adc->SR & (1U << 2)) != 0;  // EOC
}

bool adc_is_eog(adc_instance_t inst)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    return (adc->SR & (1U << 3)) != 0;  // EOG
}

bool adc_is_jeoc(adc_instance_t inst)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    return (adc->SR & (1U << 5)) != 0;  // JEOC
}

bool adc_is_jeog(adc_instance_t inst)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    return (adc->SR & (1U << 6)) != 0;  // JEOG
}

bool adc_is_awd(adc_instance_t inst)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    return (adc->SR & (1U << 7)) != 0;  // AWD
}

bool adc_is_overflow(adc_instance_t inst)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    return (adc->SR & (1U << 4)) != 0;  // OVERF
}

bool adc_is_eosmp(adc_instance_t inst)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    return (adc->SR & (1U << 1)) != 0;  // EOSMP
}

bool adc_clear_status_flag(adc_instance_t inst, uint32_t flag)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    // 写1清除（EOC/EOG/JEOC/JEOG/OVERF/AWD/EOSMP）
    uint32_t clear_mask = 0;
    if (flag & ADC_INT_EOSMP)  clear_mask |= (1U << 1);
    if (flag & ADC_INT_EOC)    clear_mask |= (1U << 2);
    if (flag & ADC_INT_EOG)    clear_mask |= (1U << 3);
    if (flag & ADC_INT_OVERF)  clear_mask |= (1U << 4);
    if (flag & ADC_INT_JEOC)   clear_mask |= (1U << 5);
    if (flag & ADC_INT_JEOG)   clear_mask |= (1U << 6);
    if (flag & ADC_INT_AWD)    clear_mask |= (1U << 7);
    adc->SR = clear_mask;
    return true;
}

bool adc_enable_interrupt(adc_instance_t inst, uint32_t interrupt)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    uint32_t ie_mask = 0;
    if (interrupt & ADC_INT_EOSMP)  ie_mask |= (1U << 1);
    if (interrupt & ADC_INT_EOC)    ie_mask |= (1U << 2);
    if (interrupt & ADC_INT_EOG)    ie_mask |= (1U << 3);
    if (interrupt & ADC_INT_OVERF)  ie_mask |= (1U << 4);
    if (interrupt & ADC_INT_JEOC)   ie_mask |= (1U << 5);
    if (interrupt & ADC_INT_JEOG)   ie_mask |= (1U << 6);
    if (interrupt & ADC_INT_AWD)    ie_mask |= (1U << 7);
    adc->IE |= ie_mask;
    return true;
}

bool adc_disable_interrupt(adc_instance_t inst, uint32_t interrupt)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    uint32_t ie_mask = 0;
    if (interrupt & ADC_INT_EOSMP)  ie_mask |= (1U << 1);
    if (interrupt & ADC_INT_EOC)    ie_mask |= (1U << 2);
    if (interrupt & ADC_INT_EOG)    ie_mask |= (1U << 3);
    if (interrupt & ADC_INT_OVERF)  ie_mask |= (1U << 4);
    if (interrupt & ADC_INT_JEOC)   ie_mask |= (1U << 5);
    if (interrupt & ADC_INT_JEOG)   ie_mask |= (1U << 6);
    if (interrupt & ADC_INT_AWD)    ie_mask |= (1U << 7);
    adc->IE &= ~ie_mask;
    return true;
}

bool adc_enable_dma(adc_instance_t inst)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    adc->CR1 |= (1U << 13);  // DMA
    return true;
}

bool adc_disable_dma(adc_instance_t inst)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    adc->CR1 &= ~(1U << 13);
    return true;
}

bool adc_calibrate(adc_instance_t inst, bool differential_mode)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);

    // 校准前ADC必须禁用
    if (adc->CR2 & (1U << 0)) return false;

    // 设置校准模式
    if (differential_mode) {
        adc->CR2 |= (1U << 30);   // ADCCALDIF
    } else {
        adc->CR2 &= ~(1U << 30);
    }

    // 启动校准
    adc->CR2 |= (1U << 31);  // ADCCAL

    // 等待校准完成
    uint32_t timeout = ADC_CAL_TIMEOUT;
    while (adc->CR2 & (1U << 31)) {
        if (--timeout == 0) return false;
    }

    return true;
}

bool adc_enable_regulator(adc_instance_t inst)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    adc->CR2 |= (1U << 28);  // ADCVREGEN
    return true;
}

bool adc_disable_regulator(adc_instance_t inst)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    adc->CR2 &= ~(1U << 28);
    return true;
}

bool adc_reset_afe(adc_instance_t inst)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    adc->CR2 &= ~(1U << 29);  // ADCRSTN = 0 (复位)
    adc_delay_us(1);
    adc->CR2 |= (1U << 29);   // ADCRSTN = 1 (释放)
    return true;
}

uint8_t adc_get_calibration_factor(adc_instance_t inst, bool differential)
{
    if (inst >= ADC_COUNT) return 0;

    ADC_TypeDef *adc = adc_get_regs(inst);
    if (differential) {
        return (uint8_t)((adc->CALFACT >> 16) & 0x7F);
    } else {
        return (uint8_t)(adc->CALFACT & 0x7F);
    }
}

bool adc_set_calibration_factor(adc_instance_t inst, bool differential, uint8_t factor)
{
    if (inst >= ADC_COUNT) return false;

    ADC_TypeDef *adc = adc_get_regs(inst);
    uint32_t cal = adc->CALFACT;
    if (differential) {
        cal &= ~(0x7F << 16);
        cal |= (((uint32_t)factor & 0x7F) << 16);
    } else {
        cal &= ~0x7F;
        cal |= ((uint32_t)factor & 0x7F);
    }
    adc->CALFACT = cal;
    return true;
}

bool adc_enable_vref_buffer(uint8_t voltage)
{
    ADC_TypeDef *adc = adc_get_regs(ADC_1);
    uint32_t cvrb = adc->CVRB;
    cvrb |= (1U << 0);   // ENVR
    cvrb &= ~(1U << 1);  // HIZ = 0 (连接VREF+)
    cvrb &= ~(0x3 << 2);
    cvrb |= (((uint32_t)voltage & 0x3) << 2);
    adc->CVRB = cvrb;
    return true;
}

bool adc_disable_vref_buffer(void)
{
    ADC_TypeDef *adc = adc_get_regs(ADC_1);
    adc->CVRB &= ~(1U << 0);  // ENVR = 0
    return true;
}

//===========================================
// 回调函数注册
//===========================================

bool adc_register_eoc_callback(adc_instance_t inst, adc_callback_t callback)
{
    if (inst >= ADC_COUNT) return false;
    eoc_callbacks[inst] = callback;
    return true;
}

bool adc_register_eog_callback(adc_instance_t inst, adc_callback_t callback)
{
    if (inst >= ADC_COUNT) return false;
    eog_callbacks[inst] = callback;
    return true;
}

bool adc_register_jeoc_callback(adc_instance_t inst, adc_callback_t callback)
{
    if (inst >= ADC_COUNT) return false;
    jeoc_callbacks[inst] = callback;
    return true;
}

bool adc_register_jeog_callback(adc_instance_t inst, adc_callback_t callback)
{
    if (inst >= ADC_COUNT) return false;
    jeog_callbacks[inst] = callback;
    return true;
}

bool adc_register_awd_callback(adc_instance_t inst, adc_callback_t callback)
{
    if (inst >= ADC_COUNT) return false;
    awd_callbacks[inst] = callback;
    return true;
}

bool adc_register_overflow_callback(adc_instance_t inst, adc_callback_t callback)
{
    if (inst >= ADC_COUNT) return false;
    overflow_callbacks[inst] = callback;
    return true;
}

//===========================================
// 辅助函数
//===========================================

uint32_t adc_calculate_sample_rate(uint32_t adc_clk_hz, uint32_t sample_cycles,
                                   uint32_t resolution)
{
    uint32_t bits;
    switch (resolution) {
        case 12: bits = 12; break;
        case 10: bits = 10; break;
        case 8:  bits = 8;  break;
        case 6:  bits = 6;  break;
        default: bits = 12; break;
    }
    if (sample_cycles == 0) sample_cycles = 1;
    return adc_clk_hz / (sample_cycles + bits);
}

//===========================================
// 中断处理函数
//===========================================

void ADC_IRQHandler(void)
{
    ADC_TypeDef *adc = adc_get_regs(ADC_1);
    uint32_t sr = adc->SR;

    if ((sr & (1U << 2)) && (adc->IE & (1U << 2))) {  // EOC
        adc->SR = (1U << 2);  // 清除
        if (eoc_callbacks[ADC_1]) eoc_callbacks[ADC_1](ADC_1);
    }

    if ((sr & (1U << 3)) && (adc->IE & (1U << 3))) {  // EOG
        adc->SR = (1U << 3);
        if (eog_callbacks[ADC_1]) eog_callbacks[ADC_1](ADC_1);
    }

    if ((sr & (1U << 5)) && (adc->IE & (1U << 5))) {  // JEOC
        adc->SR = (1U << 5);
        if (jeoc_callbacks[ADC_1]) jeoc_callbacks[ADC_1](ADC_1);
    }

    if ((sr & (1U << 6)) && (adc->IE & (1U << 6))) {  // JEOG
        adc->SR = (1U << 6);
        if (jeog_callbacks[ADC_1]) jeog_callbacks[ADC_1](ADC_1);
    }

    if ((sr & (1U << 7)) && (adc->IE & (1U << 7))) {  // AWD
        adc->SR = (1U << 7);
        if (awd_callbacks[ADC_1]) awd_callbacks[ADC_1](ADC_1);
    }

    if ((sr & (1U << 4)) && (adc->IE & (1U << 4))) {  // OVERF
        adc->SR = (1U << 4);
        if (overflow_callbacks[ADC_1]) overflow_callbacks[ADC_1](ADC_1);
    }
}
