/**
 * @file dma.h
 * @brief ACM32P4 DMA控制器驱动接口
 *
 * 本驱动支持ACM32P4系列芯片的DMA功能：
 * - 2个DMA控制器（DMA1/DMA2），每个8通道
 * - 支持存储器到存储器（M2M）、存储器到外设（M2P）、外设到存储器（P2M）传输
 * - 每个DMA支持256个外设请求源
 * - 支持字节/半字/字三种数据位宽，源和目标位宽可不同（自动封装/解封）
 * - 支持地址递增/递减/固定模式
 * - 支持突发传输（1/4/8/16/32/64/128/256次）
 * - 支持链表传输、循环传输、双缓冲传输
 * - 支持大端/小端模式
 * - 支持传输完成/半传输完成/传输错误中断
 *
 * @author ACM32P4 SDK Team
 * @version 2.0
 */

#ifndef _HARDWARE_DMA_H
#define _HARDWARE_DMA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

//===========================================
// 枚举类型定义
//===========================================

/**
 * @brief DMA控制器实例
 */
typedef enum
{
    DMA_1     = 0,  ///< DMA1控制器
    DMA_2     = 1,  ///< DMA2控制器
    DMA_COUNT       ///< DMA控制器总数
} dma_instance_t;

/**
 * @brief DMA通道编号（通道0优先级最高，通道7优先级最低）
 */
typedef enum
{
    DMA_CHANNEL_0 = 0,  ///< 通道0（最高优先级）
    DMA_CHANNEL_1 = 1,  ///< 通道1
    DMA_CHANNEL_2 = 2,  ///< 通道2
    DMA_CHANNEL_3 = 3,  ///< 通道3
    DMA_CHANNEL_4 = 4,  ///< 通道4
    DMA_CHANNEL_5 = 5,  ///< 通道5
    DMA_CHANNEL_6 = 6,  ///< 通道6
    DMA_CHANNEL_7 = 7,  ///< 通道7（最低优先级）
    DMA_CHANNEL_COUNT   ///< 通道总数
} dma_channel_t;

/**
 * @brief DMA传输方向（对应DMA_Cx_CONFIG.FLOWCTRL）
 */
typedef enum
{
    DMA_MEM_TO_MEM       = 0,  ///< 存储器到存储器（M2M），DMA控制流
    DMA_MEM_TO_PERIPH    = 1,  ///< 存储器到外设（M2P），DMA控制流
    DMA_PERIPH_TO_MEM    = 2,  ///< 外设到存储器（P2M），DMA控制流
    DMA_PERIPH_TO_PERIPH = 3   ///< 外设到外设（P2P），DMA控制流
} dma_direction_t;

/**
 * @brief DMA传输数据位宽（对应DMA_Cx_CTRL.SWIDTH / DWIDTH）
 */
typedef enum
{
    DMA_WIDTH_BYTE     = 0,  ///< 字节（8位）
    DMA_WIDTH_HALFWORD = 1,  ///< 半字（16位）
    DMA_WIDTH_WORD     = 2   ///< 字（32位）
} dma_data_width_t;

/**
 * @brief DMA地址变化模式（对应DMA_Cx_CTRL.SIORSD / DIORDD）
 */
typedef enum
{
    DMA_ADDR_FIXED     = 0,  ///< 地址固定不变（用于外设寄存器）
    DMA_ADDR_INCREMENT = 1,  ///< 地址递增（每次加1/2/4字节，取决于位宽）
    DMA_ADDR_DECREMENT = 2   ///< 地址递减
} dma_addr_mode_t;

/**
 * @brief DMA突发传输大小（对应DMA_Cx_CTRL.SBSIZE / DBSIZE）
 *
 * @note M2M模式推荐使用DMA_BURST_16或更大以提高传输效率
 * @note P2M/M2P模式推荐设置为外设FIFO深度的一半
 */
typedef enum
{
    DMA_BURST_1   = 0,  ///< 每次请求传输1次
    DMA_BURST_4   = 1,  ///< 每次请求传输4次
    DMA_BURST_8   = 2,  ///< 每次请求传输8次
    DMA_BURST_16  = 3,  ///< 每次请求传输16次
    DMA_BURST_32  = 4,  ///< 每次请求传输32次
    DMA_BURST_64  = 5,  ///< 每次请求传输64次
    DMA_BURST_128 = 6,  ///< 每次请求传输128次
    DMA_BURST_256 = 7   ///< 每次请求传输256次
} dma_burst_size_t;

/**
 * @brief DMA字节序模式（对应DMA_CONFIG.M1ENDIAN / M2ENDIAN）
 */
typedef enum
{
    DMA_ENDIAN_LITTLE = 0,  ///< 小端模式
    DMA_ENDIAN_BIG    = 1   ///< 大端模式
} dma_endian_t;

/**
 * @brief AHB主机选择（对应DMA_Cx_CONFIG.SMST / DMST 和 DMA_Cx_LLI.LM）
 */
typedef enum
{
    DMA_MASTER_1 = 0,  ///< AHB主机1
    DMA_MASTER_2 = 1   ///< AHB主机2
} dma_ahb_master_t;

/**
 * @brief DMA中断类型（可组合使用）
 */
typedef enum
{
    DMA_IRQ_NONE  = 0,          ///< 无中断
    DMA_IRQ_TC    = (1U << 0),  ///< 传输完成中断
    DMA_IRQ_HT    = (1U << 1),  ///< 半传输完成中断
    DMA_IRQ_ERROR = (1U << 2),  ///< 传输错误中断
    DMA_IRQ_ALL   = 0x07U       ///< 所有中断
} dma_irq_type_t;

