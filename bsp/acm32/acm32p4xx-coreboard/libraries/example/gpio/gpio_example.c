/**
 * @file gpio_example.c
 * @brief ACM32P4 GPIO 驱动示例
 *
 * @details
 * 演示8个场景：
 * 1. LED闪烁（基础输出）
 * 2. 按键输入（基础输入+上拉）
 * 3. UART引脚复用配置
 * 4. I2C引脚配置（开漏+上拉）
 * 5. 高级配置（使用gpio_configure）
 * 6. SPI高速信号（高驱动能力）
 * 7. 端口批量操作
 * 8. 配置锁定演示
 */

#include "SEGGER_RTT.h"
#include "acm32p4.h"
#include "hardware/system.h"

//===========================================
// 配置选项
//===========================================

#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)

// 选择要运行的场景（修改此值来切换场景）
#define DEFAULT_SCENARIO    1

//===========================================
// 全局变量
//===========================================

// ============================================================================
// 工具函数
// ============================================================================


/**
 * @brief 打印分隔线
 */
void print_separator(void)
{
    printf("==========================================\n");
}

// ============================================================================
// 场景1：LED闪烁（最简场景）
// ============================================================================

void scenario_1_led_blink(void)
{
    print_separator();
    printf("=== 场景1：LED闪烁 ===\n\n");
    // 初始化LED引脚（使用PA0）
    gpio_init(PA0);
    gpio_set_dir(PA0, GPIO_OUT);

    // 闪烁10次
    for (int i = 0; i < 10; i++) {
        gpio_put(PA0, true); // 点亮
        printf("LED ON\n");
        delay_ms(100);
        gpio_put(PA0, false); // 熄灭
        printf("LED OFF\n");
        delay_ms(100);
    }

    // 或者使用翻转方式
    for (int i = 0; i < 10; i++) {
        gpio_toggle(PA0);
        delay_ms(500);
        printf("LED toggled\n");
    }
}

// ============================================================================
// 场景2：按键输入
// ============================================================================

void scenario_2_button_input(void)
{
    print_separator();
    printf("=== 场景2：按键输入 ===\n\n");
    // LED引脚（PA5）
    gpio_init(PA5);
    gpio_set_dir(PA5, GPIO_OUT);

    // 按键引脚（PC13，上拉输入）
    gpio_init(PC13);
    gpio_set_dir(PC13, GPIO_IN);
    gpio_pull_up(PC13);

    // 运行10秒
    for (int i = 0; i < 100; i++) {
        // 按键按下时（低电平）点亮LED
        if (!gpio_get(PC13)) {
            gpio_put(PA5, true);
        } else {
            gpio_put(PA5, false);
        }
        delay_ms(100);
    }
}

// ============================================================================
// 场景3：UART引脚复用配置
// ============================================================================

void scenario_3_uart_gpio(void)
{
    print_separator();
    printf("=== 场景3：UART引脚复用配置 ===\n\n");
    // UART1_TX: PA9, AF1
    gpio_set_function(PA9, GPIO_AF_1);

    // UART1_RX: PA10, AF1
    gpio_set_function(PA10, GPIO_AF_1);
    gpio_pull_up(PA10); // RX引脚上拉

    // 验证配置
    gpio_af_t tx_af = gpio_get_function(PA9);
    gpio_af_t rx_af = gpio_get_function(PA10);

    // 这里可以添加UART通信代码...
    (void)tx_af;
    (void)rx_af;
}

// ============================================================================
// 场景4：I2C引脚配置（开漏+上拉）
// ============================================================================

