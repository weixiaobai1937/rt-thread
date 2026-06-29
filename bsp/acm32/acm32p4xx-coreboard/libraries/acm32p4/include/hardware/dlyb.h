/**
 * @file dlyb.h
 * @brief ACM32P4 延迟模块（DLYB）驱动接口
 *
 * @details
 * 本驱动支持 ACM32P4 芯片的 DLYB（Delay Block）延迟模块。
 * DLYB 对输入时钟进行相位偏移编程，输出时钟用于 SDMMC、OSPI、ETH
 * 等外设接收数据的采样计时。
 *
 * @warning 硬件 BUG（芯片勘误 2.4.1）：使能 0 个延时单元时延迟约 500ps；
 * 使能 1 个及以上延时单元时初始延迟约 9ns，其他延时单元不具备实用价值。
 * 建议使用 SEL=0（透传模式，无额外延迟单元）作为主要工作模式。
 *
 * 特性：
 * - ✅ 4 层 API 架构（快速初始化 → 基础配置 → 高级校准 → 控制查询）
 * - ✅ 支持 5 个独立实例（ETH、SDMMC_DLYBS、SDMMC_DLYBD、OSPI1、OSPI2）
 * - ✅ 类型安全（枚举类型，无魔法数字）
 * - ✅ 自动延迟线校准（延迟步长与一个输入时钟周期对齐）
 * - ✅ 输出时钟相位精确选择
 *
 * 支持的实例：
 * - DLYB_INST_ETH   : 以太网 MAC 接口 (0x52006C00)
 * - DLYB_INST_SDMMCS : SDMMC 单数据速率 (0x520CA000)
 * - DLYB_INST_SDMMCD : SDMMC 双数据速率 (0x520CA800)
 * - DLYB_INST_OSPI1  : OSPI1 接口       (0x520D2000)
 * - DLYB_INST_OSPI2  : OSPI2 接口       (0x520D2400)
 *
 * 典型使用（快速初始化 + 自动校准）：
 * @code
 * dlyb_calib_t calib;
 * dlyb_init_with_calib(DLYB_INST_SDMMCS, &calib);
 * if (calib.valid) {
 *     // 校准成功，选择第 calib.len_count / 2 个相位（居中采样）
 *     dlyb_config_phase(DLYB_INST_SDMMCS, calib.len_count / 2);
 * }
 * @endcode
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 * @date 2026-03-24
 */

#ifndef _HARDWARE_DLYB_H
#define _HARDWARE_DLYB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

//===========================================
// 1. 宏定义
//===========================================

/** @brief UNIT 字段最大值（6 位，0~63） */
#define DLYB_UNIT_MAX        63U

/** @brief SEL 字段最大值（4 位，0~12） */
#define DLYB_SEL_MAX         12U

/** @brief 等待 LENF 置位的最大轮询次数 */
#define DLYB_LENF_TIMEOUT_COUNT  10000U

//===========================================
// 2. 枚举类型定义
//===========================================

/**
 * @brief DLYB 实例选择
 *
 * 对应芯片内 5 个独立的 DLYB 寄存器块，分别服务于不同外设。
 */
typedef enum {
    DLYB_INST_ETH   = 0,  ///< 以太网 MAC DLYB（基地址 0x52006C00）
    DLYB_INST_SDMMCS = 1,  ///< SDMMC 单速率 DLYB（基地址 0x520CA000）
    DLYB_INST_SDMMCD = 2,  ///< SDMMC 双速率 DLYB（基地址 0x520CA800）
    DLYB_INST_OSPI1 = 3,  ///< OSPI1 DLYB（基地址 0x520D2000）
    DLYB_INST_OSPI2 = 4,  ///< OSPI2 DLYB（基地址 0x520D2400）
    DLYB_INST_COUNT = 5
} dlyb_inst_t;

//===========================================
// 3. 结构体类型定义
//===========================================

/**
 * @brief DLYB 基础配置结构体
 *
 * 用于 `dlyb_configure()` 函数。
 * @note 函数内部自动管理 SEN 状态，调用方无需手动操作采样器。
 */
typedef struct {
    uint8_t unit;  ///< 单位延迟步数（0~63），写入 DLYB_CFGR.UNIT
    uint8_t sel;   ///< 输出时钟相位选择（0~12），写入 DLYB_CFGR.SEL
} dlyb_config_t;

