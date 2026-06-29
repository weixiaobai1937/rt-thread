/**
 * @file dac_example.c
 * @brief ACM32P4 DAC驱动示例代码
 *
 * 演示10个场景：
 * 1. 单通道直流电压输出
 * 2. 双通道同步输出
 * 3. 外部触发转换
 * 4. LFSR噪声生成
 * 5. 三角波生成
 * 6. 锯齿波生成
 * 7. DMA数据传输
 * 8. DMA双数据模式
 * 9. 有符号数格式
 * 10. 采样保持模式
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

static volatile bool dac_dma_underrun_event = false;

//=============================================================================
// 场景1：单通道直流电压输出
//=============================================================================

/**
 * @brief 场景1：单通道直流电压输出
 *
 * 功能：在DAC1通道1（PA4）输出指定直流电压
 * 硬件连接：PA4 -> 电压表或示波器
 * API使用：dac_init_channel_basic() + dac_set_data_12r()
 */
void scenario_1_basic_dc_output(void)
{
    print_separator();
    printf("=== 场景1：单通道直流电压输出 ===\n\n");

    printf("步骤1：配置GPIO为模拟模式\n");
    gpio_set_analog_mode(PA4);

    printf("步骤2：初始化DAC1通道1（12位右对齐）\n");
    if (!dac_init_channel_basic(DAC_1, DAC_CHANNEL_1, DAC_ALIGN_12B_RIGHT)) {
        printf("  错误：DAC初始化失败\n");
        print_separator();
        return;
    }
    printf("  结果：DAC1通道1初始化成功\n");

    printf("\n步骤3：输出半量程电压（约1.65V）\n");
    dac_set_data_12r(DAC_1, DAC_CHANNEL_1, 2048);
    printf("  结果：DOR1 = %u（约%.2fV）\n",
           dac_get_output_data(DAC_1, DAC_CHANNEL_1),
           (float)dac_get_output_data(DAC_1, DAC_CHANNEL_1) * 3.3f / 4095.0f);

    printf("\n步骤4：调整输出到75%%量程\n");
    delay_ms(100);
    dac_set_data_12r(DAC_1, DAC_CHANNEL_1, 3072);
    printf("  结果：DOR1 = %u（约%.2fV）\n",
           dac_get_output_data(DAC_1, DAC_CHANNEL_1),
           (float)dac_get_output_data(DAC_1, DAC_CHANNEL_1) * 3.3f / 4095.0f);

    printf("\n步骤5：调整输出到25%%量程\n");
    delay_ms(100);
    dac_set_data_12r(DAC_1, DAC_CHANNEL_1, 1024);
    printf("  结果：DOR1 = %u（约%.2fV）\n",
           dac_get_output_data(DAC_1, DAC_CHANNEL_1),
           (float)dac_get_output_data(DAC_1, DAC_CHANNEL_1) * 3.3f / 4095.0f);

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景2：双通道同步输出
//=============================================================================

/**
 * @brief 场景2：双通道同步输出
 *
 * 功能：DAC1通道1和通道2同步输出不同的直流电压
 * 硬件连接：PA4（DAC1_OUT1）、PA5（DAC1_OUT2）
 * API使用：dac_init_dual_channel() + dac_set_dual_data_12r()
 */
void scenario_2_dual_channel_output(void)
{
    print_separator();
    printf("=== 场景2：双通道同步输出 ===\n\n");

    printf("步骤1：配置GPIO为模拟模式\n");
    gpio_set_analog_mode(PA4);
    gpio_set_analog_mode(PA5);

    printf("步骤2：初始化DAC1双通道（12位右对齐）\n");
    if (!dac_init_dual_channel(DAC_1, DAC_ALIGN_12B_RIGHT)) {
        printf("  错误：DAC初始化失败\n");
        print_separator();
        return;
    }
    printf("  结果：DAC1双通道初始化成功\n");

    printf("\n步骤3：同步输出双通道数据\n");
    dac_set_dual_data_12r(DAC_1, 1024, 3072);
    printf("  通道1：%u（约%.2fV）\n",
           dac_get_output_data(DAC_1, DAC_CHANNEL_1),
           (float)dac_get_output_data(DAC_1, DAC_CHANNEL_1) * 3.3f / 4095.0f);
    printf("  通道2：%u（约%.2fV）\n",
           dac_get_output_data(DAC_1, DAC_CHANNEL_2),
           (float)dac_get_output_data(DAC_1, DAC_CHANNEL_2) * 3.3f / 4095.0f);

    printf("\n步骤4：调整双通道输出\n");
    delay_ms(100);
    dac_set_dual_data_12r(DAC_1, 2048, 2048);
    printf("  通道1：%u（约%.2fV）\n",
           dac_get_output_data(DAC_1, DAC_CHANNEL_1),
           (float)dac_get_output_data(DAC_1, DAC_CHANNEL_1) * 3.3f / 4095.0f);
    printf("  通道2：%u（约%.2fV）\n",
           dac_get_output_data(DAC_1, DAC_CHANNEL_2),
           (float)dac_get_output_data(DAC_1, DAC_CHANNEL_2) * 3.3f / 4095.0f);

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景3：外部触发转换
//=============================================================================

/**
 * @brief 场景3：外部触发转换
 *
 * 功能：使用TIM2 TRGO触发DAC1通道1转换
 * 硬件连接：PA4 -> 示波器
 * API使用：dac_init_channel_triggered() + dac_config_trigger()
 *
 * @note 需要预先配置TIM2输出TRGO信号
 */
void scenario_3_triggered_conversion(void)
{
    print_separator();
    printf("=== 场景3：外部触发转换 ===\n\n");

    printf("步骤1：配置GPIO为模拟模式\n");
    gpio_set_analog_mode(PA4);

    printf("步骤2：初始化DAC1通道1（触发模式，TIM2 TRGO）\n");
    if (!dac_init_channel_triggered(DAC_1, DAC_CHANNEL_1,
                                     DAC_TRIG_TIM2_TRGO, DAC_ALIGN_12B_RIGHT)) {
        printf("  错误：DAC初始化失败\n");
        print_separator();
        return;
    }
    printf("  结果：DAC1通道1触发模式已配置\n");

    printf("\n步骤3：预设转换数据\n");
    dac_set_data_12r(DAC_1, DAC_CHANNEL_1, 2048);
    printf("  结果：数据已写入DHR，等待触发\n");

    printf("\n步骤4：配置触发源\n");
    dac_config_trigger(DAC_1, DAC_CHANNEL_1, DAC_TRIG_TIM2_TRGO, true);
    printf("  结果：触发源已设置为TIM2 TRGO\n");
    printf("  注意：需要使能TIM2并配置TRGO输出\n");

    printf("\n步骤5：软件触发作为演示\n");
    dac_software_trigger(DAC_1, DAC_CHANNEL_1);
    printf("  结果：软件触发完成，DOR1 = %u\n",
           dac_get_output_data(DAC_1, DAC_CHANNEL_1));

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景4：LFSR噪声生成
//=============================================================================

/**
 * @brief 场景4：LFSR噪声生成
 *
 * 功能：在直流偏置上叠加LFSR伪随机噪声
 * 硬件连接：PA4 -> 示波器
 * API使用：dac_config_waveform() + dac_config_channel()
 *
 * @note LFSR模式需要使能硬件触发（TENx=1）
 */
void scenario_4_lfsr_noise(void)
{
    print_separator();
    printf("=== 场景4：LFSR噪声生成 ===\n\n");

    printf("步骤1：配置GPIO为模拟模式\n");
    gpio_set_analog_mode(PA4);

    printf("步骤2：初始化DAC1通道1\n");
    if (!dac_init_channel_basic(DAC_1, DAC_CHANNEL_1, DAC_ALIGN_12B_RIGHT)) {
        printf("  错误：DAC初始化失败\n");
        print_separator();
        return;
    }

    printf("步骤3：配置LFSR噪声波形\n");
    dac_channel_config_t cfg = {
        .trigger_source = DAC_TRIG_SW,
        .trigger_enable = true,
        .waveform = DAC_WAVE_LFSR,
        .mamp = DAC_MAMP_7,
        .output_mode = DAC_MODE_NORMAL_BUF_PAD,
        .dma_enable = false,
        .dma_double_data = false,
        .signed_format = DAC_FORMAT_UNSIGNED,
    };
    dac_config_channel(DAC_1, DAC_CHANNEL_1, &cfg);
    printf("  结果：LFSR噪声模式已配置（MAMP=7，LFSR[2:0]有效）\n");

    printf("步骤4：设置直流偏置并触发\n");
    dac_set_data_12r(DAC_1, DAC_CHANNEL_1, 2048);
    dac_software_trigger(DAC_1, DAC_CHANNEL_1);
    printf("  结果：LFSR噪声输出中，偏置值=2048\n");

    printf("步骤5：读取当前输出\n");
    printf("  DOR1 = %u\n", dac_get_output_data(DAC_1, DAC_CHANNEL_1));

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景5：三角波生成
//=============================================================================

/**
 * @brief 场景5：三角波生成
 *
 * 功能：生成三角波叠加在直流偏置上
 * 硬件连接：PA4 -> 示波器
 * API使用：dac_config_waveform() + dac_config_channel()
 *
 * @note 三角波模式需要使能硬件触发（TENx=1）
 */
void scenario_5_triangle_wave(void)
{
    print_separator();
    printf("=== 场景5：三角波生成 ===\n\n");

    printf("步骤1：配置GPIO为模拟模式\n");
    gpio_set_analog_mode(PA4);

    printf("步骤2：初始化DAC1通道1\n");
    if (!dac_init_channel_basic(DAC_1, DAC_CHANNEL_1, DAC_ALIGN_12B_RIGHT)) {
        printf("  错误：DAC初始化失败\n");
        print_separator();
        return;
    }

    printf("步骤3：配置三角波波形（幅值=15）\n");
    dac_channel_config_t cfg = {
        .trigger_source = DAC_TRIG_SW,
        .trigger_enable = true,
        .waveform = DAC_WAVE_TRIANGLE,
        .mamp = DAC_MAMP_15,
        .output_mode = DAC_MODE_NORMAL_BUF_PAD,
        .dma_enable = false,
        .dma_double_data = false,
        .signed_format = DAC_FORMAT_UNSIGNED,
    };
    dac_config_channel(DAC_1, DAC_CHANNEL_1, &cfg);
    printf("  结果：三角波模式已配置（幅值=15 LSB）\n");

    printf("步骤4：设置直流偏置并触发\n");
    dac_set_data_12r(DAC_1, DAC_CHANNEL_1, 2048);
    dac_software_trigger(DAC_1, DAC_CHANNEL_1);

    printf("步骤5：连续触发观察三角波\n");
    for (int i = 0; i < 32; i++) {
        dac_software_trigger(DAC_1, DAC_CHANNEL_1);
        delay_us(10);
    }
    printf("  结果：三角波已生成，在偏置附近上下波动\n");

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景6：锯齿波生成
//=============================================================================

/**
 * @brief 场景6：锯齿波生成
 *
 * 功能：生成递增锯齿波
 * 硬件连接：PA4 -> 示波器
 * API使用：dac_config_sawtooth() + dac_config_waveform()
 *
 * @note 锯齿波模式下触发不受TENx控制
 */
void scenario_6_sawtooth_wave(void)
{
    print_separator();
    printf("=== 场景6：锯齿波生成 ===\n\n");

    printf("步骤1：配置GPIO为模拟模式\n");
    gpio_set_analog_mode(PA4);

    printf("步骤2：初始化DAC1通道1\n");
    if (!dac_init_channel_basic(DAC_1, DAC_CHANNEL_1, DAC_ALIGN_12B_RIGHT)) {
        printf("  错误：DAC初始化失败\n");
        print_separator();
        return;
    }

    printf("步骤3：配置锯齿波模式\n");
    dac_config_waveform(DAC_1, DAC_CHANNEL_1, DAC_WAVE_SAWTOOTH, DAC_MAMP_4095);

    printf("步骤4：配置锯齿波参数（递增，步进256）\n");
    dac_sawtooth_config_t st = {
        .reset_value = 0,
        .increment = 0x0100,       // 12.4格式，每次递增16
        .direction = true,          // 递增
        .inc_trigger = DAC_ST_TRIG_SW,
        .reset_trigger = DAC_TRIG_SW,
    };
    dac_config_sawtooth(DAC_1, DAC_CHANNEL_1, &st);
    printf("  结果：锯齿波已配置（复位值=0，步进=16/16=1）\n");

    printf("步骤5：连续递增触发生成锯齿波\n");
    for (int i = 0; i < 20; i++) {
        dac_software_trigger_swtrigb(DAC_1, DAC_CHANNEL_1);
        delay_us(10);
    }
    printf("  当前DOR1 = %u\n", dac_get_output_data(DAC_1, DAC_CHANNEL_1));

    printf("步骤6：复位锯齿波计数器\n");
    dac_software_trigger(DAC_1, DAC_CHANNEL_1);
    printf("  复位后DOR1 = %u\n", dac_get_output_data(DAC_1, DAC_CHANNEL_1));

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景7：DMA数据传输
//=============================================================================

/**
 * @brief 场景7：DMA数据传输
 *
 * 功能：使用DMA自动传输数据到DAC
 * 硬件连接：PA4 -> 示波器
 * API使用：dac_enable_dma()
 *
 * @note 需要单独配置DMA控制器（请参考dma_example.c）
 */
void scenario_7_dma_transfer(void)
{
    print_separator();
    printf("=== 场景7：DMA数据传输 ===\n\n");

    printf("步骤1：配置GPIO为模拟模式\n");
    gpio_set_analog_mode(PA4);

    printf("步骤2：初始化DAC1通道1（触发模式）\n");
    if (!dac_init_channel_triggered(DAC_1, DAC_CHANNEL_1,
                                     DAC_TRIG_TIM2_TRGO, DAC_ALIGN_12B_RIGHT)) {
        printf("  错误：DAC初始化失败\n");
        print_separator();
        return;
    }
    printf("  结果：DAC1通道1触发模式已配置\n");

    printf("步骤3：使能DMA请求\n");
    dac_enable_dma(DAC_1, DAC_CHANNEL_1, true);
    printf("  结果：DMA请求已使能\n");
    printf("  注意：需要额外配置DMA控制器通道（参见dma_example.c）\n");

    printf("步骤4：使能DMA下溢中断\n");
    dac_enable_interrupt(DAC_1, DAC_INT_DMA_UNDERRUN1);
    printf("  结果：DMA下溢中断已使能\n");

    printf("步骤5：预设第一笔数据\n");
    dac_set_data_12r(DAC_1, DAC_CHANNEL_1, 1000);
    printf("  结果：初始数据已写入\n");
    printf("  （后续数据将由DMA自动传输）\n");

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景8：DMA双数据模式
//=============================================================================

/**
 * @brief 场景8：DMA双数据模式
 *
 * 功能：DMA双数据模式，一次DMA传输两个12位数据
 * 硬件连接：PA4 -> 示波器
 * API使用：dac_enable_dma_double_data() + dac_set_data_double()
 *
 * @note 每两次外部触发产生一次DMA请求
 */
void scenario_8_dma_double_data(void)
{
    print_separator();
    printf("=== 场景8：DMA双数据模式 ===\n\n");

    printf("步骤1：配置GPIO为模拟模式\n");
    gpio_set_analog_mode(PA4);

    printf("步骤2：初始化DAC1通道1\n");
    if (!dac_init_channel_basic(DAC_1, DAC_CHANNEL_1, DAC_ALIGN_12B_RIGHT)) {
        printf("  错误：DAC初始化失败\n");
        print_separator();
        return;
    }

    printf("步骤3：配置触发模式\n");
    dac_config_trigger(DAC_1, DAC_CHANNEL_1, DAC_TRIG_SW, true);

    printf("步骤4：使能DMA双数据模式\n");
    dac_enable_dma_double_data(DAC_1, DAC_CHANNEL_1, true);
    dac_enable_dma(DAC_1, DAC_CHANNEL_1, true);
    printf("  结果：DMA双数据模式已使能\n");

    printf("步骤5：写入双数据（A和B）\n");
    dac_set_data_double(DAC_1, DAC_CHANNEL_1, DAC_ALIGN_12B_RIGHT, 500, 3500);
    printf("  结果：双数据已写入\n");

    printf("步骤6：第一次软件触发（输出数据A）\n");
    dac_software_trigger(DAC_1, DAC_CHANNEL_1);
    printf("  DOR1 = %u（数据A）\n", dac_get_output_data(DAC_1, DAC_CHANNEL_1));
    printf("  DORSTAT1 = %u\n", dac_get_dorstat(DAC_1, DAC_CHANNEL_1));

    printf("步骤7：第二次软件触发（输出数据B）\n");
    dac_software_trigger(DAC_1, DAC_CHANNEL_1);
    printf("  DOR1 = %u（数据B）\n", dac_get_output_data(DAC_1, DAC_CHANNEL_1));
    printf("  DORSTAT1 = %u\n", dac_get_dorstat(DAC_1, DAC_CHANNEL_1));

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景9：有符号数格式
//=============================================================================

/**
 * @brief 场景9：有符号数格式
 *
 * 功能：使用有符号数（2s补码）格式输出
 * 硬件连接：PA4 -> 电压表
 * API使用：dac_config_signed_format()
 *
 * @note 有符号模式下，0x7FF映射到0xFFF（最大值），0x800映射到0x000（最小值）
 */
void scenario_9_signed_format(void)
{
    print_separator();
    printf("=== 场景9：有符号数格式 ===\n\n");

    printf("步骤1：配置GPIO为模拟模式\n");
    gpio_set_analog_mode(PA4);

    printf("步骤2：初始化DAC1通道1\n");
    if (!dac_init_channel_basic(DAC_1, DAC_CHANNEL_1, DAC_ALIGN_12B_RIGHT)) {
        printf("  错误：DAC初始化失败\n");
        print_separator();
        return;
    }

    printf("步骤3：使能有符号数模式\n");
    dac_config_signed_format(DAC_1, DAC_CHANNEL_1, DAC_FORMAT_SIGNED);
    printf("  结果：有符号数模式已使能\n");

    printf("步骤4：测试有符号数输出\n");

    printf("  写入 0x000 -> DOR1 = %u（中值，约1.65V）\n",
           dac_get_output_data(DAC_1, DAC_CHANNEL_1));
    dac_set_data_12r(DAC_1, DAC_CHANNEL_1, 0x000);

    delay_ms(10);
    printf("  写入 0x7FF（最大正数）-> DOR1 = %u（约3.3V）\n",
           dac_get_output_data(DAC_1, DAC_CHANNEL_1));
    dac_set_data_12r(DAC_1, DAC_CHANNEL_1, 0x7FF);

    delay_ms(10);
    printf("  写入 0x800（最大负数）-> DOR1 = %u（约0V）\n",
           dac_get_output_data(DAC_1, DAC_CHANNEL_1));
    dac_set_data_12r(DAC_1, DAC_CHANNEL_1, 0x800);

    printf("步骤5：恢复无符号模式\n");
    dac_config_signed_format(DAC_1, DAC_CHANNEL_1, DAC_FORMAT_UNSIGNED);

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景10：采样保持模式
//=============================================================================

/**
 * @brief 场景10：采样保持模式
 *
 * 功能：配置DAC为采样保持模式以降低功耗
 * 硬件连接：PA4 -> 示波器
 * API使用：dac_config_output_mode() + dac_config_sample_hold()
 *
 * @note 采样保持模式适用于低频输出场景，在保持阶段关闭模拟电路
 */
void scenario_10_sample_and_hold(void)
{
    print_separator();
    printf("=== 场景10：采样保持模式 ===\n\n");

    printf("步骤1：配置GPIO为模拟模式\n");
    gpio_set_analog_mode(PA4);

    printf("步骤2：初始化DAC1通道1\n");
    if (!dac_init_channel_basic(DAC_1, DAC_CHANNEL_1, DAC_ALIGN_12B_RIGHT)) {
        printf("  错误：DAC初始化失败\n");
        print_separator();
        return;
    }

    printf("步骤3：配置采样保持输出模式\n");
    dac_config_output_mode(DAC_1, DAC_CHANNEL_1, DAC_MODE_SH_BUF_PAD);
    printf("  结果：采样保持模式已配置（Buffer使能，输出到PAD）\n");

    printf("步骤4：配置采样保持时序\n");
    dac_sh_config_t sh = {
        .sample_time = 11,    // 12个RC32K周期 ≈ 375us
        .hold_time = 62,      // 62个RC32K周期 ≈ 2ms
        .refresh_time = 4,    // 4个RC32K周期 ≈ 125us
    };
    dac_config_sample_hold(DAC_1, DAC_CHANNEL_1, &sh);
    printf("  结果：采样时间=375us, 保持时间=2ms, 刷新时间=125us\n");

    printf("步骤5：输出数据\n");
    dac_set_data_12r(DAC_1, DAC_CHANNEL_1, 2048);
    printf("  结果：DOR1 = %u\n", dac_get_output_data(DAC_1, DAC_CHANNEL_1));

    printf("\n步骤6：禁用采样保持模式\n");
    dac_config_sample_hold(DAC_1, DAC_CHANNEL_1, NULL);
    printf("  结果：采样保持已禁用\n");

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
    printf("DAC模块示例程序\n");
    printf("当前场景: %d\n", SCENARIO);
    print_separator();
    printf("\n");

    switch (SCENARIO) {
        case 1:  scenario_1_basic_dc_output();      break;
        case 2:  scenario_2_dual_channel_output();   break;
        case 3:  scenario_3_triggered_conversion();  break;
        case 4:  scenario_4_lfsr_noise();            break;
        case 5:  scenario_5_triangle_wave();         break;
        case 6:  scenario_6_sawtooth_wave();         break;
        case 7:  scenario_7_dma_transfer();          break;
        case 8:  scenario_8_dma_double_data();       break;
        case 9:  scenario_9_signed_format();         break;
        case 10: scenario_10_sample_and_hold();       break;
        default:
            printf("错误：无效的场景编号 %d\n", SCENARIO);
            break;
    }

    while (1) {
        __WFI();
    }
}
