/**
 * @file tim_example.c
 * @brief ACM32P4 TIM驱动示例代码
 *
 * 演示10个场景：
 * 1. 基础PWM输出
 * 2. 输入捕获测量脉宽
 * 3. 编码器接口
 * 4. 互补PWM + 死区时间
 * 5. 主从同步
 * 6. DMA Burst传输
 * 7. 刹车保护功能
 * 8. PWM输入模式
 * 9. 单脉冲模式
 * 10. 周期定时中断
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 * @date 2026-02-11
 */

#include "acm32p4.h"
#include "hardware/system.h"
#include "SEGGER_RTT.h"

//=============================================================================
// 配置选项
//=============================================================================

#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)

// 选择要运行的场景（修改此值来切换场景）
#define SCENARIO 10

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
// 场景1：基础PWM输出（边沿对齐）
//=============================================================================

/**
 * @brief 场景1：基础PWM输出
 *
 * 功能：在PA8输出10kHz、50%占空比的PWM信号
 * 硬件连接：PA8 -> LED或示波器
 * API使用：tim_init_pwm()（第1层快速初始化API）
 */
void scenario_1_basic_pwm(void)
{
    // 步骤1：配置GPIO为TIM1_CH1复用功能（PA8）
    gpio_init(PA8);
    gpio_set_dir(PA8, GPIO_OUT);
    gpio_set_function(PA8, GPIO_AF_2);

    // 步骤2：快速初始化：TIM1通道1，10kHz，50%占空比
    tim_init_pwm(TIM_1, TIM_CH1, 10000, 50);

    // PWM已自动启动，可以通过以下API动态调整占空比
    tim_set_duty(TIM_1, TIM_CH1, 75); // 调整为75%占空比

    printf("场景1：10kHz PWM输出（占空比75%%）\n");
}

//=============================================================================
// 场景2：输入捕获 - 测量脉宽
//=============================================================================

static volatile uint32_t capture_value1 = 0;
static volatile uint32_t capture_value2 = 0;
static volatile bool capture_done = false;

void tim2_cc_callback(tim_instance_t tim, tim_channel_t channel)
{
    static uint8_t capture_count = 0;

    if (capture_count == 0)
    {
        capture_value1 = tim_get_capture(TIM_2, TIM_CH1);
        capture_count = 1;
    }
    else
    {
        capture_value2 = tim_get_capture(TIM_2, TIM_CH1);
        capture_done = true;
        capture_count = 0;
    }
}

/**
 * @brief 场景2：输入捕获测量脉宽
 *
 * 功能：捕获PA0输入信号的脉宽
 * 硬件连接：PA0 -> 外部信号源
 * API使用：tim_init_input_capture(), tim_register_cc_callback()
 */
void scenario_2_input_capture(void)
{
    // 步骤1：配置GPIO为TIM2_CH1复用功能（PA0）
    gpio_init(PA0);
    gpio_set_dir(PA0, GPIO_IN);
    gpio_set_function(PA0, GPIO_AF_2); // TIM2_CH1

    // 步骤2：快速初始化输入捕获
    tim_init_input_capture(TIM_2, TIM_CH1, TIM_IC_RISING, 8);

    // 注册回调函数
    tim_register_cc_callback(TIM_2, TIM_CH1, tim2_cc_callback);

    // 使能捕获中断
    tim_enable_interrupt(TIM_2, TIM_INT_CC1);

    printf("场景2：输入捕获初始化完成，等待信号...\n");

    // 等待捕获完成
    while (!capture_done)
        ;

    // 计算脉宽
    uint32_t pulse_width = capture_value2 - capture_value1;
    uint32_t tim_clock = 180000000; // 220MHz
    float pulse_width_us = (float)pulse_width / (tim_clock / 1000000);

    printf("捕获完成！脉宽：%.2f us\n", pulse_width_us);
}

//=============================================================================
// 场景3：编码器接口 - 电机位置检测
//=============================================================================

