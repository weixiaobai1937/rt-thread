/**
 * @file i2c.h
 * @brief ACM32P4 I2C 驱动头文件
 *
 * 本文件定义了 I2C 驱动的公开 API，支持以下功能：
 * - 主模式和从模式初始化（标准/快速/快速增强）
 * - 多从机地址（最多3个）
 * - 阻塞式轮询收发（主模式）
 * - 中断驱动收发（统一回调接口，14路中断源）
 * - DMA 使能配置
 * - SCL/SDA 滤波与 SDA 跳变检测
 * - SMBus 2.0 超时配置
 * - 从机拉时钟修正
 * - 多主机仲裁配置
 * - 开漏模式配置
 *
 * @note 中断架构：
 *   每个I2C实例对应一个独立中断向量：
 *   - I2C1：I2C1_IRQHandler（IRQn=24）
 *   - I2C2：I2C2_IRQHandler（IRQn=25）
 *   用户通过 i2c_irq_register() 注册回调，回调中通过 i2c_irq_context_t 的
 *   flags 字段判断中断事件类型。
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#ifndef _HARDWARE_I2C_H
#define _HARDWARE_I2C_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

//===========================================
// 宏定义
//===========================================

/** @brief 默认轮询超时时间（毫秒） */
#define I2C_DEFAULT_TIMEOUT_MS  100U

/** @brief SCL时钟分频最小值（硬件约束：CLK_DIV必须 > 2） */
#define I2C_CLK_DIV_MIN         3U

/** @brief SCL时钟分频最大值（12位寄存器） */
#define I2C_CLK_DIV_MAX         4095U

/** @brief SDA跳变检测计数器最大值 */
#define I2C_DET_CNT_MAX         65535U

//===========================================
// 枚举类型定义
//===========================================

/**
 * @brief I2C外设实例
 */
typedef enum {
    I2C_1     = 0,  ///< I2C1，基地址0x40005400
    I2C_2     = 1,  ///< I2C2，基地址0x40005800
    I2C_COUNT = 2,  ///< I2C实例总数（用于范围检查）
} i2c_instance_t;

/**
 * @brief I2C中断类型（位掩码，可组合使用）
 *
 * 与 I2C_CR 寄存器中对应中断使能位一一对应。
 * 多个中断类型可通过 OR 运算组合传入 i2c_irq_enable/disable()。
 */
typedef enum {
    I2C_IRQ_MTF        = (1U << 0),   ///< 字节传输完成（I2C_CR.MTF_INT_EN）
    I2C_IRQ_RX_ADDR1   = (1U << 1),   ///< 从设备地址1匹配（I2C_CR.RX_ADDR1_INT_EN）
    I2C_IRQ_TXE        = (1U << 2),   ///< 发送数据寄存器空（I2C_CR.TXE_INT_EN）
    I2C_IRQ_RXNE       = (1U << 3),   ///< 接收数据寄存器非空（I2C_CR.RXNE_INT_EN）
    I2C_IRQ_OVR        = (1U << 4),   ///< 从机上溢/下溢（I2C_CR.OVR_INT_EN）
    I2C_IRQ_RX_ADDR2   = (1U << 5),   ///< 从设备地址2匹配（I2C_CR.RX_ADDR2_INT_EN）
    I2C_IRQ_DETR       = (1U << 6),   ///< SDA跳变检测（I2C_CR.DETR_INT_EN）
    I2C_IRQ_RX_ADDR3   = (1U << 7),   ///< 从设备地址3匹配（I2C_CR.RX_ADDR3_INT_EN）
    I2C_IRQ_STOPF      = (1U << 8),   ///< STOP条件检测（I2C_CR.STOPF_INT_EN）
    I2C_IRQ_TIMEOUTAF  = (1U << 9),   ///< TIMEOUTA超时（I2C_TIMEOUT.TOUTA_INT_EN，SMBus）
    I2C_IRQ_TIMEOUTBF  = (1U << 10),  ///< TIMEOUTB超时（I2C_TIMEOUT.TOUTB_INT_EN，SMBus）
    I2C_IRQ_NACKF      = (1U << 11),  ///< 接收到NACK（I2C_CR.NACKF_INT_EN）
    I2C_IRQ_TX_RX_FLAG = (1U << 12),  ///< 主机发送转接收（I2C_CR.TX_RX_FLAG_INT_EN）
    I2C_IRQ_MARLO      = (1U << 13),  ///< 主机仲裁丢失（I2C_CR.MARLO_INT_EN）
    I2C_IRQ_ALL        = 0x3FFFU,     ///< 所有中断（批量使能/禁用）
} i2c_irq_type_t;