void scenario_4_i2c_gpio(void)
{
    print_separator();
    printf("=== 场景4：I2C引脚配置（开漏+上拉） ===\n\n");
    // I2C1_SCL: PB6, AF7, 开漏, 上拉
    gpio_set_function(PB6, GPIO_AF_7);
    gpio_set_output_type(PB6, GPIO_OTYPE_OD);
    gpio_pull_up(PB6);

    // I2C1_SDA: PB7, AF7, 开漏, 上拉
    gpio_set_function(PB7, GPIO_AF_7);
    gpio_set_output_type(PB7, GPIO_OTYPE_OD);
    gpio_pull_up(PB7);

    // 打印PB6（SCL）配置信息
    gpio_mode_t mode_scl = gpio_get_mode(PB6);
    gpio_otype_t otype_scl = gpio_get_output_type(PB6);
    gpio_pull_t pull_scl = gpio_get_pull(PB6);
    gpio_af_t af_scl = gpio_get_function(PB6);

    printf("PB6（I2C1_SCL）配置成功：\n");
    printf("  模式：%d (2=AF)\n", mode_scl);
    printf("  输出类型：%d (1=OD)\n", otype_scl);
    printf("  拉：%d (1=UP)\n", pull_scl);
    printf("  复用：%d (AF7=I2C)\n", af_scl);
    printf("  端口：%d (GPIOB)\n", GPIO_GET_PORT(PB6));
    printf("  位：%d\n\n", GPIO_GET_PIN_NUM(PB6));

    // 打印PB7（SDA）配置信息
    gpio_mode_t mode_sda = gpio_get_mode(PB7);
    gpio_otype_t otype_sda = gpio_get_output_type(PB7);
    gpio_pull_t pull_sda = gpio_get_pull(PB7);
    gpio_af_t af_sda = gpio_get_function(PB7);

    printf("PB7（I2C1_SDA）配置成功：\n");
    printf("  模式：%d (2=AF)\n", mode_sda);
    printf("  输出类型：%d (1=OD)\n", otype_sda);
    printf("  拉：%d (1=UP)\n", pull_sda);
    printf("  复用：%d (AF7=I2C)\n", af_sda);
    printf("  端口：%d (GPIOB)\n", GPIO_GET_PORT(PB7));
    printf("  位：%d\n\n", GPIO_GET_PIN_NUM(PB7));

    // 这里可以添加I2C通信代码...
}

// ============================================================================
// 场景5：高级配置（使用gpio_configure）
// ============================================================================

void scenario_5_advanced_config(void)
{
    print_separator();
    printf("=== 场景5：高级配置（使用gpio_configure） ===\n\n");
    // 使用配置结构体配置PA9（UART TX，AF1）
    gpio_config_t cfg = {
        .mode = GPIO_MODE_AF,
        .output_type = GPIO_OTYPE_PP,
        .pull = GPIO_PULL_NONE,
        .drive = GPIO_DRIVE_8MA, // 8mA驱动
        .af = GPIO_AF_1,
        .schmitt = true // 使能施密特触发器
    };

    gpio_configure(PA9, &cfg);

    // 验证配置
    gpio_mode_t mode = gpio_get_mode(PA9);
    gpio_otype_t otype = gpio_get_output_type(PA9);
    gpio_pull_t pull = gpio_get_pull(PA9);
    gpio_af_t af = gpio_get_function(PA9);
    bool locked = gpio_is_locked(PA9);
    bool state = gpio_get(PA9);

    printf("PA9配置成功：\n");
    printf("  模式：%d (2=AF)\n", mode);
    printf("  输出类型：%d (0=PP)\n", otype);
    printf("  拉：%d (0=NONE)\n", pull);
    printf("  复用：%d (AF1)\n", af);
    printf("  端口：%d (GPIOA)\n", GPIO_GET_PORT(PA9));
    printf("  位：%d\n", GPIO_GET_PIN_NUM(PA9));
    printf("  锁：%d\n", locked);
    printf("  状态：%d\n", state);
}

// ============================================================================
// 场景6：SPI高速信号（高驱动能力）
// ============================================================================

