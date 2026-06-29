/**
 * @file eth.c
 * @brief ACM32P4 以太网 MAC 控制器（ETH）驱动实现
 *
 * 实现了 ETH 驱动的全部功能，包括：
 * - MAC 参数完整配置（速率、双工、帧过滤、流控、VLAN）
 * - DMA 总线模式配置（突发长度、增强描述符、仲裁比）
 * - PHY SMI 接口读写（MDIO/MDC，最多 32 个 PHY）
 * - 帧发送与接收（DMA 描述符链，普通/增强模式）
 * - PTP 精确时间协议（IEEE 1588-2008，粗/精校准，PPS 输出，辅助时间戳）
 * - LPI/EEE（802.3az）节能控制
 * - L3/L4 过滤器（IPv4/IPv6 + TCP/UDP 端口过滤，双路）
 * - MAC 地址过滤（4 组完美过滤，含 SA/DA/掩码）
 * - MMC 统计计数器读取
 * - 双中断向量管理（DMA IRQn=45 + 唤醒 IRQn=46）
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#include <assert.h>
#include <stddef.h>
#include "device/acm32p4xx.h"
#include "system.h"
#include "hardware/eth.h"

//===========================================
// 内部宏定义
//===========================================

/** MACCR 寄存器位掩码 */
#define MACCR_RE_MSK            (1UL << 2)   ///< 接收使能
#define MACCR_TE_MSK            (1UL << 3)   ///< 发送使能
#define MACCR_DC_MSK            (1UL << 4)   ///< 延迟检查
#define MACCR_BL_POS            (5U)
#define MACCR_BL_MSK            (3UL << MACCR_BL_POS)
#define MACCR_APCS_MSK          (1UL << 7)   ///< PAD/CRC 自动去除
#define MACCR_DR_MSK            (1UL << 9)   ///< 禁止重传
#define MACCR_IPCO_MSK          (1UL << 10)  ///< IP 校验和卸载
#define MACCR_DM_MSK            (1UL << 11)  ///< 双工模式
#define MACCR_LM_MSK            (1UL << 12)  ///< 回环模式
#define MACCR_ROD_MSK           (1UL << 13)  ///< 禁止接收自身
#define MACCR_FES_MSK           (1UL << 14)  ///< 速率（100M）
#define MACCR_CSD_MSK           (1UL << 16)  ///< 载波侦听禁用
#define MACCR_IFG_POS           (17U)
#define MACCR_IFG_MSK           (7UL << MACCR_IFG_POS)
#define MACCR_JE_MSK            (1UL << 20)  ///< 巨型帧
#define MACCR_JD_MSK            (1UL << 22)  ///< Jabber 禁用
#define MACCR_WD_MSK            (1UL << 23)  ///< 看门狗禁用
#define MACCR_CSTF_MSK          (1UL << 25)  ///< Type2 帧 CRC 去除
#define MACCR_S2KP_MSK          (1UL << 27)  ///< 2K 包支持
#define MACCR_SAIRC_POS         (28U)
#define MACCR_SAIRC_MSK         (7UL << MACCR_SAIRC_POS)
#define MACCR_PRELEN_POS        (0U)
#define MACCR_PRELEN_MSK        (3UL << MACCR_PRELEN_POS)

/** MACFFR 寄存器位掩码 */
#define MACFFR_PM_MSK           (1UL << 0)   ///< 混杂模式
#define MACFFR_HU_MSK           (1UL << 1)   ///< 单播哈希过滤
#define MACFFR_HMC_MSK          (1UL << 2)   ///< 多播哈希过滤
#define MACFFR_DAIF_MSK         (1UL << 3)   ///< DA 反向过滤
#define MACFFR_PAM_MSK          (1UL << 4)   ///< 全多播
#define MACFFR_DBF_MSK          (1UL << 5)   ///< 禁止广播
#define MACFFR_PCF_POS          (6U)
#define MACFFR_PCF_MSK          (3UL << MACFFR_PCF_POS)
#define MACFFR_SAIF_MSK         (1UL << 8)   ///< SA 反向过滤
#define MACFFR_SAF_MSK          (1UL << 9)   ///< SA 过滤
#define MACFFR_HPF_MSK          (1UL << 10)  ///< Hash OR 完美过滤
#define MACFFR_VTFE_MSK         (1UL << 16)  ///< VLAN 过滤
#define MACFFR_IPFE_MSK         (1UL << 20)  ///< IP DA 过滤
#define MACFFR_DNTU_MSK         (1UL << 21)  ///< 丢弃非 TCP/UDP
#define MACFFR_RA_MSK           (1UL << 31)  ///< 接收所有

/** MACMIIAR 寄存器位掩码 */
#define MACMIIAR_MB_MSK         (1UL << 0)   ///< MII 繁忙
#define MACMIIAR_MW_MSK         (1UL << 1)   ///< MII 写
#define MACMIIAR_CR_POS         (2U)
#define MACMIIAR_CR_MSK         (0xFUL << MACMIIAR_CR_POS)
#define MACMIIAR_MR_POS         (6U)
#define MACMIIAR_MR_MSK         (0x1FUL << MACMIIAR_MR_POS)
#define MACMIIAR_PA_POS         (11U)
#define MACMIIAR_PA_MSK         (0x1FUL << MACMIIAR_PA_POS)

/** MACFCR 寄存器位掩码（设备头文件 MACFCR_f 中 [15:4] 为保留，PLT[5:4] 按 ETH 标准补充） */
#define MACFCR_FCB_BPA_MSK      (1UL << 0)
#define MACFCR_TFCE_MSK         (1UL << 1)
#define MACFCR_RFCE_MSK         (1UL << 2)
#define MACFCR_UPFD_MSK         (1UL << 3)
#define MACFCR_PLT_POS          (4U)
#define MACFCR_PLT_MSK          (3UL << MACFCR_PLT_POS)
#define MACFCR_PT_POS           (16U)
#define MACFCR_PT_MSK           (0xFFFFUL << MACFCR_PT_POS)

/** MACVLANTR 寄存器位掩码 */
#define MACVLANTR_VL_POS        (0U)
#define MACVLANTR_VL_MSK        (0xFFFFUL << MACVLANTR_VL_POS)
#define MACVLANTR_ETV_MSK       (1UL << 16)
#define MACVLANTR_VTIM_MSK      (1UL << 17)
#define MACVLANTR_ESVL_MSK      (1UL << 18)
#define MACVLANTR_VTHM_MSK      (1UL << 19)

/** MACPMTCSR 寄存器位掩码 */
#define MACPMTCSR_PD_MSK        (1UL << 0)
#define MACPMTCSR_MPE_MSK       (1UL << 1)
#define MACPMTCSR_WFE_MSK       (1UL << 2)
#define MACPMTCSR_MPR_MSK       (1UL << 5)
#define MACPMTCSR_WFR_MSK       (1UL << 6)
#define MACPMTCSR_GU_MSK        (1UL << 9)
#define MACPMTCSR_RWFP_POS      (24U)
#define MACPMTCSR_RWFP_MSK      (7UL << MACPMTCSR_RWFP_POS)
#define MACPMTCSR_RWFFPR_MSK    (1UL << 31)

/** MACLPICSR 寄存器位掩码 */
#define MACLPICSR_TLPIEN_MSK    (1UL << 0)
#define MACLPICSR_TLPIEX_MSK    (1UL << 1)
#define MACLPICSR_RLPIEN_MSK    (1UL << 2)
#define MACLPICSR_RLPIEX_MSK    (1UL << 3)
#define MACLPICSR_TLPIST_MSK    (1UL << 8)
#define MACLPICSR_RLPIST_MSK    (1UL << 9)
#define MACLPICSR_LPIEN_MSK     (1UL << 16)
#define MACLPICSR_PLS_MSK       (1UL << 17)
#define MACLPICSR_LPITXA_MSK    (1UL << 19)

