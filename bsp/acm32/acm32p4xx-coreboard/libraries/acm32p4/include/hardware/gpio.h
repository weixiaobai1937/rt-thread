/**
 * @file gpio.h
 * @brief ACM32P4 GPIO驱动 - Pico SDK风格简洁API
 * 
 * @details
 * 极简GPIO控制接口，3行代码点亮LED：
 * @code
 * gpio_init(PA5);
 * gpio_set_dir(PA5, GPIO_OUT);
 * gpio_put(PA5, 1);
 * @endcode
 * 
 * 功能特性：
 * - 线性引脚编号（PA0=0, PB0=16, ...）
 * - 自动时钟使能，零配置
 * - 语义化函数名，函数名即文档
 * - 分层API设计，按需使用
 */

#ifndef ACM32P4_GPIO_H
#define ACM32P4_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// 类型定义
//=============================================================================

/**
 * @brief GPIO引脚编号（线性编号：0-127）
 * 
 * @note 编号规则：pin = port * 16 + pin_num
 *       PA0=0, PA1=1, ..., PA15=15
 *       PB0=16, PB1=17, ..., PB15=31
 *       ...
 *       PH0=112, PH1=113, ..., PH15=127
 */
typedef uint16_t gpio_pin_t;

/**
 * @brief GPIO方向
 */
typedef enum
{
    GPIO_IN  = 0,   ///< 输入模式
    GPIO_OUT = 1    ///< 输出模式
} gpio_dir_t;

/**
 * @brief GPIO工作模式
 */
typedef enum
{
    GPIO_MODE_INPUT  = 0x00,    ///< 输入模式
    GPIO_MODE_OUTPUT = 0x01,    ///< 通用输出模式
    GPIO_MODE_AF     = 0x02,    ///< 复用功能模式
    GPIO_MODE_ANALOG = 0x03     ///< 模拟功能模式
} gpio_mode_t;

/**
 * @brief GPIO输出类型
 */
typedef enum
{
    GPIO_OTYPE_PP = 0,  ///< 推挽输出（Push-Pull）
    GPIO_OTYPE_OD = 1   ///< 开漏输出（Open-Drain）
} gpio_otype_t;

/**
 * @brief GPIO上下拉配置
 */
typedef enum
{
    GPIO_PULL_NONE = 0x00,  ///< 无上下拉（浮空）
    GPIO_PULL_UP   = 0x01,  ///< 上拉
    GPIO_PULL_DOWN = 0x02   ///< 下拉
} gpio_pull_t;

/**
 * @brief GPIO驱动能力（标准IO）
 */
typedef enum
{
    GPIO_DRIVE_2MA  = 0x00,  ///< 2mA (默认)
    GPIO_DRIVE_4MA  = 0x01,  ///< 4mA
    GPIO_DRIVE_6MA  = 0x02,  ///< 6mA
    GPIO_DRIVE_8MA  = 0x03,  ///< 8mA
    GPIO_DRIVE_10MA = 0x04,  ///< 10mA
    GPIO_DRIVE_12MA = 0x05,  ///< 12mA
    GPIO_DRIVE_14MA = 0x06,  ///< 14mA
    GPIO_DRIVE_16MA = 0x07   ///< 16mA
} gpio_drive_t;

/**
 * @brief GPIO驱动能力（大驱动IO）
 * 
 * @note 只有部分引脚支持大驱动能力
 */
typedef enum
{
    GPIO_DRIVE_HIGH_11MA = 0x00,  ///< 11mA
    GPIO_DRIVE_HIGH_14MA = 0x01,  ///< 14mA
    GPIO_DRIVE_HIGH_17MA = 0x02,  ///< 17mA
    GPIO_DRIVE_HIGH_20MA = 0x03,  ///< 20mA
    GPIO_DRIVE_HIGH_23MA = 0x04,  ///< 23mA
    GPIO_DRIVE_HIGH_26MA = 0x05,  ///< 26mA
    GPIO_DRIVE_HIGH_29MA = 0x06,  ///< 29mA
    GPIO_DRIVE_HIGH_32MA = 0x07   ///< 32mA
} gpio_drive_high_t;

/**
 * @brief GPIO复用功能编号
 */
typedef enum
{
    GPIO_AF_0  = 0,
    GPIO_AF_1  = 1,
    GPIO_AF_2  = 2,
    GPIO_AF_3  = 3,
    GPIO_AF_4  = 4,
    GPIO_AF_5  = 5,
    GPIO_AF_6  = 6,
    GPIO_AF_7  = 7,
    GPIO_AF_8  = 8,
    GPIO_AF_9  = 9,
    GPIO_AF_10 = 10,
    GPIO_AF_11 = 11,
    GPIO_AF_12 = 12,
    GPIO_AF_13 = 13,
    GPIO_AF_14 = 14,
    GPIO_AF_15 = 15
} gpio_af_t;