/**
 * @brief DLYB 校准结果结构体
 *
 * 由 `dlyb_calibrate()` 和 `dlyb_init_with_calib()` 填写。
 */
typedef struct {
    uint8_t  unit;       ///< 找到的最优单位延迟值（DLYB_CFGR.UNIT 的设置值）
    uint8_t  len_count;  ///< 覆盖一个完整输入时钟周期所需的单位延迟数量（N）
    bool     valid;      ///< 校准是否成功
} dlyb_calib_t;

/**
 * @brief DLYB 状态结构体
 *
 * 由 `dlyb_get_status()` 填写，反映当前 DLYB 寄存器的全部字段。
 */
typedef struct {
    bool     den;    ///< 延迟模块使能位（DLYB_CR.DEN）
    bool     sen;    ///< 采样器使能位（DLYB_CR.SEN，=1 时输出时钟被禁止）
    bool     lenf;   ///< 延迟线长度有效标志（DLYB_CFGR.LENF）
    uint16_t len;    ///< 延迟线长度值（DLYB_CFGR.LEN[11:0]，仅 LENF=1 时有效）
    uint8_t  unit;   ///< 当前单位延迟值（DLYB_CFGR.UNIT）
    uint8_t  sel;    ///< 当前输出时钟相位选择（DLYB_CFGR.SEL）
} dlyb_status_t;

//===========================================
// 4. 第 1 层 API - 快速初始化
//===========================================

/**
 * @brief 快速初始化 DLYB 模块（仅使能，不校准）
 *
 * 设置 DEN=1 使能延迟模块；SEN 保持 0（输出时钟直通）。
 * 适用于不需要延迟的透传场景或后续手动配置。
 *
 * @param inst DLYB 实例
 *
 * @note 父外设时钟须在调用前由调用方使能（如 CLK_ETH_MAC 等）
 * @note 覆盖寄存器：DLYB_CR.DEN
 *
 * @code
 * dlyb_init(DLYB_INST_SDMMCS);
 * @endcode
 */
void dlyb_init(dlyb_inst_t inst);

/**
 * @brief 快速初始化并自动校准延迟线长度
 *
 * 等效于先调用 `dlyb_enable()`，再调用 `dlyb_calibrate()`，
 * 校准完成后禁止采样器（SEN=0）以恢复输出时钟。
 *
 * @param inst  DLYB 实例
 * @param calib 输出参数，校准结果（可为 NULL，此时忽略结果）
 *
 * @note 父外设时钟须在调用前由调用方使能
 * @note 覆盖寄存器：DLYB_CR.DEN/SEN, DLYB_CFGR.SEL/UNIT
 * @warning 硬件 BUG（芯片勘误 2.4.1）：使能 1+ 延时单元时初始延迟约 9ns，
 * 校准结果可能不实用。建议优先使用 SEL=0 透传模式。
 *
 * @code
 * dlyb_calib_t calib;
 * dlyb_init_with_calib(DLYB_INST_SDMMCS, &calib);
 * if (calib.valid) {
 *     dlyb_config_phase(DLYB_INST_SDMMCS, calib.len_count / 2);
 * }
 * @endcode
 */
void dlyb_init_with_calib(dlyb_inst_t inst, dlyb_calib_t *calib);

//===========================================
// 5. 第 2 层 API - 基础配置
//===========================================

/**
 * @brief 配置 DLYB 的 UNIT 和 SEL
 *
 * 自动置位 SEN=1（禁止输出时钟），写入 UNIT 和 SEL，
 * 等待 LENF=1 后清除 SEN（恢复输出时钟）。
 *
 * @param inst   DLYB 实例
 * @param config 配置参数（UNIT 和 SEL）
 *
 * @return bool 配置是否成功（等待 LENF 是否超时）
 *
 * @note 覆盖寄存器：DLYB_CR.SEN, DLYB_CFGR.UNIT/SEL
 *
 * @code
 * dlyb_config_t cfg = { .unit = 5, .sel = 3 };
 * dlyb_configure(DLYB_INST_SDMMCS, &cfg);
 * @endcode
 */
bool dlyb_configure(dlyb_inst_t inst, const dlyb_config_t *config);