/** MACLPITCR 寄存器位掩码 */
#define MACLPITCR_TWT_POS       (0U)
#define MACLPITCR_TWT_MSK       (0xFFFFUL << MACLPITCR_TWT_POS)
#define MACLPITCR_LST_POS       (16U)
#define MACLPITCR_LST_MSK       (0x3FFUL << MACLPITCR_LST_POS)

/** MACIMR 寄存器位掩码 */
#define MACIMR_PIM_MSK          (1UL << 3)
#define MACIMR_TIM_MSK          (1UL << 9)
#define MACIMR_LPIIM_MSK        (1UL << 10)

/** MACWTR 寄存器位掩码 */
#define MACWTR_WTO_POS          (0U)
#define MACWTR_WTO_MSK          (0x3FFFUL << MACWTR_WTO_POS)
#define MACWTR_PWE_MSK          (1UL << 16)

/** MMCCR 寄存器位掩码 */
#define MMCCR_CNTRST_MSK        (1UL << 0)
#define MMCCR_CNTSTOPRO_MSK     (1UL << 1)
#define MMCCR_RSTONRD_MSK       (1UL << 2)
#define MMCCR_CNTFREEZ_MSK      (1UL << 3)
#define MMCCR_CNTPRST_MSK       (1UL << 4)
#define MMCCR_CNTPRSTLVL_MSK    (1UL << 5)
#define MMCCR_UCDBC_MSK         (1UL << 8)

/** MACL3L4CxR 寄存器位掩码 */
#define L3L4C_L3PEN_MSK         (1UL << 0)
#define L3L4C_L3SAM_MSK         (1UL << 2)
#define L3L4C_L3SAIM_MSK        (1UL << 3)
#define L3L4C_L3DAM_MSK         (1UL << 4)
#define L3L4C_L3DAIM_MSK        (1UL << 5)
#define L3L4C_L3HSBM_POS        (6U)
#define L3L4C_L3HSBM_MSK        (0x1FUL << L3L4C_L3HSBM_POS)
#define L3L4C_L3HDBM_POS        (11U)
#define L3L4C_L3HDBM_MSK        (0x1FUL << L3L4C_L3HDBM_POS)
#define L3L4C_L4PEN_MSK         (1UL << 16)
#define L3L4C_L4SPM_MSK         (1UL << 18)
#define L3L4C_L4SPIM_MSK        (1UL << 19)
#define L3L4C_L4DPM_MSK         (1UL << 20)
#define L3L4C_L4DPIM_MSK        (1UL << 21)

/** MACL4AxR 寄存器位掩码 */
#define L4A_L4SP_POS            (0U)
#define L4A_L4SP_MSK            (0xFFFFUL << L4A_L4SP_POS)
#define L4A_L4DP_POS            (16U)
#define L4A_L4DP_MSK            (0xFFFFUL << L4A_L4DP_POS)

/** DMABMR 寄存器位掩码 */
#define DMABMR_SWR_MSK          (1UL << 0)   ///< 软复位
#define DMABMR_DA_MSK           (1UL << 1)   ///< 固定优先级（仲裁比由 PM 控制时置0）
#define DMABMR_DSL_POS          (2U)
#define DMABMR_DSL_MSK          (0x1FUL << DMABMR_DSL_POS)
#define DMABMR_EDFE_MSK         (1UL << 7)   ///< 增强描述符
#define DMABMR_PBL_POS          (8U)
#define DMABMR_PBL_MSK          (0x3FUL << DMABMR_PBL_POS)
#define DMABMR_PM_POS           (14U)
#define DMABMR_PM_MSK           (3UL << DMABMR_PM_POS)
#define DMABMR_FB_MSK           (1UL << 16)  ///< 固定突发
#define DMABMR_RDP_POS          (17U)
#define DMABMR_RDP_MSK          (0x3FUL << DMABMR_RDP_POS)
#define DMABMR_USP_MSK          (1UL << 23)  ///< RX 独立 PBL
#define DMABMR_EPM_MSK          (1UL << 24)  ///< 8×PBL 模式
#define DMABMR_AAB_MSK          (1UL << 25)  ///< 地址对齐突发
#define DMABMR_MB_MSK           (1UL << 26)  ///< 混合突发
#define DMABMR_TXPR_MSK         (1UL << 27)  ///< TX 优先
#define DMABMR_RIB_MSK          (1UL << 31)  ///< 重建 IB

/** DMAOMR 寄存器位掩码 */
#define DMAOMR_SR_MSK           (1UL << 1)   ///< 启动接收
#define DMAOMR_OSF_MSK          (1UL << 2)   ///< 对第二帧操作
#define DMAOMR_RTC_POS          (3U)
#define DMAOMR_RTC_MSK          (3UL << DMAOMR_RTC_POS)

#define DMAOMR_FUGF_MSK         (1UL << 6)   ///< 转发过小好帧
#define DMAOMR_FEF_MSK          (1UL << 7)   ///< 转发错误帧
#define DMAOMR_ST_MSK           (1UL << 13)  ///< 启动发送
#define DMAOMR_TTC_POS          (14U)
#define DMAOMR_TTC_MSK          (7UL << DMAOMR_TTC_POS)
#define DMAOMR_FTF_MSK          (1UL << 20)  ///< 冲洗 TX FIFO
#define DMAOMR_TSF_MSK          (1UL << 21)  ///< TX 存储转发
#define DMAOMR_DFRF_MSK         (1UL << 24)  ///< 禁止冲洗接收帧
#define DMAOMR_RSF_MSK          (1UL << 25)  ///< RX 存储转发
#define DMAOMR_DTCEFD_MSK       (1UL << 26)  ///< 丢弃 TCP/IP CS 错误帧

/** DMAIER 寄存器全部有效位 */
#define DMAIER_ALL_MSK          (0x0001E7FFUL)

/**
 * PTPTSCR 寄存器位掩码
 *
 * 依据 ACM32P4 ETH_TypeDef（acm32p4xx.h）实际字段布局：
 *   bit 0  TE       - 时间戳使能
 *   bit 1  TFCU     - 精细/粗略校准
 *   bit 2  TI       - 初始化系统时间（命令位，自清零）
 *   bit 3  TU       - 更新系统时间（命令位，自清零）
 *   bit 4  TITE     - 时间戳触发中断使能
 *   bit 5  TARU     - 更新加法器寄存器（命令位，自清零）
 *   bit 8  TSARFE   - 对所有接收帧打时间戳
 *   bit 9  TSR      - 亚秒计数回转模式
 *   bit 10 TSPTPPSV2E - PTPv2 使能
 *   bit 11 EPPEF    - 以太网帧 PTP 过滤
 *   bit 12 EPPFSIP6U - IPv6 PTP 过滤
 *   bit 13 EPPFSIP4U - IPv4 PTP 过滤
 *   bit 14 ETSFEM   - 仅事件消息打时间戳
 *   bit 15 ESFMRTM  - 仅主节点消息打时间戳
 *   bit[17:16] SPPFTS - PTP 时钟节点类型
 *   bit 18 EMAFPFF  - MAC 地址过滤 PTP 帧
 */
