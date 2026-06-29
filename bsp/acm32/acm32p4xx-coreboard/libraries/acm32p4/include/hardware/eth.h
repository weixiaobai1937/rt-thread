/**
 * @file eth.h
 * @brief ACM32P4 以太网 MAC 控制器（ETH）驱动头文件
 *
 * 本文件定义了 ETH 驱动的公开 API，支持以下功能：
 * - MAC 参数完整配置（速率、双工、帧过滤、流控、VLAN）
 * - DMA 总线模式配置（突发长度、增强描述符、仲裁比）
 * - PHY SMI 接口读写（MDIO/MDC，支持32个PHY）
 * - 帧发送与接收（DMA 描述符链，普通/增强模式）
 * - PTP 精确时间协议（IEEE 1588-2008，粗/精校准）
 * - LPI（EEE，802.3az）节能控制
 * - L3/L4 过滤器配置（IPv4/IPv6 + TCP/UDP 端口过滤）
 * - MAC 地址过滤（4组完美过滤，含 SA/DA 掩码支持）
 * - MMC 统计计数器读取
 * - 双中断向量管理（DMA 中断 + 唤醒/PMT 中断）
 *
 * @note 中断架构：
 *   - ETH_IRQHandler（IRQn=45）：DMA发送/接收完成、总线错误、早期接收/发送、
 *     接收缓冲不可用；MAC子中断（MMC、PTP时间戳、LPI、PMT）
 *   - ETH_WKUP_IRQHandler（IRQn=46）：网络唤醒帧、Magic Packet
 *   用户通过 eth_irq_register() 注册回调，区分两条向量。
 *
 * @note PHY 接口通过 SYSCFG EPIS 位在 MII（25 MHz）和 RMII（50 MHz）间选择，
 *   须在以太网时钟使能前完成配置。
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#ifndef _HARDWARE_ETH_H
#define _HARDWARE_ETH_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hardware/syscfg.h"

#ifdef __cplusplus
extern "C" {
#endif

//===========================================
// 宏定义
//===========================================

/** @brief MAC 地址过滤器数量（MACA0~3，共4组） */
#define ETH_MAC_ADDR_COUNT          4U

/** @brief L3/L4 过滤器数量（Filter0 和 Filter1） */
#define ETH_L3L4_FILTER_COUNT       2U

/** @brief TX/RX DMA 描述符字数（增强模式 8×32-bit） */
#define ETH_DESC_WORD_COUNT         8U

//===========================================
// 枚举类型定义
//===========================================

/**
 * @brief MAC 速率选择（对应 MACCR.FES 位）
 */
typedef enum {
    ETH_SPEED_10M  = 0,  ///< 10 Mbps（MACCR.FES=0）
    ETH_SPEED_100M = 1,  ///< 100 Mbps（MACCR.FES=1）
} eth_speed_t;

/**
 * @brief 双工模式（对应 MACCR.DM 位）
 */
typedef enum {
    ETH_DUPLEX_HALF = 0,  ///< 半双工（MACCR.DM=0）
    ETH_DUPLEX_FULL = 1,  ///< 全双工（MACCR.DM=1）
} eth_duplex_t;

/**
 * @brief IP 校验和卸载配置（对应 MACCR.IPCO 位）
 */
typedef enum {
    ETH_CHECKSUM_NONE    = 0,  ///< 禁用校验和卸载
    ETH_CHECKSUM_OFFLOAD = 1,  ///< 使能 IPv4/TCP/UDP/ICMP 校验和卸载（需增强描述符）
} eth_checksum_t;

/**
 * @brief MAC 帧间间隔（对应 MACCR.IFG[2:0]，以 8 bit 为步长）
 */
typedef enum {
    ETH_IFG_96BIT = 0,  ///< 96 bit（默认，IEEE 802.3）
    ETH_IFG_88BIT = 1,  ///< 88 bit
    ETH_IFG_80BIT = 2,  ///< 80 bit
    ETH_IFG_72BIT = 3,  ///< 72 bit
    ETH_IFG_64BIT = 4,  ///< 64 bit
    ETH_IFG_56BIT = 5,  ///< 56 bit
    ETH_IFG_48BIT = 6,  ///< 48 bit
    ETH_IFG_40BIT = 7,  ///< 40 bit（最小值）
} eth_ifg_t;

/**
 * @brief 背退限制（对应 MACCR.BL[1:0]，半双工冲突重传）
 */
typedef enum {
    ETH_BACKOFF_LIMIT_10 = 0,  ///< k = min(n, 10)（默认）
    ETH_BACKOFF_LIMIT_8  = 1,  ///< k = min(n, 8)
    ETH_BACKOFF_LIMIT_4  = 2,  ///< k = min(n, 4)
    ETH_BACKOFF_LIMIT_1  = 3,  ///< k = min(n, 1)
} eth_backoff_limit_t;

/**
 * @brief 报头前导码长度（对应 MACCR.PRELEN[1:0]）
 */
typedef enum {
    ETH_PREAMBLE_7BYTE = 0,  ///< 7 字节前导码（默认）
    ETH_PREAMBLE_5BYTE = 1,  ///< 5 字节前导码
    ETH_PREAMBLE_3BYTE = 2,  ///< 3 字节前导码
} eth_preamble_t;

/**
 * @brief 源地址插入/替换控制（对应 MACCR.SAIRC[2:0]）
 */
typedef enum {
    ETH_SA_INSERT_NONE    = 0,  ///< 不修改源地址
    ETH_SA_INSERT_ADDR0   = 2,  ///< 插入 MAC 地址0
    ETH_SA_REPLACE_ADDR0  = 3,  ///< 替换为 MAC 地址0
    ETH_SA_INSERT_ADDR1   = 6,  ///< 插入 MAC 地址1
    ETH_SA_REPLACE_ADDR1  = 7,  ///< 替换为 MAC 地址1
} eth_sa_insert_t;

/**
 * @brief 控制帧传递策略（对应 MACFFR.PCF[1:0]）
 */
typedef enum {
    ETH_PCF_BLOCK_ALL      = 0,  ///< 阻止所有控制帧
    ETH_PCF_FORWARD_PAUSE  = 1,  ///< 转发已通过地址过滤的控制帧（除暂停帧）
    ETH_PCF_FORWARD_ALL    = 2,  ///< 转发所有控制帧（包括暂停帧）
    ETH_PCF_FILTER_ADDR    = 3,  ///< 转发通过地址过滤的控制帧并对其进行过滤
} eth_pcf_t;

/**
 * @brief DMA 任意/固定突发（对应 DMABMR.FB/MB）
 */
typedef enum {
    ETH_DMA_BURST_UNDEFINED = 0,  ///< 不定长度突发（INCR）
    ETH_DMA_BURST_FIXED     = 1,  ///< 固定长度突发（FIXED，由 PBL 决定）
    ETH_DMA_BURST_MIXED     = 2,  ///< 混合突发（MB=1，允许超过4字节边界）
} eth_dma_burst_t;

/**
 * @brief DMA RX/TX 优先级仲裁比（对应 DMABMR.PM[1:0]）
 */
typedef enum {
    ETH_DMA_PRIORITY_ROUND_ROBIN = 0,  ///< 循环调度（Rx:Tx = 1:1）
    ETH_DMA_PRIORITY_RX_2_TX_1  = 1,  ///< Rx 优先 2:1
    ETH_DMA_PRIORITY_RX_3_TX_1  = 2,  ///< Rx 优先 3:1
    ETH_DMA_PRIORITY_RX_4_TX_1  = 3,  ///< Rx 优先 4:1
    ETH_DMA_PRIORITY_TX_PRIOR    = 4,  ///< TX 优先（DMABMR.TXPR=1）
} eth_dma_priority_t;

