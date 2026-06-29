/**
 * @file adc_example.c
 * @brief ACM32P4 ADC驱动示例代码
 *
 * 演示10个场景：
 * 1. 单通道单次转换（轮询）
 * 2. 单通道连续转换
 * 3. 多通道扫描转换
 * 4. 多通道DMA传输
 * 5. 注入组转换
 * 6. 模拟看门狗中断
 * 7. 温度传感器读取
 * 8. 双ADC同步模式（ADC1 + ADC2）
 * 9. 过采样降噪
 * 10. 差分输入模式
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#include "acm32p4.h"
#include "hardware/system.h"
#include "SEGGER_RTT.h"

//=============================================================================
// 配置选项
//=============================================================================

#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)

// 选择要运行的场景（修改此值来切换场景）
#define SCENARIO 1

//=============================================================================
// 辅助函数
//=============================================================================

/**
 * @brief 打印分隔线
 */
void print_separator(void)
{
    printf("========================================\n");
}

//=============================================================================
// 全局变量
//=============================================================================

static volatile bool adc_awd_event = false;
static volatile uint16_t adc_dma_buffer[16];
static volatile uint32_t adc_dma_index = 0;

//=============================================================================
// 场景1：单通道单次转换（轮询模式）
//=============================================================================

/**
 * @brief 场景1：单通道单次转换
 *
 * 功能：在ADC1通道3（PA3）进行单次电压采集
 * 硬件连接：PA3 -> 可调电压源（0-3.3V）
 * API使用：adc_init_single_channel() + adc_start_regular_conversion()
 */