#define PTPTSCR_TE_MSK          (1UL << 0)   ///< 时间戳使能
#define PTPTSCR_TFCU_MSK        (1UL << 1)   ///< 精细校准（0=粗略）
#define PTPTSCR_TI_MSK          (1UL << 2)   ///< 初始化系统时间（命令）
#define PTPTSCR_TU_MSK          (1UL << 3)   ///< 更新系统时间（命令）
#define PTPTSCR_TITE_MSK        (1UL << 4)   ///< 触发中断使能
#define PTPTSCR_TARU_MSK        (1UL << 5)   ///< 更新加法器（命令）
#define PTPTSCR_TSARFE_MSK      (1UL << 8)   ///< 对所有帧打时间戳
#define PTPTSCR_TSR_MSK         (1UL << 9)   ///< 亚秒回转
#define PTPTSCR_TSPTPPSV2E_MSK  (1UL << 10)  ///< PTPv2
#define PTPTSCR_EPPEF_MSK       (1UL << 11)  ///< 以太网帧 PTP 过滤
#define PTPTSCR_EPPFSIP6U_MSK   (1UL << 12)  ///< IPv6 PTP 过滤
#define PTPTSCR_EPPFSIP4U_MSK   (1UL << 13)  ///< IPv4 PTP 过滤
#define PTPTSCR_ETSFEM_MSK      (1UL << 14)  ///< 仅事件消息
#define PTPTSCR_ESFMRTM_MSK     (1UL << 15)  ///< 仅主节点消息
#define PTPTSCR_SPPFTS_POS      (16U)
#define PTPTSCR_SPPFTS_MSK      (3UL << PTPTSCR_SPPFTS_POS)
#define PTPTSCR_EMAFPFF_MSK     (1UL << 18)  ///< MAC 地址过滤时间戳

/** PTPTSLR 纳秒负数符号位（PTPTSLUR.AST 同位置） */
#define PTPTSLUR_AST_MSK        (1UL << 31)  ///< 加/减符号位（1=减法）

/** PTPPPSCR 寄存器位掩码 */
#define PTPPPSCR_PPSCTRL_POS    (0U)
#define PTPPPSCR_PPSCTRL_MSK    (0xFUL << PTPPPSCR_PPSCTRL_POS)
#define PTPPPSCR_PPSEN_MSK      (1UL << 4)
#define PTPPPSCR_TRGTMODSEL_POS (5U)
#define PTPPPSCR_TRGTMODSEL_MSK (3UL << PTPPPSCR_TRGTMODSEL_POS)

/** TX 描述符 TDES0 关键位 */
#define TDES0_OWN_MSK           (1UL << 31)  ///< DMA 拥有权位
#define TDES0_IC_MSK            (1UL << 30)  ///< 完成中断
#define TDES0_LS_MSK            (1UL << 29)  ///< 最后段
#define TDES0_FS_MSK            (1UL << 28)  ///< 第一段
#define TDES0_DC_MSK            (1UL << 27)  ///< 禁止 CRC
#define TDES0_DP_MSK            (1UL << 26)  ///< 禁止 PAD
#define TDES0_TTSE_MSK          (1UL << 25)  ///< 时间戳使能
#define TDES0_TER_MSK           (1UL << 21)  ///< 发送结束环
#define TDES0_TCH_MSK           (1UL << 20)  ///< 第二地址链接

/** TX 描述符 TDES1 关键位 */
#define TDES1_TBS1_POS          (0U)
#define TDES1_TBS1_MSK          (0x1FFFUL << TDES1_TBS1_POS)
#define TDES1_TBS2_POS          (16U)
#define TDES1_TBS2_MSK          (0x1FFFUL << TDES1_TBS2_POS)

/** RX 描述符 RDES0 关键位 */
#define RDES0_OWN_MSK           (1UL << 31)  ///< DMA 拥有权位
#define RDES0_ES_MSK            (1UL << 15)  ///< 错误汇总
#define RDES0_LS_MSK            (1UL << 9)   ///< 最后描述符
#define RDES0_FS_MSK            (1UL << 8)   ///< 第一描述符
#define RDES0_FL_POS            (16U)
#define RDES0_FL_MSK            (0x3FFFUL << RDES0_FL_POS)

/** RX 描述符 RDES1 关键位 */
#define RDES1_RBS1_POS          (0U)
#define RDES1_RBS1_MSK          (0x1FFFUL << RDES1_RBS1_POS)
#define RDES1_RCH_MSK           (1UL << 14)  ///< 第二地址链接
#define RDES1_RER_MSK           (1UL << 15)  ///< 接收结束环

//===========================================
// 内部类型与状态变量
//===========================================

/** 驱动内部每向量回调状态 + 全局运行参数 */
static struct {
    eth_irq_callback_t dma_callback;    ///< DMA 中断回调（IRQn=45）
    void              *dma_context;     ///< DMA 回调用户上下文
    eth_irq_callback_t wkup_callback;   ///< 唤醒中断回调（IRQn=46）
    void              *wkup_context;    ///< 唤醒回调用户上下文
    uint8_t            smi_cr;          ///< 当前 SMI 时钟档位（ETH_SMI_CR_xxx）
    uint32_t           ptp_addend;      ///< 当前 PTP 频率补偿加法器基准值
} s_state;

//===========================================
// 内部辅助函数
//===========================================

/**
 * @brief 获取 ETH 外设基地址指针
 */
static inline ETH_TypeDef *eth_periph(void)
{
    return ETH;
}

#define ETH_POLL_TIMEOUT_MS    100U

/**
 * @brief 轮询等待某寄存器中某位清零
 * @return true: 位已清零，false: 超时
 */
static bool poll_until_clear(volatile uint32_t *reg, uint32_t mask)
{
    uint64_t start = system_get_tick();
    while ((*reg & mask) && !system_elapsed(start, ETH_POLL_TIMEOUT_MS)) {
    }
    return !(*reg & mask);
}

/**
 * @brief 轮询等待某寄存器中某位置位
 * @return true: 位已置位，false: 超时
 */
static bool poll_until_set(volatile uint32_t *reg, uint32_t mask)
{
    uint64_t start = system_get_tick();
    while (!(*reg & mask) && !system_elapsed(start, ETH_POLL_TIMEOUT_MS)) {
    }
    return (*reg & mask) != 0U;
}

/**
 * @brief 根据 index 返回对应 MACL3L4CxR 寄存器地址
 */
static volatile uint32_t *get_l3l4_ctrl_reg(ETH_TypeDef *eth, uint8_t index)
{
    return (index == 0U) ? &eth->MACL3L4C0R : &eth->MACL3L4C1R;
}

/**
 * @brief 根据 index 返回对应 MACL4AxR 寄存器地址
 */
static volatile uint32_t *get_l4a_reg(ETH_TypeDef *eth, uint8_t index)
{
    return (index == 0U) ? &eth->MACL4A0R : &eth->MACL4A1R;
}

/**
 * @brief 根据 index 和 word 返回 MACL3Axxx 地址寄存器（IPv4/IPv6）
 */
static volatile uint32_t *get_l3a_reg(ETH_TypeDef *eth, uint8_t index, uint8_t word)
{
    if (index == 0U) {
        volatile uint32_t *base[] = {
            &eth->MACL3A00R, &eth->MACL3A10R,
            &eth->MACL3A20R, &eth->MACL3A30R
        };
        return base[word & 3U];
    } else {
        volatile uint32_t *base[] = {
            &eth->MACL3A01R, &eth->MACL3A11R,
            &eth->MACL3A21R, &eth->MACL3A31R
        };
        return base[word & 3U];
    }
}

//===========================================
// 第1层：快速初始化 API
//===========================================