/**
 * @brief DMA 接收阈值（对应 DMAOMR.RTC[1:0]）
 */
typedef enum {
    ETH_RX_THRESHOLD_64  = 0,  ///< 64 字节（默认）
    ETH_RX_THRESHOLD_32  = 1,  ///< 32 字节
    ETH_RX_THRESHOLD_96  = 2,  ///< 96 字节
    ETH_RX_THRESHOLD_128 = 3,  ///< 128 字节
} eth_rx_threshold_t;

/**
 * @brief DMA 发送阈值（对应 DMAOMR.TTC[2:0]）
 */
typedef enum {
    ETH_TX_THRESHOLD_64  = 0,  ///< 64 字节（默认）
    ETH_TX_THRESHOLD_128 = 1,  ///< 128 字节
    ETH_TX_THRESHOLD_192 = 2,  ///< 192 字节
    ETH_TX_THRESHOLD_256 = 3,  ///< 256 字节
    ETH_TX_THRESHOLD_40  = 4,  ///< 40 字节
    ETH_TX_THRESHOLD_32  = 5,  ///< 32 字节
    ETH_TX_THRESHOLD_24  = 6,  ///< 24 字节
    ETH_TX_THRESHOLD_16  = 7,  ///< 16 字节
} eth_tx_threshold_t;

/**
 * @brief SMI 时钟分频档位（对应 MACMIIAR.CR[3:0]）
 *
 * 依据 HCLK 频率范围自动选择最接近 2.5 MHz 及以下的 MDC 时钟。
 */
typedef enum {
    ETH_SMI_CR_60_100MHZ  = 0,  ///< HCLK 60~100 MHz，MDC = HCLK/42
    ETH_SMI_CR_100_150MHZ = 1,  ///< HCLK 100~150 MHz，MDC = HCLK/62
    ETH_SMI_CR_20_35MHZ   = 2,  ///< HCLK 20~35 MHz，MDC = HCLK/16
    ETH_SMI_CR_35_60MHZ   = 3,  ///< HCLK 35~60 MHz，MDC = HCLK/26
    ETH_SMI_CR_150_250MHZ = 4,  ///< HCLK 150~250 MHz，MDC = HCLK/102
    ETH_SMI_CR_250_300MHZ = 5,  ///< HCLK 250~300 MHz，MDC = HCLK/124
} eth_smi_cr_t;

/**
 * @brief 流控暂停低阈值（对应 MACFCR.PLT[1:0]）
 */
typedef enum {
    ETH_PAUSE_THRESHOLD_MINUS_4  = 0,  ///< PT - 4 个间隔（默认）
    ETH_PAUSE_THRESHOLD_MINUS_28 = 1,  ///< PT - 28 个间隔
    ETH_PAUSE_THRESHOLD_MINUS_144= 2,  ///< PT - 144 个间隔
    ETH_PAUSE_THRESHOLD_MINUS_256= 3,  ///< PT - 256 个间隔
} eth_pause_threshold_t;

/**
 * @brief PTP 时钟节点类型（对应 PTPTSCR[8:6]）
 */
typedef enum {
    ETH_PTP_CLOCK_ORDINARY_SLAVE  = 0,  ///< 普通从节点
    ETH_PTP_CLOCK_BOUNDARY_MASTER = 1,  ///< 边界主节点
    ETH_PTP_CLOCK_E2E_TRANSPARENT = 2,  ///< 端到端透明时钟
    ETH_PTP_CLOCK_P2P_TRANSPARENT = 3,  ///< 对等透明时钟
} eth_ptp_clock_type_t;

/**
 * @brief DMA 状态标志（对应 DMASR 寄存器各位，可 OR 组合）
 */
typedef enum {
    ETH_DMA_FLAG_TS    = (1U << 0),   ///< 发送中断（帧发送完成）
    ETH_DMA_FLAG_TPSS  = (1U << 1),   ///< 发送进程停止
    ETH_DMA_FLAG_TBUS  = (1U << 2),   ///< 发送缓冲区不可用
    ETH_DMA_FLAG_TJTS  = (1U << 3),   ///< 发送 Jabber 超时
    ETH_DMA_FLAG_ROS   = (1U << 4),   ///< 接收溢出
    ETH_DMA_FLAG_TUS   = (1U << 5),   ///< 发送下溢
    ETH_DMA_FLAG_RS    = (1U << 6),   ///< 接收中断（帧接收完成）
    ETH_DMA_FLAG_RBUS  = (1U << 7),   ///< 接收缓冲区不可用
    ETH_DMA_FLAG_RPSS  = (1U << 8),   ///< 接收进程停止
    ETH_DMA_FLAG_RWTS  = (1U << 9),   ///< 接收看门狗超时
    ETH_DMA_FLAG_ETS   = (1U << 10),  ///< 早期发送中断
    ETH_DMA_FLAG_FBES  = (1U << 13),  ///< 总线错误（致命）
    ETH_DMA_FLAG_ERS   = (1U << 14),  ///< 早期接收中断
    ETH_DMA_FLAG_AIS   = (1U << 15),  ///< 异常中断汇总
    ETH_DMA_FLAG_NIS   = (1U << 16),  ///< 正常中断汇总
    ETH_DMA_FLAG_MMCS  = (1U << 27),  ///< MMC 状态（来自 MACISR）
    ETH_DMA_FLAG_PMTS  = (1U << 28),  ///< PMT 状态（来自 MACISR）
    ETH_DMA_FLAG_TSTS  = (1U << 29),  ///< 时间戳触发状态
    ETH_DMA_FLAG_LPIS  = (1U << 30),  ///< LPI 状态
} eth_dma_flag_t;

/**
 * @brief DMA 中断使能位（对应 DMAIER 寄存器各位，可 OR 组合）
 */
typedef enum {
    ETH_DMA_IT_TIE   = (1U << 0),   ///< 发送中断使能
    ETH_DMA_IT_TPSIE = (1U << 1),   ///< 发送进程停止中断使能
    ETH_DMA_IT_TBUIE = (1U << 2),   ///< 发送缓冲区不可用中断使能
    ETH_DMA_IT_TJTIE = (1U << 3),   ///< 发送 Jabber 超时中断使能
    ETH_DMA_IT_ROIE  = (1U << 4),   ///< 溢出中断使能
    ETH_DMA_IT_TUIE  = (1U << 5),   ///< 下溢中断使能
    ETH_DMA_IT_RIE   = (1U << 6),   ///< 接收中断使能
    ETH_DMA_IT_RBUIE = (1U << 7),   ///< 接收缓冲区不可用中断使能
    ETH_DMA_IT_RPSIE = (1U << 8),   ///< 接收进程停止中断使能
    ETH_DMA_IT_RWTIE = (1U << 9),   ///< 接收看门狗超时中断使能
    ETH_DMA_IT_ETIE  = (1U << 10),  ///< 早期发送中断使能
    ETH_DMA_IT_FBEIE = (1U << 13),  ///< 总线错误中断使能
    ETH_DMA_IT_ERIE  = (1U << 14),  ///< 早期接收中断使能
    ETH_DMA_IT_AISE  = (1U << 15),  ///< 异常中断汇总使能
    ETH_DMA_IT_NISE  = (1U << 16),  ///< 正常中断汇总使能
} eth_dma_it_t;