/**
 * @brief GPIO完整配置结构体（用于高级配置）
 */
typedef struct
{
    gpio_mode_t   mode;          ///< 工作模式
    gpio_otype_t  output_type;   ///< 输出类型（PP/OD）
    gpio_pull_t   pull;          ///< 上下拉配置
    gpio_drive_t  drive;         ///< 驱动能力
    gpio_af_t     af;            ///< 复用功能编号
    bool          schmitt;       ///< 施密特触发器使能
} gpio_config_t;

//=============================================================================
// 引脚编号宏定义（128个引脚）
//=============================================================================

// 辅助宏：计算线性引脚编号
#define GPIO_PIN(port, pin)  ((port) * 16 + (pin))

// 引脚有效性检查（P4芯片仅8端口：GPIOA-H，共128引脚）
#define GPIO_PIN_VALID(pin)      ((pin) < 128)
#define GPIO_GET_PORT(pin)       ((pin) / 16)
#define GPIO_GET_PIN_NUM(pin)    ((pin) % 16)

/**
 * @brief GPIO引脚编号枚举
 * @details 定义所有GPIO引脚的编号，从PA0(0)到PH15(127)
 *          每个端口包含16个引脚
 */
typedef enum
{
    // GPIOA (0-15)
    PA0  = 0,
    PA1  = 1,
    PA2  = 2,
    PA3  = 3,
    PA4  = 4,
    PA5  = 5,
    PA6  = 6,
    PA7  = 7,
    PA8  = 8,
    PA9  = 9,
    PA10 = 10,
    PA11 = 11,
    PA12 = 12,
    PA13 = 13,
    PA14 = 14,
    PA15 = 15,

    // GPIOB (16-31)
    PB0  = 16,
    PB1  = 17,
    PB2  = 18,
    PB3  = 19,
    PB4  = 20,
    PB5  = 21,
    PB6  = 22,
    PB7  = 23,
    PB8  = 24,
    PB9  = 25,
    PB10 = 26,
    PB11 = 27,
    PB12 = 28,
    PB13 = 29,
    PB14 = 30,
    PB15 = 31,

    // GPIOC (32-47)
    PC0  = 32,
    PC1  = 33,
    PC2  = 34,
    PC3  = 35,
    PC4  = 36,
    PC5  = 37,
    PC6  = 38,
    PC7  = 39,
    PC8  = 40,
    PC9  = 41,
    PC10 = 42,
    PC11 = 43,
    PC12 = 44,
    PC13 = 45,
    PC14 = 46,
    PC15 = 47,

    // GPIOD (48-63)
    PD0  = 48,
    PD1  = 49,
    PD2  = 50,
    PD3  = 51,
    PD4  = 52,
    PD5  = 53,
    PD6  = 54,
    PD7  = 55,
    PD8  = 56,
    PD9  = 57,
    PD10 = 58,
    PD11 = 59,
    PD12 = 60,
    PD13 = 61,
    PD14 = 62,
    PD15 = 63,

    // GPIOE (64-79)
    PE0  = 64,
    PE1  = 65,
    PE2  = 66,
    PE3  = 67,
    PE4  = 68,
    PE5  = 69,
    PE6  = 70,
    PE7  = 71,
    PE8  = 72,
    PE9  = 73,
    PE10 = 74,
    PE11 = 75,
    PE12 = 76,
    PE13 = 77,
    PE14 = 78,
    PE15 = 79,

    // GPIOF (80-95)
    PF0  = 80,
    PF1  = 81,
    PF2  = 82,
    PF3  = 83,
    PF4  = 84,
    PF5  = 85,
    PF6  = 86,
    PF7  = 87,
    PF8  = 88,
    PF9  = 89,
    PF10 = 90,
    PF11 = 91,
    PF12 = 92,
    PF13 = 93,
    PF14 = 94,
    PF15 = 95,

    // GPIOG (96-111)
    PG0  = 96,
    PG1  = 97,
    PG2  = 98,
    PG3  = 99,
    PG4  = 100,
    PG5  = 101,
    PG6  = 102,
    PG7  = 103,
    PG8  = 104,
    PG9  = 105,
    PG10 = 106,
    PG11 = 107,
    PG12 = 108,
    PG13 = 109,
    PG14 = 110,
    PG15 = 111,

    // GPIOH (112-127)
    PH0  = 112,
    PH1  = 113,
    PH2  = 114,
    PH3  = 115,
    PH4  = 116,
    PH5  = 117,
    PH6  = 118,
    PH7  = 119,
    PH8  = 120,
    PH9  = 121,
    PH10 = 122,
    PH11 = 123,
    PH12 = 124,
    PH13 = 125,
    PH14 = 126,
    PH15 = 127
} gpio_pin_name_t;