/**
 * @brief I2C状态标志（对应I2C_SR寄存器各位）
 *
 * 用于 i2c_flag_is_set() 和 i2c_flag_clear() 函数。
 * 标志位值与SR寄存器位掩码一一对应。
 *
 * @note 只读标志（RACK/SRW/BUS_BUSY）不支持写1清0。
 * @note TXE/RXNE 通过写DR/读DR操作清零，不支持直接写1清0。
 */
typedef enum {
    I2C_FLAG_RACK       = (1U << 0),   ///< NACK检测（0=ACK，1=NACK，只读，START清0）
    I2C_FLAG_STARTF     = (1U << 1),   ///< 起始条件已发送（写DR自动清0，也可写1清0）
    I2C_FLAG_STOPF      = (1U << 2),   ///< STOP条件检测（写1清0）
    I2C_FLAG_SRW        = (1U << 3),   ///< 从机读写状态（只读：1=从机发送，0=从机接收）
    I2C_FLAG_BUS_BUSY   = (1U << 4),   ///< 总线忙碌（只读）
    I2C_FLAG_TX_RX_FLAG = (1U << 5),   ///< 主机发送转接收（写1清0，清0后产生SCL）
    I2C_FLAG_MARLO      = (1U << 6),   ///< 主机仲裁丢失（写1清0）
    I2C_FLAG_MTF        = (1U << 7),   ///< 字节传输完成（写1清0）
    I2C_FLAG_RX_ADDR1   = (1U << 8),   ///< 从设备地址1匹配（写1清0）
    I2C_FLAG_TXE        = (1U << 9),   ///< 发送数据寄存器空（写DR清0）
    I2C_FLAG_RXNE       = (1U << 10),  ///< 接收数据寄存器非空（读DR清0）
    I2C_FLAG_OVR        = (1U << 11),  ///< 从机上溢/下溢（写1清0）
    I2C_FLAG_RX_ADDR2   = (1U << 12),  ///< 从设备地址2匹配（写1清0）
    I2C_FLAG_DETR       = (1U << 13),  ///< SDA跳变检测（写1清0）
    I2C_FLAG_RX_ADDR3   = (1U << 14),  ///< 从设备地址3匹配（写1清0）
    I2C_FLAG_TIMEOUTAF  = (1U << 15),  ///< TIMEOUTA超时（写1清0）
    I2C_FLAG_TIMEOUTBF  = (1U << 16),  ///< TIMEOUTB超时（写1清0）
    I2C_FLAG_NACKF      = (1U << 17),  ///< 接收到NACK（写1清0）
} i2c_flag_t;

/**
 * @brief I2C应答/非应答控制（对应I2C_CR.TACK）
 */
typedef enum {
    I2C_ACK  = 0,  ///< 接收一字节后产生ACK
    I2C_NACK = 1,  ///< 接收一字节后产生NACK（接收最后一字节前设置）
} i2c_ack_t;

//===========================================
// 结构体类型定义
//===========================================

/**
 * @brief I2C主模式配置结构体
 *
 * 用于 i2c_config_master()，涵盖主模式下所有可配置项。
 */
typedef struct {
    uint16_t clk_div;          ///< 时钟分频值（Fscl=Fpclk/(4×(clk_div+1))），必须≥I2C_CLK_DIV_MIN
    bool     tx_auto_en;       ///< SDA数据线方向自动切换（推荐使能）
    bool     multi_master_en;  ///< 多主机使能（总线有多个主机时必须置true）
    bool     high_wait_en;     ///< 主机时钟高等待使能（推荐使能，兼容慢速从机）
    bool     marlo_sel;        ///< 仲裁丢失后自动切换从机（true=自动切换，false=不切换）
} i2c_master_config_t;

/**
 * @brief I2C从模式配置结构体
 *
 * 用于 i2c_config_slave()，涵盖从模式下所有可配置项，支持最多3个从机地址。
 */
typedef struct {
    uint8_t addr1;         ///< 从机地址1（7位，必须配置，有效范围0x08~0x77）
    uint8_t addr2;         ///< 从机地址2（7位，addr2_en=true时生效）
    bool    addr2_en;      ///< 从机地址2使能
    uint8_t addr3;         ///< 从机地址3（7位，addr3_en=true时生效）
    bool    addr3_en;      ///< 从机地址3使能
    bool    nostretch;     ///< 禁止时钟延长（true=不拉低SCL等待，false=允许拉低）
    bool    tx_auto_en;    ///< SDA数据线方向自动切换（推荐使能）
    bool    txe_sel;       ///< 地址匹配后TXE立即置高（DMA发送模式必须置true）
} i2c_slave_config_t;