/**
 * @brief MAC 中断类型（对应 MACIMR 寄存器掩码位）
 */
typedef enum {
    ETH_MAC_IT_PMT = (1U << 3),   ///< PMT 中断掩码（PIM，写1屏蔽）
    ETH_MAC_IT_TS  = (1U << 9),   ///< 时间戳触发中断掩码（TIM）
    ETH_MAC_IT_LPI = (1U << 10),  ///< LPI 中断掩码（LPIIM）
} eth_mac_it_t;

/**
 * @brief MMC 接收中断标志（对应 MMCRIR 寄存器）
 */
typedef enum {
    ETH_MMC_RX_IT_RFCE  = (1U << 5),   ///< 接收帧 CRC 错误计数器达到半满/全满
    ETH_MMC_RX_IT_RFAE  = (1U << 6),   ///< 接收帧对齐错误计数器达到半满/全满
    ETH_MMC_RX_IT_RGUFC = (1U << 17),  ///< 接收单播好帧计数器达到半满/全满
} eth_mmc_rx_it_t;

/**
 * @brief MMC 发送中断标志（对应 MMCTIR 寄存器）
 */
typedef enum {
    ETH_MMC_TX_IT_TSCGFC = (1U << 14),  ///< 发送单次冲突好帧计数器达到半满/全满
    ETH_MMC_TX_IT_TMCGFC = (1U << 15),  ///< 发送多次冲突好帧计数器达到半满/全满
    ETH_MMC_TX_IT_TGF    = (1U << 21),  ///< 发送好帧计数器达到半满/全满
} eth_mmc_tx_it_t;

/**
 * @brief 中断向量类型（用于 eth_irq_register()）
 */
typedef enum {
    ETH_IRQ_DMA  = 0,  ///< ETH_IRQn（45），DMA 中断
    ETH_IRQ_WKUP = 1,  ///< ETH_WKUP_IRQn（46），唤醒/PMT 中断
} eth_irq_type_t;

//===========================================
// 结构体类型定义
//===========================================

/**
 * @brief MAC 配置结构体
 *
 * 用于 eth_config_mac()的配置参数。
 */
typedef struct {
    eth_speed_t       speed;               ///< 速率：10/100 Mbps（MACCR.FES）
    eth_duplex_t      duplex;              ///< 双工：全双工/半双工（MACCR.DM）
    eth_checksum_t    checksum_offload;    ///< IP 校验和卸载（MACCR.IPCO）
    eth_preamble_t    preamble_length;     ///< 前导码长度（MACCR.PRELEN）
    eth_ifg_t         inter_frame_gap;     ///< 帧间间隔（MACCR.IFG）
    eth_backoff_limit_t backoff_limit;     ///< 背退限制（MACCR.BL）
    eth_sa_insert_t   sa_insert;           ///< 源地址插入/替换控制（MACCR.SAIRC）
    bool              loopback;            ///< MII 内部回环（MACCR.LM）
    bool              jumbo_frame;         ///< 巨型帧使能（MACCR.JE，最大 16 KB）
    bool              watchdog_disable;    ///< 看门狗禁用（MACCR.WD，允许>2048字节帧接收）
    bool              jabber_disable;      ///< Jabber 禁用（MACCR.JD，允许>2048字节帧发送）
    bool              carrier_sense_disable;///< 载波侦听禁用（MACCR.CSD，仅半双工）
    bool              retry_disable;       ///< 禁止重传（MACCR.DR，仅半双工）
    bool              pad_crc_strip;       ///< 接收自动去除 PAD/CRC（MACCR.APCS）
    bool              deferral_check;      ///< 延迟检查（MACCR.DC，仅半双工）
    bool              crc_strip_type2;     ///< 对 Type2 帧去除 CRC（MACCR.CSTF）
    bool              s2kp_enable;         ///< IEEE 802.3as 2 KB 包支持（MACCR.S2KP）
    bool              rx_own_disable;      ///< 半双工接收自身单元禁用（MACCR.ROD）
} eth_mac_config_t;

/**
 * @brief DMA 总线模式配置结构体
 *
 * 用于 eth_config_dma()的配置参数。
 */
typedef struct {
    uint8_t           pbl;                 ///< 可编程突发长度（1/2/4/8/16/32，DMABMR.PBL）
    uint8_t           rx_dma_pbl;          ///< RX DMA 独立突发长度（use_separate_pbl=true 时生效，DMABMR.RDP）
    uint8_t           desc_skip_length;    ///< 描述符间跳空字数（DMABMR.DSL，0-31）
    eth_dma_priority_t priority;           ///< TX/RX 仲裁策略（DMABMR.PM + DMABMR.TXPR）
    eth_rx_threshold_t rx_threshold;       ///< 接收启动阈值（DMAOMR.RTC，SF模式下忽略）
    eth_tx_threshold_t tx_threshold;       ///< 发送启动阈值（DMAOMR.TTC，SF模式下忽略）
    bool               address_aligned;    ///< 地址对齐突发（DMABMR.AAB）
    bool               mixed_burst;        ///< 混合突发模式（DMABMR.MB）
    bool               fixed_burst;        ///< 固定突发模式（DMABMR.FB）
    bool               enhanced_desc;      ///< 增强描述符模式（DMABMR.EDFE，PTP或校验和卸载时必须）
    bool               use_separate_pbl;   ///< 为 RX DMA 使用独立 PBL（DMABMR.USP）
    bool               rx_store_forward;   ///< 接收存储转发（DMAOMR.RSF）
    bool               tx_store_forward;   ///< 发送存储转发（DMAOMR.TSF）
    bool               flush_rx_frame;     ///< 禁用接收帧冲洗（DMAOMR.DFRF）
    bool               fwd_error_frames;   ///< 转发错误帧（DMAOMR.FEF）
    bool               fwd_undersized_gf;  ///< 转发过小好帧（DMAOMR.FUGF）
    bool               drop_tcp_cs_err;    ///< 丢弃 TCP/IP 校验和错误帧（DMAOMR.DTCEFD）
    bool               operate_on_2nd_frame;///< 对第二帧直接操作（DMAOMR.OSF，提高总线利用率）
    bool               flush_tx_fifo;      ///< 冲洗发送 FIFO（DMAOMR.FTF，写后自动清零）
    bool               rib;                ///< 重建 IB（DMABMR.RIB，AHB 主接口总线连接修正）
    bool               epm;                ///< 使用 8 × PBL 模式（DMABMR.EPM）
} eth_dma_config_t;

/**
 * @brief 帧过滤配置结构体
 *
 * 用于 eth_config_filter()的配置参数。
 */
typedef struct {
    eth_pcf_t         pass_ctrl_frames;    ///< 控制帧传递策略（MACFFR.PCF）
    bool              receive_all;         ///< 接收所有帧，绕过过滤（MACFFR.RA）
    bool              hash_or_perfect;     ///< Hash 与完美过滤 OR（MACFFR.HPF）
    bool              sa_filter;           ///< 源地址过滤使能（MACFFR.SAF）
    bool              sa_inverse_filter;   ///< 源地址反向过滤（MACFFR.SAIF）
    bool              broadcast_disable;   ///< 禁止广播帧（MACFFR.DBF）
    bool              da_inverse_filter;   ///< 目的地址反向过滤（MACFFR.DAIF）
    bool              promiscuous;         ///< 混杂模式（MACFFR.PM）
    bool              hash_multicast;      ///< 多播哈希过滤（MACFFR.HMC）
    bool              hash_unicast;        ///< 单播哈希过滤（MACFFR.HU）
    bool              vlan_tag_filter;     ///< VLAN 标签过滤使能（MACFFR.VTFE）
    bool              ip_da_filter;        ///< IP 目的地址过滤（MACFFR.IPFE，需L3过滤器支持）
    bool              drop_non_tcp_udp;    ///< 丢弃非 TCP/UDP 帧（MACFFR.DNTU）
} eth_filter_config_t;