/**
 * @brief 仅配置输出时钟相位（SEL）
 *
 * 在保持当前 UNIT 不变的情况下，仅修改 SEL 字段以选择新的输出时钟相位。
 * 自动置位 SEN=1，写入 SEL，随后清除 SEN 恢复输出时钟。
 *
 * @param inst DLYB 实例
 * @param sel  输出时钟相位选择（0~12）
 *
 * @note 覆盖寄存器：DLYB_CR.SEN, DLYB_CFGR.SEL
 *
 * @code
 * dlyb_config_phase(DLYB_INST_SDMMCS, 4);
 * @endcode
 */
void dlyb_config_phase(dlyb_inst_t inst, uint8_t sel);

//===========================================
// 6. 第 3 层 API - 高级校准
//===========================================

/**
 * @brief 自动校准延迟线长度（UNIT 自动搜索）
 *
 * 严格按照手册描述的步骤执行延迟线长度校准：
 *  1. 设置 DEN=1，SEN=1，SEL=12（使能所有延迟单元）
 *  2. 从 UNIT=0 循环到 63，每次更新 UNIT 后等待 LENF=1，
 *     读取 LEN；若 LEN[10:0]>0 且 LEN[11]=0 或 LEN[10]=0，则退出
 *  3. 从 N=10 向下找到 LEN[N]=1 的最高位，得到 len_count=N
 *  4. 清零 SEN，使能输出时钟
 *
 * @param inst  DLYB 实例（须已调用 dlyb_enable()）
 * @param calib 校准结果输出参数（不能为 NULL）
 *
 * @note 覆盖寄存器：DLYB_CR.DEN/SEN, DLYB_CFGR.SEL/UNIT, DLYB_CFGR.LEN/LENF(读)
 * @note 若校准失败（calib->valid=false），建议用 UNIT=0、SEL=0 作为回退值
 * @warning 硬件 BUG（芯片勘误 2.4.1）：使能 1+ 延时单元时初始延迟约 9ns，
 * 校准结果可能不实用。建议优先使用 SEL=0 透传模式。
 *
 * @code
 * dlyb_calib_t calib;
 * dlyb_enable(DLYB_INST_SDMMCS);
 * dlyb_calibrate(DLYB_INST_SDMMCS, &calib);
 * @endcode
 */
void dlyb_calibrate(dlyb_inst_t inst, dlyb_calib_t *calib);

//===========================================
// 7. 第 4 层 API - 控制与查询
//===========================================

/**
 * @brief 使能 DLYB 模块（DEN=1）
 *
 * @param inst DLYB 实例
 *
 * @note 覆盖寄存器：DLYB_CR.DEN
 *
 * @code
 * dlyb_enable(DLYB_INST_ETH);
 * @endcode
 */
void dlyb_enable(dlyb_inst_t inst);

/**
 * @brief 禁止 DLYB 模块（DEN=0）
 *
 * 禁止后输出时钟=输入时钟（直通）。
 *
 * @param inst DLYB 实例
 *
 * @note 覆盖寄存器：DLYB_CR.DEN
 *
 * @code
 * dlyb_disable(DLYB_INST_ETH);
 * @endcode
 */
void dlyb_disable(dlyb_inst_t inst);

/**
 * @brief 使能长度采样器，同时禁止输出时钟（SEN=1）
 *
 * SEN=1 时输出时钟被禁止，允许修改 UNIT 和 SEL。
 *
 * @param inst DLYB 实例
 *
 * @note 覆盖寄存器：DLYB_CR.SEN
 *
 * @code
 * dlyb_sampler_enable(DLYB_INST_SDMMCS);
 * @endcode
 */
void dlyb_sampler_enable(dlyb_inst_t inst);

/**
 * @brief 禁止采样器，同时使能输出时钟（SEN=0）
 *
 * SEN=0 时输出时钟恢复，UNIT 和 SEL 不可修改。
 *
 * @param inst DLYB 实例
 *
 * @note 覆盖寄存器：DLYB_CR.SEN
 *
 * @code
 * dlyb_sampler_disable(DLYB_INST_SDMMCS);
 * @endcode
 */
void dlyb_sampler_disable(dlyb_inst_t inst);

