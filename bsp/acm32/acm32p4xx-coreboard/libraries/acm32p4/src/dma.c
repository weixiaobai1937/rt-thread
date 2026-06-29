/**
 * @file dma.c
 * @brief ACM32P4 DMA控制器驱动实现
 *
 * 本文件实现了ACM32P4芯片的DMA驱动，提供以下功能：
 * - M2M/M2P/P2M/P2P 四种传输方向
 * - 链表传输（含循环传输和双缓冲传输）
 * - 传输完成/半传输完成/传输错误中断
 * - 中断回调函数注册
 *
 * @author ACM32P4 SDK Team
 * @version 2.0
 */

#include <string.h>
#include <assert.h>
#include "hardware/clocks.h"
#include "device/acm32p4xx.h"
#include "hardware/dma.h"

/*
 * ============================================================================
 * DMA1 请求映射表（ACM32P4 手册表 7-1）
 * ============================================================================
 *  REQ  请求源        REQ  请求源        REQ  请求源        REQ  请求源
 * ----------------------------------------------------------------------------
 *    0  ADC1           32  I2S1_TX        64  UART5_RX       96  （保留）
 *    1  SPI1_TX        33  I2S1_RX        65  UART6_TX       97  （保留）
 *    2  SPI1_RX        34  I2S2_TX        66  UART6_RX       98  （保留）
 *    3  SPI2_TX        35  I2S2_RX        67  UART7_TX       99  （保留）
 *    4  SPI2_RX        36  DAC1_CH1       68  UART7_RX       100 （保留）
 *    5  UART1_TX       37  DAC1_CH2       69  UART8_TX       101 （保留）
 *    6  UART1_RX       38  UART4_TX       70  UART8_RX       102 （保留）
 *    7  UART2_TX       39  UART4_RX       71  FDCAN1_TX      103 （保留）
 *    8  UART2_RX       40  SPI3_TX        72  FDCAN1_RX      104 （保留）
 *    9  I2C1_TX        41  SPI3_RX        73  FDCAN2_TX      105 （保留）
 *   10  I2C1_RX        42  TIM4_CH1       74  FDCAN2_RX      106 （保留）
 *   11  I2C2_TX        43  TIM4_CH2       75  （保留）         107 （保留）
 *   12  I2C2_RX        44  TIM4_CH3       76  （保留）         108 （保留）
 *   13  TIM1_CH1       45  TIM4_CH4       77  （保留）         109 （保留）
 *   14  TIM1_CH2       46  TIM4_UP        78  （保留）         110 （保留）
 *   15  TIM1_CH3       47  TIM4_TRIG      79  （保留）         111 （保留）
 *   16  TIM1_CH4       48  SPI7_RX        80  （保留）         112 （保留）
 *   17  TIM1_UP        49  SPI7_TX        81  （保留）         113 （保留）
 *   18  TIM1_TRIG      50  TIM5_UP        82  （保留）         114 （保留）
 *   19  TIM1_COM       51  TIM5_CH1       83  （保留）         115 （保留）
 *   20  TIM2_CH1       52  TIM5_CH2       84  （保留）         116 （保留）
 *   21  TIM2_CH2       53  TIM5_CH3       85  （保留）         117 （保留）
 *   22  TIM2_CH3       54  TIM5_CH4       86  （保留）         118 （保留）
 *   23  TIM2_CH4       55  TIM5_TRIG      87  （保留）         119 （保留）
 *   24  TIM2_UP        56  TIM5_COM       88  （保留）         120 （保留）
 *   25  TIM2_TRIG      57  SPI4_RX        89  （保留）         121 （保留）
 *   26  UART3_TX       58  SPI4_TX        90  （保留）         122 （保留）
 *   27  UART3_RX       59  OSPI1_RX       91  （保留）         123 （保留）
 *   28  LPUART1_TX     60  OSPI1_TX       92  （保留）         124 （保留）
 *   29  LPUART1_RX     61  OSPI2_RX       93  （保留）         125 （保留）
 *   30  LPUART2_TX     62  OSPI2_TX       94  （保留）         126 （保留）
 *   31  LPUART2_RX     63  UART5_TX       95  （保留）         127 （保留）
 * ============================================================================
 *
 * ============================================================================
 * DMA2 请求映射表（ACM32P4 手册表 7-1）
 * ============================================================================
 *  REQ  请求源        REQ  请求源        REQ  请求源        REQ  请求源
 * ----------------------------------------------------------------------------
 *    0  ADC1           32  I2S1_TX        64  UART5_RX       96  （保留）
 *    1  SPI1_TX        33  I2S1_RX        65  UART6_TX       97  （保留）
 *    2  SPI1_RX        34  I2S2_TX        66  UART6_RX       98  （保留）
 *    3  SPI2_TX        35  I2S2_RX        67  UART7_TX       99  （保留）
 *    4  SPI2_RX        36  DAC1_CH1       68  UART7_RX       100 （保留）
 *    5  UART1_TX       37  DAC1_CH2       69  UART8_TX       101 （保留）
 *    6  UART1_RX       38  UART4_TX       70  UART8_RX       102 （保留）
 *    7  UART2_TX       39  UART4_RX       71  FDCAN1_TX      103 （保留）
 *    8  UART2_RX       40  SPI3_TX        72  FDCAN1_RX      104 （保留）
 *    9  I2C1_TX        41  SPI3_RX        73  FDCAN2_TX      105 （保留）
 *   10  I2C1_RX        42  TIM6_UP        74  FDCAN2_RX      106 （保留）
 *   11  I2C2_TX        43  TIM7_UP        75  （保留）         107 （保留）
 *   12  I2C2_RX        44  （保留）         76  （保留）         108 （保留）
 *   13  TIM8_CH1       45  （保留）         77  （保留）         109 （保留）
 *   14  TIM8_CH2       46  （保留）         78  （保留）         110 （保留）
 *   15  TIM8_CH3       47  （保留）         79  （保留）         111 （保留）
 *   16  TIM8_CH4       48  SPI7_RX        80  （保留）         112 （保留）
 *   17  TIM8_UP        49  SPI7_TX        81  （保留）         113 （保留）
 *   18  TIM8_TRIG      50  TIM5_UP        82  （保留）         114 （保留）
 *   19  TIM8_COM       51  TIM5_CH1       83  （保留）         115 （保留）
 *   20  TIM3_CH1       52  TIM5_CH2       84  （保留）         116 （保留）
 *   21  TIM3_CH2       53  TIM5_CH3       85  （保留）         117 （保留）
 *   22  TIM3_CH3       54  TIM5_CH4       86  （保留）         118 （保留）
 *   23  TIM3_CH4       55  TIM5_TRIG      87  （保留）         119 （保留）
 *   24  TIM3_UP        56  TIM5_COM       88  （保留）         120 （保留）
 *   25  TIM3_TRIG      57  SPI4_RX        89  （保留）         121 （保留）
 *   26  UART3_TX       58  SPI4_TX        90  （保留）         122 （保留）
 *   27  UART3_RX       59  OSPI1_RX       91  （保留）         123 （保留）
 *   28  LPUART1_TX     60  OSPI1_TX       92  （保留）         124 （保留）
 *   29  LPUART1_RX     61  OSPI2_RX       93  （保留）         125 （保留）
 *   30  LPUART2_TX     62  OSPI2_TX       94  （保留）         126 （保留）
 *   31  LPUART2_RX     63  UART5_TX       95  （保留）         127 （保留）
 * ============================================================================
 */