void scenario_1_single_conversion(void)
{
    print_separator();
    printf("=== 场景1：单通道单次转换 ===\n\n");

    printf("步骤1：配置GPIO为模拟模式\n");
    gpio_set_analog_mode(PA3);

    printf("步骤2：初始化ADC1通道3（单次转换）\n");
    if (!adc_init_single_channel(ADC_1, ADC_CHANNEL_3, ADC_SAMPLE_13)) {
        printf("  错误：ADC初始化失败\n");
        print_separator();
        return;
    }
    printf("  结果：ADC1初始化成功\n");

    printf("\n步骤3：启动软件触发转换\n");
    adc_start_regular_conversion(ADC_1);
    printf("  结果：转换已启动\n");

    printf("\n步骤4：等待转换完成\n");
    while (!adc_is_eoc(ADC_1)) {
        __NOP();
    }
    printf("  结果：转换完成\n");

    printf("\n步骤5：读取转换结果\n");
    uint16_t val = adc_get_result(ADC_1);
    printf("  结果：ADC值 = %u（%.2fV）\n", val, (float)val * 3.3f / 4095.0f);

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景2：单通道连续转换
//=============================================================================

/**
 * @brief 场景2：单通道连续转换
 *
 * 功能：在ADC1通道3（PA3）连续采集电压
 * 硬件连接：PA3 -> 可调电压源
 * API使用：adc_init_continuous_channel()
 */
void scenario_2_continuous_conversion(void)
{
    print_separator();
    printf("=== 场景2：单通道连续转换 ===\n\n");

    printf("步骤1：配置GPIO为模拟模式\n");
    gpio_set_analog_mode(PA3);

    printf("步骤2：初始化ADC1通道3（连续转换）\n");
    if (!adc_init_continuous_channel(ADC_1, ADC_CHANNEL_3, ADC_SAMPLE_13)) {
        printf("  错误：ADC初始化失败\n");
        print_separator();
        return;
    }
    printf("  结果：ADC1连续转换已启动\n");

    printf("\n步骤3：启动转换\n");
    adc_start_regular_conversion(ADC_1);

    printf("步骤4：连续读取5次结果\n");
    for (int i = 0; i < 5; i++) {
        delay_ms(100);
        uint16_t val = adc_get_result(ADC_1);
        printf("  第%d次：ADC值 = %u（%.2fV）\n", i + 1, val,
               (float)val * 3.3f / 4095.0f);
    }

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景3：多通道扫描转换
//=============================================================================

/**
 * @brief 场景3：多通道扫描转换
 *
 * 功能：扫描ADC1通道3-6四个通道的电压
 * 硬件连接：PA3-PA6 -> 不同电压源
 * API使用：adc_config_regular_sequence()
 */
void scenario_3_multi_channel_scan(void)
{
    print_separator();
    printf("=== 场景3：多通道扫描转换 ===\n\n");

    printf("步骤1：配置GPIO为模拟模式\n");
    gpio_set_analog_mode(PA3);
    gpio_set_analog_mode(PA4);
    gpio_set_analog_mode(PA5);
    gpio_set_analog_mode(PA6);

    printf("步骤2：初始化时钟和稳压器\n");
    adc_enable_regulator(ADC_1);
    delay_ms(1);

    printf("步骤3：配置规则序列（4通道：CH3-CH6）\n");
    adc_regular_config_t reg = {
        .length = 4,
        .sequence = {ADC_CHANNEL_3, ADC_CHANNEL_4, ADC_CHANNEL_5, ADC_CHANNEL_6},
    };
    adc_config_regular_sequence(ADC_1, &reg);
    printf("  结果：序列配置成功\n");

    printf("步骤4：配置所有通道采样时间为13周期\n");
    adc_sampling_config_t smp = {0};
    smp.smp[3] = ADC_SAMPLE_13;
    smp.smp[4] = ADC_SAMPLE_13;
    smp.smp[5] = ADC_SAMPLE_13;
    smp.smp[6] = ADC_SAMPLE_13;
    adc_config_sampling_time(ADC_1, &smp);

    printf("步骤5：使能ADC并启动转换\n");
    adc_config_resolution(ADC_1, ADC_RES_12BIT);
    adc_enable(ADC_1);
    adc_start_regular_conversion(ADC_1);

    printf("步骤6：等待组转换结束并读取结果\n");
    while (!adc_is_eog(ADC_1)) { __NOP(); }

    printf("  通道3：%u（%.2fV）\n", adc_get_result(ADC_1),
           (float)adc_get_result(ADC_1) * 3.3f / 4095.0f);
    printf("  通道4：%u（%.2fV）\n", adc_get_result(ADC_1),
           (float)adc_get_result(ADC_1) * 3.3f / 4095.0f);
    printf("  通道5：%u（%.2fV）\n", adc_get_result(ADC_1),
           (float)adc_get_result(ADC_1) * 3.3f / 4095.0f);
    printf("  通道6：%u（%.2fV）\n", adc_get_result(ADC_1),
           (float)adc_get_result(ADC_1) * 3.3f / 4095.0f);

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景4：多通道DMA传输
//=============================================================================

/**
 * @brief 场景4：多通道DMA传输
 *
 * 功能：ADC1通道3-6通过DMA自动传输转换结果
 * 硬件连接：PA3-PA6 -> 不同电压源
 * API使用：adc_init_multi_channel_dma()
 */
void scenario_4_multi_channel_dma(void)
{
    print_separator();
    printf("=== 场景4：多通道DMA传输 ===\n\n");

    printf("步骤1：配置GPIO为模拟模式\n");
    gpio_set_analog_mode(PA3);
    gpio_set_analog_mode(PA4);
    gpio_set_analog_mode(PA5);
    gpio_set_analog_mode(PA6);

    printf("步骤2：初始化DMA控制器\n");
    printf("  注意：本示例仅展示ADC配置部分\n");
    printf("  DMA通道需单独配置（请参考dma_example.c）\n");

    printf("步骤3：快速初始化多通道DMA模式\n");
    adc_channel_t channels[] = {ADC_CHANNEL_3, ADC_CHANNEL_4,
                                ADC_CHANNEL_5, ADC_CHANNEL_6};

    if (!adc_init_multi_channel_dma(ADC_1, channels, 4, ADC_SAMPLE_13,
                                     (uint16_t *)adc_dma_buffer)) {
        printf("  错误：ADC DMA初始化失败\n");
        print_separator();
        return;
    }
    printf("  结果：ADC DMA模式已启动\n");
    printf("  数据自动传输到缓冲区，无需CPU干预\n");

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景5：注入组转换
//=============================================================================

/**
 * @brief 场景5：注入组转换
 *
 * 功能：规则组连续转换 + 注入组触发转换
 * 硬件连接：PA3（规则通道）、PA4（注入通道）
 * API使用：adc_config_injected_sequence(), adc_start_injected_conversion()
 */
void scenario_5_injected_conversion(void)
{
    print_separator();
    printf("=== 场景5：注入组转换 ===\n\n");

    printf("步骤1：配置GPIO为模拟模式\n");
    gpio_set_analog_mode(PA3);
    gpio_set_analog_mode(PA4);

    printf("步骤2：初始化ADC1\n");
    adc_enable_regulator(ADC_1);
    delay_ms(1);

    printf("步骤3：配置规则序列（通道3）\n");
    adc_regular_config_t reg = {
        .length = 1,
        .sequence = {ADC_CHANNEL_3},
    };
    adc_config_regular_sequence(ADC_1, &reg);

    printf("步骤4：配置注入序列（通道4）\n");
    adc_injected_config_t inj = {
        .length = 1,
        .sequence = {ADC_CHANNEL_4},
    };
    adc_config_injected_sequence(ADC_1, &inj);

    printf("步骤5：配置采样时间\n");
    adc_sampling_config_t smp = {0};
    smp.smp[3] = ADC_SAMPLE_13;
    smp.smp[4] = ADC_SAMPLE_13;
    adc_config_sampling_time(ADC_1, &smp);

    printf("步骤6：使能ADC\n");
    adc_config_resolution(ADC_1, ADC_RES_12BIT);
    adc_enable(ADC_1);

    printf("步骤7：启动规则组转换\n");
    adc_start_regular_conversion(ADC_1);
    while (!adc_is_eog(ADC_1)) { __NOP(); }
    uint16_t reg_val = adc_get_result(ADC_1);
    printf("  规则组结果：%u（%.2fV）\n", reg_val, (float)reg_val * 3.3f / 4095.0f);

    printf("步骤8：软件触发注入组转换\n");
    adc_start_injected_conversion(ADC_1);
    while (!adc_is_jeog(ADC_1)) { __NOP(); }
    uint16_t inj_val = adc_get_injected_result(ADC_1, 0);
    printf("  注入组结果：%u（%.2fV）\n", inj_val, (float)inj_val * 3.3f / 4095.0f);

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景6：模拟看门狗中断
//=============================================================================

/**
 * @brief 场景6：模拟看门狗中断
 *
 * 功能：设定电压阈值窗口，超出时触发中断
 * 硬件连接：PA3 -> 可调电压源
 * API使用：adc_config_watchdog(), adc_register_awd_callback()
 */
void awd_callback_handler(adc_instance_t adc)
{
    adc_awd_event = true;
    printf("  *** 看门狗中断！ADC%d电压超出阈值 ***\n", adc + 1);
}

void scenario_6_analog_watchdog(void)
{
    print_separator();
    printf("=== 场景6：模拟看门狗中断 ===\n\n");

    printf("步骤1：配置GPIO为模拟模式\n");
    gpio_set_analog_mode(PA3);

    printf("步骤2：初始化ADC1通道3\n");
    adc_enable_regulator(ADC_1);
    delay_ms(1);

    adc_regular_config_t reg = {
        .length = 1,
        .sequence = {ADC_CHANNEL_3},
    };
    adc_config_regular_sequence(ADC_1, &reg);

    adc_sampling_config_t smp = {0};
    smp.smp[3] = ADC_SAMPLE_13;
    adc_config_sampling_time(ADC_1, &smp);

    printf("步骤3：配置模拟看门狗（1.0V - 2.0V窗口）\n");
    adc_watchdog_config_t awd = {
        .mode = ADC_AWD_ALL_REGULAR,
        .channel = ADC_CHANNEL_3,
        .high_threshold = 2483,   // 2.0V = 2483 * 3.3/4095
        .low_threshold = 1241,    // 1.0V = 1241 * 3.3/4095
        .high_threshold_diff = 0,
        .low_threshold_diff = 0,
        .enable_interrupt = true,
    };
    adc_config_watchdog(ADC_1, &awd);
    printf("  结果：看门狗已配置\n");

    printf("步骤4：注册看门狗中断回调\n");
    adc_register_awd_callback(ADC_1, awd_callback_handler);

    printf("步骤5：使能ADC中断\n");
    adc_enable_interrupt(ADC_1, ADC_INT_AWD);

    printf("步骤6：使能ADC并启动转换\n");
    adc_config_resolution(ADC_1, ADC_RES_12BIT);
    adc_enable(ADC_1);
    adc_enable_continuous_mode(ADC_1, true);
    adc_start_regular_conversion(ADC_1);

    printf("  等待看门狗事件（调整输入电压以触发）...\n");
    delay_ms(100);
    if (adc_awd_event) {
        printf("  结果：看门狗功能正常\n");
    } else {
        printf("  结果：未触发（电压在窗口内）\n");
    }

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景7：温度传感器读取
//=============================================================================

/**
 * @brief 场景7：温度传感器读取
 *
 * 功能：读取内部温度传感器（ADC3通道18）
 * API使用：adc_init_internal_channel()
 *
 * @note 温度传感器连接到ADC3_INP[18]
 * @note 温度计算公式：T(C) = ((V_sense - V_25) / Avg_Slope) + 25
 */
void scenario_7_temperature_sensor(void)
{
    print_separator();
    printf("=== 场景7：温度传感器读取 ===\n\n");

    printf("步骤1：使能内部温度传感器\n");
    if (!adc_init_internal_channel(ADC_3, ADC_INTERNAL_TEMPSENSOR, ADC_SAMPLE_160)) {
        printf("  错误：温度传感器初始化失败\n");
        print_separator();
        return;
    }
    printf("  结果：温度传感器已使能\n");

    printf("\n步骤2：等待稳定（1.5us）\n");
    __NOP(); __NOP(); __NOP();

    printf("步骤3：启动转换并读取\n");
    adc_start_regular_conversion(ADC_3);
    while (!adc_is_eoc(ADC_3)) { __NOP(); }
    uint16_t val = adc_get_result(ADC_3);
    printf("  原始ADC值：%u\n", val);

    // 典型值：V_25 = 1.43V, Avg_Slope = 4.3mV/C
    float voltage = (float)val * 3.3f / 4095.0f;
    float temp = ((voltage - 1.43f) / 0.0043f) + 25.0f;
    printf("  电压：%.3fV\n", voltage);
    printf("  温度：%.1f°C\n", temp);

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景8：双ADC同步模式
//=============================================================================

/**
 * @brief 场景8：双ADC同步模式
 *
 * 功能：ADC1（主）和ADC2（从）同步采样
 * 硬件连接：PA3（ADC1）、PB1（ADC2）
 * API使用：adc_config_dual_mode()
 *
 * @note 仅ADC1（主）和ADC2（从）支持双ADC模式
 */
void scenario_8_dual_adc_sync(void)
{
    print_separator();
    printf("=== 场景8：双ADC同步模式 ===\n\n");

    printf("步骤1：配置GPIO为模拟模式\n");
    gpio_set_analog_mode(PA3);   // ADC1通道3
    gpio_set_analog_mode(PB1);   // ADC2通道9

    printf("步骤2：配置双ADC模式（规则同步）\n");
    adc_dual_config_t dual = {
        .mode = ADC_DUAL_REG_SYNC_ONLY,
        .dma_mode = ADC_DUAL_DMA_COMBINED_12,
        .delay = 0,
    };
    if (!adc_config_dual_mode(&dual)) {
        printf("  错误：双ADC模式配置失败\n");
        print_separator();
        return;
    }
    printf("  结果：双ADC模式已配置\n");

    printf("步骤3：使能ADC1时钟和稳压器\n");
    adc_enable_regulator(ADC_1);
    delay_ms(1);

    printf("步骤4：配置ADC1规则序列（通道3）\n");
    adc_regular_config_t reg = {
        .length = 1,
        .sequence = {ADC_CHANNEL_3},
    };
    adc_config_regular_sequence(ADC_1, &reg);

    adc_sampling_config_t smp = {0};
    smp.smp[3] = ADC_SAMPLE_13;
    adc_config_sampling_time(ADC_1, &smp);

    printf("步骤5：使能ADC1和ADC2\n");
    adc_config_resolution(ADC_1, ADC_RES_12BIT);
    adc_enable(ADC_1);

    // ADC2在双ADC模式下由主ADC控制序列
    adc_enable_regulator(ADC_2);
    adc_config_resolution(ADC_2, ADC_RES_12BIT);
    adc_enable(ADC_2);

    printf("步骤6：启动转换（主ADC触发从ADC）\n");
    adc_start_regular_conversion(ADC_1);
    while (!adc_is_eog(ADC_1)) { __NOP(); }

    printf("步骤7：读取组合结果\n");
    uint32_t combined = adc_get_dual_result();
    uint16_t adc1_val = combined & 0xFFFF;
    uint16_t adc2_val = combined >> 16;
    printf("  ADC1结果：%u（%.2fV）\n", adc1_val, (float)adc1_val * 3.3f / 4095.0f);
    printf("  ADC2结果：%u（%.2fV）\n", adc2_val, (float)adc2_val * 3.3f / 4095.0f);

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景9：过采样降噪
//=============================================================================

/**
 * @brief 场景9：过采样降噪
 *
 * 功能：使用16倍过采样提高测量分辨率
 * 硬件连接：PA3 -> 电压源
 * API使用：adc_config_oversampling()
 */
void scenario_9_oversampling(void)
{
    print_separator();
    printf("=== 场景9：过采样降噪 ===\n\n");

    printf("步骤1：配置GPIO为模拟模式\n");
    gpio_set_analog_mode(PA3);

    printf("步骤2：初始化ADC1\n");
    adc_enable_regulator(ADC_1);
    delay_ms(1);

    printf("步骤3：配置规则序列\n");
    adc_regular_config_t reg = {
        .length = 1,
        .sequence = {ADC_CHANNEL_3},
    };
    adc_config_regular_sequence(ADC_1, &reg);

    adc_sampling_config_t smp = {0};
    smp.smp[3] = ADC_SAMPLE_13;
    adc_config_sampling_time(ADC_1, &smp);

    printf("步骤4：配置过采样（16倍，右移4位）\n");
    adc_oversampling_config_t os = {
        .enable_regular = true,
        .enable_injected = false,
        .rate = ADC_OVERSAMPLE_16X,
        .shift = ADC_OVSS_SHIFT4,
        .trigger_mode = ADC_TROVS_MULTI,
    };
    adc_config_oversampling(ADC_1, &os);
    printf("  结果：过采样已配置（等效14位分辨率）\n");

    printf("步骤5：使能ADC并采样\n");
    adc_config_resolution(ADC_1, ADC_RES_12BIT);
    adc_enable(ADC_1);
    adc_start_regular_conversion(ADC_1);
    while (!adc_is_eoc(ADC_1)) { __NOP(); }

    uint16_t val = adc_get_result(ADC_1);
    printf("  过采样结果：%u\n", val);
    printf("  无过采样时的原始值约在附近抖动\n");

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景10：差分输入模式
//=============================================================================

/**
 * @brief 场景10：差分输入模式
 *
 * 功能：配置ADC1通道3为差分输入模式
 * 硬件连接：PA3（正极INP）、PA2（负极INN，需外部连接）
 * API使用：adc_config_channel_input_mode()
 *
 * @note 差分模式下ADC范围为-VREF/2 到 +VREF/2
 */
void scenario_10_differential_mode(void)
{
    print_separator();
    printf("=== 场景10：差分输入模式 ===\n\n");

    printf("步骤1：配置GPIO为模拟模式\n");
    gpio_set_analog_mode(PA2);   // ADC1_IN2（差分负极）
    gpio_set_analog_mode(PA3);   // ADC1_IN3（差分正极）

    printf("步骤2：配置ADC1通道3为差分输入\n");
    adc_enable_regulator(ADC_1);

    if (!adc_config_channel_input_mode(ADC_1, ADC_CHANNEL_3, ADC_INPUT_DIFFERENTIAL)) {
        printf("  错误：差分模式配置失败\n");
        print_separator();
        return;
    }
    printf("  结果：差分模式已配置\n");

    printf("步骤3：配置规则序列\n");
    adc_regular_config_t reg = {
        .length = 1,
        .sequence = {ADC_CHANNEL_3},
    };
    adc_config_regular_sequence(ADC_1, &reg);

    adc_sampling_config_t smp = {0};
    smp.smp[3] = ADC_SAMPLE_13;
    adc_config_sampling_time(ADC_1, &smp);

    printf("步骤4：差分模式校准\n");
    if (!adc_calibrate(ADC_1, true)) {
        printf("  错误：差分校准失败\n");
    } else {
        printf("  结果：差分校准完成\n");
    }

    printf("步骤5：使能ADC并采样\n");
    adc_config_resolution(ADC_1, ADC_RES_12BIT);
    adc_enable(ADC_1);
    adc_start_regular_conversion(ADC_1);
    while (!adc_is_eoc(ADC_1)) { __NOP(); }

    uint16_t val = adc_get_result(ADC_1);
    // 差分模式下，0x800 = 0V，>0x800为正，<0x800为负
    printf("  差分结果：%u\n", val);

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 主函数
//=============================================================================

int main(void)
{
    system_clock_init();

    printf("\n");
    print_separator();
    printf("ADC模块示例程序\n");
    printf("当前场景: %d\n", SCENARIO);
    print_separator();
    printf("\n");

    switch (SCENARIO) {
        case 1:  scenario_1_single_conversion();    break;
        case 2:  scenario_2_continuous_conversion(); break;
        case 3:  scenario_3_multi_channel_scan();    break;
        case 4:  scenario_4_multi_channel_dma();     break;
        case 5:  scenario_5_injected_conversion();   break;
        case 6:  scenario_6_analog_watchdog();       break;
        case 7:  scenario_7_temperature_sensor();    break;
        case 8:  scenario_8_dual_adc_sync();         break;
        case 9:  scenario_9_oversampling();          break;
        case 10: scenario_10_differential_mode();    break;
        default:
            printf("错误：无效的场景编号 %d\n", SCENARIO);
            break;
    }

    while (1) {
        __WFI();
    }
}