/**
 * @brief 流控配置结构体
 *
 * 用于 eth_config_flow_ctrl()的配置参数。
 */
typedef struct {
    eth_pause_threshold_t pause_low_threshold; ///< 暂停低阈值（MACFCR.PLT）
    uint16_t          pause_time;          ///< 暂停时间值（MACFCR.PT，0~65535）
    bool              tx_flow_ctrl;        ///< 发送流控使能（MACFCR.TFCE）
    bool              rx_flow_ctrl;        ///< 接收流控使能（MACFCR.RFCE）
    bool              unicast_pause_detect;///< 单播暂停帧检测（MACFCR.UPFD）
    bool              zero_quanta_pause;   ///< 发送零时间片暂停帧（MACFCR.FCB_BPA，全双工写1触发）
    bool              back_pressure;       ///< 背压激活（MACFCR.FCB_BPA，半双工写1触发）
} eth_flow_ctrl_config_t;

/**
 * @brief VLAN 标记配置结构体
 *
 * 用于 eth_config_vlan()的配置参数。
 */
typedef struct {
    uint16_t          vlan_id;             ///< VLAN 标签 ID（MACVLANTR.VL，12 或 16 位）
    uint16_t          vlan_hash_table;     ///< VLAN Hash 过滤表（MACVHTR，16 位）
    bool              vlan_12bit_compare;  ///< 仅比较 VLAN ID 低 12 位（MACVLANTR.ETV）
    bool              vlan_tag_invert;     ///< VLAN 标签反向过滤（MACVLANTR.VTIM）
    bool              svlan_enable;        ///< S-VLAN（802.1ad）使能（MACVLANTR.ESVL）
    bool              vlan_hash_filter;    ///< VLAN Hash 过滤使能（MACVLANTR.VTHM）
} eth_vlan_config_t;

/**
 * @brief PTP 时间戳配置结构体
 *
 * 用于 eth_config_ptp()的配置参数。
 */
typedef struct {
    eth_ptp_clock_type_t clock_type;       ///< PTP 时钟节点类型（PTPTSCR[8:6]）
    uint32_t          subsecond_increment; ///< 亚秒计数增量（PTPSSIR，纳秒单位）
    uint32_t          addend;              ///< 频率补偿累加值（PTPTSAR）
    bool              timestamp_enable;    ///< 发送接收时间戳全局使能（PTPTSCR.TSE）
    bool              fine_update;         ///< 精细校准模式（PTPTSCR.TSFCU，否则为粗略）
    bool              ipv6_enable;         ///< IPv6 帧时间戳过滤（PTPTSCR.TSIPV6FE）
    bool              ipv4_enable;         ///< IPv4 帧时间戳过滤（PTPTSCR.TSIPV4FE）
    bool              eth_frames_enable;   ///< 以太网帧 PTP 报文时间戳（PTPTSCR.TSIPENA）
    bool              snapshot_all;        ///< 对所有帧打时间戳（PTPTSCR.TSSARFE）
    bool              mac_addr_filter;     ///< 目的地址 MAC 过滤时间戳（PTPTSCR.TSENMACADDR）
    bool              ptp_v2;              ///< PTP v2（IEEE 1588-2008）格式（PTPTSCR.TSVER2FE）
    bool              snap_master_only;    ///< 仅对主节点事件帧打时间戳（PTPTSCR.TSMSTRENA）
    bool              event_msgs_only;     ///< 仅对事件消息打时间戳（PTPTSCR.TSEVNTENA）
    bool              pps_signal_enable;   ///< PPS 秒脉冲输出使能（PTPTSCR.TSPPSEN）
} eth_ptp_config_t;

/**
 * @brief LPI（EEE，802.3az）配置结构体
 *
 * 用于 eth_config_lpi()的配置参数。
 */
typedef struct {
    uint16_t          ls_timer_ms;         ///< LinkStatus 计时器（MACLPITCR.LST，单位 ms）
    uint16_t          tw_timer_us;         ///< Tw 唤醒计时器（MACLPITCR.TWT，单位 μs）
    bool              lpi_enable;          ///< LPI 使能（MACLPICSR.LPIEN）
    bool              pls;                 ///< PHY 链路状态（MACLPICSR.PLS：1=链路 UP）
    bool              lpitxa;              ///< LPI TX 自动触发（MACLPICSR.LPITXA）
} eth_lpi_config_t;

/**
 * @brief L3/L4 过滤器配置结构体
 *
 * 用于 eth_config_l3l4_filter() 的配置参数（index = 0 或 1，对应两套独立过滤器）。
 */
typedef struct {
    uint32_t          l3_addr[4];          ///< L3 地址（IPv4 仅使用 [0]；IPv6 使用 [0]~[3]）
    uint16_t          l4_dst_port;         ///< L4 目标端口（MACL4AxR[31:16]）
    uint16_t          l4_src_port;         ///< L4 源端口（MACL4AxR[15:0]）
    uint8_t           l3_da_prefix_len;    ///< L3 目的地址前缀匹配长度（MACL3L4CxR.L3HDBMx）
    uint8_t           l3_sa_prefix_len;    ///< L3 源地址前缀匹配长度（MACL3L4CxR.L3HSBMx）
    bool              enable;              ///< 过滤器总使能（通过 L3SAM/L3DAM/L4SPM/L4DPM 控制）
    bool              l4_protocol_udp;     ///< L4 协议：false=TCP，true=UDP（MACL3L4CxR.L4PENx）
    bool              l4_dst_match;        ///< 目标端口匹配使能（MACL3L4CxR.L4DPMx）
    bool              l4_src_match;        ///< 源端口匹配使能（MACL3L4CxR.L4SPMx）
    bool              l4_dst_inverse;      ///< 目标端口反向匹配（MACL3L4CxR.L4DPIMx）
    bool              l4_src_inverse;      ///< 源端口反向匹配（MACL3L4CxR.L4SPIMx）
    bool              l3_protocol_ipv6;    ///< L3 协议：false=IPv4，true=IPv6（MACL3L4CxR.L3PENx）
    bool              l3_da_match;         ///< L3 目的地址匹配使能（MACL3L4CxR.L3DAMx）
    bool              l3_sa_match;         ///< L3 源地址匹配使能（MACL3L4CxR.L3SAMx）
    bool              l3_da_inverse;       ///< L3 目的地址反向匹配（MACL3L4CxR.L3DAIMx）
    bool              l3_sa_inverse;       ///< L3 源地址反向匹配（MACL3L4CxR.L3SAIMx）
} eth_l3l4_filter_config_t;

/**
 * @brief MAC 地址过滤配置结构体
 *
 * 用于 eth_config_mac_addr()的配置参数。
 * index=0 时 SA 位和 MBC 位无效（地址0为主 MAC 地址，始终使能 DA 过滤）。
 */