//===========================================
// 内部宏定义
//===========================================

#define DMA_GET_BASE(instance) \
    ((instance) == DMA_1 ? DMAC1 : DMAC2)

// DMA_Cx_CTRL寄存器位域位置
#define DMA_CTRL_ITC_POS 31U               ///< [31] 原始传输完成中断使能
#define DMA_CTRL_DIORDD_POS 28U            ///< [29:28] 目标地址变化模式（2位）
#define DMA_CTRL_SIORSD_POS 26U            ///< [27:26] 源地址变化模式（2位）
#define DMA_CTRL_DWIDTH_POS 24U            ///< [25:24] 目标传输位宽（2位）
#define DMA_CTRL_SWIDTH_POS 22U            ///< [23:22] 源传输位宽（2位）
#define DMA_CTRL_DBSIZE_POS 19U            ///< [21:19] 目标突发大小（3位）
#define DMA_CTRL_SBSIZE_POS 16U            ///< [18:16] 源突发大小（3位）
#define DMA_CTRL_TRANSFERSIZE_MASK 0xFFFFU ///< [15:0] 传输次数掩码

// DMA_Cx_CONFIG寄存器位定义
#define DMA_CONFIG_SRCID_POS 24U
#define DMA_CONFIG_DESTID_POS 16U
#define DMA_CONFIG_LOCK_BIT (1U << 11)
#define DMA_CONFIG_DMST_BIT (1U << 10)
#define DMA_CONFIG_SMST_BIT (1U << 9)
#define DMA_CONFIG_HALT_BIT (1U << 8)
#define DMA_CONFIG_ACTIVE_BIT (1U << 7)
#define DMA_CONFIG_IHFTC_BIT (1U << 6)
#define DMA_CONFIG_ITC_BIT (1U << 5)
#define DMA_CONFIG_IE_BIT (1U << 4)
#define DMA_CONFIG_FLOWCTRL_POS 1U
#define DMA_CONFIG_EN_BIT (1U << 0)

// DMA全局配置寄存器位定义
#define DMA_GLOBAL_M2ENDIAN_BIT (1U << 2)
#define DMA_GLOBAL_M1ENDIAN_BIT (1U << 1)
#define DMA_GLOBAL_EN_BIT (1U << 0)

// DMA_Cx_LLI寄存器：bit[0]=LM（AHB主机选择），bit[1]=保留，bit[31:2]=地址[31:2]
// 地址直接存储在寄存器中（4字节对齐，bit[1:0]不用于地址）
#define DMA_LLI_LM_BIT (1U << 0)

// 等待传输完成的超时计数基准值（基于循环计数，非精确毫秒）
#define DMA_WAIT_LOOP_COUNT 100000U

//===========================================
// 内部类型定义
//===========================================

// 通道寄存器访问结构体（匹配硬件寄存器布局）
typedef struct
{
    volatile uint32_t SRC_ADDR;  ///< 0x00: 源地址
    volatile uint32_t DEST_ADDR; ///< 0x04: 目标地址
    volatile uint32_t LLI;       ///< 0x08: 链表节点寄存器
    volatile uint32_t CTRL;      ///< 0x0C: 控制寄存器
    volatile uint32_t CONFIG;    ///< 0x10: 配置寄存器
} dma_ch_regs_t;

// 通道内部状态
typedef struct
{
    bool in_use;               ///< 通道是否已注册回调
    dma_callbacks_t callbacks; ///< 中断回调函数集合
} dma_ch_state_t;

//===========================================
// 内部状态变量
//===========================================

// 所有DMA实例的通道状态
static dma_ch_state_t g_dma_state[DMA_COUNT][DMA_CHANNEL_COUNT];

// 循环传输/双缓冲的内部链表节点池（每通道最多2个节点）
static dma_lli_node_t g_lli_pool[DMA_COUNT][DMA_CHANNEL_COUNT][2];

//===========================================
// 内部辅助函数
//===========================================

/**
 * @brief 获取指定通道的寄存器指针
 *
 * 通道寄存器起始于DMA基地址偏移0x100处，每个通道占0x20字节。
 */
static inline dma_ch_regs_t *get_ch_regs(dma_instance_t instance, dma_channel_t channel)
{
    uint8_t *base = (uint8_t *)DMA_GET_BASE(instance);
    return (dma_ch_regs_t *)(base + 0x100U + (uint32_t)channel * 0x20U);
}

/**
 * @brief 检查地址对齐是否符合位宽要求
 */
static bool is_aligned(uint32_t address, dma_data_width_t width)
{
    switch (width) {
    case DMA_WIDTH_BYTE:
        return true;
    case DMA_WIDTH_HALFWORD:
        return (address & 0x1U) == 0U;
    case DMA_WIDTH_WORD:
        return (address & 0x3U) == 0U;
    default:
        return false;
    }
}

/**
 * @brief 根据数据位宽给出快速初始化场景的默认突发大小
 *
 * @note 外设流（M2P/P2M）默认建议使用 BURST_1，以匹配大多数按次请求的外设握手节奏。
 */
static dma_burst_size_t default_burst_for_width(dma_data_width_t width, bool peripheral_flow)
{
    if (peripheral_flow)
    {
        return DMA_BURST_1;
    }

    switch (width) {
    case DMA_WIDTH_WORD:
        return DMA_BURST_4;
    case DMA_WIDTH_HALFWORD:
        return DMA_BURST_4;
    case DMA_WIDTH_BYTE:
    default:
        return DMA_BURST_1;
    }
}

/**
 * @brief 根据通道配置生成CTRL寄存器值
 */
static uint32_t build_ctrl(const dma_channel_config_t *config)
{
    uint32_t ctrl = 0;

    ctrl |= (1U << DMA_CTRL_ITC_POS);  // 使能RAW状态（轮询和中断均依赖此位）
    ctrl |= ((uint32_t)config->dest_addr_mode << DMA_CTRL_DIORDD_POS);
    ctrl |= ((uint32_t)config->src_addr_mode << DMA_CTRL_SIORSD_POS);
    ctrl |= ((uint32_t)config->dest_width << DMA_CTRL_DWIDTH_POS);
    ctrl |= ((uint32_t)config->src_width << DMA_CTRL_SWIDTH_POS);
    ctrl |= ((uint32_t)config->dest_burst << DMA_CTRL_DBSIZE_POS);
    ctrl |= ((uint32_t)config->src_burst << DMA_CTRL_SBSIZE_POS);
    ctrl |= (uint32_t)config->transfer_size & DMA_CTRL_TRANSFERSIZE_MASK;

    return ctrl;
}

/**
 * @brief 根据通道配置生成CONFIG寄存器值（不含EN位）
 */
static uint32_t build_config(const dma_channel_config_t *config)
{
    uint32_t cfg = 0;

    cfg |= ((uint32_t)config->src_periph_id << DMA_CONFIG_SRCID_POS);
    cfg |= ((uint32_t)config->dest_periph_id << DMA_CONFIG_DESTID_POS);
    cfg |= ((uint32_t)config->direction << DMA_CONFIG_FLOWCTRL_POS);

    if (config->bus_lock)
    {
        cfg |= DMA_CONFIG_LOCK_BIT;
    }
    if (config->src_master == DMA_MASTER_2)
    {
        cfg |= DMA_CONFIG_SMST_BIT;
    }
    if (config->dest_master == DMA_MASTER_2)
    {
        cfg |= DMA_CONFIG_DMST_BIT;
    }
    if (config->ht_interrupt_enable)
    {
        cfg |= DMA_CONFIG_IHFTC_BIT;
    }
    if (config->tc_interrupt_enable)
    {
        cfg |= DMA_CONFIG_ITC_BIT;
    }
    if (config->error_interrupt_enable)
    {
        cfg |= DMA_CONFIG_IE_BIT;
    }

    return cfg;
}