/**
 * @brief I2C SCL/SDA滤波配置结构体
 *
 * 用于 i2c_config_filter()。
 * SCL_FILTER=0 表示禁用滤波；非0时滤除宽度小于 Tcntc×SCL_FILTER 的毛刺，
 * Tcntc 为 PCLK 的8分频时钟周期。
 *
 * @note 建议 sda_in_delay 与 scl_filter 设置相同值，保持SDA/SCL输入相位一致。
 */
typedef struct {
    uint8_t scl_filter;    ///< SCL滤波值（0=禁用，1-31=滤波级数）
    uint8_t sda_in_delay;  ///< SDA输入延时（建议与scl_filter相同，0-31）
} i2c_filter_config_t;

/**
 * @brief I2C SMBus超时配置结构体
 *
 * 用于 i2c_config_smbus_timeout()，仅在需要SMBus功能时使用。
 * 超时时长 = TIMEOUTA/B值 × 2048 × PCLK周期。
 *
 * @note TimeoutA：检测SCL低电平持续时间（对应SMBus tTIMEOUT，最大35ms）。
 * @note TimeoutB：检测累积时钟低电平延长时间（tLOW:SEXT或tLOW:MEXT）。
 */
typedef struct {
    uint16_t timeouta;          ///< TimeoutA值（[11:0]，SCL低电平超时计数）
    bool     timeouta_en;       ///< TimeoutA定时使能（I2C_TIMEOUT.TIMOUTEN）
    bool     timeouta_irq_en;   ///< TimeoutA超时中断使能（I2C_TIMEOUT.TOUTA_INTEN）
    uint16_t timeoutb;          ///< TimeoutB值（[11:0]，累积低电平延长超时计数）
    bool     timeoutb_en;       ///< TimeoutB定时使能（I2C_TIMEOUT.EXTEN）
    bool     timeoutb_irq_en;   ///< TimeoutB超时中断使能（I2C_TIMEOUT.TOUTB_INTEN）
    bool     ext_mode;          ///< EXT计数模式（false=SEXT从机，true=MEXT主机）
} i2c_smbus_timeout_config_t;

/**
 * @brief I2C从机拉时钟修正配置结构体
 *
 * 用于 i2c_config_stretch_fix()。
 * 从机在拉低SCL延长时钟结束时，可配置延迟释放SCL，修正时序裕量。
 */
typedef struct {
    bool     stretch_fix_en;  ///< 从机拉时钟修正使能（I2C_CR.STRETCH_FIX_EN）
    uint16_t dly_time;        ///< SCL延迟释放时间（APB时钟计数，[11:0]，I2C_CLK_DIV.DLY_TIME）
    bool     su_hd_fix_en;    ///< START/STOP的setup和hold时序修正使能（I2C_CR.SU_HD_FIX_EN）
} i2c_stretch_fix_config_t;

/**
 * @brief I2C中断回调上下文
 *
 * 中断发生时传递给用户回调函数，包含触发中断的实例和SR寄存器快照。
 * 用户可通过检查 flags 中的 I2C_FLAG_xxx 位掩码判断具体中断原因。
 *
 * @code
 * static void on_i2c_event(const i2c_irq_context_t *ctx)
 * {
 *     if (ctx->flags & I2C_FLAG_RXNE) {
 *         uint8_t data = i2c_read_dr(ctx->inst);
 *         // 处理接收数据
 *     }
 * }
 * @endcode
 */
typedef struct {
    i2c_instance_t inst;   ///< 触发中断的I2C实例
    uint32_t       flags;  ///< I2C_SR寄存器快照（I2C_FLAG_xxx位掩码）
} i2c_irq_context_t;

//===========================================
// 回调函数类型定义
//===========================================

/**
 * @brief I2C中断回调函数类型
 *
 * @param[in] ctx 中断上下文，包含实例标识和所有置位的状态标志
 *
 * @warning 回调函数在中断上下文中执行，应保持简短，禁止调用阻塞式传输API
 */
typedef void (*i2c_callback_t)(const i2c_irq_context_t *ctx);

//===========================================
// 第1层：快速初始化 API
//===========================================

