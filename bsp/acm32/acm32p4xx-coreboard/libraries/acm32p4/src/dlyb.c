/**
 * @file dlyb.c
 * @brief ACM32P4 延迟模块（DLYB）驱动实现
 *
 * @details
 * 本文件实现了 ACM32P4 芯片的 DLYB（Delay Block）延迟模块驱动。
 *
 * 特性：
 *   DLYB_CFGR.UNIT、DLYB_CFGR.LEN、DLYB_CFGR.LENF）
 * - 4 层 API 架构
 * - 严格按手册描述的步骤执行延迟线长度校准
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 * @date 2026-03-24
 */

#include <assert.h>
#include "device/acm32p4xx.h"
#include "hardware/clocks.h"
#include "hardware/dlyb.h"

//===========================================
// 1. 寄存器位定义
//===========================================

/* DLYB_CR 寄存器位定义 */
#define DLYB_CR_DEN_Pos     (0U)
#define DLYB_CR_DEN_Msk     (0x1UL << DLYB_CR_DEN_Pos)
#define DLYB_CR_SEN_Pos     (1U)
#define DLYB_CR_SEN_Msk     (0x1UL << DLYB_CR_SEN_Pos)

/* DLYB_CFGR 寄存器位定义 */
#define DLYB_CFGR_SEL_Pos   (0U)
#define DLYB_CFGR_SEL_Msk   (0xFUL  << DLYB_CFGR_SEL_Pos)
#define DLYB_CFGR_UNIT_Pos  (8U)
#define DLYB_CFGR_UNIT_Msk  (0x3FUL << DLYB_CFGR_UNIT_Pos)
#define DLYB_CFGR_LEN_Pos   (16U)
#define DLYB_CFGR_LEN_Msk   (0xFFFUL << DLYB_CFGR_LEN_Pos)
#define DLYB_CFGR_LENF_Pos  (31U)
#define DLYB_CFGR_LENF_Msk  (0x1UL  << DLYB_CFGR_LENF_Pos)

//===========================================
// 2. 内部辅助函数
//===========================================

/*
 * 使能 DLYB 所属外设的时钟
 *
 * 每个 DLYB 实例挂载在对应外设的时钟域下，访问寄存器前须先使能该时钟。
 * - ETH_DLYB   → CLK_ETH_MAC
 * - SDMMC_DLYBS → CLK_SDMMC
 * - SDMMC_DLYBD → CLK_SDMMC
 * - OSPI1_DLYB → CLK_OSPI1
 * - OSPI2_DLYB → CLK_OSPI2
 */
static void dlyb_clock_enable(dlyb_inst_t inst)
{
    static const clock_periph_t clocks[] = {
        CLK_ETH_MAC, /* DLYB_INST_ETH   */
        CLK_SDMMC,   /* DLYB_INST_SDMMCS */
        CLK_SDMMC,   /* DLYB_INST_SDMMCD */
        CLK_OSPI1,   /* DLYB_INST_OSPI1 */
        CLK_OSPI2,   /* DLYB_INST_OSPI2 */
    };
    assert((uint32_t)inst < (sizeof(clocks) / sizeof(clocks[0])));
    clock_periph_enable(clocks[(uint32_t)inst]);
}

/*
 * 根据实例编号获取 DLYB 寄存器指针
 */
static DLYB_TypeDef *dlyb_get_reg(dlyb_inst_t inst)
{
    static DLYB_TypeDef * const dlyb_regs[] = {
        ETH_DLYB,    /* DLYB_INST_ETH   */
        SDMMC_DLYBS,  /* DLYB_INST_SDMMCS */
        SDMMC_DLYBD,  /* DLYB_INST_SDMMCD */
        OSPI1_DLYB,  /* DLYB_INST_OSPI1 */
        OSPI2_DLYB,  /* DLYB_INST_OSPI2 */
    };
    assert((uint32_t)inst < (sizeof(dlyb_regs) / sizeof(dlyb_regs[0])));
    return dlyb_regs[(uint32_t)inst];
}

//===========================================
// 第1层：快速初始化 API
//===========================================

void dlyb_init(dlyb_inst_t inst)
{
    dlyb_clock_enable(inst);
    DLYB_TypeDef *reg = dlyb_get_reg(inst);
    /* SEN 清零（以防前一状态残留），DEN 置位使能模块 */
    reg->CR = DLYB_CR_DEN_Msk;
}

void dlyb_init_with_calib(dlyb_inst_t inst, dlyb_calib_t *calib)
{
    dlyb_clock_enable(inst);
    DLYB_TypeDef *reg = dlyb_get_reg(inst);
    /* 先使能模块 */
    reg->CR = DLYB_CR_DEN_Msk;
    /* 执行校准 */
    dlyb_calibrate(inst, calib);
}

