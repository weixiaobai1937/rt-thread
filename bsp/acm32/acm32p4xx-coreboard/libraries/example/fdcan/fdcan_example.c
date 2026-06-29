/**
 * @file fdcan_example.c
 * @brief FDCAN模块功能示例程序
 *
 * @details
 * 演示10个场景：
 * 1. 基础CAN2.0B初始化
 * 2. CAN2.0B数据发送
 * 3. CAN2.0B数据接收（中断模式）
 * 4. CAN FD初始化与位时间配置
 * 5. CAN FD高速数据收发
 * 6. 内部回环自测试
 * 7. 硬件过滤器配置
 * 8. 静默模式总线监听
 * 9. 错误诊断与状态查询
 * 10. TTCAN时间触发通信
 */

#include "acm32p4.h"
#include "hardware/fdcan.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "SEGGER_RTT.h"
#include <string.h>

//===========================================
// 配置选项
//===========================================

#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)
#define DEFAULT_SCENARIO 1  // 修改此值选择运行的场景

// 时钟配置
#define APB_CLK_HZ          110000000U
#define CAN20_BAUD_RATE     500000U
#define CANFD_NOM_BAUD      500000U
#define CANFD_DATA_BAUD     4000000U

// 测试用CAN ID
#define TEST_CAN_ID         0x123U
#define TEST_CAN_EXT_ID     0x1ABCDEFU

//===========================================
// 全局变量
//===========================================

static volatile bool rx_complete = false;
static volatile bool tx_complete = false;
static volatile uint32_t rx_frame_count = 0;
static fdcan_frame_t last_rx_frame;

//===========================================
// 辅助函数
//===========================================

void print_separator(void)
{
    printf("========================================\n");
}

/**
 * @brief 打印帧内容
 */
void print_frame(const char *prefix, const fdcan_frame_t *frame)
{
    printf("%s ID=0x%08lX, %s, %s, ",
           prefix,
           frame->id,
           (frame->id_type == FDCAN_ID_EXTENDED) ? "扩展帧" : "标准帧",
           (frame->kind == FDCAN_FRAME_REMOTE) ? "远程帧" : "数据帧");

    if (frame->frame_type == FDCAN_FRAME_CLASSIC) {
        printf("CAN2.0B");
    } else if (frame->frame_type == FDCAN_FRAME_FD_BRS) {
        printf("CAN FD+BRS");
    } else {
        printf("CAN FD");
    }

    printf(", DLC=%d, 数据:", frame->dlc);
    for (uint8_t i = 0; i < frame->data_len && i < 16; i++) {
        printf(" %02X", frame->data[i]);
    }
    if (frame->data_len > 16) {
        printf(" ...(%d字节)", frame->data_len);
    }
    printf("\n");
}

/**
 * @brief 打印错误信息
 */
void print_error_info(fdcan_instance_t inst)
{
    fdcan_error_counters_t cnt;
    fdcan_get_error_counters(inst, &cnt);

    fdcan_error_state_t state = fdcan_get_error_state(inst);
    fdcan_error_type_t err = fdcan_get_error_type(inst);
    uint8_t alc = fdcan_get_arbitration_lost_pos(inst);

    const char *state_str;
    switch (state) {
        case FDCAN_ERROR_ACTIVE:  state_str = "错误主动"; break;
        case FDCAN_ERROR_PASSIVE: state_str = "错误被动"; break;
        case FDCAN_ERROR_BUS_OFF: state_str = "总线关闭"; break;
        default:                  state_str = "未知"; break;
    }

    const char *err_str;
    switch (err) {
        case FDCAN_ERROR_NONE:  err_str = "无错误"; break;
        case FDCAN_ERROR_BIT:   err_str = "位错误"; break;
        case FDCAN_ERROR_FORM:  err_str = "格式错误"; break;
        case FDCAN_ERROR_STUFF: err_str = "填充错误"; break;
        case FDCAN_ERROR_ACK:   err_str = "应答错误"; break;
        case FDCAN_ERROR_CRC:   err_str = "CRC错误"; break;
        case FDCAN_ERROR_OTHER: err_str = "其他错误"; break;
        default:                err_str = "未知"; break;
    }

    printf("  错误状态: %s\n", state_str);
    printf("  错误类型: %s\n", err_str);
    printf("  TECNT=%u, RECNT=%u\n", cnt.tecnt, cnt.recnt);
    printf("  仲裁丢失位置: %u\n", alc);

    if (fdcan_is_bus_off(inst)) {
        printf("  !! 总线已关闭 !!\n");
    }
}

/**
 * @brief FDCAN事件回调函数
 */
static void fdcan_event_callback(fdcan_instance_t inst, uint32_t flags)
{
    if (flags & FDCAN_FLAG_RX) {
        rx_complete = true;
        rx_frame_count++;
    }

    if (flags & FDCAN_FLAG_TX_PTB) {
        tx_complete = true;
    }

    if (flags & FDCAN_FLAG_TX_STB) {
        tx_complete = true;
    }

    if (flags & FDCAN_FLAG_ERROR) {
        printf("  [中断] 错误事件触发\n");
    }

    if (flags & FDCAN_FLAG_ARBITRATION_LOST) {
        printf("  [中断] 仲裁失败\n");
    }

    if (flags & FDCAN_FLAG_ERROR_PASSIVE) {
        printf("  [中断] 进入错误被动状态\n");
    }

    if (flags & FDCAN_FLAG_ERROR_WARNING) {
        printf("  [中断] 错误警告\n");
    }
}