//=============================================================================
// 第1层：核心API（90%场景只需这些）
//=============================================================================

/**
 * @brief 初始化GPIO（默认输入浮空）
 * @param pin 引脚编号
 * 
 * @note 自动使能时钟，无需手动配置
 */
void gpio_init(gpio_pin_t pin);

/**
 * @brief 设置方向
 * @param pin 引脚编号
 * @param dir GPIO_IN 或 GPIO_OUT
 */
void gpio_set_dir(gpio_pin_t pin, gpio_dir_t dir);

/**
 * @brief 输出电平
 * @param pin 引脚编号
 * @param value 1=高电平, 0=低电平
 */
void gpio_put(gpio_pin_t pin, bool value);

/**
 * @brief 读取电平
 * @param pin 引脚编号
 * @return 1=高电平, 0=低电平
 */
bool gpio_get(gpio_pin_t pin);

/**
 * @brief 翻转电平
 * @param pin 引脚编号
 */
void gpio_toggle(gpio_pin_t pin);

/**
 * @brief 使能上拉
 * @param pin 引脚编号
 */
void gpio_pull_up(gpio_pin_t pin);

/**
 * @brief 使能下拉
 * @param pin 引脚编号
 */
void gpio_pull_down(gpio_pin_t pin);

/**
 * @brief 禁用上下拉
 * @param pin 引脚编号
 */
void gpio_disable_pulls(gpio_pin_t pin);

/**
 * @brief 设置上下拉模式
 * @param pin 引脚编号
 * @param pull GPIO_PULL_NONE/UP/DOWN
 */
void gpio_set_pulls(gpio_pin_t pin, gpio_pull_t pull);

//=============================================================================
// 便利函数（简化常用场景）
//=============================================================================

/**
 * @brief 一步初始化输出引脚
 * @param pin 引脚编号
 * @param value 初始电平（1=高, 0=低）
 * 
 * @code
 * gpio_init_output(PA5, 1);  // PA5配置为输出并置高
 * @endcode
 */
static inline void gpio_init_output(gpio_pin_t pin, bool value) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, value);
}

/**
 * @brief 一步初始化输入引脚（带上拉）
 * @param pin 引脚编号
 * 
 * @code
 * gpio_init_input_pullup(PA10);  // PA10配置为上拉输入
 * @endcode
 */
static inline void gpio_init_input_pullup(gpio_pin_t pin) {
    gpio_init(pin);
    gpio_pull_up(pin);
}

/**
 * @brief 一步初始化输入引脚（带下拉）
 * @param pin 引脚编号
 */
static inline void gpio_init_input_pulldown(gpio_pin_t pin) {
    gpio_init(pin);
    gpio_pull_down(pin);
}

/**
 * @brief 设置输出高电平
 * @param pin 引脚编号
 */
static inline void gpio_set(gpio_pin_t pin) {
    gpio_put(pin, 1);
}

/**
 * @brief 设置输出低电平
 * @param pin 引脚编号
 */
static inline void gpio_clear(gpio_pin_t pin) {
    gpio_put(pin, 0);
}

//=============================================================================
// 第2层：复用功能
//=============================================================================

/**
 * @brief 设置复用功能
 * @param pin 引脚编号
 * @param af 功能编号（GPIO_AF_0 ~ GPIO_AF_15）
 * 
 * @code
 * gpio_set_function(PB0, GPIO_AF_7);  // UART
 * @endcode
 */
void gpio_set_function(gpio_pin_t pin, gpio_af_t af);

/**
 * @brief 获取当前复用功能
 * @param pin 引脚编号
 * @return 功能编号
 */
gpio_af_t gpio_get_function(gpio_pin_t pin);

//=============================================================================
// 第3层：输出特性（高速/大电流场景）
//=============================================================================

/**
 * @brief 设置输出类型
 * @param pin 引脚编号
 * @param otype GPIO_OTYPE_PP（推挽）或 GPIO_OTYPE_OD（开漏）
 */
void gpio_set_output_type(gpio_pin_t pin, gpio_otype_t otype);

/**
 * @brief 设置驱动强度
 * @param pin 引脚编号
 * @param drive GPIO_DRIVE_2MA ~ GPIO_DRIVE_16MA
 */