typedef struct {
    uint8_t           addr[6];             ///< MAC 地址字节（大端序，addr[0] 为高字节）
    uint8_t           mask_bits;           ///< 字节掩码（MACAxHR.MBC，bit0~5 对应字节 5~0）
    bool              enable;              ///< 地址使能（MACAxHR.AE）
    bool              src_addr;            ///< true=SA 过滤，false=DA 过滤（MACAxHR.SA）
} eth_mac_addr_config_t;

/**
 * @brief MMC 控制配置结构体
 *
 * 用于 eth_config_mmc()的配置参数。
 */
typedef struct {
    bool              counter_reset;       ///< 计数器复位（MMCCR.CNTRST，写1后自动清零）
    bool              stop_on_rollover;    ///< 溢出后停止计数（MMCCR.CNTSTOPRO）
    bool              reset_on_read;       ///< 读后清零（MMCCR.RSTONRD）
    bool              freeze_on_preset;    ///< 预设时冻结（MMCCR.CNTFREEZ）
    bool              counter_preset;      ///< 计数器预设为 0x7FFF_FFFF（MMCCR.CNTPRST）
    bool              counter_preset_full; ///< 预设值：false=半满，true=全满（MMCCR.CNTPRSTLVL）
    bool              ucast_detect_bcast;  ///< 单播检测框架包含广播（MMCCR.UCDBC）
} eth_mmc_config_t;

/**
 * @brief MAC 看门狗超时配置结构体
 *
 * 用于 eth_config_watchdog_timer()的配置参数。
 */
typedef struct {
    uint16_t          timeout_val;         ///< 看门狗超时值（MACWTR.WTO，单位 256 字节）
    bool              prog_wdt_enable;     ///< 可编程看门狗使能（MACWTR.PWE）
} eth_watchdog_timer_config_t;

/**
 * @brief 快速初始化配置结构体（第 1 层）
 *
 * 汇总了 MAC、DMA、PHY 的关键参数，用于 eth_init() 一键初始化。
 */
typedef struct {
    eth_phy_interface_t  phy_interface;    ///< PHY 接口类型（MII / RMII）
    eth_smi_cr_t         smi_clk_range;    ///< SMI 时钟分频档位（依 HCLK 选择）
    eth_speed_t          speed;            ///< MAC 速率
    eth_duplex_t         duplex;           ///< MAC 双工模式
    eth_checksum_t       checksum_offload; ///< 校验和卸载
    uint8_t              tx_desc_count;    ///< TX 描述符数量（仅记录用途，驱动不使用此值）
    uint8_t              rx_desc_count;    ///< RX 描述符数量（仅记录用途，驱动不使用此值）
    void                *tx_desc_base;     ///< TX 描述符链表基地址（4字节对齐）
    void                *rx_desc_base;     ///< RX 描述符链表基地址（4字节对齐）
    bool                 enhanced_desc;    ///< 使用增强描述符（8字）
} eth_init_config_t;

/**
 * @brief TX DMA 描述符（增强模式 8×32-bit）
 *
 * 普通模式只使用 tdes0~tdes3（高4字保留）。
 */
typedef struct {
    volatile uint32_t  tdes0;  ///< 状态字（OWN、IC、LS、FS、DC、DP、TTSE、IHE 等）
    volatile uint32_t  tdes1;  ///< 控制 + 缓冲大小（TBS1、TBS2、SAC、CRCR 等）
    volatile uint32_t  tdes2;  ///< 缓冲 1 地址
    volatile uint32_t  tdes3;  ///< 缓冲 2 地址 / 下一描述符地址（链表模式）
    volatile uint32_t  tdes4;  ///< 保留（增强模式）
    volatile uint32_t  tdes5;  ///< 保留（增强模式）
    volatile uint32_t  tdes6;  ///< 发送时间戳低字（TTSL，增强模式）
    volatile uint32_t  tdes7;  ///< 发送时间戳高字（TTSH，增强模式）
} eth_tx_desc_t;

/**
 * @brief RX DMA 描述符（增强模式 8×32-bit）
 *
 * 普通模式只使用 rdes0~rdes3（高4字保留）。
 */
typedef struct {
    volatile uint32_t  rdes0;  ///< 状态字（OWN、AFM、ES、LE、SAF、CE 等）
    volatile uint32_t  rdes1;  ///< 控制 + 缓冲大小（RBS1、RBS2、RCH、RER）
    volatile uint32_t  rdes2;  ///< 缓冲 1 地址
    volatile uint32_t  rdes3;  ///< 缓冲 2 地址 / 下一描述符地址（链表模式）
    volatile uint32_t  rdes4;  ///< 扩展状态（IPCS、IPPE、IPHE、PMT 等，增强模式）
    volatile uint32_t  rdes5;  ///< 保留（增强模式）
    volatile uint32_t  rdes6;  ///< 接收时间戳低字（RTSL，增强模式）
    volatile uint32_t  rdes7;  ///< 接收时间戳高字（RTSH，增强模式）
} eth_rx_desc_t;

/**
 * @brief PTP 时间值
 */
typedef struct {
    uint32_t  seconds;      ///< 秒（PTPTSHR）
    uint32_t  nanoseconds;  ///< 纳秒（PTPTSLR，精度约 0.46 ns）
} eth_ptp_time_t;

/**
 * @brief MMC 统计计数器
 *
 * 字段对应各 MMC 硬件计数器寄存器，通过 eth_mmc_get_stats() 一次性读取。
 */
typedef struct {
    uint32_t  tx_single_collision_gf;  ///< 发送单次冲突好帧数（MMCTGFSCCR）
    uint32_t  tx_multi_collision_gf;   ///< 发送多次冲突好帧数（MMCTGFMCCR）
    uint32_t  tx_good_frames;          ///< 发送好帧总数（MMCTGFCR）
    uint32_t  rx_crc_error;            ///< 接收 CRC 错误帧数（MMCRFCECR）
    uint32_t  rx_align_error;          ///< 接收对齐错误帧数（MMCRFAECR）
    uint32_t  rx_unicast_good_frames;  ///< 接收单播好帧数（MMCRGUFCR）
} eth_mmc_stats_t;

/**
 * @brief MAC 调试信息
 *
 * 字段对应 MACDBGR 寄存器各状态位，通过 eth_get_mac_debug_info() 读取。
 */
typedef struct {
    uint8_t  rx_fifo_fill_level;    ///< 接收 FIFO 填充级别（MACDBGR.RFFL[1:0]）
    uint8_t  rx_fifo_read_state;    ///< 接收 FIFO 读取控制器状态（MACDBGR.RFRCS[1:0]）
    uint8_t  tx_fifo_read_state;    ///< 发送 FIFO 读取状态（MACDBGR.TFRS[1:0]）
    uint8_t  tx_frame_ctrl_state;   ///< 发送帧控制器状态（MACDBGR.MTFCS[1:0]）
    bool     rx_fifo_write_active;  ///< 接收 FIFO 写控制器活跃（MACDBGR.RFWCS）
    bool     rx_fifo_overflow;      ///< MAC 接收器模块 PE（MACDBGR.MMRPES）
    bool     tx_fifo_not_empty;     ///< 发送 FIFO 非空（MACDBGR.TFNE）
    bool     tx_fifo_full;          ///< 发送 FIFO 已满（MACDBGR.TFF）
    bool     tx_fifo_write_active;  ///< 发送 FIFO 写活跃（MACDBGR.TFWA）
    bool     tx_paused;             ///< MAC 发送暂停（MACDBGR.MTP）
    bool     tx_mac_active;         ///< MAC 发送 MAC 引擎激活（MACDBGR.MMTEA）
} eth_mac_debug_t;