//===========================================
// 结构体类型定义
//===========================================

/**
 * @brief DMA全局配置结构体
 *
 */
typedef struct
{
    bool         enable;          ///< DMA控制器使能（DMA_CONFIG.EN）
    dma_endian_t master1_endian;  ///< AHB主机1字节序（DMA_CONFIG.M1ENDIAN）
    dma_endian_t master2_endian;  ///< AHB主机2字节序（DMA_CONFIG.M2ENDIAN）
} dma_global_config_t;

/**
 * @brief DMA通道完整配置结构体
 *
 *                   DMA_Cx_CTRL（全部位域）、DMA_Cx_CONFIG（全部位域）
 */
typedef struct
{
    dma_direction_t  direction;          ///< 传输方向（FLOWCTRL）
    uint32_t         src_address;        ///< 源地址
    uint32_t         dest_address;       ///< 目标地址
    dma_addr_mode_t  src_addr_mode;      ///< 源地址变化模式（SIORSD）
    dma_addr_mode_t  dest_addr_mode;     ///< 目标地址变化模式（DIORDD）
    dma_data_width_t src_width;          ///< 源数据位宽（SWIDTH）
    dma_data_width_t dest_width;         ///< 目标数据位宽（DWIDTH）
    dma_burst_size_t src_burst;          ///< 源突发大小（SBSIZE）
    dma_burst_size_t dest_burst;         ///< 目标突发大小（DBSIZE）
    uint16_t         transfer_size;      ///< 传输次数（1-65535）（TRANSFERSIZE）
    uint8_t          src_periph_id;      ///< 源外设请求ID（0-255）（SRCID）
    uint8_t          dest_periph_id;     ///< 目标外设请求ID（0-255）（DESTID）
    dma_ahb_master_t src_master;         ///< 源AHB主机（SMST）
    dma_ahb_master_t dest_master;        ///< 目标AHB主机（DMST）
    bool             bus_lock;           ///< AHB总线锁定（LOCK）
    bool             tc_interrupt_enable;   ///< 传输完成中断使能（CTRL.ITC + CONFIG.ITC）
    bool             ht_interrupt_enable;   ///< 半传输完成中断使能（CTRL.ITC + CONFIG.IHFTC）
    bool             error_interrupt_enable;///< 传输错误中断使能（CONFIG.IE）
} dma_channel_config_t;

/**
 * @brief DMA链表节点结构体
 *
 * @note 硬件按此顺序依次加载4个32位字，必须4字节对齐
 * @note next_lli字段编码：寄存器 = (节点地址 & ~3) | LM_bit。
 *       硬件地址解码：地址 = 寄存器 & ~3（即文档中"LLI字段[31:2] = 地址[31:2]"）
 * @note 若next_lli=0，表示最后一个节点，传输完成后硬件关闭通道
 */
typedef struct
{
    uint32_t src_address;   ///< 源地址（加载到DMA_Cx_SRC_ADDR）
    uint32_t dest_address;  ///< 目标地址（加载到DMA_Cx_DEST_ADDR）
    uint32_t next_lli;      ///< 下一节点地址，0表示最后节点（加载到DMA_Cx_LLI）
    uint32_t control;       ///< 控制字，即DMA_Cx_CTRL寄存器值（加载到DMA_Cx_CTRL）
} __attribute__((aligned(16))) dma_lli_node_t;

/**
 * @brief DMA链表传输配置结构体
 *
 *                   DMA_Cx_CTRL、DMA_Cx_CONFIG
 */
typedef struct
{
    dma_lli_node_t  *nodes;               ///< 链表节点数组指针（必须4字节对齐）
    uint32_t         node_count;          ///< 链表节点数量（>=1）
    bool             circular;            ///< 是否循环（最后节点指向第一节点）
    dma_ahb_master_t lli_master;          ///< 链表节点所在存储器使用的AHB主机（LM位）
    dma_direction_t  direction;           ///< 传输方向（FLOWCTRL）
    uint8_t          src_periph_id;       ///< 源外设请求ID（M2M时忽略）（SRCID）
    uint8_t          dest_periph_id;      ///< 目标外设请求ID（M2M时忽略）（DESTID）
    dma_ahb_master_t src_master;          ///< 源AHB主机（SMST）
    dma_ahb_master_t dest_master;         ///< 目标AHB主机（DMST）
    bool             bus_lock;            ///< AHB总线锁定（LOCK）
    bool             tc_interrupt_enable;    ///< 传输完成中断使能（CONFIG.ITC）
    bool             ht_interrupt_enable;    ///< 半传输完成中断使能（CONFIG.IHFTC）
    bool             error_interrupt_enable; ///< 传输错误中断使能（CONFIG.IE）
} dma_linked_list_config_t;

/**
 * @brief DMA双缓冲传输配置结构体
 *
 * @note P2M模式：periph_address为固定源地址，buffer0/buffer1为交替目标地址
 * @note M2P模式：buffer0/buffer1为交替源地址，periph_address为固定目标地址
 */