//===========================================
// 第2层：基础配置 API
//===========================================

bool dlyb_configure(dlyb_inst_t inst, const dlyb_config_t *config)
{
    assert(config != NULL);

    DLYB_TypeDef *reg = dlyb_get_reg(inst);
    uint8_t unit = (config->unit > DLYB_UNIT_MAX) ? DLYB_UNIT_MAX : config->unit;
    uint8_t sel  = (config->sel  > DLYB_SEL_MAX)  ? DLYB_SEL_MAX  : config->sel;

    /* 置位 SEN：禁止输出时钟，允许修改 UNIT 和 SEL */
    reg->CR_f.SEN = 1U;

    /* 写入 UNIT 和 SEL */
    uint32_t cfgr = reg->CFGR;
    cfgr &= ~(DLYB_CFGR_UNIT_Msk | DLYB_CFGR_SEL_Msk);
    cfgr |=  ((uint32_t)unit << DLYB_CFGR_UNIT_Pos) & DLYB_CFGR_UNIT_Msk;
    cfgr |=  ((uint32_t)sel  << DLYB_CFGR_SEL_Pos)  & DLYB_CFGR_SEL_Msk;
    reg->CFGR = cfgr;

    /* 等待 LENF=1 确认配置生效 */
    bool ok = dlyb_wait_lenf(inst);

    /* 清除 SEN：恢复输出时钟 */
    reg->CR_f.SEN = 0U;

    return ok;
}

void dlyb_config_phase(dlyb_inst_t inst, uint8_t sel)
{
    if (sel > DLYB_SEL_MAX) {
        sel = DLYB_SEL_MAX;
    }

    DLYB_TypeDef *reg = dlyb_get_reg(inst);

    /* 置位 SEN：禁止输出时钟 */
    reg->CR_f.SEN = 1U;

    /* 仅更新 SEL 字段 */
    uint32_t cfgr = reg->CFGR;
    cfgr &= ~DLYB_CFGR_SEL_Msk;
    cfgr |=  ((uint32_t)sel << DLYB_CFGR_SEL_Pos) & DLYB_CFGR_SEL_Msk;
    reg->CFGR = cfgr;

    /* 清除 SEN：恢复输出时钟 */
    reg->CR_f.SEN = 0U;
}

//===========================================
// 第3层：高级校准 API
//===========================================

void dlyb_calibrate(dlyb_inst_t inst, dlyb_calib_t *calib)
{
    assert(calib != NULL);

    DLYB_TypeDef *reg = dlyb_get_reg(inst);
    uint32_t unit;
    uint32_t cfgr;
    uint32_t len_val;
    int32_t  n;
    bool     found = false;

    /* 初始化输出 */
    calib->unit      = 0U;
    calib->len_count = 0U;
    calib->valid     = false;

    /* 步骤 1：DEN=1，SEN=1，SEL=12（使能全部延迟单元） */
    reg->CR = DLYB_CR_DEN_Msk | DLYB_CR_SEN_Msk;

    cfgr  = reg->CFGR;
    cfgr &= ~(DLYB_CFGR_SEL_Msk | DLYB_CFGR_UNIT_Msk);
    cfgr |=  ((uint32_t)DLYB_SEL_MAX << DLYB_CFGR_SEL_Pos) & DLYB_CFGR_SEL_Msk;
    reg->CFGR = cfgr;

    /* 步骤 2：循环 UNIT 从 0 到 63
     * @note 手册建议 0~15，本驱动扩展至 0~63 以覆盖更大延迟范围 */
    for (unit = 0U; unit <= DLYB_UNIT_MAX; unit++) {
        /* 更新 UNIT 值 */
        cfgr  = reg->CFGR;
        cfgr &= ~DLYB_CFGR_UNIT_Msk;
        cfgr |=  (unit << DLYB_CFGR_UNIT_Pos) & DLYB_CFGR_UNIT_Msk;
        reg->CFGR = cfgr;

        /* 等待 LENF=1 */
        if (!dlyb_wait_lenf(inst)) {
            /* LENF 超时，本 UNIT 跳过 */
            continue;
        }

        /* 读取 LEN */
        len_val = (reg->CFGR & DLYB_CFGR_LEN_Msk) >> DLYB_CFGR_LEN_Pos;

        /*
         * 退出条件：LEN[10:0] > 0 且 (LEN[11]=0 或 LEN[10]=0)
         * 即延迟线已覆盖一个完整时钟周期，且未发生高位溢出
         */
        if (((len_val & 0x7FFU) > 0U) &&
            (((len_val & (1U << 11)) == 0U) || ((len_val & (1U << 10)) == 0U))) {
            found = true;
            calib->unit = (uint8_t)unit;
            break;
        }
    }

    if (!found) {
        /* 未找到合适的 UNIT 值，禁止采样器后返回 */
        reg->CR_f.SEN = 0U;
        return;
    }

    /* 步骤 3：确定 N（从 N=10 到 N=0，找最高置位） */
    len_val = (reg->CFGR & DLYB_CFGR_LEN_Msk) >> DLYB_CFGR_LEN_Pos;
    for (n = 10; n >= 0; n--) {
        if (len_val & (1U << (uint32_t)n)) {
            calib->len_count = (uint8_t)n;
            calib->valid     = true;
            break;
        }
    }

    /* 步骤 4：SEN=0，使能输出时钟 */
    reg->CR_f.SEN = 0U;
}