//===========================================
// 示例场景
//===========================================

/**
 * @brief 场景1：基础CAN2.0B初始化
 */
void scenario_1_basic_can20_init(void)
{
    print_separator();
    printf("=== 场景1：基础CAN2.0B初始化 ===\n\n");

    printf("步骤1：使能FDCAN1外设时钟\n");
    clock_periph_enable(CLK_FDCAN1);
    printf("  结果: 时钟已使能\n");

    printf("\n步骤2：初始化FDCAN1 (CAN2.0B, 500Kbps)\n");
    if (fdcan_init_can20(FDCAN_INSTANCE_1, CAN20_BAUD_RATE, APB_CLK_HZ)) {
        printf("  结果: CAN2.0B初始化成功\n");
        printf("  配置: 标称波特率 %u bps\n", CAN20_BAUD_RATE);
        printf("  模式: 正常模式\n");
        printf("  状态: 滤波器0已使能（接收所有帧）\n");
    } else {
        printf("  结果: 初始化失败！\n");
    }

    printf("\n步骤3：查询FDCAN1状态\n");
    fdcan_rx_status_t rx_st = fdcan_get_rx_status(FDCAN_INSTANCE_1);
    printf("  接收FIFO状态: %d (0=空, 3=满)\n", (int)rx_st);
    printf("  总线状态: %s\n", fdcan_is_bus_off(FDCAN_INSTANCE_1) ? "关闭" : "正常");
    printf("  发送状态: %s\n", fdcan_is_transmitting(FDCAN_INSTANCE_1) ? "正在发送" : "空闲");

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景2：CAN2.0B数据发送
 */
void scenario_2_can20_transmit(void)
{
    print_separator();
    printf("=== 场景2：CAN2.0B数据发送 ===\n\n");

    printf("步骤1：初始化FDCAN1\n");
    clock_periph_enable(CLK_FDCAN1);
    fdcan_init_can20(FDCAN_INSTANCE_1, CAN20_BAUD_RATE, APB_CLK_HZ);
    printf("  结果: CAN2.0B已就绪\n");

    printf("\n步骤2：准备一帧标准数据帧\n");
    fdcan_frame_t tx_frame = {0};
    tx_frame.id = TEST_CAN_ID;
    tx_frame.id_type = FDCAN_ID_STANDARD;
    tx_frame.kind = FDCAN_FRAME_DATA;
    tx_frame.frame_type = FDCAN_FRAME_CLASSIC;
    tx_frame.dlc = 8;
    tx_frame.data_len = 8;
    for (uint8_t i = 0; i < 8; i++) {
        tx_frame.data[i] = i;
    }
    printf("  ID=0x%03lX, DLC=8, 数据: 00 01 02 03 04 05 06 07\n", tx_frame.id);

    printf("\n步骤3：通过PTB发送数据帧\n");
    if (fdcan_send_ptb(FDCAN_INSTANCE_1, &tx_frame)) {
        printf("  结果: 发送请求已提交\n");
    } else {
        printf("  结果: PTB忙，发送失败\n");
        printf("\n完成！\n");
        print_separator();
        return;
    }

    printf("\n步骤4：等待发送完成...\n");
    uint32_t timeout = 10000;
    while (!(fdcan_get_irq_status(FDCAN_INSTANCE_1) & FDCAN_FLAG_TX_PTB) && --timeout) {
        __NOP();
    }
    if (timeout > 0) {
        printf("  结果: 发送完成 (TPIF=1)\n");
        fdcan_clear_irq_flags(FDCAN_INSTANCE_1, FDCAN_FLAG_TX_PTB);
    } else {
        printf("  结果: 发送超时\n");
    }

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景3：CAN2.0B数据接收（中断模式）
 */
void scenario_3_can20_receive(void)
{
    print_separator();
    printf("=== 场景3：CAN2.0B数据接收（中断模式） ===\n\n");

    printf("步骤1：初始化FDCAN1\n");
    clock_periph_enable(CLK_FDCAN1);
    fdcan_init_can20(FDCAN_INSTANCE_1, CAN20_BAUD_RATE, APB_CLK_HZ);
    printf("  结果: CAN2.0B已就绪\n");

    printf("\n步骤2：使能过滤器0接收所有帧\n");
    fdcan_filter_config_t filter_cfg = {
        .width = FDCAN_FILTER_32BIT,
        .mask_enabled = false,
        .code = 0,
        .mask = 0,
        .enabled = true,
    };
    fdcan_reset(FDCAN_INSTANCE_1);
    fdcan_config_filter(FDCAN_INSTANCE_1, 0, &filter_cfg);
    fdcan_start(FDCAN_INSTANCE_1);
    printf("  结果: 过滤器0已配置（接收所有ID）\n");

    printf("\n步骤3：注册回调函数\n");
    fdcan_register_callback(FDCAN_INSTANCE_1, fdcan_event_callback);
    printf("  结果: 回调函数已注册\n");

    printf("\n步骤4：使能接收中断\n");
    fdcan_irq_enable(FDCAN_INSTANCE_1, FDCAN_IRQ_RX);
    NVIC_EnableIRQ(FDCAN1_IRQn);
    printf("  结果: 接收中断已使能\n");

    printf("\n步骤5：等待接收CAN帧（5秒超时）...\n");
    rx_complete = false;
    uint64_t start = system_get_tick();
    while (!rx_complete && !system_elapsed(start, 5000)) {
        __NOP();
    }

    if (rx_complete) {
        printf("\n步骤6：读取接收到的帧\n");
        if (fdcan_receive(FDCAN_INSTANCE_1, &last_rx_frame)) {
            print_frame("  收到", &last_rx_frame);
            printf("  TTCAN周期时间: %u\n", last_rx_frame.cycle_time);
            printf("  时间戳: %llu\n", (unsigned long long)last_rx_frame.timestamp);
            fdcan_release_rx_buffer(FDCAN_INSTANCE_1);
            printf("  结果: 帧已释放\n");
        }
    } else {
        printf("  结果: 超时未收到数据\n");
        printf("  提示: 请连接CAN总线或使用回环模式测试\n");
    }

    printf("\n步骤7：查询接收统计\n");
    printf("  累计接收帧数: %lu\n", rx_frame_count);
    printf("  接收FIFO可用帧: %s\n",
           fdcan_is_frame_available(FDCAN_INSTANCE_1) ? "是" : "否");

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景4：CAN FD初始化与位时间配置
 */
void scenario_4_canfd_init(void)
{
    print_separator();
    printf("=== 场景4：CAN FD初始化与位时间配置 ===\n\n");

    printf("步骤1：使能FDCAN1外设时钟\n");
    clock_periph_enable(CLK_FDCAN1);
    printf("  结果: 时钟已使能\n");

    printf("\n步骤2：CAN FD初始化 (ISO标准)\n");
    printf("  标称波特率: %u bps\n", CANFD_NOM_BAUD);
    printf("  数据波特率: %u bps\n", CANFD_DATA_BAUD);
    printf("  协议模式: ISO 11898-1\n");

    if (fdcan_init_canfd(FDCAN_INSTANCE_1, CANFD_NOM_BAUD, CANFD_DATA_BAUD,
                         APB_CLK_HZ, FDCAN_FD_MODE_ISO)) {
        printf("  结果: CAN FD初始化成功\n");
    } else {
        printf("  结果: 初始化失败！\n");
        printf("\n完成！\n");
        print_separator();
        return;
    }

    printf("\n步骤3：手动调整标称位时间配置\n");
    fdcan_reset(FDCAN_INSTANCE_1);

    fdcan_bit_timing_config_t nom_timing = {
        .prescaler = 10,   // 预分频=11，TQ_freq=110MHz/11=10MHz
        .seg1 = 13,        // Prop_Seg+Phase_Seg1 = 15 TQ
        .seg2 = 4,         // Phase_Seg2 = 5 TQ
        .sjw = 3,          // SJW = 4 TQ
    };

    if (fdcan_config_nominal_bit_timing(FDCAN_INSTANCE_1, &nom_timing)) {
        printf("  结果: 标称位时间已配置\n");
        printf("    TQ频率: %lu Hz\n", APB_CLK_HZ / (nom_timing.prescaler + 1));
        printf("    总TQ: %d, 采样点约 %.1f%%\n",
               nom_timing.seg1 + nom_timing.seg2 + 3,
               100.0f * (nom_timing.seg1 + 2) / (nom_timing.seg1 + nom_timing.seg2 + 3));
    }

    printf("\n步骤4：配置发送延迟补偿（高速模式）\n");
    fdcan_config_tx_delay_compensation(FDCAN_INSTANCE_1, true, nom_timing.seg1);
    printf("  结果: TDC已使能，SSP偏移=%u TQ\n", nom_timing.seg1);

    printf("\n步骤5：设置STB为优先级模式\n");
    fdcan_set_stb_mode(FDCAN_INSTANCE_1, FDCAN_STB_MODE_PRIORITY);
    printf("  结果: STB模式=优先级（ID低者优先发送）\n");

    fdcan_start(FDCAN_INSTANCE_1);
    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景5：CAN FD高速数据收发
 */
void scenario_5_canfd_transmit_receive(void)
{
    print_separator();
    printf("=== 场景5：CAN FD高速数据收发 ===\n\n");

    printf("步骤1：初始化FDCAN1为CAN FD模式\n");
    clock_periph_enable(CLK_FDCAN1);
    fdcan_init_canfd(FDCAN_INSTANCE_1, CANFD_NOM_BAUD, CANFD_DATA_BAUD,
                     APB_CLK_HZ, FDCAN_FD_MODE_ISO);
    printf("  结果: CAN FD就已绪\n");

    printf("\n步骤2：准备64字节CAN FD数据帧\n");
    fdcan_frame_t tx_frame = {0};
    tx_frame.id = 0x555;
    tx_frame.id_type = FDCAN_ID_EXTENDED;
    tx_frame.kind = FDCAN_FRAME_DATA;
    tx_frame.frame_type = FDCAN_FRAME_FD_BRS;  // FD + 速率切换
    tx_frame.dlc = 15;  // DLC=15 => 64字节
    tx_frame.data_len = 64;
    for (uint8_t i = 0; i < 64; i++) {
        tx_frame.data[i] = i;
    }
    printf("  ID=0x%08lX (扩展帧), CAN FD+BRS, DLC=15 (64字节)\n", tx_frame.id);

    printf("\n步骤3：通过PTB发送64字节CAN FD帧\n");
    if (fdcan_send_ptb(FDCAN_INSTANCE_1, &tx_frame)) {
        printf("  结果: CAN FD帧发送请求已提交\n");

        printf("\n步骤4：等待发送完成...\n");
        uint32_t timeout = 100000;
        while (!(fdcan_get_irq_status(FDCAN_INSTANCE_1) & FDCAN_FLAG_TX_PTB) && --timeout) {
            __NOP();
        }
        if (timeout > 0) {
            printf("  结果: 发送完成\n");
            fdcan_clear_irq_flags(FDCAN_INSTANCE_1, FDCAN_FLAG_TX_PTB);
        } else {
            printf("  结果: 发送超时（可能无应答节点）\n");
        }
    } else {
        printf("  结果: PTB忙\n");
    }

    printf("\n步骤5：连续发送多帧到STB\n");
    for (uint8_t i = 0; i < 3; i++) {
        fdcan_frame_t stb_frame = {0};
        stb_frame.id = 0x100 + i;
        stb_frame.id_type = FDCAN_ID_STANDARD;
        stb_frame.kind = FDCAN_FRAME_DATA;
        stb_frame.frame_type = FDCAN_FRAME_FD_NO_BRS;
        stb_frame.dlc = 12;  // DLC=12 => 32字节
        stb_frame.data_len = 32;
        for (uint8_t j = 0; j < 32; j++) {
            stb_frame.data[j] = j + i;
        }

        if (fdcan_send_stb(FDCAN_INSTANCE_1, &stb_frame)) {
            printf("  帧%u: ID=0x%03lX, 32字节 -> STB写入成功\n", i + 1, stb_frame.id);
        } else {
            printf("  帧%u: STB写入失败（缓冲区满）\n", i + 1);
        }
    }

    printf("\n步骤6：一次性发送STB中的所有帧\n");
    fdcan_transmit_stb_all(FDCAN_INSTANCE_1);
    printf("  结果: TSALL请求已发出\n");

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景6：内部回环自测试
 */
void scenario_6_loopback_test(void)
{
    print_separator();
    printf("=== 场景6：内部回环自测试 ===\n\n");

    printf("步骤1：初始化FDCAN1\n");
    clock_periph_enable(CLK_FDCAN1);
    fdcan_init_can20(FDCAN_INSTANCE_1, CAN20_BAUD_RATE, APB_CLK_HZ);
    printf("  结果: CAN2.0B已就绪\n");

    printf("\n步骤2：配置内部回环模式\n");
    fdcan_config_loopback(FDCAN_INSTANCE_1, FDCAN_LOOPBACK_INTERNAL);
    printf("  结果: 内部回环模式已使能\n");
    printf("  说明: 发送数据不输出到总线，内部自收自发\n");

    printf("\n步骤3：使能过滤器0接收所有帧\n");
    fdcan_reset(FDCAN_INSTANCE_1);
    fdcan_filter_config_t filter_cfg = {
        .width = FDCAN_FILTER_32BIT,
        .mask_enabled = false,
        .code = 0,
        .mask = 0,
        .enabled = true,
    };
    fdcan_config_filter(FDCAN_INSTANCE_1, 0, &filter_cfg);
    fdcan_config_loopback(FDCAN_INSTANCE_1, FDCAN_LOOPBACK_INTERNAL);
    fdcan_start(FDCAN_INSTANCE_1);
    printf("  结果: 配置完成\n");

    printf("\n步骤4：发送并回环接收测试（3帧）\n");
    for (uint8_t test = 1; test <= 3; test++) {
        fdcan_frame_t tx_frame = {0};
        tx_frame.id = 0x200 + test;
        tx_frame.id_type = (test == 2) ? FDCAN_ID_EXTENDED : FDCAN_ID_STANDARD;
        tx_frame.kind = FDCAN_FRAME_DATA;
        tx_frame.frame_type = FDCAN_FRAME_CLASSIC;
        tx_frame.dlc = 8;
        tx_frame.data_len = 8;
        for (uint8_t i = 0; i < 8; i++) {
            tx_frame.data[i] = i * test;
        }

        printf("\n  测试%u: 发送 0x%03lX\n", test, tx_frame.id);
        fdcan_send_ptb(FDCAN_INSTANCE_1, &tx_frame);

        // 等待发送和接收完成
        uint32_t timeout = 10000;
        while (!fdcan_is_frame_available(FDCAN_INSTANCE_1) && --timeout) {
            __NOP();
        }

        if (timeout > 0) {
            fdcan_frame_t rx_frame = {0};
            if (fdcan_receive(FDCAN_INSTANCE_1, &rx_frame)) {
                print_frame("  收到", &rx_frame);
                fdcan_release_rx_buffer(FDCAN_INSTANCE_1);

                // 验证数据一致性
                bool match = (rx_frame.id == tx_frame.id) && (rx_frame.data_len == tx_frame.data_len);
                for (uint8_t i = 0; i < tx_frame.data_len && match; i++) {
                    if (rx_frame.data[i] != tx_frame.data[i]) match = false;
                }
                printf("  回环验证: %s\n", match ? "通过" : "失败");
            }
        } else {
            printf("  回环测试: 超时未收到数据\n");
        }

        fdcan_clear_irq_flags(FDCAN_INSTANCE_1, FDCAN_FLAG_TX_PTB | FDCAN_FLAG_RX);
    }

    printf("\n步骤5：恢复正常模式\n");
    fdcan_config_loopback(FDCAN_INSTANCE_1, FDCAN_LOOPBACK_NONE);
    fdcan_reset(FDCAN_INSTANCE_1);
    fdcan_start(FDCAN_INSTANCE_1);
    printf("  结果: 已退出回环模式\n");

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景7：硬件过滤器配置
 */
void scenario_7_filter_config(void)
{
    print_separator();
    printf("=== 场景7：硬件过滤器配置 ===\n\n");

    printf("步骤1：初始化FDCAN1\n");
    clock_periph_enable(CLK_FDCAN1);
    fdcan_init_can20(FDCAN_INSTANCE_1, CAN20_BAUD_RATE, APB_CLK_HZ);
    printf("  结果: CAN2.0B已就绪\n");

    printf("\n步骤2：进入复位状态以配置过滤器\n");
    fdcan_reset(FDCAN_INSTANCE_1);
    printf("  结果: RESET=1，可配置过滤器\n");

    printf("\n步骤3：配置过滤器组\n");

    // 过滤器0：仅接收ID=0x123（精确匹配）
    printf("  过滤器0: 精确匹配 ID=0x123 (屏蔽模式)\n");
    fdcan_filter_config_t filter0 = {
        .width = FDCAN_FILTER_32BIT,
        .mask_enabled = true,   // 使用MASK
        .code = 0x123,          // 期望的ID
        .mask = 0x7FF,          // 比较所有11位
        .enabled = true,
    };
    fdcan_config_filter(FDCAN_INSTANCE_1, 0, &filter0);
    printf("    配置: CODE=0x%03lX, MASK=0x%03lX, 仅接收匹配该ID的帧\n",
           filter0.code, filter0.mask);

    // 过滤器1：接收ID范围0x100-0x1FF（高4位匹配）
    printf("  过滤器1: 接收ID范围 0x100-0x1FF (高4位=0x1)\n");
    fdcan_filter_config_t filter1 = {
        .width = FDCAN_FILTER_32BIT,
        .mask_enabled = true,
        .code = 0x100,
        .mask = 0x700,  // MASK位=0表示需要匹配，=1表示忽略该位
        .enabled = true,
    };
    fdcan_config_filter(FDCAN_INSTANCE_1, 1, &filter1);
    printf("    配置: CODE=0x%03lX, MASK=0x%03lX\n", filter1.code, filter1.mask);

    // 过滤器2：接收所有帧（无MASK模式）
    printf("  过滤器2: 接收所有帧（无MASK模式）\n");
    fdcan_filter_config_t filter2 = {
        .width = FDCAN_FILTER_32BIT,
        .mask_enabled = false,  // 不使用MASK，所有帧都通过
        .code = 0,
        .mask = 0,
        .enabled = true,
    };
    fdcan_config_filter(FDCAN_INSTANCE_1, 2, &filter2);
    printf("    配置: NOMASK=1, 此过滤器接收所有帧\n");

    // 过滤器3：禁用
    printf("  过滤器3-15: 全部禁用\n");
    for (uint8_t i = 3; i < 16; i++) {
        fdcan_filter_config_t disabled = {
            .width = FDCAN_FILTER_32BIT,
            .mask_enabled = false,
            .code = 0,
            .mask = 0,
            .enabled = false,
        };
        fdcan_config_filter(FDCAN_INSTANCE_1, i, &disabled);
    }

    printf("\n步骤4：配置全局过滤参数\n");
    fdcan_global_filter_config_t global_cfg = {
        .accept_all = false,                    // 不接收错误帧
        .overflow_mode = FDCAN_RX_OVERFLOW_DISCARD_OLD,  // 溢出丢弃旧帧
        .limit = {
            .afwl = 8,    // 接收FIFO快满阈值=8帧
            .ewl  = 11,   // 错误警告阈值=(11+1)*8=96
        },
    };
    fdcan_config_global_filter(FDCAN_INSTANCE_1, &global_cfg);
    printf("  结果: 全局过滤已配置\n");
    printf("    AFWL=%u (FIFO≥%u帧时产生快满中断)\n", global_cfg.limit.afwl, global_cfg.limit.afwl);
    printf("    EWL=%u (错误≥%u时产生警告)\n", global_cfg.limit.ewl, (global_cfg.limit.ewl + 1) * 8);

    printf("\n步骤5：启动CAN通信\n");
    fdcan_start(FDCAN_INSTANCE_1);
    printf("  结果: RESET=0, CAN已启动\n");

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景8：静默模式总线监听
 */
void scenario_8_silent_mode(void)
{
    print_separator();
    printf("=== 场景8：静默模式总线监听 ===\n\n");

    printf("步骤1：初始化FDCAN1\n");
    clock_periph_enable(CLK_FDCAN1);
    fdcan_init_can20(FDCAN_INSTANCE_1, CAN20_BAUD_RATE, APB_CLK_HZ);
    printf("  结果: CAN2.0B已就绪\n");

    printf("\n步骤2：使能静默模式\n");
    fdcan_config_silent_mode(FDCAN_INSTANCE_1, true);
    printf("  结果: 静默模式已使能\n");
    printf("  说明: 可接收CAN总线数据，但不发送任何数据\n");
    printf("        不会产生错误标志和过载帧\n");

    printf("\n步骤3：注册回调并开始监听\n");
    rx_frame_count = 0;
    fdcan_register_callback(FDCAN_INSTANCE_1, fdcan_event_callback);
    fdcan_irq_enable(FDCAN_INSTANCE_1, FDCAN_IRQ_RX | FDCAN_IRQ_ERROR);
    NVIC_EnableIRQ(FDCAN1_IRQn);
    printf("  结果: 监听已启动，等待CAN总线数据...\n");

    printf("\n步骤4：持续监听5秒\n");
    uint64_t start = system_get_tick();
    while (!system_elapsed(start, 5000)) {
        if (rx_complete) {
            rx_complete = false;
            fdcan_frame_t rx_frame = {0};
            if (fdcan_receive(FDCAN_INSTANCE_1, &rx_frame)) {
                print_frame("  [监听]", &rx_frame);
                fdcan_release_rx_buffer(FDCAN_INSTANCE_1);
            }
        }

        // 显示总线状态变化
        if (fdcan_is_bus_off(FDCAN_INSTANCE_1)) {
            printf("  [警告] 总线关闭状态\n");
        }
    }
    printf("  监听期间收到 %lu 帧\n", rx_frame_count);

    printf("\n步骤5：退出静默模式\n");
    fdcan_config_silent_mode(FDCAN_INSTANCE_1, false);
    printf("  结果: 已恢复正常模式\n");

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景9：错误诊断与状态查询
 */
void scenario_9_error_diagnostics(void)
{
    print_separator();
    printf("=== 场景9：错误诊断与状态查询 ===\n\n");

    printf("步骤1：初始化FDCAN1\n");
    clock_periph_enable(CLK_FDCAN1);
    fdcan_init_can20(FDCAN_INSTANCE_1, CAN20_BAUD_RATE, APB_CLK_HZ);
    printf("  结果: CAN2.0B已就绪\n");

    printf("\n步骤2：使能所有错误相关中断\n");
    fdcan_irq_enable(FDCAN_INSTANCE_1,
                     FDCAN_IRQ_ERROR |
                     FDCAN_IRQ_ARBITRATION_LOST |
                     FDCAN_IRQ_BUS_ERROR |
                     FDCAN_IRQ_ERROR_PASSIVE);
    fdcan_register_callback(FDCAN_INSTANCE_1, fdcan_event_callback);
    printf("  结果: 错误中断已全部使能\n");

    printf("\n步骤3：查询当前错误状态\n");
    print_error_info(FDCAN_INSTANCE_1);

    printf("\n步骤4：检查FDCAN各状态\n");
    printf("  接收FIFO状态: ");
    switch (fdcan_get_rx_status(FDCAN_INSTANCE_1)) {
        case FDCAN_RX_EMPTY:       printf("空\n"); break;
        case FDCAN_RX_BELOW_AFWL:  printf("低于快满阈值\n"); break;
        case FDCAN_RX_ABOVE_AFWL:  printf("高于快满阈值\n"); break;
        case FDCAN_RX_FULL:        printf("已满\n"); break;
    }

    printf("  接收FIFO帧数: %u\n", fdcan_get_rx_count(FDCAN_INSTANCE_1));
    printf("  STB已存放帧数: %u\n", fdcan_get_stb_count(FDCAN_INSTANCE_1));
    printf("  STB是否满: %s\n", fdcan_is_stb_full(FDCAN_INSTANCE_1) ? "是" : "否");
    printf("  接收FIFO溢出: %s\n", fdcan_is_rx_overflow(FDCAN_INSTANCE_1) ? "是" : "否");
    printf("  正在发送: %s\n", fdcan_is_transmitting(FDCAN_INSTANCE_1) ? "是" : "否");
    printf("  正在接收: %s\n", fdcan_is_receiving(FDCAN_INSTANCE_1) ? "是" : "否");

    printf("\n步骤5：配置Single Shot模式（用于诊断）\n");
    printf("  将STB设为Single Shot模式\n");
    fdcan_config_auto_retransmit(FDCAN_INSTANCE_1, FDCAN_TX_STB, false);
    printf("  结果: STB Single Shot已使能\n");
    printf("  说明: 发送失败时不自动重发，便于诊断总线问题\n");

    printf("\n步骤6：尝试发送诊断帧\n");
    fdcan_frame_t diag_frame = {0};
    diag_frame.id = 0x7DF;  // OBD-II 诊断请求ID
    diag_frame.id_type = FDCAN_ID_STANDARD;
    diag_frame.kind = FDCAN_FRAME_DATA;
    diag_frame.frame_type = FDCAN_FRAME_CLASSIC;
    diag_frame.dlc = 8;
    diag_frame.data_len = 8;
    diag_frame.data[0] = 0x02;
    diag_frame.data[1] = 0x01;
    diag_frame.data[2] = 0x00;  // PID 0x00 (PIDs supported)

    if (fdcan_send_stb(FDCAN_INSTANCE_1, &diag_frame)) {
        printf("  结果: 诊断帧已写入STB\n");
        fdcan_transmit_stb_one(FDCAN_INSTANCE_1);

        // 等待结果
        uint32_t timeout = 100000;
        uint32_t flags = 0;
        while (timeout--) {
            flags = fdcan_get_irq_status(FDCAN_INSTANCE_1);
            if (flags & (FDCAN_FLAG_TX_STB | FDCAN_FLAG_ABORT | FDCAN_FLAG_BUS_ERROR)) {
                break;
            }
        }

        if (flags & FDCAN_FLAG_TX_STB) {
            printf("  发送结果: 成功\n");
        } else if (flags & FDCAN_FLAG_ARBITRATION_LOST) {
            printf("  发送结果: 仲裁失败 (ALC=%u)\n",
                   fdcan_get_arbitration_lost_pos(FDCAN_INSTANCE_1));
        } else if (flags & FDCAN_FLAG_BUS_ERROR) {
            printf("  发送结果: 总线错误\n");
        } else if (flags & FDCAN_FLAG_ABORT) {
            printf("  发送结果: 已取消\n");
        } else {
            printf("  发送结果: 超时（可能无应答）\n");
        }
    } else {
        printf("  结果: STB满，无法写入\n");
    }

    printf("\n步骤7：查询错误计数器变化\n");
    print_error_info(FDCAN_INSTANCE_1);

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景10：TTCAN时间触发通信
 */
void scenario_10_ttcan_time_trigger(void)
{
    print_separator();
    printf("=== 场景10：TTCAN时间触发通信 ===\n\n");

    printf("步骤1：初始化FDCAN1\n");
    clock_periph_enable(CLK_FDCAN1);
    fdcan_init_can20(FDCAN_INSTANCE_1, CAN20_BAUD_RATE, APB_CLK_HZ);
    printf("  结果: CAN2.0B已就绪\n");

    printf("\n步骤2：配置时间戳功能\n");
    fdcan_timestamp_config_t ts_cfg = {
        .enabled = true,
        .position = FDCAN_TIMESTAMP_EOF,  // 帧结束时采样
    };
    fdcan_config_timestamp(FDCAN_INSTANCE_1, &ts_cfg);
    printf("  结果: 时间戳已使能（EOF采样）\n");

    printf("\n步骤3：配置TTCAN模式（Time Master角色）\n");
    fdcan_ttcan_config_t ttcan_cfg = {
        .ttcan_enabled = true,
        .ttcan_tbm_mode = true,     // 组合发送缓冲模式
        .time_prescaler = 0,         // 1分频，定时器时钟=位时间
        .ref_id = 0x100,            // 参考消息ID
        .ref_ide = false,            // 标准帧
    };
    fdcan_config_ttcan(FDCAN_INSTANCE_1, &ttcan_cfg);
    printf("  结果: TTCAN已使能\n");
    printf("    参考消息ID: 0x%03lX\n", ttcan_cfg.ref_id);
    printf("    TBUF模式: 组合模式（PTB+STB组成统一槽）\n");

    printf("\n步骤4：使能看门狗触发（100ms超时）\n");
    // 看门狗时间 = 参考消息最大间隔 (单位：位时间)
    // 100ms @ 500Kbps = 50000 位时间
    fdcan_config_ttcan_watchdog(FDCAN_INSTANCE_1, 50000);
    printf("  结果: 看门狗触发=50000位时间（约100ms）\n");

    printf("\n步骤5：配置时间触发槽\n");

    // 槽0 (PTB): 周期时间=0时立即发送参考消息
    fdcan_ttcan_trigger_config_t trig_ref = {
        .type = FDCAN_TT_TRIG_SINGLE_TX,
        .slot = 0,               // PTB
        .trigger_time = 0,       // cycle_time=0
        .tx_enable_window = 2,   // 2个cycle_tick窗口
    };
    fdcan_config_ttcan_trigger(FDCAN_INSTANCE_1, &trig_ref);
    printf("  触发1: 槽0(PTB) cycle_time=0 -> 发送参考消息\n");

    // 槽1 (STB slot1): 周期时间=1000时发送消息A
    fdcan_ttcan_trigger_config_t trig_a = {
        .type = FDCAN_TT_TRIG_SINGLE_TX,
        .slot = 1,               // STB Slot1
        .trigger_time = 1000,    // cycle_time=1000
        .tx_enable_window = 4,
    };
    fdcan_config_ttcan_trigger(FDCAN_INSTANCE_1, &trig_a);
    printf("  触发2: 槽1(STB) cycle_time=1000 -> 发送消息A\n");

    // 槽2 (STB slot2): 时间触发（仅产生中断）
    fdcan_ttcan_trigger_config_t trig_b = {
        .type = FDCAN_TT_TRIG_TIME,   // 仅中断
        .slot = 2,
        .trigger_time = 2000,
        .tx_enable_window = 0,
    };
    fdcan_config_ttcan_trigger(FDCAN_INSTANCE_1, &trig_b);
    printf("  触发3: 槽2(STB) cycle_time=2000 -> 中断通知（期待接收消息B）\n");

    printf("\n步骤6：预装消息到STB槽\n");
    // 消息A (STB Slot1)
    fdcan_frame_t msg_a = {0};
    msg_a.id = 0x101;
    msg_a.id_type = FDCAN_ID_STANDARD;
    msg_a.kind = FDCAN_FRAME_DATA;
    msg_a.frame_type = FDCAN_FRAME_CLASSIC;
    msg_a.dlc = 8;
    msg_a.data_len = 8;
    for (uint8_t i = 0; i < 8; i++) msg_a.data[i] = 0xA0 + i;

    if (fdcan_send_stb(FDCAN_INSTANCE_1, &msg_a)) {
        printf("  消息A (ID=0x101) -> STB槽1, 已预装\n");
    }

    // 标记槽1为满
    fdcan_set_ttbuf_slot_status(FDCAN_INSTANCE_1, 1, true);
    printf("  槽1标记为: 满\n");

    printf("\n步骤7：TTCAN已就绪，等待参考消息触发\n");
    printf("  说明: 当接收到参考消息(0x100)时，cycle_time被清零\n");
    printf("        然后按预设的时间窗口自动触发消息发送\n");

    // 使能TTCAN相关中断
    fdcan_irq_enable(FDCAN_INSTANCE_1, FDCAN_IRQ_TX_STB);
    fdcan_register_callback(FDCAN_INSTANCE_1, fdcan_event_callback);
    NVIC_EnableIRQ(FDCAN1_IRQn);

    printf("\n步骤8：监控TTCAN运行（10秒）\n");
    uint64_t start = system_get_tick();
    while (!system_elapsed(start, 10000)) {
        uint32_t flags = fdcan_get_irq_status(FDCAN_INSTANCE_1);

        if (flags & FDCAN_FLAG_TX_STB) {
            printf("  [TTCAN] 消息发送完成 (TSIF)\n");
            fdcan_clear_irq_flags(FDCAN_INSTANCE_1, FDCAN_FLAG_TX_STB);
        }

        // 检查TTCAN触发错误
        {
            FDCAN_TypeDef *regs = (FDCAN_TypeDef *)fdcan_get_base(FDCAN_INSTANCE_1);
            if (regs->TTCFG_f.TEIF) {
                printf("  [TTCAN] 触发错误 (cycle_time已过)\n");
                regs->TTCFG_f.TEIF = 1;
            }
            if (regs->TTCFG_f.WTIF) {
                printf("  [TTCAN] 看门狗触发 (参考消息丢失)\n");
                regs->TTCFG_f.WTIF = 1;
            }
        }

        delay_ms(100);
    }

    printf("\n步骤9：关闭TTCAN\n");
    fdcan_reset(FDCAN_INSTANCE_1);
    fdcan_start(FDCAN_INSTANCE_1);
    printf("  结果: TTCAN已关闭\n");

    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 主函数
//===========================================

int main(void)
{
    system_init();
    SEGGER_RTT_Init();

    printf("\n");
    print_separator();
    printf("FDCAN模块示例程序\n");
    printf("当前场景: %d\n", DEFAULT_SCENARIO);
    print_separator();
    printf("\n");

    switch (DEFAULT_SCENARIO) {
        case 1:  scenario_1_basic_can20_init();     break;
        case 2:  scenario_2_can20_transmit();       break;
        case 3:  scenario_3_can20_receive();         break;
        case 4:  scenario_4_canfd_init();            break;
        case 5:  scenario_5_canfd_transmit_receive(); break;
        case 6:  scenario_6_loopback_test();         break;
        case 7:  scenario_7_filter_config();         break;
        case 8:  scenario_8_silent_mode();           break;
        case 9:  scenario_9_error_diagnostics();     break;
        case 10: scenario_10_ttcan_time_trigger();   break;
        default:
            printf("错误：无效的场景编号 %d\n", DEFAULT_SCENARIO);
            printf("有效范围: 1-10\n");
            break;
    }

    while (1) {
        __WFI();
    }
}