/**
 * @brief 场景3：编码器接口
 *
 * 功能：使用TIM3读取编码器位置
 * 硬件连接：PA6 -> 编码器A相，PA7 -> 编码器B相
 * API使用：tim_init_encoder()
 */
void scenario_3_encoder_interface(void)
{
    // 步骤1：配置GPIO为TIM3编码器输入（PA6=TI1, PA7=TI2）
    gpio_init(PA6);
    gpio_set_dir(PA6, GPIO_IN);
    gpio_set_function(PA6, GPIO_AF_2); // TIM3_CH1

    gpio_init(PA7);
    gpio_set_dir(PA7, GPIO_IN);
    gpio_set_function(PA7, GPIO_AF_2); // TIM3_CH2

    // 步骤2：快速初始化编码器模式
    tim_init_encoder(TIM_3, TIM_ENCODER_MODE_TI12);

    printf("场景3：编码器接口初始化完成\n");

    // 读取编码器位置
    for (int i = 0; i < 10; i++)
    {
        uint32_t position = tim_get_counter(TIM_3);
        bool direction = tim_get_direction(TIM_3);
        printf("位置：%u，方向：%s\n", position, direction ? "反向" : "正向");

        // 延时1秒
        delay_ms(1000);
    }
}

//=============================================================================
// 场景4：互补PWM + 死区时间（电机驱动）
//=============================================================================

/**
 * @brief 场景4：互补PWM + 死区时间
 *
 * 功能：生成互补PWM信号，用于半桥或全桥驱动
 * 硬件连接：PA8 -> 高侧开关，PB13 -> 低侧开关
 * API使用：tim_config_timebase(), tim_config_channel_pwm(),
 *          tim_config_complementary_output()
 */
void scenario_4_complementary_pwm(void)
{
    // 步骤1：配置GPIO为TIM1_CH1和TIM1_CH1N复用功能
    gpio_init(PA8);
    gpio_set_dir(PA8, GPIO_OUT);
    gpio_set_function(PA8, GPIO_AF_2); // TIM1_CH1

    gpio_init(PB13);
    gpio_set_dir(PB13, GPIO_OUT);
    gpio_set_function(PB13, GPIO_AF_2); // TIM1_CH1N

    // 步骤2：配置时基（20kHz）
    tim_timebase_config_t timebase = {
        .prescaler = 0,
        .period = 10999, // 220MHz / 11000 = 20kHz
        .count_mode = TIM_COUNT_UP,
        .clock_division = TIM_CKD_DIV1,
        .repetition_counter = 0};
    tim_config_timebase(TIM_1, &timebase);

    // 配置PWM通道1
    tim_pwm_config_t pwm = {
        .mode = TIM_OC_PWM1,
        .pulse = 5500,                  // 50%占空比
        .polarity = TIM_POLARITY_HIGH,  // 主输出极性
        .npolarity = TIM_POLARITY_HIGH, // 互补输出极性
        .idle_state = TIM_IDLE_RESET,   // 主输出空闲状态
        .nidle_state = TIM_IDLE_RESET,  // 互补输出空闲状态
        .fast_mode = false,             // 快速模式
        .preload = true                 // 预装载
    };
    tim_config_channel_pwm(TIM_1, TIM_CH1, &pwm);

    // 配置互补输出（死区时间1us @ 220MHz）
    tim_complementary_config_t comp = {
        .dead_time = 174,                // 1us死区
        .n_polarity = TIM_POLARITY_HIGH, // 互补输出极性
        .idle_state = TIM_IDLE_RESET,    // 主输出空闲状态
        .nidle_state = TIM_IDLE_RESET,   // 互补输出空闲状态
        .automatic_output = true         // 自动输出使能
    };
    tim_config_complementary_output(TIM_1, TIM_CH1, &comp);

    // 启动PWM
    tim_start_pwm(TIM_1, TIM_CH1);
    tim_enable_complementary_channel(TIM_1, TIM_CH1, true);

    printf("场景4：互补PWM输出（死区1us）\n");
}

//=============================================================================
// 场景5：主从同步 - 多定时器联动
//=============================================================================