typedef struct
{
    uint32_t         buffer0_address;        ///< 内存缓冲区0地址
    uint32_t         buffer1_address;        ///< 内存缓冲区1地址
    uint32_t         periph_address;         ///< 外设地址（P2M时为源地址，M2P时为目标地址）
    uint16_t         buffer_size;            ///< 每个缓冲区传输次数（1-65535）
    dma_data_width_t data_width;             ///< 数据位宽
    dma_direction_t  direction;              ///< 传输方向（P2M或M2P）
    uint8_t          periph_id;              ///< 外设请求ID
    bool             circular;               ///< 是否循环传输（传输完后自动重启）
    bool             tc_interrupt_enable;    ///< 传输完成中断使能
    bool             error_interrupt_enable; ///< 传输错误中断使能
} dma_double_buffer_config_t;

/**
 * @brief DMA通道运行状态结构体
 */
typedef struct
{
    bool     enabled;            ///< 通道是否已使能（DMA_EN_CH_STATUS）
    bool     active;             ///< 通道FIFO中是否有数据（CONFIG.ACTIVE）
    bool     halted;             ///< 通道是否已暂停（CONFIG.HALT）
    uint16_t remaining_size;     ///< 剩余传输次数（CTRL.TRANSFERSIZE，仅供参考）
    uint32_t current_src_addr;   ///< 当前源地址（DMA_Cx_SRC_ADDR）
    uint32_t current_dest_addr;  ///< 当前目标地址（DMA_Cx_DEST_ADDR）
} dma_channel_status_t;

//===========================================
// 回调函数类型
//===========================================

/**
 * @brief DMA传输完成回调函数类型
 *
 * @param[in] instance 触发中断的DMA实例
 * @param[in] channel  触发中断的DMA通道
 */
typedef void (*dma_tc_callback_t)(dma_instance_t instance, dma_channel_t channel);

/**
 * @brief DMA半传输完成回调函数类型
 *
 * @param[in] instance 触发中断的DMA实例
 * @param[in] channel  触发中断的DMA通道
 */
typedef void (*dma_ht_callback_t)(dma_instance_t instance, dma_channel_t channel);

/**
 * @brief DMA传输错误回调函数类型
 *
 * @param[in] instance 触发中断的DMA实例
 * @param[in] channel  触发中断的DMA通道
 */
typedef void (*dma_error_callback_t)(dma_instance_t instance, dma_channel_t channel);

/**
 * @brief DMA回调函数集合
 */
typedef struct
{
    dma_tc_callback_t    tc_callback;     ///< 传输完成回调（可为NULL）
    dma_ht_callback_t    ht_callback;     ///< 半传输完成回调（可为NULL）
    dma_error_callback_t error_callback;  ///< 传输错误回调（可为NULL）
} dma_callbacks_t;

//===========================================
// 第1层：快速初始化API
//===========================================

/**
 * @brief 快速初始化存储器到存储器传输
 *
 * 使用32位位宽、地址递增、突发16的默认配置，完成M2M传输的一次性初始化。
 * 调用此函数后使用 dma_start_channel() 启动传输。
 *
 * @param[in] instance     DMA实例
 * @param[in] channel      DMA通道
 * @param[in] src_address  源起始地址（必须4字节对齐）
 * @param[in] dest_address 目标起始地址（必须4字节对齐）
 * @param[in] byte_count   传输字节数（建议4字节倍数，1-262140）
 * @return true: 配置成功, false: 参数错误或通道已使能
 *
 * @note 覆盖寄存器：DMA_Cx_SRC_ADDR, DMA_Cx_DEST_ADDR, DMA_Cx_LLI, DMA_Cx_CTRL, DMA_Cx_CONFIG, DMA_CONFIG
 */
bool dma_init_mem2mem(
    dma_instance_t instance,
    dma_channel_t channel,
    uint32_t src_address,
    uint32_t dest_address,
    uint32_t byte_count
);

/**
 * @brief 快速初始化外设到存储器传输
 *
 * 使用外设地址固定、存储器地址递增的默认配置，完成P2M传输的一次性初始化。
 *
 * @param[in] instance       DMA实例
 * @param[in] channel        DMA通道
 * @param[in] periph_id      外设请求ID（参见外设请求号表）
 * @param[in] periph_address 外设数据寄存器地址（固定地址）
 * @param[in] mem_address    存储器目标地址
 * @param[in] transfer_count 传输次数（1-65535，与periph_width对应）
 * @param[in] periph_width   外设数据位宽
 * @return true: 配置成功, false: 参数错误或通道已使能
 *
 * @note 覆盖寄存器：DMA_Cx_SRC_ADDR, DMA_Cx_DEST_ADDR, DMA_Cx_LLI, DMA_Cx_CTRL, DMA_Cx_CONFIG, DMA_CONFIG
 */
bool dma_init_periph2mem(
    dma_instance_t instance,
    dma_channel_t channel,
    uint8_t periph_id,
    uint32_t periph_address,
    uint32_t mem_address,
    uint16_t transfer_count,
    dma_data_width_t periph_width
);

/**
 * @brief 快速初始化存储器到外设传输
 *
 * 使用存储器地址递增、外设地址固定的默认配置，完成M2P传输的一次性初始化。
 *
 * @param[in] instance       DMA实例
 * @param[in] channel        DMA通道
 * @param[in] periph_id      外设请求ID（参见外设请求号表）
 * @param[in] mem_address    存储器源地址
 * @param[in] periph_address 外设数据寄存器地址（固定地址）
 * @param[in] transfer_count 传输次数（1-65535，与periph_width对应）
 * @param[in] periph_width   外设数据位宽
 * @return true: 配置成功, false: 参数错误或通道已使能
 *
 * @note 覆盖寄存器：DMA_Cx_SRC_ADDR, DMA_Cx_DEST_ADDR, DMA_Cx_LLI, DMA_Cx_CTRL, DMA_Cx_CONFIG, DMA_CONFIG
 */