void scenario_6_spi_high_speed(void)
{
    print_separator();
    printf("=== 场景6：SPI高速信号（高驱动能力） ===\n\n");
    // SPI1_SCK: PA5, AF0, 高驱动
    gpio_set_function(PA5, GPIO_AF_0);
    gpio_set_drive_strength(PA5, GPIO_DRIVE_16MA);

    // SPI1_MOSI: PA7, AF0
    gpio_set_function(PA7, GPIO_AF_0);
    gpio_set_drive_strength(PA7, GPIO_DRIVE_16MA);

    // SPI1_MISO: PA6, AF0, 上拉
    gpio_set_function(PA6, GPIO_AF_0);
    gpio_pull_up(PA6);

    // 打印PA5（SCK）配置信息
    gpio_mode_t mode_sck = gpio_get_mode(PA5);
    gpio_otype_t otype_sck = gpio_get_output_type(PA5);
    gpio_af_t af_sck = gpio_get_function(PA5);

    printf("PA5（SPI1_SCK）配置成功：\n");
    printf("  模式：%d (2=AF)\n", mode_sck);
    printf("  输出类型：%d (0=PP)\n", otype_sck);
    printf("  复用：%d (AF0=SPI)\n", af_sck);
    printf("  驱动能力：16mA\n");
    printf("  端口：%d (GPIOA)\n", GPIO_GET_PORT(PA5));
    printf("  位：%d\n\n", GPIO_GET_PIN_NUM(PA5));

    // 打印PA7（MOSI）配置信息
    gpio_mode_t mode_mosi = gpio_get_mode(PA7);
    gpio_af_t af_mosi = gpio_get_function(PA7);

    printf("PA7（SPI1_MOSI）配置成功：\n");
    printf("  模式：%d (2=AF)\n", mode_mosi);
    printf("  复用：%d (AF0=SPI)\n", af_mosi);
    printf("  驱动能力：16mA\n");
    printf("  端口：%d (GPIOA)\n", GPIO_GET_PORT(PA7));
    printf("  位：%d\n\n", GPIO_GET_PIN_NUM(PA7));

    // 打印PA6（MISO）配置信息
    gpio_mode_t mode_miso = gpio_get_mode(PA6);
    gpio_pull_t pull_miso = gpio_get_pull(PA6);
    gpio_af_t af_miso = gpio_get_function(PA6);

    printf("PA6（SPI1_MISO）配置成功：\n");
    printf("  模式：%d (2=AF)\n", mode_miso);
    printf("  拉：%d (1=UP)\n", pull_miso);
    printf("  复用：%d (AF0=SPI)\n", af_miso);
    printf("  端口：%d (GPIOA)\n", GPIO_GET_PORT(PA6));
    printf("  位：%d\n\n", GPIO_GET_PIN_NUM(PA6));

    // 这里可以添加SPI通信代码...
}

// ============================================================================
// 场景7：端口批量操作
// ============================================================================

void scenario_7_port_operations(void)
{
    print_separator();
    printf("=== 场景7：端口批量操作 ===\n\n");
    // 配置PA0-PA7为输出（使用端口操作）
    for (int i = 0; i < 8; i++) {
        gpio_init(PA0 + i);
        gpio_set_dir(PA0 + i, GPIO_OUT);
    }

    // 打印PA0配置信息（示例）
    gpio_mode_t mode = gpio_get_mode(PA0);
    gpio_dir_t dir = gpio_get_dir(PA0);
    gpio_otype_t otype = gpio_get_output_type(PA0);

    printf("PA0-PA7配置成功（以PA0为例）：\n");
    printf("  模式：%d (1=OUT)\n", mode);
    printf("  方向：%d (1=OUT)\n", dir);
    printf("  输出类型：%d (0=PP)\n", otype);
    printf("  端口：%d (GPIOA)\n\n", GPIO_GET_PORT(PA0));

    // 批量设置PA0-PA3
    printf("批量设置PA0-PA3...\n");
    gpio_port_set_masked(0, 0x000F); // GPIOA, bit0-3
    delay_ms(500);

    // 批量清除PA0-PA3
    printf("批量清除PA0-PA3...\n");
    gpio_port_clear_masked(0, 0x000F);
    delay_ms(500);

    // 写入整个端口
    printf("写入端口：0xAA (PA0,PA2,PA4,PA6=1)...\n");
    gpio_port_put(0, 0x00AA); // PA0, PA2, PA4, PA6 = 1
    delay_ms(500);

    printf("写入端口：0x55 (PA1,PA3,PA5,PA7=1)...\n");
    gpio_port_put(0, 0x0055); // PA1, PA3, PA5, PA7 = 1
    delay_ms(500);

    // 清零
    printf("清零端口...\n");
    gpio_port_put(0, 0x0000);
}

// ============================================================================
// 场景8：配置锁定演示
// ============================================================================