/**
 * @brief 快速初始化I2C主模式
 *
 * 根据 pclk_hz 和 scl_hz 自动计算时钟分频值，配置主模式，
 * 默认使能 TX_AUTO_EN（方向自动切换）、HIGH_WAITEN（时钟高等待）和 MULTIMA_EN（多主机）。
 * 调用前须先使能外设时钟（clock_periph_enable(CLK_I2Cx)）。
 *
 * @param[in] inst     I2C实例（I2C_1 ~ I2C_2）
 * @param[in] pclk_hz  APB时钟频率（Hz），用于计算分频值
 * @param[in] scl_hz   目标SCL时钟频率（Hz），推荐值：100000/400000/1000000
 * @return true: 初始化成功, false: 参数无效（实例越界/分频值超范围）
 *
 * @note 分频公式：clk_div = Fpclk / (4×Fscl) - 1，要求结果 ≥ I2C_CLK_DIV_MIN(3)
 *
 * @code
 * clock_periph_enable(CLK_I2C1);
 * i2c_init_master(I2C_1, 120000000U, 400000U);  // 120MHz PCLK，400KHz SCL
 * @endcode
 */
bool i2c_init_master(i2c_instance_t inst, uint32_t pclk_hz, uint32_t scl_hz);

/**
 * @brief 快速初始化I2C从模式
 *
 * 配置从模式，设置从机地址1，默认使能 TX_AUTO_EN（方向自动切换）。
 * 调用前须先使能外设时钟（clock_periph_enable(CLK_I2Cx)）。
 *
 * @param[in] inst  I2C实例（I2C_1 ~ I2C_2）
 * @param[in] addr  从机地址1（7位，有效范围 0x08 ~ 0x77）
 * @return true: 初始化成功, false: 参数无效
 *
 *
 * @code
 * clock_periph_enable(CLK_I2C2);
 * i2c_init_slave(I2C_2, 0x50);
 * @endcode
 */
bool i2c_init_slave(i2c_instance_t inst, uint8_t addr);

//===========================================
// 第2层：基础配置 API
//===========================================

/**
 * @brief 主模式完整结构体配置
 *
 * 根据配置结构体对I2C主模式进行完整初始化，支持精细控制所有主模式参数。
 * 调用前须先使能外设时钟。
 *
 * @param[in] inst    I2C实例
 * @param[in] config  主模式配置结构体指针，不能为NULL
 * @return true: 配置成功, false: 参数无效（inst越界或config为NULL）
 *
 */
bool i2c_config_master(i2c_instance_t inst, const i2c_master_config_t *config);

/**
 * @brief 从模式完整结构体配置
 *
 * 根据配置结构体对I2C从模式进行完整初始化，支持配置3个从机地址。
 * 调用前须先使能外设时钟。
 *
 * @param[in] inst    I2C实例
 * @param[in] config  从模式配置结构体指针，不能为NULL
 * @return true: 配置成功, false: 参数无效
 *
 *       I2C_CR.MEN/TX_AUTO_EN/NOSTRETCH/TXE_SEL
 */
bool i2c_config_slave(i2c_instance_t inst, const i2c_slave_config_t *config);

/**
 * @brief 配置时钟分频值
 *
 * 直接写入CLK_DIV字段（不影响DLY_TIME字段）。
 * 优先使用 i2c_init_master() 自动计算；本函数适用于精细调整。
 *
 * @param[in] inst     I2C实例
 * @param[in] clk_div  时钟分频值（必须 ≥ I2C_CLK_DIV_MIN(3)，Fscl=Fpclk/(4×(clk_div+1))）
 *
 */
void i2c_config_clock_div(i2c_instance_t inst, uint16_t clk_div);

/**
 * @brief 配置SCL/SDA滤波
 *
 * 设置SCL毛刺滤波级数和SDA输入延时，用于抑制总线干扰噪声。
 *
 * @param[in] inst    I2C实例
 * @param[in] config  滤波配置结构体指针，不能为NULL
 *
 * @note sda_in_delay 建议与 scl_filter 设置相同，保持SDA/SCL输入相位一致
 */
void i2c_config_filter(i2c_instance_t inst, const i2c_filter_config_t *config);

/**
 * @brief 配置从机地址1
 *
 * 写入从机地址1（默认使能，无需额外使能控制位）。
 *
 * @param[in] inst  I2C实例
 * @param[in] addr  7位从机地址（有效范围：0x08 ~ 0x77）
 *
 */
void i2c_config_slave_addr1(i2c_instance_t inst, uint8_t addr);

/**
 * @brief 配置从机地址2（含使能控制）
 *
 * @param[in] inst    I2C实例
 * @param[in] addr    7位从机地址（有效范围：0x08 ~ 0x77）
 * @param[in] enable  true=使能地址2匹配，false=禁用
 *
 */
void i2c_config_slave_addr2(i2c_instance_t inst, uint8_t addr, bool enable);