/**
 * @brief 场景5：主从同步
 *
 * 功能：TIM1作为主定时器，TIM2作为从定时器，实现同步启动
 * 硬件连接：无需外部连接（内部触发）
 * API使用：tim_config_master_slave()
 */
void scenario_5_master_slave_sync(void)
{
    // 配置TIM1为主定时器（输出更新事件作为TRGO）
    tim_master_slave_config_t master = {
        .slave_mode = TIM_SLAVE_DISABLE, // 从模式禁用
        .trigger_source = TIM_TS_ITR0,   // 内部触发0
        .master_mode = TIM_TRGO_UPDATE,  // 更新事件
        .master_mode2 = TIM_TRGO2_RESET, // 主模式2复位
        .msm_enable = false              // 主从模式使能
    };
    tim_config_master_slave(TIM_1, &master);

    // 配置TIM2为从定时器（TIM1触发启动）
    tim_master_slave_config_t slave = {
        .slave_mode = TIM_SLAVE_TRIGGER,
        .trigger_source = TIM_TS_ITR0,   // TIM1作为触发源
        .master_mode = TIM_TRGO_RESET,   // 主模式复位
        .master_mode2 = TIM_TRGO2_RESET, // 主模式2复位
        .msm_enable = true               // 主从模式使能
    };
    tim_config_master_slave(TIM_2, &slave);

    // 配置两个定时器的时基
    tim_timebase_config_t timebase = {
        .prescaler = 23999,             // 预分频器
        .period = 9999,                 // 1Hz
        .count_mode = TIM_COUNT_UP,     // 向上计数
        .clock_division = TIM_CKD_DIV1, // 不分频
        .repetition_counter = 0         // 重复计数器
    };
    tim_config_timebase(TIM_1, &timebase);
    tim_config_timebase(TIM_2, &timebase);

    // 启动TIM1，TIM2会自动启动
    tim_start(TIM_1);
    printf("场景5：主从同步配置完成\n");
    printf("等待TIM2启动...\n");
    // 等待TIM2启动
    while (!tim_is_running(TIM_2))
        ;

    printf("TIM2启动完成\n");
    printf("TIM1状态：%s\n", tim_is_running(TIM_1) ? "运行中" : "已停止");
    printf("TIM2状态：%s\n", tim_is_running(TIM_2) ? "运行中" : "已停止");
}

//=============================================================================
// 场景6：DMA Burst传输 - 高效更新多个通道
//=============================================================================

uint32_t dma_buffer[4] = {3000, 4500, 6000, 7500}; // 4个通道的CCR值

/**
 * @brief 场景6：DMA Burst传输
 *
 * 功能：使用DMA Burst模式同时更新4个通道的比较值
 * 硬件连接：PA8-PA11 -> 4路PWM输出
 * API使用：tim_config_dma_burst(), tim_enable_dma_request()
 */
void scenario_6_dma_burst(void)
{
    // 步骤1：配置GPIO为TIM1的4个PWM通道
    gpio_init(PA8);
    gpio_set_dir(PA8, GPIO_OUT);
    gpio_set_function(PA8, GPIO_AF_2); // TIM1_CH1

    gpio_init(PA9);
    gpio_set_dir(PA9, GPIO_OUT);
    gpio_set_function(PA9, GPIO_AF_2); // TIM1_CH2

    gpio_init(PA10);
    gpio_set_dir(PA10, GPIO_OUT);
    gpio_set_function(PA10, GPIO_AF_2); // TIM1_CH3

    gpio_init(PA11);
    gpio_set_dir(PA11, GPIO_OUT);
    gpio_set_function(PA11, GPIO_AF_2); // TIM1_CH4

    // 步骤2：配置DMA Burst
    tim_dma_burst_config_t dma_cfg = {
        .base_address = TIM_DMA_BASE_CCR1,
        .burst_length = TIM_DMA_BURST_4TRANSFERS // CCR1-CCR4
    };
    tim_config_dma_burst(TIM_1, &dma_cfg);

    // 使能更新DMA请求
    tim_enable_dma_request(TIM_1, TIM_DMA_UPDATE);

    // 获取DMAR地址（用于DMA配置）
    volatile uint32_t *dmar = tim_get_dma_burst_address(TIM_1);
    printf("场景6：DMA Burst地址：0x%p\n", (void *)dmar);

    // 注：实际的DMA配置需要使用DMA驱动
    // 此处仅展示TIM侧的配置
}