void scenario_8_lock_demo(void)
{
    print_separator();
    printf("=== 场景8：配置锁定演示 ===\n\n");
    // 配置PA8为输出
    gpio_init(PA8);
    gpio_set_dir(PA8, GPIO_OUT);
    gpio_set_drive_strength(PA8, GPIO_DRIVE_8MA);

    // 打印锁定前的配置
    gpio_mode_t mode = gpio_get_mode(PA8);
    gpio_otype_t otype = gpio_get_output_type(PA8);
    bool locked_before = gpio_is_locked(PA8);

    printf("PA8锁定前配置：\n");
    printf("  模式：%d (1=OUT)\n", gpio_get_dir(PA8));
    printf("  输出类型：%d (0=PP)\n", otype);
    printf("  拉：%d (0=NONE)\n", gpio_get_pull(PA8));
    printf("  驱动能力：%d (8mA)\n", gpio_get_drive_strength(PA8));
    printf("  锁定状态：%d (0=未锁定)\n\n", locked_before);

    // 锁定配置
    printf("执行锁定操作...\n");
    gpio_lock(PA8);

    // 验证锁定状态
    bool locked_after = gpio_is_locked(PA8);

    printf("PA8锁定后配置：\n");
    printf("  锁定状态：%d (1=已锁定)\n", locked_after);
    printf("  拉：%d (0=NONE)\n", gpio_get_pull(PA8));
    printf("  驱动能力：%d (8mA)\n", gpio_get_drive_strength(PA8));
    printf("  端口：%d (GPIOA)\n", GPIO_GET_PORT(PA8));
    printf("  位：%d\n\n", GPIO_GET_PIN_NUM(PA8));

    // 尝试修改配置（会失败，因为已锁定）
    printf("尝试修改驱动能力到16mA（会失败）...\n");
    gpio_set_drive_strength(PA8, GPIO_DRIVE_16MA); // 这个操作无效
    gpio_set_pulls(PA8, GPIO_PULL_UP);

    printf("配置已被锁定，无法修改\n\n");

    // 打印锁定后的配置
    gpio_mode_t mode_after = gpio_get_mode(PA8);
    gpio_otype_t otype_after = gpio_get_output_type(PA8);
    gpio_pull_t pull_after = gpio_get_pull(PA8);
    gpio_af_t af_after = gpio_get_function(PA8);
    gpio_drive_t drive_after = gpio_get_drive_strength(PA8);

    printf("PA8锁定后配置：\n");
    printf("  模式：%d (1=OUT)\n", mode_after);
    printf("  输出类型：%d (0=PP)\n", otype_after);
    printf("  拉：%d (0=NONE)\n", pull_after);
    printf("  复用：%d (AF0)\n", af_after);
    printf("  驱动能力：%d (8mA)\n", drive_after);
    printf("  端口：%d (GPIOA)\n", GPIO_GET_PORT(PA8));
    printf("  位：%d\n\n", GPIO_GET_PIN_NUM(PA8));


    // 但仍然可以改变输出电平（锁定不影响输出控制）
    printf("但输出电平仍可控制：\n");
    gpio_put(PA8, true);
    printf("  PA8 set to HIGH\n");
    delay_ms(10);
    gpio_put(PA8, false);
    printf("  PA8 set to LOW\n");
    delay_ms(10);

    (void)mode;
}

// ============================================================================
// 主函数
// ============================================================================

int main(void)
{
    system_init();
    SEGGER_RTT_Init();

    // 打印欢迎信息
    printf("\n\n");
    print_separator();
    printf("  ACM32P4 GPIO 示例程序\n");
    printf("  SDK Version: 1.0\n");
    print_separator();
    printf("\n");
    // 根据选择运行场景
    switch (DEFAULT_SCENARIO) {
    case 1:
        scenario_1_led_blink();
        break;
    case 2:
        scenario_2_button_input();
        break;
    case 3:
        scenario_3_uart_gpio();
        break;
    case 4:
        scenario_4_i2c_gpio();
        break;
    case 5:
        scenario_5_advanced_config();
        break;
    case 6:
        scenario_6_spi_high_speed();
        break;
    case 7:
        scenario_7_port_operations();
        break;
    case 8:
        scenario_8_lock_demo();
        break;
    default:
        scenario_1_led_blink();
        break;
    }
    // 主循环：初始化PA5为输出以演示
    gpio_init(PA5);
    gpio_set_dir(PA5, GPIO_OUT);
    printf("\n进入主循环（PA5 toggle）...\n");
    while (1) {
        gpio_toggle(PA5);
        delay_ms(1000);
    }

    return 0;
}