/**
 * @brief 配置从机地址3（含使能控制）
 *
 * @param[in] inst    I2C实例
 * @param[in] addr    7位从机地址（有效范围：0x08 ~ 0x77）
 * @param[in] enable  true=使能地址3匹配，false=禁用
 *
 */
void i2c_config_slave_addr3(i2c_instance_t inst, uint8_t addr, bool enable);

/**
 * @brief 配置从机拉时钟修正
 *
 * 使能从机拉低SCL结束时的SCL延迟释放，以及START/STOP的时序修正。
 *
 * @param[in] inst    I2C实例
 * @param[in] config  拉时钟修正配置结构体指针，不能为NULL
 *
 *       I2C_CLK_DIV.DLY_TIME[23:12]
 */
void i2c_config_stretch_fix(i2c_instance_t inst, const i2c_stretch_fix_config_t *config);

/**
 * @brief 配置SMBus超时
 *
 * 配置TimeoutA（SCL低电平超时）和TimeoutB（累积时钟低电平延长超时）。
 * 仅在需要SMBus 2.0兼容时使用。
 *
 * @param[in] inst    I2C实例
 * @param[in] config  SMBus超时配置结构体指针，不能为NULL
 * @return true: 配置成功, false: 参数无效
 *
 *
 * @code
 * // PCLK=60MHz，配置TimeoutA=25ms
 * i2c_smbus_timeout_config_t cfg = {
 *     .timeouta       = 0x2DA,
 *     .timeouta_en    = true,
 *     .timeouta_irq_en = true,
 *     .timeoutb       = 0,
 *     .timeoutb_en    = false,
 *     .timeoutb_irq_en = false,
 *     .ext_mode       = false,
 * };
 * i2c_config_smbus_timeout(I2C_1, &cfg);
 * @endcode
 */
bool i2c_config_smbus_timeout(i2c_instance_t inst, const i2c_smbus_timeout_config_t *config);

//===========================================
// 第3层：高级功能 API
//===========================================

/**
 * @brief 配置DMA功能使能
 *
 * 使能后，TXE=1时产生发送DMA请求，RXNE=1时产生接收DMA请求。
 *
 * @param[in] inst    I2C实例
 * @param[in] enable  true=使能DMA，false=禁用DMA
 *
 * @warning DMA使能时，禁止同时使能TXE/RXNE中断，禁止软件清除TXE/RXNE状态
 * @warning DMA模式下从机发送，TXE_SEL必须置1（i2c_config_txe_sel(inst, true)）
 */
void i2c_config_dma(i2c_instance_t inst, bool enable);

/**
 * @brief 配置从模式发送TXE时机
 *
 * 控制从机地址匹配后TXE是否立即置高。
 *
 * @param[in] inst               I2C实例
 * @param[in] set_on_addr_match  true=地址匹配后TXE立即置高，false=不立即置高
 *
 * @note DMA从机发送模式必须设为true
 */
void i2c_config_txe_sel(i2c_instance_t inst, bool set_on_addr_match);

/**
 * @brief 配置从机时钟延长禁用
 *
 * 禁用后从机不拉低SCL等待，接收溢出时旧数据保留，发送溢出时上一字节不再重发。
 *
 * @param[in] inst     I2C实例
 * @param[in] disable  true=禁止时钟延长（NOSTRETCH=1），false=允许时钟延长
 *
 */
void i2c_config_nostretch(i2c_instance_t inst, bool disable);

/**
 * @brief 配置SDA数据线方向自动切换
 *
 * 使能后硬件根据地址字节RW位自动切换收发方向，无需软件手动配置TX位。
 *
 * @param[in] inst    I2C实例
 * @param[in] enable  true=使能自动切换，false=禁用（需手动调用i2c_set_tx_mode()）
 *
 */
void i2c_config_tx_auto_en(i2c_instance_t inst, bool enable);

/**
 * @brief 配置多主机使能
 *
 * 总线上存在多个主机时必须使能，以支持仲裁机制。
 *
 * @param[in] inst    I2C实例
 * @param[in] enable  true=使能多主机模式，false=禁用
 *
 */
void i2c_config_multi_master(i2c_instance_t inst, bool enable);

/**
 * @brief 配置主机时钟高等待使能
 *
 * 使能后，若内部时钟已升高但IO仍为低，主机等待IO变高后再采样。
 *
 * @param[in] inst    I2C实例
 * @param[in] enable  true=使能，false=禁用
 *
 */
void i2c_config_high_wait(i2c_instance_t inst, bool enable);