bool dma_init_mem2periph(
    dma_instance_t instance,
    dma_channel_t channel,
    uint8_t periph_id,
    uint32_t mem_address,
    uint32_t periph_address,
    uint16_t transfer_count,
    dma_data_width_t periph_width
);

/**
 * @brief 快速初始化M2M循环传输（单节点自循环链表）
 *
 * 使用链表模式实现自动循环的存储器到存储器传输。
 *
 * @param[in] instance      DMA实例
 * @param[in] channel       DMA通道
 * @param[in] src_address   源地址（必须4字节对齐）
 * @param[in] dest_address  目标地址（必须4字节对齐）
 * @param[in] byte_count    每次循环传输字节数（建议4字节倍数，1-262140）
 * @return true: 配置成功, false: 参数错误或通道已使能
 *
 * @note 覆盖寄存器：DMA_Cx_SRC_ADDR, DMA_Cx_DEST_ADDR, DMA_Cx_LLI, DMA_Cx_CTRL, DMA_Cx_CONFIG
 * @note 内部使用静态分配的链表节点，调用此API后不需要手动管理节点内存
 */
bool dma_init_circular(
    dma_instance_t instance,
    dma_channel_t channel,
    uint32_t src_address,
    uint32_t dest_address,
    uint32_t byte_count
);

//===========================================
// 第2层：基础配置API
//===========================================

/**
 * @brief 配置DMA全局参数（字节序和使能）
 *
 * @param[in] instance DMA实例
 * @param[in] config   全局配置结构体指针
 * @return true: 成功, false: 参数错误
 *
 * @note 覆盖寄存器：DMA_CONFIG.EN, M1ENDIAN, M2ENDIAN
 */
bool dma_config_global(
    dma_instance_t instance,
    const dma_global_config_t *config
);

/**
 * @brief 配置通道完整传输参数
 *
 * 配置通道所有寄存器，适用于非链表模式的一次性传输。
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @param[in] config   通道配置结构体指针
 * @return true: 成功, false: 参数错误、地址未对齐或通道已使能
 *
 * @note 覆盖寄存器：DMA_Cx_SRC_ADDR, DMA_Cx_DEST_ADDR, DMA_Cx_CTRL, DMA_Cx_CONFIG
 * @note 配置前通道必须处于禁用状态
 */
bool dma_config_channel(
    dma_instance_t instance,
    dma_channel_t channel,
    const dma_channel_config_t *config
);

/**
 * @brief 单独配置通道源地址和目标地址
 *
 * @param[in] instance     DMA实例
 * @param[in] channel      DMA通道
 * @param[in] src_address  源地址
 * @param[in] dest_address 目标地址
 * @return true: 成功, false: 参数错误或通道已使能
 *
 * @note 覆盖寄存器：DMA_Cx_SRC_ADDR, DMA_Cx_DEST_ADDR
 */
bool dma_config_address(
    dma_instance_t instance,
    dma_channel_t channel,
    uint32_t src_address,
    uint32_t dest_address
);

/**
 * @brief 单独配置通道传输次数
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @param[in] size     传输次数（1-65535）
 * @return true: 成功, false: 参数错误或通道已使能
 *
 * @note 覆盖寄存器：DMA_Cx_CTRL.TRANSFERSIZE
 */
bool dma_config_transfer_size(
    dma_instance_t instance,
    dma_channel_t channel,
    uint16_t size
);

/**
 * @brief 单独配置通道中断使能
 *
 * @param[in] instance     DMA实例
 * @param[in] channel      DMA通道
 * @param[in] tc_enable    传输完成中断使能
 * @param[in] ht_enable    半传输完成中断使能
 * @param[in] error_enable 传输错误中断使能
 * @return true: 成功, false: 参数错误
 *
 * @note 覆盖寄存器：DMA_Cx_CTRL.ITC, DMA_Cx_CONFIG.ITC, IHFTC, IE
 */
bool dma_config_interrupt(
    dma_instance_t instance,
    dma_channel_t channel,
    bool tc_enable,
    bool ht_enable,
    bool error_enable
);

/**
 * @brief 复位通道到默认状态
 *
 * 停止通道、清除所有中断标志、清零所有通道寄存器和回调函数。
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @return true: 成功, false: 参数错误
 *
 * @note 覆盖寄存器：DMA_Cx_SRC_ADDR, DMA_Cx_DEST_ADDR, DMA_Cx_LLI, DMA_Cx_CTRL, DMA_Cx_CONFIG
 */
bool dma_reset_channel(
    dma_instance_t instance,
    dma_channel_t channel
);

//===========================================
// 第3层：高级功能API
//===========================================

/**
 * @brief 配置链表传输
 *
 * 将通道配置为链表模式。内部会根据circular标志自动将最后一个节点链接到第一个节点。
 * 配置完成后调用 dma_start_channel() 启动传输。
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @param[in] config   链表配置结构体指针
 * @return true: 成功, false: 参数错误或通道已使能
 *
 * @note 覆盖寄存器：DMA_Cx_LLI, DMA_Cx_CTRL, DMA_Cx_CONFIG
 */
bool dma_config_linked_list(
    dma_instance_t instance,
    dma_channel_t channel,
    const dma_linked_list_config_t *config
);