//=============================================================================
// 场景7：刹车保护功能（安全应用）
//=============================================================================

void tim1_break_callback(tim_instance_t tim)
{
    printf("警告：刹车事件触发！电机已停止\n");
    // 可以在此处记录故障日志
}

/**
 * @brief 场景7：刹车保护功能
 *
 * 功能：配置刹车输入，保护电机驱动电路
 * 硬件连接：PA8 -> PWM输出（CH1）
 *          PA6 -> 过流检测信号（低电平有效，BKIN）
 * API使用：tim_config_break(), tim_register_break_callback()
 */
void scenario_7_break_protection(void)
{
    // 步骤1：配置GPIO为TIM1_CH1输出和刹车输入
    gpio_init(PA8);
    gpio_set_dir(PA8, GPIO_OUT);
    gpio_set_function(PA8, GPIO_AF_2); // TIM1_CH1

    gpio_init(PA6);
    gpio_set_dir(PA6, GPIO_IN);
    gpio_set_function(PA6, GPIO_AF_2); // TIM1_BKIN

    // 步骤2：配置时基（20kHz PWM）
    tim_timebase_config_t timebase = {
        .prescaler = 0,                 // 预分频器
        .period = 10999,                // 220MHz / 11000 = 20kHz
        .count_mode = TIM_COUNT_UP,     // 向上计数
        .clock_division = TIM_CKD_DIV1, // 不分频
        .repetition_counter = 0         // 重复计数器
    };
    tim_config_timebase(TIM_1, &timebase);

    // 步骤3：配置PWM通道
    tim_pwm_config_t pwm = {
        .mode = TIM_OC_PWM1,
        .pulse = 5500,                  // 50%占空比
        .polarity = TIM_POLARITY_HIGH,  // 主输出极性
        .npolarity = TIM_POLARITY_HIGH, // 互补输出极性
        .idle_state = TIM_IDLE_RESET,   // 主输出空闲状态
        .nidle_state = TIM_IDLE_RESET,  // 互补输出空闲状态
        .fast_mode = false,             // 快速模式
        .preload = true                 // 预装载
    };
    tim_config_channel_pwm(TIM_1, TIM_CH1, &pwm);

    // 步骤4：配置刹车功能
    tim_break_config_t break_cfg = {
        .brk_enable = true,                    // 刹车功能使能
        .brk_pin_enable = true,                // 刹车引脚使能
        .brk_pin_polarity = TIM_BREAK_POL_LOW, // 低电平触发
        .brk_filter = 3,                       // 刹车滤波器
        .comp1_enable = false,                 // 不使用比较器作为刹车源
        .comp1_polarity = TIM_BREAK_POL_HIGH,  // 比较器1极性
        .comp2_enable = false,                 // 不使用比较器作为刹车源
        .comp2_polarity = TIM_BREAK_POL_HIGH,  // 比较器2极性
        .comp3_enable = false,                 // 不使用比较器作为刹车源
        .comp3_polarity = TIM_BREAK_POL_HIGH,  // 比较器3极性
        .comp4_enable = false,                 // 不使用比较器作为刹车源
        .comp4_polarity = TIM_BREAK_POL_HIGH,  // 比较器4极性
        .automatic_output = true,              // 故障恢复后自动输出
        .off_state_idle = TIM_OFF_STATE_HIZ,   // 空闲状态时输出高阻态
        .off_state_run = TIM_OFF_STATE_HIZ,    // 运行状态时输出高阻态
        .lock_level = TIM_LOCK_LEVEL1          // 锁定级别
    };
    tim_config_break(TIM_1, &break_cfg);

    // 步骤5：注册刹车回调并使能中断
    tim_register_break_callback(TIM_1, tim1_break_callback);
    tim_enable_interrupt(TIM_1, TIM_INT_BREAK);
    NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);
    NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, 0);

    // 步骤6：启动PWM输出
    tim_enable_channel(TIM_1, TIM_CH1, true);
    tim_enable_main_output(TIM_1);
    tim_start(TIM_1);

    printf("场景7：刹车保护已配置\n");
    printf("PWM正在输出，等待刹车信号...\n");
    printf("提示：PA6接地将触发刹车保护\n");
}