//===========================================
// 第1层：快速初始化API
//===========================================

bool dma_init_mem2mem(
    dma_instance_t instance,
    dma_channel_t channel,
    uint32_t src_address,
    uint32_t dest_address,
    uint32_t byte_count)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return false;
    }
    if (byte_count == 0)
    {
        return false;
    }
    if (dma_is_channel_enabled(instance, channel))
    {
        return false;
    }

    // 传输次数上限：65535次，最大字节数 65535*4=262140
    if (byte_count > 262140U)
    {
        return false;
    }

    dma_enable(instance);

    // 字传输：每次移动4字节，向上取整计算传输次数
    uint16_t transfer_count = (uint16_t)((byte_count + 3U) / 4U);

    dma_channel_config_t config = {
        .direction = DMA_MEM_TO_MEM,
        .src_address = src_address,
        .dest_address = dest_address,
        .src_addr_mode = DMA_ADDR_INCREMENT,
        .dest_addr_mode = DMA_ADDR_INCREMENT,
        .src_width = DMA_WIDTH_WORD,
        .dest_width = DMA_WIDTH_WORD,
        .src_burst = DMA_BURST_4,
        .dest_burst = DMA_BURST_4,
        .transfer_size = transfer_count,
        .src_periph_id = 0,
        .dest_periph_id = 0,
        .src_master = DMA_MASTER_1,
        .dest_master = DMA_MASTER_1,
        .bus_lock = false,
        .tc_interrupt_enable = true,
        .ht_interrupt_enable = false,
        .error_interrupt_enable = false,
    };

    return dma_config_channel(instance, channel, &config);
}

bool dma_init_periph2mem(
    dma_instance_t instance,
    dma_channel_t channel,
    uint8_t periph_id,
    uint32_t periph_address,
    uint32_t mem_address,
    uint16_t transfer_count,
    dma_data_width_t periph_width)
{
    dma_burst_size_t burst;

    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return false;
    }
    if (transfer_count == 0)
    {
        return false;
    }
    if (dma_is_channel_enabled(instance, channel))
    {
        return false;
    }

    dma_enable(instance);

    burst = default_burst_for_width(periph_width, true);

    dma_channel_config_t config = {
        .direction = DMA_PERIPH_TO_MEM,
        .src_address = periph_address,
        .dest_address = mem_address,
        .src_addr_mode = DMA_ADDR_FIXED,
        .dest_addr_mode = DMA_ADDR_INCREMENT,
        .src_width = periph_width,
        .dest_width = periph_width,
        .src_burst = burst,
        .dest_burst = burst,
        .transfer_size = transfer_count,
        .src_periph_id = periph_id,
        .dest_periph_id = 0,
        .src_master = DMA_MASTER_1,
        .dest_master = DMA_MASTER_1,
        .bus_lock = false,
        .tc_interrupt_enable = true,
        .ht_interrupt_enable = false,
        .error_interrupt_enable = false,
    };

    return dma_config_channel(instance, channel, &config);
}

bool dma_init_mem2periph(
    dma_instance_t instance,
    dma_channel_t channel,
    uint8_t periph_id,
    uint32_t mem_address,
    uint32_t periph_address,
    uint16_t transfer_count,
    dma_data_width_t periph_width)
{
    dma_burst_size_t burst;

    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return false;
    }
    if (transfer_count == 0)
    {
        return false;
    }
    if (dma_is_channel_enabled(instance, channel))
    {
        return false;
    }

    dma_enable(instance);

    burst = default_burst_for_width(periph_width, true);

    dma_channel_config_t config = {
        .direction = DMA_MEM_TO_PERIPH,
        .src_address = mem_address,
        .dest_address = periph_address,
        .src_addr_mode = DMA_ADDR_INCREMENT,
        .dest_addr_mode = DMA_ADDR_FIXED,
        .src_width = periph_width,
        .dest_width = periph_width,
        .src_burst = burst,
        .dest_burst = burst,
        .transfer_size = transfer_count,
        .src_periph_id = 0,
        .dest_periph_id = periph_id,
        .src_master = DMA_MASTER_1,
        .dest_master = DMA_MASTER_1,
        .bus_lock = false,
        .tc_interrupt_enable = true,
        .ht_interrupt_enable = false,
        .error_interrupt_enable = false,
    };

    return dma_config_channel(instance, channel, &config);
}

bool dma_init_circular(
    dma_instance_t instance,
    dma_channel_t channel,
    uint32_t src_address,
    uint32_t dest_address,
    uint32_t byte_count)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return false;
    }
    if (byte_count == 0)
    {
        return false;
    }
    if (dma_is_channel_enabled(instance, channel))
    {
        return false;
    }

    // 传输次数上限：65535次，最大字节数 65535*4=262140
    if (byte_count > 262140U)
    {
        return false;
    }

    dma_enable(instance);

    uint16_t transfer_count = (uint16_t)((byte_count + 3U) / 4U);
    if (transfer_count == 0)
    {
        transfer_count = 1;
    }

    // BURST_4：每次突发4个字 = 16字节，与通道FIFO容量（16字节）匹配
    uint32_t ctrl = 0;
    ctrl |= (1U << DMA_CTRL_ITC_POS);
    ctrl |= ((uint32_t)DMA_ADDR_INCREMENT << DMA_CTRL_DIORDD_POS);
    ctrl |= ((uint32_t)DMA_ADDR_INCREMENT << DMA_CTRL_SIORSD_POS);
    ctrl |= ((uint32_t)DMA_WIDTH_WORD << DMA_CTRL_DWIDTH_POS);
    ctrl |= ((uint32_t)DMA_WIDTH_WORD << DMA_CTRL_SWIDTH_POS);
    ctrl |= ((uint32_t)DMA_BURST_4 << DMA_CTRL_DBSIZE_POS);
    ctrl |= ((uint32_t)DMA_BURST_4 << DMA_CTRL_SBSIZE_POS);
    ctrl |= (uint32_t)transfer_count;

    // 单节点自循环链表
    dma_lli_node_t *node = &g_lli_pool[instance][channel][0];
    dma_init_lli_node(node, src_address, dest_address, ctrl, node, DMA_MASTER_1);

    dma_linked_list_config_t ll_cfg = {
        .nodes = node,
        .node_count = 1,
        .circular = true, // 已通过节点自引用实现循环
        .lli_master = DMA_MASTER_1,
        .direction = DMA_MEM_TO_MEM,
        .src_periph_id = 0,
        .dest_periph_id = 0,
        .src_master = DMA_MASTER_1,
        .dest_master = DMA_MASTER_1,
        .bus_lock = false,
        .tc_interrupt_enable = true,
        .ht_interrupt_enable = false,
        .error_interrupt_enable = false,
    };

    return dma_config_linked_list(instance, channel, &ll_cfg);
}

//===========================================
// 第2层：基础配置API
//===========================================