/**
 * @brief 初始化一个链表节点
 *
 * @param[out] node          链表节点指针（必须非NULL）
 * @param[in]  src_address   该节点的源地址
 * @param[in]  dest_address  该节点的目标地址
 * @param[in]  control       该节点的控制字（DMA_Cx_CTRL寄存器值，含TRANSFERSIZE和ITC等）
 * @param[in]  next          下一个节点的指针（NULL表示最后一个节点）
 * @param[in]  lli_master    下一个节点所在存储器使用的AHB主机
 *
 * @note 此函数不操作寄存器，仅初始化内存中的节点结构
 * @note control字中的ITC位（bit31）控制该节点传输完成时是否产生原始中断标志
 */
void dma_init_lli_node(
    dma_lli_node_t *node,
    uint32_t src_address,
    uint32_t dest_address,
    uint32_t control,
    const dma_lli_node_t *next,
    dma_ahb_master_t lli_master
);

/**
 * @brief 链接两个链表节点
 *
 * @param[in] current    当前节点指针（必须非NULL）
 * @param[in] next       下一个节点指针（NULL表示结束）
 * @param[in] lli_master 下一个节点所在存储器使用的AHB主机
 *
 * @note 此函数不操作寄存器，仅修改current->next_lli字段
 */
void dma_link_nodes(
    dma_lli_node_t *current,
    const dma_lli_node_t *next,
    dma_ahb_master_t lli_master
);

/**
 * @brief 单独配置通道突发传输大小
 *
 * @param[in] instance   DMA实例
 * @param[in] channel    DMA通道
 * @param[in] src_burst  源突发大小
 * @param[in] dest_burst 目标突发大小
 * @return true: 成功, false: 参数错误或通道已使能
 *
 * @note 覆盖寄存器：DMA_Cx_CTRL.SBSIZE, DBSIZE
 */
bool dma_config_burst(
    dma_instance_t instance,
    dma_channel_t channel,
    dma_burst_size_t src_burst,
    dma_burst_size_t dest_burst
);

/**
 * @brief 单独配置通道数据位宽
 *
 * @param[in] instance   DMA实例
 * @param[in] channel    DMA通道
 * @param[in] src_width  源数据位宽
 * @param[in] dest_width 目标数据位宽
 * @return true: 成功, false: 参数错误或通道已使能
 *
 * @note 覆盖寄存器：DMA_Cx_CTRL.SWIDTH, DWIDTH
 * @note 源和目标位宽可不同，硬件自动完成数据封装/解封
 */
bool dma_config_data_width(
    dma_instance_t instance,
    dma_channel_t channel,
    dma_data_width_t src_width,
    dma_data_width_t dest_width
);

/**
 * @brief 单独配置通道地址变化模式
 *
 * @param[in] instance  DMA实例
 * @param[in] channel   DMA通道
 * @param[in] src_mode  源地址变化模式
 * @param[in] dest_mode 目标地址变化模式
 * @return true: 成功, false: 参数错误或通道已使能
 *
 * @note 覆盖寄存器：DMA_Cx_CTRL.SIORSD, DIORDD
 */
bool dma_config_address_mode(
    dma_instance_t instance,
    dma_channel_t channel,
    dma_addr_mode_t src_mode,
    dma_addr_mode_t dest_mode
);

/**
 * @brief 配置DMA全局字节序模式
 *
 * @param[in] instance       DMA实例
 * @param[in] master1_endian AHB主机1字节序
 * @param[in] master2_endian AHB主机2字节序
 * @return true: 成功, false: 参数错误
 *
 * @note 覆盖寄存器：DMA_CONFIG.M1ENDIAN, M2ENDIAN
 */
bool dma_config_endian(
    dma_instance_t instance,
    dma_endian_t master1_endian,
    dma_endian_t master2_endian
);

/**
 * @brief 单独配置通道外设请求ID
 *
 * @param[in] instance      DMA实例
 * @param[in] channel       DMA通道
 * @param[in] src_periph_id 源外设请求ID（0-255）
 * @param[in] dest_periph_id 目标外设请求ID（0-255）
 * @return true: 成功, false: 参数错误或通道已使能
 *
 * @note 覆盖寄存器：DMA_Cx_CONFIG.SRCID, DESTID
 */
bool dma_config_periph_request(
    dma_instance_t instance,
    dma_channel_t channel,
    uint8_t src_periph_id,
    uint8_t dest_periph_id
);

/**
 * @brief 单独配置通道传输方向和流控制
 *
 * @param[in] instance  DMA实例
 * @param[in] channel   DMA通道
 * @param[in] direction 传输方向
 * @return true: 成功, false: 参数错误或通道已使能
 *
 * @note 覆盖寄存器：DMA_Cx_CONFIG.FLOWCTRL
 */
bool dma_config_flow_control(
    dma_instance_t instance,
    dma_channel_t channel,
    dma_direction_t direction
);

/**
 * @brief 单独配置通道AHB主机选择
 *
 * @param[in] instance    DMA实例
 * @param[in] channel     DMA通道
 * @param[in] src_master  源AHB主机
 * @param[in] dest_master 目标AHB主机
 * @return true: 成功, false: 参数错误或通道已使能
 *
 * @note 覆盖寄存器：DMA_Cx_CONFIG.SMST, DMST
 */
bool dma_config_ahb_master(
    dma_instance_t instance,
    dma_channel_t channel,
    dma_ahb_master_t src_master,
    dma_ahb_master_t dest_master
);

/**
 * @brief 单独配置通道AHB总线锁定
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @param[in] enable   true=锁定总线, false=不锁定
 * @return true: 成功, false: 参数错误或通道已使能
 *
 * @note 覆盖寄存器：DMA_Cx_CONFIG.LOCK
 */