//=============================================================================
// 场景8：PWM输入模式 - 测量频率和占空比
//=============================================================================

static volatile uint32_t pwm_freq_hz = 0;
static volatile uint8_t pwm_duty = 0;

void tim3_pwm_input_callback(tim_instance_t tim, tim_channel_t channel)
{
    uint32_t ccr1 = tim_get_capture(TIM_3, TIM_CH1); // 周期
    uint32_t ccr2 = tim_get_capture(TIM_3, TIM_CH2); // 脉宽

    if (ccr1 > 0)
    {
        uint32_t tim_clock = 180000000;
        pwm_freq_hz = tim_clock / ccr1;
        pwm_duty = (uint8_t)((ccr2 * 100) / ccr1);
    }
}

/**
 * @brief 场景8：PWM输入模式
 *
 * 功能：同时测量输入PWM信号的频率和占空比
 * 硬件连接：PA6 -> 外部PWM信号
 * API使用：tim_config_channel_input_capture() × 2
 */
void scenario_8_pwm_input_mode(void)
{
    // 步骤1：配置GPIO为TIM3_CH1输入
    gpio_init(PA6);
    gpio_set_dir(PA6, GPIO_IN);
    gpio_set_function(PA6, GPIO_AF_2); // TIM3_CH1

    // 步骤2：配置时基
    tim_timebase_config_t timebase = {
        .prescaler = 0,
        .period = 0xFFFF,
        .count_mode = TIM_COUNT_UP,
        .clock_division = TIM_CKD_DIV1,
        .repetition_counter = 0};
    tim_config_timebase(TIM_3, &timebase);

    // 配置CH1捕获周期（TI1直接映射）
    tim_ic_config_t ic1 = {
        .polarity = TIM_IC_RISING,
        .selection = TIM_IC_SELECT_DIRECT,
        .prescaler = TIM_IC_PSC_DIV1,
        .filter = 0};
    tim_config_channel_input_capture(TIM_3, TIM_CH1, &ic1);

    // 配置CH2捕获脉宽（TI1交叉映射）
    tim_ic_config_t ic2 = {
        .polarity = TIM_IC_FALLING,
        .selection = TIM_IC_SELECT_INDIRECT,
        .prescaler = TIM_IC_PSC_DIV1,
        .filter = 0};
    tim_config_channel_input_capture(TIM_3, TIM_CH2, &ic2);

    // 配置从模式为复位模式
    tim_master_slave_config_t ms = {
        .slave_mode = TIM_SLAVE_RESET,
        .trigger_source = TIM_TS_TI1FP1,
        .master_mode = TIM_TRGO_RESET,
        .master_mode2 = TIM_TRGO2_RESET,
        .msm_enable = false};
    tim_config_master_slave(TIM_3, &ms);

    // 注册回调
    tim_register_cc_callback(TIM_3, TIM_CH1, tim3_pwm_input_callback);

    // 使能中断和定时器
    tim_enable_interrupt(TIM_3, TIM_INT_CC1);
    tim_start(TIM_3);

    printf("场景8：PWM输入模式配置完成\n");

    // 延时并显示结果
    delay_ms(1000);
    printf("测量结果：频率=%u Hz，占空比=%u%%\n", pwm_freq_hz, pwm_duty);
}

//=============================================================================
// 场景9：单脉冲模式 - 触发式输出
//=============================================================================