bool dma_config_global(
    dma_instance_t instance,
    const dma_global_config_t *config)
{
    if (instance >= DMA_COUNT || config == NULL)
    {
        return false;
    }

    DMAC_TypeDef *dma = DMA_GET_BASE(instance);
    uint32_t val = 0;

    if (config->enable)
    {
        val |= DMA_GLOBAL_EN_BIT;
    }
    if (config->master1_endian == DMA_ENDIAN_BIG)
    {
        val |= DMA_GLOBAL_M1ENDIAN_BIT;
    }
    if (config->master2_endian == DMA_ENDIAN_BIG)
    {
        val |= DMA_GLOBAL_M2ENDIAN_BIT;
    }

    dma->CONFIG = val;
    return true;
}

bool dma_config_channel(
    dma_instance_t instance,
    dma_channel_t channel,
    const dma_channel_config_t *config)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT || config == NULL)
    {
        return false;
    }
    if (config->transfer_size == 0)
    {
        return false;
    }
    if (!is_aligned(config->src_address, config->src_width))
    {
        return false;
    }
    if (!is_aligned(config->dest_address, config->dest_width))
    {
        return false;
    }
    if (dma_is_channel_enabled(instance, channel))
    {
        return false;
    }

    dma_ch_regs_t *ch = get_ch_regs(instance, channel);
    ch->SRC_ADDR = config->src_address;
    ch->DEST_ADDR = config->dest_address;
    ch->LLI = 0;
    ch->CTRL = build_ctrl(config);
    ch->CONFIG = build_config(config);

    return true;
}

bool dma_config_address(
    dma_instance_t instance,
    dma_channel_t channel,
    uint32_t src_address,
    uint32_t dest_address)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return false;
    }
    if (dma_is_channel_enabled(instance, channel))
    {
        return false;
    }

    dma_ch_regs_t *ch = get_ch_regs(instance, channel);
    ch->SRC_ADDR = src_address;
    ch->DEST_ADDR = dest_address;

    return true;
}

bool dma_config_transfer_size(
    dma_instance_t instance,
    dma_channel_t channel,
    uint16_t size)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return false;
    }
    if (size == 0)
    {
        return false;
    }
    if (dma_is_channel_enabled(instance, channel))
    {
        return false;
    }

    dma_ch_regs_t *ch = get_ch_regs(instance, channel);
    ch->CTRL = (ch->CTRL & ~DMA_CTRL_TRANSFERSIZE_MASK) | (uint32_t)size;

    return true;
}

bool dma_config_interrupt(
    dma_instance_t instance,
    dma_channel_t channel,
    bool tc_enable,
    bool ht_enable,
    bool error_enable)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return false;
    }

    dma_ch_regs_t *ch = get_ch_regs(instance, channel);

    // CTRL.ITC 由 build_ctrl / dma_config_channel 在通道启动前一次性配置。
    // 手册 7.5.14：通道使能后 CTRL 寄存器不可写，因此此处仅操作 CONFIG 屏蔽位。
    // 若通道已使能，禁止运行时修改 CTRL.ITC，防止原始中断标志停止产生。
    if (!dma_is_channel_enabled(instance, channel))
    {
        if (tc_enable || ht_enable)
        {
            ch->CTRL |= (1U << DMA_CTRL_ITC_POS);
        }
        else
        {
            ch->CTRL &= ~(1U << DMA_CTRL_ITC_POS);
        }
    }

    uint32_t cfg = ch->CONFIG;
    cfg &= ~(DMA_CONFIG_ITC_BIT | DMA_CONFIG_IHFTC_BIT | DMA_CONFIG_IE_BIT);
    if (tc_enable)
    {
        cfg |= DMA_CONFIG_ITC_BIT;
    }
    if (ht_enable)
    {
        cfg |= DMA_CONFIG_IHFTC_BIT;
    }
    if (error_enable)
    {
        cfg |= DMA_CONFIG_IE_BIT;
    }
    ch->CONFIG = cfg;

    return true;
}

bool dma_reset_channel(
    dma_instance_t instance,
    dma_channel_t channel)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return false;
    }

    dma_stop_channel(instance, channel);
    dma_clear_channel_all_flags(instance, channel);

    dma_ch_regs_t *ch = get_ch_regs(instance, channel);
    ch->SRC_ADDR = 0;
    ch->DEST_ADDR = 0;
    ch->LLI = 0;
    ch->CTRL = 0;
    ch->CONFIG = 0;

    memset(&g_dma_state[instance][channel], 0, sizeof(dma_ch_state_t));

    return true;
}

//===========================================
// 第3层：高级功能API
//===========================================

bool dma_config_linked_list(
    dma_instance_t instance,
    dma_channel_t channel,
    const dma_linked_list_config_t *config)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT || config == NULL)
    {
        return false;
    }
    if (config->nodes == NULL || config->node_count == 0)
    {
        return false;
    }
    if (dma_is_channel_enabled(instance, channel))
    {
        return false;
    }

    // 循环模式：将最后一个节点链接到第一个节点
    if (config->circular)
    {
        dma_link_nodes(&config->nodes[config->node_count - 1U],
                       &config->nodes[0],
                       config->lli_master);
    }

    // 将第一个节点加载到通道寄存器（传输完成后硬件自动从next_lli加载下一节点）
    // 注意：必须先写 CTRL（含 TRANSFERSIZE），再写 LLI。
    // 原因：部分硬件在 LLI 非零时会屏蔽对 CTRL.TRANSFERSIZE 的写入。
    dma_ch_regs_t *ch = get_ch_regs(instance, channel);
    ch->SRC_ADDR = config->nodes[0].src_address;
    ch->DEST_ADDR = config->nodes[0].dest_address;
    ch->CTRL = config->nodes[0].control;
    ch->LLI = config->nodes[0].next_lli;

    // 配置CONFIG寄存器（方向、外设ID、AHB主机、中断使能）
    uint32_t cfg = 0;
    cfg |= ((uint32_t)config->src_periph_id << DMA_CONFIG_SRCID_POS);
    cfg |= ((uint32_t)config->dest_periph_id << DMA_CONFIG_DESTID_POS);
    cfg |= ((uint32_t)config->direction << DMA_CONFIG_FLOWCTRL_POS);

    if (config->bus_lock)
    {
        cfg |= DMA_CONFIG_LOCK_BIT;
    }
    if (config->src_master == DMA_MASTER_2)
    {
        cfg |= DMA_CONFIG_SMST_BIT;
    }
    if (config->dest_master == DMA_MASTER_2)
    {
        cfg |= DMA_CONFIG_DMST_BIT;
    }
    if (config->tc_interrupt_enable)
    {
        cfg |= DMA_CONFIG_ITC_BIT;
    }
    if (config->ht_interrupt_enable)
    {
        cfg |= DMA_CONFIG_IHFTC_BIT;
    }
    if (config->error_interrupt_enable)
    {
        cfg |= DMA_CONFIG_IE_BIT;
    }

    ch->CONFIG = cfg;

    return true;
}

void dma_init_lli_node(
    dma_lli_node_t *node,
    uint32_t src_address,
    uint32_t dest_address,
    uint32_t control,
    const dma_lli_node_t *next,
    dma_ahb_master_t lli_master)
{
    assert(node != NULL);

    node->src_address = src_address;
    node->dest_address = dest_address;
    node->control = control;

    if (next != NULL)
    {
        // 文档：LLI寄存器[31:2] 直接存储地址[31:2]（Addr[1:0]固定为0）。
        // bit[0] = LM（AHB主机选择）。
        // 编码：寄存器 = (地址 & ~3) | LM_bit；
        // 硬件解码：地址 = 寄存器 & ~3，即"DMA_Cx_LLI.LLI << 2"中的LLI字段值。
        node->next_lli = ((uint32_t)next & ~3U) | (lli_master == DMA_MASTER_2 ? DMA_LLI_LM_BIT : 0U);
    }
    else
    {
        node->next_lli = 0;
    }
}