/**
 * @brief 配置主机仲裁丢失后的行为
 *
 * @param[in] inst         I2C实例
 * @param[in] auto_switch  true=仲裁丢失后自动切换为从机，false=不自动切换
 *
 * @note 自动切换为从机后，在本次传输期间无法应答自身地址，但在新的寻址周期中可以应答
 */
void i2c_config_marlo_sel(i2c_instance_t inst, bool auto_switch);

/**
 * @brief 配置SDA跳变检测阈值
 *
 * SCL低电平期间SDA上升沿计数达到阈值时置位DETR标志。
 * threshold=0 表示禁用检测。
 *
 * @param[in] inst       I2C实例
 * @param[in] threshold  跳变计数阈值（0=禁用，1~I2C_DET_CNT_MAX=检测阈值）
 *
 */
void i2c_config_sda_detect(i2c_instance_t inst, uint16_t threshold);

/**
 * @brief 阻塞式主机发送（轮询模式）
 *
 * 产生START，发送从机地址（写方向），逐字节发送数据，最后产生STOP。
 * 内部轮询MTF/TXE/NACKF等标志，超时后返回false并产生STOP释放总线。
 *
 * @param[in] inst        I2C实例
 * @param[in] slave_addr  从机7位地址（不含R/W位，范围0x08~0x77）
 * @param[in] data        待发送数据缓冲区，不能为NULL
 * @param[in] len         发送字节数（不能为0）
 * @param[in] timeout_ms  操作超时（毫秒，0使用默认值I2C_DEFAULT_TIMEOUT_MS）
 * @return true: 发送完成且全部ACK，false: NACK/总线错误/参数无效/超时
 *
 *       I2C_SR.MTF/TXE/NACKF/BUS_BUSY/RACK，I2C_DR
 *
 * @code
 * uint8_t data[] = {0x01, 0x02, 0x03};
 * bool ok = i2c_master_transmit(I2C_1, 0x50, data, sizeof(data), 100U);
 * @endcode
 */
bool i2c_master_transmit(i2c_instance_t inst, uint8_t slave_addr,
                         const uint8_t *data, size_t len, uint32_t timeout_ms);

/**
 * @brief 阻塞式主机接收（轮询模式）
 *
 * 产生START，发送从机地址（读方向），在接收最后一字节前设置TACK=NACK，
 * 完成后产生STOP。内部轮询相关标志，超时后返回false。
 *
 * @param[in]  inst        I2C实例
 * @param[in]  slave_addr  从机7位地址（不含R/W位，范围0x08~0x77）
 * @param[out] data        接收数据缓冲区，不能为NULL
 * @param[in]  len         接收字节数（不能为0）
 * @param[in]  timeout_ms  操作超时（毫秒，0使用默认值I2C_DEFAULT_TIMEOUT_MS）
 * @return true: 接收成功，false: NACK/总线错误/参数无效/超时
 *
 *       I2C_SR.MTF/RXNE/TX_RX_FLAG/BUS_BUSY/RACK，I2C_DR
 *
 * @code
 * uint8_t buf[4];
 * bool ok = i2c_master_receive(I2C_1, 0x50, buf, sizeof(buf), 100U);
 * @endcode
 */
bool i2c_master_receive(i2c_instance_t inst, uint8_t slave_addr,
                        uint8_t *data, size_t len, uint32_t timeout_ms);

//===========================================
// 第4层：控制与查询 API
//===========================================

/* ── 使能控制 ── */

/**
 * @brief 使能I2C模块
 *
 * @param[in] inst  I2C实例
 *
 */
void i2c_enable(i2c_instance_t inst);

/**
 * @brief 禁用I2C模块
 *
 * @param[in] inst  I2C实例
 *
 */
void i2c_disable(i2c_instance_t inst);

/**
 * @brief 查询I2C模块是否已使能
 *
 * @param[in] inst  I2C实例
 * @return true: 已使能，false: 未使能
 *
 */
bool i2c_is_enabled(i2c_instance_t inst);

/* ── 总线操作 ── */

/**
 * @brief 产生START（或重复START）条件
 *
 * 置位 I2C_CR.START。总线空闲时产生起始条件；主设备传输中调用则在
 * 当前字节传输完成后产生重复起始条件。
 *
 * @param[in] inst  I2C实例
 *
 * @note START产生后 I2C_SR.STARTF=1，需向DR写入地址字节才会清STARTF并开始发送
 */
void i2c_start(i2c_instance_t inst);

/**
 * @brief 产生STOP条件
 *
 * 置位 I2C_CR.STOP，在当前字节传输完成后产生停止条件，硬件自动清0。
 *
 * @param[in] inst  I2C实例
 *
 */