/**
 * @brief 场景9：单脉冲模式
 *
 * 功能：外部信号触发后输出一个固定宽度的脉冲
 * 硬件连接：PA0 -> 触发输入，PA8 -> 脉冲输出
 * API使用：tim_config_one_pulse_mode()
 */
void scenario_9_one_pulse_mode(void)
{
    // 步骤1：配置GPIO
    gpio_init(PA0);
    gpio_set_dir(PA0, GPIO_IN);
    gpio_set_function(PA0, GPIO_AF_2); // TIM1_CH2（触发输入）

    gpio_init(PA8);
    gpio_set_dir(PA8, GPIO_OUT);
    gpio_set_function(PA8, GPIO_AF_2); // TIM1_CH1（脉冲输出）

    // 步骤2：配置时基
    tim_timebase_config_t timebase = {
        .prescaler = 219, // 220MHz -> 1MHz
        .period = 9999,   // 10ms周期
        .count_mode = TIM_COUNT_UP,
        .clock_division = TIM_CKD_DIV1,
        .repetition_counter = 0};
    tim_config_timebase(TIM_1, &timebase);

    // 配置单脉冲模式
    tim_opm_config_t opm = {
        .oc_mode = TIM_OC_PWM2,
        .pulse = 5000, // 5ms脉宽
        .oc_polarity = TIM_POLARITY_HIGH,
        .ic_polarity = TIM_IC_RISING,
        .ic_selection = TIM_IC_SELECT_DIRECT,
        .ic_filter = 0};
    tim_config_one_pulse_mode(TIM_1, &opm);

    // 使能通道
    tim_enable_channel(TIM_1, TIM_CH1, true);
    tim_enable_main_output(TIM_1);

    printf("场景9：单脉冲模式配置完成，等待触发...\n");
}

//=============================================================================
// 场景10：周期定时中断 - 系统定时器
//=============================================================================

static volatile uint32_t tick_count = 0;

void tim6_update_callback(tim_instance_t tim)
{
    tick_count++;

    if (tick_count % 1000 == 0)
    {
        printf("系统运行时间：%u 秒\n", tick_count / 1000);
    }
}

/**
 * @brief 场景10：周期定时中断
 *
 * 功能：使用基本定时器生成1ms定时中断
 * 硬件连接：无
 * API使用：tim_init_periodic(), tim_register_update_callback()
 */
void scenario_10_periodic_interrupt(void)
{
    // 快速初始化1ms定时器
    tim_init_periodic(TIM_6, 1000); // 1000us = 1ms

    // 注册回调函数
    tim_register_update_callback(TIM_6, tim6_update_callback);

    // 启动定时器
    tim_start(TIM_6);

    printf("场景10：1ms定时中断已启动\n");
}

//=============================================================================
// 主函数 - 场景选择
//=============================================================================

int main(void)
{
    system_init();
    SEGGER_RTT_Init();
    // 打印欢迎信息
    printf("\n\n");
    print_separator();
    printf("  ACM32P4 TIM驱动示例程序\n");
    printf("  SDK Version: 1.0\n");
    printf("  当前场景: %d\n", SCENARIO);
    print_separator();
    printf("\n");

    // 根据选择运行场景
    switch (SCENARIO)
    {
    case 1:
        scenario_1_basic_pwm();
        break;
    case 2:
        scenario_2_input_capture();
        break;
    case 3:
        scenario_3_encoder_interface();
        break;
    case 4:
        scenario_4_complementary_pwm();
        break;
    case 5:
        scenario_5_master_slave_sync();
        break;
    case 6:
        scenario_6_dma_burst();
        break;
    case 7:
        scenario_7_break_protection();
        break;
    case 8:
        scenario_8_pwm_input_mode();
        break;
    case 9:
        scenario_9_one_pulse_mode();
        break;
    case 10:
        scenario_10_periodic_interrupt();
        break;
    default:
        printf("错误：未知的场景编号 %d\n", SCENARIO);
        printf("提示：场景编号范围为 1-10\n");
        break;
    }

    // 主循环
    printf("\n进入主循环...\n");
    while (1)
    {
        delay_ms(1000);
    }

    return 0;
}