void dma_link_nodes(
    dma_lli_node_t *current,
    const dma_lli_node_t *next,
    dma_ahb_master_t lli_master)
{
    assert(current != NULL);

    if (next != NULL)
    {
        current->next_lli = ((uint32_t)next & ~3U) | (lli_master == DMA_MASTER_2 ? DMA_LLI_LM_BIT : 0U);
    }
    else
    {
        current->next_lli = 0;
    }
}

bool dma_config_burst(
    dma_instance_t instance,
    dma_channel_t channel,
    dma_burst_size_t src_burst,
    dma_burst_size_t dest_burst)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return false;
    }
    if (dma_is_channel_enabled(instance, channel))
    {
        return false;
    }

    dma_ch_regs_t *ch = get_ch_regs(instance, channel);
    uint32_t ctrl = ch->CTRL;
    ctrl &= ~(0x7U << DMA_CTRL_SBSIZE_POS);
    ctrl &= ~(0x7U << DMA_CTRL_DBSIZE_POS);
    ctrl |= ((uint32_t)src_burst << DMA_CTRL_SBSIZE_POS);
    ctrl |= ((uint32_t)dest_burst << DMA_CTRL_DBSIZE_POS);
    ch->CTRL = ctrl;

    return true;
}

bool dma_config_data_width(
    dma_instance_t instance,
    dma_channel_t channel,
    dma_data_width_t src_width,
    dma_data_width_t dest_width)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return false;
    }
    if (dma_is_channel_enabled(instance, channel))
    {
        return false;
    }

    dma_ch_regs_t *ch = get_ch_regs(instance, channel);
    uint32_t ctrl = ch->CTRL;
    ctrl &= ~(0x3U << DMA_CTRL_SWIDTH_POS);
    ctrl &= ~(0x3U << DMA_CTRL_DWIDTH_POS);
    ctrl |= ((uint32_t)src_width << DMA_CTRL_SWIDTH_POS);
    ctrl |= ((uint32_t)dest_width << DMA_CTRL_DWIDTH_POS);
    ch->CTRL = ctrl;

    return true;
}

bool dma_config_address_mode(
    dma_instance_t instance,
    dma_channel_t channel,
    dma_addr_mode_t src_mode,
    dma_addr_mode_t dest_mode)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return false;
    }
    if (dma_is_channel_enabled(instance, channel))
    {
        return false;
    }

    dma_ch_regs_t *ch = get_ch_regs(instance, channel);
    uint32_t ctrl = ch->CTRL;
    ctrl &= ~(0x3U << DMA_CTRL_SIORSD_POS);
    ctrl &= ~(0x3U << DMA_CTRL_DIORDD_POS);
    ctrl |= ((uint32_t)src_mode << DMA_CTRL_SIORSD_POS);
    ctrl |= ((uint32_t)dest_mode << DMA_CTRL_DIORDD_POS);
    ch->CTRL = ctrl;

    return true;
}

bool dma_config_endian(
    dma_instance_t instance,
    dma_endian_t master1_endian,
    dma_endian_t master2_endian)
{
    if (instance >= DMA_COUNT)
    {
        return false;
    }

    DMAC_TypeDef *dma = DMA_GET_BASE(instance);
    uint32_t cfg = dma->CONFIG;
    cfg &= ~(DMA_GLOBAL_M1ENDIAN_BIT | DMA_GLOBAL_M2ENDIAN_BIT);
    if (master1_endian == DMA_ENDIAN_BIG)
    {
        cfg |= DMA_GLOBAL_M1ENDIAN_BIT;
    }
    if (master2_endian == DMA_ENDIAN_BIG)
    {
        cfg |= DMA_GLOBAL_M2ENDIAN_BIT;
    }
    dma->CONFIG = cfg;

    return true;
}

bool dma_config_periph_request(
    dma_instance_t instance,
    dma_channel_t channel,
    uint8_t src_periph_id,
    uint8_t dest_periph_id)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return false;
    }
    if (dma_is_channel_enabled(instance, channel))
    {
        return false;
    }

    dma_ch_regs_t *ch = get_ch_regs(instance, channel);
    uint32_t cfg = ch->CONFIG;
    cfg &= ~(0xFFU << DMA_CONFIG_SRCID_POS);
    cfg &= ~(0xFFU << DMA_CONFIG_DESTID_POS);
    cfg |= ((uint32_t)src_periph_id << DMA_CONFIG_SRCID_POS);
    cfg |= ((uint32_t)dest_periph_id << DMA_CONFIG_DESTID_POS);
    ch->CONFIG = cfg;

    return true;
}

bool dma_config_flow_control(
    dma_instance_t instance,
    dma_channel_t channel,
    dma_direction_t direction)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return false;
    }
    if (dma_is_channel_enabled(instance, channel))
    {
        return false;
    }

    dma_ch_regs_t *ch = get_ch_regs(instance, channel);
    uint32_t cfg = ch->CONFIG;
    cfg &= ~(0x7U << DMA_CONFIG_FLOWCTRL_POS);
    cfg |= ((uint32_t)direction << DMA_CONFIG_FLOWCTRL_POS);
    ch->CONFIG = cfg;

    return true;
}

bool dma_config_ahb_master(
    dma_instance_t instance,
    dma_channel_t channel,
    dma_ahb_master_t src_master,
    dma_ahb_master_t dest_master)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return false;
    }
    if (dma_is_channel_enabled(instance, channel))
    {
        return false;
    }

    dma_ch_regs_t *ch = get_ch_regs(instance, channel);
    uint32_t cfg = ch->CONFIG;
    cfg &= ~(DMA_CONFIG_SMST_BIT | DMA_CONFIG_DMST_BIT);
    if (src_master == DMA_MASTER_2)
    {
        cfg |= DMA_CONFIG_SMST_BIT;
    }
    if (dest_master == DMA_MASTER_2)
    {
        cfg |= DMA_CONFIG_DMST_BIT;
    }
    ch->CONFIG = cfg;

    return true;
}

bool dma_config_bus_lock(
    dma_instance_t instance,
    dma_channel_t channel,
    bool enable)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return false;
    }
    if (dma_is_channel_enabled(instance, channel))
    {
        return false;
    }

    dma_ch_regs_t *ch = get_ch_regs(instance, channel);
    if (enable)
    {
        ch->CONFIG |= DMA_CONFIG_LOCK_BIT;
    }
    else
    {
        ch->CONFIG &= ~DMA_CONFIG_LOCK_BIT;
    }

    return true;
}