//===========================================
// 回调函数类型定义
//===========================================

/**
 * @brief ETH 中断回调函数类型
 *
 * @param[in] context 注册时传入的用户上下文指针
 * @param[in] event   中断事件掩码（ETH_DMA_FLAG_xxx 或 MAC 中断位）
 *
 * @warning 回调在中断上下文中执行，应保持精简，禁止调用阻塞式 API
 */
typedef void (*eth_irq_callback_t)(void *context, uint32_t event);

//===========================================
// 第1层：快速初始化 API
//===========================================

/**
 * @brief 一键初始化 ETH MAC + DMA + 配置 SMI 时钟
 *
 * 按以下顺序执行：软复位 DMA（DMABMR.SWR）→ 配置 DMA 总线模式 →
 * 配置 SMI 时钟（MACMIIAR.CR）→ 配置 MAC 参数 → 设置 TX/RX 描述符链表基地址 →
 * 配置 MACIMR（屏蔽所有 MAC 子中断）。
 *
 * 调用前须：(1) 使能 ETH AHB 时钟，(2) 通过 syscfg_eth_set_phy_interface() 设置好
 * MII/RMII 模式（本函数不配置 PHY 接口类型，phy_interface 字段仅作记录用途）。
 *
 * @param[in] config  初始化配置（不可为 NULL）
 * @return true: 成功（DMA 复位完成），false: DMA 软复位超时
 *
 *
 * @code
 * eth_init_config_t cfg = {
 *     .phy_interface   = ETH_PHY_RMII,
 *     .smi_clk_range   = ETH_SMI_CR_100_150MHZ,
 *     .speed           = ETH_SPEED_100M,
 *     .duplex          = ETH_DUPLEX_FULL,
 *     .checksum_offload = ETH_CHECKSUM_NONE,
 *     .enhanced_desc   = false,
 *     .tx_desc_base    = tx_desc_ring,
 *     .rx_desc_base    = rx_desc_ring,
 * };
 * eth_init(&cfg);
 * @endcode
 */
bool eth_init(const eth_init_config_t *config);

/**
 * @brief 执行 DMA 软复位并等待完成
 *
 * 向 DMABMR.SWR 写 1，轮询等待清零（复位期间所有寄存器恢复默认值）。
 *
 * @return true: 复位完成，false: 超时（> ETH_PHY_TIMEOUT 次循环）
 *
 * @warning 复位后所有 DMA 配置将丢失，需重新初始化
 */
bool eth_dma_software_reset(void);

//===========================================
// 第2层：分组配置 API
//===========================================

/**
 * @brief MAC 参数完整配置
 *
 * 按结构体字段一一映射到 MACCR 寄存器，先禁止 TE/RE 再写入，
 * 写入完成后恢复原 TE/RE 状态。
 *
 * @param[in] config  MAC 配置结构体（不可为 NULL）
 *
 */
void eth_config_mac(const eth_mac_config_t *config);

/**
 * @brief DMA 总线模式完整配置
 *
 * 配置 DMABMR 和 DMAOMR 寄存器（不触发 FTF 冲洗，除非 config->flush_tx_fifo=true）。
 *
 * @param[in] config  DMA 配置结构体（不可为 NULL）
 *
 */
void eth_config_dma(const eth_dma_config_t *config);

/**
 * @brief 帧接收过滤配置
 *
 * @param[in] config  过滤配置结构体（不可为 NULL）
 *
 */
void eth_config_filter(const eth_filter_config_t *config);

/**
 * @brief 流控配置
 *
 * @param[in] config  流控配置结构体（不可为 NULL）
 *
 */
void eth_config_flow_ctrl(const eth_flow_ctrl_config_t *config);

/**
 * @brief PTP 时间戳配置
 *
 * 写入 PTPTSCR 后，若 fine_update=true 且 addend != 0 则同时初始化频率补偿加法器；
 * 若 timestamp_enable=true 且当前系统时间为零则初始化系统时间。
 *
 * @param[in] config  PTP 配置结构体（不可为 NULL）
 *
 */
void eth_config_ptp(const eth_ptp_config_t *config);

/**
 * @brief LPI（EEE）配置
 *
 * @param[in] config  LPI 配置结构体（不可为 NULL）
 *
 */
void eth_config_lpi(const eth_lpi_config_t *config);

/**
 * @brief VLAN 标记配置
 *
 * @param[in] config  VLAN 配置结构体（不可为 NULL）
 *
 */
void eth_config_vlan(const eth_vlan_config_t *config);

/**
 * @brief L3/L4 过滤器配置
 *
 * @param[in] index   过滤器索引（0 或 1）
 * @param[in] config  L3/L4 过滤器配置结构体（不可为 NULL）
 *
 *       MACL3A0[index]R ~ MACL3A3[index]R
 * @warning 须确保 index < ETH_L3L4_FILTER_COUNT
 */
void eth_config_l3l4_filter(uint8_t index, const eth_l3l4_filter_config_t *config);

/**
 * @brief MAC 地址过滤配置
 *
 * 配置 MACA[index]HR/LR，将 addr[6] 写入寄存器并设置 MBC/SA/AE 控制位。
 * index=0 时自动忽略 SA 和 MBC 设置（MACA0HR 不含这些位）。
 *
 * @param[in] index   地址索引（0 ~ 3）
 * @param[in] config  MAC 地址配置结构体（不可为 NULL）
 *
 * @warning 须确保 index < ETH_MAC_ADDR_COUNT
 */
void eth_config_mac_addr(uint8_t index, const eth_mac_addr_config_t *config);

/**
 * @brief MMC 计数器控制配置
 *
 * @param[in] config  MMC 控制配置（不可为 NULL）
 *
 */
void eth_config_mmc(const eth_mmc_config_t *config);

/**
 * @brief 看门狗超时定时器配置
 *
 * @param[in] config  看门狗配置（不可为 NULL）
 *
 */
void eth_config_watchdog_timer(const eth_watchdog_timer_config_t *config);

/**
 * @brief 配置 MAC Hash 过滤表（组播 / 单播）
 *
 * 直接写入 MACHTHR/MACHTLR，供 Hash 过滤模式使用。
 *
 * @param[in] hash_high  Hash 表高 32 位（MACHTHR）
 * @param[in] hash_low   Hash 表低 32 位（MACHTLR）
 *
 * @note 覆盖寄存器：MACHTHR, MACHTLR
 */
void eth_set_hash_table(uint32_t hash_high, uint32_t hash_low);

/**
 * @brief 配置唤醒帧寄存器（Wake-Up Frame Filter）
 *
 * 向 MACRWUFFR 按顺序写入 8 个 32 位字（共 256 bit 的帧格式描述）。
 * 调用时 MACPMTCSR.RWFFPR 必须已置位以使能远端唤醒帧过滤器。
 *
 * @param[in] wuf_regs  指向 8 个 uint32_t 值的数组（不可为 NULL）
 *
 */
void eth_set_wake_up_frame(const uint32_t *wuf_regs);

/**
 * @brief PMT（掉电管理）控制配置
 *
 * 配置 MACPMTCSR 寄存器，使能 Magic Packet 和/或唤醒帧检测，控制掉电位。
 *
 * @param[in] power_down     掉电模式（MACPMTCSR.PD）
 * @param[in] magic_pkt_en   Magic Packet 使能（MACPMTCSR.MPE）
 * @param[in] wake_frame_en  唤醒帧使能（MACPMTCSR.WFE）
 * @param[in] global_ucast   全局单播（MACPMTCSR.GU）
 * @param[in] rwffpr         唤醒帧过滤寄存器指针复位（MACPMTCSR.RWFFPR 写1复位）
 *
 */