bool eth_init(const eth_init_config_t *config)
{
    assert(config != NULL);

    /* 1. DMA 软复位 */
    if (!eth_dma_software_reset()) {
        return false;
    }

    /* 2. 保存 SMI 时钟档位、配置 MACMIIAR.CR */
    s_state.smi_cr = (uint8_t)config->smi_clk_range;
    ETH->MACMIIAR  = ((uint32_t)config->smi_clk_range << MACMIIAR_CR_POS) & MACMIIAR_CR_MSK;

    /* 3. 配置 DMA 总线模式 */
    eth_dma_config_t dma_cfg = {
        .pbl             = 32U,
        .address_aligned = true,
        .enhanced_desc   = config->enhanced_desc,
        .fixed_burst     = true,
        .rx_store_forward = true,
        .tx_store_forward = true,
        .operate_on_2nd_frame = true,
    };
    eth_config_dma(&dma_cfg);

    /* 4. 配置 MAC */
    eth_mac_config_t mac_cfg = {
        .speed            = config->speed,
        .duplex           = config->duplex,
        .checksum_offload = config->checksum_offload,
        .preamble_length  = ETH_PREAMBLE_7BYTE,
        .inter_frame_gap  = ETH_IFG_96BIT,
        .backoff_limit    = ETH_BACKOFF_LIMIT_10,
        .sa_insert        = ETH_SA_INSERT_NONE,
    };
    eth_config_mac(&mac_cfg);

    /* 5. 配置 DMA 描述符链表基地址 */
    eth_set_desc_base((uint32_t)config->tx_desc_base,
                      (uint32_t)config->rx_desc_base);

    /* 6. 屏蔽全部 MAC 子中断（上层按需解除） */
    ETH->MACIMR = MACIMR_PIM_MSK | MACIMR_TIM_MSK | MACIMR_LPIIM_MSK;

    return true;
}

bool eth_dma_software_reset(void)
{
    ETH->DMABMR |= DMABMR_SWR_MSK;
    return poll_until_clear(&ETH->DMABMR, DMABMR_SWR_MSK);
}

//===========================================
// 第2层：分组配置 API
//===========================================

void eth_config_mac(const eth_mac_config_t *config)
{
    assert(config != NULL);

    /* 暂时禁止 TE/RE，安全修改 MACCR */
    uint32_t saved_trx = ETH->MACCR & (MACCR_TE_MSK | MACCR_RE_MSK);
    ETH->MACCR &= ~(MACCR_TE_MSK | MACCR_RE_MSK);

    uint32_t reg = 0U;
    reg |= ((uint32_t)config->preamble_length  << MACCR_PRELEN_POS) & MACCR_PRELEN_MSK;
    reg |= (config->deferral_check              ? MACCR_DC_MSK   : 0U);
    reg |= ((uint32_t)config->backoff_limit     << MACCR_BL_POS) & MACCR_BL_MSK;
    reg |= (config->pad_crc_strip               ? MACCR_APCS_MSK : 0U);
    reg |= (config->retry_disable               ? MACCR_DR_MSK   : 0U);
    reg |= (config->checksum_offload            ? MACCR_IPCO_MSK : 0U);
    reg |= (config->duplex == ETH_DUPLEX_FULL   ? MACCR_DM_MSK   : 0U);
    reg |= (config->loopback                    ? MACCR_LM_MSK   : 0U);
    reg |= (config->rx_own_disable              ? MACCR_ROD_MSK  : 0U);
    reg |= (config->speed == ETH_SPEED_100M     ? MACCR_FES_MSK  : 0U);
    reg |= (config->carrier_sense_disable       ? MACCR_CSD_MSK  : 0U);
    reg |= ((uint32_t)config->inter_frame_gap   << MACCR_IFG_POS) & MACCR_IFG_MSK;
    reg |= (config->jumbo_frame                 ? MACCR_JE_MSK   : 0U);
    reg |= (config->jabber_disable              ? MACCR_JD_MSK   : 0U);
    reg |= (config->watchdog_disable            ? MACCR_WD_MSK   : 0U);
    reg |= (config->crc_strip_type2             ? MACCR_CSTF_MSK : 0U);
    reg |= (config->s2kp_enable                 ? MACCR_S2KP_MSK : 0U);
    reg |= ((uint32_t)config->sa_insert         << MACCR_SAIRC_POS) & MACCR_SAIRC_MSK;
    ETH->MACCR = reg | saved_trx;
}

void eth_config_dma(const eth_dma_config_t *config)
{
    assert(config != NULL);

    /* DMABMR */
    uint32_t bmr = 0U;
    bmr |= ((uint32_t)config->desc_skip_length << DMABMR_DSL_POS) & DMABMR_DSL_MSK;
    bmr |= (config->enhanced_desc    ? DMABMR_EDFE_MSK : 0U);
    bmr |= ((uint32_t)config->pbl    << DMABMR_PBL_POS) & DMABMR_PBL_MSK;
    /* 仲裁策略 */
    if (config->priority == ETH_DMA_PRIORITY_TX_PRIOR) {
        bmr |= DMABMR_TXPR_MSK;
    } else {
        bmr |= ((uint32_t)config->priority << DMABMR_PM_POS) & DMABMR_PM_MSK;
    }
    bmr |= (config->fixed_burst      ? DMABMR_FB_MSK  : 0U);
    if (config->use_separate_pbl) {
        bmr |= DMABMR_USP_MSK;
        bmr |= ((uint32_t)config->rx_dma_pbl << DMABMR_RDP_POS) & DMABMR_RDP_MSK;
    }
    bmr |= (config->epm              ? DMABMR_EPM_MSK : 0U);
    bmr |= (config->address_aligned  ? DMABMR_AAB_MSK : 0U);
    bmr |= (config->mixed_burst      ? DMABMR_MB_MSK  : 0U);
    bmr |= (config->rib              ? DMABMR_RIB_MSK : 0U);
    ETH->DMABMR = bmr;

    /* DMAOMR（不触碰 SR/ST 引擎控制位） */
    uint32_t omr = ETH->DMAOMR & (DMAOMR_SR_MSK | DMAOMR_ST_MSK);
    omr |= (config->operate_on_2nd_frame ? DMAOMR_OSF_MSK   : 0U);
    omr |= ((uint32_t)config->rx_threshold << DMAOMR_RTC_POS) & DMAOMR_RTC_MSK;
    omr |= (config->fwd_undersized_gf    ? DMAOMR_FUGF_MSK  : 0U);
    omr |= (config->fwd_error_frames     ? DMAOMR_FEF_MSK   : 0U);
    omr |= ((uint32_t)config->tx_threshold << DMAOMR_TTC_POS) & DMAOMR_TTC_MSK;
    if (config->flush_tx_fifo) {
        omr |= DMAOMR_FTF_MSK;
    }
    omr |= (config->tx_store_forward     ? DMAOMR_TSF_MSK   : 0U);
    omr |= (config->flush_rx_frame       ? DMAOMR_DFRF_MSK  : 0U);
    omr |= (config->rx_store_forward     ? DMAOMR_RSF_MSK   : 0U);
    omr |= (config->drop_tcp_cs_err      ? DMAOMR_DTCEFD_MSK : 0U);
    ETH->DMAOMR = omr;
}

void eth_config_filter(const eth_filter_config_t *config)
{
    assert(config != NULL);

    uint32_t reg = 0U;
    reg |= (config->promiscuous         ? MACFFR_PM_MSK   : 0U);
    reg |= (config->hash_unicast        ? MACFFR_HU_MSK   : 0U);
    reg |= (config->hash_multicast      ? MACFFR_HMC_MSK  : 0U);
    reg |= (config->da_inverse_filter   ? MACFFR_DAIF_MSK : 0U);
    reg |= (config->broadcast_disable   ? MACFFR_DBF_MSK  : 0U);
    reg |= ((uint32_t)config->pass_ctrl_frames << MACFFR_PCF_POS) & MACFFR_PCF_MSK;
    reg |= (config->sa_inverse_filter   ? MACFFR_SAIF_MSK : 0U);
    reg |= (config->sa_filter           ? MACFFR_SAF_MSK  : 0U);
    reg |= (config->hash_or_perfect     ? MACFFR_HPF_MSK  : 0U);
    reg |= (config->vlan_tag_filter     ? MACFFR_VTFE_MSK : 0U);
    reg |= (config->ip_da_filter        ? MACFFR_IPFE_MSK : 0U);
    reg |= (config->drop_non_tcp_udp    ? MACFFR_DNTU_MSK : 0U);
    reg |= (config->receive_all         ? MACFFR_RA_MSK   : 0U);
    ETH->MACFFR = reg;
}