bool dma_config_double_buffer(
    dma_instance_t instance,
    dma_channel_t channel,
    const dma_double_buffer_config_t *config)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT || config == NULL)
    {
        return false;
    }
    if (config->buffer_size == 0)
    {
        return false;
    }
    if (dma_is_channel_enabled(instance, channel))
    {
        return false;
    }

    if (!is_aligned(config->buffer0_address, config->data_width))
    {
        return false;
    }
    if (!is_aligned(config->buffer1_address, config->data_width))
    {
        return false;
    }
    if (!is_aligned(config->periph_address, config->data_width))
    {
        return false;
    }

    dma_enable(instance);

    // 构建每个节点的控制字（使能ITC以产生传输完成中断标志）
    uint32_t ctrl = 0;
    ctrl |= (1U << DMA_CTRL_ITC_POS);
    ctrl |= ((uint32_t)DMA_BURST_4 << DMA_CTRL_SBSIZE_POS);
    ctrl |= ((uint32_t)DMA_BURST_4 << DMA_CTRL_DBSIZE_POS);
    ctrl |= ((uint32_t)config->data_width << DMA_CTRL_SWIDTH_POS);
    ctrl |= ((uint32_t)config->data_width << DMA_CTRL_DWIDTH_POS);
    ctrl |= (uint32_t)config->buffer_size & DMA_CTRL_TRANSFERSIZE_MASK;

    uint32_t src0, src1, dst0, dst1;

    if (config->direction == DMA_PERIPH_TO_MEM)
    {
        // P2M：源为固定外设，目标为交替缓冲区
        ctrl |= ((uint32_t)DMA_ADDR_FIXED << DMA_CTRL_SIORSD_POS);
        ctrl |= ((uint32_t)DMA_ADDR_INCREMENT << DMA_CTRL_DIORDD_POS);
        src0 = config->periph_address;
        src1 = config->periph_address;
        dst0 = config->buffer0_address;
        dst1 = config->buffer1_address;
    }
    else
    {
        // M2P：源为交替缓冲区，目标为固定外设
        ctrl |= ((uint32_t)DMA_ADDR_INCREMENT << DMA_CTRL_SIORSD_POS);
        ctrl |= ((uint32_t)DMA_ADDR_FIXED << DMA_CTRL_DIORDD_POS);
        src0 = config->buffer0_address;
        src1 = config->buffer1_address;
        dst0 = config->periph_address;
        dst1 = config->periph_address;
    }

    dma_lli_node_t *nodes = &g_lli_pool[instance][channel][0];

    dma_init_lli_node(&nodes[0], src0, dst0, ctrl,
                      &nodes[1], DMA_MASTER_1);
    dma_init_lli_node(&nodes[1], src1, dst1, ctrl,
                      config->circular ? &nodes[0] : NULL, DMA_MASTER_1);

    dma_linked_list_config_t ll_cfg = {
        .nodes = nodes,
        .node_count = 2,
        .circular = false, // 已手动设置循环链接，此处不重复设置
        .lli_master = DMA_MASTER_1,
        .direction = config->direction,
        .src_periph_id = (config->direction == DMA_PERIPH_TO_MEM) ? config->periph_id : 0,
        .dest_periph_id = (config->direction == DMA_MEM_TO_PERIPH) ? config->periph_id : 0,
        .src_master = DMA_MASTER_1,
        .dest_master = DMA_MASTER_1,
        .bus_lock = false,
        .tc_interrupt_enable = config->tc_interrupt_enable,
        .ht_interrupt_enable = false,
        .error_interrupt_enable = config->error_interrupt_enable,
    };

    return dma_config_linked_list(instance, channel, &ll_cfg);
}

bool dma_get_channel_status(
    dma_instance_t instance,
    dma_channel_t channel,
    dma_channel_status_t *status)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT || status == NULL)
    {
        return false;
    }

    DMAC_TypeDef *dma = DMA_GET_BASE(instance);
    dma_ch_regs_t *ch = get_ch_regs(instance, channel);

    status->enabled = (dma->ENCHSTATUS & (1U << channel)) != 0U;
    status->active = (ch->CONFIG & DMA_CONFIG_ACTIVE_BIT) != 0U;
    status->halted = (ch->CONFIG & DMA_CONFIG_HALT_BIT) != 0U;
    status->remaining_size = (uint16_t)(ch->CTRL & DMA_CTRL_TRANSFERSIZE_MASK);
    status->current_src_addr = ch->SRC_ADDR;
    status->current_dest_addr = ch->DEST_ADDR;

    return true;
}

uint16_t dma_get_transfer_progress(
    dma_instance_t instance,
    dma_channel_t channel)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return 0;
    }

    dma_ch_regs_t *ch = get_ch_regs(instance, channel);
    return (uint16_t)(ch->CTRL & DMA_CTRL_TRANSFERSIZE_MASK);
}

bool dma_is_channel_busy(
    dma_instance_t instance,
    dma_channel_t channel)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return false;
    }

    dma_ch_regs_t *ch = get_ch_regs(instance, channel);
    return (ch->CONFIG & DMA_CONFIG_ACTIVE_BIT) != 0U;
}

bool dma_wait_transfer_complete(
    dma_instance_t instance,
    dma_channel_t channel,
    uint32_t timeout)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return false;
    }

    DMAC_TypeDef *dma = DMA_GET_BASE(instance);
    uint32_t ch_mask = 1U << channel;
    uint32_t counter = 0;
    bool use_timeout = (timeout > 0U);

    while (true)
    {
        if (dma->RAWINTTCSTATUS & ch_mask)
        {
            dma->INTTCCLR = ch_mask;
            return true;
        }
        if (dma->RAWINTERRSTATUS & ch_mask)
        {
            dma->INTERRCLR = ch_mask;
            return false;
        }
        if (use_timeout)
        {
            counter++;
            if (counter >= timeout * DMA_WAIT_LOOP_COUNT)
            {
                return false;
            }
        }
    }
}

//===========================================
// 第4层：控制与查询API
//===========================================

void dma_enable(dma_instance_t instance)
{
    if (instance >= DMA_COUNT)
    {
        return;
    }

    if (instance == DMA_1)
    {
        clock_periph_enable(CLK_DMA1);
    }
    else
    {
        clock_periph_enable(CLK_DMA2);
    }

    DMAC_TypeDef *dma = DMA_GET_BASE(instance);
    dma->CONFIG |= DMA_GLOBAL_EN_BIT;
}

void dma_disable(dma_instance_t instance)
{
    if (instance >= DMA_COUNT)
    {
        return;
    }

    DMAC_TypeDef *dma = DMA_GET_BASE(instance);
    dma->CONFIG &= ~DMA_GLOBAL_EN_BIT;

    if (instance == DMA_1)
    {
        clock_periph_disable(CLK_DMA1);
    }
    else
    {
        clock_periph_disable(CLK_DMA2);
    }
}

void dma_start_channel(dma_instance_t instance, dma_channel_t channel)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return;
    }

    dma_ch_regs_t *ch = get_ch_regs(instance, channel);
    ch->CONFIG |= DMA_CONFIG_EN_BIT;
}

void dma_stop_channel(dma_instance_t instance, dma_channel_t channel)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return;
    }

    dma_ch_regs_t *ch = get_ch_regs(instance, channel);
    ch->CONFIG &= ~DMA_CONFIG_EN_BIT;
}

void dma_pause_channel(dma_instance_t instance, dma_channel_t channel)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return;
    }

    dma_ch_regs_t *ch = get_ch_regs(instance, channel);
    ch->CONFIG |= DMA_CONFIG_HALT_BIT;
}

void dma_resume_channel(dma_instance_t instance, dma_channel_t channel)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return;
    }

    dma_ch_regs_t *ch = get_ch_regs(instance, channel);
    ch->CONFIG &= ~DMA_CONFIG_HALT_BIT;
}

bool dma_abort_channel(
    dma_instance_t instance,
    dma_channel_t channel,
    uint32_t timeout)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return false;
    }

    // 步骤1：暂停通道，阻止后续传输请求
    dma_pause_channel(instance, channel);

    // 步骤2：等待ACTIVE位清零（FIFO中的数据传输完毕）
    uint32_t counter = 0;
    bool use_timeout = (timeout > 0U);

    while (dma_is_channel_busy(instance, channel))
    {
        if (use_timeout)
        {
            counter++;
            if (counter >= timeout * DMA_WAIT_LOOP_COUNT)
            {
                dma_stop_channel(instance, channel);
                return false;
            }
        }
    }

    // 步骤3：禁用通道
    dma_stop_channel(instance, channel);
    return true;
}