void eth_config_pmt(bool power_down, bool magic_pkt_en,
                    bool wake_frame_en, bool global_ucast, bool rwffpr);

/**
 * @brief 配置 DMA 发送/接收描述符链表基地址
 *
 * 分别写入 DMATDLAR 和 DMARDLAR。
 *
 * @param[in] tx_desc_addr  TX 描述符链表起始地址（4 字节对齐）
 * @param[in] rx_desc_addr  RX 描述符链表起始地址（4 字节对齐）
 *
 * @note 覆盖寄存器：DMATDLAR, DMARDLAR
 */
void eth_set_desc_base(uint32_t tx_desc_addr, uint32_t rx_desc_addr);

/**
 * @brief 配置 DMA 接收中断看门狗计时器
 *
 * RIWTC 每个单位为 256 个 HCLK 周期。
 *
 * @param[in] riwtc  计时器值（DMARIWTR.RIWTC，0~255；0 = 禁用）
 *
 * @note 覆盖寄存器：DMARIWTR
 */
void eth_set_rx_watchdog_timer(uint8_t riwtc);

/**
 * @brief 使能或屏蔽指定 MAC 子中断
 *
 * @param[in] it_mask  ETH_MAC_IT_xxx 掩码组合（写 1 屏蔽对应中断）
 * @param[in] enable   true=屏蔽（写1），false=解除屏蔽（写0）
 *
 * @note 覆盖寄存器：MACIMR
 */
void eth_mac_irq_config(uint32_t it_mask, bool enable);

/**
 * @brief 使能或禁用指定 DMA 中断
 *
 * @param[in] it_mask  ETH_DMA_IT_xxx 掩码组合
 * @param[in] enable   true=使能，false=禁用
 *
 * @note 覆盖寄存器：DMAIER
 */
void eth_dma_irq_config(uint32_t it_mask, bool enable);

/**
 * @brief 配置 MMC 接收中断屏蔽
 *
 * @param[in] it_mask  ETH_MMC_RX_IT_xxx 掩码组合（写 1 屏蔽）
 * @param[in] mask     true=屏蔽，false=解除屏蔽
 *
 * @note 覆盖寄存器：MMCRIMR
 */
void eth_mmc_rx_irq_config(uint32_t it_mask, bool mask);

/**
 * @brief 配置 MMC 发送中断屏蔽
 *
 * @param[in] it_mask  ETH_MMC_TX_IT_xxx 掩码组合（写 1 屏蔽）
 * @param[in] mask     true=屏蔽，false=解除屏蔽
 *
 * @note 覆盖寄存器：MMCTIMR
 */
void eth_mmc_tx_irq_config(uint32_t it_mask, bool mask);

//===========================================
// 第3层：高级功能 API
//===========================================

/**
 * @brief 启动 DMA 接收引擎
 *
 * 置位 DMAOMR.SR。调用前须确认 DMARDLAR 已配置。
 *
 * @note 覆盖寄存器：DMAOMR.SR
 */
void eth_start_rx(void);

/**
 * @brief 停止 DMA 接收引擎
 *
 * 清除 DMAOMR.SR。
 *
 * @note 覆盖寄存器：DMAOMR.SR
 */
void eth_stop_rx(void);

/**
 * @brief 启动 DMA 发送引擎
 *
 * 置位 DMAOMR.ST。调用前须确认 DMATDLAR 已配置。
 *
 * @note 覆盖寄存器：DMAOMR.ST
 */
void eth_start_tx(void);

/**
 * @brief 停止 DMA 发送引擎
 *
 * 清除 DMAOMR.ST。
 *
 * @note 覆盖寄存器：DMAOMR.ST
 */
void eth_stop_tx(void);

/**
 * @brief 使能 MAC 接收（MACCR.RE）
 *
 * @note 覆盖寄存器：MACCR.RE
 */
void eth_mac_rx_enable(void);

/**
 * @brief 禁用 MAC 接收（MACCR.RE）
 *
 * @note 覆盖寄存器：MACCR.RE
 */
void eth_mac_rx_disable(void);

/**
 * @brief 使能 MAC 发送（MACCR.TE）
 *
 * @note 覆盖寄存器：MACCR.TE
 */
void eth_mac_tx_enable(void);

/**
 * @brief 禁用 MAC 发送（MACCR.TE）
 *
 * @note 覆盖寄存器：MACCR.TE
 */
void eth_mac_tx_disable(void);

/**
 * @brief 通过描述符发送以太网帧
 *
 * 将 buf 中的数据填入描述符并提交给 DMA。
 * 若 DMA TX 进程挂起（DMASR.TPSS），写 DMATPDR 唤醒。
 *
 * @param[in] desc  起始 TX 描述符指针（不可为 NULL，须已初始化 OWN=0）
 * @param[in] buf   帧数据缓冲区（不可为 NULL）
 * @param[in] len   帧长度（字节，含以太网头，不含 FCS）
 * @return true: 成功（描述符已提交），false: OWN 位仍为 1（DMA 尚未释放）
 *
 */
bool eth_transmit_frame(eth_tx_desc_t *desc, const uint8_t *buf, uint32_t len);

/**
 * @brief 通过描述符接收以太网帧
 *
 * 从 desc 读取接收到的帧数据到 buf，清除 OWN 位还给 DMA。
 * 若 DMA RX 进程挂起（DMASR.RPSS 或 RBUS），写 DMARPDR 唤醒。
 *
 * @param[in]  desc  起始 RX 描述符指针（不可为 NULL）
 * @param[out] buf   输出缓冲区（不可为 NULL，须足够大）
 * @param[out] len   实际接收长度（字节），NULL 时忽略
 * @return true: 成功（帧有效），false: 描述符 OWN=1（DMA 尚未写入）或帧错误
 *
 */
bool eth_receive_frame(eth_rx_desc_t *desc, uint8_t *buf, uint32_t *len);

/**
 * @brief 通过 SMI 接口读取 PHY 寄存器
 *
 * 向 MACMIIAR 写入 PHY 地址 + 寄存器地址 + CR + MB（读操作），
 * 轮询 MB 位清零后从 MACMIIDR 读取结果。
 *
 * @param[in]  phy_addr  PHY 地址（0~31）
 * @param[in]  reg_addr  PHY 寄存器地址（0~31）
 * @param[out] data      读取结果（不可为 NULL）
 * @return true: 成功，false: 超时（> ETH_PHY_TIMEOUT）
 *
 */
bool eth_phy_read(uint8_t phy_addr, uint8_t reg_addr, uint16_t *data);

/**
 * @brief 通过 SMI 接口写入 PHY 寄存器
 *
 * 向 MACMIIDR 写入待写数据，向 MACMIIAR 写入 PHY 地址 + 寄存器地址 + CR + MW + MB，
 * 轮询 MB 位清零确认写入完成。
 *
 * @param[in] phy_addr  PHY 地址（0~31）
 * @param[in] reg_addr  PHY 寄存器地址（0~31）
 * @param[in] data      待写入数据（16 位）
 * @return true: 成功，false: 超时
 *
 */
bool eth_phy_write(uint8_t phy_addr, uint8_t reg_addr, uint16_t data);

/**
 * @brief 获取当前 PTP 系统时间
 *
 * 先读 PTPTSHR（秒），再读 PTPTSLR（纳秒），避免跨秒竞态（如有需要重读）。
 *
 * @param[out] time  PTP 时间结构体（不可为 NULL）
 *
 */