void gpio_set_drive_strength(gpio_pin_t pin, gpio_drive_t drive);

/**
 * @brief 设置大驱动强度（仅部分引脚）
 * @param pin 引脚编号
 * @param drive GPIO_DRIVE_HIGH_11MA ~ GPIO_DRIVE_HIGH_32MA
 * 
 * @note 仅支持大驱动的引脚有效，见芯片手册
 */
void gpio_set_drive_strength_high(gpio_pin_t pin, gpio_drive_high_t drive);

/**
 * @brief 使能施密特触发器
 * @param pin 引脚编号
 * @param enable true=使能（提高抗干扰）
 */
void gpio_set_schmitt(gpio_pin_t pin, bool enable);

//=============================================================================
// 第4层：模拟和批量配置
//=============================================================================

/**
 * @brief 配置为模拟模式
 * @param pin 引脚编号
 * 
 * @note ADC/DAC/COMP使用
 */
void gpio_set_analog_mode(gpio_pin_t pin);

/**
 * @brief 控制模拟开关
 * @param pin 引脚编号
 * @param enable true=闭合（慢速ADC通道）
 * 
 * @note 快速ADC通道需设为false
 */
void gpio_set_analog_switch(gpio_pin_t pin, bool enable);

/**
 * @brief 批量配置引脚
 * @param pin 引脚编号
 * @param config 配置结构体
 * 
 * @code
 * gpio_config_t cfg = {
 *     .mode = GPIO_MODE_OUTPUT,
 *     .output_type = GPIO_OTYPE_PP,
 *     .pull = GPIO_PULL_NONE,
 *     .drive = GPIO_DRIVE_8MA,
 *     .schmitt = false
 * };
 * gpio_configure(PA5, &cfg);
 * @endcode
 */
void gpio_configure(gpio_pin_t pin, const gpio_config_t *config);

//=============================================================================
// 第5层：保护功能（安全关键场景）
//=============================================================================

/**
 * @brief 锁定配置
 * @param pin 引脚编号
 * 
 * @note 锁定后直到复位前无法修改
 * @note 用于安全关键应用
 */
void gpio_lock(gpio_pin_t pin);

/**
 * @brief 检查是否锁定
 * @param pin 引脚编号
 * @return true=已锁定
 */
bool gpio_is_locked(gpio_pin_t pin);

//=============================================================================
// 第6层：端口操作（并行IO场景）
//=============================================================================

/**
 * @brief 批量置位
 * @param port 端口号（0=GPIOA, 1=GPIOB, ...）
 * @param mask 引脚掩码（bit0-15）
 * 
 * @code
 * gpio_port_set_masked(0, 0x00FF);  // PA0-7置高
 * @endcode
 */
void gpio_port_set_masked(uint8_t port, uint16_t mask);

/**
 * @brief 批量清零
 * @param port 端口号
 * @param mask 引脚掩码
 */
void gpio_port_clear_masked(uint8_t port, uint16_t mask);

/**
 * @brief 读取整个端口
 * @param port 端口号
 * @return 16位输入值
 */
uint16_t gpio_port_get(uint8_t port);

/**
 * @brief 写入整个端口
 * @param port 端口号
 * @param value 16位输出值
 */
void gpio_port_put(uint8_t port, uint16_t value);

//=============================================================================
// 第7层：状态查询
//=============================================================================

/**
 * @brief 获取工作模式
 * @param pin 引脚编号
 * @return GPIO_MODE_INPUT/OUTPUT/AF/ANALOG
 */
gpio_mode_t gpio_get_mode(gpio_pin_t pin);

/**
 * @brief 获取方向
 * @param pin 引脚编号
 * @return GPIO_IN 或 GPIO_OUT
 */
gpio_dir_t gpio_get_dir(gpio_pin_t pin);

/**
 * @brief 获取输出类型
 * @param pin 引脚编号
 * @return GPIO_OTYPE_PP 或 GPIO_OTYPE_OD
 */
gpio_otype_t gpio_get_output_type(gpio_pin_t pin);

/**
 * @brief 获取上下拉配置
 * @param pin 引脚编号
 * @return GPIO_PULL_NONE/UP/DOWN
 */
gpio_pull_t gpio_get_pull(gpio_pin_t pin);

/**
 * @brief 获取驱动能力
 * @param pin 引脚编号
 * @return GPIO_DRIVE_2MA ~ GPIO_DRIVE_16MA
 */
gpio_drive_t gpio_get_drive_strength(gpio_pin_t pin);


#ifdef __cplusplus
}
#endif

#endif // ACM32P4_GPIO_H