//===========================================
// 第4层：控制与查询 API
//===========================================

void dlyb_enable(dlyb_inst_t inst)
{
    dlyb_get_reg(inst)->CR_f.DEN = 1U;
}

void dlyb_disable(dlyb_inst_t inst)
{
    dlyb_get_reg(inst)->CR_f.DEN = 0U;
}

void dlyb_sampler_enable(dlyb_inst_t inst)
{
    dlyb_get_reg(inst)->CR_f.SEN = 1U;
}

void dlyb_sampler_disable(dlyb_inst_t inst)
{
    dlyb_get_reg(inst)->CR_f.SEN = 0U;
}

void dlyb_set_unit(dlyb_inst_t inst, uint8_t unit)
{
    if (unit > DLYB_UNIT_MAX) {
        unit = DLYB_UNIT_MAX;
    }
    DLYB_TypeDef *reg = dlyb_get_reg(inst);
    uint32_t cfgr = reg->CFGR;
    cfgr &= ~DLYB_CFGR_UNIT_Msk;
    cfgr |=  ((uint32_t)unit << DLYB_CFGR_UNIT_Pos) & DLYB_CFGR_UNIT_Msk;
    reg->CFGR = cfgr;
}

void dlyb_set_sel(dlyb_inst_t inst, uint8_t sel)
{
    if (sel > DLYB_SEL_MAX) {
        sel = DLYB_SEL_MAX;
    }
    DLYB_TypeDef *reg = dlyb_get_reg(inst);
    uint32_t cfgr = reg->CFGR;
    cfgr &= ~DLYB_CFGR_SEL_Msk;
    cfgr |=  ((uint32_t)sel << DLYB_CFGR_SEL_Pos) & DLYB_CFGR_SEL_Msk;
    reg->CFGR = cfgr;
}

bool dlyb_wait_lenf(dlyb_inst_t inst)
{
    DLYB_TypeDef *reg = dlyb_get_reg(inst);
    uint32_t count = DLYB_LENF_TIMEOUT_COUNT;
    while ((reg->CFGR & DLYB_CFGR_LENF_Msk) == 0U) {
        if (--count == 0U) {
            return false;
        }
    }
    return true;
}

bool dlyb_is_len_valid(dlyb_inst_t inst)
{
    return (dlyb_get_reg(inst)->CFGR & DLYB_CFGR_LENF_Msk) != 0U;
}

uint16_t dlyb_get_len(dlyb_inst_t inst)
{
    return (uint16_t)((dlyb_get_reg(inst)->CFGR & DLYB_CFGR_LEN_Msk)
                      >> DLYB_CFGR_LEN_Pos);
}

bool dlyb_is_enabled(dlyb_inst_t inst)
{
    return (dlyb_get_reg(inst)->CR & DLYB_CR_DEN_Msk) != 0U;
}

bool dlyb_is_sampler_enabled(dlyb_inst_t inst)
{
    return (dlyb_get_reg(inst)->CR & DLYB_CR_SEN_Msk) != 0U;
}

void dlyb_get_status(dlyb_inst_t inst, dlyb_status_t *status)
{
    assert(status != NULL);

    DLYB_TypeDef *reg = dlyb_get_reg(inst);
    uint32_t cr   = reg->CR;
    uint32_t cfgr = reg->CFGR;

    status->den  = (cr   & DLYB_CR_DEN_Msk)   != 0U;
    status->sen  = (cr   & DLYB_CR_SEN_Msk)   != 0U;
    status->lenf = (cfgr & DLYB_CFGR_LENF_Msk) != 0U;
    status->len  = (uint16_t)((cfgr & DLYB_CFGR_LEN_Msk)  >> DLYB_CFGR_LEN_Pos);
    status->unit = (uint8_t)((cfgr  & DLYB_CFGR_UNIT_Msk) >> DLYB_CFGR_UNIT_Pos);
    status->sel  = (uint8_t)((cfgr  & DLYB_CFGR_SEL_Msk)  >> DLYB_CFGR_SEL_Pos);
}