void eth_ptp_get_time(eth_ptp_time_t *time);

/**
 * @brief 设置 PTP 系统时间（初始化时间）
 *
 * 写入 PTPTSHUR/PTPTSLUR，然后置 PTPTSCR.TSINIT 触发时间初始化。
 *
 * @param[in] time  目标时间（不可为 NULL）
 *
 */
void eth_ptp_set_time(const eth_ptp_time_t *time);

/**
 * @brief 更新 PTP 系统时间（偏移调整）
 *
 * 写入 PTPTSHUR/PTPTSLUR，置 PTPTSCR.TSUPDT 触发时间更新（加/减偏移）。
 *
 * @param[in] time  偏移时间（秒字段高位=1表示负值/减法）
 *
 */
void eth_ptp_update_time(const eth_ptp_time_t *time);

/**
 * @brief 调整 PTP 频率（精细校准）
 *
 * 根据 ppb（parts per billion）调整频率，计算新的 addend 值写入 PTPTSAR，
 * 再置 PTPTSCR.TSADDREG 更新频率寄存器。精细校准模式（PTPTSCR.TSFCU=1）时有效。
 *
 * @param[in] ppb  频率偏差（ppb），正值加速，负值减速
 *
 */
void eth_ptp_adjust_freq(int32_t ppb);

/**
 * @brief 设置 PTP 目标时间（用于事件触发中断）
 *
 * @param[in] time  目标时间（不可为 NULL），到达时产生 TSTS 中断
 *
 */
void eth_ptp_set_target_time(const eth_ptp_time_t *time);

/**
 * @brief 配置 PTP PPS 输出间隔和宽度
 *
 * @param[in] interval_ns  PPS 输出间隔纳秒值（PTPPPSIR）
 * @param[in] width_ns     PPS 脉冲宽度纳秒值（PTPPPSWR）
 *
 */
void eth_ptp_set_pps_output(uint32_t interval_ns, uint32_t width_ns);

/**
 * @brief 读取 PTP 辅助时间戳（捕获时间）
 *
 * @param[out] time  读取结果（不可为 NULL）
 *
 */
void eth_ptp_get_aux_timestamp(eth_ptp_time_t *time);

/**
 * @brief 获取 MMC 统计计数器快照
 *
 * 一次读取所有 MMC 计数器寄存器，写入 stats 结构体。
 * 若已配置 RSTONRD，读后计数器自动清零。
 *
 * @param[out] stats  统计结构体（不可为 NULL）
 *
 *       MMCRFCECR, MMCRFAECR, MMCRGUFCR
 */
void eth_mmc_get_stats(eth_mmc_stats_t *stats);

//===========================================
// 第4层：控制与查询 API
//===========================================

/**
 * @brief 读取 DMA 状态寄存器并可选清除标志
 *
 * @param[in] clear_mask  需要写 1 清除的标志掩码（ETH_DMA_FLAG_xxx），0 表示只读不清
 * @return DMASR 寄存器当前值（读后再清）
 *
 * @note 覆盖寄存器：DMASR
 */
uint32_t eth_get_dma_status(uint32_t clear_mask);

/**
 * @brief 读取 MAC ISR 状态寄存器（只读）
 *
 * @return MACISR 寄存器当前值
 *
 * @note 覆盖寄存器：MACISR
 */
uint32_t eth_get_mac_isr(void);

/**
 * @brief 读取 MAC 调试寄存器信息
 *
 * @param[out] info  调试信息结构体（不可为 NULL）
 *
 * @note 覆盖寄存器：MACDBGR
 */
void eth_get_mac_debug_info(eth_mac_debug_t *info);

/**
 * @brief 读取 DMA 未接收帧/缓冲区溢出计数
 *
 * @param[out] missed_frame_count   接收丢失帧数（DMAMFBOCR.MFC，16位）
 * @param[out] overflow_frame_count FIFO 溢出帧数（DMAMFBOCR.OFC，11位）
 * @param[out] overflow_missed_count 溢出丢失帧标志（DMAMFBOCR.OBFOC/OBMFC）
 *
 * @note 覆盖寄存器：DMAMFBOCR
 */
void eth_get_dma_missed_frame_count(uint16_t *missed_frame_count,
                                    uint16_t *overflow_frame_count,
                                    uint8_t  *overflow_missed_count);

/**
 * @brief 读取 DMA 当前宿主描述符地址（TX）
 *
 * @return DMACHTDR.HTDAP（DMA 当前 TX 描述符指针）
 *
 * @note 覆盖寄存器：DMACHTDR
 */
uint32_t eth_get_dma_tx_desc_ptr(void);

/**
 * @brief 读取 DMA 当前宿主描述符地址（RX）
 *
 * @return DMACHRDR.HRDAP（DMA 当前 RX 描述符指针）
 *
 * @note 覆盖寄存器：DMACHRDR
 */
uint32_t eth_get_dma_rx_desc_ptr(void);

/**
 * @brief 读取 DMA 当前宿主缓冲区地址（TX）
 *
 * @return DMACHTBAR.HTBAP（DMA 当前 TX 缓冲区地址）
 *
 * @note 覆盖寄存器：DMACHTBAR
 */
uint32_t eth_get_dma_tx_buf_ptr(void);

/**
 * @brief 读取 DMA 当前宿主缓冲区地址（RX）
 *
 * @return DMACHRBAR.HRBAP（DMA 当前 RX 缓冲区地址）
 *
 * @note 覆盖寄存器：DMACHRBAR
 */
uint32_t eth_get_dma_rx_buf_ptr(void);

/**
 * @brief 读取 LPI 中断/状态寄存器
 *
 * @return MACLPICSR 寄存器当前值（读取 TLPIEN/TLPIEX/RLPIEN/RLPIEX 等状态位）
 *
 * @note 覆盖寄存器：MACLPICSR
 */
uint32_t eth_get_lpi_status(void);

/**
 * @brief 读取 PTP 时间戳状态寄存器
 *
 * @return PTPTSSR 寄存器值（TSSO、TSOFS 标志位）
 *
 * @note 覆盖寄存器：PTPTSSR
 */
uint32_t eth_get_ptp_status(void);

/**
 * @brief 读取 MMC 接收中断状态
 *
 * @return MMCRIR 寄存器值（ETH_MMC_RX_IT_xxx 位掩码）
 *
 * @note 覆盖寄存器：MMCRIR
 */
uint32_t eth_get_mmc_rx_isr(void);

/**
 * @brief 读取 MMC 发送中断状态
 *
 * @return MMCTIR 寄存器值（ETH_MMC_TX_IT_xxx 位掩码）
 *
 * @note 覆盖寄存器：MMCTIR
 */
uint32_t eth_get_mmc_tx_isr(void);

/**
 * @brief 注册中断回调函数
 *
 * @param[in] type      中断向量（ETH_IRQ_DMA 或 ETH_IRQ_WKUP）
 * @param[in] callback  回调函数指针（NULL 表示注销）
 * @param[in] context   用户上下文指针（随回调传入）
 *
 * @note 覆盖寄存器：NVIC（中断注册/注销，ETH_IRQn / ETH_WKUP_IRQn）
 */
void eth_irq_register(eth_irq_type_t type,
                      eth_irq_callback_t callback,
                      void *context);

#ifdef __cplusplus
}
#endif

#endif /* _HARDWARE_ETH_H */