bool dma_config_bus_lock(
    dma_instance_t instance,
    dma_channel_t channel,
    bool enable
);

/**
 * @brief 配置双缓冲传输（自动构建双节点链表）
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @param[in] config   双缓冲配置结构体指针
 * @return true: 成功, false: 参数错误或通道已使能
 *
 * @note 覆盖寄存器：DMA_Cx_CTRL, DMA_Cx_CONFIG
 * @note 内部使用静态分配的链表节点，不需要手动管理节点内存
 */
bool dma_config_double_buffer(
    dma_instance_t instance,
    dma_channel_t channel,
    const dma_double_buffer_config_t *config
);

/**
 * @brief 获取通道运行状态
 *
 * @param[in]  instance DMA实例
 * @param[in]  channel  DMA通道
 * @param[out] status   通道状态结构体指针
 * @return true: 成功, false: 参数错误
 *
 * @note 查询寄存器：DMA_EN_CH_STATUS, DMA_Cx_CONFIG, DMA_Cx_CTRL,
 *                   DMA_Cx_SRC_ADDR, DMA_Cx_DEST_ADDR
 */
bool dma_get_channel_status(
    dma_instance_t instance,
    dma_channel_t channel,
    dma_channel_status_t *status
);

/**
 * @brief 获取通道剩余传输次数
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @return 剩余传输次数（注意：由于硬件持续更新，返回值仅供参考）
 *
 * @note 查询寄存器：DMA_Cx_CTRL.TRANSFERSIZE
 */
uint16_t dma_get_transfer_progress(
    dma_instance_t instance,
    dma_channel_t channel
);

/**
 * @brief 检查通道FIFO是否有数据（通道是否活跃）
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @return true: FIFO有数据（活跃）, false: FIFO为空
 *
 * @note 查询寄存器：DMA_Cx_CONFIG.ACTIVE
 */
bool dma_is_channel_busy(
    dma_instance_t instance,
    dma_channel_t channel
);

/**
 * @brief 等待传输完成（轮询模式）
 *
 * 轮询RAW中断状态寄存器等待传输完成，自动清除完成标志。
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @param[in] timeout  超时计数（0=无限等待，建议根据传输量设置合适值）
 * @return true: 传输完成, false: 超时或传输错误
 *
 * @note 查询寄存器：DMA_RAW_INT_TC_STATUS, DMA_RAW_INT_ERR_STATUS
 * @note tc_interrupt_enable必须在配置时使能（CTRL.ITC=1），否则无法产生TC状态标志
 */
bool dma_wait_transfer_complete(
    dma_instance_t instance,
    dma_channel_t channel,
    uint32_t timeout
);

//===========================================
// 第4层：控制与查询API
//===========================================

// --- 通道控制 ---

/**
 * @brief 使能DMA控制器（自动管理外设时钟）
 *
 * @param[in] instance DMA实例
 *
 * @note 覆盖寄存器：DMA_CONFIG.EN
 * @note 自动调用clock_periph_enable()使能DMA时钟
 */
void dma_enable(dma_instance_t instance);

/**
 * @brief 禁用DMA控制器（自动管理外设时钟）
 *
 * @param[in] instance DMA实例
 *
 * @note 覆盖寄存器：DMA_CONFIG.EN
 * @note 禁用前请确保所有通道已停止
 */
void dma_disable(dma_instance_t instance);

/**
 * @brief 启动DMA通道传输
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 *
 * @note 覆盖寄存器：DMA_Cx_CONFIG.EN
 */
void dma_start_channel(dma_instance_t instance, dma_channel_t channel);

/**
 * @brief 立即停止DMA通道传输（可能丢失FIFO数据）
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 *
 * @note 覆盖寄存器：DMA_Cx_CONFIG.EN
 * @warning 此操作会立即停止传输，FIFO中的数据将丢失
 */
void dma_stop_channel(dma_instance_t instance, dma_channel_t channel);

/**
 * @brief 暂停DMA通道传输（保留FIFO数据）
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 *
 * @note 覆盖寄存器：DMA_Cx_CONFIG.HALT
 * @note 暂停后仲裁器忽略该通道的后续请求，FIFO数据保留
 */
void dma_pause_channel(dma_instance_t instance, dma_channel_t channel);

/**
 * @brief 恢复DMA通道传输
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 *
 * @note 覆盖寄存器：DMA_Cx_CONFIG.HALT
 */
void dma_resume_channel(dma_instance_t instance, dma_channel_t channel);

/**
 * @brief 安全停止DMA通道（等待FIFO清空后停止，不丢失数据）
 *
 * 步骤：暂停 → 等待ACTIVE=0（FIFO清空）→ 禁用通道。
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @param[in] timeout  超时计数（0=无限等待）
 * @return true: 成功, false: 超时
 *
 * @note 覆盖寄存器：DMA_Cx_CONFIG.HALT, EN, ACTIVE
 */
bool dma_abort_channel(
    dma_instance_t instance,
    dma_channel_t channel,
    uint32_t timeout
);

// --- 地址读写 ---

/**
 * @brief 设置源地址（通道禁用时可用）
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @param[in] address  源地址
 *
 * @note 覆盖寄存器：DMA_Cx_SRC_ADDR
 */
void dma_set_source_address(
    dma_instance_t instance,
    dma_channel_t channel,
    uint32_t address
);