void eth_config_flow_ctrl(const eth_flow_ctrl_config_t *config)
{
    assert(config != NULL);

    uint32_t reg = 0U;
    /* FCB_BPA：全双工=发送零时间片暂停帧；半双工=激活背压。
     * 全双工与半双工互斥，不建议同时设置 zero_quanta_pause 和 back_pressure */
    reg |= (config->zero_quanta_pause || config->back_pressure ? MACFCR_FCB_BPA_MSK : 0U);
    reg |= (config->tx_flow_ctrl        ? MACFCR_TFCE_MSK : 0U);
    reg |= (config->rx_flow_ctrl        ? MACFCR_RFCE_MSK : 0U);
    reg |= (config->unicast_pause_detect? MACFCR_UPFD_MSK : 0U);
    reg |= ((uint32_t)config->pause_low_threshold << MACFCR_PLT_POS) & MACFCR_PLT_MSK;
    reg |= ((uint32_t)config->pause_time          << MACFCR_PT_POS)  & MACFCR_PT_MSK;
    ETH->MACFCR = reg;
}

void eth_config_ptp(const eth_ptp_config_t *config)
{
    assert(config != NULL);

    /* 配置 PTPTSCR（仅配置位，不写命令位） */
    uint32_t reg = 0U;
    reg |= (config->timestamp_enable ? PTPTSCR_TE_MSK       : 0U);
    reg |= (config->fine_update      ? PTPTSCR_TFCU_MSK     : 0U);
    reg |= (config->snapshot_all     ? PTPTSCR_TSARFE_MSK   : 0U);
    reg |= (config->ptp_v2           ? PTPTSCR_TSPTPPSV2E_MSK : 0U);
    reg |= (config->eth_frames_enable? PTPTSCR_EPPEF_MSK    : 0U);
    reg |= (config->ipv6_enable      ? PTPTSCR_EPPFSIP6U_MSK : 0U);
    reg |= (config->ipv4_enable      ? PTPTSCR_EPPFSIP4U_MSK : 0U);
    reg |= (config->event_msgs_only  ? PTPTSCR_ETSFEM_MSK   : 0U);
    reg |= (config->snap_master_only ? PTPTSCR_ESFMRTM_MSK  : 0U);
    reg |= ((uint32_t)config->clock_type << PTPTSCR_SPPFTS_POS) & PTPTSCR_SPPFTS_MSK;
    reg |= (config->mac_addr_filter  ? PTPTSCR_EMAFPFF_MSK  : 0U);
    ETH->PTPTSCR = reg;

    /* 配置亚秒增量寄存器 */
    ETH->PTPSSIR = config->subsecond_increment & 0xFFU;

    /* 若提供加法器值，写入 PTPTSAR 并触发更新 */
    if (config->addend != 0U) {
        s_state.ptp_addend = config->addend;
        ETH->PTPTSAR       = config->addend;
        ETH->PTPTSCR      |= PTPTSCR_TARU_MSK;
        /* 不必等待 TARU 自清零，硬件通常一个 AHB 周期内完成 */
    }

    /* 配置 PPS 输出使能（PTPPPSCR.PPSEN） */
    if (config->pps_signal_enable) {
        ETH->PTPPPSCR |= PTPPPSCR_PPSEN_MSK;
    } else {
        ETH->PTPPPSCR &= ~PTPPPSCR_PPSEN_MSK;
    }
}

void eth_config_lpi(const eth_lpi_config_t *config)
{
    assert(config != NULL);

    /* MACLPITCR：计时器值 */
    uint32_t tcr = 0U;
    tcr |= ((uint32_t)config->tw_timer_us  << MACLPITCR_TWT_POS) & MACLPITCR_TWT_MSK;
    tcr |= ((uint32_t)config->ls_timer_ms  << MACLPITCR_LST_POS) & MACLPITCR_LST_MSK;
    ETH->MACLPITCR = tcr;

    /* MACLPICSR：使能位（状态标志位在此只读，写入无效；
     * PLS 位在设备头文件中标记为读写，写入由硬件忽略，无副作用） */
    uint32_t csr = ETH->MACLPICSR;
    csr &= ~(MACLPICSR_LPIEN_MSK | MACLPICSR_PLS_MSK | MACLPICSR_LPITXA_MSK);
    csr |= (config->lpi_enable ? MACLPICSR_LPIEN_MSK  : 0U);
    csr |= (config->pls        ? MACLPICSR_PLS_MSK    : 0U);
    csr |= (config->lpitxa     ? MACLPICSR_LPITXA_MSK : 0U);
    ETH->MACLPICSR = csr;
}

void eth_config_vlan(const eth_vlan_config_t *config)
{
    assert(config != NULL);

    /* MACVLANTR：VLAN 接收过滤配置 */
    uint32_t reg = 0U;
    reg |= ((uint32_t)config->vlan_id << MACVLANTR_VL_POS) & MACVLANTR_VL_MSK;
    reg |= (config->vlan_12bit_compare ? MACVLANTR_ETV_MSK  : 0U);
    reg |= (config->vlan_tag_invert    ? MACVLANTR_VTIM_MSK : 0U);
    reg |= (config->svlan_enable       ? MACVLANTR_ESVL_MSK : 0U);
    reg |= (config->vlan_hash_filter   ? MACVLANTR_VTHM_MSK : 0U);
    ETH->MACVLANTR = reg;

    /* MACVHTR：VLAN Hash 过滤表 */
    ETH->MACVHTR = (uint32_t)config->vlan_hash_table & 0xFFFFU;
}

void eth_config_l3l4_filter(uint8_t index, const eth_l3l4_filter_config_t *config)
{
    assert(config != NULL);
    assert(index < ETH_L3L4_FILTER_COUNT);

    ETH_TypeDef *eth = eth_periph();

    /* L3L4 控制寄存器 */
    uint32_t ctrl = 0U;
    ctrl |= (config->l3_protocol_ipv6 ? L3L4C_L3PEN_MSK  : 0U);
    ctrl |= (config->l3_sa_match      ? L3L4C_L3SAM_MSK   : 0U);
    ctrl |= (config->l3_sa_inverse    ? L3L4C_L3SAIM_MSK  : 0U);
    ctrl |= (config->l3_da_match      ? L3L4C_L3DAM_MSK   : 0U);
    ctrl |= (config->l3_da_inverse    ? L3L4C_L3DAIM_MSK  : 0U);
    ctrl |= ((uint32_t)config->l3_sa_prefix_len << L3L4C_L3HSBM_POS) & L3L4C_L3HSBM_MSK;
    ctrl |= ((uint32_t)config->l3_da_prefix_len << L3L4C_L3HDBM_POS) & L3L4C_L3HDBM_MSK;
    ctrl |= (config->l4_protocol_udp  ? L3L4C_L4PEN_MSK   : 0U);
    ctrl |= (config->l4_src_match     ? L3L4C_L4SPM_MSK   : 0U);
    ctrl |= (config->l4_src_inverse   ? L3L4C_L4SPIM_MSK  : 0U);
    ctrl |= (config->l4_dst_match     ? L3L4C_L4DPM_MSK   : 0U);
    ctrl |= (config->l4_dst_inverse   ? L3L4C_L4DPIM_MSK  : 0U);
    *get_l3l4_ctrl_reg(eth, index) = ctrl;

    /* L4 端口寄存器 */
    uint32_t l4a = ((uint32_t)config->l4_src_port << L4A_L4SP_POS) & L4A_L4SP_MSK;
    l4a         |= ((uint32_t)config->l4_dst_port << L4A_L4DP_POS) & L4A_L4DP_MSK;
    *get_l4a_reg(eth, index) = l4a;

    /* L3 地址寄存器（IPv4 仅写 word[0]，IPv6 写 word[0]~[3]） */
    uint8_t words = config->l3_protocol_ipv6 ? 4U : 1U;
    for (uint8_t i = 0U; i < words; i++) {
        *get_l3a_reg(eth, index, i) = config->l3_addr[i];
    }
}