void dma_set_source_address(
    dma_instance_t instance,
    dma_channel_t channel,
    uint32_t address)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return;
    }

    dma_ch_regs_t *ch = get_ch_regs(instance, channel);
    ch->SRC_ADDR = address;
}

void dma_set_destination_address(
    dma_instance_t instance,
    dma_channel_t channel,
    uint32_t address)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return;
    }

    dma_ch_regs_t *ch = get_ch_regs(instance, channel);
    ch->DEST_ADDR = address;
}

uint32_t dma_get_source_address(
    dma_instance_t instance,
    dma_channel_t channel)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return 0;
    }

    dma_ch_regs_t *ch = get_ch_regs(instance, channel);
    return ch->SRC_ADDR;
}

uint32_t dma_get_destination_address(
    dma_instance_t instance,
    dma_channel_t channel)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return 0;
    }

    dma_ch_regs_t *ch = get_ch_regs(instance, channel);
    return ch->DEST_ADDR;
}

void dma_enable_interrupt(
    dma_instance_t instance,
    dma_channel_t channel,
    dma_irq_type_t irq_type)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return;
    }

    dma_ch_regs_t *ch = get_ch_regs(instance, channel);

    // CTRL.ITC 在通道使能后不可写（手册 7.5.14）。
    // 运行时仅操作 CONFIG 屏蔽位即可控制中断送达 NVIC。
    // CTRL.ITC 必须在通道启动前由 dma_config_channel/build_ctrl 配置为 1。
    if (irq_type & DMA_IRQ_TC)
    {
        ch->CONFIG |= DMA_CONFIG_ITC_BIT;
    }
    if (irq_type & DMA_IRQ_HT)
    {
        ch->CONFIG |= DMA_CONFIG_IHFTC_BIT;
    }
    if (irq_type & DMA_IRQ_ERROR)
    {
        ch->CONFIG |= DMA_CONFIG_IE_BIT;
    }
}

void dma_disable_interrupt(
    dma_instance_t instance,
    dma_channel_t channel,
    dma_irq_type_t irq_type)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return;
    }

    dma_ch_regs_t *ch = get_ch_regs(instance, channel);

    if (irq_type & DMA_IRQ_TC)
    {
        ch->CONFIG &= ~DMA_CONFIG_ITC_BIT;
    }
    if (irq_type & DMA_IRQ_HT)
    {
        ch->CONFIG &= ~DMA_CONFIG_IHFTC_BIT;
    }
    if (irq_type & DMA_IRQ_ERROR)
    {
        ch->CONFIG &= ~DMA_CONFIG_IE_BIT;
    }
}

void dma_enable_tc_interrupt(dma_instance_t instance, dma_channel_t channel)
{
    dma_enable_interrupt(instance, channel, DMA_IRQ_TC);
}

void dma_disable_tc_interrupt(dma_instance_t instance, dma_channel_t channel)
{
    dma_disable_interrupt(instance, channel, DMA_IRQ_TC);
}

void dma_enable_ht_interrupt(dma_instance_t instance, dma_channel_t channel)
{
    dma_enable_interrupt(instance, channel, DMA_IRQ_HT);
}

void dma_disable_ht_interrupt(dma_instance_t instance, dma_channel_t channel)
{
    dma_disable_interrupt(instance, channel, DMA_IRQ_HT);
}

void dma_enable_error_interrupt(dma_instance_t instance, dma_channel_t channel)
{
    dma_enable_interrupt(instance, channel, DMA_IRQ_ERROR);
}

void dma_disable_error_interrupt(dma_instance_t instance, dma_channel_t channel)
{
    dma_disable_interrupt(instance, channel, DMA_IRQ_ERROR);
}

uint32_t dma_get_interrupt_status(dma_instance_t instance)
{
    if (instance >= DMA_COUNT)
    {
        return 0;
    }

    DMAC_TypeDef *dma = DMA_GET_BASE(instance);
    return dma->INTSTATUS & 0xFFU;
}

uint32_t dma_get_channel_interrupt_status(
    dma_instance_t instance,
    dma_channel_t channel)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return 0;
    }

    DMAC_TypeDef *dma = DMA_GET_BASE(instance);
    uint32_t ch_mask = 1U << channel;
    uint32_t status = 0;

    if (dma->INTTCSTATUS & ch_mask)
    {
        status |= (uint32_t)DMA_IRQ_TC;
    }
    if (dma->INTTCSTATUS & (ch_mask << 8))
    {
        status |= (uint32_t)DMA_IRQ_HT;
    }
    if (dma->INTERRSTATUS & ch_mask)
    {
        status |= (uint32_t)DMA_IRQ_ERROR;
    }

    return status;
}

uint32_t dma_get_raw_interrupt_status(
    dma_instance_t instance,
    dma_channel_t channel)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return 0;
    }

    DMAC_TypeDef *dma = DMA_GET_BASE(instance);
    uint32_t ch_mask = 1U << channel;
    uint32_t status = 0;

    if (dma->RAWINTTCSTATUS & ch_mask)
    {
        status |= (uint32_t)DMA_IRQ_TC;
    }
    if (dma->RAWINTTCSTATUS & (ch_mask << 8))
    {
        status |= (uint32_t)DMA_IRQ_HT;
    }
    if (dma->RAWINTERRSTATUS & ch_mask)
    {
        status |= (uint32_t)DMA_IRQ_ERROR;
    }

    return status;
}

uint32_t dma_get_tc_status(dma_instance_t instance)
{
    if (instance >= DMA_COUNT)
    {
        return 0;
    }

    DMAC_TypeDef *dma = DMA_GET_BASE(instance);
    return dma->INTTCSTATUS & 0xFFU;
}

uint32_t dma_get_ht_status(dma_instance_t instance)
{
    if (instance >= DMA_COUNT)
    {
        return 0;
    }

    DMAC_TypeDef *dma = DMA_GET_BASE(instance);
    return (dma->INTTCSTATUS >> 8) & 0xFFU;
}

uint32_t dma_get_error_status(dma_instance_t instance)
{
    if (instance >= DMA_COUNT)
    {
        return 0;
    }

    DMAC_TypeDef *dma = DMA_GET_BASE(instance);
    return dma->INTERRSTATUS & 0xFFU;
}

bool dma_is_transfer_complete(dma_instance_t instance, dma_channel_t channel)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return false;
    }

    DMAC_TypeDef *dma = DMA_GET_BASE(instance);
    return (dma->RAWINTTCSTATUS & (1U << channel)) != 0U;
}

bool dma_is_half_transfer_complete(dma_instance_t instance, dma_channel_t channel)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return false;
    }

    DMAC_TypeDef *dma = DMA_GET_BASE(instance);
    return (dma->RAWINTTCSTATUS & (1U << (channel + 8U))) != 0U;
}

bool dma_has_error(dma_instance_t instance, dma_channel_t channel)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return false;
    }

    DMAC_TypeDef *dma = DMA_GET_BASE(instance);
    return (dma->RAWINTERRSTATUS & (1U << channel)) != 0U;
}