/**
 * @brief 设置目标地址（通道禁用时可用）
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @param[in] address  目标地址
 *
 * @note 覆盖寄存器：DMA_Cx_DEST_ADDR
 */
void dma_set_destination_address(
    dma_instance_t instance,
    dma_channel_t channel,
    uint32_t address
);

/**
 * @brief 读取当前源地址
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @return 当前源地址
 *
 * @note 查询寄存器：DMA_Cx_SRC_ADDR
 */
uint32_t dma_get_source_address(
    dma_instance_t instance,
    dma_channel_t channel
);

/**
 * @brief 读取当前目标地址
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @return 当前目标地址
 *
 * @note 查询寄存器：DMA_Cx_DEST_ADDR
 */
uint32_t dma_get_destination_address(
    dma_instance_t instance,
    dma_channel_t channel
);

// --- 中断使能控制 ---

/**
 * @brief 使能DMA通道中断（可组合）
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @param[in] irq_type 中断类型（DMA_IRQ_TC | DMA_IRQ_HT | DMA_IRQ_ERROR）
 *
 * @note 覆盖寄存器：DMA_Cx_CONFIG.ITC, IHFTC, IE（CTRL.ITC 需启动前配置）
 */
void dma_enable_interrupt(
    dma_instance_t instance,
    dma_channel_t channel,
    dma_irq_type_t irq_type
);

/**
 * @brief 禁用DMA通道中断（可组合）
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @param[in] irq_type 中断类型
 *
 * @note 覆盖寄存器：DMA_Cx_CONFIG.ITC, IHFTC, IE
 */
void dma_disable_interrupt(
    dma_instance_t instance,
    dma_channel_t channel,
    dma_irq_type_t irq_type
);

/**
 * @brief 使能传输完成中断
 *
 * @note 覆盖寄存器：DMA_Cx_CONFIG.ITC（CTRL.ITC 需启动前配置）
 */
void dma_enable_tc_interrupt(dma_instance_t instance, dma_channel_t channel);

/**
 * @brief 禁用传输完成中断
 *
 * @note 覆盖寄存器：DMA_Cx_CONFIG.ITC
 */
void dma_disable_tc_interrupt(dma_instance_t instance, dma_channel_t channel);

/**
 * @brief 使能半传输完成中断
 *
 * @note 覆盖寄存器：DMA_Cx_CONFIG.IHFTC（CTRL.ITC 需启动前配置）
 */
void dma_enable_ht_interrupt(dma_instance_t instance, dma_channel_t channel);

/**
 * @brief 禁用半传输完成中断
 *
 * @note 覆盖寄存器：DMA_Cx_CONFIG.IHFTC
 */
void dma_disable_ht_interrupt(dma_instance_t instance, dma_channel_t channel);

/**
 * @brief 使能传输错误中断
 *
 * @note 覆盖寄存器：DMA_Cx_CONFIG.IE
 */
void dma_enable_error_interrupt(dma_instance_t instance, dma_channel_t channel);

/**
 * @brief 禁用传输错误中断
 *
 * @note 覆盖寄存器：DMA_Cx_CONFIG.IE
 */
void dma_disable_error_interrupt(dma_instance_t instance, dma_channel_t channel);

// --- 中断状态查询 ---

/**
 * @brief 获取DMA全部通道中断状态（屏蔽后）
 *
 * @param[in] instance DMA实例
 * @return 中断状态位图，bit[N]对应通道N
 *
 * @note 查询寄存器：DMA_INT_STATUS
 */
uint32_t dma_get_interrupt_status(dma_instance_t instance);

/**
 * @brief 获取指定通道中断状态（屏蔽后，可区分TC/HT/ERROR）
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @return 中断类型位图（DMA_IRQ_TC | DMA_IRQ_HT | DMA_IRQ_ERROR的组合）
 *
 * @note 查询寄存器：DMA_INT_TC_STATUS, DMA_INT_ERR_STATUS
 */
uint32_t dma_get_channel_interrupt_status(
    dma_instance_t instance,
    dma_channel_t channel
);

/**
 * @brief 获取指定通道原始中断状态（不受中断使能影响）
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @return 中断类型位图（DMA_IRQ_TC | DMA_IRQ_HT | DMA_IRQ_ERROR的组合）
 *
 * @note 查询寄存器：DMA_RAW_INT_TC_STATUS, DMA_RAW_INT_ERR_STATUS
 */
uint32_t dma_get_raw_interrupt_status(
    dma_instance_t instance,
    dma_channel_t channel
);

/**
 * @brief 获取全部通道传输完成中断状态（屏蔽后）
 *
 * @param[in] instance DMA实例
 * @return 传输完成状态位图，bit[N]对应通道N
 *
 * @note 查询寄存器：DMA_INT_TC_STATUS bit[7:0]
 */
uint32_t dma_get_tc_status(dma_instance_t instance);

/**
 * @brief 获取全部通道半传输完成中断状态（屏蔽后）
 *
 * @param[in] instance DMA实例
 * @return 半传输完成状态位图，bit[N]对应通道N
 *
 * @note 查询寄存器：DMA_INT_TC_STATUS bit[15:8]
 */
uint32_t dma_get_ht_status(dma_instance_t instance);

/**
 * @brief 获取全部通道传输错误中断状态（屏蔽后）
 *
 * @param[in] instance DMA实例
 * @return 传输错误状态位图，bit[N]对应通道N
 *
 * @note 查询寄存器：DMA_INT_ERR_STATUS
 */
uint32_t dma_get_error_status(dma_instance_t instance);