void eth_config_mac_addr(uint8_t index, const eth_mac_addr_config_t *config)
{
    assert(config != NULL);
    assert(index < ETH_MAC_ADDR_COUNT);

    uint32_t hr = ((uint32_t)config->addr[0] << 8U) | ((uint32_t)config->addr[1]);
    uint32_t lr = ((uint32_t)config->addr[2] << 24U) | ((uint32_t)config->addr[3] << 16U)
                | ((uint32_t)config->addr[4] << 8U)  |  (uint32_t)config->addr[5];

    if (index == 0U) {
        /* MACA0HR：只有 AE 和地址字段（无 MBC/SA 位） */
        hr |= (config->enable ? (1UL << 31) : 0U);
        ETH->MACA0HR = hr;
        ETH->MACA0LR = lr;
    } else {
        /* MACA1~3HR：有 AE、SA、MBC 位 */
        hr |= (config->enable     ? (1UL << 31) : 0U);
        hr |= (config->src_addr   ? (1UL << 30) : 0U);
        hr |= ((uint32_t)config->mask_bits << 24U) & (0x3FUL << 24U);

        volatile uint32_t *hr_reg;
        volatile uint32_t *lr_reg;
        switch (index) {
            case 1U: hr_reg = &ETH->MACA1HR; lr_reg = &ETH->MACA1LR; break;
            case 2U: hr_reg = &ETH->MACA2HR; lr_reg = &ETH->MACA2LR; break;
            default: hr_reg = &ETH->MACA3HR; lr_reg = &ETH->MACA3LR; break;
        }
        *hr_reg = hr;
        *lr_reg = lr;
    }
}

void eth_config_mmc(const eth_mmc_config_t *config)
{
    assert(config != NULL);

    uint32_t reg = 0U;
    reg |= (config->counter_reset       ? MMCCR_CNTRST_MSK    : 0U);
    reg |= (config->stop_on_rollover    ? MMCCR_CNTSTOPRO_MSK : 0U);
    reg |= (config->reset_on_read       ? MMCCR_RSTONRD_MSK   : 0U);
    reg |= (config->freeze_on_preset    ? MMCCR_CNTFREEZ_MSK  : 0U);
    reg |= (config->counter_preset      ? MMCCR_CNTPRST_MSK   : 0U);
    reg |= (config->counter_preset_full ? MMCCR_CNTPRSTLVL_MSK: 0U);
    reg |= (config->ucast_detect_bcast  ? MMCCR_UCDBC_MSK     : 0U);
    ETH->MMCCR = reg;
}

void eth_config_watchdog_timer(const eth_watchdog_timer_config_t *config)
{
    assert(config != NULL);

    uint32_t reg = 0U;
    reg |= ((uint32_t)config->timeout_val << MACWTR_WTO_POS) & MACWTR_WTO_MSK;
    reg |= (config->prog_wdt_enable ? MACWTR_PWE_MSK : 0U);
    ETH->MACWTR = reg;
}

void eth_set_hash_table(uint32_t hash_high, uint32_t hash_low)
{
    ETH->MACHTHR = hash_high;
    ETH->MACHTLR = hash_low;
}

void eth_set_wake_up_frame(const uint32_t *wuf_regs)
{
    assert(wuf_regs != NULL);

    /* 写 MACRWUFFR 时硬件会按写入顺序自动递增内部地址指针（共 8 个字） */
    for (uint8_t i = 0U; i < 8U; i++) {
        ETH->MACRWUFFR = wuf_regs[i];
    }
}

void eth_config_pmt(bool power_down, bool magic_pkt_en,
                    bool wake_frame_en, bool global_ucast, bool rwffpr)
{
    uint32_t reg = ETH->MACPMTCSR;
    reg &= ~(MACPMTCSR_PD_MSK | MACPMTCSR_MPE_MSK | MACPMTCSR_WFE_MSK
           | MACPMTCSR_GU_MSK | MACPMTCSR_RWFFPR_MSK);
    reg |= (power_down     ? MACPMTCSR_PD_MSK     : 0U);
    reg |= (magic_pkt_en   ? MACPMTCSR_MPE_MSK    : 0U);
    reg |= (wake_frame_en  ? MACPMTCSR_WFE_MSK    : 0U);
    reg |= (global_ucast   ? MACPMTCSR_GU_MSK     : 0U);
    reg |= (rwffpr         ? MACPMTCSR_RWFFPR_MSK : 0U);
    ETH->MACPMTCSR = reg;
}

void eth_set_desc_base(uint32_t tx_desc_addr, uint32_t rx_desc_addr)
{
    ETH->DMATDLAR = tx_desc_addr;
    ETH->DMARDLAR = rx_desc_addr;
}

void eth_set_rx_watchdog_timer(uint8_t riwtc)
{
    ETH->DMARIWTR = (uint32_t)riwtc & 0xFFU;
}

void eth_mac_irq_config(uint32_t it_mask, bool enable)
{
    /* MACIMR：写 1 = 屏蔽（mask），写 0 = 解除 */
    if (enable) {
        /* enable=true：解除屏蔽（中断使能），清对应位 */
        ETH->MACIMR &= ~(it_mask & (MACIMR_PIM_MSK | MACIMR_TIM_MSK | MACIMR_LPIIM_MSK));
    } else {
        /* enable=false：屏蔽（中断禁用），置对应位 */
        ETH->MACIMR |= (it_mask & (MACIMR_PIM_MSK | MACIMR_TIM_MSK | MACIMR_LPIIM_MSK));
    }
}

void eth_dma_irq_config(uint32_t it_mask, bool enable)
{
    if (enable) {
        ETH->DMAIER |= (it_mask & DMAIER_ALL_MSK);
    } else {
        ETH->DMAIER &= ~(it_mask & DMAIER_ALL_MSK);
    }
}

void eth_mmc_rx_irq_config(uint32_t it_mask, bool mask)
{
    if (mask) {
        ETH->MMCRIMR |= it_mask;
    } else {
        ETH->MMCRIMR &= ~it_mask;
    }
}

void eth_mmc_tx_irq_config(uint32_t it_mask, bool mask)
{
    if (mask) {
        ETH->MMCTIMR |= it_mask;
    } else {
        ETH->MMCTIMR &= ~it_mask;
    }
}

//===========================================
// 第3层：高级功能 API
//===========================================

void eth_start_rx(void)
{
    ETH->DMAOMR |= DMAOMR_SR_MSK;
}

void eth_stop_rx(void)
{
    ETH->DMAOMR &= ~DMAOMR_SR_MSK;
}

void eth_start_tx(void)
{
    ETH->DMAOMR |= DMAOMR_ST_MSK;
}

void eth_stop_tx(void)
{
    ETH->DMAOMR &= ~DMAOMR_ST_MSK;
}

void eth_mac_rx_enable(void)
{
    ETH->MACCR |= MACCR_RE_MSK;
}

void eth_mac_rx_disable(void)
{
    ETH->MACCR &= ~MACCR_RE_MSK;
}