void i2c_stop(i2c_instance_t inst);

/**
 * @brief 设置应答控制位（TACK）
 *
 * 控制下一个接收字节完成后产生ACK还是NACK。
 * 接收最后一字节前（倒数第二个RXNE事件之后）必须设置为 I2C_NACK。
 *
 * @param[in] inst  I2C实例
 * @param[in] ack   I2C_ACK=产生ACK，I2C_NACK=产生NACK
 *
 * @warning TACK必须在应答周期前写入，否则无效
 */
void i2c_set_tack(i2c_instance_t inst, i2c_ack_t ack);

/**
 * @brief 手动设置发送/接收模式（TX位）
 *
 * 手动控制SDA数据线方向。TX_AUTO_EN使能后，此函数设置无效。
 *
 * @param[in] inst  I2C实例
 * @param[in] tx    true=发送模式，false=接收模式
 *
 */
void i2c_set_tx_mode(i2c_instance_t inst, bool tx);

/**
 * @brief 设置主/从模式
 *
 * @param[in] inst    I2C实例
 * @param[in] master  true=主模式，false=从模式
 *
 */
void i2c_set_master_mode(i2c_instance_t inst, bool master);

/* ── 数据读写 ── */

/**
 * @brief 向数据寄存器写入字节
 *
 * 写入 I2C_DR，在主模式下触发发送时序，同时清除TXE和STARTF标志。
 *
 * @param[in] inst  I2C实例
 * @param[in] data  待写入字节
 *
 */
void i2c_write_dr(i2c_instance_t inst, uint8_t data);

/**
 * @brief 从数据寄存器读取字节
 *
 * 读取 I2C_DR，同时清除RXNE标志，触发主机继续产生接收时序。
 *
 * @param[in] inst  I2C实例
 * @return 接收到的字节
 *
 */
uint8_t i2c_read_dr(i2c_instance_t inst);

/* ── 状态查询 ── */

/**
 * @brief 查询指定状态标志是否置位
 *
 * @param[in] inst  I2C实例
 * @param[in] flag  待查询的标志（I2C_FLAG_xxx）
 * @return true: 标志已置位，false: 标志未置位
 *
 */
bool i2c_flag_is_set(i2c_instance_t inst, i2c_flag_t flag);

/**
 * @brief 清除指定状态标志（写1清0）
 *
 * 向SR寄存器对应位写1以清零。只读标志（RACK/SRW/BUS_BUSY）和
 * 由DR操作清除的标志（TXE/RXNE）无效。
 *
 * @param[in] inst  I2C实例
 * @param[in] flag  待清除的标志（I2C_FLAG_xxx）
 *
 */
void i2c_flag_clear(i2c_instance_t inst, i2c_flag_t flag);

/**
 * @brief 获取SR寄存器原始值
 *
 * @param[in] inst  I2C实例
 * @return I2C_SR寄存器原始值（18位有效）
 *
 */
uint32_t i2c_get_sr(i2c_instance_t inst);

/**
 * @brief 轮询等待总线空闲
 *
 * 持续检测BUS_BUSY标志，直到总线释放或超时。
 *
 * @param[in] inst        I2C实例
 * @param[in] timeout_ms  超时时间（毫秒）
 * @return true: 总线已空闲，false: 超时
 *
 */
bool i2c_wait_bus_free(i2c_instance_t inst, uint32_t timeout_ms);

/* ── 中断管理 ── */

/**
 * @brief 使能一个或多个I2C中断
 *
 * @param[in] inst       I2C实例
 * @param[in] irq_types  待使能的中断类型（I2C_IRQ_xxx，可OR组合）
 *
 * @note 使能DMA（I2C_CR.DMA_EN=1）后，禁止使能I2C_IRQ_TXE和I2C_IRQ_RXNE
 */
void i2c_irq_enable(i2c_instance_t inst, i2c_irq_type_t irq_types);

/**
 * @brief 禁用一个或多个I2C中断
 *
 * @param[in] inst       I2C实例
 * @param[in] irq_types  待禁用的中断类型（I2C_IRQ_xxx，可OR组合）
 *
 */
void i2c_irq_disable(i2c_instance_t inst, i2c_irq_type_t irq_types);