void dma_clear_interrupt_flag(
    dma_instance_t instance,
    dma_channel_t channel,
    dma_irq_type_t irq_type)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return;
    }

    DMAC_TypeDef *dma = DMA_GET_BASE(instance);
    uint32_t ch_mask = 1U << channel;

    if (irq_type & DMA_IRQ_TC)
    {
        dma->INTTCCLR = ch_mask;
    }
    if (irq_type & DMA_IRQ_HT)
    {
        dma->INTTCCLR = ch_mask << 8;
    }
    if (irq_type & DMA_IRQ_ERROR)
    {
        dma->INTERRCLR = ch_mask;
    }
}

void dma_clear_tc_flag(dma_instance_t instance, dma_channel_t channel)
{
    dma_clear_interrupt_flag(instance, channel, DMA_IRQ_TC);
}

void dma_clear_ht_flag(dma_instance_t instance, dma_channel_t channel)
{
    dma_clear_interrupt_flag(instance, channel, DMA_IRQ_HT);
}

void dma_clear_error_flag(dma_instance_t instance, dma_channel_t channel)
{
    dma_clear_interrupt_flag(instance, channel, DMA_IRQ_ERROR);
}

void dma_clear_channel_all_flags(dma_instance_t instance, dma_channel_t channel)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return;
    }

    DMAC_TypeDef *dma = DMA_GET_BASE(instance);
    uint32_t ch_mask = 1U << channel;
    dma->INTTCCLR = ch_mask | (ch_mask << 8);
    dma->INTERRCLR = ch_mask;
}

void dma_clear_all_interrupt_flags(dma_instance_t instance)
{
    if (instance >= DMA_COUNT)
    {
        return;
    }

    DMAC_TypeDef *dma = DMA_GET_BASE(instance);
    dma->INTTCCLR = 0xFFFFU;
    dma->INTERRCLR = 0xFFU;
}

bool dma_register_callback(
    dma_instance_t instance,
    dma_channel_t channel,
    const dma_callbacks_t *callbacks)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT || callbacks == NULL)
    {
        return false;
    }

    g_dma_state[instance][channel].callbacks = *callbacks;
    g_dma_state[instance][channel].in_use = true;

    return true;
}

void dma_unregister_callback(dma_instance_t instance, dma_channel_t channel)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return;
    }

    memset(&g_dma_state[instance][channel], 0, sizeof(dma_ch_state_t));
}

void dma_register_tc_callback(
    dma_instance_t instance,
    dma_channel_t channel,
    dma_tc_callback_t callback)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return;
    }

    g_dma_state[instance][channel].callbacks.tc_callback = callback;
    g_dma_state[instance][channel].in_use = true;
}

void dma_register_ht_callback(
    dma_instance_t instance,
    dma_channel_t channel,
    dma_ht_callback_t callback)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return;
    }

    g_dma_state[instance][channel].callbacks.ht_callback = callback;
    g_dma_state[instance][channel].in_use = true;
}

void dma_register_error_callback(
    dma_instance_t instance,
    dma_channel_t channel,
    dma_error_callback_t callback)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return;
    }

    g_dma_state[instance][channel].callbacks.error_callback = callback;
    g_dma_state[instance][channel].in_use = true;
}

uint32_t dma_get_enabled_channels(dma_instance_t instance)
{
    if (instance >= DMA_COUNT)
    {
        return 0;
    }

    DMAC_TypeDef *dma = DMA_GET_BASE(instance);
    return dma->ENCHSTATUS & 0xFFU;
}

bool dma_is_channel_enabled(dma_instance_t instance, dma_channel_t channel)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return false;
    }

    DMAC_TypeDef *dma = DMA_GET_BASE(instance);
    return (dma->ENCHSTATUS & (1U << channel)) != 0U;
}

bool dma_is_channel_halted(dma_instance_t instance, dma_channel_t channel)
{
    if (instance >= DMA_COUNT || channel >= DMA_CHANNEL_COUNT)
    {
        return false;
    }

    dma_ch_regs_t *ch = get_ch_regs(instance, channel);
    return (ch->CONFIG & DMA_CONFIG_HALT_BIT) != 0U;
}

//===========================================
// 中断处理函数
//===========================================

/**
 * @brief 通用DMA中断处理函数（内部使用）
 */
static void dma_irq_handler(dma_instance_t instance, dma_channel_t channel)
{
    DMAC_TypeDef *dma = DMA_GET_BASE(instance);
    uint32_t ch_mask = 1U << channel;
    uint32_t tc_status = dma->INTTCSTATUS;
    uint32_t er_status = dma->INTERRSTATUS;

    // 处理传输完成中断（TC）
    if (tc_status & ch_mask)
    {
        dma->INTTCCLR = ch_mask;
        if (g_dma_state[instance][channel].callbacks.tc_callback != NULL)
        {
            g_dma_state[instance][channel].callbacks.tc_callback(instance, channel);
        }
    }

    // 处理半传输完成中断（HT）
    if (tc_status & (ch_mask << 8))
    {
        dma->INTTCCLR = ch_mask << 8;
        if (g_dma_state[instance][channel].callbacks.ht_callback != NULL)
        {
            g_dma_state[instance][channel].callbacks.ht_callback(instance, channel);
        }
    }

    // 处理传输错误中断（ERROR）
    if (er_status & ch_mask)
    {
        dma->INTERRCLR = ch_mask;
        if (g_dma_state[instance][channel].callbacks.error_callback != NULL)
        {
            g_dma_state[instance][channel].callbacks.error_callback(instance, channel);
        }
    }
}

// DMA1通道中断服务函数
void DMA1_CH0_IRQHandler(void) { dma_irq_handler(DMA_1, DMA_CHANNEL_0); }
void DMA1_CH1_IRQHandler(void) { dma_irq_handler(DMA_1, DMA_CHANNEL_1); }
void DMA1_CH2_IRQHandler(void) { dma_irq_handler(DMA_1, DMA_CHANNEL_2); }
void DMA1_CH3_IRQHandler(void) { dma_irq_handler(DMA_1, DMA_CHANNEL_3); }
void DMA1_CH4_IRQHandler(void) { dma_irq_handler(DMA_1, DMA_CHANNEL_4); }
void DMA1_CH5_IRQHandler(void) { dma_irq_handler(DMA_1, DMA_CHANNEL_5); }
void DMA1_CH6_IRQHandler(void) { dma_irq_handler(DMA_1, DMA_CHANNEL_6); }
void DMA1_CH7_IRQHandler(void) { dma_irq_handler(DMA_1, DMA_CHANNEL_7); }

// DMA2通道中断服务函数
void DMA2_CH0_IRQHandler(void) { dma_irq_handler(DMA_2, DMA_CHANNEL_0); }
void DMA2_CH1_IRQHandler(void) { dma_irq_handler(DMA_2, DMA_CHANNEL_1); }
void DMA2_CH2_IRQHandler(void) { dma_irq_handler(DMA_2, DMA_CHANNEL_2); }
void DMA2_CH3_IRQHandler(void) { dma_irq_handler(DMA_2, DMA_CHANNEL_3); }
void DMA2_CH4_IRQHandler(void) { dma_irq_handler(DMA_2, DMA_CHANNEL_4); }
void DMA2_CH5_IRQHandler(void) { dma_irq_handler(DMA_2, DMA_CHANNEL_5); }
void DMA2_CH6_IRQHandler(void) { dma_irq_handler(DMA_2, DMA_CHANNEL_6); }
void DMA2_CH7_IRQHandler(void) { dma_irq_handler(DMA_2, DMA_CHANNEL_7); }