void eth_mac_tx_enable(void)
{
    ETH->MACCR |= MACCR_TE_MSK;
}

void eth_mac_tx_disable(void)
{
    ETH->MACCR &= ~MACCR_TE_MSK;
}

bool eth_transmit_frame(eth_tx_desc_t *desc, const uint8_t *buf, uint32_t len)
{
    assert(desc != NULL);
    assert(buf  != NULL);

    /* 检查 DMA 所有权 */
    if (desc->tdes0 & TDES0_OWN_MSK) {
        return false;   /* DMA 尚未释放描述符 */
    }

    /* 填充缓冲区地址和长度（单描述符单片段模式） */
    desc->tdes2 = (uint32_t)buf;
    desc->tdes1 = (desc->tdes1 & ~TDES1_TBS1_MSK) | (len & TDES1_TBS1_MSK);

    /* 设置第一/最后段、中断完成、交还 DMA */
    desc->tdes0 = TDES0_OWN_MSK | TDES0_IC_MSK | TDES0_FS_MSK | TDES0_LS_MSK
                | (desc->tdes0 & (TDES0_TER_MSK | TDES0_TCH_MSK));

    /* 若 TX DMA 进程挂起，写任意值到 DMATPDR 唤醒 */
    if (ETH->DMASR & ETH_DMA_FLAG_TPSS) {
        /* DMATPDR 在设备头文件中声明为只读（实为写立即触发轮询），强制转换写入 */
        *((volatile uint32_t *)&ETH->DMATPDR) = 1U;
    }

    return true;
}

bool eth_receive_frame(eth_rx_desc_t *desc, uint8_t *buf, uint32_t *len)
{
    assert(desc != NULL);
    assert(buf  != NULL);

    /* 检查 DMA 所有权 */
    if (desc->rdes0 & RDES0_OWN_MSK) {
        return false;   /* DMA 尚未写入 */
    }

    /* 检查帧错误汇总位 */
    if (desc->rdes0 & RDES0_ES_MSK) {
        /* 清零 OWN 位，还给 DMA */
        desc->rdes0 = RDES0_OWN_MSK;
        return false;
    }

    /* 读取帧长度（RDES0[29:16]，含 4 字节 CRC） */
    uint32_t frame_len = (desc->rdes0 & RDES0_FL_MSK) >> RDES0_FL_POS;
    if (frame_len >= 4U) {
        frame_len -= 4U;   /* 去除 FCS */
    }

    /* 拷贝数据 */
    uint8_t *src = (uint8_t *)desc->rdes2;
    for (uint32_t i = 0U; i < frame_len; i++) {
        buf[i] = src[i];
    }

    if (len != NULL) {
        *len = frame_len;
    }

    /* 清零 OWN，将描述符还给 DMA */
    desc->rdes0 = RDES0_OWN_MSK;

    /* 若 RX DMA 进程挂起（RBUS/RPSS），写任意值到 DMARPDR 恢复 */
    if (ETH->DMASR & (ETH_DMA_FLAG_RBUS | ETH_DMA_FLAG_RPSS)) {
        *((volatile uint32_t *)&ETH->DMARPDR) = 1U;
    }

    return true;
}

bool eth_phy_read(uint8_t phy_addr, uint8_t reg_addr, uint16_t *data)
{
    assert(data != NULL);

    /* 等待上次操作完成 */
    if (!poll_until_clear(&ETH->MACMIIAR, MACMIIAR_MB_MSK)) {
        return false;
    }

    /* 配置 MACMIIAR：CR + PHY 地址 + 寄存器地址 + MB，MW=0（读） */
    uint32_t miiar = ((uint32_t)s_state.smi_cr << MACMIIAR_CR_POS) & MACMIIAR_CR_MSK;
    miiar |= ((uint32_t)reg_addr << MACMIIAR_MR_POS) & MACMIIAR_MR_MSK;
    miiar |= ((uint32_t)phy_addr << MACMIIAR_PA_POS) & MACMIIAR_PA_MSK;
    miiar |= MACMIIAR_MB_MSK;
    ETH->MACMIIAR = miiar;

    /* 等待操作完成 */
    if (!poll_until_clear(&ETH->MACMIIAR, MACMIIAR_MB_MSK)) {
        return false;
    }

    *data = (uint16_t)(ETH->MACMIIDR & 0xFFFFU);
    return true;
}

bool eth_phy_write(uint8_t phy_addr, uint8_t reg_addr, uint16_t data)
{
    /* 等待上次操作完成 */
    if (!poll_until_clear(&ETH->MACMIIAR, MACMIIAR_MB_MSK)) {
        return false;
    }

    /* 先写数据寄存器 */
    ETH->MACMIIDR = (uint32_t)data;

    /* 配置 MACMIIAR：CR + PHY 地址 + 寄存器地址 + MB + MW=1（写） */
    uint32_t miiar = ((uint32_t)s_state.smi_cr << MACMIIAR_CR_POS) & MACMIIAR_CR_MSK;
    miiar |= ((uint32_t)reg_addr << MACMIIAR_MR_POS) & MACMIIAR_MR_MSK;
    miiar |= ((uint32_t)phy_addr << MACMIIAR_PA_POS) & MACMIIAR_PA_MSK;
    miiar |= MACMIIAR_MW_MSK | MACMIIAR_MB_MSK;
    ETH->MACMIIAR = miiar;

    return poll_until_clear(&ETH->MACMIIAR, MACMIIAR_MB_MSK);
}

void eth_ptp_get_time(eth_ptp_time_t *time)
{
    assert(time != NULL);

    /* 先读高字，再读低字；若读高字后发生进位则重读一次 */
    uint32_t secs1, secs2, nsec;
    secs1 = ETH->PTPTSHR;
    nsec  = ETH->PTPTSLR;
    secs2 = ETH->PTPTSHR;
    if (secs2 != secs1) {
        /* 跨秒，重新读取 */
        nsec = ETH->PTPTSLR;
    }
    time->seconds     = secs2;
    time->nanoseconds = nsec & 0x7FFFFFFFU;  /* bit31 保留/STPNS，取低31位 */
}

void eth_ptp_set_time(const eth_ptp_time_t *time)
{
    assert(time != NULL);

    /* 写入目标时间，AST=0（正值，绝对设置） */
    ETH->PTPTSHUR = time->seconds;
    ETH->PTPTSLUR = time->nanoseconds & 0x7FFFFFFFU;

    /* 触发初始化命令（TI 自清零） */
    ETH->PTPTSCR |= PTPTSCR_TI_MSK;
    /* 等待命令执行完成（TI 自清零） */
    poll_until_clear(&ETH->PTPTSCR, PTPTSCR_TI_MSK);
}

void eth_ptp_update_time(const eth_ptp_time_t *time)
{
    assert(time != NULL);

    uint32_t tslur = time->nanoseconds;
    /* 若 seconds 最高位置 1 表示负偏移，设置 PTPTSLUR.AST（bit31）=1 做减法 */
    if (time->seconds & 0x80000000U) {
        tslur |= PTPTSLUR_AST_MSK;
    }

    ETH->PTPTSHUR = time->seconds;
    ETH->PTPTSLUR = tslur;

    ETH->PTPTSCR |= PTPTSCR_TU_MSK;
    poll_until_clear(&ETH->PTPTSCR, PTPTSCR_TU_MSK);
}

void eth_ptp_adjust_freq(int32_t ppb)
{
    /* new_addend = base_addend + base_addend × ppb / 1e9
     * 使用 64 位中间量避免溢出 */
    uint32_t base  = s_state.ptp_addend;
    int64_t  delta = ((int64_t)base * (int64_t)ppb) / 1000000000LL;
    uint32_t new_addend = (uint32_t)((int64_t)base + delta);

    s_state.ptp_addend = new_addend;
    ETH->PTPTSAR       = new_addend;
    ETH->PTPTSCR      |= PTPTSCR_TARU_MSK;
    poll_until_clear(&ETH->PTPTSCR, PTPTSCR_TARU_MSK);
}