/**
 * @brief 注册I2C中断回调
 *
 * 注册在 I2Cx_IRQHandler 中调用的用户回调函数。重复注册覆盖旧回调。
 *
 * @param[in] inst      I2C实例
 * @param[in] callback  回调函数指针（NULL等同于调用 i2c_irq_unregister()）
 *
 * @note 回调在中断上下文执行，保持简短，禁止调用阻塞式传输API
 *
 * @code
 * static void on_i2c1_event(const i2c_irq_context_t *ctx)
 * {
 *     if (ctx->flags & I2C_FLAG_RXNE) {
 *         uint8_t b = i2c_read_dr(ctx->inst);
 *         (void)b;
 *     }
 *     if (ctx->flags & I2C_FLAG_STOPF) {
 *         i2c_flag_clear(ctx->inst, I2C_FLAG_STOPF);
 *     }
 * }
 * i2c_irq_register(I2C_1, on_i2c1_event);
 * i2c_irq_enable(I2C_1, I2C_IRQ_RXNE | I2C_IRQ_STOPF);
 * NVIC_EnableIRQ(I2C1_IRQn);
 * @endcode
 */
void i2c_irq_register(i2c_instance_t inst, i2c_callback_t callback);

/**
 * @brief 注销I2C中断回调
 *
 * 清除已注册的回调，后续中断不再调用用户函数（不影响中断使能状态）。
 *
 * @param[in] inst  I2C实例
 */
void i2c_irq_unregister(i2c_instance_t inst);

/* ── 寄存器读取与综合操作 ── */

/**
 * @brief 获取时钟分频寄存器原始值
 *
 * @param[in] inst  I2C实例
 * @return I2C_CLK_DIV寄存器原始值（含CLK_DIV和DLY_TIME字段）
 *
 */
uint32_t i2c_get_clk_div(i2c_instance_t inst);

/**
 * @brief 获取当前配置的从机地址1
 *
 * @param[in] inst  I2C实例
 * @return 从机地址1（7位，实际地址值）
 *
 */
uint8_t i2c_get_slave_addr1(i2c_instance_t inst);

/**
 * @brief 获取当前配置的从机地址2
 *
 * @param[in] inst  I2C实例
 * @return 从机地址2（7位，实际地址值）
 *
 */
uint8_t i2c_get_slave_addr2(i2c_instance_t inst);

/**
 * @brief 获取当前配置的从机地址3
 *
 * @param[in] inst  I2C实例
 * @return 从机地址3（7位，实际地址值）
 *
 */
uint8_t i2c_get_slave_addr3(i2c_instance_t inst);

/**
 * @brief 获取控制寄存器原始值
 *
 * @param[in] inst  I2C实例
 * @return I2C_CR寄存器原始值
 *
 */
uint32_t i2c_get_cr(i2c_instance_t inst);

/**
 * @brief 查询SMBus超时标志是否置位
 *
 * @param[in] inst       I2C实例
 * @param[in] timeout_a  true=查询TIMEOUTAF，false=查询TIMEOUTBF
 * @return true: 对应超时标志已置位
 *
 */
bool i2c_smbus_timeout_is_set(i2c_instance_t inst, bool timeout_a);

/**
 * @brief 清除SMBus超时标志（写1清0）
 *
 * @param[in] inst       I2C实例
 * @param[in] timeout_a  true=清除TIMEOUTAF，false=清除TIMEOUTBF
 *
 */
void i2c_smbus_timeout_clear(i2c_instance_t inst, bool timeout_a);

/**
 * @brief 阻塞式主机先写后读（带重复START）
 *
 * 先产生START，发送从机地址（写方向）和 write_data 数据；
 * 再产生重复START，发送从机地址（读方向），接收 read_data 数据，最后产生STOP。
 * 常用于读取从设备内部寄存器（先发寄存器地址，再读取寄存器值）。
 *
 * @param[in]  inst        I2C实例
 * @param[in]  slave_addr  从机7位地址（不含R/W位）
 * @param[in]  write_data  写入数据（通常为寄存器地址），不能为NULL
 * @param[in]  write_len   写入字节数（不能为0）
 * @param[out] read_data   接收缓冲区，不能为NULL
 * @param[in]  read_len    接收字节数（不能为0）
 * @param[in]  timeout_ms  操作超时（毫秒，0使用默认值I2C_DEFAULT_TIMEOUT_MS）
 * @return true: 操作成功，false: NACK/总线错误/参数无效/超时
 *
 *
 * @code
 * uint8_t reg = 0x10;
 * uint8_t val[2];
 * bool ok = i2c_master_write_read(I2C_1, 0x50, &reg, 1, val, 2, 100U);
 * @endcode
 */
bool i2c_master_write_read(i2c_instance_t inst, uint8_t slave_addr,
                           const uint8_t *write_data, size_t write_len,
                           uint8_t *read_data, size_t read_len,
                           uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // _HARDWARE_I2C_H