/**
 * @brief 设置单位延迟步数（UNIT 字段）
 *
 * @param inst DLYB 实例
 * @param unit 单位延迟值（0~63），超出范围自动截断至 DLYB_UNIT_MAX
 *
 * @note 覆盖寄存器：DLYB_CFGR.UNIT
 * @note 须在 SEN=1 时调用，否则写入无效
 *
 * @code
 * dlyb_sampler_enable(DLYB_INST_SDMMCS);
 * dlyb_set_unit(DLYB_INST_SDMMCS, 8);
 * @endcode
 */
void dlyb_set_unit(dlyb_inst_t inst, uint8_t unit);

/**
 * @brief 设置输出时钟相位选择（SEL 字段）
 *
 * @param inst DLYB 实例
 * @param sel  相位选择值（0~12），超出范围自动截断至 DLYB_SEL_MAX
 *
 * @note 覆盖寄存器：DLYB_CFGR.SEL
 * @note 须在 SEN=1 时调用，否则写入无效
 *
 * @code
 * dlyb_sampler_enable(DLYB_INST_SDMMCS);
 * dlyb_set_sel(DLYB_INST_SDMMCS, 4);
 * dlyb_sampler_disable(DLYB_INST_SDMMCS);
 * @endcode
 */
void dlyb_set_sel(dlyb_inst_t inst, uint8_t sel);

/**
 * @brief 等待 LENF=1（延迟线长度有效）
 *
 * 轮询 DLYB_CFGR.LENF 直到置位或超时。
 *
 * @param inst DLYB 实例
 *
 * @return bool true=LENF 已置位，false=超时
 *
 * @note 覆盖寄存器：DLYB_CFGR.LENF(读)
 *
 * @code
 * dlyb_set_unit(DLYB_INST_SDMMCS, 10);
 * bool ok = dlyb_wait_lenf(DLYB_INST_SDMMCS);
 * @endcode
 */
bool dlyb_wait_lenf(dlyb_inst_t inst);

/**
 * @brief 查询 LENF 标志（无等待）
 *
 * @param inst DLYB 实例
 *
 * @return bool true=延迟线长度有效，false=无效
 *
 * @note 覆盖寄存器：DLYB_CFGR.LENF(读)
 */
bool dlyb_is_len_valid(dlyb_inst_t inst);

/**
 * @brief 读取延迟线长度值（LEN 字段）
 *
 * @param inst DLYB 实例
 *
 * @return uint16_t 延迟线长度（12 位，仅 LENF=1 时有意义）
 *
 * @note 覆盖寄存器：DLYB_CFGR.LEN(读)
 *
 * @code
 * uint16_t len = dlyb_get_len(DLYB_INST_SDMMCS);
 * @endcode
 */
uint16_t dlyb_get_len(dlyb_inst_t inst);

/**
 * @brief 查询模块使能状态（DEN）
 *
 * @param inst DLYB 实例
 *
 * @return bool true=已使能
 *
 * @note 覆盖寄存器：DLYB_CR.DEN(读)
 */
bool dlyb_is_enabled(dlyb_inst_t inst);

/**
 * @brief 查询采样器使能状态（SEN）
 *
 * @param inst DLYB 实例
 *
 * @return bool true=采样器已使能（输出时钟被禁止）
 *
 * @note 覆盖寄存器：DLYB_CR.SEN(读)
 */
bool dlyb_is_sampler_enabled(dlyb_inst_t inst);

/**
 * @brief 获取 DLYB 完整状态
 *
 * 读取 DLYB_CR 和 DLYB_CFGR 的全部字段并填写 status 结构体。
 *
 * @param inst   DLYB 实例
 * @param status 输出参数，不能为 NULL
 *
 * @note 覆盖寄存器：DLYB_CR(读), DLYB_CFGR(读)
 *
 * @code
 * dlyb_status_t st;
 * dlyb_get_status(DLYB_INST_SDMMCS, &st);
 * printf("DEN=%d SEN=%d LENF=%d LEN=%d UNIT=%d SEL=%d\n",
 *        st.den, st.sen, st.lenf, st.len, st.unit, st.sel);
 * @endcode
 */
void dlyb_get_status(dlyb_inst_t inst, dlyb_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* _HARDWARE_DLYB_H */