/**
 * @brief 检查指定通道传输完成标志（原始，不受中断使能影响）
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @return true: 传输完成
 *
 * @note 查询寄存器：DMA_RAW_INT_TC_STATUS
 */
bool dma_is_transfer_complete(dma_instance_t instance, dma_channel_t channel);

/**
 * @brief 检查指定通道半传输完成标志（原始，不受中断使能影响）
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @return true: 半传输完成
 *
 * @note 查询寄存器：DMA_RAW_INT_TC_STATUS bit[15:8]
 */
bool dma_is_half_transfer_complete(dma_instance_t instance, dma_channel_t channel);

/**
 * @brief 检查指定通道传输错误标志（原始，不受中断使能影响）
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @return true: 有传输错误
 *
 * @note 查询寄存器：DMA_RAW_INT_ERR_STATUS
 */
bool dma_has_error(dma_instance_t instance, dma_channel_t channel);

// --- 中断标志清除 ---

/**
 * @brief 清除指定通道指定类型的中断标志
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @param[in] irq_type 要清除的中断类型（可组合）
 *
 * @note 覆盖寄存器：DMA_INT_TC_CLR, DMA_INT_ERR_CLR
 */
void dma_clear_interrupt_flag(
    dma_instance_t instance,
    dma_channel_t channel,
    dma_irq_type_t irq_type
);

/**
 * @brief 清除传输完成中断标志
 *
 * @note 覆盖寄存器：DMA_INT_TC_CLR
 */
void dma_clear_tc_flag(dma_instance_t instance, dma_channel_t channel);

/**
 * @brief 清除半传输完成中断标志
 *
 * @note 覆盖寄存器：DMA_INT_TC_CLR
 */
void dma_clear_ht_flag(dma_instance_t instance, dma_channel_t channel);

/**
 * @brief 清除传输错误中断标志
 *
 * @note 覆盖寄存器：DMA_INT_ERR_CLR
 */
void dma_clear_error_flag(dma_instance_t instance, dma_channel_t channel);

/**
 * @brief 清除指定通道所有中断标志（TC、HT、ERROR）
 *
 * @note 覆盖寄存器：DMA_INT_TC_CLR, DMA_INT_ERR_CLR
 */
void dma_clear_channel_all_flags(dma_instance_t instance, dma_channel_t channel);

/**
 * @brief 清除DMA所有通道的所有中断标志
 *
 * @note 覆盖寄存器：DMA_INT_TC_CLR, DMA_INT_ERR_CLR
 */
void dma_clear_all_interrupt_flags(dma_instance_t instance);

// --- 回调函数注册 ---

/**
 * @brief 注册通道回调函数集合
 *
 * @param[in] instance  DMA实例
 * @param[in] channel   DMA通道
 * @param[in] callbacks 回调函数集合指针（NULL成员表示不使用该回调）
 * @return true: 成功, false: 参数错误
 *
 * @note 覆盖寄存器：不操作寄存器，仅注册中断回调函数
 * @note 回调函数在中断服务程序中调用，应尽量简短
 */
bool dma_register_callback(
    dma_instance_t instance,
    dma_channel_t channel,
    const dma_callbacks_t *callbacks
);

/**
 * @brief 注销通道所有回调函数
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 *
 * @note 覆盖寄存器：不操作寄存器，仅清除回调函数
 */
void dma_unregister_callback(dma_instance_t instance, dma_channel_t channel);

/**
 * @brief 注册传输完成回调函数
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @param[in] callback 传输完成回调（NULL=注销）
 *
 * @note 覆盖寄存器：不操作寄存器，仅注册回调函数
 */
void dma_register_tc_callback(
    dma_instance_t instance,
    dma_channel_t channel,
    dma_tc_callback_t callback
);

/**
 * @brief 注册半传输完成回调函数
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @param[in] callback 半传输完成回调（NULL=注销）
 *
 * @note 覆盖寄存器：不操作寄存器，仅注册回调函数
 */
void dma_register_ht_callback(
    dma_instance_t instance,
    dma_channel_t channel,
    dma_ht_callback_t callback
);

/**
 * @brief 注册传输错误回调函数
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @param[in] callback 传输错误回调（NULL=注销）
 *
 * @note 覆盖寄存器：不操作寄存器，仅注册回调函数
 */
void dma_register_error_callback(
    dma_instance_t instance,
    dma_channel_t channel,
    dma_error_callback_t callback
);

// --- 通道状态查询 ---

/**
 * @brief 获取全部通道使能状态位图
 *
 * @param[in] instance DMA实例
 * @return 使能状态位图，bit[N]=1表示通道N已使能
 *
 * @note 查询寄存器：DMA_EN_CH_STATUS
 */
uint32_t dma_get_enabled_channels(dma_instance_t instance);

/**
 * @brief 检查指定通道是否已使能
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @return true: 已使能
 *
 * @note 查询寄存器：DMA_EN_CH_STATUS
 */
bool dma_is_channel_enabled(dma_instance_t instance, dma_channel_t channel);

/**
 * @brief 检查指定通道是否已暂停
 *
 * @param[in] instance DMA实例
 * @param[in] channel  DMA通道
 * @return true: 已暂停（HALT=1）
 *
 * @note 查询寄存器：DMA_Cx_CONFIG.HALT
 */
bool dma_is_channel_halted(dma_instance_t instance, dma_channel_t channel);

#ifdef __cplusplus
}
#endif

#endif // _HARDWARE_DMA_H