void eth_ptp_set_target_time(const eth_ptp_time_t *time)
{
    assert(time != NULL);

    ETH->PTPTTHR = time->seconds;
    ETH->PTPTTLR = time->nanoseconds & 0x7FFFFFFFU;  /* TTLR[30:0] 纳秒，TTRB[31] 精调/粗调控制位保留待确认 */
}

void eth_ptp_set_pps_output(uint32_t interval_ns, uint32_t width_ns)
{
    ETH->PTPPPSIR = interval_ns;
    ETH->PTPPPSWR = width_ns;
}

void eth_ptp_get_aux_timestamp(eth_ptp_time_t *time)
{
    assert(time != NULL);

    time->nanoseconds = ETH->PTPATSNR & 0x7FFFFFFFU;
    time->seconds     = ETH->PTPATSSR;
}

void eth_mmc_get_stats(eth_mmc_stats_t *stats)
{
    assert(stats != NULL);

    stats->tx_single_collision_gf = ETH->MMCTGFSCCR;
    stats->tx_multi_collision_gf  = ETH->MMCTGFMCCR;
    stats->tx_good_frames         = ETH->MMCTGFCR;
    stats->rx_crc_error           = ETH->MMCRFCECR;
    stats->rx_align_error         = ETH->MMCRFAECR;
    stats->rx_unicast_good_frames = ETH->MMCRGUFCR;
}

//===========================================
// 第4层：控制与查询 API
//===========================================

uint32_t eth_get_dma_status(uint32_t clear_mask)
{
    uint32_t status = ETH->DMASR;
    if (clear_mask != 0U) {
        /* DMASR 中大部分标志写 1 清零（DMASR 在设备头文件中为只读，强制转换写入） */
        *((volatile uint32_t *)&ETH->DMASR) = (status & clear_mask);
    }
    return status;
}

uint32_t eth_get_mac_isr(void)
{
    return ETH->MACISR;  /* 只读，读后对应子模块自行清零 */
}

void eth_get_mac_debug_info(eth_mac_debug_t *info)
{
    assert(info != NULL);

    uint32_t dbg = ETH->MACDBGR;
    info->rx_fifo_fill_level    = (uint8_t)((dbg >> 8U) & 0x3U);  /* RFFL[9:8] */
    info->rx_fifo_read_state    = (uint8_t)((dbg >> 5U) & 0x3U);  /* RFRCS[6:5] */
    info->rx_fifo_write_active  = (dbg & (1UL << 4U)) != 0U;      /* RFWCS */
    info->rx_fifo_overflow      = (dbg & (1UL << 0U)) != 0U;      /* MMRPES */
    info->tx_fifo_not_empty     = (dbg & (1UL << 24U)) != 0U;     /* TFNE */
    info->tx_fifo_full          = (dbg & (1UL << 25U)) != 0U;     /* TFF */
    info->tx_fifo_write_active  = (dbg & (1UL << 22U)) != 0U;     /* TFWA */
    info->tx_mac_active         = (dbg & (1UL << 16U)) != 0U;     /* MMTEA */
    info->tx_frame_ctrl_state   = (uint8_t)((dbg >> 17U) & 0x3U); /* MTFCS[18:17] */
    info->tx_fifo_read_state    = (uint8_t)((dbg >> 20U) & 0x3U); /* TFRS[21:20] */
    info->tx_paused             = (dbg & (1UL << 19U)) != 0U;     /* MTP */
}

void eth_get_dma_missed_frame_count(uint16_t *missed_frame_count,
                                    uint16_t *overflow_frame_count,
                                    uint8_t  *overflow_missed_count)
{
    uint32_t reg = ETH->DMAMFBOCR;  /* 读取后计数器自动清零 */

    if (missed_frame_count   != NULL) {
        *missed_frame_count   = (uint16_t)(reg & 0xFFFFU);         /* MFC[15:0] */
    }
    if (overflow_frame_count != NULL) {
        *overflow_frame_count = (uint16_t)((reg >> 17U) & 0x7FFU); /* OFC[27:17] */
    }
    if (overflow_missed_count != NULL) {
        /* OBMFC[16] 和 OBFOC[28] 溢出标志合并为一个字节 */
        *overflow_missed_count = (uint8_t)(((reg >> 16U) & 0x1U) | (((reg >> 28U) & 0x1U) << 1U));
    }
}

uint32_t eth_get_dma_tx_desc_ptr(void)
{
    return ETH->DMACHTDR;
}

uint32_t eth_get_dma_rx_desc_ptr(void)
{
    return ETH->DMACHRDR;
}

uint32_t eth_get_dma_tx_buf_ptr(void)
{
    return ETH->DMACHTBAR;
}

uint32_t eth_get_dma_rx_buf_ptr(void)
{
    return ETH->DMACHRBAR;
}

uint32_t eth_get_lpi_status(void)
{
    return ETH->MACLPICSR;
}

uint32_t eth_get_ptp_status(void)
{
    return ETH->PTPTSSR;
}

uint32_t eth_get_mmc_rx_isr(void)
{
    return ETH->MMCRIR;
}

uint32_t eth_get_mmc_tx_isr(void)
{
    return ETH->MMCTIR;
}

void eth_irq_register(eth_irq_type_t type,
                      eth_irq_callback_t callback,
                      void *context)
{
    if (type == ETH_IRQ_DMA) {
        s_state.dma_callback = callback;
        s_state.dma_context  = context;
    } else {
        s_state.wkup_callback = callback;
        s_state.wkup_context  = context;
    }
}

//===========================================
// 中断服务程序（ISR）
//===========================================

/**
 * @brief ETH DMA 中断处理程序（IRQn=45）
 *
 * 处理流程：
 * 1. 读取 DMASR，获取所有置位标志
 * 2. 若 AIS（异常汇总）置位，读取具体异常标志
 * 3. 若 NIS（正常汇总，含 TX/RX 完成）置位，通知上层回调
 * 4. 读取 MACISR 处理 MAC 子中断（PTP、LPI、PMT 等）
 * 5. 写 1 清除已处理的 DMASR 标志
 */
void ETH_IRQHandler(void)
{
    uint32_t dmasr = ETH->DMASR;

    /* 读取 MAC 子中断状态（读即清零）*/
    uint32_t macisr = 0U;
    if (dmasr & ((1UL << 27) | (1UL << 28) | (1UL << 29) | (1UL << 30))) {
        macisr = ETH->MACISR;
    }

    /* 调用用户 DMA 回调，传入 DMASR（附加 MACISR 高位） */
    if (s_state.dma_callback != NULL) {
        uint32_t event = dmasr | (macisr << 16U);
        s_state.dma_callback(s_state.dma_context, event);
    }

    /* 清除 DMASR 中可清除的标志（写 1 清零）；
     * RPS[19:17]/TPS[22:20]/EBS[25:23] 为只读位，写入被硬件忽略，无副作用 */
    *((volatile uint32_t *)&ETH->DMASR) = dmasr;
}

/**
 * @brief ETH 唤醒中断处理程序（IRQn=46）
 *
 * 读取 MACPMTCSR，判断 Magic Packet 或唤醒帧事件并通知回调。
 */
void ETH_WKUP_IRQHandler(void)
{
    uint32_t pmtcsr = ETH->MACPMTCSR;

    if (s_state.wkup_callback != NULL) {
        s_state.wkup_callback(s_state.wkup_context, pmtcsr);
    }
}
