/**
 * @file    acm32p4xx.h
 * @brief   CMSIS Cortex-M Peripheral Access Layer Header File for ACM32P4XX
 * @date    2026-06-29
 * @note    Generated from SVD file
 */

#ifndef __ACM32P4XX_H__
#define __ACM32P4XX_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* =========================================================================================================================== */
/* ================                           Processor and Core Peripheral Section                           ================ */
/* =========================================================================================================================== */

/* ========================  Configuration of the SMC1 Processor and Core Peripherals  ========================= */
#define __SMC1_REV           0x0001U      /*!< SMC1 Core Revision */
#define __NVIC_PRIO_BITS          3            /*!< Number of Bits used for Priority Levels */
#define __Vendor_SysTickConfig    0            /*!< Set to 1 if different SysTick Config is used */
#define __VTOR_PRESENT            1            /*!< Set to 1 if CPU supports Vector Table Offset Register */
#define __MPU_PRESENT             1            /*!< MPU present */
#define __FPU_PRESENT             1            /*!< FPU present */
#define __FPU_DP                  0            /*!< Double Precision FPU */
#define __DSP_PRESENT             1            /*!< DSP extension present */
#define __SAUREGION_PRESENT       0            /*!< SAU region present */

/** @} */ /* End of group Configuration_of_CMSIS */

/* ================================================================================ */
/* ================       Interrupt Number Definition             ================ */
/* ================================================================================ */

typedef enum IRQn {
    /******  Cortex-M Processor Exceptions Numbers *******************************************************/
    NonMaskableInt_IRQn         = -14,    /*!< 2 Non Maskable Interrupt */
    HardFault_IRQn              = -13,    /*!< 3 Cortex-M Hard Fault Interrupt */
    MemoryManagement_IRQn       = -12,    /*!< 4 Cortex-M Memory Management Interrupt */
    BusFault_IRQn               = -11,    /*!< 5 Cortex-M Bus Fault Interrupt */
    UsageFault_IRQn             = -10,    /*!< 6 Cortex-M Usage Fault Interrupt */
    SVCall_IRQn                 = -5,     /*!< 11 Cortex-M SV Call Interrupt */
    DebugMonitor_IRQn           = -4,     /*!< 12 Cortex-M Debug Monitor Interrupt */
    PendSV_IRQn                 = -2,     /*!< 14 Cortex-M Pend SV Interrupt */
    SysTick_IRQn                = -1,     /*!< 15 Cortex-M System Tick Interrupt */

    /******  Device Specific Interrupt Numbers ***********************************************************/
    WDT_IRQn                     =   0,    /*!< Watchdog timer interrupt */
    LVD_IRQn                     =   1,    /*!< Power voltage detection (LVD) interrupt connected to EXTI line 16 */
    RTC_XTLSD_IRQn               =   2,    /*!< RTC global interrupt / XTL oscillator stop detection interrupt */
    CLKRDY_INT_IRQn              =   3,    /*!< Clock ready interrupt */
    EXTI0_IRQn                   =   4,    /*!< EXTI line 0 interrupt */
    EXTI1_IRQn                   =   5,    /*!< EXTI line 1 interrupt */
    EXTI2_IRQn                   =   6,    /*!< EXTI line 2 interrupt */
    EXTI3_IRQn                   =   7,    /*!< EXTI line 3 interrupt */
    EXTI4_IRQn                   =   8,    /*!< EXTI line 4 interrupt */
    DMA1_IRQn                    =   9,    /*!< DMA1 global interrupt */
    DMA2_IRQn                    =  10,    /*!< DMA2 global interrupt */
    ADC_IRQn                     =  11,    /*!< ADC global interrupt */
    DAC_IRQn                     =  12,    /*!< DAC global interrupt */
    USB_IRQn                     =  13,    /*!< USB global interrupt */
    FDCAN1_IRQn                  =  14,    /*!< FDCAN1 interrupt */
    EXTI9_5_IRQn                 =  15,    /*!< EXTI lines 5 to 9 interrupt */
    TIM1_BRK_UP_TRG_COM_IRQn     =  16,    /*!< TIM1 break/update/trigger/commutation interrupt */
    TIM1_CC_IRQn                 =  17,    /*!< TIM1 capture compare interrupt */
    TIM2_IRQn                    =  18,    /*!< TIM2 global interrupt */
    TIM3_IRQn                    =  19,    /*!< TIM3 global interrupt */
    TIM6_IRQn                    =  20,    /*!< TIM6 global interrupt */
    TIM7_IRQn                    =  21,    /*!< TIM7 global interrupt */
    TIM8_BRK_UP_TRG_COM_IRQn     =  22,    /*!< TIM8 break/update/trigger/commutation interrupt */
    TIM8_CC_IRQn                 =  23,    /*!< TIM8 capture compare interrupt */
    I2C1_IRQn                    =  24,    /*!< I2C1 interrupt */
    I2C2_IRQn                    =  25,    /*!< I2C2 interrupt */
    SPI1_IRQn                    =  26,    /*!< (Q)SPI1 interrupt */
    SPI2_IRQn                    =  27,    /*!< (Q)SPI2 interrupt */
    SPI3_IRQn                    =  28,    /*!< (Q)SPI3 interrupt */
    SPI4_IRQn                    =  29,    /*!< (Q)SPI4 interrupt */
    I2S1_IRQn                    =  30,    /*!< I2S1 interrupt */
    I2S2_IRQn                    =  31,    /*!< I2S2 interrupt */
    UART1_IRQn                   =  32,    /*!< UART1 interrupt */
    UART2_IRQn                   =  33,    /*!< UART2 interrupt */
    UART3_IRQn                   =  34,    /*!< UART3 interrupt */
    UART4_IRQn                   =  35,    /*!< UART4 interrupt */
    EXTI15_10_IRQn               =  36,    /*!< EXTI lines 10 to 15 interrupt */
    USB_WKUP_IRQn                =  37,    /*!< USB wakeup interrupt */
    LPUART1_IRQn                 =  38,    /*!< LPUART1 interrupt */
    LPUART2_IRQn                 =  39,    /*!< LPUART2 interrupt */
    LPTIM1_IRQn                  =  40,    /*!< LPTIM1 global interrupt */
    TIM4_IRQn                    =  42,    /*!< TIM4 global interrupt */
    IWDT_WKUP_IRQn               =  43,    /*!< IWDT wakeup interrupt connected to EXTI line 18 */
    AES_SPI1_IRQn                =  44,    /*!< AES/SPI1 interrupt */
    ETH_IRQn                     =  45,    /*!< ETH MAC global interrupt */
    ETH_WKUP_IRQn                =  46,    /*!< ETH wakeup interrupt connected to EXTI line 24 */
    UART5_IRQn                   =  47,    /*!< UART5 interrupt */
    UART6_IRQn                   =  48,    /*!< UART6 interrupt */
    UART7_IRQn                   =  49,    /*!< UART7 interrupt */
    UART8_IRQn                   =  50,    /*!< UART8 interrupt */
    TIM5_IRQn                    =  51,    /*!< TIM5 global interrupt */
    TIM9_IRQn                    =  52,    /*!< TIM9 global interrupt */
    TIM10_IRQn                   =  53,    /*!< TIM10 global interrupt */
    TIM26_IRQn                   =  54,    /*!< TIM26 global interrupt */
    SPI7_IRQn                    =  55,    /*!< (Q)SPI7 interrupt */
    OSPI1_IRQn                   =  56,    /*!< OSPI1 interrupt */
    OSPI2_IRQn                   =  57,    /*!< OSPI2 interrupt */
    DMA1_CH0_IRQn                =  58,    /*!< DMA1 channel 0 global interrupt */
    DMA1_CH1_IRQn                =  59,    /*!< DMA1 channel 1 global interrupt */
    DMA1_CH2_IRQn                =  60,    /*!< DMA1 channel 2 global interrupt */
    DMA1_CH3_IRQn                =  61,    /*!< DMA1 channel 3 global interrupt */
    DMA2_CH0_IRQn                =  62,    /*!< DMA2 channel 0 global interrupt */
    DMA2_CH1_IRQn                =  63,    /*!< DMA2 channel 1 global interrupt */
    DMA2_CH2_IRQn                =  64,    /*!< DMA2 channel 2 global interrupt */
    DMA2_CH3_IRQn                =  65,    /*!< DMA2 channel 3 global interrupt */
    BKPSRAM_SEC_IRQn             =  66,    /*!< Backup SRAM ECC single-bit error interrupt */
    BKPSRAM_DED_IRQn             =  67,    /*!< Backup SRAM ECC double-bit error interrupt */
    COMP_IRQn                    =  68,    /*!< Comparator interrupt connected to EXTI line 19 */
    FDCAN2_IRQn                  =  69,    /*!< FDCAN2 interrupt */
    SDMMC_IRQn                   =  72,    /*!< SDMMC global interrupt */
} IRQn_Type;

#include "core_starmc1.h"                    /*!< SMC1 processor and core peripherals */
#include "system_acm32p4xx.h"                      /*!< ACM32P4XX System */

#ifndef __IM                                    /*!< Fallback for older CMSIS versions */
  #define __IM   __I
#endif
#ifndef __OM                                    /*!< Fallback for older CMSIS versions */
  #define __OM   __O
#endif
#ifndef __IOM                                   /*!< Fallback for older CMSIS versions */
  #define __IOM  __IO
#endif

/* ========================================  Start of section using anonymous unions  ======================================== */
#if defined (__CC_ARM)
  #pragma push
  #pragma anon_unions
#elif defined (__ICCARM__)
  #pragma language=extended
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wc11-extensions"
  #pragma clang diagnostic ignored "-Wreserved-id-macro"
  #pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
  #pragma clang diagnostic ignored "-Wnested-anon-types"
#elif defined (__GNUC__)
  /* anonymous unions are enabled by default */
#elif defined (__TMS470__)
  /* anonymous unions are enabled by default */
#elif defined (__TASKING__)
  #pragma warning 586
#elif defined (__CSMC__)
  /* anonymous unions are enabled by default */
#else
  #warning Not supported compiler type
#endif

/* ================================================================================ */
/* ================      Peripheral Structure Definitions         ================ */
/* ================================================================================ */

/**
 * @brief SPI - SPI Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t DAT                 ; /*!< Offset: 0x00 (read-write) Data register - Write to fill TX_FIFO, read to get RX_FIFO data */
        struct {
            __IOM uint32_t DAT                 : 8; /*!< [7..0] Data register - Transmit and receive data for SPI communication */
            __IM  uint32_t          : 24;
        } DAT_f;
    };
    union {
        __IOM uint32_t BAUD                ; /*!< Offset: 0x04 (read-write) Baud rate register - Configures the communication clock frequency */
        struct {
            __IOM uint32_t DIV1                : 8; /*!< [7..0] Clock divider 1 - First stage clock divider value */
            __IOM uint32_t DIV2                : 8; /*!< [15..8] Clock divider 2 - Second stage clock divider value */
            __IM  uint32_t          : 16;
        } BAUD_f;
    };
    union {
        __IOM uint32_t CTL                 ; /*!< Offset: 0x08 (read-write) Control register */
        struct {
            __IOM uint32_t MST_MODE            : 1; /*!< [0..0] Master mode select - 1: SPI works in master mode, 0: SPI works in slave mode */
            __IOM uint32_t SFILTER             : 1; /*!< [1..1] Slave clock glitch filter - 1: Enable slave clock glitch filter, 0: Disable */
            __IOM uint32_t EPS                 : 1; /*!< [2..2] Even parity selection - Selects even/odd or 0/1 parity */
            __IOM uint32_t STP2                : 1; /*!< [3..3] Stop bits selection - Selects 1 or 2 stop bits */
            __IOM uint32_t LSB_FIRST           : 1; /*!< [4..4] MSB/LSB first select - 1: LSB first, 0: MSB first */
            __IOM uint32_t X_MODE              : 2; /*!< [6..5] Transfer mode - 00: 1X mode, 01: 2X mode, 10: 4X mode, 11: Reserved */
            __IOM uint32_t IO_MODE             : 1; /*!< [7..7] IO direction mode - 1: Hardware auto switch, 0: Software switch (FIFO mode only) */
            __IOM uint32_t SLAVE_EN            : 1; /*!< [8..8] Slave TX/RX logic enable - 1: Enable SPI slave function, 0: Disable */
            __IOM uint32_t CS_RST              : 1; /*!< [9..9] Slave CS reset select - 1: Do not reset slave bit counter when CS inactive, 0: Reset */
            __IOM uint32_t CS_FILTER           : 1; /*!< [10..10] Slave CS glitch filter - 1: Enable slave CS glitch filter, 0: Disable */
            __IOM uint32_t CS_TIME             : 8; /*!< [18..11] CS high duration in memory-mapped mode (value + 1 HCLK cycles) */
            __IM  uint32_t          : 1;
            __IOM uint32_t SWCS                : 1; /*!< [20..20] Software CS pin select - CS level in software CS mode (only valid when SWCS_EN=1) */
            __IOM uint32_t SWCS_EN             : 1; /*!< [21..21] Slave CS software mode select - 1: CS software mode, 0: CS hardware mode */
            __IOM uint32_t SID_EN              : 1; /*!< [22..22] Single-line zero-code (SID) enable - 1: Enable, 0: Disable */
            __IM  uint32_t          : 9;
        } CTL_f;
    };
    union {
        __IOM uint32_t TX_CTL              ; /*!< Offset: 0x0C (read-write) Transmit control register - Configures transmitter parameters */
        struct {
            __IOM uint32_t TX_EN               : 1; /*!< [0..0] Transmit enable - Enables the SPI transmitter */
            __IOM uint32_t TX_FIFO_RESET       : 1; /*!< [1..1] Transmit FIFO reset - Resets the transmit FIFO buffer */
            __IOM uint32_t TX_MODE             : 1; /*!< [2..2] Transmit mode - Selects the transmit mode of operation */
            __IOM uint32_t TX_DMA_REQ_EN       : 1; /*!< [3..3] Transmit DMA request enable - Enables DMA requests for transmit */
            __IOM uint32_t TX_DMA_LEVEL        : 4; /*!< [7..4] Transmit DMA level - Configures FIFO level for DMA request generation */
            __IOM uint32_t DUMMY               : 8; /*!< [15..8] Dummy cycles - Configures the number of dummy cycles in read operations */
            __IM  uint32_t          : 16;
        } TX_CTL_f;
    };
    union {
        __IOM uint32_t RX_CTL              ; /*!< Offset: 0x10 (read-write) Receive control register - Configures receiver parameters */
        struct {
            __IOM uint32_t RX_EN               : 1; /*!< [0..0] Receive enable - Enables the SPI receiver */
            __IOM uint32_t RX_FIFO_RESET       : 1; /*!< [1..1] Receive FIFO reset - Resets the receive FIFO buffer */
            __IM  uint32_t          : 1;
            __IOM uint32_t RX_DMA_REQ_EN       : 1; /*!< [3..3] Receive DMA request enable - Enables DMA requests for receive */
            __IOM uint32_t RX_DMA_LEVEL        : 4; /*!< [7..4] Receive DMA level - Configures FIFO level for DMA request generation */
            __IOM uint32_t SSHIFT1             : 2; /*!< [9..8] Sample shift 1 - Configures sampling point shift for data capture */
            __IOM uint32_t MSDA_EN             : 1; /*!< [10..10] Multi-bit SD line enable - Enables multi-bit serial data line operation */
            __IOM uint32_t MSDA_X0             : 1; /*!< [11..11] Multi-bit SD line X0 - Configures multi-bit SD line 0 */
            __IOM uint32_t MSDA_X1             : 1; /*!< [12..12] Multi-bit SD line X1 - Configures multi-bit SD line 1 */
            __IOM uint32_t MSDA_X2             : 1; /*!< [13..13] Multi-bit SD line X2 - Configures multi-bit SD line 2 */
            __IOM uint32_t MSDA_X3             : 1; /*!< [14..14] Multi-bit SD line X3 - Configures multi-bit SD line 3 */
            __IM  uint32_t          : 1;
            __IOM uint32_t MSDHA_X0            : 1; /*!< [16..16] Multi-bit SD hold adjust X0 - Adjusts hold timing for multi-bit SD line 0 */
            __IOM uint32_t MSDHA_X1            : 1; /*!< [17..17] Multi-bit SD hold adjust X1 - Adjusts hold timing for multi-bit SD line 1 */
            __IOM uint32_t MSDHA_X2            : 1; /*!< [18..18] Multi-bit SD hold adjust X2 - Adjusts hold timing for multi-bit SD line 2 */
            __IOM uint32_t MSDHA_X3            : 1; /*!< [19..19] Multi-bit SD hold adjust X3 - Adjusts hold timing for multi-bit SD line 3 */
            __IM  uint32_t          : 4;
            __IOM uint32_t SSHIFT2             : 1; /*!< [24..24] Sample shift 2 - Additional sampling point shift configuration */
            __IM  uint32_t          : 7;
        } RX_CTL_f;
    };
    union {
        __IOM uint32_t IE                  ; /*!< Offset: 0x14 (read-write) Interrupt enable register - Enables or disables various interrupt sources */
        struct {
            __IM  uint32_t          : 1;
            __IOM uint32_t BATCH_DONE_EN       : 1; /*!< [1..1] Batch done interrupt enable - Enables interrupt on batch transfer completion */
            __IOM uint32_t TX_FIFO_EMPTY_EN    : 1; /*!< [2..2] TX FIFO empty interrupt enable - Enables interrupt when transmit FIFO is empty */
            __IOM uint32_t TX_FIFO_FULL_EN     : 1; /*!< [3..3] TX FIFO full interrupt enable - Enables interrupt when transmit FIFO is full */
            __IOM uint32_t RX_FIFO_EMPTY_EN    : 1; /*!< [4..4] RX FIFO empty interrupt enable - Enables interrupt when receive FIFO is empty */
            __IOM uint32_t RX_FIFO_FULL_EN     : 1; /*!< [5..5] RX FIFO full interrupt enable - Enables interrupt when receive FIFO is full */
            __IOM uint32_t TX_FIFO_HALF_EMPTY_EN: 1; /*!< [6..6] TX FIFO half empty interrupt enable - Enables interrupt when TX FIFO is half empty */
            __IOM uint32_t TX_FIFO_HALF_FULL_EN: 1; /*!< [7..7] TX FIFO half full interrupt enable - Enables interrupt when TX FIFO is half full */
            __IOM uint32_t RX_FIFO_HALF_EMPTY_EN: 1; /*!< [8..8] RX FIFO half empty interrupt enable - Enables interrupt when RX FIFO is half empty */
            __IOM uint32_t RX_FIFO_HALF_FULL_EN: 1; /*!< [9..9] RX FIFO half full interrupt enable - Enables interrupt when RX FIFO is half full */
            __IOM uint32_t CS_POS_EN           : 1; /*!< [10..10] Chip select position enable - Enables chip select position detection */
            __IOM uint32_t RX_FIFO_NOT_EMPTY_EN: 1; /*!< [11..11] RX FIFO not empty interrupt enable - Enables interrupt when RX FIFO contains data */
            __IOM uint32_t RX_FIFO_EMPTY_OVERFLOW_EN: 1; /*!< [12..12] RX FIFO empty overflow interrupt enable - Enables interrupt on RX FIFO overflow while empty */
            __IOM uint32_t RX_FIFO_FULL_OVERFLOW_EN: 1; /*!< [13..13] RX FIFO full overflow interrupt enable - Enables interrupt on RX FIFO overflow when full */
            __IOM uint32_t TX_BATCH_DONE_EN    : 1; /*!< [14..14] TX batch done interrupt enable - Enables interrupt on transmit batch completion */
            __IOM uint32_t RX_BATCH_DONE_EN    : 1; /*!< [15..15] RX batch done interrupt enable - Enables interrupt on receive batch completion */
            __IM  uint32_t          : 16;
        } IE_f;
    };
    union {
        __IOM uint32_t STATUS              ; /*!< Offset: 0x18 (read-write) Status register - Indicates the current status of the interface */
        struct {
            __IOM uint32_t BUSY                : 1; /*!< [0..0] Busy flag - Indicates the SPI is currently busy transferring data */
            __IOM uint32_t BATCH_DONE          : 1; /*!< [1..1] Batch done flag - Indicates batch transfer has completed */
            __IOM uint32_t TX_FIFO_EMPTY       : 1; /*!< [2..2] TX FIFO empty flag - Indicates the transmit FIFO is empty */
            __IOM uint32_t TX_FIFO_FULL        : 1; /*!< [3..3] TX FIFO full flag - Indicates the transmit FIFO is full */
            __IOM uint32_t RX_FIFO_EMPTY       : 1; /*!< [4..4] RX FIFO empty flag - Indicates the receive FIFO is empty */
            __IOM uint32_t RX_FIFO_FULL        : 1; /*!< [5..5] RX FIFO full flag - Indicates the receive FIFO is full */
            __IOM uint32_t TX_FIFO_HALF_EMPTY  : 1; /*!< [6..6] TX FIFO half empty flag - Indicates TX FIFO is half empty */
            __IOM uint32_t TX_FIFO_HALF_FULL   : 1; /*!< [7..7] TX FIFO half full flag - Indicates TX FIFO is half full */
            __IOM uint32_t RX_FIFO_HALF_EMPTY  : 1; /*!< [8..8] RX FIFO half empty flag - Indicates RX FIFO is half empty */
            __IOM uint32_t RX_FIFO_HALF_FULL   : 1; /*!< [9..9] RX FIFO half full flag - Indicates RX FIFO is half full */
            __IOM uint32_t CS_POS_FLG          : 1; /*!< [10..10] Chip select position flag - Indicates chip select position status */
            __IOM uint32_t RX_FIFO_NOT_EMPTY   : 1; /*!< [11..11] RX FIFO not empty flag - Indicates the receive FIFO contains data */
            __IOM uint32_t RX_FIFO_EMPTY_OVERFLOW: 1; /*!< [12..12] RX FIFO empty overflow flag - Indicates overflow when RX FIFO was empty */
            __IOM uint32_t RX_FIFO_FULL_OVERFLOW: 1; /*!< [13..13] RX FIFO full overflow flag - Indicates overflow when RX FIFO was full */
            __IOM uint32_t TX_BATCH_DONE       : 1; /*!< [14..14] TX batch done flag - Indicates transmit batch complete */
            __IOM uint32_t RX_BATCH_DONE       : 1; /*!< [15..15] RX batch done flag - Indicates receive batch complete */
            __IOM uint32_t INSTR_SEND_DONE     : 1; /*!< [16..16] Instruction send done flag - Indicates instruction send is complete */
            __IM  uint32_t          : 15;
        } STATUS_f;
    };
    union {
        __IOM uint32_t TXDELAY             ; /*!< Offset: 0x1C (read-write) Transmit delay register - Configures delay timing for transmit operations */
        struct {
            __IOM uint32_t TDY                 : 32; /*!< [31..0] Transmit delay value - Sets the number of clock cycles for transmit delay */
        } TXDELAY_f;
    };
    union {
        __IOM uint32_t BATCH               ; /*!< Offset: 0x20 (read-write) Batch register - Configures batch transfer mode and size */
        struct {
            __IOM uint32_t BATCH_NUMBER        : 32; /*!< [31..0] Batch number - Sets the number of data units in a batch transfer */
        } BATCH_f;
    };
    union {
        __IOM uint32_t CS                  ; /*!< Offset: 0x24 (read-write) Chip select register - Configures chip select signal behavior */
        struct {
            __IOM uint32_t CS                  : 5; /*!< [4..0] Chip select register - Configures chip select signal behavior */
            __IM  uint32_t          : 3;
            __IOM uint32_t CSMAP_EN            : 1; /*!< [8..8] Chip select mapping enable - Enables chip select mapping to GPIO */
            __IM  uint32_t          : 23;
        } CS_f;
    };
    union {
        __IOM uint32_t OUT_EN              ; /*!< Offset: 0x28 (read-write) Output enable register - Enables output drivers for data lines */
        struct {
            __IOM uint32_t MOSI_EN             : 1; /*!< [0..0] MOSI output enable - Enables the MOSI line output driver */
            __IOM uint32_t MISO_EN             : 1; /*!< [1..1] MISO output enable - Enables the MISO line output driver */
            __IOM uint32_t WP_EN               : 1; /*!< [2..2] Write protect enable - Enables the write protect line output driver */
            __IOM uint32_t HOLD_EN             : 1; /*!< [3..3] Hold enable - Enables the hold line output driver */
            __IM  uint32_t          : 28;
        } OUT_EN_f;
    };
    union {
        __IOM uint32_t MEMO_ACC            ; /*!< Offset: 0x2C (read-write) Memory access register - Configures memory access mode and timing */
        struct {
            __IOM uint32_t ACC_EN              : 1; /*!< [0..0] Access enable - Enables memory access mode */
            __IOM uint32_t CS_TOUT_EN          : 1; /*!< [1..1] Chip select timeout enable - Enables chip select timeout detection */
            __IM  uint32_t          : 1;
            __IOM uint32_t CON_MODE_EN         : 1; /*!< [3..3] Continuous mode enable - Enables continuous transfer mode */
            __IOM uint32_t SEND_INSTR_ONCE_EN  : 1; /*!< [4..4] Send instruction once enable - Sends instruction only at start of continuous transfer */
            __IOM uint32_t WR_AB_EN            : 1; /*!< [5..5] Write alternate byte enable - Enables alternate byte during write operations */
            __IOM uint32_t RD_AB_EN            : 1; /*!< [6..6] Read alternate byte enable - Enables alternate byte during read operations */
            __IOM uint32_t ALTER_BYTE_SIZE     : 2; /*!< [8..7] Alternate byte size - Configures the size of the alternate byte field */
            __IM  uint32_t          : 1;
            __IOM uint32_t WR_DB_EN            : 1; /*!< [10..10] Write dummy byte enable - Enables dummy byte during write operations */
            __IOM uint32_t RD_DB_EN            : 1; /*!< [11..11] Read dummy byte enable - Enables dummy byte during read operations */
            __IOM uint32_t DUMMY_CYCLE_SIZE    : 3; /*!< [14..12] Dummy cycle size - Configures the number of dummy cycles */
            __IM  uint32_t          : 2;
            __IOM uint32_t ADDR_WIDTH          : 2; /*!< [18..17] Address width - Configures the address width (16/24/32-bit) */
            __IOM uint32_t INSTR_MODE          : 2; /*!< [20..19] Instruction mode - Selects the instruction phase mode */
            __IOM uint32_t ADDR_MODE           : 2; /*!< [22..21] Address mode - Selects the address phase mode */
            __IOM uint32_t ALTER_BYTE_MODE     : 2; /*!< [24..23] Alternate byte mode - Selects the alternate byte phase mode */
            __IOM uint32_t DATA_MODE           : 2; /*!< [26..25] Data mode - Selects the data phase mode */
            __IOM uint32_t ONCE_INSTR_CLR      : 1; /*!< [27..27] Once instruction clear - Clears the single-instruction mode flag */
            __IM  uint32_t          : 4;
        } MEMO_ACC_f;
    };
    union {
        __IOM uint32_t CMD                 ; /*!< Offset: 0x30 (read-write) Command register - Sets the command for memory access operations */
        struct {
            __IOM uint32_t RD_CMD              : 8; /*!< [7..0] Read command - Sets the command byte for read operations */
            __IOM uint32_t WR_CMD              : 8; /*!< [15..8] Write command - Sets the command byte for write operations */
            __IM  uint32_t          : 16;
        } CMD_f;
    };
    union {
        __IOM uint32_t ALTER_BYTE          ; /*!< Offset: 0x34 (read-write) Alternate byte register - Sets the alternate byte value for memory access */
        struct {
            __IOM uint32_t ALTER_BYTE          : 32; /*!< [31..0] Alternate byte register - Sets the alternate byte value for memory access */
        } ALTER_BYTE_f;
    };
    union {
        __IOM uint32_t CS_TOUT_VAL         ; /*!< Offset: 0x38 (read-write) CS low timeout value - Forces CS high after timeout */
        struct {
            __IOM uint32_t CS_TOUT_VAL         : 16; /*!< [15..0] CS low timeout count value */
            __IM  uint32_t          : 16;
        } CS_TOUT_VAL_f;
    };
    union {
        __IOM uint32_t MEMO_ACC2           ; /*!< Offset: 0x3C (read-write) Memory access control register 2 */
        struct {
            __IOM uint32_t WRPS                : 3; /*!< [2..0] Wrap size - 000: Disabled, 010: 16B, 011: 32B, 100: 64B */
            __IOM uint32_t BL                  : 4; /*!< [6..3] Burst length - 000: No boundary, 001: 16B, 010: 32B, 011: 64B, 100: 128B, 101: 256B, 110: 512B, 111: 1KB */
            __IOM uint32_t WP                  : 1; /*!< [7..7] Write protect - 1: Enable memory-mapped write protection, 0: Disable */
            __IM  uint32_t          : 24;
        } MEMO_ACC2_f;
    };
    union {
        __IOM uint32_t RGB_DATA            ; /*!< Offset: 0x40 (read-write) RGB data register (valid only when SID_EN is enabled) */
        struct {
            __IOM uint32_t RGB_DATA            : 8; /*!< [7..0] RGB data value */
            __IM  uint32_t          : 24;
        } RGB_DATA_f;
    };
    union {
        __IOM uint32_t SID_RESET_VAL       ; /*!< Offset: 0x44 (read-write) SID reset counter value (valid only when SID_EN is enabled) */
        struct {
            __IOM uint32_t RESET_VAL           : 24; /*!< [23..0] Reset code counter value (counted in HCLK cycles) */
            __IM  uint32_t          : 8;
        } SID_RESET_VAL_f;
    };
} SPI_TypeDef;

/**
 * @brief OSPI - OSPI Register Structure
 */
typedef struct {
    union {
        union {
            __OM  uint32_t TX_DAT              ; /*!< Offset: 0x00 (write-only) Transmit data register - Data to be transmitted */
            struct {
                __OM  uint32_t TX_DAT              : 32; /*!< [31..0] TX_DAT0 - Transmit data byte 0 */
            } TX_DAT_f;
        };
        union {
            __IM  uint32_t RX_DAT              ; /*!< Offset: 0x00 (read-only) Receive data register - Data received */
            struct {
                __IM  uint32_t RX_DAT              : 32; /*!< [31..0] Receive data register - Data received via OSPI */
            } RX_DAT_f;
        };
    };
    union {
        __IOM uint32_t BAUD                ; /*!< Offset: 0x04 (read-write) Baud rate register - Configures the communication clock frequency */
        struct {
            __IOM uint32_t DIV1                : 8; /*!< [7..0] Clock divider 1 - First stage clock divider value */
            __IOM uint32_t DIV2                : 8; /*!< [15..8] Clock divider 2 - Second stage clock divider value */
            __IM  uint32_t          : 16;
        } BAUD_f;
    };
    union {
        __IOM uint32_t CTL                 ; /*!< Offset: 0x08 (read-write) Control register - Watchdog timer control configuration */
        struct {
            __IOM uint32_t MST_MODE            : 1; /*!< [0..0] Master mode select - Configures the SPI as master or slave */
            __IM  uint32_t          : 1;
            __IOM uint32_t CPHA                : 1; /*!< [2..2] Clock phase - Selects the clock phase for data sampling */
            __IOM uint32_t CPOL                : 1; /*!< [3..3] Clock polarity - Selects the idle state clock level */
            __IOM uint32_t LSB_FIRST           : 1; /*!< [4..4] LSB first mode - When set, data is transmitted LSB first instead of MSB first */
            __IOM uint32_t X_MODE              : 2; /*!< [6..5] Transfer mode - Selects the SPI transfer mode (Standard/Dual/Quad) */
            __IOM uint32_t IO_MODE             : 1; /*!< [7..7] I/O mode - Selects the I/O mode for data transfer */
            __IOM uint32_t DTRM                : 1; /*!< [8..8] Dual transfer rate mode - Enables double data rate operation */
            __IOM uint32_t DQSOE               : 1; /*!< [9..9] DQS output enable - Enables the DQS strobe signal output */
            __IM  uint32_t          : 1;
            __IOM uint32_t CS_TIME             : 8; /*!< [18..11] Chip select time - Configures chip select timing parameters */
            __IM  uint32_t          : 2;
            __IOM uint32_t MEM_MODE            : 2; /*!< [22..21] Memory mode - Selects the memory access protocol (SPI/HyperBus) */
            __IOM uint32_t APMD_CLK            : 3; /*!< [25..23] APM clock divider - Configures the clock divider for automatic poll mode */
            __IOM uint32_t DM_EN               : 1; /*!< [26..26] Data mask enable - Enables data mask signal for OSPI */
            __IOM uint32_t DMCTRL              : 1; /*!< [27..27] Data mask control - Configures data mask timing and behavior */
            __IOM uint32_t FW_MODE             : 2; /*!< [29..28] Fixed word mode - Enables fixed word length transfers */
            __IOM uint32_t FR_MODE             : 2; /*!< [31..30] Fixed read mode - Enables fixed read length transfers */
        } CTL_f;
    };
    union {
        __IOM uint32_t TX_CTL              ; /*!< Offset: 0x0C (read-write) Transmit control register - Configures transmitter parameters */
        struct {
            __IOM uint32_t TX_EN               : 1; /*!< [0..0] Transmit enable - Enables the SPI transmitter */
            __IOM uint32_t TX_FIFO_RESET       : 1; /*!< [1..1] Transmit FIFO reset - Resets the transmit FIFO buffer */
            __IM  uint32_t          : 1;
            __IOM uint32_t TX_DMA_REQ_EN       : 1; /*!< [3..3] Transmit DMA request enable - Enables DMA requests for transmit */
            __IOM uint32_t TX_DMA_LEVEL        : 4; /*!< [7..4] Transmit DMA level - Configures FIFO level for DMA request generation */
            __IOM uint32_t DUMMY               : 8; /*!< [15..8] Dummy cycles - Configures the number of dummy cycles in read operations */
            __IOM uint32_t OUTDLY              : 2; /*!< [17..16] Output delay - Configures output data delay for timing adjustment */
            __IM  uint32_t          : 14;
        } TX_CTL_f;
    };
    union {
        __IOM uint32_t RX_CTL              ; /*!< Offset: 0x10 (read-write) Receive control register - Configures receiver parameters */
        struct {
            __IOM uint32_t RX_EN               : 1; /*!< [0..0] Receive enable - Enables the SPI receiver */
            __IOM uint32_t RX_FIFO_RESET       : 1; /*!< [1..1] Receive FIFO reset - Resets the receive FIFO buffer */
            __IM  uint32_t          : 1;
            __IOM uint32_t RX_DMA_REQ_EN       : 1; /*!< [3..3] Receive DMA request enable - Enables DMA requests for receive */
            __IOM uint32_t RX_DMA_LEVEL        : 4; /*!< [7..4] Receive DMA level - Configures FIFO level for DMA request generation */
            __IOM uint32_t DQS_SAMP_EN         : 1; /*!< [8..8] DQS sampling enable - Enables data strobe based sampling */
            __IM  uint32_t          : 1;
            __IOM uint32_t MSDA_EN             : 1; /*!< [10..10] Multi-bit SD line enable - Enables multi-bit serial data line operation */
            __IOM uint32_t MSDA_X0             : 1; /*!< [11..11] Multi-bit SD line X0 - Configures multi-bit SD line 0 */
            __IOM uint32_t MSDA_X1             : 1; /*!< [12..12] Multi-bit SD line X1 - Configures multi-bit SD line 1 */
            __IOM uint32_t MSDA_X2             : 1; /*!< [13..13] Multi-bit SD line X2 - Configures multi-bit SD line 2 */
            __IOM uint32_t MSDA_X3             : 1; /*!< [14..14] Multi-bit SD line X3 - Configures multi-bit SD line 3 */
            __IOM uint32_t MSDA_X4             : 1; /*!< [15..15] Multi-bit SD line X4 - Configures multi-bit SD line 4 */
            __IOM uint32_t MSDA_X5             : 1; /*!< [16..16] Multi-bit SD line X5 - Configures multi-bit SD line 5 */
            __IOM uint32_t MSDA_X6             : 1; /*!< [17..17] Multi-bit SD line X6 - Configures multi-bit SD line 6 */
            __IOM uint32_t MSDA_X7             : 1; /*!< [18..18] Multi-bit SD line X7 - Configures multi-bit SD line 7 */
            __IOM uint32_t MSDHA_X0            : 1; /*!< [19..19] Multi-bit SD hold adjust X0 - Adjusts hold timing for multi-bit SD line 0 */
            __IOM uint32_t MSDHA_X1            : 1; /*!< [20..20] Multi-bit SD hold adjust X1 - Adjusts hold timing for multi-bit SD line 1 */
            __IOM uint32_t MSDHA_X2            : 1; /*!< [21..21] Multi-bit SD hold adjust X2 - Adjusts hold timing for multi-bit SD line 2 */
            __IOM uint32_t MSDHA_X3            : 1; /*!< [22..22] Multi-bit SD hold adjust X3 - Adjusts hold timing for multi-bit SD line 3 */
            __IOM uint32_t MSDHA_X4            : 1; /*!< [23..23] Multi-bit SD hold adjust X4 - Adjusts hold timing for multi-bit SD line 4 */
            __IOM uint32_t MSDHA_X5            : 1; /*!< [24..24] Multi-bit SD hold adjust X5 - Adjusts hold timing for multi-bit SD line 5 */
            __IOM uint32_t MSDHA_X6            : 1; /*!< [25..25] Multi-bit SD hold adjust X6 - Adjusts hold timing for multi-bit SD line 6 */
            __IOM uint32_t MSDHA_X7            : 1; /*!< [26..26] Multi-bit SD hold adjust X7 - Adjusts hold timing for multi-bit SD line 7 */
            __IM  uint32_t          : 1;
            __IOM uint32_t SSHIFT              : 4; /*!< [31..28] Sample shift - Configures sampling point shift for data capture */
        } RX_CTL_f;
    };
    union {
        __IOM uint32_t IE                  ; /*!< Offset: 0x14 (read-write) Interrupt enable register - Enables or disables various interrupt sources */
        struct {
            __IM  uint32_t          : 1;
            __IOM uint32_t BATCH_DONE_EN       : 1; /*!< [1..1] Batch done interrupt enable - Enables interrupt on batch transfer completion */
            __IOM uint32_t TX_FIFO_EMPTY_EN    : 1; /*!< [2..2] TX FIFO empty interrupt enable - Enables interrupt when transmit FIFO is empty */
            __IOM uint32_t TX_FIFO_FULL_EN     : 1; /*!< [3..3] TX FIFO full interrupt enable - Enables interrupt when transmit FIFO is full */
            __IOM uint32_t RX_FIFO_EMPTY_EN    : 1; /*!< [4..4] RX FIFO empty interrupt enable - Enables interrupt when receive FIFO is empty */
            __IOM uint32_t RX_FIFO_FULL_EN     : 1; /*!< [5..5] RX FIFO full interrupt enable - Enables interrupt when receive FIFO is full */
            __IOM uint32_t TX_FIFO_HALF_EMPTY_EN: 1; /*!< [6..6] TX FIFO half empty interrupt enable - Enables interrupt when TX FIFO is half empty */
            __IOM uint32_t TX_FIFO_HALF_FULL_EN: 1; /*!< [7..7] TX FIFO half full interrupt enable - Enables interrupt when TX FIFO is half full */
            __IOM uint32_t RX_FIFO_HALF_EMPTY_EN: 1; /*!< [8..8] RX FIFO half empty interrupt enable - Enables interrupt when RX FIFO is half empty */
            __IOM uint32_t RX_FIFO_HALF_FULL_EN: 1; /*!< [9..9] RX FIFO half full interrupt enable - Enables interrupt when RX FIFO is half full */
            __IM  uint32_t          : 1;
            __IOM uint32_t RX_FIFO_NOT_EMPTY_EN: 1; /*!< [11..11] RX FIFO not empty interrupt enable - Enables interrupt when RX FIFO contains data */
            __IM  uint32_t          : 2;
            __IOM uint32_t TX_BATCH_DONE_EN    : 1; /*!< [14..14] TX batch done interrupt enable - Enables interrupt on transmit batch completion */
            __IOM uint32_t RX_BATCH_DONE_EN    : 1; /*!< [15..15] RX batch done interrupt enable - Enables interrupt on receive batch completion */
            __IM  uint32_t          : 16;
        } IE_f;
    };
    union {
        __IOM uint32_t STATUS              ; /*!< Offset: 0x18 (read-write) Status register - Indicates the current status of the interface */
        struct {
            __IOM uint32_t TX_BUSY             : 1; /*!< [0..0] Transmit busy flag - Indicates transmit operation in progress */
            __IOM uint32_t BATCH_DONE          : 1; /*!< [1..1] Batch done flag - Indicates batch transfer has completed */
            __IOM uint32_t TX_FIFO_EMPTY       : 1; /*!< [2..2] TX FIFO empty flag - Indicates the transmit FIFO is empty */
            __IOM uint32_t TX_FIFO_FULL        : 1; /*!< [3..3] TX FIFO full flag - Indicates the transmit FIFO is full */
            __IOM uint32_t RX_FIFO_EMPTY       : 1; /*!< [4..4] RX FIFO empty flag - Indicates the receive FIFO is empty */
            __IOM uint32_t RX_FIFO_FULL        : 1; /*!< [5..5] RX FIFO full flag - Indicates the receive FIFO is full */
            __IOM uint32_t TX_FIFO_HALF_EMPTY  : 1; /*!< [6..6] TX FIFO half empty flag - Indicates TX FIFO is half empty */
            __IOM uint32_t TX_FIFO_HALF_FULL   : 1; /*!< [7..7] TX FIFO half full flag - Indicates TX FIFO is half full */
            __IOM uint32_t RX_FIFO_HALF_EMPTY  : 1; /*!< [8..8] RX FIFO half empty flag - Indicates RX FIFO is half empty */
            __IOM uint32_t RX_FIFO_HALF_FULL   : 1; /*!< [9..9] RX FIFO half full flag - Indicates RX FIFO is half full */
            __IM  uint32_t          : 1;
            __IOM uint32_t RX_FIFO_NOT_EMPTY   : 1; /*!< [11..11] RX FIFO not empty flag - Indicates the receive FIFO contains data */
            __IM  uint32_t          : 2;
            __IOM uint32_t TX_BATCH_DONE       : 1; /*!< [14..14] TX batch done flag - Indicates transmit batch complete */
            __IOM uint32_t RX_BATCH_DONE       : 1; /*!< [15..15] RX batch done flag - Indicates receive batch complete */
            __IOM uint32_t APM_DUMY_DONE       : 1; /*!< [16..16] APM dummy done flag - Indicates automatic poll mode dummy cycle complete */
            __IOM uint32_t RWDS                : 1; /*!< [17..17] Read-write data strobe - Indicates read/write DQS status */
            __IOM uint32_t INSTR_SEND_DONE     : 1; /*!< [18..18] Instruction send done flag - Indicates instruction send is complete */
            __IM  uint32_t          : 13;
        } STATUS_f;
    };
    union {
        __IOM uint32_t TXDELAY             ; /*!< Offset: 0x1C (read-write) Transmit delay register - Configures delay timing for transmit operations */
        struct {
            __IOM uint32_t TDY                 : 32; /*!< [31..0] Transmit delay value - Sets the number of clock cycles for transmit delay */
        } TXDELAY_f;
    };
    union {
        __IOM uint32_t BATCH               ; /*!< Offset: 0x20 (read-write) Batch register - Configures batch transfer mode and size */
        struct {
            __IOM uint32_t BATCH_NUMBER        : 32; /*!< [31..0] Batch number - Sets the number of data units in a batch transfer */
        } BATCH_f;
    };
    union {
        __IOM uint32_t CS                  ; /*!< Offset: 0x24 (read-write) Chip select register - Configures chip select signal behavior */
        struct {
            __IOM uint32_t SPI_CS              : 3; /*!< [2..0] SPI chip select - Controls SPI chip select signal */
            __IOM uint32_t CSMAP_EN            : 1; /*!< [3..3] Chip select mapping enable - Enables chip select mapping to GPIO */
            __IM  uint32_t          : 28;
        } CS_f;
    };
    union {
        __IOM uint32_t OUT_EN              ; /*!< Offset: 0x28 (read-write) Output enable register - Enables output drivers for data lines */
        struct {
            __IOM uint32_t MOSI_EN             : 1; /*!< [0..0] MOSI output enable - Enables the MOSI line output driver */
            __IOM uint32_t MISO_EN             : 1; /*!< [1..1] MISO output enable - Enables the MISO line output driver */
            __IOM uint32_t WP_EN               : 1; /*!< [2..2] Write protect enable - Enables the write protect line output driver */
            __IOM uint32_t HOLD_EN             : 1; /*!< [3..3] Hold enable - Enables the hold line output driver */
            __IOM uint32_t IO4_EN              : 1; /*!< [4..4] IO4 output enable - Enables the IO4 line output driver */
            __IOM uint32_t IO5_EN              : 1; /*!< [5..5] IO5 output enable - Enables the IO5 line output driver */
            __IOM uint32_t IO6_EN              : 1; /*!< [6..6] IO6 output enable - Enables the IO6 line output driver */
            __IOM uint32_t IO7_EN              : 1; /*!< [7..7] IO7 output enable - Enables the IO7 line output driver */
            __IM  uint32_t          : 24;
        } OUT_EN_f;
    };
    union {
        __IOM uint32_t MEMO_ACC1           ; /*!< Offset: 0x2C (read-write) Memory access register 1 - Configures memory access parameters */
        struct {
            __IOM uint32_t ACC_EN              : 1; /*!< [0..0] SPI_ACC_EN - SPI access enable */
            __IOM uint32_t CS_TOUT_EN          : 1; /*!< [1..1] Chip select timeout enable - Enables chip select timeout detection */
            __IM  uint32_t          : 1;
            __IOM uint32_t CON_MODE_EN         : 1; /*!< [3..3] Continuous mode enable - Enables continuous transfer mode */
            __IOM uint32_t SEND_INSTR_ONCE_EN  : 1; /*!< [4..4] Send instruction once enable - Sends instruction only at start of continuous transfer */
            __IOM uint32_t WR_AB_EN            : 1; /*!< [5..5] Write alternate byte enable - Enables alternate byte during write operations */
            __IOM uint32_t RD_AB_EN            : 1; /*!< [6..6] Read alternate byte enable - Enables alternate byte during read operations */
            __IOM uint32_t ALTER_BYTE_SIZE     : 2; /*!< [8..7] Alternate byte size - Configures the size of the alternate byte field */
            __IM  uint32_t          : 1;
            __IOM uint32_t WR_DB_EN            : 1; /*!< [10..10] Write dummy byte enable - Enables dummy byte during write operations */
            __IOM uint32_t RD_DB_EN            : 1; /*!< [11..11] Read dummy byte enable - Enables dummy byte during read operations */
            __IOM uint32_t DUMMY_CYCLE_SIZE    : 5; /*!< [16..12] Dummy cycle size - Configures the number of dummy cycles */
            __IOM uint32_t ADDR_WIDTH          : 2; /*!< [18..17] Address width - Configures the address width (16/24/32-bit) */
            __IOM uint32_t INSTR_MODE          : 2; /*!< [20..19] Instruction mode - Selects the instruction phase mode */
            __IOM uint32_t ADDR_MODE           : 2; /*!< [22..21] Address mode - Selects the address phase mode */
            __IOM uint32_t ALTER_BYTE_MODE     : 2; /*!< [24..23] Alternate byte mode - Selects the alternate byte phase mode */
            __IOM uint32_t DATA_MODE           : 2; /*!< [26..25] Data mode - Selects the data phase mode */
            __IOM uint32_t HYPER_BT            : 1; /*!< [27..27] HyperBus mode - Enables HyperBus protocol for memory access */
            __IM  uint32_t          : 1;
            __IOM uint32_t ONCE_INSTR_CLR      : 1; /*!< [29..29] Once instruction clear - Clears the single-instruction mode flag */
            __IM  uint32_t          : 2;
        } MEMO_ACC1_f;
    };
    union {
        __IOM uint32_t CMD                 ; /*!< Offset: 0x30 (read-write) Command register - Sets the command for memory access operations */
        struct {
            __IOM uint32_t RD_CMD              : 16; /*!< [15..0] Read command - Sets the command byte for read operations */
            __IOM uint32_t WR_CMD              : 16; /*!< [31..16] Write command - Sets the command byte for write operations */
        } CMD_f;
    };
    union {
        __IOM uint32_t ALTER_BYTE          ; /*!< Offset: 0x34 (read-write) Alternate byte register - Sets the alternate byte value for memory access */
        struct {
            __IOM uint32_t ALTER_BYTE          : 32; /*!< [31..0] Alternate byte register - Sets the alternate byte value for memory access */
        } ALTER_BYTE_f;
    };
    union {
        __IOM uint32_t CS_TOUT_VAL         ; /*!< Offset: 0x38 (read-write) Chip select timeout value - Sets the timeout period for chip select */
        struct {
            __IOM uint32_t CS_TOUT_VAL         : 16; /*!< [15..0] Chip select timeout value - Sets the timeout period for chip select */
            __IOM uint32_t CS_DELAY_VAL        : 16; /*!< [31..16] Chip select delay value - Sets the delay for chip select assertion */
        } CS_TOUT_VAL_f;
    };
    union {
        __IOM uint32_t MEMO_ACC2           ; /*!< Offset: 0x3C (read-write) Memory access register 2 - Additional memory access configuration */
        struct {
            __IOM uint32_t HXSPI_LC0           : 4; /*!< [3..0] HyperBus latency control 0 - Configures latency for HyperBus protocol */
            __IOM uint32_t HXSPI_LC1           : 4; /*!< [7..4] HyperBus latency control 1 - Additional latency configuration for HyperBus */
            __IOM uint32_t RBL                 : 4; /*!< [11..8] Read burst length - Configures the read burst length */
            __IOM uint32_t WRPS                : 3; /*!< [14..12] Write prefetch size - Configures write prefetch buffer size */
            __IOM uint32_t WBL                 : 4; /*!< [18..15] Write burst length - Configures the write burst length */
            __IOM uint32_t DUMMYOFF_EN         : 1; /*!< [19..19] Dummy off enable - Enables dummy cycle elimination */
            __IOM uint32_t RDUMMYOFF_LC        : 6; /*!< [25..20] Read dummy off latency - Configures latency when read dummy is off */
            __IOM uint32_t DQS_EN              : 1; /*!< [26..26] DQS enable - Enables data strobe signal */
            __IOM uint32_t WLC                 : 5; /*!< [31..27] Write latency control - Configures write latency parameters */
        } MEMO_ACC2_f;
    };
    union {
        __IOM uint32_t MEMO_ACC3           ; /*!< Offset: 0x40 (read-write) Memory access register 3 - Extended memory access configuration */
        struct {
            __IOM uint32_t CMD_SD              : 1; /*!< [0..0] Command serial data - Sets serial data for commands */
            __IOM uint32_t CMD_BYTE            : 1; /*!< [1..1] Command byte - Configures the command byte format */
            __IOM uint32_t WDUMMYOFF_LC        : 6; /*!< [7..2] Write dummy off latency - Configures latency when write dummy is off */
            __IM  uint32_t          : 24;
        } MEMO_ACC3_f;
    };
} OSPI_TypeDef;

/**
 * @brief TIM1 - TIM1 Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t CR1                 ; /*!< Offset: 0x00 (read-write) Control register 1 - UART control and configuration */
        struct {
            __IOM uint32_t CEN                 : 1; /*!< [0..0] Counter enable - Enables or disables the timer counter */
            __IOM uint32_t UDIS                : 1; /*!< [1..1] Update disable - Disables update event generation */
            __IOM uint32_t URS                 : 1; /*!< [2..2] Update request source - Selects the update event source */
            __IOM uint32_t OPM                 : 1; /*!< [3..3] One-pulse mode - Enables one-pulse mode operation */
            __IOM uint32_t DIR                 : 1; /*!< [4..4] Direction - Selects counter direction (up/down counting) */
            __IOM uint32_t CMS                 : 2; /*!< [6..5] Center-aligned mode select - Selects center-aligned PWM mode */
            __IOM uint32_t ARPE                : 1; /*!< [7..7] Auto-reload preload enable - Enables auto-reload register buffering */
            __IOM uint32_t CKD                 : 2; /*!< [9..8] Clock division - Configures clock division factor for dead-time and filter */
            __IOM uint32_t BKF                 : 4; /*!< [13..10] Break filter - Configures input filter for break signal */
            __IOM uint32_t OC_SEL              : 1; /*!< [14..14] Output compare selection - Selects output compare mode */
            __IM  uint32_t          : 17;
        } CR1_f;
    };
    union {
        __IOM uint32_t CR2                 ; /*!< Offset: 0x04 (read-write) Control register 2 - Timer additional control */
        struct {
            __IOM uint32_t CCPC                : 1; /*!< [0..0] Capture/compare preloaded control - Enables shadow register update on COM */
            __IM  uint32_t          : 1;
            __IOM uint32_t CCUS                : 1; /*!< [2..2] Capture/compare update selection - Selects when shadow registers update */
            __IOM uint32_t CCDS                : 1; /*!< [3..3] Capture/compare DMA selection - Selects DMA request source */
            __IOM uint32_t MMS                 : 3; /*!< [6..4] Master mode selection - Selects the trigger output (TRGO) source */
            __IOM uint32_t TI1S                : 1; /*!< [7..7] TIMx input 1 selection - Selects input 1 source for timer */
            __IOM uint32_t OIS1                : 1; /*!< [8..8] Output idle state 1 - Sets output state of channel 1 in idle */
            __IOM uint32_t OIS1N               : 1; /*!< [9..9] Output idle state 1N - Sets complementary output state of channel 1 in idle */
            __IOM uint32_t OIS2                : 1; /*!< [10..10] Output idle state 2 - Sets output state of channel 2 in idle */
            __IOM uint32_t OIS2N               : 1; /*!< [11..11] Output idle state 2N - Sets complementary output state of channel 2 in idle */
            __IOM uint32_t OIS3                : 1; /*!< [12..12] Output idle state 3 - Sets output state of channel 3 in idle */
            __IOM uint32_t OIS3N               : 1; /*!< [13..13] Output idle state 3N - Sets complementary output state of channel 3 in idle */
            __IOM uint32_t OIS4                : 1; /*!< [14..14] Output idle state 4 - Sets output state of channel 4 in idle */
            __IOM uint32_t OIS4N               : 1; /*!< [15..15] Output idle state 4N - Sets complementary output state of channel 4 in idle */
            __IOM uint32_t OIS5                : 1; /*!< [16..16] Output idle state 5 - Sets output state of channel 5 in idle */
            __IM  uint32_t          : 1;
            __IOM uint32_t OIS6                : 1; /*!< [18..18] Output idle state 6 - Sets output state of channel 6 in idle */
            __IM  uint32_t          : 1;
            __IOM uint32_t MMS2                : 4; /*!< [23..20] Master mode selection 2 - Secondary master mode selection for TRGO2 */
            __IM  uint32_t          : 8;
        } CR2_f;
    };
    union {
        __IOM uint32_t SMCR                ; /*!< Offset: 0x08 (read-write) Slave mode control register - Configures timer synchronization and slave mode */
        struct {
            __IOM uint32_t SMS                 : 3; /*!< [2..0] Slave mode selection - Selects timer slave mode */
            __IOM uint32_t OCCS                : 1; /*!< [3..3] Output compare clock selection - Selects clock source for output compare */
            __IOM uint32_t TS_L                : 3; /*!< [6..4] Trigger selection (low bits) - Selects trigger input source bits [2:0] */
            __IOM uint32_t MSM                 : 1; /*!< [7..7] Master-slave mode - Enables master-slave synchronization mode */
            __IOM uint32_t ETF                 : 4; /*!< [11..8] External trigger filter - Configures filter for external trigger */
            __IOM uint32_t ETPS                : 2; /*!< [13..12] External trigger prescaler - Configures prescaler for external trigger */
            __IOM uint32_t ECE                 : 1; /*!< [14..14] External clock enable - Enables external clock mode */
            __IOM uint32_t ETP                 : 1; /*!< [15..15] External trigger polarity - Selects external trigger polarity */
            __IM  uint32_t          : 4;
            __IOM uint32_t TS_H                : 2; /*!< [21..20] Trigger selection (high bits) - Selects trigger input source bits [4:3] */
            __IM  uint32_t          : 10;
        } SMCR_f;
    };
    union {
        __IOM uint32_t DIER                ; /*!< Offset: 0x0C (read-write) DMA/interrupt enable register - Enables timer DMA requests and interrupts */
        struct {
            __IOM uint32_t UIE                 : 1; /*!< [0..0] Update interrupt enable - Enables update event interrupt */
            __IOM uint32_t CC1IE               : 1; /*!< [1..1] Capture/compare 1 interrupt enable - Enables channel 1 capture/compare interrupt */
            __IOM uint32_t CC2IE               : 1; /*!< [2..2] Capture/compare 2 interrupt enable - Enables channel 2 capture/compare interrupt */
            __IOM uint32_t CC3IE               : 1; /*!< [3..3] Capture/compare 3 interrupt enable - Enables channel 3 capture/compare interrupt */
            __IOM uint32_t CC4IE               : 1; /*!< [4..4] Capture/compare 4 interrupt enable - Enables channel 4 capture/compare interrupt */
            __IOM uint32_t COMIE               : 1; /*!< [5..5] COM interrupt enable - Enables commutation event interrupt */
            __IOM uint32_t TIE                 : 1; /*!< [6..6] Trigger interrupt enable - Enables trigger event interrupt */
            __IOM uint32_t BIE                 : 1; /*!< [7..7] Break interrupt enable - Enables break event interrupt */
            __IOM uint32_t UDE                 : 1; /*!< [8..8] Update DMA request enable - Enables DMA request on update event */
            __IOM uint32_t CC1DE               : 1; /*!< [9..9] Capture/compare 1 DMA request enable - Enables DMA request on channel 1 */
            __IOM uint32_t CC2DE               : 1; /*!< [10..10] Capture/compare 2 DMA request enable - Enables DMA request on channel 2 */
            __IOM uint32_t CC3DE               : 1; /*!< [11..11] Capture/compare 3 DMA request enable - Enables DMA request on channel 3 */
            __IOM uint32_t CC4DE               : 1; /*!< [12..12] Capture/compare 4 DMA request enable - Enables DMA request on channel 4 */
            __IOM uint32_t COMDE               : 1; /*!< [13..13] COM DMA request enable - Enables DMA request on commutation event */
            __IOM uint32_t TDE                 : 1; /*!< [14..14] Trigger DMA request enable - Enables DMA request on trigger event */
            __IM  uint32_t          : 17;
        } DIER_f;
    };
    union {
        __IOM uint32_t SR                  ; /*!< Offset: 0x10 (read-write) Status register - Power management status flags */
        struct {
            __IOM uint32_t UIF                 : 1; /*!< [0..0] Update interrupt flag - Set when update event occurs */
            __IOM uint32_t CC1IF               : 1; /*!< [1..1] Capture/compare 1 interrupt flag - Set when channel 1 capture/compare occurs */
            __IOM uint32_t CC2IF               : 1; /*!< [2..2] Capture/compare 2 interrupt flag - Set when channel 2 capture/compare occurs */
            __IOM uint32_t CC3IF               : 1; /*!< [3..3] Capture/compare 3 interrupt flag - Set when channel 3 capture/compare occurs */
            __IOM uint32_t CC4IF               : 1; /*!< [4..4] Capture/compare 4 interrupt flag - Set when channel 4 capture/compare occurs */
            __IOM uint32_t COMIF               : 1; /*!< [5..5] COM interrupt flag - Set when commutation event occurs */
            __IOM uint32_t TIF                 : 1; /*!< [6..6] Trigger interrupt flag - Set when trigger event occurs */
            __IOM uint32_t BIF                 : 1; /*!< [7..7] Break interrupt flag - Set when break event occurs */
            __IM  uint32_t          : 1;
            __IOM uint32_t CC1OF               : 1; /*!< [9..9] Capture/compare 1 overcapture flag - Set when channel 1 capture overrun occurs */
            __IOM uint32_t CC2OF               : 1; /*!< [10..10] Capture/compare 2 overcapture flag - Set when channel 2 capture overrun occurs */
            __IOM uint32_t CC3OF               : 1; /*!< [11..11] Capture/compare 3 overcapture flag - Set when channel 3 capture overrun occurs */
            __IOM uint32_t CC4OF               : 1; /*!< [12..12] Capture/compare 4 overcapture flag - Set when channel 4 capture overrun occurs */
            __IM  uint32_t          : 3;
            __IOM uint32_t CC5IF               : 1; /*!< [16..16] Capture/compare 5 interrupt flag - Set when channel 5 capture/compare occurs */
            __IOM uint32_t CC6IF               : 1; /*!< [17..17] Capture/compare 6 interrupt flag - Set when channel 6 capture/compare occurs */
            __IM  uint32_t          : 14;
        } SR_f;
    };
    union {
        __IOM uint32_t EGR                 ; /*!< Offset: 0x14 (read-write) Event generation register - Software generates timer events */
        struct {
            __IOM uint32_t UG                  : 1; /*!< [0..0] Update generation - Software generates update event */
            __IOM uint32_t CC1G                : 1; /*!< [1..1] Capture/compare 1 generation - Software generates channel 1 event */
            __IOM uint32_t CC2G                : 1; /*!< [2..2] Capture/compare 2 generation - Software generates channel 2 event */
            __IOM uint32_t CC3G                : 1; /*!< [3..3] Capture/compare 3 generation - Software generates channel 3 event */
            __IOM uint32_t CC4G                : 1; /*!< [4..4] Capture/compare 4 generation - Software generates channel 4 event */
            __IOM uint32_t COMG                : 1; /*!< [5..5] COM event generation - Software generates commutation event */
            __IOM uint32_t TG                  : 1; /*!< [6..6] Trigger generation - Software generates trigger event */
            __IOM uint32_t BG                  : 1; /*!< [7..7] Break generation - Software generates break event */
            __IM  uint32_t          : 24;
        } EGR_f;
    };
    union {
        union {
            __IOM uint32_t CCMR1_OUTPUT        ; /*!< Offset: 0x18 (read-write) Capture/compare mode register 1 (output) - Configures channels 1-2 in output mode */
            struct {
                __IOM uint32_t CC1S                : 2; /*!< [1..0] Capture/compare 1 selection - Selects channel 1 input/output direction */
                __IOM uint32_t OC1FE               : 1; /*!< [2..2] Output compare 1 fast enable - Enables fast output compare mode */
                __IOM uint32_t OC1PE               : 1; /*!< [3..3] Output compare 1 preload enable - Enables shadow register for channel 1 */
                __IOM uint32_t OC1M                : 3; /*!< [6..4] Output compare 1 mode - Selects output compare mode for channel 1 */
                __IOM uint32_t OC1CE               : 1; /*!< [7..7] Output compare 1 clear enable - Enables clearing of OC1 signal on ETRF */
                __IOM uint32_t CC2S                : 2; /*!< [9..8] Capture/compare 2 selection - Selects channel 2 input/output direction */
                __IOM uint32_t OC2FE               : 1; /*!< [10..10] Output compare 2 fast enable - Enables fast output compare mode */
                __IOM uint32_t OC2PE               : 1; /*!< [11..11] Output compare 2 preload enable - Enables shadow register for channel 2 */
                __IOM uint32_t OC2M                : 3; /*!< [14..12] Output compare 2 mode - Selects output compare mode for channel 2 */
                __IOM uint32_t OC2CE               : 1; /*!< [15..15] Output compare 2 clear enable - Enables clearing of OC2 signal on ETRF */
                __IM  uint32_t          : 16;
            } CCMR1_OUTPUT_f;
        };
        union {
            __IOM uint32_t CCMR1_INPUT         ; /*!< Offset: 0x18 (read-write) Capture/compare mode register 1 (input) - Configures channels 1-2 in input capture mode */
            struct {
                __IOM uint32_t CC1S                : 2; /*!< [1..0] Capture/compare 1 selection - Selects channel 1 input/output direction */
                __IOM uint32_t IC1PSC              : 2; /*!< [3..2] Input capture 1 prescaler - Configures prescaler for channel 1 input */
                __IOM uint32_t IC1F                : 4; /*!< [7..4] Input capture 1 filter - Configures digital filter for channel 1 input */
                __IOM uint32_t CC2S                : 2; /*!< [9..8] Capture/compare 2 selection - Selects channel 2 input/output direction */
                __IOM uint32_t IC2PSC              : 2; /*!< [11..10] Input capture 2 prescaler - Configures prescaler for channel 2 input */
                __IOM uint32_t IC2F                : 4; /*!< [15..12] Input capture 2 filter - Configures digital filter for channel 2 input */
                __IM  uint32_t          : 16;
            } CCMR1_INPUT_f;
        };
    };
    union {
        union {
            __IOM uint32_t CCMR2_OUTPUT        ; /*!< Offset: 0x1C (read-write) Capture/compare mode register 2 (output) - Configures channels 3-4 in output mode */
            struct {
                __IOM uint32_t CC3S                : 2; /*!< [1..0] Capture/compare 3 selection - Selects channel 3 input/output direction */
                __IOM uint32_t OC3FE               : 1; /*!< [2..2] Output compare 3 fast enable - Enables fast output compare mode */
                __IOM uint32_t OC3PE               : 1; /*!< [3..3] Output compare 3 preload enable - Enables shadow register for channel 3 */
                __IOM uint32_t OC3M                : 3; /*!< [6..4] Output compare 3 mode - Selects output compare mode for channel 3 */
                __IOM uint32_t OC3CE               : 1; /*!< [7..7] Output compare 3 clear enable - Enables clearing of OC3 signal on ETRF */
                __IOM uint32_t CC4S                : 2; /*!< [9..8] Capture/compare 4 selection - Selects channel 4 input/output direction */
                __IOM uint32_t OC4FE               : 1; /*!< [10..10] Output compare 4 fast enable - Enables fast output compare mode */
                __IOM uint32_t OC4PE               : 1; /*!< [11..11] Output compare 4 preload enable - Enables shadow register for channel 4 */
                __IOM uint32_t OC4M                : 3; /*!< [14..12] Output compare 4 mode - Selects output compare mode for channel 4 */
                __IOM uint32_t OC4CE               : 1; /*!< [15..15] Output compare 4 clear enable - Enables clearing of OC4 signal on ETRF */
                __IM  uint32_t          : 16;
            } CCMR2_OUTPUT_f;
        };
        union {
            __IOM uint32_t CCMR2_INPUT         ; /*!< Offset: 0x1C (read-write) Capture/compare mode register 2 (input) - Configures channels 3-4 in input capture mode */
            struct {
                __IOM uint32_t CC3S                : 2; /*!< [1..0] Capture/compare 3 selection - Selects channel 3 input/output direction */
                __IOM uint32_t IC3PSC              : 2; /*!< [3..2] Input capture 3 prescaler - Configures prescaler for channel 3 input */
                __IOM uint32_t IC3F                : 4; /*!< [7..4] Input capture 3 filter - Configures digital filter for channel 3 input */
                __IOM uint32_t CC4S                : 2; /*!< [9..8] Capture/compare 4 selection - Selects channel 4 input/output direction */
                __IOM uint32_t IC4PSC              : 2; /*!< [11..10] Input capture 4 prescaler - Configures prescaler for channel 4 input */
                __IOM uint32_t IC4F                : 4; /*!< [15..12] Input capture 4 filter - Configures digital filter for channel 4 input */
                __IM  uint32_t          : 16;
            } CCMR2_INPUT_f;
        };
    };
    union {
        __IOM uint32_t CCER                ; /*!< Offset: 0x20 (read-write) Capture/compare enable register - Enables capture/compare channels */
        struct {
            __IOM uint32_t CC1E                : 1; /*!< [0..0] Capture/compare 1 enable - Enables channel 1 capture/compare output */
            __IOM uint32_t CC1P                : 1; /*!< [1..1] Capture/compare 1 polarity - Selects channel 1 output polarity */
            __IOM uint32_t CC1NE               : 1; /*!< [2..2] Capture/compare 1 complementary enable - Enables channel 1 complementary output */
            __IOM uint32_t CC1NP               : 1; /*!< [3..3] Capture/compare 1 complementary polarity - Selects channel 1 complementary polarity */
            __IOM uint32_t CC2E                : 1; /*!< [4..4] Capture/compare 2 enable - Enables channel 2 capture/compare output */
            __IOM uint32_t CC2P                : 1; /*!< [5..5] Capture/compare 2 polarity - Selects channel 2 output polarity */
            __IOM uint32_t CC2NE               : 1; /*!< [6..6] Capture/compare 2 complementary enable - Enables channel 2 complementary output */
            __IOM uint32_t CC2NP               : 1; /*!< [7..7] Capture/compare 2 complementary polarity - Selects channel 2 complementary polarity */
            __IOM uint32_t CC3E                : 1; /*!< [8..8] Capture/compare 3 enable - Enables channel 3 capture/compare output */
            __IOM uint32_t CC3P                : 1; /*!< [9..9] Capture/compare 3 polarity - Selects channel 3 output polarity */
            __IOM uint32_t CC3NE               : 1; /*!< [10..10] Capture/compare 3 complementary enable - Enables channel 3 complementary output */
            __IOM uint32_t CC3NP               : 1; /*!< [11..11] Capture/compare 3 complementary polarity - Selects channel 3 complementary polarity */
            __IOM uint32_t CC4E                : 1; /*!< [12..12] Capture/compare 4 enable - Enables channel 4 capture/compare output */
            __IOM uint32_t CC4P                : 1; /*!< [13..13] Capture/compare 4 polarity - Selects channel 4 output polarity */
            __IOM uint32_t CC4NE               : 1; /*!< [14..14] Capture/compare 4 complementary enable - Enables channel 4 complementary output */
            __IOM uint32_t CC4NP               : 1; /*!< [15..15] Capture/compare 4 complementary polarity - Selects channel 4 complementary polarity */
            __IOM uint32_t CC5E                : 1; /*!< [16..16] Capture/compare 5 enable - Enables channel 5 capture/compare output */
            __IOM uint32_t CC5P                : 1; /*!< [17..17] Capture/compare 5 polarity - Selects channel 5 output polarity */
            __IM  uint32_t          : 2;
            __IOM uint32_t CC6E                : 1; /*!< [20..20] Capture/compare 6 enable - Enables channel 6 capture/compare output */
            __IOM uint32_t CC6P                : 1; /*!< [21..21] Capture/compare 6 polarity - Selects channel 6 output polarity */
            __IM  uint32_t          : 10;
        } CCER_f;
    };
    union {
        __IOM uint32_t CNT                 ; /*!< Offset: 0x24 (read-write) Counter register - Current timer counter value */
        struct {
            __IOM uint32_t CNT                 : 16; /*!< [15..0] Counter register - Current timer counter value */
            __IM  uint32_t          : 16;
        } CNT_f;
    };
    union {
        __IOM uint32_t PSC                 ; /*!< Offset: 0x28 (read-write) Prescaler register - Clock divider for timer counter */
        struct {
            __IOM uint32_t PSC                 : 16; /*!< [15..0] Prescaler register - Clock divider for timer counter */
            __IM  uint32_t          : 16;
        } PSC_f;
    };
    union {
        __IOM uint32_t ARR                 ; /*!< Offset: 0x2C (read-write) Auto-reload register - Counter auto-reload value */
        struct {
            __IOM uint32_t ARR                 : 16; /*!< [15..0] Auto-reload register - Counter auto-reload value */
            __IM  uint32_t          : 16;
        } ARR_f;
    };
    union {
        __IOM uint32_t RCR                 ; /*!< Offset: 0x30 (read-write) Repetition counter register - Configures underflow repetition count */
        struct {
            __IOM uint32_t REP                 : 8; /*!< [7..0] Repetition counter value - Sets the repetition counter value */
            __IM  uint32_t          : 24;
        } RCR_f;
    };
    union {
        __IOM uint32_t CCR1                ; /*!< Offset: 0x34 (read-write) Capture/compare register 1 - Compare or capture value for channel 1 */
        struct {
            __IOM uint32_t CCR1                : 16; /*!< [15..0] Capture/compare register 1 - Compare value or captured counter value for channel 1 */
            __IM  uint32_t          : 16;
        } CCR1_f;
    };
    union {
        __IOM uint32_t CCR2                ; /*!< Offset: 0x38 (read-write) Capture/compare register 2 - Compare or capture value for channel 2 */
        struct {
            __IOM uint32_t CCR2                : 16; /*!< [15..0] Capture/compare register 2 - Compare value or captured counter value for channel 2 */
            __IM  uint32_t          : 16;
        } CCR2_f;
    };
    union {
        __IOM uint32_t CCR3                ; /*!< Offset: 0x3C (read-write) Capture/compare register 3 - Compare or capture value for channel 3 */
        struct {
            __IOM uint32_t CCR3                : 16; /*!< [15..0] Capture/compare register 3 - Compare value or captured counter value for channel 3 */
            __IM  uint32_t          : 16;
        } CCR3_f;
    };
    union {
        __IOM uint32_t CCR4                ; /*!< Offset: 0x40 (read-write) Capture/compare register 4 - Compare or capture value for channel 4 */
        struct {
            __IOM uint32_t CCR4                : 16; /*!< [15..0] Capture/compare register 4 - Compare value or captured counter value for channel 4 */
            __IM  uint32_t          : 16;
        } CCR4_f;
    };
    union {
        __IOM uint32_t BDTR                ; /*!< Offset: 0x44 (read-write) Break and dead-time register - Configures break input and dead-time */
        struct {
            __IOM uint32_t DTG                 : 8; /*!< [7..0] Dead-time generator setup - Configures dead-time duration between complementary outputs */
            __IOM uint32_t LOCK                : 2; /*!< [9..8] Lock register - Locks timer configuration bits against modification */
            __IOM uint32_t OSSI                : 1; /*!< [10..10] Off-state selection for idle - Selects output state in idle mode */
            __IOM uint32_t OSSR                : 1; /*!< [11..11] Off-state selection for run - Selects output state in run mode */
            __IOM uint32_t BKE                 : 1; /*!< [12..12] Break enable - Enables break input */
            __IOM uint32_t BKP                 : 1; /*!< [13..13] Break polarity - Selects break input polarity */
            __IOM uint32_t AOE                 : 1; /*!< [14..14] Automatic output enable - Enables automatic output enable on next update */
            __IOM uint32_t MOE                 : 1; /*!< [15..15] Main output enable - Enables all timer outputs */
            __IM  uint32_t          : 16;
        } BDTR_f;
    };
    union {
        __IOM uint32_t DCR                 ; /*!< Offset: 0x48 (read-write) DMA control register - Configures DMA burst transfer */
        struct {
            __IOM uint32_t DBA                 : 5; /*!< [4..0] DMA base address - Sets base address for DMA burst transfer */
            __IM  uint32_t          : 3;
            __IOM uint32_t DBL                 : 5; /*!< [12..8] DMA burst length - Sets length for DMA burst transfer */
            __IM  uint32_t          : 19;
        } DCR_f;
    };
    union {
        __IOM uint32_t DMAR                ; /*!< Offset: 0x4C (read-write) DMA address for burst - DMA address for burst mode transfers */
        struct {
            __IOM uint32_t DMAB                : 16; /*!< [15..0] DMA data buffer - DMA burst data buffer */
            __IM  uint32_t          : 16;
        } DMAR_f;
    };
    __IO uint32_t RESERVED0;
    union {
        __IOM uint32_t CCMR3               ; /*!< Offset: 0x54 (read-write) Capture/compare mode register 3 - Configures channels 5-6 output modes */
        struct {
            __IM  uint32_t          : 2;
            __IOM uint32_t OC5FE               : 1; /*!< [2..2] Output compare 5 fast enable - Enables fast output compare for channel 5 */
            __IOM uint32_t OC5PE               : 1; /*!< [3..3] Output compare 5 preload enable - Enables shadow register for channel 5 */
            __IOM uint32_t OC5M                : 3; /*!< [6..4] Output compare 5 mode - Selects output compare mode for channel 5 */
            __IOM uint32_t OC5CE               : 1; /*!< [7..7] Output compare 5 clear enable - Enables clearing of OC5 signal on ETRF */
            __IM  uint32_t          : 2;
            __IOM uint32_t OC6FE               : 1; /*!< [10..10] Output compare 6 fast enable - Enables fast output compare for channel 6 */
            __IOM uint32_t OC6PE               : 1; /*!< [11..11] Output compare 6 preload enable - Enables shadow register for channel 6 */
            __IOM uint32_t OC6M                : 3; /*!< [14..12] Output compare 6 mode - Selects output compare mode for channel 6 */
            __IOM uint32_t OC6CE               : 1; /*!< [15..15] Output compare 6 clear enable - Enables clearing of OC6 signal on ETRF */
            __IM  uint32_t          : 16;
        } CCMR3_f;
    };
    union {
        __IOM uint32_t CCR5                ; /*!< Offset: 0x58 (read-write) Capture/compare register 5 - Compare value or captured counter value for channel 5 */
        struct {
            __IOM uint32_t CCR5                : 16; /*!< [15..0] Capture/compare register 5 - Compare value or captured counter value for channel 5 */
            __IM  uint32_t          : 16;
        } CCR5_f;
    };
    union {
        __IOM uint32_t CCR6                ; /*!< Offset: 0x5C (read-write) Capture/compare register 6 - Compare value or captured counter value for channel 6 */
        struct {
            __IOM uint32_t CCR6                : 16; /*!< [15..0] Capture/compare register 6 - Compare value or captured counter value for channel 6 */
            __IM  uint32_t          : 16;
        } CCR6_f;
    };
    union {
        __IOM uint32_t AF1                 ; /*!< Offset: 0x60 (read-write) Alternate function register 1 - Configures break input sources */
        struct {
            __IOM uint32_t BKINE               : 1; /*!< [0..0] Break input NMI enable - Enables break input as NMI source */
            __IOM uint32_t BKCMP1E             : 1; /*!< [1..1] Break comparator 1 enable - Enables comparator 1 as break source */
            __IOM uint32_t BKCMP2E             : 1; /*!< [2..2] Break comparator 2 enable - Enables comparator 2 as break source */
            __IOM uint32_t BKCMP3E             : 1; /*!< [3..3] Break comparator 3 enable - Enables comparator 3 as break source */
            __IOM uint32_t BKCMP4E             : 1; /*!< [4..4] Break comparator 4 enable - Enables comparator 4 as break source */
            __IM  uint32_t          : 4;
            __IOM uint32_t BKINP               : 1; /*!< [9..9] Break input polarity - Selects break input polarity */
            __IOM uint32_t BKCMP1P             : 1; /*!< [10..10] Break comparator 1 polarity - Selects comparator 1 break polarity */
            __IOM uint32_t BKCMP2P             : 1; /*!< [11..11] Break comparator 2 polarity - Selects comparator 2 break polarity */
            __IOM uint32_t BKCMP3P             : 1; /*!< [12..12] Break comparator 3 polarity - Selects comparator 3 break polarity */
            __IOM uint32_t BKCMP4P             : 1; /*!< [13..13] Break comparator 4 polarity - Selects comparator 4 break polarity */
            __IOM uint32_t ETRSEL              : 4; /*!< [17..14] External trigger selection - Selects external trigger source */
            __IM  uint32_t          : 14;
        } AF1_f;
    };
    union {
        __IOM uint32_t AF2                 ; /*!< Offset: 0x64 (read-write) Alternate function register 2 - Configures output compare and input sources */
        struct {
            __IM  uint32_t          : 16;
            __IOM uint32_t OCRSEL              : 3; /*!< [18..16] Output compare reference selection - Selects reference signal for OC */
            __IM  uint32_t          : 13;
        } AF2_f;
    };
    union {
        __IOM uint32_t TISEL               ; /*!< Offset: 0x68 (read-write) Timer input selection - Selects timer input sources */
        struct {
            __IOM uint32_t TI1SEL              : 4; /*!< [3..0] Timer input 1 selection - Selects input 1 source for timer */
            __IM  uint32_t          : 4;
            __IOM uint32_t TI2SEL              : 4; /*!< [11..8] Timer input 2 selection - Selects input 2 source for timer */
            __IM  uint32_t          : 4;
            __IOM uint32_t TI3SEL              : 4; /*!< [19..16] Timer input 3 selection - Selects input 3 source for timer */
            __IM  uint32_t          : 4;
            __IOM uint32_t TI4SEL              : 4; /*!< [27..24] Timer input 4 selection - Selects input 4 source for timer */
            __IM  uint32_t          : 4;
        } TISEL_f;
    };
    union {
        __IOM uint32_t DBER                ; /*!< Offset: 0x6C (read-write) DMA burst enable register - Selects DMA request type (single/burst) */
        struct {
            __IOM uint32_t UBE                 : 1; /*!< [0..0] Update burst enable - Selects DMA request type for update event */
            __IOM uint32_t CC1BE               : 1; /*!< [1..1] CC1 burst enable - Selects DMA request type for channel 1 event */
            __IOM uint32_t CC2BE               : 1; /*!< [2..2] CC2 burst enable - Selects DMA request type for channel 2 event */
            __IOM uint32_t CC3BE               : 1; /*!< [3..3] CC3 burst enable - Selects DMA request type for channel 3 event */
            __IOM uint32_t CC4BE               : 1; /*!< [4..4] CC4 burst enable - Selects DMA request type for channel 4 event */
            __IOM uint32_t COMBE               : 1; /*!< [5..5] COM burst enable - Selects DMA request type for commutation event */
            __IOM uint32_t TBE                 : 1; /*!< [6..6] Trigger burst enable - Selects DMA request type for trigger event */
            __IM  uint32_t          : 25;
        } DBER_f;
    };
} TIM1_TypeDef;

/**
 * @brief TIM2 - TIM2 Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t CR1                 ; /*!< Offset: 0x00 (read-write) Control register 1 - UART control and configuration */
        struct {
            __IOM uint32_t CEN                 : 1; /*!< [0..0] Counter enable - Enables or disables the timer counter */
            __IOM uint32_t UDIS                : 1; /*!< [1..1] Update disable - Disables update event generation */
            __IOM uint32_t URS                 : 1; /*!< [2..2] Update request source - Selects the update event source */
            __IOM uint32_t OPM                 : 1; /*!< [3..3] One-pulse mode - Enables one-pulse mode operation */
            __IOM uint32_t DIR                 : 1; /*!< [4..4] Direction - Selects counter direction (up/down counting) */
            __IOM uint32_t CMS                 : 2; /*!< [6..5] Center-aligned mode select - Selects center-aligned PWM mode */
            __IOM uint32_t ARPE                : 1; /*!< [7..7] Auto-reload preload enable - Enables auto-reload register buffering */
            __IOM uint32_t CKD                 : 2; /*!< [9..8] Clock division - Configures clock division factor for dead-time and filter */
            __IM  uint32_t          : 22;
        } CR1_f;
    };
    union {
        __IOM uint32_t CR2                 ; /*!< Offset: 0x04 (read-write) Control register 2 - UART additional control */
        struct {
            __IM  uint32_t          : 3;
            __IOM uint32_t CCDS                : 1; /*!< [3..3] Capture/compare DMA selection - Selects DMA request source */
            __IOM uint32_t MMS                 : 3; /*!< [6..4] Master mode selection - Selects the trigger output (TRGO) source */
            __IOM uint32_t TI1S                : 1; /*!< [7..7] TIMx input 1 selection - Selects input 1 source for timer */
            __IM  uint32_t          : 24;
        } CR2_f;
    };
    union {
        __IOM uint32_t SMCR                ; /*!< Offset: 0x08 (read-write) Slave mode control register - Configures timer synchronization and slave mode */
        struct {
            __IOM uint32_t SMS                 : 3; /*!< [2..0] Slave mode selection - Selects timer slave mode */
            __IM  uint32_t          : 1;
            __IOM uint32_t TS                  : 3; /*!< [6..4] Trigger selection - Selects trigger input for slave mode */
            __IOM uint32_t MSM                 : 1; /*!< [7..7] Master-slave mode - Enables master-slave synchronization mode */
            __IOM uint32_t ETF                 : 4; /*!< [11..8] External trigger filter - Configures filter for external trigger */
            __IOM uint32_t ETPS                : 2; /*!< [13..12] External trigger prescaler - Configures prescaler for external trigger */
            __IOM uint32_t ECE                 : 1; /*!< [14..14] External clock enable - Enables external clock mode */
            __IOM uint32_t ETP                 : 1; /*!< [15..15] External trigger polarity - Selects external trigger polarity */
            __IM  uint32_t          : 16;
        } SMCR_f;
    };
    union {
        __IOM uint32_t DIER                ; /*!< Offset: 0x0C (read-write) DMA/interrupt enable register - Enables timer DMA requests and interrupts */
        struct {
            __IOM uint32_t UIE                 : 1; /*!< [0..0] Update interrupt enable - Enables update event interrupt */
            __IOM uint32_t CC1IE               : 1; /*!< [1..1] Capture/compare 1 interrupt enable - Enables channel 1 capture/compare interrupt */
            __IOM uint32_t CC2IE               : 1; /*!< [2..2] Capture/compare 2 interrupt enable - Enables channel 2 capture/compare interrupt */
            __IOM uint32_t CC3IE               : 1; /*!< [3..3] Capture/compare 3 interrupt enable - Enables channel 3 capture/compare interrupt */
            __IOM uint32_t CC4IE               : 1; /*!< [4..4] Capture/compare 4 interrupt enable - Enables channel 4 capture/compare interrupt */
            __IM  uint32_t          : 1;
            __IOM uint32_t TIE                 : 1; /*!< [6..6] Trigger interrupt enable - Enables trigger event interrupt */
            __IM  uint32_t          : 1;
            __IOM uint32_t UDE                 : 1; /*!< [8..8] Update DMA request enable - Enables DMA request on update event */
            __IOM uint32_t CC1DE               : 1; /*!< [9..9] Capture/compare 1 DMA request enable - Enables DMA request on channel 1 */
            __IOM uint32_t CC2DE               : 1; /*!< [10..10] Capture/compare 2 DMA request enable - Enables DMA request on channel 2 */
            __IOM uint32_t CC3DE               : 1; /*!< [11..11] Capture/compare 3 DMA request enable - Enables DMA request on channel 3 */
            __IOM uint32_t CC4DE               : 1; /*!< [12..12] Capture/compare 4 DMA request enable - Enables DMA request on channel 4 */
            __IM  uint32_t          : 1;
            __IOM uint32_t TDE                 : 1; /*!< [14..14] Trigger DMA request enable - Enables DMA request on trigger event */
            __IM  uint32_t          : 17;
        } DIER_f;
    };
    union {
        __IOM uint32_t SR                  ; /*!< Offset: 0x10 (read-write) Status register - Power management status flags */
        struct {
            __IOM uint32_t UIF                 : 1; /*!< [0..0] Update interrupt flag - Set when update event occurs */
            __IOM uint32_t CC1IF               : 1; /*!< [1..1] Capture/compare 1 interrupt flag - Set when channel 1 capture/compare occurs */
            __IOM uint32_t CC2IF               : 1; /*!< [2..2] Capture/compare 2 interrupt flag - Set when channel 2 capture/compare occurs */
            __IOM uint32_t CC3IF               : 1; /*!< [3..3] Capture/compare 3 interrupt flag - Set when channel 3 capture/compare occurs */
            __IOM uint32_t CC4IF               : 1; /*!< [4..4] Capture/compare 4 interrupt flag - Set when channel 4 capture/compare occurs */
            __IM  uint32_t          : 1;
            __IOM uint32_t TIF                 : 1; /*!< [6..6] Trigger interrupt flag - Set when trigger event occurs */
            __IM  uint32_t          : 2;
            __IOM uint32_t CC1OF               : 1; /*!< [9..9] Capture/compare 1 overcapture flag - Set when channel 1 capture overrun occurs */
            __IOM uint32_t CC2OF               : 1; /*!< [10..10] Capture/compare 2 overcapture flag - Set when channel 2 capture overrun occurs */
            __IOM uint32_t CC3OF               : 1; /*!< [11..11] Capture/compare 3 overcapture flag - Set when channel 3 capture overrun occurs */
            __IOM uint32_t CC4OF               : 1; /*!< [12..12] Capture/compare 4 overcapture flag - Set when channel 4 capture overrun occurs */
            __IM  uint32_t          : 19;
        } SR_f;
    };
    union {
        __IOM uint32_t EGR                 ; /*!< Offset: 0x14 (read-write) Event generation register - Software generates timer events */
        struct {
            __IOM uint32_t UG                  : 1; /*!< [0..0] Update generation - Software generates update event */
            __IOM uint32_t CC1G                : 1; /*!< [1..1] Capture/compare 1 generation - Software generates channel 1 event */
            __IOM uint32_t CC2G                : 1; /*!< [2..2] Capture/compare 2 generation - Software generates channel 2 event */
            __IOM uint32_t CC3G                : 1; /*!< [3..3] Capture/compare 3 generation - Software generates channel 3 event */
            __IOM uint32_t CC4G                : 1; /*!< [4..4] Capture/compare 4 generation - Software generates channel 4 event */
            __IM  uint32_t          : 1;
            __IOM uint32_t TG                  : 1; /*!< [6..6] Trigger generation - Software generates trigger event */
            __IM  uint32_t          : 25;
        } EGR_f;
    };
    union {
        union {
            __IOM uint32_t CCMR1_OUTPUT        ; /*!< Offset: 0x18 (read-write) Capture/compare mode register 1 (output) - Configures channels 1-2 in output mode */
            struct {
                __IOM uint32_t CC1S                : 2; /*!< [1..0] Capture/compare 1 selection - Selects channel 1 input/output direction */
                __IOM uint32_t OC1FE               : 1; /*!< [2..2] Output compare 1 fast enable - Enables fast output compare mode */
                __IOM uint32_t OC1PE               : 1; /*!< [3..3] Output compare 1 preload enable - Enables shadow register for channel 1 */
                __IOM uint32_t OC1M                : 3; /*!< [6..4] Output compare 1 mode - Selects output compare mode for channel 1 */
                __IOM uint32_t OC1CE               : 1; /*!< [7..7] Output compare 1 clear enable - Enables clearing of OC1 signal on ETRF */
                __IOM uint32_t CC2S                : 2; /*!< [9..8] Capture/compare 2 selection - Selects channel 2 input/output direction */
                __IOM uint32_t OC2FE               : 1; /*!< [10..10] Output compare 2 fast enable - Enables fast output compare mode */
                __IOM uint32_t OC2PE               : 1; /*!< [11..11] Output compare 2 preload enable - Enables shadow register for channel 2 */
                __IOM uint32_t OC2M                : 3; /*!< [14..12] Output compare 2 mode - Selects output compare mode for channel 2 */
                __IOM uint32_t OC2CE               : 1; /*!< [15..15] Output compare 2 clear enable - Enables clearing of OC2 signal on ETRF */
                __IM  uint32_t          : 16;
            } CCMR1_OUTPUT_f;
        };
        union {
            __IOM uint32_t CCMR1_INPUT         ; /*!< Offset: 0x18 (read-write) Capture/compare mode register 1 (input) - Configures channels 1-2 in input capture mode */
            struct {
                __IOM uint32_t CC1S                : 2; /*!< [1..0] Capture/compare 1 selection - Selects channel 1 input/output direction */
                __IOM uint32_t IC1PSC              : 2; /*!< [3..2] Input capture 1 prescaler - Configures prescaler for channel 1 input */
                __IOM uint32_t IC1F                : 4; /*!< [7..4] Input capture 1 filter - Configures digital filter for channel 1 input */
                __IOM uint32_t CC2S                : 2; /*!< [9..8] Capture/compare 2 selection - Selects channel 2 input/output direction */
                __IOM uint32_t IC2PSC              : 2; /*!< [11..10] Input capture 2 prescaler - Configures prescaler for channel 2 input */
                __IOM uint32_t IC2F                : 4; /*!< [15..12] Input capture 2 filter - Configures digital filter for channel 2 input */
                __IM  uint32_t          : 16;
            } CCMR1_INPUT_f;
        };
    };
    union {
        union {
            __IOM uint32_t CCMR2_OUTPUT        ; /*!< Offset: 0x1C (read-write) Capture/compare mode register 2 (output) - Configures channels 3-4 in output mode */
            struct {
                __IOM uint32_t CC3S                : 2; /*!< [1..0] Capture/compare 3 selection - Selects channel 3 input/output direction */
                __IOM uint32_t OC3FE               : 1; /*!< [2..2] Output compare 3 fast enable - Enables fast output compare mode */
                __IOM uint32_t OC3PE               : 1; /*!< [3..3] Output compare 3 preload enable - Enables shadow register for channel 3 */
                __IOM uint32_t OC3M                : 3; /*!< [6..4] Output compare 3 mode - Selects output compare mode for channel 3 */
                __IOM uint32_t OC3CE               : 1; /*!< [7..7] Output compare 3 clear enable - Enables clearing of OC3 signal on ETRF */
                __IOM uint32_t CC4S                : 2; /*!< [9..8] Capture/compare 4 selection - Selects channel 4 input/output direction */
                __IOM uint32_t OC4FE               : 1; /*!< [10..10] Output compare 4 fast enable - Enables fast output compare mode */
                __IOM uint32_t OC4PE               : 1; /*!< [11..11] Output compare 4 preload enable - Enables shadow register for channel 4 */
                __IOM uint32_t OC4M                : 3; /*!< [14..12] Output compare 4 mode - Selects output compare mode for channel 4 */
                __IOM uint32_t OC4CE               : 1; /*!< [15..15] Output compare 4 clear enable - Enables clearing of OC4 signal on ETRF */
                __IM  uint32_t          : 16;
            } CCMR2_OUTPUT_f;
        };
        union {
            __IOM uint32_t CCMR2_INPUT         ; /*!< Offset: 0x1C (read-write) Capture/compare mode register 2 (input) - Configures channels 3-4 in input capture mode */
            struct {
                __IOM uint32_t CC3S                : 2; /*!< [1..0] Capture/compare 3 selection - Selects channel 3 input/output direction */
                __IOM uint32_t IC3PSC              : 2; /*!< [3..2] Input capture 3 prescaler - Configures prescaler for channel 3 input */
                __IOM uint32_t IC3F                : 4; /*!< [7..4] Input capture 3 filter - Configures digital filter for channel 3 input */
                __IOM uint32_t CC4S                : 2; /*!< [9..8] Capture/compare 4 selection - Selects channel 4 input/output direction */
                __IOM uint32_t IC4PSC              : 2; /*!< [11..10] Input capture 4 prescaler - Configures prescaler for channel 4 input */
                __IOM uint32_t IC4F                : 4; /*!< [15..12] Input capture 4 filter - Configures digital filter for channel 4 input */
                __IM  uint32_t          : 16;
            } CCMR2_INPUT_f;
        };
    };
    union {
        __IOM uint32_t CCER                ; /*!< Offset: 0x20 (read-write) Capture/compare enable register - Enables capture/compare channels */
        struct {
            __IOM uint32_t CC1E                : 1; /*!< [0..0] Capture/compare 1 enable - Enables channel 1 capture/compare output */
            __IOM uint32_t CC1P                : 1; /*!< [1..1] Capture/compare 1 polarity - Selects channel 1 output polarity */
            __IM  uint32_t          : 1;
            __IOM uint32_t CC1NP               : 1; /*!< [3..3] Capture/compare 1 complementary polarity - Selects channel 1 complementary polarity */
            __IOM uint32_t CC2E                : 1; /*!< [4..4] Capture/compare 2 enable - Enables channel 2 capture/compare output */
            __IOM uint32_t CC2P                : 1; /*!< [5..5] Capture/compare 2 polarity - Selects channel 2 output polarity */
            __IM  uint32_t          : 1;
            __IOM uint32_t CC2NP               : 1; /*!< [7..7] Capture/compare 2 complementary polarity - Selects channel 2 complementary polarity */
            __IOM uint32_t CC3E                : 1; /*!< [8..8] Capture/compare 3 enable - Enables channel 3 capture/compare output */
            __IOM uint32_t CC3P                : 1; /*!< [9..9] Capture/compare 3 polarity - Selects channel 3 output polarity */
            __IM  uint32_t          : 1;
            __IOM uint32_t CC3NP               : 1; /*!< [11..11] Capture/compare 3 complementary polarity - Selects channel 3 complementary polarity */
            __IOM uint32_t CC4E                : 1; /*!< [12..12] Capture/compare 4 enable - Enables channel 4 capture/compare output */
            __IOM uint32_t CC4P                : 1; /*!< [13..13] Capture/compare 4 polarity - Selects channel 4 output polarity */
            __IM  uint32_t          : 1;
            __IOM uint32_t CC4NP               : 1; /*!< [15..15] Capture/compare 4 complementary polarity - Selects channel 4 complementary polarity */
            __IM  uint32_t          : 16;
        } CCER_f;
    };
    union {
        __IOM uint32_t CNT                 ; /*!< Offset: 0x24 (read-write) Counter register - Current timer counter value */
        struct {
            __IOM uint32_t CNT                 : 16; /*!< [15..0] Counter register - Current timer counter value */
            __IM  uint32_t          : 16;
        } CNT_f;
    };
    union {
        __IOM uint32_t PSC                 ; /*!< Offset: 0x28 (read-write) Prescaler register - Clock divider for timer counter */
        struct {
            __IOM uint32_t PSC                 : 16; /*!< [15..0] Prescaler register - Clock divider for timer counter */
            __IM  uint32_t          : 16;
        } PSC_f;
    };
    union {
        __IOM uint32_t ARR                 ; /*!< Offset: 0x2C (read-write) Auto-reload register - Counter auto-reload value */
        struct {
            __IOM uint32_t ARR                 : 16; /*!< [15..0] Auto-reload register - Counter auto-reload value */
            __IM  uint32_t          : 16;
        } ARR_f;
    };
    __IO uint32_t RESERVED0;
    union {
        __IOM uint32_t CCR1                ; /*!< Offset: 0x34 (read-write) Capture/compare register 1 - Compare or capture value for channel 1 */
        struct {
            __IOM uint32_t CCR1                : 16; /*!< [15..0] Capture/compare register 1 - Compare value or captured counter value for channel 1 */
            __IM  uint32_t          : 16;
        } CCR1_f;
    };
    union {
        __IOM uint32_t CCR2                ; /*!< Offset: 0x38 (read-write) Capture/compare register 2 - Compare or capture value for channel 2 */
        struct {
            __IOM uint32_t CCR2                : 16; /*!< [15..0] Capture/compare register 2 - Compare value or captured counter value for channel 2 */
            __IM  uint32_t          : 16;
        } CCR2_f;
    };
    union {
        __IOM uint32_t CCR3                ; /*!< Offset: 0x3C (read-write) Capture/compare register 3 - Compare or capture value for channel 3 */
        struct {
            __IOM uint32_t CCR3                : 16; /*!< [15..0] Capture/compare register 3 - Compare value or captured counter value for channel 3 */
            __IM  uint32_t          : 16;
        } CCR3_f;
    };
    union {
        __IOM uint32_t CCR4                ; /*!< Offset: 0x40 (read-write) Capture/compare register 4 - Compare or capture value for channel 4 */
        struct {
            __IOM uint32_t CCR4                : 16; /*!< [15..0] Capture/compare register 4 - Compare value or captured counter value for channel 4 */
            __IM  uint32_t          : 16;
        } CCR4_f;
    };
    __IO uint32_t RESERVED1;
    union {
        __IOM uint32_t DCR                 ; /*!< Offset: 0x48 (read-write) DMA control register - Configures DMA burst transfer */
        struct {
            __IOM uint32_t DBA                 : 5; /*!< [4..0] DMA base address - Sets base address for DMA burst transfer */
            __IM  uint32_t          : 3;
            __IOM uint32_t DBL                 : 5; /*!< [12..8] DMA burst length - Sets length for DMA burst transfer */
            __IM  uint32_t          : 19;
        } DCR_f;
    };
    union {
        __IOM uint32_t DMAR                ; /*!< Offset: 0x4C (read-write) DMA address for burst - DMA address for burst mode transfers */
        struct {
            __IOM uint32_t DMAB                : 16; /*!< [15..0] DMA data buffer - DMA burst data buffer */
            __IM  uint32_t          : 16;
        } DMAR_f;
    };
    __IO uint32_t RESERVED2[4];
    union {
        __IOM uint32_t AF1                 ; /*!< Offset: 0x60 (read-write) Alternate function register 1 - Configures break input sources */
        struct {
            __IM  uint32_t          : 14;
            __IOM uint32_t ETRSEL              : 2; /*!< [15..14] External trigger selection - Selects external trigger source */
            __IM  uint32_t          : 16;
        } AF1_f;
    };
    __IO uint32_t RESERVED3;
    union {
        __IOM uint32_t TISEL               ; /*!< Offset: 0x68 (read-write) Timer input selection - Selects timer input sources */
        struct {
            __IOM uint32_t T1SEL               : 1; /*!< [0..0] T1 selection - Timer 1 source selection */
            __IM  uint32_t          : 7;
            __IOM uint32_t T2SEL               : 1; /*!< [8..8] T2 selection - Timer 2 source selection */
            __IM  uint32_t          : 23;
        } TISEL_f;
    };
    union {
        __IOM uint32_t DBER                ; /*!< Offset: 0x6C (read-write) Debug enable register - Enables debug features during breakpoint */
        struct {
            __IOM uint32_t UBE                 : 1; /*!< [0..0] Update break enable - Enables break on update event */
            __IOM uint32_t CC1BE               : 1; /*!< [1..1] CC1 break enable - Enables break on channel 1 event */
            __IOM uint32_t CC2BE               : 1; /*!< [2..2] CC2 break enable - Enables break on channel 2 event */
            __IOM uint32_t CC3BE               : 1; /*!< [3..3] CC3 break enable - Enables break on channel 3 event */
            __IOM uint32_t CC4BE               : 1; /*!< [4..4] CC4 break enable - Enables break on channel 4 event */
            __IOM uint32_t COMBE               : 1; /*!< [5..5] COM break enable - Enables break on commutation event */
            __IOM uint32_t TBE                 : 1; /*!< [6..6] Trigger break enable - Enables break on trigger event */
            __IM  uint32_t          : 25;
        } DBER_f;
    };
} TIM2_TypeDef;

/**
 * @brief TIM6 - TIM6 Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t CR1                 ; /*!< Offset: 0x00 (read-write) Control register 1 */
        struct {
            __IOM uint32_t CEN                 : 1; /*!< [0..0] Counter enable - Enables or disables the timer counter */
            __IOM uint32_t UDIS                : 1; /*!< [1..1] Update disable - Disables update event generation */
            __IOM uint32_t URS                 : 1; /*!< [2..2] Update request source - Selects the update event source */
            __IOM uint32_t OPM                 : 1; /*!< [3..3] One-pulse mode - Enables one-pulse mode operation */
            __IM  uint32_t          : 3;
            __IOM uint32_t ARPE                : 1; /*!< [7..7] Auto-reload preload enable - Enables auto-reload register buffering */
            __IM  uint32_t          : 24;
        } CR1_f;
    };
    union {
        __IOM uint32_t CR2                 ; /*!< Offset: 0x04 (read-write) Control register 2 */
        struct {
            __IM  uint32_t          : 4;
            __IOM uint32_t MMS                 : 3; /*!< [6..4] Master mode selection - Selects the trigger output (TRGO) source */
            __IM  uint32_t          : 25;
        } CR2_f;
    };
    __IO uint32_t RESERVED0;
    union {
        __IOM uint32_t DIER                ; /*!< Offset: 0x0C (read-write) DMA/interrupt enable register - Enables timer DMA requests and interrupts */
        struct {
            __IOM uint32_t UIE                 : 1; /*!< [0..0] Update interrupt enable - Enables update event interrupt */
            __IM  uint32_t          : 7;
            __IOM uint32_t UDE                 : 1; /*!< [8..8] Update DMA request enable - Enables DMA request on update event */
            __IM  uint32_t          : 23;
        } DIER_f;
    };
    union {
        __IOM uint32_t SR                  ; /*!< Offset: 0x10 (read-write) Status register - Power management status flags */
        struct {
            __IOM uint32_t UIF                 : 1; /*!< [0..0] Update interrupt flag - Set when update event occurs */
            __IM  uint32_t          : 31;
        } SR_f;
    };
    union {
        __IOM uint32_t EGR                 ; /*!< Offset: 0x14 (read-write) Event generation register - Software generates timer events */
        struct {
            __IOM uint32_t UG                  : 1; /*!< [0..0] Update generation - Software generates update event */
            __IM  uint32_t          : 31;
        } EGR_f;
    };
    __IO uint32_t RESERVED1[3];
    union {
        __IOM uint32_t CNT                 ; /*!< Offset: 0x24 (read-write) Counter register - Current timer counter value */
        struct {
            __IOM uint32_t CNT                 : 16; /*!< [15..0] Counter register - Current timer counter value */
            __IM  uint32_t          : 16;
        } CNT_f;
    };
    union {
        __IOM uint32_t PSC                 ; /*!< Offset: 0x28 (read-write) Prescaler register - Clock divider for timer counter */
        struct {
            __IOM uint32_t PSC                 : 16; /*!< [15..0] Prescaler register - Clock divider for timer counter */
            __IM  uint32_t          : 16;
        } PSC_f;
    };
    union {
        __IOM uint32_t ARR                 ; /*!< Offset: 0x2C (read-write) Auto-reload register - Counter auto-reload value */
        struct {
            __IOM uint32_t ARR                 : 16; /*!< [15..0] Auto-reload register - Counter auto-reload value */
            __IM  uint32_t          : 16;
        } ARR_f;
    };
} TIM6_TypeDef;

/**
 * @brief TIM10 - TIM10 Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t CR1                 ; /*!< Offset: 0x00 (read-write) Control register 1 */
        struct {
            __IOM uint32_t CEN                 : 1; /*!< [0..0] Counter enable - Enables or disables the timer counter */
            __IOM uint32_t UDIS                : 1; /*!< [1..1] Update disable - Disables update event generation */
            __IOM uint32_t URS                 : 1; /*!< [2..2] Update request source - Selects the update event source */
            __IOM uint32_t OPM                 : 1; /*!< [3..3] One-pulse mode - Stops counter at next update event */
            __IOM uint32_t DIR                 : 1; /*!< [4..4] Direction - 0=up, 1=down */
            __IOM uint32_t CMS                 : 2; /*!< [6..5] Center-aligned mode selection - 00=edge-aligned, 01/10/11=center-aligned */
            __IOM uint32_t ARPE                : 1; /*!< [7..7] Auto-reload preload enable - Enables auto-reload register buffering */
            __IOM uint32_t CKD                 : 2; /*!< [9..8] Clock division - Configures clock division factor for dead-time and filter */
            __IM  uint32_t          : 22;
        } CR1_f;
    };
    union {
        __IOM uint32_t CR2                 ; /*!< Offset: 0x04 (read-write) Control register 2 */
        struct {
            __IOM uint32_t CCPC                : 1; /*!< [0..0] Capture/compare preloaded control */
            __IM  uint32_t          : 1;
            __IOM uint32_t CCUS                : 1; /*!< [2..2] Capture/compare update selection */
            __IOM uint32_t CCDS                : 1; /*!< [3..3] Capture/compare DMA selection */
            __IOM uint32_t MMS                 : 3; /*!< [6..4] Master mode selection */
            __IOM uint32_t TI1S                : 1; /*!< [7..7] TI1 selection - 0=CH1 pin, 1=XOR of CH1/CH2/CH3 */
            __IOM uint32_t OIS1                : 1; /*!< [8..8] Output idle state 1 */
            __IOM uint32_t OIS1N               : 1; /*!< [9..9] Output idle state 1N */
            __IM  uint32_t          : 22;
        } CR2_f;
    };
    union {
        __IOM uint32_t SMCR                ; /*!< Offset: 0x08 (read-write) Slave mode control register */
        struct {
            __IOM uint32_t SMS                 : 3; /*!< [2..0] Slave mode selection - 000=disabled, 001/010/011=encoder, 100=reset, 101=gated, 110=trigger, 111=ext clock */
            __IOM uint32_t OCCS                : 1; /*!< [3..3] OCREF clear selection - 0=COMPx, 1=ETRF */
            __IOM uint32_t TS                  : 3; /*!< [6..4] Trigger selection (bits 2:0) */
            __IOM uint32_t MSM                 : 1; /*!< [7..7] Master/slave mode - 0=no action, 1=TRGI delayed for sync */
            __IM  uint32_t          : 12;
            __IOM uint32_t TS_4_3              : 2; /*!< [21..20] Trigger selection (bits 4:3) */
            __IM  uint32_t          : 10;
        } SMCR_f;
    };
    union {
        __IOM uint32_t DIER                ; /*!< Offset: 0x0C (read-write) DMA/interrupt enable register - Enables timer DMA requests and interrupts */
        struct {
            __IOM uint32_t UIE                 : 1; /*!< [0..0] Update interrupt enable - Enables update event interrupt */
            __IOM uint32_t CC1IE               : 1; /*!< [1..1] Capture/compare 1 interrupt enable - Enables channel 1 capture/compare interrupt */
            __IM  uint32_t          : 4;
            __IOM uint32_t TIE                 : 1; /*!< [6..6] Trigger interrupt enable */
            __IM  uint32_t          : 1;
            __IOM uint32_t UDE                 : 1; /*!< [8..8] Update DMA request enable */
            __IOM uint32_t CC1DE               : 1; /*!< [9..9] Capture/compare 1 DMA request enable */
            __IM  uint32_t          : 4;
            __IOM uint32_t TDE                 : 1; /*!< [14..14] Trigger DMA request enable */
            __IM  uint32_t          : 17;
        } DIER_f;
    };
    union {
        __IOM uint32_t SR                  ; /*!< Offset: 0x10 (read-write) Status register */
        struct {
            __IOM uint32_t UIF                 : 1; /*!< [0..0] Update interrupt flag - Set when update event occurs */
            __IOM uint32_t CC1IF               : 1; /*!< [1..1] Capture/compare 1 interrupt flag - Set when channel 1 capture/compare occurs */
            __IM  uint32_t          : 4;
            __IOM uint32_t TIF                 : 1; /*!< [6..6] Trigger interrupt flag - Set when trigger event occurs */
            __IM  uint32_t          : 2;
            __IOM uint32_t CC1OF               : 1; /*!< [9..9] Capture/compare 1 overcapture flag - Set when channel 1 capture overrun occurs */
            __IM  uint32_t          : 22;
        } SR_f;
    };
    union {
        __IOM uint32_t EGR                 ; /*!< Offset: 0x14 (read-write) Event generation register - Software generates timer events */
        struct {
            __IOM uint32_t UG                  : 1; /*!< [0..0] Update generation - Software generates update event */
            __IOM uint32_t CC1G                : 1; /*!< [1..1] Capture/compare 1 generation - Software generates channel 1 event */
            __IM  uint32_t          : 4;
            __IOM uint32_t TG                  : 1; /*!< [6..6] Trigger generation - Software generates trigger event */
            __IM  uint32_t          : 25;
        } EGR_f;
    };
    union {
        union {
            __IOM uint32_t CCMR1_OUTPUT        ; /*!< Offset: 0x18 (read-write) Capture/compare mode register 1 (output) - Configures channels 1-2 in output mode */
            struct {
                __IOM uint32_t CC1S                : 2; /*!< [1..0] Capture/compare 1 selection - Selects channel 1 input/output direction */
                __IOM uint32_t OC1FE               : 1; /*!< [2..2] Output compare 1 fast enable - Enables fast output compare mode */
                __IOM uint32_t OC1PE               : 1; /*!< [3..3] Output compare 1 preload enable - Enables shadow register for channel 1 */
                __IOM uint32_t OC1M                : 3; /*!< [6..4] Output compare 1 mode - Selects output compare mode for channel 1 */
                __IM  uint32_t          : 25;
            } CCMR1_OUTPUT_f;
        };
        union {
            __IOM uint32_t CCMR1_INPUT         ; /*!< Offset: 0x18 (read-write) Capture/compare mode register 1 (input) - Configures channels 1-2 in input capture mode */
            struct {
                __IOM uint32_t CC1S                : 2; /*!< [1..0] Capture/compare 1 selection - Selects channel 1 input/output direction */
                __IOM uint32_t IC1PSC              : 2; /*!< [3..2] Input capture 1 prescaler - Configures prescaler for channel 1 input */
                __IOM uint32_t IC1F                : 4; /*!< [7..4] Input capture 1 filter - Configures digital filter for channel 1 input */
                __IM  uint32_t          : 24;
            } CCMR1_INPUT_f;
        };
    };
    __IO uint32_t RESERVED0;
    union {
        __IOM uint32_t CCER                ; /*!< Offset: 0x20 (read-write) Capture/compare enable register - Enables capture/compare channels */
        struct {
            __IOM uint32_t CC1E                : 1; /*!< [0..0] Capture/compare 1 enable - Enables channel 1 capture/compare output */
            __IOM uint32_t CC1P                : 1; /*!< [1..1] Capture/compare 1 polarity - Selects channel 1 output polarity */
            __IOM uint32_t CC1NE               : 1; /*!< [2..2] Capture/compare 1 complementary output enable */
            __IOM uint32_t CC1NP               : 1; /*!< [3..3] Capture/compare 1 complementary polarity - Selects channel 1 complementary polarity */
            __IM  uint32_t          : 28;
        } CCER_f;
    };
    union {
        __IOM uint32_t CNT                 ; /*!< Offset: 0x24 (read-write) Counter register - Current timer counter value */
        struct {
            __IOM uint32_t CNT                 : 16; /*!< [15..0] Counter register - Current timer counter value */
            __IM  uint32_t          : 16;
        } CNT_f;
    };
    union {
        __IOM uint32_t PSC                 ; /*!< Offset: 0x28 (read-write) Prescaler register - Clock divider for timer counter */
        struct {
            __IOM uint32_t PSC                 : 16; /*!< [15..0] Prescaler register - Clock divider for timer counter */
            __IM  uint32_t          : 16;
        } PSC_f;
    };
    union {
        __IOM uint32_t ARR                 ; /*!< Offset: 0x2C (read-write) Auto-reload register - Counter auto-reload value */
        struct {
            __IOM uint32_t ARR                 : 16; /*!< [15..0] Auto-reload register - Counter auto-reload value */
            __IM  uint32_t          : 16;
        } ARR_f;
    };
    __IO uint32_t RESERVED1;
    union {
        __IOM uint32_t CCR1                ; /*!< Offset: 0x34 (read-write) Capture/compare register 1 - Compare or capture value for channel 1 */
        struct {
            __IOM uint32_t CCR1                : 16; /*!< [15..0] Capture/compare register 1 - Compare value or captured counter value for channel 1 */
            __IM  uint32_t          : 16;
        } CCR1_f;
    };
    __IO uint32_t RESERVED2[4];
    union {
        __IOM uint32_t DCR                 ; /*!< Offset: 0x48 (read-write) DMA control register */
        struct {
            __IOM uint32_t DBA                 : 5; /*!< [4..0] DMA base address - Offset from TIMx_CR1 */
            __IM  uint32_t          : 3;
            __IOM uint32_t DBL                 : 5; /*!< [12..8] DMA burst length - Number of transfers */
            __IM  uint32_t          : 19;
        } DCR_f;
    };
    union {
        __IOM uint32_t DMAR                ; /*!< Offset: 0x4C (read-write) DMA address for burst transfers */
        struct {
            __IOM uint32_t DMAB                : 32; /*!< [31..0] DMA burst access register */
        } DMAR_f;
    };
    __IO uint32_t RESERVED3[6];
    union {
        __IOM uint32_t TISEL               ; /*!< Offset: 0x68 (read-write) Timer input selection register */
        struct {
            __IOM uint32_t TI1SEL              : 4; /*!< [3..0] TI1 input selection - 0000=tim_ti1_in0 */
            __IM  uint32_t          : 28;
        } TISEL_f;
    };
    union {
        __IOM uint32_t DBER                ; /*!< Offset: 0x6C (read-write) DMA burst enable register */
        struct {
            __IOM uint32_t UBE                 : 1; /*!< [0..0] Update DMA request type - 0=Single, 1=Burst */
            __IOM uint32_t CC1BE               : 1; /*!< [1..1] Capture/compare 1 DMA request type - 0=Single, 1=Burst */
            __IM  uint32_t          : 4;
            __IOM uint32_t TBE                 : 1; /*!< [6..6] Trigger DMA request type - 0=Single, 1=Burst */
            __IM  uint32_t          : 25;
        } DBER_f;
    };
} TIM10_TypeDef;

/**
 * @brief TIM9 - TIM9 Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t CR1                 ; /*!< Offset: 0x00 (read-write) Control register 1 */
        struct {
            __IOM uint32_t CEN                 : 1; /*!< [0..0] Counter enable - Enables or disables the timer counter */
            __IOM uint32_t UDIS                : 1; /*!< [1..1] Update disable - Disables update event generation */
            __IOM uint32_t URS                 : 1; /*!< [2..2] Update request source - Selects the update event source */
            __IOM uint32_t OPM                 : 1; /*!< [3..3] One-pulse mode - Stops counter at next update event */
            __IOM uint32_t DIR                 : 1; /*!< [4..4] Direction - 0=up, 1=down */
            __IOM uint32_t CMS                 : 2; /*!< [6..5] Center-aligned mode selection - 00=edge-aligned, 01/10/11=center-aligned */
            __IOM uint32_t ARPE                : 1; /*!< [7..7] Auto-reload preload enable - Enables auto-reload register buffering */
            __IOM uint32_t CKD                 : 2; /*!< [9..8] Clock division - Configures clock division factor for dead-time and filter */
            __IM  uint32_t          : 22;
        } CR1_f;
    };
    union {
        __IOM uint32_t CR2                 ; /*!< Offset: 0x04 (read-write) Control register 2 */
        struct {
            __IOM uint32_t CCPC                : 1; /*!< [0..0] Capture/compare preloaded control - Enables shadow register update on COM */
            __IM  uint32_t          : 1;
            __IOM uint32_t CCUS                : 1; /*!< [2..2] Capture/compare update selection - Selects when shadow registers update */
            __IOM uint32_t CCDS                : 1; /*!< [3..3] Capture/compare DMA selection - Selects DMA request source */
            __IOM uint32_t MMS                 : 3; /*!< [6..4] Master mode selection - Selects the trigger output (TRGO) source */
            __IOM uint32_t TI1S                : 1; /*!< [7..7] TI1 selection - 0=CH1 pin, 1=XOR of CH1/CH2/CH3 */
            __IM  uint32_t          : 24;
        } CR2_f;
    };
    union {
        __IOM uint32_t SMCR                ; /*!< Offset: 0x08 (read-write) Slave mode control register */
        struct {
            __IOM uint32_t SMS                 : 3; /*!< [2..0] Slave mode selection - 000=disabled, 001/010/011=encoder, 100=reset, 101=gated, 110=trigger, 111=ext clock */
            __IOM uint32_t OCCS                : 1; /*!< [3..3] OCREF clear selection - 0=COMPx, 1=ETRF */
            __IOM uint32_t TS                  : 3; /*!< [6..4] Trigger selection (bits 2:0) */
            __IOM uint32_t MSM                 : 1; /*!< [7..7] Master/slave mode - 0=no action, 1=TRGI delayed for sync */
            __IM  uint32_t          : 12;
            __IOM uint32_t TS_4_3              : 2; /*!< [21..20] Trigger selection (bits 4:3) */
            __IM  uint32_t          : 10;
        } SMCR_f;
    };
    union {
        __IOM uint32_t DIER                ; /*!< Offset: 0x0C (read-write) DMA/interrupt enable register - Enables timer DMA requests and interrupts */
        struct {
            __IOM uint32_t UIE                 : 1; /*!< [0..0] Update interrupt enable - Enables update event interrupt */
            __IOM uint32_t CC1IE               : 1; /*!< [1..1] Capture/compare 1 interrupt enable - Enables channel 1 capture/compare interrupt */
            __IOM uint32_t CC2IE               : 1; /*!< [2..2] Capture/compare 2 interrupt enable - Enables channel 2 capture/compare interrupt */
            __IM  uint32_t          : 3;
            __IOM uint32_t TIE                 : 1; /*!< [6..6] Trigger interrupt enable - Enables trigger event interrupt */
            __IM  uint32_t          : 1;
            __IOM uint32_t UDE                 : 1; /*!< [8..8] Update DMA request enable - Enables update event DMA request */
            __IOM uint32_t CC1DE               : 1; /*!< [9..9] Capture/compare 1 DMA request enable - Enables channel 1 DMA request */
            __IOM uint32_t CC2DE               : 1; /*!< [10..10] Capture/compare 2 DMA request enable - Enables channel 2 DMA request */
            __IM  uint32_t          : 3;
            __IOM uint32_t TDE                 : 1; /*!< [14..14] Trigger DMA request enable - Enables trigger event DMA request */
            __IM  uint32_t          : 17;
        } DIER_f;
    };
    union {
        __IOM uint32_t SR                  ; /*!< Offset: 0x10 (read-write) Status register */
        struct {
            __IOM uint32_t UIF                 : 1; /*!< [0..0] Update interrupt flag - Set when update event occurs */
            __IOM uint32_t CC1IF               : 1; /*!< [1..1] Capture/compare 1 interrupt flag - Set when channel 1 capture/compare occurs */
            __IOM uint32_t CC2IF               : 1; /*!< [2..2] Capture/compare 2 interrupt flag - Set when channel 2 capture/compare occurs */
            __IM  uint32_t          : 3;
            __IOM uint32_t TIF                 : 1; /*!< [6..6] Trigger interrupt flag - Set when trigger event occurs */
            __IM  uint32_t          : 2;
            __IOM uint32_t CC1OF               : 1; /*!< [9..9] Capture/compare 1 overcapture flag - Set when channel 1 capture overrun occurs */
            __IOM uint32_t CC2OF               : 1; /*!< [10..10] Capture/compare 2 overcapture flag - Set when channel 2 capture overrun occurs */
            __IM  uint32_t          : 21;
        } SR_f;
    };
    union {
        __IOM uint32_t EGR                 ; /*!< Offset: 0x14 (read-write) Event generation register - Software generates timer events */
        struct {
            __IOM uint32_t UG                  : 1; /*!< [0..0] Update generation - Software generates update event */
            __IOM uint32_t CC1G                : 1; /*!< [1..1] Capture/compare 1 generation - Software generates channel 1 event */
            __IOM uint32_t CC2G                : 1; /*!< [2..2] Capture/compare 2 generation - Software generates channel 2 event */
            __IM  uint32_t          : 3;
            __IOM uint32_t TG                  : 1; /*!< [6..6] Trigger generation - Software generates trigger event */
            __IM  uint32_t          : 25;
        } EGR_f;
    };
    union {
        union {
            __IOM uint32_t CCMR1_OUTPUT        ; /*!< Offset: 0x18 (read-write) Capture/compare mode register 1 (output) - Configures channels 1-2 in output mode */
            struct {
                __IOM uint32_t CC1S                : 2; /*!< [1..0] Capture/compare 1 selection - Selects channel 1 input/output direction */
                __IOM uint32_t OC1FE               : 1; /*!< [2..2] Output compare 1 fast enable - Enables fast output compare mode */
                __IOM uint32_t OC1PE               : 1; /*!< [3..3] Output compare 1 preload enable - Enables shadow register for channel 1 */
                __IOM uint32_t OC1M                : 3; /*!< [6..4] Output compare 1 mode - Selects output compare mode for channel 1 */
                __IOM uint32_t OC1CE               : 1; /*!< [7..7] Output compare 1 clear enable - Enables ETRF to clear OC1REF */
                __IOM uint32_t CC2S                : 2; /*!< [9..8] Capture/compare 2 selection - Selects channel 2 input/output direction */
                __IOM uint32_t OC2FE               : 1; /*!< [10..10] Output compare 2 fast enable - Enables fast output compare mode */
                __IOM uint32_t OC2PE               : 1; /*!< [11..11] Output compare 2 preload enable - Enables shadow register for channel 2 */
                __IOM uint32_t OC2M                : 3; /*!< [14..12] Output compare 2 mode - Selects output compare mode for channel 2 */
                __IOM uint32_t OC2CE               : 1; /*!< [15..15] Output compare 2 clear enable - Enables ETRF to clear OC2REF */
                __IM  uint32_t          : 16;
            } CCMR1_OUTPUT_f;
        };
        union {
            __IOM uint32_t CCMR1_INPUT         ; /*!< Offset: 0x18 (read-write) Capture/compare mode register 1 (input) - Configures channels 1-2 in input capture mode */
            struct {
                __IOM uint32_t CC1S                : 2; /*!< [1..0] Capture/compare 1 selection - Selects channel 1 input/output direction */
                __IOM uint32_t IC1PSC              : 2; /*!< [3..2] Input capture 1 prescaler - Configures prescaler for channel 1 input */
                __IOM uint32_t IC1F                : 4; /*!< [7..4] Input capture 1 filter - Configures digital filter for channel 1 input */
                __IOM uint32_t CC2S                : 2; /*!< [9..8] Capture/compare 2 selection - Selects channel 2 input/output direction */
                __IOM uint32_t IC2PSC              : 2; /*!< [11..10] Input capture 2 prescaler - Configures prescaler for channel 2 input */
                __IOM uint32_t IC2F                : 4; /*!< [15..12] Input capture 2 filter - Configures digital filter for channel 2 input */
                __IM  uint32_t          : 16;
            } CCMR1_INPUT_f;
        };
    };
    __IO uint32_t RESERVED0;
    union {
        __IOM uint32_t CCER                ; /*!< Offset: 0x20 (read-write) Capture/compare enable register - Enables capture/compare channels */
        struct {
            __IOM uint32_t CC1E                : 1; /*!< [0..0] Capture/compare 1 enable - Enables channel 1 capture/compare output */
            __IOM uint32_t CC1P                : 1; /*!< [1..1] Capture/compare 1 polarity - Selects channel 1 output polarity */
            __IOM uint32_t CC1NE               : 1; /*!< [2..2] Capture/compare 1 complementary output enable */
            __IOM uint32_t CC1NP               : 1; /*!< [3..3] Capture/compare 1 complementary polarity - Selects channel 1 complementary polarity */
            __IOM uint32_t CC2E                : 1; /*!< [4..4] Capture/compare 2 enable - Enables channel 2 capture/compare output */
            __IOM uint32_t CC2P                : 1; /*!< [5..5] Capture/compare 2 polarity - Selects channel 2 output polarity */
            __IOM uint32_t CC2NE               : 1; /*!< [6..6] Capture/compare 2 complementary output enable */
            __IOM uint32_t CC2NP               : 1; /*!< [7..7] Capture/compare 2 complementary polarity - Selects channel 2 complementary polarity */
            __IM  uint32_t          : 24;
        } CCER_f;
    };
    union {
        __IOM uint32_t CNT                 ; /*!< Offset: 0x24 (read-write) Counter register - Current timer counter value */
        struct {
            __IOM uint32_t CNT                 : 16; /*!< [15..0] Counter value */
            __IM  uint32_t          : 16;
        } CNT_f;
    };
    union {
        __IOM uint32_t PSC                 ; /*!< Offset: 0x28 (read-write) Prescaler register - Clock divider for timer counter */
        struct {
            __IOM uint32_t PSC                 : 16; /*!< [15..0] Prescaler value - Counter clock = CK_PSC / (PSC+1) */
            __IM  uint32_t          : 16;
        } PSC_f;
    };
    union {
        __IOM uint32_t ARR                 ; /*!< Offset: 0x2C (read-write) Auto-reload register - Counter auto-reload value */
        struct {
            __IOM uint32_t ARR                 : 16; /*!< [15..0] Auto-reload value */
            __IM  uint32_t          : 16;
        } ARR_f;
    };
    __IO uint32_t RESERVED1;
    union {
        __IOM uint32_t CCR1                ; /*!< Offset: 0x34 (read-write) Capture/compare register 1 - Compare or capture value for channel 1 */
        struct {
            __IOM uint32_t CCR1                : 16; /*!< [15..0] Capture/compare 1 value */
            __IM  uint32_t          : 16;
        } CCR1_f;
    };
    union {
        __IOM uint32_t CCR2                ; /*!< Offset: 0x38 (read-write) Capture/compare register 2 - Compare or capture value for channel 2 */
        struct {
            __IOM uint32_t CCR2                : 16; /*!< [15..0] Capture/compare 2 value */
            __IM  uint32_t          : 16;
        } CCR2_f;
    };
    __IO uint32_t RESERVED2[3];
    union {
        __IOM uint32_t DCR                 ; /*!< Offset: 0x48 (read-write) DMA control register */
        struct {
            __IOM uint32_t DBA                 : 5; /*!< [4..0] DMA base address - Offset from TIMx_CR1 */
            __IM  uint32_t          : 3;
            __IOM uint32_t DBL                 : 5; /*!< [12..8] DMA burst length - Number of transfers */
            __IM  uint32_t          : 19;
        } DCR_f;
    };
    union {
        __IOM uint32_t DMAR                ; /*!< Offset: 0x4C (read-write) DMA address for burst transfers */
        struct {
            __IOM uint32_t DMAB                : 32; /*!< [31..0] DMA burst access register */
        } DMAR_f;
    };
    __IO uint32_t RESERVED3[6];
    union {
        __IOM uint32_t TISEL               ; /*!< Offset: 0x68 (read-write) Timer input selection register */
        struct {
            __IOM uint32_t TI1SEL              : 4; /*!< [3..0] TI1 input selection - 0000=tim_ti1_in0 */
            __IM  uint32_t          : 4;
            __IOM uint32_t TI2SEL              : 4; /*!< [11..8] TI2 input selection - 0000=tim_ti2_in0 */
            __IM  uint32_t          : 20;
        } TISEL_f;
    };
    union {
        __IOM uint32_t DBER                ; /*!< Offset: 0x6C (read-write) DMA burst enable register */
        struct {
            __IOM uint32_t UBE                 : 1; /*!< [0..0] Update DMA request type - 0=Single, 1=Burst */
            __IOM uint32_t CC1BE               : 1; /*!< [1..1] Capture/compare 1 DMA request type - 0=Single, 1=Burst */
            __IOM uint32_t CC2BE               : 1; /*!< [2..2] Capture/compare 2 DMA request type - 0=Single, 1=Burst */
            __IM  uint32_t          : 3;
            __IOM uint32_t TBE                 : 1; /*!< [6..6] Trigger DMA request type - 0=Single, 1=Burst */
            __IM  uint32_t          : 25;
        } DBER_f;
    };
} TIM9_TypeDef;

/**
 * @brief TIM64B - TIM64B Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t CTRL                ; /*!< Offset: 0x00 (read-write) Control register - Configures timer counter mode and prescaler */
        struct {
            __IOM uint32_t CEN                 : 1; /*!< [0..0] Counter enable - Enables or disables the timer counter */
            __IOM uint32_t CMOD                : 1; /*!< [1..1] Counter mode: 0=free-run, 1=loop */
            __IOM uint32_t PSC                 : 6; /*!< [7..2] Prescaler register - Clock divider for timer counter */
            __IOM uint32_t IE                  : 1; /*!< [8..8] Interrupt enable register - Enables or disables various interrupt sources */
            __IOM uint32_t ARPE                : 1; /*!< [9..9] Auto-reload preload enable - Enables auto-reload register buffering */
            __IOM uint32_t ARRLOAD             : 1; /*!< [10..10] Auto-reload load - Loads auto-reload value into counter */
            __IOM uint32_t CNTLOAD             : 1; /*!< [11..11] Counter load - Loads counter with preset value */
            __IM  uint32_t          : 20;
        } CTRL_f;
    };
    union {
        __IOM uint32_t SR                  ; /*!< Offset: 0x04 (read-write) Status register */
        struct {
            __IM  uint32_t          : 8;
            __IOM uint32_t UF                  : 1; /*!< [8..8] Update flag */
            __IM  uint32_t          : 23;
        } SR_f;
    };
    union {
        __IOM uint32_t ARRL                ; /*!< Offset: 0x08 (read-write) Auto-reload low register - Lower 32 bits of auto-reload value */
        struct {
            __IOM uint32_t ARRL                : 32; /*!< [31..0] Auto-reload low - Lower 32 bits of auto-reload value */
        } ARRL_f;
    };
    union {
        __IOM uint32_t ARRH                ; /*!< Offset: 0x0C (read-write) Auto-reload high register - Upper 32 bits of auto-reload value */
        struct {
            __IOM uint32_t ARRH                : 32; /*!< [31..0] Auto-reload high - Upper 32 bits of auto-reload value */
        } ARRH_f;
    };
    union {
        __IOM uint32_t CNTL                ; /*!< Offset: 0x10 (read-write) Counter low register - Lower 32 bits of current counter value */
        struct {
            __IOM uint32_t CNTL                : 32; /*!< [31..0] Count low - Lower 32 bits of counter value */
        } CNTL_f;
    };
    union {
        __IOM uint32_t CNTH                ; /*!< Offset: 0x14 (read-write) Counter high register - Upper 32 bits of current counter value */
        struct {
            __IOM uint32_t CNTH                : 32; /*!< [31..0] Count high - Upper 32 bits of counter value */
        } CNTH_f;
    };
} TIM64B_TypeDef;

/**
 * @brief RTC - RTC Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t WP                  ; /*!< Offset: 0x00 (read-write) WP - WP value */
        struct {
            __IOM uint32_t WE                  : 32; /*!< [31..0] WE - WE value */
        } WP_f;
    };
    union {
        __IOM uint32_t IE                  ; /*!< Offset: 0x04 (read-write) Interrupt enable register - Enables or disables various interrupt sources */
        struct {
            __IOM uint32_t DATE_IE             : 1; /*!< [0..0] Date interrupt enable - Enables date event interrupt */
            __IOM uint32_t HOUR_IE             : 1; /*!< [1..1] Hour interrupt enable - Enables hour change interrupt */
            __IOM uint32_t MIN_IE              : 1; /*!< [2..2] Minute interrupt enable - Enables minute change interrupt */
            __IOM uint32_t SEC_IE              : 1; /*!< [3..3] Security interrupt enable - Enables security interrupt */
            __IOM uint32_t BIT_2HZ_IE          : 1; /*!< [4..4] 2 Hz interrupt enable - Enables 2 Hz periodic interrupt */
            __IOM uint32_t BIT_4HZ_IE          : 1; /*!< [5..5] 4 Hz interrupt enable - Enables 4 Hz periodic interrupt */
            __IOM uint32_t BIT_8HZ_IE          : 1; /*!< [6..6] 8 Hz interrupt enable - Enables 8 Hz periodic interrupt */
            __IOM uint32_t BIT_16HZ_IE         : 1; /*!< [7..7] 16 Hz interrupt enable - Enables 16 Hz periodic interrupt */
            __IOM uint32_t BIT_64HZ_IE         : 1; /*!< [8..8] 64 Hz interrupt enable - Enables 64 Hz periodic interrupt */
            __IOM uint32_t BIT_256HZ_IE        : 1; /*!< [9..9] 256 Hz interrupt enable - Enables 256 Hz periodic interrupt */
            __IOM uint32_t BIT_1KHZ_IE         : 1; /*!< [10..10] 1 kHz interrupt enable - Enables 1 kHz periodic interrupt */
            __IOM uint32_t ALM_IE              : 1; /*!< [11..11] Alarm interrupt enable - Enables RTC alarm interrupt */
            __IOM uint32_t ADJ32_IE            : 1; /*!< [12..12] Adjust 32 Hz interrupt enable - Enables 32 Hz calibration adjustment interrupt */
            __IOM uint32_t STP1FIE             : 1; /*!< [13..13] Stop 1 flag interrupt enable - Enables STP1 flag interrupt */
            __IOM uint32_t STP1RIE             : 1; /*!< [14..14] Stop 1 ready interrupt enable - Enables STP1 ready interrupt */
            __IOM uint32_t STP2FIE             : 1; /*!< [15..15] Stop 2 flag interrupt enable - Enables STP2 flag interrupt */
            __IOM uint32_t STP2RIE             : 1; /*!< [16..16] Stop 2 ready interrupt enable - Enables STP2 ready interrupt */
            __IOM uint32_t WUTIE               : 1; /*!< [17..17] Wakeup timer interrupt enable - Enables wakeup timer interrupt */
            __IM  uint32_t          : 14;
        } IE_f;
    };
    union {
        __IM  uint32_t SR                  ; /*!< Offset: 0x08 (read-only) Status register - Power management status flags */
        struct {
            __IM  uint32_t DATE_IF             : 1; /*!< [0..0] Date interrupt flag - Date event interrupt flag */
            __IM  uint32_t HOUR_IF             : 1; /*!< [1..1] Hour interrupt flag - Hour change interrupt flag */
            __IM  uint32_t MIN_IF              : 1; /*!< [2..2] Minute interrupt flag - Minute change interrupt flag */
            __IM  uint32_t SEC_IF              : 1; /*!< [3..3] Security interrupt flag - Security interrupt flag */
            __IM  uint32_t BIT_2HZ_IF          : 1; /*!< [4..4] 2 Hz interrupt flag - Set on 2 Hz periodic event */
            __IM  uint32_t BIT_4HZ_IF          : 1; /*!< [5..5] 4 Hz interrupt flag - Set on 4 Hz periodic event */
            __IM  uint32_t BIT_8HZ_IF          : 1; /*!< [6..6] 8 Hz interrupt flag - Set on 8 Hz periodic event */
            __IM  uint32_t BIT_16HZ_IF         : 1; /*!< [7..7] 16 Hz interrupt flag - Set on 16 Hz periodic event */
            __IM  uint32_t BIT_64HZ_IF         : 1; /*!< [8..8] 64 Hz interrupt flag - Set on 64 Hz periodic event */
            __IM  uint32_t BIT_256HZ_IF        : 1; /*!< [9..9] 256 Hz interrupt flag - Set on 256 Hz periodic event */
            __IM  uint32_t BIT_1KHZ_IF         : 1; /*!< [10..10] 1 kHz interrupt flag - Set on 1 kHz periodic event */
            __IM  uint32_t ALM_IF              : 1; /*!< [11..11] Alarm interrupt flag - Set when RTC alarm occurs */
            __IM  uint32_t ADJ32_IF            : 1; /*!< [12..12] Adjust 32 Hz interrupt flag - Set on 32 Hz calibration adjustment event */
            __IM  uint32_t STP1FIF             : 1; /*!< [13..13] Stop 1 flag interrupt flag - STP1 flag interrupt status */
            __IM  uint32_t STP1RIF             : 1; /*!< [14..14] Stop 1 ready interrupt flag - STP1 ready interrupt status */
            __IM  uint32_t STP2FIF             : 1; /*!< [15..15] Stop 2 flag interrupt flag - STP2 flag interrupt status */
            __IM  uint32_t STP2RIF             : 1; /*!< [16..16] Stop 2 ready interrupt flag - STP2 ready interrupt status */
            __IM  uint32_t WUTF                : 1; /*!< [17..17] Wakeup timer flag - Wakeup timer event flag */
            __IM  uint32_t          : 6;
            __IM  uint32_t WUTWF               : 1; /*!< [24..24] Wakeup timer write flag - Wakeup timer write in progress */
            __IM  uint32_t          : 7;
        } SR_f;
    };
    union {
        __IOM uint32_t SEC                 ; /*!< Offset: 0x0C (read-write) Second - RTC second counter value */
        struct {
            __IOM uint32_t BCDSEC              : 7; /*!< [6..0] BCD second - RTC second value in BCD format */
            __IM  uint32_t          : 25;
        } SEC_f;
    };
    union {
        __IOM uint32_t MIN                 ; /*!< Offset: 0x10 (read-write) Minute - RTC minute counter value */
        struct {
            __IOM uint32_t BCDMIN              : 7; /*!< [6..0] BCD minute - RTC minute value in BCD format */
            __IM  uint32_t          : 25;
        } MIN_f;
    };
    union {
        __IOM uint32_t HOUR                ; /*!< Offset: 0x14 (read-write) Hour - RTC hour counter value */
        struct {
            __IOM uint32_t BCDHOUR             : 6; /*!< [5..0] BCD hour - RTC hour value in BCD format */
            __IM  uint32_t          : 26;
        } HOUR_f;
    };
    union {
        __IOM uint32_t DAY                 ; /*!< Offset: 0x18 (read-write) Day - RTC day counter value */
        struct {
            __IOM uint32_t BCDDATE             : 6; /*!< [5..0] BCD date - RTC date value in BCD format */
            __IM  uint32_t          : 26;
        } DAY_f;
    };
    union {
        __IOM uint32_t WEEK                ; /*!< Offset: 0x1C (read-write) Week - RTC weekday counter value */
        struct {
            __IOM uint32_t BCDWEEK             : 3; /*!< [2..0] BCD weekday - RTC weekday value in BCD format */
            __IM  uint32_t          : 29;
        } WEEK_f;
    };
    union {
        __IOM uint32_t MONTH               ; /*!< Offset: 0x20 (read-write) Month - RTC month counter value */
        struct {
            __IOM uint32_t BCDMONTH            : 5; /*!< [4..0] BCD month - RTC month value in BCD format */
            __IM  uint32_t          : 27;
        } MONTH_f;
    };
    union {
        __IOM uint32_t YEAR                ; /*!< Offset: 0x24 (read-write) Year - RTC year counter value */
        struct {
            __IOM uint32_t BCDYEAR             : 8; /*!< [7..0] BCD year - RTC year value in BCD format */
            __IM  uint32_t          : 24;
        } YEAR_f;
    };
    union {
        __IOM uint32_t ALM                 ; /*!< Offset: 0x28 (read-write) Alarm - RTC alarm status or control */
        struct {
            __IOM uint32_t ALMSEC              : 7; /*!< [6..0] Alarm second - Alarm second value */
            __IM  uint32_t          : 1;
            __IOM uint32_t ALMMIN              : 7; /*!< [14..8] Alarm minute - Alarm minute value */
            __IM  uint32_t          : 1;
            __IOM uint32_t ALMHOUR             : 6; /*!< [21..16] Alarm hour - Alarm hour value */
            __IM  uint32_t          : 2;
            __IOM uint32_t ALMWEEK_ALMDAY      : 7; /*!< [30..24] Alarm week/day - Alarm weekday or date value */
            __IOM uint32_t ALM_WDS             : 1; /*!< [31..31] Alarm week/day selection - Selects weekday or date for alarm */
        } ALM_f;
    };
    union {
        __IOM uint32_t CR                  ; /*!< Offset: 0x2C (read-write) Control register - CRC calculation control */
        struct {
            __IOM uint32_t FSEL                : 4; /*!< [3..0] Function select - Function selection for multiplexed pin */
            __IOM uint32_t ALM_EN              : 1; /*!< [4..4] Alarm enable - Enables RTC alarm function */
            __IOM uint32_t TAMP1EN             : 1; /*!< [5..5] Tamper 1 enable - Enables RTC tamper 1 detection */
            __IOM uint32_t TAMP2EN             : 1; /*!< [6..6] Tamper 2 enable - Enables RTC tamper 2 detection */
            __IOM uint32_t TAMP1RCLR           : 1; /*!< [7..7] Tamper 1 raw clear - Clears tamper 1 raw flag */
            __IOM uint32_t TAMP1FCLR           : 1; /*!< [8..8] Tamper 1 flag clear - Clears tamper 1 flag */
            __IOM uint32_t ALM_MSKM            : 1; /*!< [9..9] Alarm mask minute - Masks minute comparison for alarm */
            __IOM uint32_t ALM_MSKH            : 1; /*!< [10..10] Alarm mask hour - Masks hour comparison for alarm */
            __IOM uint32_t ALM_MSKD            : 1; /*!< [11..11] Alarm mask day - Masks day comparison for alarm */
            __IOM uint32_t TAMP1FLTEN          : 1; /*!< [12..12] Tamper 1 filter enable - Enables tamper 1 filter */
            __IOM uint32_t TAMP1FLT            : 2; /*!< [14..13] Tamper 1 filter - Tamper 1 filter configuration */
            __IOM uint32_t TS1EDGE             : 1; /*!< [15..15] TS1 edge - Trigger selection 1 edge configuration */
            __IOM uint32_t TAMP2RCLR           : 1; /*!< [16..16] Tamper 2 raw clear - Clears tamper 2 raw flag */
            __IOM uint32_t TAMP2FCLR           : 1; /*!< [17..17] Tamper 2 flag clear - Clears tamper 2 flag */
            __IOM uint32_t TAMP2FLTEN          : 1; /*!< [18..18] Tamper 2 filter enable - Enables tamper 2 filter */
            __IOM uint32_t TAMP2FLT            : 2; /*!< [20..19] Tamper 2 filter - Tamper 2 filter configuration */
            __IOM uint32_t TS2EDGE             : 1; /*!< [21..21] TS2 edge - Trigger selection 2 edge configuration */
            __IOM uint32_t TAMPFLTCLK          : 1; /*!< [22..22] Tamper filter clock - Tamper filter sampling clock */
            __IOM uint32_t WUTE                : 1; /*!< [23..23] Wakeup timer enable - Enables wakeup timer */
            __IOM uint32_t WUCKSEL             : 3; /*!< [26..24] Wakeup clock select - Wakeup timer clock source */
            __IM  uint32_t          : 5;
        } CR_f;
    };
    union {
        __IOM uint32_t ADJUST              ; /*!< Offset: 0x30 (read-write) Adjustment value - RTC calibration adjustment value */
        struct {
            __IOM uint32_t ADJVALUE            : 9; /*!< [8..0] Adjust value - Calibration adjustment magnitude */
            __IOM uint32_t ADJSIGN             : 1; /*!< [9..9] Adjust sign - Selects positive or negative calibration adjustment */
            __IM  uint32_t          : 22;
        } ADJUST_f;
    };
    __IO uint32_t RESERVED0[2];
    union {
        __IOM uint32_t MSECCNT             ; /*!< Offset: 0x3C (read-write) Millisecond counter - Millisecond counter value */
        struct {
            __IOM uint32_t MSCNT               : 6; /*!< [5..0] MS counter - Millisecond counter value */
            __IM  uint32_t          : 26;
        } MSECCNT_f;
    };
    __IO uint32_t RESERVED1;
    union {
        __IM  uint32_t CLKSTAMP1           ; /*!< Offset: 0x44 (read-only) Clock stamp 1 - Clock timestamp value 1 */
        struct {
            __IM  uint32_t SECSTP1             : 7; /*!< [6..0] Security stop 1 - Security STOP 1 control */
            __IM  uint32_t          : 1;
            __IM  uint32_t MINSTP1             : 7; /*!< [14..8] Minimum stop 1 time - Minimum STOP 1 mode duration */
            __IM  uint32_t          : 1;
            __IM  uint32_t HRSTP1              : 6; /*!< [21..16] HRSTP1 - HRNG start-up time 1 */
            __IM  uint32_t          : 10;
        } CLKSTAMP1_f;
    };
    union {
        __IM  uint32_t CALSTAMP1           ; /*!< Offset: 0x48 (read-only) Calibration stamp 1 - Calibration timestamp 1 */
        struct {
            __IM  uint32_t DAYSTP1             : 6; /*!< [5..0] Day step 1 - Day adjustment step 1 */
            __IM  uint32_t          : 2;
            __IM  uint32_t WKSTP1              : 3; /*!< [10..8] Wakeup stop 1 - Wakeup from STOP 1 */
            __IM  uint32_t          : 5;
            __IM  uint32_t MONSTP1             : 5; /*!< [20..16] Monitor stop 1 - STOP 1 mode monitor */
            __IM  uint32_t YEARSTP1            : 11; /*!< [31..21] Year step 1 - Year adjustment step 1 */
        } CALSTAMP1_f;
    };
    union {
        __IM  uint32_t CLKSTAMP2           ; /*!< Offset: 0x4C (read-only) Clock stamp 2 - Clock timestamp value 2 */
        struct {
            __IM  uint32_t SECSTP2             : 7; /*!< [6..0] Security stop 2 - Security STOP 2 control */
            __IM  uint32_t          : 1;
            __IM  uint32_t MINSTP2             : 7; /*!< [14..8] Minimum stop 2 time - Minimum STOP 2 mode duration */
            __IM  uint32_t          : 1;
            __IM  uint32_t HRSTP2              : 6; /*!< [21..16] HRSTP2 - HRNG start-up time 2 */
            __IM  uint32_t          : 10;
        } CLKSTAMP2_f;
    };
    union {
        __IM  uint32_t CALSTAMP2           ; /*!< Offset: 0x50 (read-only) Calibration stamp 2 - Calibration timestamp 2 */
        struct {
            __IM  uint32_t DAYSTP2             : 6; /*!< [5..0] Day step 2 - Day adjustment step 2 */
            __IM  uint32_t          : 2;
            __IM  uint32_t WKSTP2              : 3; /*!< [10..8] Wakeup stop 2 - Wakeup from STOP 2 */
            __IM  uint32_t          : 5;
            __IM  uint32_t MONSTP2             : 5; /*!< [20..16] Monitor stop 2 - STOP 2 mode monitor */
            __IM  uint32_t          : 3;
            __IM  uint32_t YEARSTP2            : 8; /*!< [31..24] Year step 2 - Year adjustment step 2 */
        } CALSTAMP2_f;
    };
    union {
        __IOM uint32_t WUTR                ; /*!< Offset: 0x54 (read-write) Wakeup timer reload - Wakeup timer reload value */
        struct {
            __IOM uint32_t WUT                 : 16; /*!< [15..0] Wakeup timer - Wakeup timer counter */
            __IM  uint32_t          : 16;
        } WUTR_f;
    };
    __IO uint32_t RESERVED2[6];
    union {
        __IOM uint32_t BAKUP0              ; /*!< Offset: 0x70 (read-write) Backup register 0 - RTC backup data storage register 0 */
        struct {
            __IOM uint32_t BAKUP               : 32; /*!< [31..0] Backup data - Backup storage data value */
        } BAKUP0_f;
    };
    union {
        __IOM uint32_t BAKUP1              ; /*!< Offset: 0x74 (read-write) Backup register 1 - RTC backup data storage register 1 */
        struct {
            __IOM uint32_t BAKUP               : 32; /*!< [31..0] Backup data - Backup storage data value */
        } BAKUP1_f;
    };
    union {
        __IOM uint32_t BAKUP2              ; /*!< Offset: 0x78 (read-write) Backup register 2 - RTC backup data storage register 2 */
        struct {
            __IOM uint32_t BAKUP               : 32; /*!< [31..0] Backup data - Backup storage data value */
        } BAKUP2_f;
    };
    union {
        __IOM uint32_t BAKUP3              ; /*!< Offset: 0x7c (read-write) Backup register 3 - RTC backup data storage register 3 */
        struct {
            __IOM uint32_t BAKUP               : 32; /*!< [31..0] Backup data - Backup storage data value */
        } BAKUP3_f;
    };
    union {
        __IOM uint32_t BAKUP4              ; /*!< Offset: 0x80 (read-write) Backup register 4 - RTC backup data storage register 4 */
        struct {
            __IOM uint32_t BAKUP               : 32; /*!< [31..0] Backup data - Backup storage data value */
        } BAKUP4_f;
    };
    union {
        __IOM uint32_t BAKUP5              ; /*!< Offset: 0x84 (read-write) Backup register 5 - RTC backup data storage register 5 */
        struct {
            __IOM uint32_t BAKUP               : 32; /*!< [31..0] Backup data - Backup storage data value */
        } BAKUP5_f;
    };
    union {
        __IOM uint32_t BAKUP6              ; /*!< Offset: 0x88 (read-write) Backup register 6 - RTC backup data storage register 6 */
        struct {
            __IOM uint32_t BAKUP               : 32; /*!< [31..0] Backup data - Backup storage data value */
        } BAKUP6_f;
    };
    union {
        __IOM uint32_t BAKUP7              ; /*!< Offset: 0x8c (read-write) Backup register 7 - RTC backup data storage register 7 */
        struct {
            __IOM uint32_t BAKUP               : 32; /*!< [31..0] Backup data - Backup storage data value */
        } BAKUP7_f;
    };
    union {
        __IOM uint32_t BAKUP8              ; /*!< Offset: 0x90 (read-write) Backup register 8 - RTC backup data storage register 8 */
        struct {
            __IOM uint32_t BAKUP               : 32; /*!< [31..0] Backup data - Backup storage data value */
        } BAKUP8_f;
    };
    union {
        __IOM uint32_t BAKUP9              ; /*!< Offset: 0x94 (read-write) Backup register 9 - RTC backup data storage register 9 */
        struct {
            __IOM uint32_t BAKUP               : 32; /*!< [31..0] Backup data - Backup storage data value */
        } BAKUP9_f;
    };
    union {
        __IOM uint32_t BAKUP10             ; /*!< Offset: 0x98 (read-write) Backup register 10 - RTC backup data storage register 10 */
        struct {
            __IOM uint32_t BAKUP               : 32; /*!< [31..0] Backup data - Backup storage data value */
        } BAKUP10_f;
    };
    union {
        __IOM uint32_t BAKUP11             ; /*!< Offset: 0x9c (read-write) Backup register 11 - RTC backup data storage register 11 */
        struct {
            __IOM uint32_t BAKUP               : 32; /*!< [31..0] Backup data - Backup storage data value */
        } BAKUP11_f;
    };
    union {
        __IOM uint32_t BAKUP12             ; /*!< Offset: 0xa0 (read-write) Backup register 12 - RTC backup data storage register 12 */
        struct {
            __IOM uint32_t BAKUP               : 32; /*!< [31..0] Backup data - Backup storage data value */
        } BAKUP12_f;
    };
    union {
        __IOM uint32_t BAKUP13             ; /*!< Offset: 0xa4 (read-write) Backup register 13 - RTC backup data storage register 13 */
        struct {
            __IOM uint32_t BAKUP               : 32; /*!< [31..0] Backup data - Backup storage data value */
        } BAKUP13_f;
    };
    union {
        __IOM uint32_t BAKUP14             ; /*!< Offset: 0xa8 (read-write) Backup register 14 - RTC backup data storage register 14 */
        struct {
            __IOM uint32_t BAKUP               : 32; /*!< [31..0] Backup data - Backup storage data value */
        } BAKUP14_f;
    };
    union {
        __IOM uint32_t BAKUP15             ; /*!< Offset: 0xac (read-write) Backup register 15 - RTC backup data storage register 15 */
        struct {
            __IOM uint32_t BAKUP               : 32; /*!< [31..0] Backup data - Backup storage data value */
        } BAKUP15_f;
    };
} RTC_TypeDef;

/**
 * @brief WDT - WDT Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t LOAD                ; /*!< Offset: 0x00 (read-write) Load register - Watchdog counter load value */
        struct {
            __IOM uint32_t LOAD                : 32; /*!< [31..0] Load register - Watchdog counter load value */
        } LOAD_f;
    };
    union {
        __IM  uint32_t COUNT               ; /*!< Offset: 0x04 (read-only) Count value - Current count value */
        struct {
            __IM  uint32_t COUNT               : 32; /*!< [31..0] Count value - Current count value */
        } COUNT_f;
    };
    union {
        __IOM uint32_t CTRL                ; /*!< Offset: 0x08 (read-write) Control register */
        struct {
            __IOM uint32_t DIVISOR             : 3; /*!< [2..0] Watchdog clock prescaler divisor */
            __IM  uint32_t          : 1;
            __IOM uint32_t INTEN               : 1; /*!< [4..4] Interrupt enable */
            __IM  uint32_t          : 1;
            __IOM uint32_t MODE                : 1; /*!< [6..6] Watchdog mode: 0=reset, 1=interrupt */
            __IOM uint32_t EN                  : 1; /*!< [7..7] Watchdog enable */
            __IM  uint32_t          : 24;
        } CTRL_f;
    };
    union {
        __OM  uint32_t FEED                ; /*!< Offset: 0x0C (write-only) Feed - Watchdog feed/refresh control */
        struct {
            __OM  uint32_t FEED                : 32; /*!< [31..0] Feed - Watchdog feed/refresh control */
        } FEED_f;
    };
    union {
        __IOM uint32_t INTCLRTIME          ; /*!< Offset: 0x10 (read-write) Interrupt clear time - Interrupt clear timing */
        struct {
            __IOM uint32_t INTCLRT             : 16; /*!< [15..0] Interrupt clear - Interrupt clear register */
            __IM  uint32_t          : 16;
        } INTCLRTIME_f;
    };
    union {
        __IM  uint32_t RIS                 ; /*!< Offset: 0x14 (read-only) Raw interrupt status register */
        struct {
            __IM  uint32_t WDTRIS              : 1; /*!< [0..0] Raw interrupt status flag */
            __IM  uint32_t          : 31;
        } RIS_f;
    };
} WDT_TypeDef;

/**
 * @brief IWDT - IWDT Register Structure
 */
typedef struct {
    union {
        __OM  uint32_t CMDR                ; /*!< Offset: 0x00 (write-only) Command register */
        struct {
            __OM  uint32_t CMD                 : 32; /*!< [31..0] Command: 0xAAAA=feed, 0x5555=unlock write protect, 0xCCCC=enable IWDT, 0x6666=wakeup enable, 0x9999=wakeup disable */
        } CMDR_f;
    };
    union {
        __IOM uint32_t PR                  ; /*!< Offset: 0x04 (read-write) Prescaler register */
        struct {
            __IOM uint32_t PR                  : 3; /*!< [2..0] Prescaler factor */
            __IM  uint32_t          : 29;
        } PR_f;
    };
    union {
        __IOM uint32_t RLR                 ; /*!< Offset: 0x08 (read-write) Reload register */
        struct {
            __IOM uint32_t PL                  : 12; /*!< [11..0] Watchdog reload value */
            __IM  uint32_t          : 20;
        } RLR_f;
    };
    union {
        __IM  uint32_t SR                  ; /*!< Offset: 0x0C (read-only) Status register */
        struct {
            __IM  uint32_t PVU                 : 1; /*!< [0..0] Prescaler value update status */
            __IM  uint32_t RVU                 : 1; /*!< [1..1] Reload value update status */
            __IM  uint32_t WVU                 : 1; /*!< [2..2] Window value update status */
            __IM  uint32_t WTU                 : 1; /*!< [3..3] Wakeup timer value update status */
            __IM  uint32_t RLF                 : 1; /*!< [4..4] Reload flag: 0=done, 1=ongoing */
            __IM  uint32_t          : 27;
        } SR_f;
    };
    union {
        __IOM uint32_t WINR                ; /*!< Offset: 0x10 (read-write) Window register */
        struct {
            __IOM uint32_t WIN                 : 12; /*!< [11..0] Window threshold value */
            __IM  uint32_t          : 20;
        } WINR_f;
    };
    union {
        __IOM uint32_t WUTR                ; /*!< Offset: 0x14 (read-write) Wakeup threshold register */
        struct {
            __IOM uint32_t WUT                 : 12; /*!< [11..0] Wakeup threshold value */
            __IM  uint32_t          : 20;
        } WUTR_f;
    };
} IWDT_TypeDef;

/**
 * @brief I2S - I2S Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t TXDR                ; /*!< Offset: 0x00 (read-write) Transmit data register - I2S data to transmit */
        struct {
            __IOM uint32_t TXDR                : 32; /*!< [31..0] Transmit data register - I2C data to transmit */
        } TXDR_f;
    };
    union {
        __IOM uint32_t RXDR                ; /*!< Offset: 0x04 (read-write) Receive data register - I2S received data */
        struct {
            __IOM uint32_t RXDR                : 32; /*!< [31..0] Receive data register - I2C received data */
        } RXDR_f;
    };
    union {
        __IOM uint32_t CR                  ; /*!< Offset: 0x08 (read-write) Control register - I2S configuration and control */
        struct {
            __IOM uint32_t CHLEN               : 1; /*!< [0..0] Channel length - I2S audio channel length (16/32-bit) */
            __IOM uint32_t DLEN                : 2; /*!< [2..1] Data length - Data transfer length in bytes */
            __IOM uint32_t CKPL                : 1; /*!< [3..3] Clock polarity - Clock signal polarity */
            __IOM uint32_t STD                 : 2; /*!< [5..4] I2S standard selection - 00=I2S Philips, 01=MSB aligned, 10=LSB aligned, 11=PCM */
            __IOM uint32_t RSV                 : 1; /*!< [6..6] Reserved */
            __IOM uint32_t PCMMODE             : 1; /*!< [7..7] PCM mode - PCM audio mode selection */
            __IOM uint32_t MODE                : 1; /*!< [8..8] Mode - Operating mode select */
            __IOM uint32_t TEN                 : 1; /*!< [9..9] TEN - TEN value */
            __IOM uint32_t REN                 : 1; /*!< [10..10] REN - REN value */
            __IOM uint32_t EN                  : 1; /*!< [11..11] Enable - Enables the module or function */
            __IOM uint32_t IOSWP               : 1; /*!< [12..12] I/O swap - Swaps I/O pin functions */
            __IOM uint32_t START               : 1; /*!< [13..13] Start transfer - Set by software, cleared by hardware */
            __IOM uint32_t STOP                : 1; /*!< [14..14] Stop condition - Generates stop condition */
            __IOM uint32_t RXDMAEN             : 1; /*!< [15..15] Receive DMA enable - Enables receive DMA channel */
            __IOM uint32_t TXDMAEN             : 1; /*!< [16..16] TX DMA enable - Enables transmit DMA channel */
            __IM  uint32_t          : 15;
        } CR_f;
    };
    union {
        __IOM uint32_t PR                  ; /*!< Offset: 0x0C (read-write) Prescaler register - I2S clock prescaler */
        struct {
            __IOM uint32_t DIV                 : 9; /*!< [8..0] Divider - Clock divider factor */
            __IOM uint32_t OF                  : 1; /*!< [9..9] Overflow - Overflow flag */
            __IOM uint32_t MCKOE               : 1; /*!< [10..10] Master clock output enable - Enables I2S master clock output */
            __IM  uint32_t          : 21;
        } PR_f;
    };
    union {
        __IOM uint32_t IER                 ; /*!< Offset: 0x10 (read-write) Interrupt enable register - Enables I2S interrupts */
        struct {
            __IOM uint32_t RXNEIE              : 1; /*!< [0..0] Receive data register not empty interrupt enable - Enables RXNE interrupt */
            __IOM uint32_t TXEIE               : 1; /*!< [1..1] Transmit data register empty interrupt enable - Enables TXE interrupt */
            __IM  uint32_t          : 3;
            __IOM uint32_t ERRIE               : 1; /*!< [5..5] Error interrupt enable - Error interrupt enable */
            __IM  uint32_t          : 3;
            __IOM uint32_t MSUSPIE             : 1; /*!< [9..9] MS suspend interrupt enable - Enables suspend interrupt */
            __IOM uint32_t SVTCIE              : 1; /*!< [10..10] SVTCIE - SVTCIE value */
            __IM  uint32_t          : 21;
        } IER_f;
    };
    union {
        __IOM uint32_t SR                  ; /*!< Offset: 0x14 (read-write) Status register - I2S status flags */
        struct {
            __IOM uint32_t RXNE                : 1; /*!< [0..0] Receive buffer not empty - Indicates RX buffer contains data */
            __IOM uint32_t TXE                 : 1; /*!< [1..1] Transmit buffer empty - Indicates TX buffer is empty */
            __IOM uint32_t CH                  : 1; /*!< [2..2] Channel - Channel selection field */
            __IOM uint32_t UDR                 : 1; /*!< [3..3] Underrun flag - Indicates transmit underrun error */
            __IOM uint32_t OVR                 : 1; /*!< [4..4] Overrun flag - Indicates receive overrun error */
            __IOM uint32_t FE                  : 1; /*!< [5..5] Framing error - Indicates framing error detected */
            __IM  uint32_t          : 3;
            __IOM uint32_t MSUSP               : 1; /*!< [9..9] MS suspend - Millisecond suspend control */
            __IOM uint32_t SVTC                : 1; /*!< [10..10] SVTC - SVTC value */
            __IM  uint32_t          : 21;
        } SR_f;
    };
    __IO uint32_t RESERVED0;
    union {
        __IOM uint32_t RSIZE               ; /*!< Offset: 0x1C (read-write) RSIZE - RSIZE value */
        struct {
            __IOM uint32_t RSIZE               : 16; /*!< [15..0] RSIZE - RSIZE value */
            __IM  uint32_t          : 16;
        } RSIZE_f;
    };
} I2S_TypeDef;

/**
 * @brief UART - UART Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t DR                  ; /*!< Offset: 0x00 (read-write) Data register - CRC calculation data input and result */
        struct {
            __IOM uint32_t DATA                : 9; /*!< [8..0] Data value - Data to be transferred or received data value */
            __IOM uint32_t FE                  : 1; /*!< [9..9] Framing error - Indicates framing error detected */
            __IOM uint32_t PE                  : 1; /*!< [10..10] Parity error - Indicates parity error detected */
            __IOM uint32_t BE                  : 1; /*!< [11..11] Break error - Break error flag */
            __IOM uint32_t OE                  : 1; /*!< [12..12] Overrun error - Overrun error flag */
            __IM  uint32_t          : 19;
        } DR_f;
    };
    union {
        __IM  uint32_t FR                  ; /*!< Offset: 0x04 (read-only) Flag register - UART flag register */
        struct {
            __IM  uint32_t FE                  : 1; /*!< [0..0] Framing error - Indicates framing error detected */
            __IM  uint32_t PE                  : 1; /*!< [1..1] Parity error - Indicates parity error detected */
            __IM  uint32_t BE                  : 1; /*!< [2..2] Break error - Break error flag */
            __IM  uint32_t OE                  : 1; /*!< [3..3] Overrun error - Overrun error flag */
            __IM  uint32_t RXFE                : 1; /*!< [4..4] Receive FIFO empty - Receive FIFO empty flag */
            __IM  uint32_t TXFF                : 1; /*!< [5..5] TX FIFO full - Transmit FIFO full flag */
            __IM  uint32_t RXFF                : 1; /*!< [6..6] Receive FIFO full - Receive FIFO full flag */
            __IM  uint32_t TXFE                : 1; /*!< [7..7] TX FIFO empty - Transmit FIFO empty flag */
            __IM  uint32_t CTS                 : 1; /*!< [8..8] Clear to send - CTS flow control signal */
            __IM  uint32_t BUSY                : 1; /*!< [9..9] Busy flag - UART transmit busy flag */
            __IM  uint32_t          : 22;
        } FR_f;
    };
    union {
        __IOM uint32_t BRR                 ; /*!< Offset: 0x08 (read-write) Baud rate register - UART baud rate divider value */
        struct {
            __IOM uint32_t FBAUD               : 6; /*!< [5..0] Fractional baud rate - Fractional part of baud rate divider */
            __IOM uint32_t IBAUD               : 16; /*!< [21..6] Integer baud rate - Integer part of baud rate divider */
            __IM  uint32_t          : 10;
        } BRR_f;
    };
    union {
        __IOM uint32_t IE                  ; /*!< Offset: 0x0C (read-write) Interrupt enable register - Enables or disables various interrupt sources */
        struct {
            __IM  uint32_t          : 4;
            __IOM uint32_t RXI                 : 1; /*!< [4..4] Receive interrupt - Receive interrupt flag */
            __IOM uint32_t TXI                 : 1; /*!< [5..5] TX interrupt - Transmit interrupt flag */
            __IM  uint32_t          : 1;
            __IOM uint32_t FEI                 : 1; /*!< [7..7] Framing error interrupt - Framing error interrupt flag */
            __IOM uint32_t PEI                 : 1; /*!< [8..8] PEI - PE interrupt */
            __IOM uint32_t BEI                 : 1; /*!< [9..9] Bus error interrupt - Bus error interrupt flag */
            __IOM uint32_t OEI                 : 1; /*!< [10..10] OEI - OE interrupt */
            __IOM uint32_t LBDI                : 1; /*!< [11..11] LIN break detection interrupt enable - LIN break detection interrupt enable */
            __IOM uint32_t BCNTI               : 1; /*!< [12..12] Bit count timeout interrupt enable - Bit count timeout interrupt enable */
            __IOM uint32_t IDLEI               : 1; /*!< [13..13] Idle interrupt - Idle detection interrupt flag */
            __IOM uint32_t ABRI                : 1; /*!< [14..14] Auto-baud rate interrupt - Auto-baud rate detection interrupt flag */
            __IOM uint32_t TCI                 : 1; /*!< [15..15] Transmit complete interrupt enable - Transmit complete interrupt enable */
            __IM  uint32_t          : 16;
        } IE_f;
    };
    union {
        __IOM uint32_t ISR                 ; /*!< Offset: 0x10 (read-write) Interrupt status register - UART interrupt status flags */
        struct {
            __IM  uint32_t          : 4;
            __IOM uint32_t RXI                 : 1; /*!< [4..4] Receive interrupt - Receive interrupt flag */
            __IOM uint32_t TXI                 : 1; /*!< [5..5] TX interrupt - Transmit interrupt flag */
            __IM  uint32_t          : 1;
            __IOM uint32_t FEI                 : 1; /*!< [7..7] Framing error interrupt - Framing error interrupt flag */
            __IOM uint32_t PEI                 : 1; /*!< [8..8] PEI - PE interrupt */
            __IOM uint32_t BEI                 : 1; /*!< [9..9] Bus error interrupt - Bus error interrupt flag */
            __IOM uint32_t OEI                 : 1; /*!< [10..10] OEI - OE interrupt */
            __IOM uint32_t LBDI                : 1; /*!< [11..11] LIN break detection interrupt - LIN break detection interrupt flag */
            __IOM uint32_t BCNTI               : 1; /*!< [12..12] Bit count timeout interrupt - Bit count timeout interrupt flag */
            __IOM uint32_t IDLEI               : 1; /*!< [13..13] Idle interrupt - Idle detection interrupt flag */
            __IOM uint32_t ABRI                : 1; /*!< [14..14] Auto-baud rate interrupt - Auto-baud rate detection interrupt flag */
            __IOM uint32_t TCI                 : 1; /*!< [15..15] Transmit complete interrupt - Transmit complete interrupt flag */
            __IM  uint32_t          : 16;
        } ISR_f;
    };
    union {
        __IOM uint32_t CR1                 ; /*!< Offset: 0x14 (read-write) Control register 1 - UART control and configuration */
        struct {
            __IOM uint32_t UARTEN              : 1; /*!< [0..0] UART enable - Enables UART peripheral */
            __IOM uint32_t SIREN               : 1; /*!< [1..1] SIR enable - SIR protocol enable */
            __IOM uint32_t SIRLP               : 1; /*!< [2..2] SIR low power - SIR low power mode */
            __IOM uint32_t RXDMAE              : 1; /*!< [3..3] Receive DMA enable - Enables receive DMA */
            __IOM uint32_t TXDMAE              : 1; /*!< [4..4] TX DMA enable - Enables transmit DMA */
            __IOM uint32_t DMAONERR            : 1; /*!< [5..5] DMA on error - DMA behavior on error */
            __IM  uint32_t          : 2;
            __IOM uint32_t TXE                 : 1; /*!< [8..8] Transmit enable - Enables transmitter */
            __IOM uint32_t RXE                 : 1; /*!< [9..9] Receive enable - Enables receiver */
            __IM  uint32_t          : 1;
            __IOM uint32_t RTS                 : 1; /*!< [11..11] RTS output control - RTS output pin state */
            __IM  uint32_t          : 2;
            __IOM uint32_t RTSEN               : 1; /*!< [14..14] RTS flow control enable - Enables RTS flow control */
            __IOM uint32_t CTSEN               : 1; /*!< [15..15] CTS enable - Enables CTS flow control */
            __IOM uint32_t ENDIAN_SEL          : 1; /*!< [16..16] Endianness selection - Big/little endian select */
            __IM  uint32_t          : 15;
        } CR1_f;
    };
    union {
        __IOM uint32_t CR2                 ; /*!< Offset: 0x18 (read-write) Control register 2 - UART additional control */
        struct {
            __IOM uint32_t HDSEL               : 1; /*!< [0..0] Half-duplex selection - Selects half-duplex mode */
            __IM  uint32_t          : 1;
            __IOM uint32_t RWU                 : 1; /*!< [2..2] Receiver wakeup - Enables receiver wakeup from mute mode */
            __IOM uint32_t WAKE                : 1; /*!< [3..3] Wakeup method - Selects UART wakeup method */
            __IOM uint32_t ABREN               : 1; /*!< [4..4] Auto-baud rate enable - Enables automatic baud rate detection */
            __IOM uint32_t SCEN                : 1; /*!< [5..5] Smartcard mode enable - Enables smartcard mode */
            __IOM uint32_t NACK                : 1; /*!< [6..6] Not acknowledge - Sends NACK after data reception */
            __IOM uint32_t CLKEN               : 1; /*!< [7..7] Clock enable - Peripheral clock enable */
            __IOM uint32_t LBCL                : 1; /*!< [8..8] Last bit clock pulse - Controls last data bit clock pulse output */
            __IOM uint32_t CPHA                : 1; /*!< [9..9] Clock phase - Selects the clock phase for data sampling */
            __IOM uint32_t CPOL                : 1; /*!< [10..10] Clock polarity - Selects the idle state clock level */
            __IOM uint32_t ADDR                : 7; /*!< [17..11] Address - UART node address for multiprocessor communication */
            __IOM uint32_t ADDM7               : 1; /*!< [18..18] 7-bit address mode - Selects 7-bit address mode */
            __IM  uint32_t          : 13;
        } CR2_f;
    };
    union {
        __IOM uint32_t CR3                 ; /*!< Offset: 0x1C (read-write) Control register 3 - UART extended control */
        struct {
            __IOM uint32_t BRK                 : 1; /*!< [0..0] Break send enable - Enables break signal transmission */
            __IOM uint32_t PEN                 : 1; /*!< [1..1] Parity enable - Enables parity check */
            __IOM uint32_t EPS                 : 1; /*!< [2..2] Endpoint status - USB endpoint status */
            __IOM uint32_t STP2                : 1; /*!< [3..3] STOP 2 mode - Stop 2 mode control */
            __IM  uint32_t          : 1;
            __IOM uint32_t FEN                 : 1; /*!< [5..5] FIFO enable - Enables FIFO mode */
            __IOM uint32_t WLEN                : 3; /*!< [8..6] Word length - Selects data word length (5-9 bits) */
            __IOM uint32_t SPS                 : 1; /*!< [9..9] Parity mode selection - Selects parity or 0/1 check mode */
            __IOM uint32_t TXIFLSEL            : 3; /*!< [12..10] TX interrupt FIFO level select - TX interrupt FIFO threshold */
            __IOM uint32_t RXIFLSEL            : 3; /*!< [15..13] Receive interrupt FIFO level select - RX interrupt FIFO threshold level */
            __IM  uint32_t          : 16;
        } CR3_f;
    };
    union {
        __IOM uint32_t GTPR                ; /*!< Offset: 0x20 (read-write) Guard time and prescaler register - UART guard time and prescaler */
        struct {
            __IOM uint32_t PSC                 : 8; /*!< [7..0] Prescaler - IrDA/Smartcard clock prescaler */
            __IOM uint32_t GT                  : 4; /*!< [11..8] Guard time - Smartcard guard time in ETU units */
            __IM  uint32_t          : 20;
        } GTPR_f;
    };
    union {
        __IOM uint32_t BCNT                ; /*!< Offset: 0x24 (read-write) Bit count register - UART bit count and RS485 DE control */
        struct {
            __IOM uint32_t BCNT_VALUE          : 24; /*!< [23..0] Bit count value - Bit count timer value / DEAT[7:4] DEDT[3:0] */
            __IOM uint32_t BCNT_START          : 1; /*!< [24..24] Bit count start - Starts the bit count timer */
            __IOM uint32_t AUTO_START_EN       : 1; /*!< [25..25] Auto start enable - Enables hardware auto bit count on stop bit */
            __IOM uint32_t DEP                 : 1; /*!< [26..26] DE polarity - Selects DE signal polarity */
            __IOM uint32_t DEM                 : 1; /*!< [27..27] DE mode enable - Enables RS485 DE function */
            __IOM uint32_t LBD_CNT             : 4; /*!< [31..28] LIN break detection count - LIN break bit count threshold */
        } BCNT_f;
    };
} UART_TypeDef;

/**
 * @brief I2C - I2C Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t SLAVE_ADDR1         ; /*!< Offset: 0x00 (read-write) Slave address 1 - I2C slave address 1 */
        struct {
            __IM  uint32_t          : 1;
            __IOM uint32_t ADDR1               : 7; /*!< [7..1] Address 1 - First address value */
            __IM  uint32_t          : 24;
        } SLAVE_ADDR1_f;
    };
    union {
        __IOM uint32_t CLK_DIV             ; /*!< Offset: 0x04 (read-write) Clock divider - Clock division factor */
        struct {
            __IOM uint32_t CLK_DIV             : 12; /*!< [11..0] Clock divider - Clock division factor */
            __IOM uint32_t DLY_TIME            : 12; /*!< [23..12] Delay time - Delay time configuration value */
            __IM  uint32_t          : 8;
        } CLK_DIV_f;
    };
    union {
        __IOM uint32_t CR                  ; /*!< Offset: 0x08 (read-write) Control register - I2C control configuration */
        struct {
            __IOM uint32_t MEN                 : 1; /*!< [0..0] Module enable - Enables module */
            __IOM uint32_t NOSTRETCH           : 1; /*!< [1..1] No stretch - Disables clock stretching */
            __IOM uint32_t MASTER              : 1; /*!< [2..2] Master - Master mode enable */
            __IOM uint32_t TX                  : 1; /*!< [3..3] TX - TX value */
            __IOM uint32_t START               : 1; /*!< [4..4] Start condition - Generates start condition */
            __IOM uint32_t STOP                : 1; /*!< [5..5] Stop condition - Generates stop condition */
            __IOM uint32_t TACK                : 1; /*!< [6..6] TACK - TACK value */
            __IOM uint32_t MTF_INT_EN          : 1; /*!< [7..7] MTF interrupt enable - Enables MTF interrupt */
            __IOM uint32_t RX_ADDR1_INT_EN     : 1; /*!< [8..8] Receive address 1 interrupt enable - Enables address 1 match interrupt */
            __IOM uint32_t TXE_INT_EN          : 1; /*!< [9..9] TX empty interrupt enable - Enables TX empty interrupt */
            __IOM uint32_t RXNE_INT_EN         : 1; /*!< [10..10] Receive not empty interrupt enable - Enables RX not empty interrupt */
            __IOM uint32_t OVR_INT_EN          : 1; /*!< [11..11] Overrun interrupt enable - Enables overrun interrupt */
            __IOM uint32_t RX_ADDR2_INT_EN     : 1; /*!< [12..12] Receive address 2 interrupt enable - Enables address 2 match interrupt */
            __IOM uint32_t DETR_INT_EN         : 1; /*!< [13..13] Detection result interrupt enable - Enables detection result interrupt */
            __IOM uint32_t RSV14               : 1; /*!< [14..14] Reserved - Reserved bit 14 */
            __IOM uint32_t TX_AUTO_EN          : 1; /*!< [15..15] TX auto enable - Enables automatic transmit mode */
            __IOM uint32_t MARLO_INT_EN        : 1; /*!< [16..16] MARLO interrupt enable - Enables MARLO interrupt */
            __IOM uint32_t TXE_SEL             : 1; /*!< [17..17] TX empty selection - TX empty source selection */
            __IOM uint32_t DMA_EN              : 1; /*!< [18..18] DMA enable - Enables DMA data transfer */
            __IOM uint32_t RX_ADDR3_INT_EN     : 1; /*!< [19..19] Receive address 3 interrupt enable - Enables address 3 match interrupt */
            __IOM uint32_t STOPF_INT_EN        : 1; /*!< [20..20] Stop flag interrupt enable - Enables stop flag interrupt */
            __IOM uint32_t NACKF_INT_EN        : 1; /*!< [21..21] NACKF interrupt enable - Enables NACK flag interrupt */
            __IOM uint32_t TX_RX_FLAG_INT_EN   : 1; /*!< [22..22] TX/RX flag interrupt enable - Enables TX/RX flag interrupt */
            __IOM uint32_t MARLO_SEL           : 1; /*!< [23..23] MARLO selection - MARLO source selection */
            __IOM uint32_t SU_HD_FIX_EN        : 1; /*!< [24..24] SU HD fix enable - SU HD fix enable */
            __IOM uint32_t STRETCH_FIX_EN      : 1; /*!< [25..25] Stretch fix enable - Stretch fix enable */
            __IOM uint32_t HIGH_WAIT_EN        : 1; /*!< [26..26] High wait enable - Enables high-level wait condition */
            __IOM uint32_t MULTI_MA_EN         : 1; /*!< [27..27] Multi-master enable - Enables multi-master mode */
            __IM  uint32_t          : 4;
        } CR_f;
    };
    union {
        __IM  uint32_t SR                  ; /*!< Offset: 0x0C (read-only) Status register - I2C status flags */
        struct {
            __IM  uint32_t RACK                : 1; /*!< [0..0] RACK - RACK status */
            __IM  uint32_t STARTF              : 1; /*!< [1..1] Start flag - Start condition flag */
            __IM  uint32_t STOPF               : 1; /*!< [2..2] Stop flag - Stop mode flag */
            __IM  uint32_t SRW                 : 1; /*!< [3..3] SRW - SRW value */
            __IM  uint32_t BUS_BUSY            : 1; /*!< [4..4] Bus busy - Indicates bus is busy */
            __IM  uint32_t TX_RX_FLAG          : 1; /*!< [5..5] TX/RX flag - Transmit/receive status flag */
            __IM  uint32_t MARLO               : 1; /*!< [6..6] MARLO - MARLO status */
            __IM  uint32_t MTF                 : 1; /*!< [7..7] M trigger flag - MT status */
            __IM  uint32_t RX_ADDR1            : 1; /*!< [8..8] Receive address 1 - Receive address 1 filter */
            __IM  uint32_t TXE                 : 1; /*!< [9..9] Transmit buffer empty - Indicates TX buffer is empty */
            __IM  uint32_t RXNE                : 1; /*!< [10..10] Receive buffer not empty - Indicates RX buffer contains data */
            __IM  uint32_t OVR                 : 1; /*!< [11..11] Overrun flag - Indicates receive overrun error */
            __IM  uint32_t RX_ADDR2            : 1; /*!< [12..12] Receive address 2 - Receive address 2 filter */
            __IM  uint32_t DETR                : 1; /*!< [13..13] Detection result - Detection result value */
            __IM  uint32_t RX_ADDR3            : 1; /*!< [14..14] Receive address 3 - Receive address 3 filter */
            __IM  uint32_t TIMEOUTAF           : 1; /*!< [15..15] TIMEOUTAF - TIMEOUTAF value */
            __IM  uint32_t TIMEOUTBF           : 1; /*!< [16..16] TIMEOUTBF - TIMEOUTBF value */
            __IM  uint32_t NACKF               : 1; /*!< [17..17] NACK flag - NACK received */
            __IM  uint32_t          : 14;
        } SR_f;
    };
    union {
        __IOM uint32_t DR                  ; /*!< Offset: 0x10 (read-write) Data register - I2C data register */
        struct {
            __IOM uint32_t I2CDR               : 8; /*!< [7..0] I2C data register - I2C data register */
            __IM  uint32_t          : 24;
        } DR_f;
    };
    union {
        __IOM uint32_t SLAVE_ADDR2_3       ; /*!< Offset: 0x14 (read-write) Slave address 2/3 - I2C slave address 2 and 3 */
        struct {
            __IOM uint32_t ADDR2_EN            : 1; /*!< [0..0] Address 2 enable - Enables MAC address 2 filtering */
            __IOM uint32_t ADDR2               : 7; /*!< [7..1] Address 2 - Second address value */
            __IOM uint32_t ADDR3_EN            : 1; /*!< [8..8] Address 3 enable - Enables MAC address 3 filtering */
            __IOM uint32_t ADDR3               : 7; /*!< [15..9] Address 3 - Third address value */
            __IM  uint32_t          : 16;
        } SLAVE_ADDR2_3_f;
    };
    union {
        __IOM uint32_t DET                 ; /*!< Offset: 0x18 (read-write) SDA transition threshold - SDA transition detection threshold */
        struct {
            __IOM uint32_t DETCNT              : 16; /*!< [15..0] Detection count - SDA transition count threshold when SCL is low */
            __IM  uint32_t          : 16;
        } DET_f;
    };
    union {
        __IOM uint32_t FILTER              ; /*!< Offset: 0x1C (read-write) Filter register - SCL/SDA glitch filter configuration */
        struct {
            __IOM uint32_t SCL_FILTER          : 5; /*!< [4..0] SCL filter - SCL glitch filter value (max glitch width = Tfclk*8*SCL_FILTER) */
            __IM  uint32_t          : 3;
            __IOM uint32_t SDA_IN_DELAY        : 5; /*!< [12..8] SDA input delay - SDA input delay setting (set same as SCL_FILTER when filter enabled) */
            __IM  uint32_t          : 19;
        } FILTER_f;
    };
    __IO uint32_t RESERVED0;
    union {
        __IOM uint32_t TIMEOUT             ; /*!< Offset: 0x24 (read-write) Timeout configuration - SMBus timeout settings */
        struct {
            __IOM uint32_t TIMEOUTA            : 12; /*!< [11..0] TimeoutA - SMBus timeout A value (in units of 2048 PCLK cycles) */
            __IM  uint32_t          : 2;
            __IOM uint32_t TOUTA_INT_EN        : 1; /*!< [14..14] TimeoutA interrupt enable - Enables TIMEOUTAF interrupt */
            __IOM uint32_t TIMOUTEN            : 1; /*!< [15..15] Timeout enable - Enables timeout A timer */
            __IOM uint32_t TIMEOUTB            : 12; /*!< [27..16] TimeoutB - SMBus timeout B value (in units of 2048 PCLK cycles) */
            __IOM uint32_t RSV28               : 1; /*!< [28..28] Reserved - Reserved bit 28 */
            __IOM uint32_t EXT_MODE            : 1; /*!< [29..29] EXT mode - TimeoutB mode selection (0: SEXT mode, 1: MEXT mode) */
            __IOM uint32_t TOUTB_INT_EN        : 1; /*!< [30..30] TimeoutB interrupt enable - Enables TIMEOUTBF interrupt */
            __IOM uint32_t EXTEN               : 1; /*!< [31..31] EXT enable - Enables timeout B timer */
        } TIMEOUT_f;
    };
} I2C_TypeDef;

/**
 * @brief FDCAN - FDCAN Register Structure
 */
typedef struct {
    __IO uint32_t RESERVED0[38];
    union {
        __IM  uint32_t TTSL                ; /*!< Offset: 0x98 (read-only) TTSL - TTSL value */
        struct {
            __IM  uint32_t TTS                 : 32; /*!< [31..0] TTS - TTS value */
        } TTSL_f;
    };
    union {
        __IM  uint32_t TTSH                ; /*!< Offset: 0x9C (read-only) TTSH - TTSH value */
        struct {
            __IM  uint32_t TTS                 : 32; /*!< [31..0] TTS - TTS value */
        } TTSH_f;
    };
    union {
        __IOM uint32_t CR                  ; /*!< Offset: 0xA0 (read-write) Control register - CRC calculation control */
        struct {
            __IOM uint32_t BUSOFF              : 1; /*!< [0..0] Bus off - CAN bus-off status flag */
            __IOM uint32_t TACTIVE             : 1; /*!< [1..1] TACTIVE - TACTIVE value */
            __IOM uint32_t RACTIVE             : 1; /*!< [2..2] RACTIVE - RACTIVE status */
            __IOM uint32_t TSSS                : 1; /*!< [3..3] TSSS - TSSS value */
            __IOM uint32_t TPSS                : 1; /*!< [4..4] TPSS - TPSS value */
            __IOM uint32_t LBMI                : 1; /*!< [5..5] Low battery mode indicator - Low battery mode indicator flag */
            __IOM uint32_t LBME                : 1; /*!< [6..6] Low battery mode enable - Enables low battery mode */
            __IOM uint32_t RESET               : 1; /*!< [7..7] Reset - Reset control */
            __IOM uint32_t TSA                 : 1; /*!< [8..8] TSA - TSA value */
            __IOM uint32_t TSALL               : 1; /*!< [9..9] TSALL - TSALL value */
            __IOM uint32_t TSONE               : 1; /*!< [10..10] TSONE - TSONE value */
            __IOM uint32_t TPA                 : 1; /*!< [11..11] TPA - TPA value */
            __IOM uint32_t TPE                 : 1; /*!< [12..12] TPE - TPE value */
            __IOM uint32_t STBY                : 1; /*!< [13..13] Standby mode - Enters standby mode */
            __IOM uint32_t LOM                 : 1; /*!< [14..14] Low output mode - Low output mode configuration */
            __IOM uint32_t TBSEL               : 1; /*!< [15..15] TBSEL - TBSEL value */
            __IOM uint32_t TSSTAT              : 2; /*!< [17..16] TSSTAT - TSSTAT value */
            __IM  uint32_t          : 2;
            __IOM uint32_t TTTBM               : 1; /*!< [20..20] TTTBM - TTTBM value */
            __IOM uint32_t TSMODE              : 1; /*!< [21..21] TSMODE - TSMODE value */
            __IOM uint32_t TSNEXT              : 1; /*!< [22..22] TSNEXT - TSNEXT value */
            __IOM uint32_t FD_ISO              : 1; /*!< [23..23] FD ISO - FDCAN FD ISO mode selection */
            __IOM uint32_t RSTAT               : 2; /*!< [25..24] RSTAT - RSTAT value */
            __IM  uint32_t          : 1;
            __IOM uint32_t RBALL               : 1; /*!< [27..27] RBALL - Receive buffer all */
            __IOM uint32_t RREL                : 1; /*!< [28..28] RREL - RREL value */
            __IOM uint32_t ROV                 : 1; /*!< [29..29] ROV - ROV value */
            __IOM uint32_t ROM                 : 1; /*!< [30..30] ROM - ROM value */
            __IOM uint32_t SACK                : 1; /*!< [31..31] SACK - SACK status */
        } CR_f;
    };
    union {
        __IOM uint32_t IR                  ; /*!< Offset: 0xA4 (read-write) Input register - Input data register */
        struct {
            __IOM uint32_t TSFF                : 1; /*!< [0..0] TSFF - TSFF value */
            __IOM uint32_t EIE                 : 1; /*!< [1..1] Error interrupt enable - Enables error status change interrupt */
            __IOM uint32_t TSIE                : 1; /*!< [2..2] Timestamp interrupt enable - Enables RTC timestamp interrupt */
            __IOM uint32_t TPIE                : 1; /*!< [3..3] TPIE - TPIE value */
            __IOM uint32_t RAFIE               : 1; /*!< [4..4] RAF interrupt enable - Enables RAF interrupt */
            __IOM uint32_t RFIE                : 1; /*!< [5..5] Receive FIFO interrupt enable - Enables RX FIFO interrupt */
            __IOM uint32_t ROIE                : 1; /*!< [6..6] ROIE - ROIE value */
            __IOM uint32_t RIE                 : 1; /*!< [7..7] Receive interrupt enable - Enables message reception interrupt */
            __IOM uint32_t AIF                 : 1; /*!< [8..8] Address increment flag - Indicates address increment mode */
            __IOM uint32_t EIF                 : 1; /*!< [9..9] Error interrupt flag - Error interrupt status */
            __IOM uint32_t TSIF                : 1; /*!< [10..10] TSIF - TSIF value */
            __IOM uint32_t TPIF                : 1; /*!< [11..11] TPIF - TPIF value */
            __IOM uint32_t RAFIF               : 1; /*!< [12..12] RAF interrupt flag - RAF interrupt flag */
            __IOM uint32_t RFIF                : 1; /*!< [13..13] Receive FIFO interrupt flag - RX FIFO interrupt flag */
            __IOM uint32_t ROIF                : 1; /*!< [14..14] ROIF - ROIF value */
            __IOM uint32_t RIF                 : 1; /*!< [15..15] RIF - RIF value */
            __IOM uint32_t BEIF                : 1; /*!< [16..16] Bus error interrupt flag - Bus error interrupt status */
            __IOM uint32_t BEIE                : 1; /*!< [17..17] Bus error interrupt enable - Enables bus error interrupt */
            __IOM uint32_t ALIF                : 1; /*!< [18..18] Alarm interrupt flag - Set on alarm event */
            __IOM uint32_t ALIE                : 1; /*!< [19..19] Alarm interrupt enable - Enables alarm interrupt */
            __IOM uint32_t EPIF                : 1; /*!< [20..20] Endpoint interrupt flag - USB EP interrupt flag */
            __IOM uint32_t EPIE                : 1; /*!< [21..21] Endpoint interrupt enable - USB EP interrupt enable */
            __IOM uint32_t EPASS               : 1; /*!< [22..22] Endpoint pass - USB endpoint pass flag */
            __IOM uint32_t EWARN               : 1; /*!< [23..23] Early warning - Early warning flag */
            __IM  uint32_t          : 8;
        } IR_f;
    };
    union {
        __IOM uint32_t LIMIT               ; /*!< Offset: 0xA8 (read-write) Limit - Limit value configuration */
        struct {
            __IOM uint32_t EWL                 : 4; /*!< [3..0] Early warning level - Early warning threshold level */
            __IOM uint32_t AFWL                : 4; /*!< [7..4] Active FIFO watermark level - FIFO watermark level for active state */
            __IM  uint32_t          : 24;
        } LIMIT_f;
    };
    union {
        __IOM uint32_t SBTR                ; /*!< Offset: 0xAC (read-write) SBTR - SBTR value */
        struct {
            __IOM uint32_t S_SEG_1             : 8; /*!< [7..0] Sample segment 1 - CAN sample segment 1 value */
            __IOM uint32_t S_SEG_2             : 7; /*!< [14..8] Sample segment 2 - CAN sample segment 2 value */
            __IM  uint32_t          : 1;
            __IOM uint32_t S_SJW               : 7; /*!< [22..16] Sample SJW - CAN synchronization jump width */
            __IM  uint32_t          : 1;
            __IOM uint32_t S_PRESC             : 8; /*!< [31..24] Sample prescaler - CAN sample point prescaler */
        } SBTR_f;
    };
    union {
        __IOM uint32_t FBTR                ; /*!< Offset: 0xB0 (read-write) Floating baud rate trigger - Floating baud rate trigger */
        struct {
            __IOM uint32_t F_SEG_1             : 5; /*!< [4..0] FDCAN segment 1 - FDCAN time segment 1 value */
            __IM  uint32_t          : 3;
            __IOM uint32_t F_SEG_2             : 4; /*!< [11..8] FDCAN segment 2 - FDCAN time segment 2 value */
            __IM  uint32_t          : 4;
            __IOM uint32_t F_SJW               : 4; /*!< [19..16] FDCAN SJW - FDCAN synchronization jump width */
            __IM  uint32_t          : 4;
            __IOM uint32_t F_PRESC             : 8; /*!< [31..24] FDCAN prescaler - FDCAN time segment prescaler */
        } FBTR_f;
    };
    union {
        __IOM uint32_t TDC                 ; /*!< Offset: 0xB4 (read-write) TDC - TDC value */
        struct {
            __IOM uint32_t SSPOFF              : 7; /*!< [6..0] SSP offset - SSP offset configuration */
            __IOM uint32_t TDCEN               : 1; /*!< [7..7] TDCEN - TDCEN value */
            __IM  uint32_t          : 24;
        } TDC_f;
    };
    union {
        __IM  uint32_t ECC                 ; /*!< Offset: 0xB8 (read-only) ECC - Error correction code status */
        struct {
            __IM  uint32_t ALC                 : 5; /*!< [4..0] Alignment control - Data alignment configuration */
            __IM  uint32_t KOER                : 3; /*!< [7..5] K error - K-line error flag */
            __IM  uint32_t          : 8;
            __IM  uint32_t RECNT               : 8; /*!< [23..16] RECNT - RECNT value */
            __IM  uint32_t TECNT               : 8; /*!< [31..24] TECNT - TECNT value */
        } ECC_f;
    };
    union {
        __IOM uint32_t ACFCR               ; /*!< Offset: 0xBC (read-write) Address filter control - MAC address filter control register */
        struct {
            __IOM uint32_t ACFADR              : 4; /*!< [3..0] Address filter address - MAC address for filtering configuration */
            __IM  uint32_t          : 1;
            __IOM uint32_t SELMASK             : 1; /*!< [5..5] Select mask - Selection mask value */
            __IM  uint32_t          : 10;
            __IOM uint32_t AE                  : 16; /*!< [31..16] Address error - Indicates address alignment error */
        } ACFCR_f;
    };
    union {
        __IOM uint32_t ACFMODE             ; /*!< Offset: 0xC0 (read-write) Address filter mode - MAC address filtering mode selection */
        struct {
            __IOM uint32_t ACF_NOMASK          : 16; /*!< [15..0] Address filter no mask - Address comparison without mask */
            __IOM uint32_t ACF_W16             : 16; /*!< [31..16] Address filter width 16 - 16-bit address filter mode */
        } ACFMODE_f;
    };
    union {
        __IOM uint32_t ACODR               ; /*!< Offset: 0xC4 (read-write) Address code register - MAC address filter code value */
        struct {
            __IOM uint32_t ACODE_AMASK         : 32; /*!< [31..0] Address code/address mask - Address filter code and mask configuration */
        } ACODR_f;
    };
    union {
        __IOM uint32_t TIMCFG              ; /*!< Offset: 0xC8 (read-write) TIMCFG - TIMCFG value */
        struct {
            __IOM uint32_t TIMEEN              : 1; /*!< [0..0] TIMEEN - TIMEEN value */
            __IOM uint32_t TIMEPOS             : 1; /*!< [1..1] TIMEPOS - TIMEPOS value */
            __IM  uint32_t          : 30;
        } TIMCFG_f;
    };
    union {
        __IOM uint32_t TTCFG               ; /*!< Offset: 0xCC (read-write) TTCFG - TTCFG value */
        struct {
            __IOM uint32_t TBPTR               : 6; /*!< [5..0] TBPTR - TBPTR value */
            __IOM uint32_t TBF                 : 1; /*!< [6..6] TBF - TBF value */
            __IOM uint32_t TBE                 : 1; /*!< [7..7] Trigger break enable - Enables break on trigger event */
            __IOM uint32_t TTEN                : 1; /*!< [8..8] TTEN - TTEN value */
            __IOM uint32_t T_PRESC             : 2; /*!< [10..9] Time prescaler - CAN time segment prescaler */
            __IOM uint32_t TTIF                : 1; /*!< [11..11] TTIF - TTIF value */
            __IOM uint32_t TTIE                : 1; /*!< [12..12] TTIE - TTIE value */
            __IOM uint32_t TEIF                : 1; /*!< [13..13] Transfer error flag - Set on transfer error */
            __IOM uint32_t WTIF                : 1; /*!< [14..14] WTIF - WTIF value */
            __IOM uint32_t WTIE                : 1; /*!< [15..15] WTIE - WTIE value */
            __IOM uint32_t RFMSGIF             : 1; /*!< [16..16] Receive message interrupt flag - RX message interrupt flag */
            __IOM uint32_t RFMSGIE             : 1; /*!< [17..17] Receive message interrupt enable - Enables RX message interrupt */
            __IM  uint32_t          : 14;
        } TTCFG_f;
    };
    union {
        __IOM uint32_t REFMSG              ; /*!< Offset: 0xD0 (read-write) Reference message - Reference message value */
        struct {
            __IOM uint32_t REF_ID              : 29; /*!< [28..0] Reference ID - Reference identifier */
            __IM  uint32_t          : 2;
            __IOM uint32_t REF_IDE             : 1; /*!< [31..31] Reference IDE - Reference identifier extension */
        } REFMSG_f;
    };
    union {
        __IOM uint32_t TRIGCFG             ; /*!< Offset: 0xD4 (read-write) Trigger configuration - Trigger configuration register */
        struct {
            __IOM uint32_t TTPTR               : 6; /*!< [5..0] TTPTR - TTPTR value */
            __IM  uint32_t          : 2;
            __IOM uint32_t TTYPE               : 3; /*!< [10..8] TTYPE - TTYPE value */
            __IM  uint32_t          : 1;
            __IOM uint32_t TEW                 : 4; /*!< [15..12] TEW - TEW value */
            __IM  uint32_t          : 16;
        } TRIGCFG_f;
    };
    union {
        __IOM uint32_t TTTRIG              ; /*!< Offset: 0xD8 (read-write) TTTRIG - TTTRIG value */
        struct {
            __IOM uint32_t TT_TRIG             : 16; /*!< [15..0] TT_TRIG - TT_TRIG value */
            __IM  uint32_t          : 16;
        } TTTRIG_f;
    };
    union {
        __IOM uint32_t WTRIG               ; /*!< Offset: 0xDC (read-write) WTRIG - WTRIG value */
        struct {
            __IOM uint32_t TT_WTRIG            : 16; /*!< [15..0] TT_WTRIG - TT_WTRIG value */
            __IM  uint32_t          : 16;
        } WTRIG_f;
    };
} FDCAN_TypeDef;

/**
 * @brief PMU - PMU Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t CTRL0               ; /*!< Offset: 0x00 (read-write) PMU control register 0 - Low-power mode selection, regulator control, RCH control, STOP wakeup time */
        struct {
            __IOM uint32_t LPMS                : 1; /*!< [0..0] Low-power mode selection (when core enters DeepSleep): 0=STOP mode, 1=STANDBY mode */
            __IM  uint32_t          : 3;
            __IOM uint32_t RCHPDEN             : 1; /*!< [4..4] STOP mode RCH auto-off: 0=not auto close, 1=auto close RCH in STOP (PLL/RCL/XTH/XTL not auto closed) */
            __IOM uint32_t RCH_DIV_EN          : 1; /*!< [5..5] STOP mode RCH divide-16 output select: 0=original undivided, 1=divide by 16 */
            __IM  uint32_t          : 1;
            __IOM uint32_t RTC_WE              : 1; /*!< [7..7] RTC module write enable: 0=write disabled, 1=write enabled */
            __IOM uint32_t MLDO12_LV           : 2; /*!< [9..8] STOP mode MLDO12 voltage adjust: 00=keep 1.2V, 01=adjust to 1.0V, 10=adjust to 0.9V, 11=adjust to 0.8V */
            __IOM uint32_t MLDO12_LOWP         : 1; /*!< [10..10] STOP mode MLDO low-power mode: 0=no self power reduction, 1=reduce MLDO12 self power */
            __IM  uint32_t          : 1;
            __IOM uint32_t LPLDO12_LV          : 3; /*!< [14..12] STANDBY mode LPLDO12 adjust select: 100=adjust to 1.1V (default), others=reserved */
            __IM  uint32_t          : 1;
            __IOM uint32_t STOP_WPT            : 12; /*!< [27..16] STOP mode wakeup time - Clock cycles needed after STOP wakeup (RC64M or RC64M/16 count) */
            __IM  uint32_t          : 2;
            __IOM uint32_t BKPRAMSEN           : 1; /*!< [30..30] BKP SRAM sleep enable: 0=no sleep, 1=BKP SRAM enters sleep (data lost); if BKPRAMREN also set, sleep takes priority */
            __IOM uint32_t BKPRAMREN           : 1; /*!< [31..31] BKP SRAM retention enable: 0=no retention, 1=BKP SRAM enters retention state */
        } CTRL0_f;
    };
    union {
        __IOM uint32_t CTRL1               ; /*!< Offset: 0x04 (read-write) PMU control register 1 - LVD (Low Voltage Detector) configuration */
        struct {
            __IOM uint32_t LVDEN               : 1; /*!< [0..0] LVD enable: 0=disable, 1=enable; after enable, LVD analog needs 2us settling time */
            __IOM uint32_t LVD_SEL             : 3; /*!< [3..1] LVD threshold voltage select (typical trigger values, release = trigger + 100mV): 000=1.71V, 001=2.01V, 010=2.23V, 011=2.43V, 100=2.51V, 101=2.73V, 110=2.80V, 111=2.90V */
            __IM  uint32_t          : 4;
            __IOM uint32_t LVD_FLTEN           : 1; /*!< [8..8] LVD digital filter enable: 0=disable, 1=enable; in STOP mode must disable filter or use RCL clock for filtering */
            __IOM uint32_t FLT_TIME            : 3; /*!< [11..9] LVD filter time (FiltClk cycles): 000=1, 001=2, 010=4, 011=16, 100=64, 101=256, 110=1024, 111=4095 */
            __IM  uint32_t          : 2;
            __IOM uint32_t LVD_FILTER          : 1; /*!< [14..14] LVD post-filter status (read-only) */
            __IOM uint32_t LVD_VALUE           : 1; /*!< [15..15] LVD raw status before filter (read-only) */
            __IM  uint32_t          : 16;
        } CTRL1_f;
    };
    union {
        __IOM uint32_t CTRL2               ; /*!< Offset: 0x08 (read-write) PMU control register 2 - WKUP pin enable/filter, STANDBY wakeup time, BOR configuration */
        struct {
            __IOM uint32_t EWUPX               : 8; /*!< [7..0] WKUPx(x=8~1) pin wakeup enable: bit x-1=1 enables WKUPx wakeup from STANDBY */
            __IOM uint32_t WUXFILEN            : 8; /*!< [15..8] WKUPx(x=8~1) pin filter enable using RTCCLK (1-stage glitch filter): 0=disable, 1=enable */
            __IOM uint32_t STDBY_WPT           : 4; /*!< [19..16] STANDBY mode wakeup wait time (RC32K cycles): 0000=invalid, 0001=1 cycle, ..., 1111=15 cycles */
            __IOM uint32_t BORRST_EN           : 1; /*!< [20..20] BOR reset enable: 0=disable, 1=enable; must disable BORRST_EN before disabling BOR_EN */
            __IM  uint32_t          : 3;
            __IOM uint32_t BOR_EN              : 1; /*!< [24..24] BOR enable: 0=disable, 1=enable; after enable output stabilizes in 1us, then set BORRST_EN */
            __IM  uint32_t          : 1;
            __IOM uint32_t BOR_CFG             : 2; /*!< [27..26] BOR threshold voltage select: 00=2.0~2.1V, 01=2.2~2.3V, 10=2.49~2.61V, 11=2.77~2.90V */
            __IM  uint32_t          : 4;
        } CTRL2_f;
    };
    union {
        __IOM uint32_t CTRL3               ; /*!< Offset: 0x0C (read-write) PMU control register 3 - WKUP pin wakeup polarity selection */
        struct {
            __IOM uint32_t WUPOLX              : 8; /*!< [7..0] WKUPx(x=8~1) pin wakeup polarity: bit x-1=0 for high level, =1 for low level wakeup */
            __IM  uint32_t          : 24;
        } CTRL3_f;
    };
    union {
        __IM  uint32_t SR                  ; /*!< Offset: 0x10 (read-only) Status register */
        struct {
            __IM  uint32_t WUPFX               : 8; /*!< [7..0] WKUPx wakeup flag - Set when WKUPx pin detects wakeup event; clear via CWUFX in PMU_STCLR; if high, STANDBY wakes immediately */
            __IM  uint32_t SBF                 : 1; /*!< [8..8] STANDBY flag - Hardware set when chip enters STANDBY; cleared by POR or CSBF in PMU_STCLR */
            __IM  uint32_t          : 3;
            __IM  uint32_t RTCWUF              : 1; /*!< [12..12] RTC wakeup flag - Set on RTC wakeup event; clear via CRTCWUF; even if not cleared, won't cause STANDBY immediate re-wake */
            __IM  uint32_t RSTWUF              : 1; /*!< [13..13] RSTN wakeup flag - Set on external reset wakeup; clear via CRSTWUF in PMU_STCLR */
            __IM  uint32_t IWDTWUF             : 1; /*!< [14..14] IWDT wakeup flag - Set on IWDT wakeup event; clear via CIWDTWUF in PMU_STCLR */
            __IM  uint32_t BORWUF              : 1; /*!< [15..15] BOR wakeup flag - Set on BOR wakeup event; clear via CBORWUF in PMU_STCLR; if not cleared, causes immediate STANDBY re-wake */
            __IM  uint32_t BORN                : 1; /*!< [16..16] BORN raw status - Reflects BOR comparator output (read-only, not a flag) */
            __IM  uint32_t          : 15;
        } SR_f;
    };
    union {
        __OM  uint32_t STCLR               ; /*!< Offset: 0x14 (write-only) PMU status clear register - Write 1 to clear corresponding wakeup/status flags */
        struct {
            __OM  uint32_t CWUFX               : 8; /*!< [7..0] Clear WKUPx wakeup flag: write 1 to clear corresponding WUPFx bit in PMU_SR */
            __OM  uint32_t CSBF                : 1; /*!< [8..8] Clear STANDBY flag: write 1 to clear SBF bit in PMU_SR */
            __IM  uint32_t          : 3;
            __OM  uint32_t CRTCWUF             : 1; /*!< [12..12] Clear RTC wakeup flag: write 1 to clear RTCWUF bit in PMU_SR */
            __OM  uint32_t CRSTWUF             : 1; /*!< [13..13] Clear RSTN wakeup flag: write 1 to clear RSTWUF bit in PMU_SR */
            __OM  uint32_t CIWDTWUF            : 1; /*!< [14..14] Clear IWDT wakeup flag: write 1 to clear IWDTWUF bit in PMU_SR */
            __OM  uint32_t CBORWUF             : 1; /*!< [15..15] Clear BOR wakeup flag: write 1 to clear BORWUF bit in PMU_SR */
            __IM  uint32_t          : 16;
        } STCLR_f;
    };
    union {
        __IOM uint32_t IOSEL               ; /*!< Offset: 0x18 (read-write) PMU IO selection register - STANDBY domain PC13/PC14/PC15 pin function multiplexing */
        struct {
            __IOM uint32_t PC13_SEL            : 2; /*!< [1..0] PC13 function select: 00=GPIO (input in STANDBY), 01=RTC Fout (PC13_VALUE selects push-pull=1 or open-drain=0), 10=RTC input Tamper, 11=output PC13_VALUE push-pull */
            __IOM uint32_t PC14_SEL            : 2; /*!< [3..2] PC14 function select: 00=GPIO (input in STANDBY), 01=output PC14_VALUE push-pull, other=output off; for XTL_IN, configure as analog port */
            __IOM uint32_t PC15_SEL            : 2; /*!< [5..4] PC15 function select: 00=GPIO (input in STANDBY), 01=output PC15_VALUE push-pull, other=output off; for XTL_OUT, configure as analog port */
            __IM  uint32_t          : 2;
            __IOM uint32_t PC13_VALUE          : 1; /*!< [8..8] PC13 output value when PC13_SEL configured for output */
            __IOM uint32_t PC14_VALUE          : 1; /*!< [9..9] PC14 output value when PC14_SEL configured for output */
            __IOM uint32_t PC15_VALUE          : 1; /*!< [10..10] PC15 output value when PC15_SEL configured for output */
            __IM  uint32_t          : 21;
        } IOSEL_f;
    };
} PMU_TypeDef;

/**
 * @brief LPTIM - LPTIM Register Structure
 */
typedef struct {
    union {
        __IM  uint32_t ISR                 ; /*!< Offset: 0x00 (read-only) Interrupt and status register */
        struct {
            __IM  uint32_t CMPM                : 1; /*!< [0..0] Compare match - LPTIM counter equals CMP value */
            __IM  uint32_t ARRM                : 1; /*!< [1..1] Autoreload match - LPTIM counter equals ARR value */
            __IM  uint32_t EXTTRIG             : 1; /*!< [2..2] External trigger edge event */
            __IM  uint32_t CMPOK               : 1; /*!< [3..3] Compare register update OK - LPTIM_CMP write completed */
            __IM  uint32_t ARROK               : 1; /*!< [4..4] Autoreload register update OK - LPTIM_ARR write completed */
            __IM  uint32_t UP                  : 1; /*!< [5..5] Counter direction change down to up - encoder mode only */
            __IM  uint32_t DOWN                : 1; /*!< [6..6] Counter direction change up to down - encoder mode only */
            __IM  uint32_t REPUE               : 1; /*!< [7..7] Repetition counter underflow event */
            __IM  uint32_t REPOK               : 1; /*!< [8..8] Repetition register update OK - LPTIM_RCR write completed */
            __IM  uint32_t          : 23;
        } ISR_f;
    };
    union {
        __OM  uint32_t ICR                 ; /*!< Offset: 0x04 (write-only) Interrupt clear register */
        struct {
            __OM  uint32_t CMPMCF              : 1; /*!< [0..0] Clear CMPM flag */
            __OM  uint32_t ARRMCF              : 1; /*!< [1..1] Clear ARRM flag */
            __OM  uint32_t EXTTRIGCF           : 1; /*!< [2..2] Clear EXTTRIG flag */
            __OM  uint32_t CMPOKCF             : 1; /*!< [3..3] Clear CMPOK flag */
            __OM  uint32_t ARROKCF             : 1; /*!< [4..4] Clear ARROK flag */
            __OM  uint32_t UPCF                : 1; /*!< [5..5] Clear UP flag */
            __OM  uint32_t DOWNCF              : 1; /*!< [6..6] Clear DOWN flag */
            __OM  uint32_t REPUECF             : 1; /*!< [7..7] Clear REPUE flag */
            __OM  uint32_t REPOKCF             : 1; /*!< [8..8] Clear REPOK flag */
            __IM  uint32_t          : 23;
        } ICR_f;
    };
    union {
        __IOM uint32_t IER                 ; /*!< Offset: 0x08 (read-write) Interrupt enable register */
        struct {
            __IOM uint32_t CMPMIE              : 1; /*!< [0..0] Compare match interrupt enable */
            __IOM uint32_t ARRMIE              : 1; /*!< [1..1] Autoreload match interrupt enable */
            __IOM uint32_t EXTTRIGIE           : 1; /*!< [2..2] External trigger valid edge interrupt enable */
            __IOM uint32_t CMPOKIE             : 1; /*!< [3..3] Compare register update OK interrupt enable */
            __IOM uint32_t ARROKIE             : 1; /*!< [4..4] Autoreload register update OK interrupt enable */
            __IOM uint32_t UPIE                : 1; /*!< [5..5] Direction change to up interrupt enable */
            __IOM uint32_t DOWNIE              : 1; /*!< [6..6] Direction change to down interrupt enable */
            __IOM uint32_t REPUEIE             : 1; /*!< [7..7] Repetition counter underflow event interrupt enable */
            __IOM uint32_t REPOKIE             : 1; /*!< [8..8] Repetition register update OK interrupt enable */
            __IM  uint32_t          : 23;
        } IER_f;
    };
    union {
        __IOM uint32_t CFGR                ; /*!< Offset: 0x0C (read-write) Configuration register */
        struct {
            __IOM uint32_t CKSEL               : 1; /*!< [0..0] Clock selector - LPTIM clock source selection */
            __IOM uint32_t CKPOL               : 2; /*!< [2..1] Clock polarity - external clock edge selection / encoder sub-mode */
            __IM  uint32_t          : 1;
            __IOM uint32_t CKFLT               : 2; /*!< [5..4] Configurable digital filter for external clock */
            __IOM uint32_t TRGFLT              : 2; /*!< [7..6] Configurable digital filter for trigger */
            __IM  uint32_t          : 1;
            __IOM uint32_t PRESC               : 3; /*!< [11..9] Clock prescaler - division factor from 1 to 128 */
            __IM  uint32_t          : 1;
            __IOM uint32_t TRIGSEL             : 4; /*!< [16..13] Trigger selector - selects LPTIM trigger source from 8 inputs */
            __IOM uint32_t TRIGEN              : 2; /*!< [18..17] Trigger enable and polarity - software or external trigger with edge selection */
            __IOM uint32_t TIMEOUT             : 1; /*!< [19..19] Timeout enable - external trigger resets counter when timer already started */
            __IOM uint32_t WAVE                : 1; /*!< [20..20] Waveform shape - PWM/one-shot or set-once mode */
            __IOM uint32_t WAVPOL              : 1; /*!< [21..21] Waveform output polarity */
            __IOM uint32_t PRELOAD             : 1; /*!< [22..22] Registers update mode - immediate or at end of current period */
            __IOM uint32_t COUNTMODE           : 1; /*!< [23..23] Counter mode - internal clock or external Input1 clock pulses */
            __IOM uint32_t ENC                 : 1; /*!< [24..24] Encoder mode enable */
            __IOM uint32_t REP_LODMOD          : 1; /*!< [25..25] Repetition register load mode */
            __IM  uint32_t          : 6;
        } CFGR_f;
    };
    union {
        __IOM uint32_t CR                  ; /*!< Offset: 0x10 (read-write) Control register */
        struct {
            __IOM uint32_t ENABLE              : 1; /*!< [0..0] LPTIM enable */
            __IOM uint32_t SNGSTRT             : 1; /*!< [1..1] LPTIM start in single mode */
            __IOM uint32_t CNTSTRT             : 1; /*!< [2..2] Timer start in continuous mode */
            __IOM uint32_t COUNTRST            : 1; /*!< [3..3] Counter reset - synchronous reset of LPTIM_CNT */
            __IOM uint32_t RSTARE              : 1; /*!< [4..4] Reset after read enable - async reset LPTIM_CNT on read */
            __IM  uint32_t          : 27;
        } CR_f;
    };
    union {
        __IOM uint32_t CMP                 ; /*!< Offset: 0x14 (read-write) Compare register */
        struct {
            __IOM uint32_t CMP                 : 16; /*!< [15..0] Compare value */
            __IM  uint32_t          : 16;
        } CMP_f;
    };
    union {
        __IOM uint32_t ARR                 ; /*!< Offset: 0x18 (read-write) Autoreload register */
        struct {
            __IOM uint32_t ARR                 : 16; /*!< [15..0] Autoreload value - must be strictly greater than CMP */
            __IM  uint32_t          : 16;
        } ARR_f;
    };
    union {
        __IM  uint32_t CNT                 ; /*!< Offset: 0x1C (read-only) Counter register */
        struct {
            __IM  uint32_t CNT                 : 16; /*!< [15..0] Counter value - read twice and compare for reliable result */
            __IM  uint32_t          : 16;
        } CNT_f;
    };
    __IO uint32_t RESERVED0;
    union {
        __IOM uint32_t CFGR2               ; /*!< Offset: 0x24 (read-write) Configuration register 2 */
        struct {
            __IOM uint32_t IN1SEL              : 4; /*!< [3..0] LPTIM input 1 selection - connects Input1 to available sources */
            __IOM uint32_t IN2SEL              : 4; /*!< [7..4] LPTIM input 2 selection - connects Input2 to available sources */
            __IM  uint32_t          : 24;
        } CFGR2_f;
    };
    union {
        __IOM uint32_t RCR                 ; /*!< Offset: 0x28 (read-write) Repetition counter register */
        struct {
            __IOM uint32_t REP                 : 8; /*!< [7..0] Repetition counter value - loaded on next underflow event */
            __IM  uint32_t          : 24;
        } RCR_f;
    };
} LPTIM_TypeDef;

/**
 * @brief LPUART - LPUART Register Structure
 */
typedef struct {
    union {
        __IM  uint32_t RXDR                ; /*!< Offset: 0x00 (read-only) Receive data register - LPUART received data */
        struct {
            __IM  uint32_t RXDATA              : 8; /*!< [7..0] Receive data - Received data register */
            __IM  uint32_t          : 24;
        } RXDR_f;
    };
    union {
        __OM  uint32_t TXDR                ; /*!< Offset: 0x04 (write-only) Transmit data register - LPUART data to transmit */
        struct {
            __OM  uint32_t TXDATA              : 8; /*!< [7..0] TX data - Transmit data register */
            __IM  uint32_t          : 24;
        } TXDR_f;
    };
    union {
        __IOM uint32_t LCR                 ; /*!< Offset: 0x08 (read-write) Line control register - Configures data bits, stop bits, parity, and polarity */
        struct {
            __IOM uint32_t PEN                 : 1; /*!< [0..0] Parity enable - Enables parity check */
            __IOM uint32_t SPS                 : 1; /*!< [1..1] Parity mode select - 0: odd/even parity, 1: 0/1 parity */
            __IOM uint32_t EPS                 : 1; /*!< [2..2] Even parity select - 0: odd/force 1, 1: even/force 0 */
            __IOM uint32_t STP2                : 1; /*!< [3..3] Stop bits select - 0: 1 stop bit, 1: 2 stop bits */
            __IOM uint32_t WLEN                : 1; /*!< [4..4] Word length - 0: 8 bits, 1: 7 bits */
            __IOM uint32_t RXWKS               : 2; /*!< [6..5] Receive wakeup select - 00: START bit, 01: 1 byte, 10: address match, 11: no wakeup */
            __IOM uint32_t WKCK                : 1; /*!< [7..7] Wakeup check - 0: skip parity/STOP check, 1: check parity and STOP */
            __IOM uint32_t RXPOL               : 1; /*!< [8..8] Receive polarity - Receive signal polarity */
            __IOM uint32_t TXPOL               : 1; /*!< [9..9] TX polarity - Transmit signal polarity */
            __IM  uint32_t          : 5;
            __IOM uint32_t AUTO_START_EN       : 1; /*!< [15..15] Auto start enable - Enables automatic start on trigger */
            __IOM uint32_t BCNT_VALUE          : 16; /*!< [31..16] Bit count timeout value - Internal timer duration after STOP bit */
        } LCR_f;
    };
    union {
        __IOM uint32_t CR                  ; /*!< Offset: 0x0C (read-write) Control register - TX/RX enable and DMA control */
        struct {
            __IOM uint32_t RX_EN               : 1; /*!< [0..0] Receive enable - Enables receiver */
            __IOM uint32_t TX_EN               : 1; /*!< [1..1] Transmit enable - Enables transmitter */
            __IOM uint32_t DMA_EN              : 1; /*!< [2..2] DMA enable - Enables DMA data transfer */
            __IM  uint32_t          : 29;
        } CR_f;
    };
    union {
        __IOM uint32_t IBAUD               ; /*!< Offset: 0x10 (read-write) Integer baud rate register - Integer part of baud rate divider */
        struct {
            __IOM uint32_t IBAUD               : 8; /*!< [7..0] Integer baud rate divider - (Fsys/BAUD) - 1, range 2~254 */
            __IOM uint32_t RXSAM               : 8; /*!< [15..8] Receive sampling point - Typically set to IBAUD>>1 */
            __IM  uint32_t          : 16;
        } IBAUD_f;
    };
    union {
        __IOM uint32_t FBAUD               ; /*!< Offset: 0x14 (read-write) Fractional baud rate register - Fractional part of baud rate */
        struct {
            __IOM uint32_t FBAUD               : 12; /*!< [11..0] Fractional baud rate - 0: no adjustment, 1: add one clock cycle */
            __IM  uint32_t          : 20;
        } FBAUD_f;
    };
    union {
        __IOM uint32_t IE                  ; /*!< Offset: 0x18 (read-write) Interrupt enable register - Enables or disables various interrupt sources */
        struct {
            __IOM uint32_t RXIE                : 1; /*!< [0..0] Receive interrupt enable - Enables receive interrupt */
            __IOM uint32_t TCIE                : 1; /*!< [1..1] Transmit complete interrupt enable - Enables transmission complete interrupt */
            __IOM uint32_t TXEIE               : 1; /*!< [2..2] Transmit data register empty interrupt enable - Enables TXE interrupt */
            __IOM uint32_t PEIE                : 1; /*!< [3..3] Parity error interrupt enable - Enables parity error interrupt */
            __IOM uint32_t FEIE                : 1; /*!< [4..4] Framing error interrupt enable - Enables framing error interrupt */
            __IOM uint32_t RXOVIE              : 1; /*!< [5..5] Receive overrun interrupt enable - Enables RX overrun interrupt */
            __IM  uint32_t          : 2;
            __IOM uint32_t MATCHIE             : 1; /*!< [8..8] Match interrupt enable - Enables match interrupt */
            __IOM uint32_t STARTIE             : 1; /*!< [9..9] Start interrupt enable - Enables start condition interrupt */
            __IM  uint32_t          : 6;
            __IOM uint32_t BCNTIE              : 1; /*!< [16..16] Bit count timeout interrupt enable - Enables BCNT interrupt */
            __IOM uint32_t IDLEIE              : 1; /*!< [17..17] Idle interrupt enable - Enables IDLE detection interrupt */
            __IM  uint32_t          : 14;
        } IE_f;
    };
    union {
        __IOM uint32_t SR                  ; /*!< Offset: 0x1C (read-write) Status register - Interrupt flags and status bits */
        struct {
            __IOM uint32_t RXIF                : 1; /*!< [0..0] Receive interrupt flag - Receive interrupt status */
            __IOM uint32_t TCIF                : 1; /*!< [1..1] Transfer complete flag - Set on transfer completion */
            __IOM uint32_t TXEIF               : 1; /*!< [2..2] TX empty interrupt flag - Transmit empty interrupt flag */
            __IOM uint32_t PEIF                : 1; /*!< [3..3] PEIF - PE interrupt flag */
            __IOM uint32_t FEIF                : 1; /*!< [4..4] Framing error interrupt flag - Framing error interrupt status */
            __IOM uint32_t RXOVIF              : 1; /*!< [5..5] Receive overrun interrupt flag - RX overrun interrupt flag */
            __IOM uint32_t RXF                 : 1; /*!< [6..6] Receive flag - Receive status flag */
            __IOM uint32_t TXOVF               : 1; /*!< [7..7] TX overflow - Transmit overflow flag */
            __IOM uint32_t MATCHIF             : 1; /*!< [8..8] Match interrupt flag - Match interrupt flag */
            __IOM uint32_t STARTIF             : 1; /*!< [9..9] Start interrupt flag - Start condition interrupt flag */
            __IOM uint32_t TXE                 : 1; /*!< [10..10] Transmit buffer empty - Indicates TX buffer is empty */
            __IM  uint32_t          : 5;
            __IOM uint32_t BCNTIF              : 1; /*!< [16..16] Bit count timeout interrupt flag - Write 1 to clear */
            __IOM uint32_t IDLEIF              : 1; /*!< [17..17] Idle detection interrupt flag - Write 1 to clear */
            __IM  uint32_t          : 14;
        } SR_f;
    };
    union {
        __IOM uint32_t ADDR                ; /*!< Offset: 0x20 (read-write) Address register - Match address for wakeup from STOP mode */
        struct {
            __IOM uint32_t ADDR                : 8; /*!< [7..0] Address - Address value for memory or peripheral addressing */
            __IM  uint32_t          : 24;
        } ADDR_f;
    };
} LPUART_TypeDef;

/**
 * @brief EFUSE - EFUSE Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t WP                  ; /*!< Offset: 0x00 (read-write) WP - WP value */
        struct {
            __IOM uint32_t WP                  : 32; /*!< [31..0] WP - WP value */
        } WP_f;
    };
    union {
        __IOM uint32_t CTRL                ; /*!< Offset: 0x04 (read-write) Control - Control bit for module configuration */
        struct {
            __IOM uint32_t TRIG                : 1; /*!< [0..0] Trigger - Trigger source or manual trigger bit */
            __IM  uint32_t          : 3;
            __IOM uint32_t MODE                : 1; /*!< [4..4] Mode - Operating mode select */
            __IM  uint32_t          : 27;
        } CTRL_f;
    };
    union {
        __IOM uint32_t AR                  ; /*!< Offset: 0x08 (read-write) Alarm register - RTC alarm value register */
        struct {
            __IOM uint32_t ADDR                : 11; /*!< [10..0] Address - Address value for memory or peripheral addressing */
            __IM  uint32_t          : 21;
        } AR_f;
    };
    union {
        __IOM uint32_t DWR                 ; /*!< Offset: 0x0C (read-write) Data write register - Data write register */
        struct {
            __IOM uint32_t WDATA               : 8; /*!< [7..0] W data - Write data register */
            __IM  uint32_t          : 24;
        } DWR_f;
    };
    union {
        __IM  uint32_t SR                  ; /*!< Offset: 0x10 (read-only) Status register - Power management status flags */
        struct {
            __IM  uint32_t DONE                : 1; /*!< [0..0] Done flag - Operation complete flag */
            __IM  uint32_t UNPG                : 1; /*!< [1..1] UNPG - UNPG value */
            __IM  uint32_t          : 29;
            __IM  uint32_t PREREAD_DONE        : 1; /*!< [31..31] Preread done - Preread complete flag */
        } SR_f;
    };
    union {
        __OM  uint32_t CLR                 ; /*!< Offset: 0x14 (write-only) Clear - Clear control bit for flags */
        struct {
            __OM  uint32_t CDONE               : 1; /*!< [0..0] Conversion done - ADC conversion complete flag */
            __OM  uint32_t CUNPG               : 1; /*!< [1..1] Channel unplugged - Channel unplugged status flag */
            __IM  uint32_t          : 30;
        } CLR_f;
    };
    union {
        __IM  uint32_t DR                  ; /*!< Offset: 0x18 (read-only) Data register - CRC calculation data input and result */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DR_f;
    };
    union {
        __IOM uint32_t DSDP                ; /*!< Offset: 0x1C (read-write) Deep-sleep power-down - Power-down in deep-sleep mode */
        struct {
            __IOM uint32_t DP                  : 32; /*!< [31..0] Data polarity - Data signal polarity */
        } DSDP_f;
    };
    union {
        __IOM uint32_t BYTEWP              ; /*!< Offset: 0x20 (read-write) Byte write protection - Byte-level write protection enable */
        struct {
            __IOM uint32_t BYTEWP              : 32; /*!< [31..0] Byte write protection - Byte-level write protection enable */
        } BYTEWP_f;
    };
    union {
        __IOM uint32_t PGCFG               ; /*!< Offset: 0x24 (read-write) PG configuration - PG config */
        struct {
            __IOM uint32_t PGT                 : 8; /*!< [7..0] PG time - Program time */
            __IOM uint32_t PGWT                : 8; /*!< [15..8] PG wait - Program wait time */
            __IOM uint32_t AVDD_HD             : 8; /*!< [23..16] AVDD hold - AVDD voltage hold control */
            __IOM uint32_t AVDD_SP             : 8; /*!< [31..24] AVDD sampling - AVDD voltage sampling control */
        } PGCFG_f;
    };
    __IO uint32_t RESERVED0[246];
    union {
        __IM  uint32_t DSR0                ; /*!< Offset: 0x400 (read-only) Data shift register 0 - Data shift register for bit 0 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR0_f;
    };
    union {
        __IM  uint32_t DSR1                ; /*!< Offset: 0x404 (read-only) Data shift register 1 - Data shift register for bit 1 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR1_f;
    };
    union {
        __IM  uint32_t DSR2                ; /*!< Offset: 0x408 (read-only) Data shift register 2 - Data shift register for bit 2 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR2_f;
    };
    union {
        __IM  uint32_t DSR3                ; /*!< Offset: 0x40c (read-only) Data shift register 3 - Data shift register for bit 3 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR3_f;
    };
    union {
        __IM  uint32_t DSR4                ; /*!< Offset: 0x410 (read-only) Data shift register 4 - Data shift register for bit 4 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR4_f;
    };
    union {
        __IM  uint32_t DSR5                ; /*!< Offset: 0x414 (read-only) Data shift register 5 - Data shift register for bit 5 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR5_f;
    };
    union {
        __IM  uint32_t DSR6                ; /*!< Offset: 0x418 (read-only) Data shift register 6 - Data shift register for bit 6 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR6_f;
    };
    union {
        __IM  uint32_t DSR7                ; /*!< Offset: 0x41c (read-only) Data shift register 7 - Data shift register for bit 7 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR7_f;
    };
    union {
        __IM  uint32_t DSR8                ; /*!< Offset: 0x420 (read-only) Data shift register 8 - Data shift register for bit 8 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR8_f;
    };
    union {
        __IM  uint32_t DSR9                ; /*!< Offset: 0x424 (read-only) Data shift register 9 - Data shift register for bit 9 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR9_f;
    };
    union {
        __IM  uint32_t DSR10               ; /*!< Offset: 0x428 (read-only) Data shift register 10 - Data shift register for bit 10 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR10_f;
    };
    union {
        __IM  uint32_t DSR11               ; /*!< Offset: 0x42c (read-only) Data shift register 11 - Data shift register for bit 11 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR11_f;
    };
    union {
        __IM  uint32_t DSR12               ; /*!< Offset: 0x430 (read-only) Data shift register 12 - Data shift register for bit 12 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR12_f;
    };
    union {
        __IM  uint32_t DSR13               ; /*!< Offset: 0x434 (read-only) Data shift register 13 - Data shift register for bit 13 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR13_f;
    };
    union {
        __IM  uint32_t DSR14               ; /*!< Offset: 0x438 (read-only) Data shift register 14 - Data shift register for bit 14 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR14_f;
    };
    union {
        __IM  uint32_t DSR15               ; /*!< Offset: 0x43c (read-only) Data shift register 15 - Data shift register for bit 15 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR15_f;
    };
    union {
        __IM  uint32_t DSR16               ; /*!< Offset: 0x440 (read-only) Data shift register 16 - Data shift register for bit 16 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR16_f;
    };
    union {
        __IM  uint32_t DSR17               ; /*!< Offset: 0x444 (read-only) Data shift register 17 - Data shift register for bit 17 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR17_f;
    };
    union {
        __IM  uint32_t DSR18               ; /*!< Offset: 0x448 (read-only) Data shift register 18 - Data shift register for bit 18 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR18_f;
    };
    union {
        __IM  uint32_t DSR19               ; /*!< Offset: 0x44c (read-only) Data shift register 19 - Data shift register for bit 19 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR19_f;
    };
    union {
        __IM  uint32_t DSR20               ; /*!< Offset: 0x450 (read-only) Data shift register 20 - Data shift register for bit 20 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR20_f;
    };
    union {
        __IM  uint32_t DSR21               ; /*!< Offset: 0x454 (read-only) Data shift register 21 - Data shift register for bit 21 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR21_f;
    };
    union {
        __IM  uint32_t DSR22               ; /*!< Offset: 0x458 (read-only) Data shift register 22 - Data shift register for bit 22 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR22_f;
    };
    union {
        __IM  uint32_t DSR23               ; /*!< Offset: 0x45c (read-only) Data shift register 23 - Data shift register for bit 23 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR23_f;
    };
    union {
        __IM  uint32_t DSR24               ; /*!< Offset: 0x460 (read-only) Data shift register 24 - Data shift register for bit 24 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR24_f;
    };
    union {
        __IM  uint32_t DSR25               ; /*!< Offset: 0x464 (read-only) Data shift register 25 - Data shift register for bit 25 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR25_f;
    };
    union {
        __IM  uint32_t DSR26               ; /*!< Offset: 0x468 (read-only) Data shift register 26 - Data shift register for bit 26 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR26_f;
    };
    union {
        __IM  uint32_t DSR27               ; /*!< Offset: 0x46c (read-only) Data shift register 27 - Data shift register for bit 27 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR27_f;
    };
    union {
        __IM  uint32_t DSR28               ; /*!< Offset: 0x470 (read-only) Data shift register 28 - Data shift register for bit 28 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR28_f;
    };
    union {
        __IM  uint32_t DSR29               ; /*!< Offset: 0x474 (read-only) Data shift register 29 - Data shift register for bit 29 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR29_f;
    };
    union {
        __IM  uint32_t DSR30               ; /*!< Offset: 0x478 (read-only) Data shift register 30 - Data shift register for bit 30 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR30_f;
    };
    union {
        __IM  uint32_t DSR31               ; /*!< Offset: 0x47c (read-only) Data shift register 31 - Data shift register for bit 31 */
        struct {
            __IM  uint32_t DATA                : 8; /*!< [7..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t          : 24;
        } DSR31_f;
    };
} EFUSE_TypeDef;

/**
 * @brief SYSCFG - SYSCFG Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t SYSCR               ; /*!< Offset: 0x00 (read-write) System control register - Controls system configuration settings */
        struct {
            __IOM uint32_t LOCKUP_LOCK         : 1; /*!< [0..0] Core LOCKUP output lock */
            __IOM uint32_t XTHSD_LOCK          : 1; /*!< [1..1] XTH clock stop detection lock */
            __IOM uint32_t LVD_LOCK            : 1; /*!< [2..2] LVD detection error lock */
            __IM  uint32_t          : 8;
            __IOM uint32_t BKPSRAM_LOCK        : 1; /*!< [11..11] Backup SRAM ECC double-bit error detection lock */
            __IOM uint32_t ETHMAC_TX_CLKGE     : 1; /*!< [12..12] ETH MAC TX clock gating enable - Ethernet TX clock gating */
            __IOM uint32_t EPIS                : 3; /*!< [15..13] Ethernet PHY interface selection */
            __IM  uint32_t          : 3;
            __IOM uint32_t BOOT_PUN            : 1; /*!< [19..19] BOOT0 pin pull-up disable control */
            __IOM uint32_t ETHMAC_RX_DLYSEL    : 1; /*!< [20..20] ETH MAC RX delay select - Ethernet RX delay line selection */
            __IOM uint32_t ETH_FIFO_FLOWCTRL   : 1; /*!< [21..21] ETH FIFO flow control - Ethernet FIFO flow control config */
            __IOM uint32_t ETH_FLOWCTRL        : 1; /*!< [22..22] ETH flow control - Ethernet flow control config */
            __IM  uint32_t          : 1;
            __IOM uint32_t IR_POL              : 1; /*!< [24..24] IR polarity - Infrared signal polarity */
            __IOM uint32_t IR_DATA_SEL         : 3; /*!< [27..25] IR data select - Infrared data source selection */
            __IOM uint32_t IR_CLK_SEL          : 1; /*!< [28..28] IR clock select - Infrared clock source selection */
            __IM  uint32_t          : 3;
        } SYSCR_f;
    };
    union {
        __IM  uint32_t WMR                 ; /*!< Offset: 0x04 (read-only) Working mode register - Configures device operating mode */
        struct {
            __IM  uint32_t          : 3;
            __IM  uint32_t BOOTMODE            : 1; /*!< [3..3] Boot mode - Selects boot mode configuration */
            __IM  uint32_t          : 2;
            __IM  uint32_t RTCREADY            : 1; /*!< [6..6] RTC ready - RTC ready flag */
            __IM  uint32_t          : 1;
            __IM  uint32_t BOOTDEVICE          : 2; /*!< [9..8] Boot device - Selects boot device */
            __IM  uint32_t          : 22;
        } WMR_f;
    };
    union {
        __IM  uint32_t VER                 ; /*!< Offset: 0x08 (read-only) Version register - Device version information */
        struct {
            __IM  uint32_t VERSION             : 32; /*!< [31..0] Version - Version identification register */
        } VER_f;
    };
    __IO uint32_t RESERVED0;
    union {
        __IOM uint32_t PHYCFG              ; /*!< Offset: 0x10 (read-write) USBPHY configuration register - PHY configuration register */
        struct {
            __IOM uint32_t PHY_RSTN            : 1; /*!< [0..0] PHY reset - PHY reset control */
            __IOM uint32_t FPLL_H60M_L48M      : 1; /*!< [1..1] FPLL H60M/L48M - High 60MHz/Low 48MHz PLL mode */
            __IOM uint32_t PD_PLL              : 1; /*!< [2..2] Power down PLL - PLL power down control */
            __IOM uint32_t OSC_MODE            : 2; /*!< [4..3] Oscillator mode - Oscillator mode selection */
            __IM  uint32_t          : 1;
            __IOM uint32_t DMPD                : 1; /*!< [6..6] USBPHY DM pull-down resistor selection, active high */
            __IOM uint32_t DUPD                : 1; /*!< [7..7] USBPHY DP pull-down resistor selection, active high */
            __IM  uint32_t          : 8;
            __IOM uint32_t CLKSEL_LRC_HXO      : 1; /*!< [16..16] Clock selection LRC/HXO - Selects low-speed RC or high-speed crystal */
            __IOM uint32_t PLL_LOCK            : 1; /*!< [17..17] PLL lock - PLL lock status */
            __IOM uint32_t ADJ_END             : 1; /*!< [18..18] Adjustment end flag - Indicates calibration adjustment is complete */
            __IOM uint32_t CLKSEL_END          : 1; /*!< [19..19] USB PHY external clock source query complete flag */
            __IM  uint32_t          : 4;
            __IOM uint32_t USB_EN              : 8; /*!< [31..24] USB enable - Enables USB peripheral */
        } PHYCFG_f;
    };
    union {
        __IOM uint32_t DUMMY               ; /*!< Offset: 0x14 (read-write) Dummy register, default 0, readable and writable */
        struct {
            __IOM uint32_t DUMMY               : 32; /*!< [31..0] Dummy data */
        } DUMMY_f;
    };
    union {
        __IOM uint32_t RAMECCIR            ; /*!< Offset: 0x18 (read-write) SRAM ECC interrupt register - SRAM ECC error interrupt control */
        struct {
            __IM  uint32_t          : 8;
            __IOM uint32_t BKPSRAM_SEC_IE      : 1; /*!< [8..8] BKPSRAM SEC interrupt enable - Enables single-bit ECC error interrupt */
            __IOM uint32_t BKPSRAM_DED_IE      : 1; /*!< [9..9] BKPSRAM DED interrupt enable - Enables double-bit ECC error interrupt */
            __IM  uint32_t          : 22;
        } RAMECCIR_f;
    };
    union {
        __IM  uint32_t RAMECCSR            ; /*!< Offset: 0x1C (read-only) SRAM ECC status register - SRAM ECC error status flags */
        struct {
            __IM  uint32_t          : 8;
            __IM  uint32_t BKPSRAM_SEC_IF      : 1; /*!< [8..8] BKPSRAM SEC interrupt flag - Single-bit ECC error detected */
            __IM  uint32_t BKPSRAM_DED_IF      : 1; /*!< [9..9] BKPSRAM DED interrupt flag - Double-bit ECC error detected */
            __IM  uint32_t          : 22;
        } RAMECCSR_f;
    };
    union {
        __OM  uint32_t RAMECCICR           ; /*!< Offset: 0x20 (write-only) SRAM ECC interrupt clear register - Clears SRAM ECC interrupt flags */
        struct {
            __IM  uint32_t          : 8;
            __OM  uint32_t BKPSRAM_SEC_IC      : 1; /*!< [8..8] BKPSRAM SEC interrupt clear - Clears single-bit ECC error interrupt */
            __OM  uint32_t BKPSRAM_DED_IC      : 1; /*!< [9..9] BKPSRAM DED interrupt clear - Clears double-bit ECC error interrupt */
            __IM  uint32_t          : 22;
        } RAMECCICR_f;
    };
    __IO uint32_t RESERVED1[2];
    union {
        __IOM uint32_t DFTCR               ; /*!< Offset: 0x2C (read-write) Digital filter control register - Digital filter configuration */
        struct {
            __IOM uint32_t DFT_DIS             : 32; /*!< [31..0] Digital filter disable - Disables digital filter */
        } DFTCR_f;
    };
    union {
        __IOM uint32_t JTAGCR              ; /*!< Offset: 0x30 (read-write) JTAG control register - JTAG interface control */
        struct {
            __IOM uint32_t JTAG_DIS            : 32; /*!< [31..0] JTAG disable - Disables JTAG interface */
        } JTAGCR_f;
    };
    __IO uint32_t RESERVED2;
    union {
        __IOM uint32_t BUSLOCK             ; /*!< Offset: 0x38 (read-write) Bus lock - Bus lock status flag */
        struct {
            __IOM uint32_t BUSLOCK_DIS         : 1; /*!< [0..0] Bus lock disable - Disables bus locking mechanism */
            __IM  uint32_t          : 31;
        } BUSLOCK_f;
    };
    union {
        __IOM uint32_t QSPI7RDPCR          ; /*!< Offset: 0x3C (read-write) QSPI7 read data pipeline control - QSPI7 read data pipeline */
        struct {
            __IOM uint32_t RDP_EN              : 32; /*!< [31..0] RDP enable - Enables RDP */
        } QSPI7RDPCR_f;
    };
    union {
        __IOM uint32_t OSPI2RDPCR          ; /*!< Offset: 0x40 (read-write) OSPI2 read data pipeline control - OSPI2 read data pipeline */
        struct {
            __IOM uint32_t RDP_EN              : 32; /*!< [31..0] RDP enable - Enables RDP */
        } OSPI2RDPCR_f;
    };
    union {
        __IOM uint32_t OSPI2REMAP          ; /*!< Offset: 0x44 (read-write) OSPI2 address remap control register */
        struct {
            __IOM uint32_t REMAP_EN            : 32; /*!< [31..0] OSPI2 address remap control */
        } OSPI2REMAP_f;
    };
} SYSCFG_TypeDef;

/**
 * @brief COMP - COMP Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t CR                  ; /*!< Offset: 0x00 (read-write) COMP1 control register */
        struct {
            __IOM uint32_t EN                  : 1; /*!< [0..0] Comparator 1 enable */
            __IOM uint32_t HYS                 : 3; /*!< [3..1] Hysteresis - Hysteresis configuration */
            __IOM uint32_t INMSEL              : 4; /*!< [7..4] Inverting input selection - Selects comparator inverting input */
            __IOM uint32_t INPSEL              : 4; /*!< [11..8] Input selection - Selects comparator non-inverting input */
            __IOM uint32_t BLANKSEL            : 3; /*!< [14..12] Blank selection - Selects blanking source */
            __IM  uint32_t          : 1;
            __IOM uint32_t FLTTIME             : 3; /*!< [18..16] Filter time - Digital filter time constant */
            __IOM uint32_t FLTEN               : 1; /*!< [19..19] Comparator 1 filter enable */
            __IOM uint32_t POLARITY            : 1; /*!< [20..20] Polarity - Signal polarity selection */
            __IM  uint32_t          : 2;
            __IOM uint32_t CRV_EN              : 1; /*!< [23..23] Compare reference value enable - Enables reference value comparison */
            __IOM uint32_t CRV_SEL             : 1; /*!< [24..24] Compare reference value selection - Selects reference value source */
            __IOM uint32_t CRV_CFG             : 4; /*!< [28..25] Compare reference value configuration - Configures reference value */
            __IM  uint32_t          : 2;
            __IOM uint32_t LOCK                : 1; /*!< [31..31] COMP_CR1 register write protection lock */
        } CR_f;
    };
    union {
        __IM  uint32_t SR                  ; /*!< Offset: 0x04 (read-only) COMP1 status register */
        struct {
            __IM  uint32_t VCOUT1              : 1; /*!< [0..0] Comparator 1 filtered output status */
            __IM  uint32_t          : 3;
            __IM  uint32_t VCOUT1_ORG          : 1; /*!< [4..4] VCOUT1 original - VCOUT1 original value */
            __IM  uint32_t          : 27;
        } SR_f;
    };
} COMP_TypeDef;

/**
 * @brief EXTI - EXTI Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t IENR                ; /*!< Offset: 0x0000 (read-write) Interrupt enable register - Enable/disable interrupt requests from EXTI lines 0~24 */
        struct {
            __IOM uint32_t INTEN0              : 1; /*!< [0..0] Interrupt enable on EXTI line 0 - PA0~PH0 */
            __IOM uint32_t INTEN1              : 1; /*!< [1..1] Interrupt enable on EXTI line 1 - PA1~PH1 */
            __IOM uint32_t INTEN2              : 1; /*!< [2..2] Interrupt enable on EXTI line 2 - PA2~PH2 */
            __IOM uint32_t INTEN3              : 1; /*!< [3..3] Interrupt enable on EXTI line 3 - PA3~PH3 */
            __IOM uint32_t INTEN4              : 1; /*!< [4..4] Interrupt enable on EXTI line 4 - PA4~PH4 */
            __IOM uint32_t INTEN5              : 1; /*!< [5..5] Interrupt enable on EXTI line 5 - PA5~PH5 */
            __IOM uint32_t INTEN6              : 1; /*!< [6..6] Interrupt enable on EXTI line 6 - PA6~PH6 */
            __IOM uint32_t INTEN7              : 1; /*!< [7..7] Interrupt enable on EXTI line 7 - PA7~PH7 */
            __IOM uint32_t INTEN8              : 1; /*!< [8..8] Interrupt enable on EXTI line 8 - PA8~PG8 */
            __IOM uint32_t INTEN9              : 1; /*!< [9..9] Interrupt enable on EXTI line 9 - PA9~PG9 */
            __IOM uint32_t INTEN10             : 1; /*!< [10..10] Interrupt enable on EXTI line 10 - PA10~PG10 */
            __IOM uint32_t INTEN11             : 1; /*!< [11..11] Interrupt enable on EXTI line 11 - PA11~PG11 */
            __IOM uint32_t INTEN12             : 1; /*!< [12..12] Interrupt enable on EXTI line 12 - PA12~PG12 */
            __IOM uint32_t INTEN13             : 1; /*!< [13..13] Interrupt enable on EXTI line 13 - PA13~PG13 */
            __IOM uint32_t INTEN14             : 1; /*!< [14..14] Interrupt enable on EXTI line 14 - PA14~PG14 */
            __IOM uint32_t INTEN15             : 1; /*!< [15..15] Interrupt enable on EXTI line 15 - PA15~PG15 */
            __IOM uint32_t INTEN16             : 1; /*!< [16..16] Interrupt enable on EXTI line 16 - LVD */
            __IOM uint32_t INTEN17             : 1; /*!< [17..17] Interrupt enable on EXTI line 17 - RTC/XTL stop */
            __IOM uint32_t INTEN18             : 1; /*!< [18..18] Interrupt enable on EXTI line 18 - IWDT */
            __IOM uint32_t INTEN19             : 1; /*!< [19..19] Interrupt enable on EXTI line 19 - COMP */
            __IOM uint32_t INTEN20             : 1; /*!< [20..20] Interrupt enable on EXTI line 20 - USB_Wakeup */
            __IOM uint32_t INTEN21             : 1; /*!< [21..21] Interrupt enable on EXTI line 21 - LPTIM1_Wakeup */
            __IOM uint32_t INTEN22             : 1; /*!< [22..22] Interrupt enable on EXTI line 22 - LPUART1_Wakeup */
            __IOM uint32_t INTEN23             : 1; /*!< [23..23] Interrupt enable on EXTI line 23 - LPUART2_Wakeup */
            __IOM uint32_t INTEN24             : 1; /*!< [24..24] Interrupt enable on EXTI line 24 - ETH_Wakeup */
            __IM  uint32_t          : 7;
        } IENR_f;
    };
    union {
        __IOM uint32_t EENR                ; /*!< Offset: 0x0004 (read-write) Event enable register - Enable/disable event requests from EXTI lines 0~24 */
        struct {
            __IOM uint32_t EVEN0               : 1; /*!< [0..0] Event enable on EXTI line 0 - PA0~PH0 */
            __IOM uint32_t EVEN1               : 1; /*!< [1..1] Event enable on EXTI line 1 - PA1~PH1 */
            __IOM uint32_t EVEN2               : 1; /*!< [2..2] Event enable on EXTI line 2 - PA2~PH2 */
            __IOM uint32_t EVEN3               : 1; /*!< [3..3] Event enable on EXTI line 3 - PA3~PH3 */
            __IOM uint32_t EVEN4               : 1; /*!< [4..4] Event enable on EXTI line 4 - PA4~PH4 */
            __IOM uint32_t EVEN5               : 1; /*!< [5..5] Event enable on EXTI line 5 - PA5~PH5 */
            __IOM uint32_t EVEN6               : 1; /*!< [6..6] Event enable on EXTI line 6 - PA6~PH6 */
            __IOM uint32_t EVEN7               : 1; /*!< [7..7] Event enable on EXTI line 7 - PA7~PH7 */
            __IOM uint32_t EVEN8               : 1; /*!< [8..8] Event enable on EXTI line 8 - PA8~PG8 */
            __IOM uint32_t EVEN9               : 1; /*!< [9..9] Event enable on EXTI line 9 - PA9~PG9 */
            __IOM uint32_t EVEN10              : 1; /*!< [10..10] Event enable on EXTI line 10 - PA10~PG10 */
            __IOM uint32_t EVEN11              : 1; /*!< [11..11] Event enable on EXTI line 11 - PA11~PG11 */
            __IOM uint32_t EVEN12              : 1; /*!< [12..12] Event enable on EXTI line 12 - PA12~PG12 */
            __IOM uint32_t EVEN13              : 1; /*!< [13..13] Event enable on EXTI line 13 - PA13~PG13 */
            __IOM uint32_t EVEN14              : 1; /*!< [14..14] Event enable on EXTI line 14 - PA14~PG14 */
            __IOM uint32_t EVEN15              : 1; /*!< [15..15] Event enable on EXTI line 15 - PA15~PG15 */
            __IOM uint32_t EVEN16              : 1; /*!< [16..16] Event enable on EXTI line 16 - LVD */
            __IOM uint32_t EVEN17              : 1; /*!< [17..17] Event enable on EXTI line 17 - RTC/XTL stop */
            __IOM uint32_t EVEN18              : 1; /*!< [18..18] Event enable on EXTI line 18 - IWDT */
            __IOM uint32_t EVEN19              : 1; /*!< [19..19] Event enable on EXTI line 19 - COMP */
            __IOM uint32_t EVEN20              : 1; /*!< [20..20] Event enable on EXTI line 20 - USB_Wakeup */
            __IOM uint32_t EVEN21              : 1; /*!< [21..21] Event enable on EXTI line 21 - LPTIM1_Wakeup */
            __IOM uint32_t EVEN22              : 1; /*!< [22..22] Event enable on EXTI line 22 - LPUART1_Wakeup */
            __IOM uint32_t EVEN23              : 1; /*!< [23..23] Event enable on EXTI line 23 - LPUART2_Wakeup */
            __IOM uint32_t EVEN24              : 1; /*!< [24..24] Event enable on EXTI line 24 - ETH_Wakeup */
            __IM  uint32_t          : 7;
        } EENR_f;
    };
    union {
        __IOM uint32_t RTENR               ; /*!< Offset: 0x0008 (read-write) Rising trigger enable register - Enable/disable rising edge trigger on EXTI lines 0~24 */
        struct {
            __IOM uint32_t RTEN0               : 1; /*!< [0..0] Rising trigger enable on EXTI line 0 - PA0~PH0 */
            __IOM uint32_t RTEN1               : 1; /*!< [1..1] Rising trigger enable on EXTI line 1 - PA1~PH1 */
            __IOM uint32_t RTEN2               : 1; /*!< [2..2] Rising trigger enable on EXTI line 2 - PA2~PH2 */
            __IOM uint32_t RTEN3               : 1; /*!< [3..3] Rising trigger enable on EXTI line 3 - PA3~PH3 */
            __IOM uint32_t RTEN4               : 1; /*!< [4..4] Rising trigger enable on EXTI line 4 - PA4~PH4 */
            __IOM uint32_t RTEN5               : 1; /*!< [5..5] Rising trigger enable on EXTI line 5 - PA5~PH5 */
            __IOM uint32_t RTEN6               : 1; /*!< [6..6] Rising trigger enable on EXTI line 6 - PA6~PH6 */
            __IOM uint32_t RTEN7               : 1; /*!< [7..7] Rising trigger enable on EXTI line 7 - PA7~PH7 */
            __IOM uint32_t RTEN8               : 1; /*!< [8..8] Rising trigger enable on EXTI line 8 - PA8~PG8 */
            __IOM uint32_t RTEN9               : 1; /*!< [9..9] Rising trigger enable on EXTI line 9 - PA9~PG9 */
            __IOM uint32_t RTEN10              : 1; /*!< [10..10] Rising trigger enable on EXTI line 10 - PA10~PG10 */
            __IOM uint32_t RTEN11              : 1; /*!< [11..11] Rising trigger enable on EXTI line 11 - PA11~PG11 */
            __IOM uint32_t RTEN12              : 1; /*!< [12..12] Rising trigger enable on EXTI line 12 - PA12~PG12 */
            __IOM uint32_t RTEN13              : 1; /*!< [13..13] Rising trigger enable on EXTI line 13 - PA13~PG13 */
            __IOM uint32_t RTEN14              : 1; /*!< [14..14] Rising trigger enable on EXTI line 14 - PA14~PG14 */
            __IOM uint32_t RTEN15              : 1; /*!< [15..15] Rising trigger enable on EXTI line 15 - PA15~PG15 */
            __IOM uint32_t RTEN16              : 1; /*!< [16..16] Rising trigger enable on EXTI line 16 - LVD */
            __IOM uint32_t RTEN17              : 1; /*!< [17..17] Rising trigger enable on EXTI line 17 - RTC/XTL stop */
            __IOM uint32_t RTEN18              : 1; /*!< [18..18] Rising trigger enable on EXTI line 18 - IWDT */
            __IOM uint32_t RTEN19              : 1; /*!< [19..19] Rising trigger enable on EXTI line 19 - COMP */
            __IOM uint32_t RTEN20              : 1; /*!< [20..20] Rising trigger enable on EXTI line 20 - USB_Wakeup */
            __IOM uint32_t RTEN21              : 1; /*!< [21..21] Rising trigger enable on EXTI line 21 - LPTIM1_Wakeup */
            __IOM uint32_t RTEN22              : 1; /*!< [22..22] Rising trigger enable on EXTI line 22 - LPUART1_Wakeup */
            __IOM uint32_t RTEN23              : 1; /*!< [23..23] Rising trigger enable on EXTI line 23 - LPUART2_Wakeup */
            __IOM uint32_t RTEN24              : 1; /*!< [24..24] Rising trigger enable on EXTI line 24 - ETH_Wakeup */
            __IM  uint32_t          : 7;
        } RTENR_f;
    };
    union {
        __IOM uint32_t FTENR               ; /*!< Offset: 0x000C (read-write) Falling trigger enable register - Enable/disable falling edge trigger on EXTI lines 0~24 */
        struct {
            __IOM uint32_t FTEN0               : 1; /*!< [0..0] Falling trigger enable on EXTI line 0 - PA0~PH0 */
            __IOM uint32_t FTEN1               : 1; /*!< [1..1] Falling trigger enable on EXTI line 1 - PA1~PH1 */
            __IOM uint32_t FTEN2               : 1; /*!< [2..2] Falling trigger enable on EXTI line 2 - PA2~PH2 */
            __IOM uint32_t FTEN3               : 1; /*!< [3..3] Falling trigger enable on EXTI line 3 - PA3~PH3 */
            __IOM uint32_t FTEN4               : 1; /*!< [4..4] Falling trigger enable on EXTI line 4 - PA4~PH4 */
            __IOM uint32_t FTEN5               : 1; /*!< [5..5] Falling trigger enable on EXTI line 5 - PA5~PH5 */
            __IOM uint32_t FTEN6               : 1; /*!< [6..6] Falling trigger enable on EXTI line 6 - PA6~PH6 */
            __IOM uint32_t FTEN7               : 1; /*!< [7..7] Falling trigger enable on EXTI line 7 - PA7~PH7 */
            __IOM uint32_t FTEN8               : 1; /*!< [8..8] Falling trigger enable on EXTI line 8 - PA8~PG8 */
            __IOM uint32_t FTEN9               : 1; /*!< [9..9] Falling trigger enable on EXTI line 9 - PA9~PG9 */
            __IOM uint32_t FTEN10              : 1; /*!< [10..10] Falling trigger enable on EXTI line 10 - PA10~PG10 */
            __IOM uint32_t FTEN11              : 1; /*!< [11..11] Falling trigger enable on EXTI line 11 - PA11~PG11 */
            __IOM uint32_t FTEN12              : 1; /*!< [12..12] Falling trigger enable on EXTI line 12 - PA12~PG12 */
            __IOM uint32_t FTEN13              : 1; /*!< [13..13] Falling trigger enable on EXTI line 13 - PA13~PG13 */
            __IOM uint32_t FTEN14              : 1; /*!< [14..14] Falling trigger enable on EXTI line 14 - PA14~PG14 */
            __IOM uint32_t FTEN15              : 1; /*!< [15..15] Falling trigger enable on EXTI line 15 - PA15~PG15 */
            __IOM uint32_t FTEN16              : 1; /*!< [16..16] Falling trigger enable on EXTI line 16 - LVD */
            __IOM uint32_t FTEN17              : 1; /*!< [17..17] Falling trigger enable on EXTI line 17 - RTC/XTL stop */
            __IOM uint32_t FTEN18              : 1; /*!< [18..18] Falling trigger enable on EXTI line 18 - IWDT */
            __IOM uint32_t FTEN19              : 1; /*!< [19..19] Falling trigger enable on EXTI line 19 - COMP */
            __IOM uint32_t FTEN20              : 1; /*!< [20..20] Falling trigger enable on EXTI line 20 - USB_Wakeup */
            __IOM uint32_t FTEN21              : 1; /*!< [21..21] Falling trigger enable on EXTI line 21 - LPTIM1_Wakeup */
            __IOM uint32_t FTEN22              : 1; /*!< [22..22] Falling trigger enable on EXTI line 22 - LPUART1_Wakeup */
            __IOM uint32_t FTEN23              : 1; /*!< [23..23] Falling trigger enable on EXTI line 23 - LPUART2_Wakeup */
            __IOM uint32_t FTEN24              : 1; /*!< [24..24] Falling trigger enable on EXTI line 24 - ETH_Wakeup */
            __IM  uint32_t          : 7;
        } FTENR_f;
    };
    union {
        __OM  uint32_t SWIER               ; /*!< Offset: 0x0010 (write-only) Software interrupt event register - Set pending bits via software on EXTI lines 0~24 */
        struct {
            __OM  uint32_t SWIE0               : 1; /*!< [0..0] Software interrupt on EXTI line 0 - Write 1 to set pending bit */
            __OM  uint32_t SWIE1               : 1; /*!< [1..1] Software interrupt on EXTI line 1 - Write 1 to set pending bit */
            __OM  uint32_t SWIE2               : 1; /*!< [2..2] Software interrupt on EXTI line 2 - Write 1 to set pending bit */
            __OM  uint32_t SWIE3               : 1; /*!< [3..3] Software interrupt on EXTI line 3 - Write 1 to set pending bit */
            __OM  uint32_t SWIE4               : 1; /*!< [4..4] Software interrupt on EXTI line 4 - Write 1 to set pending bit */
            __OM  uint32_t SWIE5               : 1; /*!< [5..5] Software interrupt on EXTI line 5 - Write 1 to set pending bit */
            __OM  uint32_t SWIE6               : 1; /*!< [6..6] Software interrupt on EXTI line 6 - Write 1 to set pending bit */
            __OM  uint32_t SWIE7               : 1; /*!< [7..7] Software interrupt on EXTI line 7 - Write 1 to set pending bit */
            __OM  uint32_t SWIE8               : 1; /*!< [8..8] Software interrupt on EXTI line 8 - Write 1 to set pending bit */
            __OM  uint32_t SWIE9               : 1; /*!< [9..9] Software interrupt on EXTI line 9 - Write 1 to set pending bit */
            __OM  uint32_t SWIE10              : 1; /*!< [10..10] Software interrupt on EXTI line 10 - Write 1 to set pending bit */
            __OM  uint32_t SWIE11              : 1; /*!< [11..11] Software interrupt on EXTI line 11 - Write 1 to set pending bit */
            __OM  uint32_t SWIE12              : 1; /*!< [12..12] Software interrupt on EXTI line 12 - Write 1 to set pending bit */
            __OM  uint32_t SWIE13              : 1; /*!< [13..13] Software interrupt on EXTI line 13 - Write 1 to set pending bit */
            __OM  uint32_t SWIE14              : 1; /*!< [14..14] Software interrupt on EXTI line 14 - Write 1 to set pending bit */
            __OM  uint32_t SWIE15              : 1; /*!< [15..15] Software interrupt on EXTI line 15 - Write 1 to set pending bit */
            __OM  uint32_t SWIE16              : 1; /*!< [16..16] Software interrupt on EXTI line 16 - Write 1 to set pending bit */
            __OM  uint32_t SWIE17              : 1; /*!< [17..17] Software interrupt on EXTI line 17 - Write 1 to set pending bit */
            __OM  uint32_t SWIE18              : 1; /*!< [18..18] Software interrupt on EXTI line 18 - Write 1 to set pending bit */
            __OM  uint32_t SWIE19              : 1; /*!< [19..19] Software interrupt on EXTI line 19 - Write 1 to set pending bit */
            __OM  uint32_t SWIE20              : 1; /*!< [20..20] Software interrupt on EXTI line 20 - Write 1 to set pending bit */
            __OM  uint32_t SWIE21              : 1; /*!< [21..21] Software interrupt on EXTI line 21 - Write 1 to set pending bit */
            __OM  uint32_t SWIE22              : 1; /*!< [22..22] Software interrupt on EXTI line 22 - Write 1 to set pending bit */
            __OM  uint32_t SWIE23              : 1; /*!< [23..23] Software interrupt on EXTI line 23 - Write 1 to set pending bit */
            __OM  uint32_t SWIE24              : 1; /*!< [24..24] Software interrupt on EXTI line 24 - Write 1 to set pending bit */
            __IM  uint32_t          : 7;
        } SWIER_f;
    };
    union {
        __IOM uint32_t PDR                 ; /*!< Offset: 0x0014 (read-write) Pending register - Pending bits for EXTI lines 0~24, write 1 to clear */
        struct {
            __IOM uint32_t PD0                 : 1; /*!< [0..0] Pending on EXTI line 0 - Set by hardware on trigger, write 1 to clear */
            __IOM uint32_t PD1                 : 1; /*!< [1..1] Pending on EXTI line 1 - Set by hardware on trigger, write 1 to clear */
            __IOM uint32_t PD2                 : 1; /*!< [2..2] Pending on EXTI line 2 - Set by hardware on trigger, write 1 to clear */
            __IOM uint32_t PD3                 : 1; /*!< [3..3] Pending on EXTI line 3 - Set by hardware on trigger, write 1 to clear */
            __IOM uint32_t PD4                 : 1; /*!< [4..4] Pending on EXTI line 4 - Set by hardware on trigger, write 1 to clear */
            __IOM uint32_t PD5                 : 1; /*!< [5..5] Pending on EXTI line 5 - Set by hardware on trigger, write 1 to clear */
            __IOM uint32_t PD6                 : 1; /*!< [6..6] Pending on EXTI line 6 - Set by hardware on trigger, write 1 to clear */
            __IOM uint32_t PD7                 : 1; /*!< [7..7] Pending on EXTI line 7 - Set by hardware on trigger, write 1 to clear */
            __IOM uint32_t PD8                 : 1; /*!< [8..8] Pending on EXTI line 8 - Set by hardware on trigger, write 1 to clear */
            __IOM uint32_t PD9                 : 1; /*!< [9..9] Pending on EXTI line 9 - Set by hardware on trigger, write 1 to clear */
            __IOM uint32_t PD10                : 1; /*!< [10..10] Pending on EXTI line 10 - Set by hardware on trigger, write 1 to clear */
            __IOM uint32_t PD11                : 1; /*!< [11..11] Pending on EXTI line 11 - Set by hardware on trigger, write 1 to clear */
            __IOM uint32_t PD12                : 1; /*!< [12..12] Pending on EXTI line 12 - Set by hardware on trigger, write 1 to clear */
            __IOM uint32_t PD13                : 1; /*!< [13..13] Pending on EXTI line 13 - Set by hardware on trigger, write 1 to clear */
            __IOM uint32_t PD14                : 1; /*!< [14..14] Pending on EXTI line 14 - Set by hardware on trigger, write 1 to clear */
            __IOM uint32_t PD15                : 1; /*!< [15..15] Pending on EXTI line 15 - Set by hardware on trigger, write 1 to clear */
            __IOM uint32_t PD16                : 1; /*!< [16..16] Pending on EXTI line 16 - Set by hardware on trigger, write 1 to clear */
            __IOM uint32_t PD17                : 1; /*!< [17..17] Pending on EXTI line 17 - Set by hardware on trigger, write 1 to clear */
            __IOM uint32_t PD18                : 1; /*!< [18..18] Pending on EXTI line 18 - Set by hardware on trigger, write 1 to clear */
            __IOM uint32_t PD19                : 1; /*!< [19..19] Pending on EXTI line 19 - Set by hardware on trigger, write 1 to clear */
            __IOM uint32_t PD20                : 1; /*!< [20..20] Pending on EXTI line 20 - Set by hardware on trigger, write 1 to clear */
            __IOM uint32_t PD21                : 1; /*!< [21..21] Pending on EXTI line 21 - Set by hardware on trigger, write 1 to clear */
            __IOM uint32_t PD22                : 1; /*!< [22..22] Pending on EXTI line 22 - Set by hardware on trigger, write 1 to clear */
            __IOM uint32_t PD23                : 1; /*!< [23..23] Pending on EXTI line 23 - Set by hardware on trigger, write 1 to clear */
            __IOM uint32_t PD24                : 1; /*!< [24..24] Pending on EXTI line 24 - Set by hardware on trigger, write 1 to clear */
            __IM  uint32_t          : 7;
        } PDR_f;
    };
    union {
        __IOM uint32_t EXTICR1             ; /*!< Offset: 0x0018 (read-write) External interrupt configuration register 1 - GPIO port selection for EXTI lines 0~7 */
        struct {
            __IOM uint32_t EXTI0               : 4; /*!< [3..0] EXTI line 0 GPIO port selection: 0000=PA0, 0001=PB0, 0010=PC0, 0011=PD0, 0100=PE0, 0101=PF0, 0110=PG0, 0111=PH0 */
            __IOM uint32_t EXTI1               : 4; /*!< [7..4] EXTI line 1 GPIO port selection: 0000=PA1, 0001=PB1, 0010=PC1, 0011=PD1, 0100=PE1, 0101=PF1, 0110=PG1, 0111=PH1 */
            __IOM uint32_t EXTI2               : 4; /*!< [11..8] EXTI line 2 GPIO port selection: 0000=PA2, 0001=PB2, 0010=PC2, 0011=PD2, 0100=PE2, 0101=PF2, 0110=PG2, 0111=PH2 */
            __IOM uint32_t EXTI3               : 4; /*!< [15..12] EXTI line 3 GPIO port selection: 0000=PA3, 0001=PB3, 0010=PC3, 0011=PD3, 0100=PE3, 0101=PF3, 0110=PG3, 0111=PH3 */
            __IOM uint32_t EXTI4               : 4; /*!< [19..16] EXTI line 4 GPIO port selection: 0000=PA4, 0001=PB4, 0010=PC4, 0011=PD4, 0100=PE4, 0101=PF4, 0110=PG4, 0111=PH4 */
            __IOM uint32_t EXTI5               : 4; /*!< [23..20] EXTI line 5 GPIO port selection: 0000=PA5, 0001=PB5, 0010=PC5, 0011=PD5, 0100=PE5, 0101=PF5, 0110=PG5, 0111=PH5 */
            __IOM uint32_t EXTI6               : 4; /*!< [27..24] EXTI line 6 GPIO port selection: 0000=PA6, 0001=PB6, 0010=PC6, 0011=PD6, 0100=PE6, 0101=PF6, 0110=PG6, 0111=PH6 */
            __IOM uint32_t EXTI7               : 4; /*!< [31..28] EXTI line 7 GPIO port selection: 0000=PA7, 0001=PB7, 0010=PC7, 0011=PD7, 0100=PE7, 0101=PF7, 0110=PG7, 0111=PH7 */
        } EXTICR1_f;
    };
    union {
        __IOM uint32_t EXTICR2             ; /*!< Offset: 0x001C (read-write) External interrupt configuration register 2 - GPIO port selection for EXTI lines 8~15 */
        struct {
            __IOM uint32_t EXTI8               : 4; /*!< [3..0] EXTI line 8 GPIO port selection: 0000=PA8, 0001=PB8, 0010=PC8, 0011=PD8, 0100=PE8, 0101=PF8, 0110=PG8 */
            __IOM uint32_t EXTI9               : 4; /*!< [7..4] EXTI line 9 GPIO port selection: 0000=PA9, 0001=PB9, 0010=PC9, 0011=PD9, 0100=PE9, 0101=PF9, 0110=PG9 */
            __IOM uint32_t EXTI10              : 4; /*!< [11..8] EXTI line 10 GPIO port selection: 0000=PA10, 0001=PB10, 0010=PC10, 0011=PD10, 0100=PE10, 0101=PF10, 0110=PG10 */
            __IOM uint32_t EXTI11              : 4; /*!< [15..12] EXTI line 11 GPIO port selection: 0000=PA11, 0001=PB11, 0010=PC11, 0011=PD11, 0100=PE11, 0101=PF11, 0110=PG11 */
            __IOM uint32_t EXTI12              : 4; /*!< [19..16] EXTI line 12 GPIO port selection: 0000=PA12, 0001=PB12, 0010=PC12, 0011=PD12, 0100=PE12, 0101=PF12, 0110=PG12 */
            __IOM uint32_t EXTI13              : 4; /*!< [23..20] EXTI line 13 GPIO port selection: 0000=PA13, 0001=PB13, 0010=PC13, 0011=PD13, 0100=PE13, 0101=PF13, 0110=PG13 */
            __IOM uint32_t EXTI14              : 4; /*!< [27..24] EXTI line 14 GPIO port selection: 0000=PA14, 0001=PB14, 0010=PC14, 0011=PD14, 0100=PE14, 0101=PF14, 0110=PG14 */
            __IOM uint32_t EXTI15              : 4; /*!< [31..28] EXTI line 15 GPIO port selection: 0000=PA15, 0001=PB15, 0010=PC15, 0011=PD15, 0100=PE15, 0101=PF15, 0110=PG15 */
        } EXTICR2_f;
    };
} EXTI_TypeDef;

/**
 * @brief DMAC - DMAC Register Structure
 */
typedef struct {
    union {
        __IM  uint32_t INTSTATUS           ; /*!< Offset: 0x00 (read-only) Interrupt status - Interrupt status register */
        struct {
            __IM  uint32_t INTSTATUS           : 8; /*!< [7..0] Interrupt status - Interrupt status register */
            __IM  uint32_t          : 24;
        } INTSTATUS_f;
    };
    union {
        __IM  uint32_t INTTCSTATUS         ; /*!< Offset: 0x04 (read-only) Interrupt transfer complete status - Transfer complete status */
        struct {
            __IM  uint32_t INTTCSTATUS         : 8; /*!< [7..0] Interrupt transfer complete status - Transfer complete status */
            __IM  uint32_t INTHFTCSTATUS       : 8; /*!< [15..8] Interrupt half transfer complete status - Half transfer status */
            __IM  uint32_t          : 16;
        } INTTCSTATUS_f;
    };
    union {
        __OM  uint32_t INTTCCLR            ; /*!< Offset: 0x08 (write-only) Interrupt transfer complete clear - Clears transfer complete flag */
        struct {
            __OM  uint32_t INTTCCLR            : 8; /*!< [7..0] Interrupt transfer complete clear - Clears transfer complete flag */
            __OM  uint32_t INTHFTCCLR          : 8; /*!< [15..8] Interrupt half transfer complete clear - Clears half transfer flag */
            __IM  uint32_t          : 16;
        } INTTCCLR_f;
    };
    union {
        __IM  uint32_t INTERRSTATUS        ; /*!< Offset: 0x0C (read-only) Interrupt error status - Interrupt error status */
        struct {
            __IM  uint32_t INTERRSTATUS        : 8; /*!< [7..0] Interrupt error status - Interrupt error status */
            __IM  uint32_t          : 24;
        } INTERRSTATUS_f;
    };
    union {
        __OM  uint32_t INTERRCLR           ; /*!< Offset: 0x10 (write-only) Interrupt error clear - Clears interrupt error flag */
        struct {
            __OM  uint32_t INTERRCLR           : 8; /*!< [7..0] Interrupt error clear - Clears interrupt error flag */
            __IM  uint32_t          : 24;
        } INTERRCLR_f;
    };
    union {
        __IM  uint32_t RAWINTTCSTATUS      ; /*!< Offset: 0x14 (read-only) Raw interrupt transfer complete status - Raw transfer complete status */
        struct {
            __IM  uint32_t RAWINTTCSTATUS      : 8; /*!< [7..0] Raw interrupt transfer complete status - Raw transfer complete status */
            __IM  uint32_t RAWINTHFTCSTATUS    : 8; /*!< [15..8] Raw interrupt half transfer complete status - Raw half transfer status */
            __IM  uint32_t          : 16;
        } RAWINTTCSTATUS_f;
    };
    union {
        __IM  uint32_t RAWINTERRSTATUS     ; /*!< Offset: 0x18 (read-only) Raw interrupt error status - Raw error interrupt status register */
        struct {
            __IM  uint32_t RAWINTERRSTATUS     : 8; /*!< [7..0] Raw interrupt error status - Raw error interrupt status register */
            __IM  uint32_t          : 24;
        } RAWINTERRSTATUS_f;
    };
    union {
        __IM  uint32_t ENCHSTATUS          ; /*!< Offset: 0x1C (read-only) Enable channel status - Channel enable status */
        struct {
            __IM  uint32_t ENCHSTAT            : 8; /*!< [7..0] Enable channel status - Channel enable status */
            __IM  uint32_t          : 24;
        } ENCHSTATUS_f;
    };
    __IO uint32_t RESERVED0[4];
    union {
        __IOM uint32_t CONFIG              ; /*!< Offset: 0x30 (read-write) Configuration register - Peripheral configuration */
        struct {
            __IOM uint32_t EN                  : 1; /*!< [0..0] Enable - Enables the module or function */
            __IOM uint32_t M1ENDIAN            : 1; /*!< [1..1] Memory 1 endianness - Memory 1 endianness selection */
            __IOM uint32_t M2ENDIAN            : 1; /*!< [2..2] Memory 2 endianness - Memory 2 endianness selection */
            __IM  uint32_t          : 29;
        } CONFIG_f;
    };
    union {
        __IOM uint32_t SYNCLO              ; /*!< Offset: 0x34 (read-write) SYNCLO - SYNCLO value */
        struct {
            __IOM uint32_t SYNCLO              : 32; /*!< [31..0] SYNCLO - SYNCLO value */
        } SYNCLO_f;
    };
    union {
        __IOM uint32_t SYNCHI              ; /*!< Offset: 0x38 (read-write) SYNCHI - SYNCHI value */
        struct {
            __IOM uint32_t SYNCHI              : 32; /*!< [31..0] SYNCHI - SYNCHI value */
        } SYNCHI_f;
    };
    __IO uint32_t RESERVED1[49];
    union {
        __IOM uint32_t C0SRCADDR           ; /*!< Offset: 0x100 (read-write) Channel 0 source address - DMA channel 0 source address */
        struct {
            __IOM uint32_t CXSRCADDR           : 32; /*!< [31..0] CRC source address - Source address for CRC calculation */
        } C0SRCADDR_f;
    };
    union {
        __IOM uint32_t C0DESTADDR          ; /*!< Offset: 0x104 (read-write) Channel 0 destination address - DMA channel 0 destination address */
        struct {
            __IOM uint32_t CXDESTADDR          : 32; /*!< [31..0] CRC destination address - Destination address for CRC calculation */
        } C0DESTADDR_f;
    };
    union {
        __IOM uint32_t C0LLI               ; /*!< Offset: 0x108 (read-write) Channel 0 LLI - DMA channel 0 linked list item pointer */
        struct {
            __IOM uint32_t LM                  : 1; /*!< [0..0] Loop mode - Enables circular buffer mode for DMA */
            __IM  uint32_t          : 1;
            __IOM uint32_t LLI                 : 30; /*!< [31..2] Linked list item - Next descriptor address in linked list */
        } C0LLI_f;
    };
    union {
        __IOM uint32_t C0CTRL              ; /*!< Offset: 0x10c (read-write) Channel 0 control - DMA channel 0 control register */
        struct {
            __IOM uint32_t TRANSFERSIZE        : 16; /*!< [15..0] Transfer size - Number of data items to transfer */
            __IOM uint32_t SBSIZE              : 3; /*!< [18..16] Source burst size - Source burst transfer size configuration */
            __IOM uint32_t DBSIZE              : 3; /*!< [21..19] Destination burst size - Destination burst transfer size configuration */
            __IOM uint32_t SWIDTH              : 2; /*!< [23..22] Source data width - Source data width in bytes */
            __IOM uint32_t DWIDTH              : 2; /*!< [25..24] Destination data width - Destination data width in bytes */
            __IOM uint32_t SIORSD              : 2; /*!< [27..26] Source I/O or reduced speed - Source peripheral I/O mode or reduced speed */
            __IOM uint32_t DIORDD              : 2; /*!< [29..28] Destination I/O or reduced speed - Destination peripheral I/O mode or reduced speed */
            __IM  uint32_t          : 1;
            __IOM uint32_t RITEN               : 1; /*!< [31..31] Repeat interval timer enable - Enables cyclic timer for transfers */
        } C0CTRL_f;
    };
    union {
        __IOM uint32_t C0CONFIG            ; /*!< Offset: 0x110 (read-write) Channel 0 configuration - DMA channel 0 configuration register */
        struct {
            __IOM uint32_t EN                  : 1; /*!< [0..0] Enable - Enables the module or function */
            __IOM uint32_t FLOWCTRL            : 3; /*!< [3..1] Flow control - Selects flow controller for the transfer */
            __IOM uint32_t IE                  : 1; /*!< [4..4] Interrupt enable register - Enables or disables various interrupt sources */
            __IOM uint32_t ITC                 : 1; /*!< [5..5] Interrupt trigger configuration - Configures interrupt trigger conditions */
            __IOM uint32_t IHFTC               : 1; /*!< [6..6] Interrupt hold-off timer configuration - Configures interrupt hold-off period */
            __IOM uint32_t ACTIVE              : 1; /*!< [7..7] Active flag - Indicates the channel or module is active */
            __IOM uint32_t HALT                : 1; /*!< [8..8] Halt request - Requests halting of current transfer */
            __IOM uint32_t S                   : 1; /*!< [9..9] Source - Source selection for multiplexed inputs */
            __IOM uint32_t D                   : 1; /*!< [10..10] Destination - Destination selection for multiplexed outputs */
            __IOM uint32_t LOCK                : 1; /*!< [11..11] Lock register - Locks timer configuration bits against modification */
            __IM  uint32_t          : 4;
            __IOM uint32_t DESTPERIPH          : 8; /*!< [23..16] Destination peripheral - Selects destination peripheral for transfer */
            __IOM uint32_t SRCPERIPH           : 8; /*!< [31..24] Source peripheral - Selects source peripheral for transfer */
        } C0CONFIG_f;
    };
    __IO uint32_t RESERVED2[3];
    union {
        __IOM uint32_t C1SRCADDR           ; /*!< Offset: 0x120 (read-write) Channel 1 source address - DMA channel 1 source address */
        struct {
            __IOM uint32_t CXSRCADDR           : 32; /*!< [31..0] CRC source address - Source address for CRC calculation */
        } C1SRCADDR_f;
    };
    union {
        __IOM uint32_t C1DESTADDR          ; /*!< Offset: 0x124 (read-write) Channel 1 destination address - DMA channel 1 destination address */
        struct {
            __IOM uint32_t CXDESTADDR          : 32; /*!< [31..0] CRC destination address - Destination address for CRC calculation */
        } C1DESTADDR_f;
    };
    union {
        __IOM uint32_t C1LLI               ; /*!< Offset: 0x128 (read-write) Channel 1 LLI - DMA channel 1 linked list item pointer */
        struct {
            __IOM uint32_t LM                  : 1; /*!< [0..0] Loop mode - Enables circular buffer mode for DMA */
            __IM  uint32_t          : 1;
            __IOM uint32_t LLI                 : 30; /*!< [31..2] Linked list item - Next descriptor address in linked list */
        } C1LLI_f;
    };
    union {
        __IOM uint32_t C1CTRL              ; /*!< Offset: 0x12c (read-write) Channel 1 control - DMA channel 1 control register */
        struct {
            __IOM uint32_t TRANSFERSIZE        : 16; /*!< [15..0] Transfer size - Number of data items to transfer */
            __IOM uint32_t SBSIZE              : 3; /*!< [18..16] Source burst size - Source burst transfer size configuration */
            __IOM uint32_t DBSIZE              : 3; /*!< [21..19] Destination burst size - Destination burst transfer size configuration */
            __IOM uint32_t SWIDTH              : 2; /*!< [23..22] Source data width - Source data width in bytes */
            __IOM uint32_t DWIDTH              : 2; /*!< [25..24] Destination data width - Destination data width in bytes */
            __IOM uint32_t SIORSD              : 2; /*!< [27..26] Source I/O or reduced speed - Source peripheral I/O mode or reduced speed */
            __IOM uint32_t DIORDD              : 2; /*!< [29..28] Destination I/O or reduced speed - Destination peripheral I/O mode or reduced speed */
            __IM  uint32_t          : 1;
            __IOM uint32_t RITEN               : 1; /*!< [31..31] Repeat interval timer enable - Enables cyclic timer for transfers */
        } C1CTRL_f;
    };
    union {
        __IOM uint32_t C1CONFIG            ; /*!< Offset: 0x130 (read-write) Channel 1 configuration - DMA channel 1 configuration register */
        struct {
            __IOM uint32_t EN                  : 1; /*!< [0..0] Enable - Enables the module or function */
            __IOM uint32_t FLOWCTRL            : 3; /*!< [3..1] Flow control - Selects flow controller for the transfer */
            __IOM uint32_t IE                  : 1; /*!< [4..4] Interrupt enable register - Enables or disables various interrupt sources */
            __IOM uint32_t ITC                 : 1; /*!< [5..5] Interrupt trigger configuration - Configures interrupt trigger conditions */
            __IOM uint32_t IHFTC               : 1; /*!< [6..6] Interrupt hold-off timer configuration - Configures interrupt hold-off period */
            __IOM uint32_t ACTIVE              : 1; /*!< [7..7] Active flag - Indicates the channel or module is active */
            __IOM uint32_t HALT                : 1; /*!< [8..8] Halt request - Requests halting of current transfer */
            __IOM uint32_t S                   : 1; /*!< [9..9] Source - Source selection for multiplexed inputs */
            __IOM uint32_t D                   : 1; /*!< [10..10] Destination - Destination selection for multiplexed outputs */
            __IOM uint32_t LOCK                : 1; /*!< [11..11] Lock register - Locks timer configuration bits against modification */
            __IM  uint32_t          : 4;
            __IOM uint32_t DESTPERIPH          : 8; /*!< [23..16] Destination peripheral - Selects destination peripheral for transfer */
            __IOM uint32_t SRCPERIPH           : 8; /*!< [31..24] Source peripheral - Selects source peripheral for transfer */
        } C1CONFIG_f;
    };
    __IO uint32_t RESERVED3[3];
    union {
        __IOM uint32_t C2SRCADDR           ; /*!< Offset: 0x140 (read-write) Channel 2 source address - DMA channel 2 source address */
        struct {
            __IOM uint32_t CXSRCADDR           : 32; /*!< [31..0] CRC source address - Source address for CRC calculation */
        } C2SRCADDR_f;
    };
    union {
        __IOM uint32_t C2DESTADDR          ; /*!< Offset: 0x144 (read-write) Channel 2 destination address - DMA channel 2 destination address */
        struct {
            __IOM uint32_t CXDESTADDR          : 32; /*!< [31..0] CRC destination address - Destination address for CRC calculation */
        } C2DESTADDR_f;
    };
    union {
        __IOM uint32_t C2LLI               ; /*!< Offset: 0x148 (read-write) Channel 2 LLI - DMA channel 2 linked list item pointer */
        struct {
            __IOM uint32_t LM                  : 1; /*!< [0..0] Loop mode - Enables circular buffer mode for DMA */
            __IM  uint32_t          : 1;
            __IOM uint32_t LLI                 : 30; /*!< [31..2] Linked list item - Next descriptor address in linked list */
        } C2LLI_f;
    };
    union {
        __IOM uint32_t C2CTRL              ; /*!< Offset: 0x14c (read-write) Channel 2 control - DMA channel 2 control register */
        struct {
            __IOM uint32_t TRANSFERSIZE        : 16; /*!< [15..0] Transfer size - Number of data items to transfer */
            __IOM uint32_t SBSIZE              : 3; /*!< [18..16] Source burst size - Source burst transfer size configuration */
            __IOM uint32_t DBSIZE              : 3; /*!< [21..19] Destination burst size - Destination burst transfer size configuration */
            __IOM uint32_t SWIDTH              : 2; /*!< [23..22] Source data width - Source data width in bytes */
            __IOM uint32_t DWIDTH              : 2; /*!< [25..24] Destination data width - Destination data width in bytes */
            __IOM uint32_t SIORSD              : 2; /*!< [27..26] Source I/O or reduced speed - Source peripheral I/O mode or reduced speed */
            __IOM uint32_t DIORDD              : 2; /*!< [29..28] Destination I/O or reduced speed - Destination peripheral I/O mode or reduced speed */
            __IM  uint32_t          : 1;
            __IOM uint32_t RITEN               : 1; /*!< [31..31] Repeat interval timer enable - Enables cyclic timer for transfers */
        } C2CTRL_f;
    };
    union {
        __IOM uint32_t C2CONFIG            ; /*!< Offset: 0x150 (read-write) Channel 2 configuration - DMA channel 2 configuration register */
        struct {
            __IOM uint32_t EN                  : 1; /*!< [0..0] Enable - Enables the module or function */
            __IOM uint32_t FLOWCTRL            : 3; /*!< [3..1] Flow control - Selects flow controller for the transfer */
            __IOM uint32_t IE                  : 1; /*!< [4..4] Interrupt enable register - Enables or disables various interrupt sources */
            __IOM uint32_t ITC                 : 1; /*!< [5..5] Interrupt trigger configuration - Configures interrupt trigger conditions */
            __IOM uint32_t IHFTC               : 1; /*!< [6..6] Interrupt hold-off timer configuration - Configures interrupt hold-off period */
            __IOM uint32_t ACTIVE              : 1; /*!< [7..7] Active flag - Indicates the channel or module is active */
            __IOM uint32_t HALT                : 1; /*!< [8..8] Halt request - Requests halting of current transfer */
            __IOM uint32_t S                   : 1; /*!< [9..9] Source - Source selection for multiplexed inputs */
            __IOM uint32_t D                   : 1; /*!< [10..10] Destination - Destination selection for multiplexed outputs */
            __IOM uint32_t LOCK                : 1; /*!< [11..11] Lock register - Locks timer configuration bits against modification */
            __IM  uint32_t          : 4;
            __IOM uint32_t DESTPERIPH          : 8; /*!< [23..16] Destination peripheral - Selects destination peripheral for transfer */
            __IOM uint32_t SRCPERIPH           : 8; /*!< [31..24] Source peripheral - Selects source peripheral for transfer */
        } C2CONFIG_f;
    };
    __IO uint32_t RESERVED4[3];
    union {
        __IOM uint32_t C3SRCADDR           ; /*!< Offset: 0x160 (read-write) Channel 3 source address - DMA channel 3 source address */
        struct {
            __IOM uint32_t CXSRCADDR           : 32; /*!< [31..0] CRC source address - Source address for CRC calculation */
        } C3SRCADDR_f;
    };
    union {
        __IOM uint32_t C3DESTADDR          ; /*!< Offset: 0x164 (read-write) Channel 3 destination address - DMA channel 3 destination address */
        struct {
            __IOM uint32_t CXDESTADDR          : 32; /*!< [31..0] CRC destination address - Destination address for CRC calculation */
        } C3DESTADDR_f;
    };
    union {
        __IOM uint32_t C3LLI               ; /*!< Offset: 0x168 (read-write) Channel 3 LLI - DMA channel 3 linked list item pointer */
        struct {
            __IOM uint32_t LM                  : 1; /*!< [0..0] Loop mode - Enables circular buffer mode for DMA */
            __IM  uint32_t          : 1;
            __IOM uint32_t LLI                 : 30; /*!< [31..2] Linked list item - Next descriptor address in linked list */
        } C3LLI_f;
    };
    union {
        __IOM uint32_t C3CTRL              ; /*!< Offset: 0x16c (read-write) Channel 3 control - DMA channel 3 control register */
        struct {
            __IOM uint32_t TRANSFERSIZE        : 16; /*!< [15..0] Transfer size - Number of data items to transfer */
            __IOM uint32_t SBSIZE              : 3; /*!< [18..16] Source burst size - Source burst transfer size configuration */
            __IOM uint32_t DBSIZE              : 3; /*!< [21..19] Destination burst size - Destination burst transfer size configuration */
            __IOM uint32_t SWIDTH              : 2; /*!< [23..22] Source data width - Source data width in bytes */
            __IOM uint32_t DWIDTH              : 2; /*!< [25..24] Destination data width - Destination data width in bytes */
            __IOM uint32_t SIORSD              : 2; /*!< [27..26] Source I/O or reduced speed - Source peripheral I/O mode or reduced speed */
            __IOM uint32_t DIORDD              : 2; /*!< [29..28] Destination I/O or reduced speed - Destination peripheral I/O mode or reduced speed */
            __IM  uint32_t          : 1;
            __IOM uint32_t RITEN               : 1; /*!< [31..31] Repeat interval timer enable - Enables cyclic timer for transfers */
        } C3CTRL_f;
    };
    union {
        __IOM uint32_t C3CONFIG            ; /*!< Offset: 0x170 (read-write) Channel 3 configuration - DMA channel 3 configuration register */
        struct {
            __IOM uint32_t EN                  : 1; /*!< [0..0] Enable - Enables the module or function */
            __IOM uint32_t FLOWCTRL            : 3; /*!< [3..1] Flow control - Selects flow controller for the transfer */
            __IOM uint32_t IE                  : 1; /*!< [4..4] Interrupt enable register - Enables or disables various interrupt sources */
            __IOM uint32_t ITC                 : 1; /*!< [5..5] Interrupt trigger configuration - Configures interrupt trigger conditions */
            __IOM uint32_t IHFTC               : 1; /*!< [6..6] Interrupt hold-off timer configuration - Configures interrupt hold-off period */
            __IOM uint32_t ACTIVE              : 1; /*!< [7..7] Active flag - Indicates the channel or module is active */
            __IOM uint32_t HALT                : 1; /*!< [8..8] Halt request - Requests halting of current transfer */
            __IOM uint32_t S                   : 1; /*!< [9..9] Source - Source selection for multiplexed inputs */
            __IOM uint32_t D                   : 1; /*!< [10..10] Destination - Destination selection for multiplexed outputs */
            __IOM uint32_t LOCK                : 1; /*!< [11..11] Lock register - Locks timer configuration bits against modification */
            __IM  uint32_t          : 4;
            __IOM uint32_t DESTPERIPH          : 8; /*!< [23..16] Destination peripheral - Selects destination peripheral for transfer */
            __IOM uint32_t SRCPERIPH           : 8; /*!< [31..24] Source peripheral - Selects source peripheral for transfer */
        } C3CONFIG_f;
    };
    __IO uint32_t RESERVED5[3];
    union {
        __IOM uint32_t C4SRCADDR           ; /*!< Offset: 0x180 (read-write) Channel 4 source address - DMA channel 4 source address */
        struct {
            __IOM uint32_t CXSRCADDR           : 32; /*!< [31..0] CRC source address - Source address for CRC calculation */
        } C4SRCADDR_f;
    };
    union {
        __IOM uint32_t C4DESTADDR          ; /*!< Offset: 0x184 (read-write) Channel 4 destination address - DMA channel 4 destination address */
        struct {
            __IOM uint32_t CXDESTADDR          : 32; /*!< [31..0] CRC destination address - Destination address for CRC calculation */
        } C4DESTADDR_f;
    };
    union {
        __IOM uint32_t C4LLI               ; /*!< Offset: 0x188 (read-write) Channel 4 LLI - DMA channel 4 linked list item pointer */
        struct {
            __IOM uint32_t LM                  : 1; /*!< [0..0] Loop mode - Enables circular buffer mode for DMA */
            __IM  uint32_t          : 1;
            __IOM uint32_t LLI                 : 30; /*!< [31..2] Linked list item - Next descriptor address in linked list */
        } C4LLI_f;
    };
    union {
        __IOM uint32_t C4CTRL              ; /*!< Offset: 0x18c (read-write) Channel 4 control - DMA channel 4 control register */
        struct {
            __IOM uint32_t TRANSFERSIZE        : 16; /*!< [15..0] Transfer size - Number of data items to transfer */
            __IOM uint32_t SBSIZE              : 3; /*!< [18..16] Source burst size - Source burst transfer size configuration */
            __IOM uint32_t DBSIZE              : 3; /*!< [21..19] Destination burst size - Destination burst transfer size configuration */
            __IOM uint32_t SWIDTH              : 2; /*!< [23..22] Source data width - Source data width in bytes */
            __IOM uint32_t DWIDTH              : 2; /*!< [25..24] Destination data width - Destination data width in bytes */
            __IOM uint32_t SIORSD              : 2; /*!< [27..26] Source I/O or reduced speed - Source peripheral I/O mode or reduced speed */
            __IOM uint32_t DIORDD              : 2; /*!< [29..28] Destination I/O or reduced speed - Destination peripheral I/O mode or reduced speed */
            __IM  uint32_t          : 1;
            __IOM uint32_t RITEN               : 1; /*!< [31..31] Repeat interval timer enable - Enables cyclic timer for transfers */
        } C4CTRL_f;
    };
    union {
        __IOM uint32_t C4CONFIG            ; /*!< Offset: 0x190 (read-write) Channel 4 configuration - DMA channel 4 configuration register */
        struct {
            __IOM uint32_t EN                  : 1; /*!< [0..0] Enable - Enables the module or function */
            __IOM uint32_t FLOWCTRL            : 3; /*!< [3..1] Flow control - Selects flow controller for the transfer */
            __IOM uint32_t IE                  : 1; /*!< [4..4] Interrupt enable register - Enables or disables various interrupt sources */
            __IOM uint32_t ITC                 : 1; /*!< [5..5] Interrupt trigger configuration - Configures interrupt trigger conditions */
            __IOM uint32_t IHFTC               : 1; /*!< [6..6] Interrupt hold-off timer configuration - Configures interrupt hold-off period */
            __IOM uint32_t ACTIVE              : 1; /*!< [7..7] Active flag - Indicates the channel or module is active */
            __IOM uint32_t HALT                : 1; /*!< [8..8] Halt request - Requests halting of current transfer */
            __IOM uint32_t S                   : 1; /*!< [9..9] Source - Source selection for multiplexed inputs */
            __IOM uint32_t D                   : 1; /*!< [10..10] Destination - Destination selection for multiplexed outputs */
            __IOM uint32_t LOCK                : 1; /*!< [11..11] Lock register - Locks timer configuration bits against modification */
            __IM  uint32_t          : 4;
            __IOM uint32_t DESTPERIPH          : 8; /*!< [23..16] Destination peripheral - Selects destination peripheral for transfer */
            __IOM uint32_t SRCPERIPH           : 8; /*!< [31..24] Source peripheral - Selects source peripheral for transfer */
        } C4CONFIG_f;
    };
    __IO uint32_t RESERVED6[3];
    union {
        __IOM uint32_t C5SRCADDR           ; /*!< Offset: 0x1a0 (read-write) Channel 5 source address - DMA channel 5 source address */
        struct {
            __IOM uint32_t CXSRCADDR           : 32; /*!< [31..0] CRC source address - Source address for CRC calculation */
        } C5SRCADDR_f;
    };
    union {
        __IOM uint32_t C5DESTADDR          ; /*!< Offset: 0x1a4 (read-write) Channel 5 destination address - DMA channel 5 destination address */
        struct {
            __IOM uint32_t CXDESTADDR          : 32; /*!< [31..0] CRC destination address - Destination address for CRC calculation */
        } C5DESTADDR_f;
    };
    union {
        __IOM uint32_t C5LLI               ; /*!< Offset: 0x1a8 (read-write) Channel 5 LLI - DMA channel 5 linked list item pointer */
        struct {
            __IOM uint32_t LM                  : 1; /*!< [0..0] Loop mode - Enables circular buffer mode for DMA */
            __IM  uint32_t          : 1;
            __IOM uint32_t LLI                 : 30; /*!< [31..2] Linked list item - Next descriptor address in linked list */
        } C5LLI_f;
    };
    union {
        __IOM uint32_t C5CTRL              ; /*!< Offset: 0x1ac (read-write) Channel 5 control - DMA channel 5 control register */
        struct {
            __IOM uint32_t TRANSFERSIZE        : 16; /*!< [15..0] Transfer size - Number of data items to transfer */
            __IOM uint32_t SBSIZE              : 3; /*!< [18..16] Source burst size - Source burst transfer size configuration */
            __IOM uint32_t DBSIZE              : 3; /*!< [21..19] Destination burst size - Destination burst transfer size configuration */
            __IOM uint32_t SWIDTH              : 2; /*!< [23..22] Source data width - Source data width in bytes */
            __IOM uint32_t DWIDTH              : 2; /*!< [25..24] Destination data width - Destination data width in bytes */
            __IOM uint32_t SIORSD              : 2; /*!< [27..26] Source I/O or reduced speed - Source peripheral I/O mode or reduced speed */
            __IOM uint32_t DIORDD              : 2; /*!< [29..28] Destination I/O or reduced speed - Destination peripheral I/O mode or reduced speed */
            __IM  uint32_t          : 1;
            __IOM uint32_t RITEN               : 1; /*!< [31..31] Repeat interval timer enable - Enables cyclic timer for transfers */
        } C5CTRL_f;
    };
    union {
        __IOM uint32_t C5CONFIG            ; /*!< Offset: 0x1b0 (read-write) Channel 5 configuration - DMA channel 5 configuration register */
        struct {
            __IOM uint32_t EN                  : 1; /*!< [0..0] Enable - Enables the module or function */
            __IOM uint32_t FLOWCTRL            : 3; /*!< [3..1] Flow control - Selects flow controller for the transfer */
            __IOM uint32_t IE                  : 1; /*!< [4..4] Interrupt enable register - Enables or disables various interrupt sources */
            __IOM uint32_t ITC                 : 1; /*!< [5..5] Interrupt trigger configuration - Configures interrupt trigger conditions */
            __IOM uint32_t IHFTC               : 1; /*!< [6..6] Interrupt hold-off timer configuration - Configures interrupt hold-off period */
            __IOM uint32_t ACTIVE              : 1; /*!< [7..7] Active flag - Indicates the channel or module is active */
            __IOM uint32_t HALT                : 1; /*!< [8..8] Halt request - Requests halting of current transfer */
            __IOM uint32_t S                   : 1; /*!< [9..9] Source - Source selection for multiplexed inputs */
            __IOM uint32_t D                   : 1; /*!< [10..10] Destination - Destination selection for multiplexed outputs */
            __IOM uint32_t LOCK                : 1; /*!< [11..11] Lock register - Locks timer configuration bits against modification */
            __IM  uint32_t          : 4;
            __IOM uint32_t DESTPERIPH          : 8; /*!< [23..16] Destination peripheral - Selects destination peripheral for transfer */
            __IOM uint32_t SRCPERIPH           : 8; /*!< [31..24] Source peripheral - Selects source peripheral for transfer */
        } C5CONFIG_f;
    };
    __IO uint32_t RESERVED7[3];
    union {
        __IOM uint32_t C6SRCADDR           ; /*!< Offset: 0x1c0 (read-write) Channel 6 source address - DMA channel 6 source address */
        struct {
            __IOM uint32_t CXSRCADDR           : 32; /*!< [31..0] CRC source address - Source address for CRC calculation */
        } C6SRCADDR_f;
    };
    union {
        __IOM uint32_t C6DESTADDR          ; /*!< Offset: 0x1c4 (read-write) Channel 6 destination address - DMA channel 6 destination address */
        struct {
            __IOM uint32_t CXDESTADDR          : 32; /*!< [31..0] CRC destination address - Destination address for CRC calculation */
        } C6DESTADDR_f;
    };
    union {
        __IOM uint32_t C6LLI               ; /*!< Offset: 0x1c8 (read-write) Channel 6 LLI - DMA channel 6 linked list item pointer */
        struct {
            __IOM uint32_t LM                  : 1; /*!< [0..0] Loop mode - Enables circular buffer mode for DMA */
            __IM  uint32_t          : 1;
            __IOM uint32_t LLI                 : 30; /*!< [31..2] Linked list item - Next descriptor address in linked list */
        } C6LLI_f;
    };
    union {
        __IOM uint32_t C6CTRL              ; /*!< Offset: 0x1cc (read-write) Channel 6 control - DMA channel 6 control register */
        struct {
            __IOM uint32_t TRANSFERSIZE        : 16; /*!< [15..0] Transfer size - Number of data items to transfer */
            __IOM uint32_t SBSIZE              : 3; /*!< [18..16] Source burst size - Source burst transfer size configuration */
            __IOM uint32_t DBSIZE              : 3; /*!< [21..19] Destination burst size - Destination burst transfer size configuration */
            __IOM uint32_t SWIDTH              : 2; /*!< [23..22] Source data width - Source data width in bytes */
            __IOM uint32_t DWIDTH              : 2; /*!< [25..24] Destination data width - Destination data width in bytes */
            __IOM uint32_t SIORSD              : 2; /*!< [27..26] Source I/O or reduced speed - Source peripheral I/O mode or reduced speed */
            __IOM uint32_t DIORDD              : 2; /*!< [29..28] Destination I/O or reduced speed - Destination peripheral I/O mode or reduced speed */
            __IM  uint32_t          : 1;
            __IOM uint32_t RITEN               : 1; /*!< [31..31] Repeat interval timer enable - Enables cyclic timer for transfers */
        } C6CTRL_f;
    };
    union {
        __IOM uint32_t C6CONFIG            ; /*!< Offset: 0x1d0 (read-write) Channel 6 configuration - DMA channel 6 configuration register */
        struct {
            __IOM uint32_t EN                  : 1; /*!< [0..0] Enable - Enables the module or function */
            __IOM uint32_t FLOWCTRL            : 3; /*!< [3..1] Flow control - Selects flow controller for the transfer */
            __IOM uint32_t IE                  : 1; /*!< [4..4] Interrupt enable register - Enables or disables various interrupt sources */
            __IOM uint32_t ITC                 : 1; /*!< [5..5] Interrupt trigger configuration - Configures interrupt trigger conditions */
            __IOM uint32_t IHFTC               : 1; /*!< [6..6] Interrupt hold-off timer configuration - Configures interrupt hold-off period */
            __IOM uint32_t ACTIVE              : 1; /*!< [7..7] Active flag - Indicates the channel or module is active */
            __IOM uint32_t HALT                : 1; /*!< [8..8] Halt request - Requests halting of current transfer */
            __IOM uint32_t S                   : 1; /*!< [9..9] Source - Source selection for multiplexed inputs */
            __IOM uint32_t D                   : 1; /*!< [10..10] Destination - Destination selection for multiplexed outputs */
            __IOM uint32_t LOCK                : 1; /*!< [11..11] Lock register - Locks timer configuration bits against modification */
            __IM  uint32_t          : 4;
            __IOM uint32_t DESTPERIPH          : 8; /*!< [23..16] Destination peripheral - Selects destination peripheral for transfer */
            __IOM uint32_t SRCPERIPH           : 8; /*!< [31..24] Source peripheral - Selects source peripheral for transfer */
        } C6CONFIG_f;
    };
    __IO uint32_t RESERVED8[3];
    union {
        __IOM uint32_t C7SRCADDR           ; /*!< Offset: 0x1e0 (read-write) Channel 7 source address - DMA channel 7 source address */
        struct {
            __IOM uint32_t CXSRCADDR           : 32; /*!< [31..0] CRC source address - Source address for CRC calculation */
        } C7SRCADDR_f;
    };
    union {
        __IOM uint32_t C7DESTADDR          ; /*!< Offset: 0x1e4 (read-write) Channel 7 destination address - DMA channel 7 destination address */
        struct {
            __IOM uint32_t CXDESTADDR          : 32; /*!< [31..0] CRC destination address - Destination address for CRC calculation */
        } C7DESTADDR_f;
    };
    union {
        __IOM uint32_t C7LLI               ; /*!< Offset: 0x1e8 (read-write) Channel 7 LLI - DMA channel 7 linked list item pointer */
        struct {
            __IOM uint32_t LM                  : 1; /*!< [0..0] Loop mode - Enables circular buffer mode for DMA */
            __IM  uint32_t          : 1;
            __IOM uint32_t LLI                 : 30; /*!< [31..2] Linked list item - Next descriptor address in linked list */
        } C7LLI_f;
    };
    union {
        __IOM uint32_t C7CTRL              ; /*!< Offset: 0x1ec (read-write) Channel 7 control - DMA channel 7 control register */
        struct {
            __IOM uint32_t TRANSFERSIZE        : 16; /*!< [15..0] Transfer size - Number of data items to transfer */
            __IOM uint32_t SBSIZE              : 3; /*!< [18..16] Source burst size - Source burst transfer size configuration */
            __IOM uint32_t DBSIZE              : 3; /*!< [21..19] Destination burst size - Destination burst transfer size configuration */
            __IOM uint32_t SWIDTH              : 2; /*!< [23..22] Source data width - Source data width in bytes */
            __IOM uint32_t DWIDTH              : 2; /*!< [25..24] Destination data width - Destination data width in bytes */
            __IOM uint32_t SIORSD              : 2; /*!< [27..26] Source I/O or reduced speed - Source peripheral I/O mode or reduced speed */
            __IOM uint32_t DIORDD              : 2; /*!< [29..28] Destination I/O or reduced speed - Destination peripheral I/O mode or reduced speed */
            __IM  uint32_t          : 1;
            __IOM uint32_t RITEN               : 1; /*!< [31..31] Repeat interval timer enable - Enables cyclic timer for transfers */
        } C7CTRL_f;
    };
    union {
        __IOM uint32_t C7CONFIG            ; /*!< Offset: 0x1f0 (read-write) Channel 7 configuration - DMA channel 7 configuration register */
        struct {
            __IOM uint32_t EN                  : 1; /*!< [0..0] Enable - Enables the module or function */
            __IOM uint32_t FLOWCTRL            : 3; /*!< [3..1] Flow control - Selects flow controller for the transfer */
            __IOM uint32_t IE                  : 1; /*!< [4..4] Interrupt enable register - Enables or disables various interrupt sources */
            __IOM uint32_t ITC                 : 1; /*!< [5..5] Interrupt trigger configuration - Configures interrupt trigger conditions */
            __IOM uint32_t IHFTC               : 1; /*!< [6..6] Interrupt hold-off timer configuration - Configures interrupt hold-off period */
            __IOM uint32_t ACTIVE              : 1; /*!< [7..7] Active flag - Indicates the channel or module is active */
            __IOM uint32_t HALT                : 1; /*!< [8..8] Halt request - Requests halting of current transfer */
            __IOM uint32_t S                   : 1; /*!< [9..9] Source - Source selection for multiplexed inputs */
            __IOM uint32_t D                   : 1; /*!< [10..10] Destination - Destination selection for multiplexed outputs */
            __IOM uint32_t LOCK                : 1; /*!< [11..11] Lock register - Locks timer configuration bits against modification */
            __IM  uint32_t          : 4;
            __IOM uint32_t DESTPERIPH          : 8; /*!< [23..16] Destination peripheral - Selects destination peripheral for transfer */
            __IOM uint32_t SRCPERIPH           : 8; /*!< [31..24] Source peripheral - Selects source peripheral for transfer */
        } C7CONFIG_f;
    };
} DMAC_TypeDef;

/**
 * @brief RCC - RCC Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t RCR                 ; /*!< Offset: 0x00 (read-write) Reset control register */
        struct {
            __IOM uint32_t LVDRSTEN            : 1; /*!< [0..0] LVD reset enable - Enables LVD reset generation */
            __IOM uint32_t WDTRSTEN            : 1; /*!< [1..1] Watchdog reset enable - Enables WDT reset */
            __IOM uint32_t IWDTRSTEN           : 1; /*!< [2..2] IWDT reset enable - Enables IWDT reset generation */
            __IOM uint32_t LOCKRSTEN           : 1; /*!< [3..3] Lock reset enable - Enables lock reset function */
            __IM  uint32_t          : 4;
            __IOM uint32_t IWDTRSTDIS          : 8; /*!< [15..8] IWDT reset disable - Disables IWDT reset generation */
            __IM  uint32_t          : 15;
            __IOM uint32_t SRST                : 1; /*!< [31..31] Software reset - Software reset control */
        } RCR_f;
    };
    union {
        __IOM uint32_t RSR                 ; /*!< Offset: 0x04 (read-write) Reset source status register */
        struct {
            __IOM uint32_t LVDRSTF             : 1; /*!< [0..0] LVD reset flag - LVD reset occurred flag */
            __IOM uint32_t WDTRSTF             : 1; /*!< [1..1] Watchdog reset flag - Watchdog reset occurred */
            __IOM uint32_t IWDTRSTF            : 1; /*!< [2..2] IWDT reset flag - IWDT reset occurred flag */
            __IOM uint32_t LOCKUPRSTF          : 1; /*!< [3..3] Lockup reset flag - CPU lockup reset occurred */
            __IOM uint32_t SYSREQRSTF          : 1; /*!< [4..4] System request reset flag - System request reset flag */
            __IOM uint32_t RSTNF               : 1; /*!< [5..5] Reset noise flag - Reset noise flag */
            __IM  uint32_t          : 2;
            __IOM uint32_t SRSTF               : 1; /*!< [8..8] Software reset flag - Software reset flag */
            __IOM uint32_t POR12RSTF           : 1; /*!< [9..9] POR 1.2V reset flag - POR 1.2V reset flag */
            __IOM uint32_t PWRRSTF             : 1; /*!< [10..10] Power reset flag - Power-on/power-down reset flag */
            __IM  uint32_t          : 5;
            __IOM uint32_t RSTFLAGCLR          : 1; /*!< [16..16] Reset flag clear - Clears reset flags */
            __IM  uint32_t          : 15;
        } RSR_f;
    };
    union {
        __IOM uint32_t AHB1RSTR            ; /*!< Offset: 0x08 (read-write) AHB1 peripheral reset register - Resets AHB1 bus peripherals */
        struct {
            __IOM uint32_t DMA1RST             : 1; /*!< [0..0] DMA1 reset - Resets DMA1 controller */
            __IOM uint32_t DMA2RST             : 1; /*!< [1..1] DMA2 reset - Resets DMA2 controller */
            __IM  uint32_t          : 3;
            __IOM uint32_t CRCRST              : 1; /*!< [5..5] CRC reset - Resets CRC peripheral */
            __IOM uint32_t ETHRST              : 1; /*!< [6..6] ETH reset - Resets Ethernet peripheral */
            __IM  uint32_t          : 3;
            __IOM uint32_t SPI1RST             : 1; /*!< [10..10] SPI1 reset - Resets SPI1 peripheral */
            __IOM uint32_t SPI2RST             : 1; /*!< [11..11] SPI2 reset - Resets SPI2 peripheral */
            __IOM uint32_t SPI3RST             : 1; /*!< [12..12] SPI3 reset - Resets SPI3 peripheral */
            __IOM uint32_t SPI4RST             : 1; /*!< [13..13] SPI4 reset - Resets SPI4 peripheral */
            __IM  uint32_t          : 6;
            __IOM uint32_t USB1CRST            : 1; /*!< [20..20] USB1 reset - Resets USB1 */
            __IM  uint32_t          : 1;
            __IOM uint32_t FDCAN1RST           : 1; /*!< [22..22] FDCAN1 reset - Resets FDCAN1 peripheral */
            __IOM uint32_t FDCAN2RST           : 1; /*!< [23..23] FDCAN2 reset - Resets FDCAN2 peripheral */
            __IM  uint32_t          : 8;
        } AHB1RSTR_f;
    };
    union {
        __IOM uint32_t AHB2RSTR            ; /*!< Offset: 0x0C (read-write) AHB2 peripheral reset register - Resets AHB2 bus peripherals */
        struct {
            __IOM uint32_t GPIOARST            : 1; /*!< [0..0] GPIOA reset control - Reset control for GPIO port A */
            __IOM uint32_t GPIOBRST            : 1; /*!< [1..1] GPIOB reset control - Reset control for GPIO port B */
            __IOM uint32_t GPIOCRST            : 1; /*!< [2..2] GPIOC reset control - Reset control for GPIO port C */
            __IOM uint32_t GPIODRST            : 1; /*!< [3..3] GPIOD reset control - Reset control for GPIO port D */
            __IOM uint32_t GPIOERST            : 1; /*!< [4..4] GPIOE reset control - Reset control for GPIO port E */
            __IOM uint32_t GPIOFRST            : 1; /*!< [5..5] GPIOF reset control - Reset control for GPIO port F */
            __IOM uint32_t GPIOGRST            : 1; /*!< [6..6] GPIOG reset control - Reset control for GPIO port G */
            __IOM uint32_t GPIOHRST            : 1; /*!< [7..7] GPIOH reset control - Reset control for GPIO port H */
            __IM  uint32_t          : 9;
            __IOM uint32_t ADCRST              : 1; /*!< [17..17] ADC12RST - ADC1/2 reset - Resets ADC1 and ADC2 */
            __IM  uint32_t          : 1;
            __IOM uint32_t DAC1RST             : 1; /*!< [19..19] DAC1 reset - Resets DAC1 peripheral */
            __IM  uint32_t          : 7;
            __IOM uint32_t UACRST              : 1; /*!< [27..27] UAC reset - Resets UAC */
            __IM  uint32_t          : 4;
        } AHB2RSTR_f;
    };
    union {
        __IOM uint32_t AHB3RSTR            ; /*!< Offset: 0x10 (read-write) AHB3 peripheral reset register - Resets AHB3 bus peripherals */
        struct {
            __IOM uint32_t SPI7RST             : 1; /*!< [0..0] SPI7 reset - Resets SPI7 peripheral */
            __IM  uint32_t          : 3;
            __IOM uint32_t SDMMCRST            : 1; /*!< [4..4] SDMMC reset - Resets SDMMC */
            __IM  uint32_t          : 3;
            __IOM uint32_t OSPI1RST            : 1; /*!< [8..8] OSPI1 reset - Resets OSPI1 */
            __IOM uint32_t OSPI2RST            : 1; /*!< [9..9] OSPI2 reset - Resets OSPI2 */
            __IM  uint32_t          : 22;
        } AHB3RSTR_f;
    };
    union {
        __IOM uint32_t APB1RSTR1           ; /*!< Offset: 0x14 (read-write) APB1 reset register 1 - Resets APB1 bus peripherals */
        struct {
            __IOM uint32_t TIM2RST             : 1; /*!< [0..0] TIM2 reset - Resets TIM2 timer peripheral */
            __IOM uint32_t TIM3RST             : 1; /*!< [1..1] TIM3 reset - Resets TIM3 timer peripheral */
            __IOM uint32_t TIM4RST             : 1; /*!< [2..2] TIM4 reset - Resets TIM4 timer peripheral */
            __IOM uint32_t TIM5RST             : 1; /*!< [3..3] TIM5 reset - Resets TIM5 timer peripheral */
            __IOM uint32_t TIM6RST             : 1; /*!< [4..4] TIM6 reset - Resets TIM6 timer peripheral */
            __IOM uint32_t TIM7RST             : 1; /*!< [5..5] TIM7 reset - Resets TIM7 timer peripheral */
            __IM  uint32_t          : 5;
            __IOM uint32_t WDTRST              : 1; /*!< [11..11] WDT reset - Watchdog timer reset control */
            __IM  uint32_t          : 2;
            __IOM uint32_t I2S1RST             : 1; /*!< [14..14] I2S1 reset - Resets I2S1 peripheral */
            __IOM uint32_t I2S2RST             : 1; /*!< [15..15] I2S2 reset - Resets I2S2 peripheral */
            __IM  uint32_t          : 1;
            __IOM uint32_t UART2RST            : 1; /*!< [17..17] UART2 reset - Resets UART2 peripheral */
            __IOM uint32_t UART3RST            : 1; /*!< [18..18] UART3 reset - Resets UART3 peripheral */
            __IOM uint32_t UART4RST            : 1; /*!< [19..19] UART4 reset - Resets UART4 peripheral */
            __IOM uint32_t UART5RST            : 1; /*!< [20..20] UART5 reset - Resets UART5 peripheral */
            __IOM uint32_t I2C1RST             : 1; /*!< [21..21] I2C1 reset - Resets I2C1 peripheral */
            __IOM uint32_t I2C2RST             : 1; /*!< [22..22] I2C2 reset - Resets I2C2 peripheral */
            __IM  uint32_t          : 4;
            __IOM uint32_t PMURST              : 1; /*!< [27..27] PMU reset - Resets PMU */
            __IM  uint32_t          : 1;
            __IOM uint32_t LPUART2RST          : 1; /*!< [29..29] LPUART2 reset - Resets LPUART2 */
            __IOM uint32_t LPTIM1RST           : 1; /*!< [30..30] LPTIM1 reset - Resets LPTIM1 */
            __IOM uint32_t LPUART1RST          : 1; /*!< [31..31] LPUART1 reset - Resets LPUART1 */
        } APB1RSTR1_f;
    };
    union {
        __IOM uint32_t APB1RSTR2           ; /*!< Offset: 0x18 (read-write) APB1 reset register 2 - Resets additional APB1 peripherals */
        struct {
            __IM  uint32_t          : 2;
            __IOM uint32_t UART7RST            : 1; /*!< [2..2] UART7 reset - Resets UART7 peripheral */
            __IOM uint32_t UART8RST            : 1; /*!< [3..3] UART8 reset - Resets UART8 peripheral */
            __IM  uint32_t          : 1;
            __IOM uint32_t TIM26RST            : 1; /*!< [5..5] TIM26 reset - Resets TIM26 timer peripheral */
            __IOM uint32_t EFUSERST            : 1; /*!< [6..6] EFUSE reset - Resets EFUSE controller */
            __IM  uint32_t          : 25;
        } APB1RSTR2_f;
    };
    union {
        __IOM uint32_t APB2RSTR            ; /*!< Offset: 0x1C (read-write) APB2 peripheral reset register - Resets APB2 bus peripherals */
        struct {
            __IOM uint32_t SYSCFGRST           : 1; /*!< [0..0] SYSCFG reset - Resets SYSCFG */
            __IM  uint32_t          : 1;
            __IOM uint32_t COMPRST             : 1; /*!< [2..2] COMP reset - Resets comparator peripheral */
            __IM  uint32_t          : 1;
            __IOM uint32_t EXTIRST             : 1; /*!< [4..4] EXTI reset - Resets EXTI peripheral */
            __IM  uint32_t          : 1;
            __IOM uint32_t TIM1RST             : 1; /*!< [6..6] TIM1 reset - Resets TIM1 timer peripheral */
            __IM  uint32_t          : 1;
            __IOM uint32_t TIM8RST             : 1; /*!< [8..8] TIM8 reset - Resets TIM8 timer peripheral */
            __IOM uint32_t UART1RST            : 1; /*!< [9..9] UART1 reset - Resets UART1 peripheral */
            __IOM uint32_t UART6RST            : 1; /*!< [10..10] UART6 reset - Resets UART6 peripheral */
            __IM  uint32_t          : 10;
            __IOM uint32_t TIM9RST             : 1; /*!< [21..21] TIM9 reset - Resets TIM9 timer peripheral */
            __IOM uint32_t TIM10RST            : 1; /*!< [22..22] TIM10 reset - Resets TIM10 timer peripheral */
            __IM  uint32_t          : 9;
        } APB2RSTR_f;
    };
    __IO uint32_t RESERVED0[2];
    union {
        __IOM uint32_t CCR1                ; /*!< Offset: 0x28 (read-write) Clock control register 1 */
        struct {
            __IOM uint32_t SYSCLKSEL           : 2; /*!< [1..0] System clock source selection */
            __IM  uint32_t          : 30;
        } CCR1_f;
    };
    union {
        __IOM uint32_t CCR2                ; /*!< Offset: 0x2C (read-write) Clock control register 2 */
        struct {
            __IOM uint32_t SYSDIV0             : 4; /*!< [3..0] System clock divider stage 0 */
            __IOM uint32_t SYSDIV1             : 4; /*!< [7..4] System clock divider stage 1 */
            __IOM uint32_t PCLK1DIV            : 3; /*!< [10..8] PCLK1 divider - APB1 clock divider factor */
            __IOM uint32_t PCLK2DIV            : 3; /*!< [13..11] PCLK2 divider - APB2 clock divider factor */
            __IM  uint32_t          : 6;
            __IOM uint32_t FLTCLKSEL           : 1; /*!< [20..20] Filter clock selection - Digital filter clock source */
            __IM  uint32_t          : 3;
            __IOM uint32_t HRNGSDIV            : 7; /*!< [30..24] HRNG clock divider - HRNG sample clock divider */
            __IOM uint32_t DIVDONE             : 1; /*!< [31..31] Division done - Division operation complete flag */
        } CCR2_f;
    };
    union {
        __IOM uint32_t PERCFGR             ; /*!< Offset: 0x30 (read-write) Peripheral configuration register - Peripheral config */
        struct {
            __IM  uint32_t          : 8;
            __IOM uint32_t SDMMCSCKS           : 2; /*!< [9..8] SDMMC SD clock select - SD/SDIO/MMC sample clock source */
            __IM  uint32_t          : 1;
            __IOM uint32_t SDMMCCKS            : 1; /*!< [11..11] SDMMC clock select - SDMMC clock source */
            __IOM uint32_t ADCCKS              : 1; /*!< [12..12] ADC clock selection - Selects ADC clock source */
            __IM  uint32_t          : 3;
            __IOM uint32_t LPTIM1CKS           : 2; /*!< [17..16] LPTIM1 clock select - LPTIM1 clock source selection */
            __IM  uint32_t          : 6;
            __IOM uint32_t LPUART1DIV          : 2; /*!< [25..24] LPUART1 clock divider - LPUART1 baud clock divider */
            __IOM uint32_t LPUART1CKS          : 2; /*!< [27..26] LPUART1 clock select - LPUART1 clock source */
            __IOM uint32_t LPUART2DIV          : 2; /*!< [29..28] LPUART2 clock divider - LPUART2 baud clock divider */
            __IOM uint32_t LPUART2CKS          : 2; /*!< [31..30] LPUART2 clock select - LPUART2 clock source */
        } PERCFGR_f;
    };
    union {
        __IOM uint32_t CIR                 ; /*!< Offset: 0x34 (read-write) Clock interrupt register */
        struct {
            __IOM uint32_t RCLRDYIF            : 1; /*!< [0..0] RCL ready interrupt flag - RCL ready interrupt flag */
            __IOM uint32_t XTLRDYIF            : 1; /*!< [1..1] XTL ready interrupt flag - XTL ready interrupt flag */
            __IOM uint32_t RCHRDYIF            : 1; /*!< [2..2] RCH ready interrupt flag - RCH ready interrupt flag */
            __IOM uint32_t XTHRDYIF            : 1; /*!< [3..3] XTH ready interrupt flag - XTH ready interrupt flag */
            __IOM uint32_t PLL1LOCKIF          : 1; /*!< [4..4] PLL1 lock interrupt flag - PLL1 lock interrupt flag */
            __IM  uint32_t          : 3;
            __IOM uint32_t RCLRDYIE            : 1; /*!< [8..8] RCL ready interrupt enable - Enables RCL ready interrupt */
            __IOM uint32_t XTLRDYIE            : 1; /*!< [9..9] XTL ready interrupt enable - Enables XTL ready interrupt */
            __IOM uint32_t RCHRDYIE            : 1; /*!< [10..10] RCH ready interrupt enable - Enables RCH ready interrupt */
            __IOM uint32_t XTHRDYIE            : 1; /*!< [11..11] XTH ready interrupt enable - Enables XTH ready interrupt */
            __IOM uint32_t PLL1LOCKIE          : 1; /*!< [12..12] PLL1 lock interrupt enable - Enables PLL1 lock interrupt */
            __IM  uint32_t          : 3;
            __IOM uint32_t RCLRDYIC            : 1; /*!< [16..16] RCL ready interrupt clear - Clears RCL ready interrupt */
            __IOM uint32_t XTLRDYIC            : 1; /*!< [17..17] XTL ready interrupt clear - Clears XTL ready interrupt */
            __IOM uint32_t RCHRDYIC            : 1; /*!< [18..18] RCH ready interrupt clear - Clears RCH ready interrupt */
            __IOM uint32_t XTHRDYIC            : 1; /*!< [19..19] XTH ready interrupt clear - Clears XTH ready interrupt */
            __IOM uint32_t PLL1LOCKIC          : 1; /*!< [20..20] PLL1 lock interrupt clear - Clears PLL1 lock interrupt */
            __IM  uint32_t          : 3;
            __IOM uint32_t XTHSDIE             : 1; /*!< [24..24] XTH stop detect interrupt enable - Enables XTH stop detect */
            __IOM uint32_t XTHSDIC             : 1; /*!< [25..25] XTH stop detect interrupt clear - Clears XTH stop detect */
            __IM  uint32_t          : 1;
            __IOM uint32_t XTHSDF              : 1; /*!< [27..27] XTH stop detect flag - XTH stop detection flag */
            __IOM uint32_t XTLSDIE             : 1; /*!< [28..28] XTL stop detect interrupt enable - Enables XTL stop detect */
            __IOM uint32_t XTLSDIC             : 1; /*!< [29..29] XTL stop detect interrupt clear - Clears XTL stop detect */
            __IM  uint32_t          : 1;
            __IOM uint32_t XTLSDF              : 1; /*!< [31..31] XTL stop detect flag - XTL stop detection flag */
        } CIR_f;
    };
    union {
        __IOM uint32_t AHB1CKENR           ; /*!< Offset: 0x38 (read-write) AHB1 clock enable register - Enables clocks for AHB1 bus peripherals */
        struct {
            __IOM uint32_t DMA1CKEN            : 1; /*!< [0..0] DMA1 clock enable - Enables DMA1 controller clock */
            __IOM uint32_t DMA2CKEN            : 1; /*!< [1..1] DMA2 clock enable - Enables DMA2 controller clock */
            __IM  uint32_t          : 3;
            __IOM uint32_t CRCCKEN             : 1; /*!< [5..5] CRC clock enable - Enables CRC clock */
            __IOM uint32_t ETHMACCKEN          : 1; /*!< [6..6] ETH MAC clock enable - Enables ETH MAC clock */
            __IOM uint32_t ETHTXCKEN           : 1; /*!< [7..7] ETH TX clock enable - Ethernet transmit clock enable */
            __IOM uint32_t ETHRXCKEN           : 1; /*!< [8..8] ETH RX clock enable - Ethernet receive clock enable */
            __IM  uint32_t          : 1;
            __IOM uint32_t SPI1CKEN            : 1; /*!< [10..10] SPI1 clock enable - Enables SPI1 clock */
            __IOM uint32_t SPI2CKEN            : 1; /*!< [11..11] SPI2 clock enable - Enables SPI2 clock */
            __IOM uint32_t SPI3CKEN            : 1; /*!< [12..12] SPI3 clock enable - Enables SPI3 clock */
            __IOM uint32_t SPI4CKEN            : 1; /*!< [13..13] SPI4 clock enable - Enables SPI4 clock */
            __IM  uint32_t          : 6;
            __IOM uint32_t USB1CCKEN           : 1; /*!< [20..20] USB1 clock enable - Enables USB1 clock */
            __IM  uint32_t          : 1;
            __IOM uint32_t FDCAN1CKEN          : 1; /*!< [22..22] FDCAN1 clock enable - Enables FDCAN1 clock */
            __IOM uint32_t FDCAN2CKEN          : 1; /*!< [23..23] FDCAN2 clock enable - Enables FDCAN2 clock */
            __IM  uint32_t          : 3;
            __IOM uint32_t BKPSRAMCKEN         : 1; /*!< [27..27] Backup SRAM clock enable - Enables backup SRAM clock */
            __IOM uint32_t ROMCKEN             : 1; /*!< [28..28] ROM clock enable - Enables ROM clock */
            __IOM uint32_t SRAMCKEN            : 1; /*!< [29..29] SRAM clock enable - Enables SRAM clock */
            __IM  uint32_t          : 2;
        } AHB1CKENR_f;
    };
    union {
        __IOM uint32_t AHB2CKENR           ; /*!< Offset: 0x3C (read-write) AHB2 clock enable register - Enables clocks for AHB2 bus peripherals */
        struct {
            __IOM uint32_t GPIOACKEN           : 1; /*!< [0..0] GPIOA clock enable - Clock enable for GPIO port A */
            __IOM uint32_t GPIOBCKEN           : 1; /*!< [1..1] GPIOB clock enable - Clock enable for GPIO port B */
            __IOM uint32_t GPIOCCKEN           : 1; /*!< [2..2] GPIOC clock enable - Clock enable for GPIO port C */
            __IOM uint32_t GPIODCKEN           : 1; /*!< [3..3] GPIOD clock enable - Clock enable for GPIO port D */
            __IOM uint32_t GPIOECKEN           : 1; /*!< [4..4] GPIOE clock enable - Clock enable for GPIO port E */
            __IOM uint32_t GPIOFCKEN           : 1; /*!< [5..5] GPIOF clock enable - Clock enable for GPIO port F */
            __IOM uint32_t GPIOGCKEN           : 1; /*!< [6..6] GPIOG clock enable - Clock enable for GPIO port G */
            __IOM uint32_t GPIOHCKEN           : 1; /*!< [7..7] GPIOH clock enable - Clock enable for GPIO port H */
            __IM  uint32_t          : 9;
            __IOM uint32_t ADCCKEN             : 1; /*!< [17..17] ADC clock enable - Enables clock for ADC peripheral */
            __IM  uint32_t          : 1;
            __IOM uint32_t DAC1CKEN            : 1; /*!< [19..19] DAC1 clock enable - Enables DAC1 peripheral clock */
            __IM  uint32_t          : 4;
            __IOM uint32_t AESPI1CKEN          : 1; /*!< [24..24] AES/SPI1 clock enable - Enables clock for combined AES/SPI1 peripheral */
            __IM  uint32_t          : 1;
            __IOM uint32_t HRNGCKEN            : 1; /*!< [26..26] HRNG clock enable - Enables HRNG clock */
            __IM  uint32_t          : 5;
        } AHB2CKENR_f;
    };
    union {
        __IOM uint32_t AHB3CKENR           ; /*!< Offset: 0x40 (read-write) AHB3 clock enable register - Enables clocks for AHB3 bus peripherals */
        struct {
            __IOM uint32_t SPI7CKEN            : 1; /*!< [0..0] SPI7 clock enable - Enables SPI7 clock */
            __IM  uint32_t          : 3;
            __IOM uint32_t SDMMCCKEN           : 1; /*!< [4..4] SDMMC clock enable - Enables SDMMC clock */
            __IM  uint32_t          : 3;
            __IOM uint32_t OSPI1CKEN           : 1; /*!< [8..8] OSPI1 clock enable - Enables OSPI1 clock */
            __IOM uint32_t OSPI2CKEN           : 1; /*!< [9..9] OSPI2 clock enable - Enables OSPI2 clock */
            __IM  uint32_t          : 22;
        } AHB3CKENR_f;
    };
    union {
        __IOM uint32_t APB1CKENR1          ; /*!< Offset: 0x44 (read-write) APB1 clock enable register 1 - Enables clocks for APB1 peripherals */
        struct {
            __IOM uint32_t TIM2CKEN            : 1; /*!< [0..0] TIM2 clock enable - Enables TIM2 timer clock */
            __IOM uint32_t TIM3CKEN            : 1; /*!< [1..1] TIM3 clock enable - Enables TIM3 timer clock */
            __IOM uint32_t TIM4CKEN            : 1; /*!< [2..2] TIM4 clock enable - Enables TIM4 timer clock */
            __IOM uint32_t TIM5CKEN            : 1; /*!< [3..3] TIM5 clock enable - Enables TIM5 timer clock */
            __IOM uint32_t TIM6CKEN            : 1; /*!< [4..4] TIM6 clock enable - Enables TIM6 timer clock */
            __IOM uint32_t TIM7CKEN            : 1; /*!< [5..5] TIM7 clock enable - Enables TIM7 timer clock */
            __IM  uint32_t          : 4;
            __IOM uint32_t RTCCKEN             : 1; /*!< [10..10] RTC clock enable - Enables RTC clock */
            __IOM uint32_t WDTCKEN             : 1; /*!< [11..11] WDT clock enable - Enables WDT clock */
            __IM  uint32_t          : 2;
            __IOM uint32_t I2S1CKEN            : 1; /*!< [14..14] I2S1 clock enable - Enables I2S1 clock */
            __IOM uint32_t I2S2CKEN            : 1; /*!< [15..15] I2S2 clock enable - Enables I2S2 clock */
            __IM  uint32_t          : 1;
            __IOM uint32_t UART2CKEN           : 1; /*!< [17..17] UART2 clock enable - Enables UART2 clock */
            __IOM uint32_t UART3CKEN           : 1; /*!< [18..18] UART3 clock enable - Enables UART3 clock */
            __IOM uint32_t UART4CKEN           : 1; /*!< [19..19] UART4 clock enable - Enables UART4 clock */
            __IOM uint32_t UART5CKEN           : 1; /*!< [20..20] UART5 clock enable - Enables UART5 clock */
            __IOM uint32_t I2C1CKEN            : 1; /*!< [21..21] I2C1 clock enable - Enables I2C1 clock */
            __IOM uint32_t I2C2CKEN            : 1; /*!< [22..22] I2C2 clock enable - Enables I2C2 clock */
            __IM  uint32_t          : 4;
            __IOM uint32_t PMUCKEN             : 1; /*!< [27..27] PMU clock enable - Enables PMU clock */
            __IM  uint32_t          : 1;
            __IOM uint32_t LPUART2CKEN         : 1; /*!< [29..29] LPUART2 clock enable - Enables LPUART2 clock */
            __IOM uint32_t LPTIM1CKEN          : 1; /*!< [30..30] LPTIM1 clock enable - Enables LPTIM1 clock */
            __IOM uint32_t LPUART1CKEN         : 1; /*!< [31..31] LPUART1 clock enable - Enables LPUART1 clock */
        } APB1CKENR1_f;
    };
    union {
        __IOM uint32_t APB1CKENR2          ; /*!< Offset: 0x48 (read-write) APB1 clock enable register 2 - Enables clocks for additional APB1 peripherals */
        struct {
            __IM  uint32_t          : 2;
            __IOM uint32_t UART7CKEN           : 1; /*!< [2..2] UART7 clock enable - Enables UART7 clock */
            __IOM uint32_t UART8CKEN           : 1; /*!< [3..3] UART8 clock enable - Enables UART8 clock */
            __IM  uint32_t          : 1;
            __IOM uint32_t TIM26CKEN           : 1; /*!< [5..5] TIM26 clock enable - Enables TIM26 clock */
            __IOM uint32_t EFUSECKEN           : 1; /*!< [6..6] EFUSE clock enable - Enables EFUSE clock */
            __IM  uint32_t          : 25;
        } APB1CKENR2_f;
    };
    union {
        __IOM uint32_t APB2CKENR           ; /*!< Offset: 0x4C (read-write) APB2 clock enable register - Enables clocks for APB2 bus peripherals */
        struct {
            __IOM uint32_t SYSCFGCKEN          : 1; /*!< [0..0] SYSCFG clock enable - Enables SYSCFG clock */
            __IM  uint32_t          : 1;
            __IOM uint32_t COMPCKEN            : 1; /*!< [2..2] Comparator clock enable - Enables comparator clock */
            __IM  uint32_t          : 1;
            __IOM uint32_t EXTICKEN            : 1; /*!< [4..4] EXTI clock enable - Enables EXTI clock */
            __IM  uint32_t          : 1;
            __IOM uint32_t TIM1CKEN            : 1; /*!< [6..6] TIM1 clock enable - Enables TIM1 timer clock */
            __IM  uint32_t          : 1;
            __IOM uint32_t TIM8CKEN            : 1; /*!< [8..8] TIM8 clock enable - Enables TIM8 timer clock */
            __IOM uint32_t UART1CKEN           : 1; /*!< [9..9] UART1 clock enable - Enables UART1 clock */
            __IOM uint32_t UART6CKEN           : 1; /*!< [10..10] UART6 clock enable - Enables UART6 clock */
            __IM  uint32_t          : 10;
            __IOM uint32_t TIM9CKEN            : 1; /*!< [21..21] TIM9 clock enable - Enables TIM9 timer clock */
            __IOM uint32_t TIM10CKEN           : 1; /*!< [22..22] TIM10 clock enable - Enables TIM10 timer clock */
            __IM  uint32_t          : 9;
        } APB2CKENR_f;
    };
    __IO uint32_t RESERVED1[2];
    union {
        __IOM uint32_t RCHCR               ; /*!< Offset: 0x58 (read-write) RCH control register - RCH oscillator control */
        struct {
            __IOM uint32_t RCHEN               : 1; /*!< [0..0] RCH oscillator enable - Enables the high-speed RC oscillator */
            __IM  uint32_t          : 2;
            __IOM uint32_t RCHDIV              : 1; /*!< [3..3] RCH divider - RCH clock divider factor */
            __IOM uint32_t RCHRDY              : 1; /*!< [4..4] RCH ready flag - Indicates RCH oscillator is stable */
            __IM  uint32_t          : 3;
            __IOM uint32_t RCHTRIML            : 7; /*!< [14..8] RCH trim low - RCH oscillator low trim */
            __IOM uint32_t RCHTRIMH            : 2; /*!< [16..15] RCH trim high - RCH oscillator high trim */
            __IM  uint32_t          : 15;
        } RCHCR_f;
    };
    union {
        __IOM uint32_t XTHCR               ; /*!< Offset: 0x5C (read-write) XTH control register - XTH oscillator control */
        struct {
            __IOM uint32_t XTHEN               : 1; /*!< [0..0] XTH oscillator enable - Enables the high-speed crystal oscillator */
            __IOM uint32_t XTHBYP              : 1; /*!< [1..1] XTH bypass - XTH bypass mode */
            __IOM uint32_t XTHRDYTIME          : 2; /*!< [3..2] XTH ready time - XTH stabilization time */
            __IOM uint32_t XTHRDY              : 1; /*!< [4..4] XTH ready flag - Indicates XTH oscillator is stable */
            __IM  uint32_t          : 3;
            __IOM uint32_t XTHSDEN             : 1; /*!< [8..8] XTH stop detect enable - Enables XTH stop detection */
            __IM  uint32_t          : 23;
        } XTHCR_f;
    };
    union {
        __IOM uint32_t PLL1CR              ; /*!< Offset: 0x60 (read-write) PLL1 control register - PLL1 control */
        struct {
            __IOM uint32_t PLL1EN              : 1; /*!< [0..0] PLL 1 enable - Enables PLL1 */
            __IOM uint32_t PLL1SRCSEL          : 1; /*!< [1..1] PLL1 source select - PLL1 clock source selection */
            __IM  uint32_t          : 2;
            __IOM uint32_t PLL1PCLKEN          : 1; /*!< [4..4] PLL1 P clock enable - Enables PLL1 P output clock */
            __IOM uint32_t PLL1QCLKEN          : 1; /*!< [5..5] PLL1 Q clock enable - Enables PLL1 Q output clock */
            __IM  uint32_t          : 15;
            __IOM uint32_t PLL1SLEEP           : 1; /*!< [21..21] PLL1 sleep - PLL1 sleep mode */
            __IOM uint32_t PLL1UPDATEEN        : 1; /*!< [22..22] PLL1 update enable - Enables PLL1 dynamic update */
            __IOM uint32_t PLL1LOCKDLY         : 6; /*!< [28..23] PLL1 lock delay - PLL1 lock delay configuration */
            __IOM uint32_t PLL1LOCK            : 1; /*!< [29..29] PLL1 lock - PLL1 lock status */
            __IOM uint32_t PLL1FREERUN         : 1; /*!< [30..30] PLL1 free run - PLL1 free running mode */
            __IOM uint32_t PLL1LOCKSEL         : 1; /*!< [31..31] PLL1 lock selection - PLL1 lock detector selection */
        } PLL1CR_f;
    };
    union {
        __IOM uint32_t PLL1CFR             ; /*!< Offset: 0x64 (read-write) PLL1 configuration register - PLL1 configuration */
        struct {
            __IOM uint32_t PLL1F               : 9; /*!< [8..0] PLL1 feedback - PLL1 feedback divider */
            __IM  uint32_t          : 3;
            __IOM uint32_t PLL1N               : 6; /*!< [17..12] PLL1 N factor - PLL1 multiplication factor N */
            __IM  uint32_t          : 2;
            __IOM uint32_t PLL1P               : 2; /*!< [21..20] PLL1 P factor - PLL1 output divider P */
            __IM  uint32_t          : 2;
            __IOM uint32_t PLL1Q               : 4; /*!< [27..24] PLL 1 Q - PLL1 Q divider factor */
            __IM  uint32_t          : 4;
        } PLL1CFR_f;
    };
    union {
        __IOM uint32_t PLL1SCR             ; /*!< Offset: 0x68 (read-write) PLL1 status/control register - PLL1 status and control */
        struct {
            __IOM uint32_t PLL1SSCEN           : 1; /*!< [0..0] PLL1 SSC enable - Enables PLL1 spread spectrum */
            __IOM uint32_t PLL1SSCMD           : 1; /*!< [1..1] PLL1 SSC modulation depth - SSC modulation depth */
            __IM  uint32_t          : 2;
            __IOM uint32_t PLL1SSCPER          : 13; /*!< [16..4] PLL1 SSC period - SSC modulation period */
            __IOM uint32_t PLL1SSCSTP          : 15; /*!< [31..17] PLL1 SSC step - SSC modulation step */
        } PLL1SCR_f;
    };
    __IO uint32_t RESERVED2[6];
    union {
        __IOM uint32_t CLKOCR              ; /*!< Offset: 0x84 (read-write) Clock output control register - Clock output configuration */
        struct {
            __IOM uint32_t MCOCLKS             : 5; /*!< [4..0] MCO clock selection - MCO clock source select */
            __IOM uint32_t MCO1SEL             : 1; /*!< [5..5] MCO1 selection - MCO1 source selection */
            __IOM uint32_t MCO1DIV             : 16; /*!< [21..6] MCO1 divider - MCO1 clock divider factor */
            __IOM uint32_t MCO1POL             : 1; /*!< [22..22] MCO1 polarity - MCO1 output polarity */
            __IOM uint32_t MCO1EN              : 1; /*!< [23..23] MCO1 enable - Enables MCO1 output */
            __IOM uint32_t MCO2DIV             : 6; /*!< [29..24] MCO2 divider - MCO2 clock divider factor */
            __IOM uint32_t MCO2POL             : 1; /*!< [30..30] MCO2 polarity - MCO2 output polarity */
            __IOM uint32_t MCO2EN              : 1; /*!< [31..31] MCO2 enable - Enables MCO2 output */
        } CLKOCR_f;
    };
    __IO uint32_t RESERVED3;
    union {
        __IOM uint32_t STDBYCTRL           ; /*!< Offset: 0x8C (read-write) STANDBY power domain control register */
        struct {
            __IOM uint32_t XTLEN               : 1; /*!< [0..0] XTL oscillator enable - Enables the low-speed crystal oscillator */
            __IOM uint32_t XTLRDY              : 1; /*!< [1..1] XTL ready flag - Indicates XTL oscillator is stable */
            __IOM uint32_t XTLBYP              : 1; /*!< [2..2] XTL bypass - XTL bypass mode */
            __IOM uint32_t XTLDRV              : 3; /*!< [5..3] XTL drive - XTL oscillator drive capability */
            __IOM uint32_t XTLSDEN             : 1; /*!< [6..6] XTL stop detect enable - Enables XTL stop detection */
            __IOM uint32_t RSV7                : 1; /*!< [7..7] Reserved - Reserved bit 7 (read-only) */
            __IOM uint32_t RCLEN               : 1; /*!< [8..8] RCL oscillator enable - Enables the low-speed RC oscillator */
            __IOM uint32_t RCLRDY              : 1; /*!< [9..9] RCL ready flag - Indicates RCL oscillator is stable */
            __IOM uint32_t RCLTRIM             : 6; /*!< [15..10] RCL trim - RCL oscillator trim value */
            __IOM uint32_t RCLDIS              : 4; /*!< [19..16] RCL disable - Disables RCL oscillator */
            __IOM uint32_t RTCSEL              : 2; /*!< [21..20] RTC clock selection - Selects RTC clock source */
            __IOM uint32_t RTCEN               : 1; /*!< [22..22] RTC clock enable - Enables RTC peripheral clock */
            __IOM uint32_t STDBYRST            : 1; /*!< [23..23] Standby reset - Standby mode reset control */
            __IM  uint32_t          : 8;
        } STDBYCTRL_f;
    };
    __IOM uint32_t Dummy               ; /*!< Offset: 0x90 (read-write) Dummy register - Read/write dummy register */
} RCC_TypeDef;

/**
 * @brief CRC - CRC Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t DATA                ; /*!< Offset: 0x00 (read-write) CRC data register - CRC calculation data input and result */
        struct {
            __IOM uint32_t DATA                : 32; /*!< [31..0] Data value - Data to be transferred or received data value */
        } DATA_f;
    };
    union {
        __IOM uint32_t CTRL                ; /*!< Offset: 0x04 (read-write) CRC control register - CRC configuration control */
        struct {
            __IOM uint32_t RST                 : 1; /*!< [0..0] CRC reset - Resets CRC_DATA to CRC_INIT value */
            __IOM uint32_t DATA_LEN            : 2; /*!< [2..1] Data length - Data transfer length in bytes */
            __IOM uint32_t PLOY_LEN            : 2; /*!< [4..3] Polynomial length - CRC polynomial length */
            __IOM uint32_t DATA_REV            : 2; /*!< [6..5] Data reversal - Reverses data bit order */
            __IOM uint32_t RSLT_REV            : 1; /*!< [7..7] CRC result reversal - Reverses CRC result bit order */
            __IOM uint32_t INITIAL_REV         : 1; /*!< [8..8] Initial reversal - Initial data reversal */
            __IOM uint32_t OUTXOR_REV          : 1; /*!< [9..9] Output XOR reversal - Output XOR reversal */
            __IOM uint32_t PLOY_REV            : 1; /*!< [10..10] Polynomial reversal - CRC polynomial reversal */
            __IM  uint32_t          : 21;
        } CTRL_f;
    };
    union {
        __IOM uint32_t INIT                ; /*!< Offset: 0x08 (read-write) Initial value register - CRC initial value */
        struct {
            __IOM uint32_t INIT                : 32; /*!< [31..0] Initial value register - CRC initial value */
        } INIT_f;
    };
    __IO uint32_t RESERVED0;
    union {
        __IOM uint32_t OUTXOR              ; /*!< Offset: 0x10 (read-write) Output XOR - Output XOR configuration */
        struct {
            __IOM uint32_t OUTXOR              : 32; /*!< [31..0] Output XOR - Output XOR configuration */
        } OUTXOR_f;
    };
    union {
        __IOM uint32_t POLY                ; /*!< Offset: 0x14 (read-write) Polynomial - CRC polynomial value */
        struct {
            __IOM uint32_t POLY                : 32; /*!< [31..0] Polynomial - CRC polynomial value */
        } POLY_f;
    };
    union {
        __IOM uint32_t FDATA               ; /*!< Offset: 0x18 (read-write) Free data register - Independent data register not used for CRC calculation */
        struct {
            __IOM uint32_t DATA                : 32; /*!< [31..0] Data value - Data to be transferred or received data value */
        } FDATA_f;
    };
} CRC_TypeDef;

/**
 * @brief ETH - ETH Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t MACCR               ; /*!< Offset: 0x0000 (read-write) MAC control register - Ethernet MAC control configuration */
        struct {
            __IOM uint32_t PRELEN              : 2; /*!< [1..0] Preamble length - Preamble length configuration */
            __IOM uint32_t RE                  : 1; /*!< [2..2] Receiver enable - Enables UART receiver */
            __IOM uint32_t TE                  : 1; /*!< [3..3] Transmitter enable - Enables UART transmitter */
            __IOM uint32_t DC                  : 1; /*!< [4..4] Data control - Data control configuration */
            __IOM uint32_t BL                  : 2; /*!< [6..5] Bus lock - Bus lock status flag */
            __IOM uint32_t APCS                : 1; /*!< [7..7] APB clock selection - Selects APB clock source and divider */
            __IM  uint32_t          : 1;
            __IOM uint32_t DR                  : 1; /*!< [9..9] Data register - UART data register */
            __IOM uint32_t IPCO                : 1; /*!< [10..10] IP control - IP control register */
            __IOM uint32_t DM                  : 1; /*!< [11..11] Data mask - Data mask configuration */
            __IOM uint32_t LM                  : 1; /*!< [12..12] Loop mode - Enables circular buffer mode for DMA */
            __IOM uint32_t ROD                 : 1; /*!< [13..13] ROD - ROD value */
            __IOM uint32_t FES                 : 1; /*!< [14..14] Framing error status - FES */
            __IM  uint32_t          : 1;
            __IOM uint32_t CSD                 : 1; /*!< [16..16] Clock security detection - Clock security failure detection flag */
            __IOM uint32_t IFG                 : 3; /*!< [19..17] Inter-frame gap - Inter-frame gap configuration */
            __IOM uint32_t JE                  : 1; /*!< [20..20] J enable - Injected enable */
            __IM  uint32_t          : 1;
            __IOM uint32_t JD                  : 1; /*!< [22..22] J data - Injected data */
            __IOM uint32_t WD                  : 1; /*!< [23..23] WD - WD value */
            __IM  uint32_t          : 1;
            __IOM uint32_t CSTF                : 1; /*!< [25..25] Clear stop flag - Clears stop mode flag */
            __IM  uint32_t          : 1;
            __IOM uint32_t S2KP                : 1; /*!< [27..27] S2KP - S2KP register */
            __IOM uint32_t SAIRC               : 3; /*!< [30..28] SAIRC - SAIRC value */
            __IM  uint32_t          : 1;
        } MACCR_f;
    };
    union {
        __IOM uint32_t MACFFR              ; /*!< Offset: 0x0004 (read-write) MAC frame filter register - Ethernet MAC frame filtering */
        struct {
            __IOM uint32_t PM                  : 1; /*!< [0..0] AM/PM indicator - Indicates AM or PM in 12-hour mode */
            __IOM uint32_t HU                  : 1; /*!< [1..1] Hour units - BCD units digit of hour value */
            __IOM uint32_t HMC                 : 1; /*!< [2..2] HMC - Host mode control */
            __IOM uint32_t DAIF                : 1; /*!< [3..3] Data available interrupt flag - Data available for read flag */
            __IOM uint32_t PAM                 : 1; /*!< [4..4] PAM - PAM configuration */
            __IOM uint32_t DBF                 : 1; /*!< [5..5] Debounce filter - Debounce filter configuration */
            __IOM uint32_t PCF                 : 2; /*!< [7..6] PCF - PCF value */
            __IOM uint32_t SAIF                : 1; /*!< [8..8] SAIF - SAIF value */
            __IOM uint32_t SAF                 : 1; /*!< [9..9] SAF - SAF value */
            __IOM uint32_t HPF                 : 1; /*!< [10..10] High-pass filter - High-pass filter configuration */
            __IM  uint32_t          : 5;
            __IOM uint32_t VTFE                : 1; /*!< [16..16] VTFE - VTFE value */
            __IM  uint32_t          : 3;
            __IOM uint32_t IPFE                : 1; /*!< [20..20] IP feature - IP feature enable */
            __IOM uint32_t DNTU                : 1; /*!< [21..21] Down time - Down time counter value */
            __IM  uint32_t          : 9;
            __IOM uint32_t RA                  : 1; /*!< [31..31] RA - RA value */
        } MACFFR_f;
    };
    union {
        __IOM uint32_t MACHTHR             ; /*!< Offset: 0x0008 (read-write) MAC hash table high register - Ethernet MAC hash table high bits */
        struct {
            __IOM uint32_t HTH                 : 32; /*!< [31..0] HRNG test high - HRNG test high threshold */
        } MACHTHR_f;
    };
    union {
        __IOM uint32_t MACHTLR             ; /*!< Offset: 0x000C (read-write) MAC hash table low register - Ethernet MAC hash table low bits */
        struct {
            __IOM uint32_t HTL                 : 32; /*!< [31..0] HRNG test low - HRNG test low threshold */
        } MACHTLR_f;
    };
    union {
        __IOM uint32_t MACMIIAR            ; /*!< Offset: 0x0010 (read-write) MAC MII address register - Ethernet MII management address */
        struct {
            __IOM uint32_t MB                  : 1; /*!< [0..0] Mailbox - Mailbox register */
            __IOM uint32_t MW                  : 1; /*!< [1..1] MW - Memory width */
            __IOM uint32_t CR                  : 4; /*!< [5..2] DAC control register */
            __IOM uint32_t MR                  : 5; /*!< [10..6] Mode register - Mode config */
            __IOM uint32_t PA                  : 5; /*!< [15..11] PA - PA value */
            __IM  uint32_t          : 16;
        } MACMIIAR_f;
    };
    union {
        __IOM uint32_t MACMIIDR            ; /*!< Offset: 0x0014 (read-write) MAC MII data register - Ethernet MII management data */
        struct {
            __IOM uint32_t MD                  : 16; /*!< [15..0] Mode - Mode select */
            __IM  uint32_t          : 16;
        } MACMIIDR_f;
    };
    union {
        __IOM uint32_t MACFCR              ; /*!< Offset: 0x0018 (read-write) MAC flow control register - Ethernet MAC flow control */
        struct {
            __IOM uint32_t FCB_BPA             : 1; /*!< [0..0] FCB BPA - Flow control backpressure active */
            __IOM uint32_t TFCE                : 1; /*!< [1..1] TFCE - TFCE value */
            __IOM uint32_t RFCE                : 1; /*!< [2..2] Receive frame count error - Receive frame count error flag */
            __IOM uint32_t UPFD                : 1; /*!< [3..3] UPFD - UPFD value */
            __IM  uint32_t          : 12;
            __IOM uint32_t PT                  : 16; /*!< [31..16] PT - PT value */
        } MACFCR_f;
    };
    union {
        __IOM uint32_t MACVLANTR           ; /*!< Offset: 0x001C (read-write) MAC VLAN tag register - Ethernet MAC VLAN tag configuration */
        struct {
            __IOM uint32_t VL                  : 16; /*!< [15..0] VL - VL value */
            __IOM uint32_t ETV                 : 1; /*!< [16..16] Ethernet tag valid - VLAN tag valid indicator */
            __IOM uint32_t VTIM                : 1; /*!< [17..17] VTIM - VTIM value */
            __IOM uint32_t ESVL                : 1; /*!< [18..18] ESV level - Error severity level */
            __IOM uint32_t VTHM                : 1; /*!< [19..19] VTHM - VTHM value */
            __IM  uint32_t          : 12;
        } MACVLANTR_f;
    };
    __IO uint32_t RESERVED0;
    union {
        __IM  uint32_t MACDBGR             ; /*!< Offset: 0x0024 (read-only) MAC debug register - Ethernet MAC debug status */
        struct {
            __IM  uint32_t MMRPES              : 1; /*!< [0..0] MMC RPES - MMC RPES status */
            __IM  uint32_t MRFFCS              : 2; /*!< [2..1] MR FCS - MMC receive FCS error */
            __IM  uint32_t          : 1;
            __IM  uint32_t RFWCS               : 1; /*!< [4..4] Receive FIFO write count status - RX FIFO write count status */
            __IM  uint32_t RFRCS               : 2; /*!< [6..5] Receive FIFO read count status - RX FIFO read count status */
            __IM  uint32_t          : 1;
            __IM  uint32_t RFFL                : 2; /*!< [9..8] Receive FIFO fill level - RX FIFO fill level indication */
            __IM  uint32_t          : 6;
            __IM  uint32_t MMTEA               : 1; /*!< [16..16] MMC TEA - MMC TEA status */
            __IM  uint32_t MTFCS               : 2; /*!< [18..17] MT FCS - MMC transmit FCS */
            __IM  uint32_t MTP                 : 1; /*!< [19..19] MTP - MTP value */
            __IM  uint32_t TFRS                : 2; /*!< [21..20] TFRS - TFRS value */
            __IM  uint32_t TFWA                : 1; /*!< [22..22] TFWA - TFWA value */
            __IM  uint32_t          : 1;
            __IM  uint32_t TFNE                : 1; /*!< [24..24] TFNE - TFNE value */
            __IM  uint32_t TFF                 : 1; /*!< [25..25] TFF - TFF value */
            __IM  uint32_t          : 6;
        } MACDBGR_f;
    };
    union {
        __IOM uint32_t MACRWUFFR           ; /*!< Offset: 0x0028 (read-write) MAC remote wakeup frame filter register - Ethernet remote wakeup filtering */
        struct {
            __IOM uint32_t MACRWUFF            : 32; /*!< [31..0] MAC remote wakeup frame filter - Remote wakeup frame filter register */
        } MACRWUFFR_f;
    };
    union {
        __IOM uint32_t MACPMTCSR           ; /*!< Offset: 0x002C (read-write) MAC PMT control status register - Ethernet power management */
        struct {
            __IOM uint32_t PD                  : 1; /*!< [0..0] Pending flag - Indicates pending interrupt/event */
            __IOM uint32_t MPE                 : 1; /*!< [1..1] Match polarity enable - Enables MPE */
            __IOM uint32_t WFE                 : 1; /*!< [2..2] WFE - WFE value */
            __IM  uint32_t          : 2;
            __IOM uint32_t MPR                 : 1; /*!< [5..5] MPR - MPR value register */
            __IOM uint32_t WFR                 : 1; /*!< [6..6] WFR - WFR value */
            __IM  uint32_t          : 2;
            __IOM uint32_t GU                  : 1; /*!< [9..9] Guard - Guard event status */
            __IM  uint32_t          : 14;
            __IOM uint32_t RWFP                : 3; /*!< [26..24] RWFP - RWFP value */
            __IM  uint32_t          : 4;
            __IOM uint32_t RWFFPR              : 1; /*!< [31..31] RWFFPR - RWFFPR value */
        } MACPMTCSR_f;
    };
    union {
        __IOM uint32_t MACLPICSR           ; /*!< Offset: 0x0030 (read-write) MAC LPI control/status register - LPI control and status */
        struct {
            __IOM uint32_t TLPIEN              : 1; /*!< [0..0] TLPIEN - TLPIEN value */
            __IOM uint32_t TLPIEX              : 1; /*!< [1..1] TLPIEX - TLPIEX value */
            __IOM uint32_t RLPIEN              : 1; /*!< [2..2] RLPIEN - RLPIEN value */
            __IOM uint32_t RLPIEX              : 1; /*!< [3..3] RLPIEX - RLPIEX value */
            __IM  uint32_t          : 4;
            __IOM uint32_t TLPIST              : 1; /*!< [8..8] TLPIST - TLPIST value */
            __IOM uint32_t RLPIST              : 1; /*!< [9..9] RLPIST - RLPIST value */
            __IM  uint32_t          : 6;
            __IOM uint32_t LPIEN               : 1; /*!< [16..16] Low power interrupt enable - Enables low power interrupt */
            __IOM uint32_t PLS                 : 1; /*!< [17..17] PLS - PLS value */
            __IM  uint32_t          : 1;
            __IOM uint32_t LPITXA              : 1; /*!< [19..19] Low power idle TX - Low power idle transmit */
            __IM  uint32_t          : 12;
        } MACLPICSR_f;
    };
    union {
        __IOM uint32_t MACLPITCR           ; /*!< Offset: 0x0034 (read-write) MAC LPI timer control register - LPI timer control */
        struct {
            __IOM uint32_t TWT                 : 16; /*!< [15..0] TWT - TWT value */
            __IOM uint32_t LST                 : 10; /*!< [25..16] Low state - Low state value */
            __IM  uint32_t          : 6;
        } MACLPITCR_f;
    };
    union {
        __IM  uint32_t MACISR              ; /*!< Offset: 0x0038 (read-only) MAC interrupt status register - MAC interrupt status */
        struct {
            __IM  uint32_t          : 3;
            __IM  uint32_t PMTS                : 1; /*!< [3..3] PMTS - PMT status */
            __IM  uint32_t MMCIS               : 1; /*!< [4..4] MMC interrupt status - MMC interrupt status register */
            __IM  uint32_t MMCRIS              : 1; /*!< [5..5] MMC receive interrupt status - MMC receive interrupt status */
            __IM  uint32_t MMCTIS              : 1; /*!< [6..6] MMC transmit interrupt status - MMC transmit interrupt status */
            __IM  uint32_t          : 2;
            __IM  uint32_t TIS                 : 1; /*!< [9..9] TIS - TIS value */
            __IM  uint32_t LPIIS               : 1; /*!< [10..10] Low power interrupt status - Low power interrupt status */
            __IM  uint32_t          : 21;
        } MACISR_f;
    };
    union {
        __IOM uint32_t MACIMR              ; /*!< Offset: 0x003C (read-write) MAC interrupt mask register - Ethernet MAC interrupt mask */
        struct {
            __IM  uint32_t          : 3;
            __IOM uint32_t PIM                 : 1; /*!< [3..3] PIM - PIM value */
            __IM  uint32_t          : 5;
            __IOM uint32_t TIM                 : 1; /*!< [9..9] TIM - TIM value */
            __IOM uint32_t LPIIM               : 1; /*!< [10..10] Low power interrupt mask - Masks low power interrupt */
            __IM  uint32_t          : 21;
        } MACIMR_f;
    };
    union {
        __IOM uint32_t MACA0HR             ; /*!< Offset: 0x0040 (read-write) MAC address 0 high register - Ethernet MAC address 0 high bits */
        struct {
            __IOM uint32_t MACA0H              : 16; /*!< [15..0] MAC address 0 high - MAC address 0 high 16 bits */
            __IM  uint32_t          : 15;
            __IOM uint32_t AE                  : 1; /*!< [31..31] Address error - Indicates address alignment error */
        } MACA0HR_f;
    };
    union {
        __IOM uint32_t MACA0LR             ; /*!< Offset: 0x0044 (read-write) MAC address 0 low register - Ethernet MAC address 0 low bits */
        struct {
            __IOM uint32_t MACA0L              : 32; /*!< [31..0] MAC address 0 low - MAC address 0 low 32 bits */
        } MACA0LR_f;
    };
    union {
        __IOM uint32_t MACA1HR             ; /*!< Offset: 0x0048 (read-write) MAC address 1 high register - MAC address 1 high reg */
        struct {
            __IOM uint32_t MACA1H              : 16; /*!< [15..0] MAC address 1 high - MAC address 1 high 16 bits */
            __IM  uint32_t          : 8;
            __IOM uint32_t MBC                 : 6; /*!< [29..24] MBC - Mailbox code */
            __IOM uint32_t SA                  : 1; /*!< [30..30] SA - Source address */
            __IOM uint32_t AE                  : 1; /*!< [31..31] Address error - Indicates address alignment error */
        } MACA1HR_f;
    };
    union {
        __IOM uint32_t MACA1LR             ; /*!< Offset: 0x004C (read-write) MAC address 1 low register - MAC address 1 low register */
        struct {
            __IOM uint32_t MACA1L              : 32; /*!< [31..0] MAC address 1 low - MAC address 1 low 32 bits */
        } MACA1LR_f;
    };
    union {
        __IOM uint32_t MACA2HR             ; /*!< Offset: 0x0050 (read-write) MAC address 2 high register - MAC address 2 high register */
        struct {
            __IOM uint32_t MACA2H              : 16; /*!< [15..0] MAC address 2 high - MAC address 2 high 16 bits */
            __IM  uint32_t          : 8;
            __IOM uint32_t MBC                 : 6; /*!< [29..24] MBC - Mailbox code */
            __IOM uint32_t SA                  : 1; /*!< [30..30] SA - Source address */
            __IOM uint32_t AE                  : 1; /*!< [31..31] Address error - Indicates address alignment error */
        } MACA2HR_f;
    };
    union {
        __IOM uint32_t MACA2LR             ; /*!< Offset: 0x0054 (read-write) MAC address 2 low register - MAC address 2 low register */
        struct {
            __IOM uint32_t MACA2L              : 32; /*!< [31..0] MAC address 2 low - MAC address 2 low 32 bits */
        } MACA2LR_f;
    };
    union {
        __IOM uint32_t MACA3HR             ; /*!< Offset: 0x0058 (read-write) MAC address 3 high register - MAC address 3 high register */
        struct {
            __IOM uint32_t MACA3H              : 16; /*!< [15..0] MAC address 3 high - MAC address 3 high 16 bits */
            __IM  uint32_t          : 8;
            __IOM uint32_t MBC                 : 6; /*!< [29..24] MBC - Mailbox code */
            __IOM uint32_t SA                  : 1; /*!< [30..30] SA - Source address */
            __IOM uint32_t AE                  : 1; /*!< [31..31] Address error - Indicates address alignment error */
        } MACA3HR_f;
    };
    union {
        __IOM uint32_t MACA3LR             ; /*!< Offset: 0x005C (read-write) MAC address 3 low register - MAC address 3 low register */
        struct {
            __IOM uint32_t MACA3L              : 32; /*!< [31..0] MAC address 3 low - MAC address 3 low 32 bits */
        } MACA3LR_f;
    };
    __IO uint32_t RESERVED1[31];
    union {
        __IOM uint32_t MACWTR              ; /*!< Offset: 0x00DC (read-write) MAC watchdog timeout register - MAC watchdog timeout */
        struct {
            __IOM uint32_t WTO                 : 14; /*!< [13..0] WTO - WTO value */
            __IM  uint32_t          : 2;
            __IOM uint32_t PWE                 : 1; /*!< [16..16] PWE - PWE value */
            __IM  uint32_t          : 15;
        } MACWTR_f;
    };
    __IO uint32_t RESERVED2[8];
    union {
        __IOM uint32_t MMCCR               ; /*!< Offset: 0x0100 (read-write) MMC control register - Ethernet MMC control */
        struct {
            __IOM uint32_t CNTRST              : 1; /*!< [0..0] Counter reset - Resets counter to zero */
            __IOM uint32_t CNTSTOPRO           : 1; /*!< [1..1] Counter stop read-only - Counter value is read-only when stopped */
            __IOM uint32_t RSTONRD             : 1; /*!< [2..2] RSTONRD - RSTONRD value */
            __IOM uint32_t CNTFREEZ            : 1; /*!< [3..3] Counter freeze - Freezes counter value for debug */
            __IOM uint32_t CNTPRST             : 1; /*!< [4..4] Counter preset - Presets counter to initial value */
            __IOM uint32_t CNTPRSTLVL          : 1; /*!< [5..5] Counter preset level - Level for counter preset */
            __IM  uint32_t          : 2;
            __IOM uint32_t UCDBC               : 1; /*!< [8..8] UCDBC - UCDBC value */
            __IM  uint32_t          : 23;
        } MMCCR_f;
    };
    union {
        __IM  uint32_t MMCRIR              ; /*!< Offset: 0x0104 (read-only) MMC receive interrupt register - Ethernet MMC receive interrupt */
        struct {
            __IM  uint32_t          : 5;
            __IM  uint32_t RFCEIS              : 1; /*!< [5..5] Receive frame count error interrupt status - Frame count error int status */
            __IM  uint32_t RFAEIS              : 1; /*!< [6..6] Receive FCS error interrupt status - RX FCS error interrupt status */
            __IM  uint32_t          : 10;
            __IM  uint32_t RGUFIS              : 1; /*!< [17..17] Receive good unicast frames interrupt status - Good unicast int status */
            __IM  uint32_t          : 14;
        } MMCRIR_f;
    };
    union {
        __IM  uint32_t MMCTIR              ; /*!< Offset: 0x0108 (read-only) MMC transmit interrupt register - Ethernet MMC transmit interrupt */
        struct {
            __IM  uint32_t          : 14;
            __IM  uint32_t TSCGFCIS            : 1; /*!< [14..14] TSCGFCIS - TSCGFCIS value */
            __IM  uint32_t TMCGFCIS            : 1; /*!< [15..15] TMCGFCIS - TMCGFCIS value */
            __IM  uint32_t          : 5;
            __IM  uint32_t TGFIS               : 1; /*!< [21..21] TGFIS - TGFIS value */
            __IM  uint32_t          : 10;
        } MMCTIR_f;
    };
    union {
        __IOM uint32_t MMCRIMR             ; /*!< Offset: 0x010C (read-write) MMC receive interrupt mask register - Ethernet MMC receive mask */
        struct {
            __IM  uint32_t          : 5;
            __IOM uint32_t RFCEIM              : 1; /*!< [5..5] Receive frame count error interrupt mask - Masks frame count error int */
            __IOM uint32_t RFAEIM              : 1; /*!< [6..6] Receive FCS error interrupt mask - Masks RX FCS error interrupt */
            __IM  uint32_t          : 10;
            __IOM uint32_t RGUFIM              : 1; /*!< [17..17] Receive good unicast frames interrupt mask - Masks good unicast int */
            __IM  uint32_t          : 14;
        } MMCRIMR_f;
    };
    union {
        __IOM uint32_t MMCTIMR             ; /*!< Offset: 0x0110 (read-write) MMC transmit interrupt mask register - Ethernet MMC transmit mask */
        struct {
            __IM  uint32_t          : 14;
            __IOM uint32_t TSCGFCIM            : 1; /*!< [14..14] TSCGFCIM - TSCGFCIM value */
            __IOM uint32_t TMCGFCIM            : 1; /*!< [15..15] TMCGFCIM - TMCGFCIM value */
            __IM  uint32_t          : 5;
            __IOM uint32_t TGFIM               : 1; /*!< [21..21] TGFIM - TGFIM value */
            __IM  uint32_t          : 10;
        } MMCTIMR_f;
    };
    __IO uint32_t RESERVED3[14];
    union {
        __IM  uint32_t MMCTGFSCCR          ; /*!< Offset: 0x014C (read-only) MMC TX good frames SC counter - MMC TX good single collision */
        struct {
            __IM  uint32_t TGFSCC              : 32; /*!< [31..0] TGFSCC - TGFSCC value */
        } MMCTGFSCCR_f;
    };
    union {
        __IM  uint32_t MMCTGFMCCR          ; /*!< Offset: 0x0150 (read-only) MMC TX good frames MC counter - MMC TX good multicast count */
        struct {
            __IM  uint32_t TGFMCC              : 32; /*!< [31..0] TGFMCC - TGFMCC value */
        } MMCTGFMCCR_f;
    };
    __IO uint32_t RESERVED4[5];
    union {
        __IM  uint32_t MMCTGFCR            ; /*!< Offset: 0x0168 (read-only) MMC TX good frames counter - MMC TX good frames count */
        struct {
            __IM  uint32_t TGFC                : 32; /*!< [31..0] TGFC - TGFC value */
        } MMCTGFCR_f;
    };
    __IO uint32_t RESERVED5[10];
    union {
        __IM  uint32_t MMCRFCECR           ; /*!< Offset: 0x0194 (read-only) MMC RX frame count error counter - MMC receive frame count error */
        struct {
            __IM  uint32_t RFCEC               : 32; /*!< [31..0] Receive frame count error counter - RX frame count error count */
        } MMCRFCECR_f;
    };
    union {
        __IM  uint32_t MMCRFAECR           ; /*!< Offset: 0x0198 (read-only) MMC RX FCS error counter - MMC receive FCS error count */
        struct {
            __IM  uint32_t RFAEC               : 32; /*!< [31..0] Receive FCS error counter - Receive frame check sequence error counter */
        } MMCRFAECR_f;
    };
    __IO uint32_t RESERVED6[10];
    union {
        __IOM uint32_t MMCRGUFCR           ; /*!< Offset: 0x01C4 (read-write) MMC RX good unicast frames counter - Good unicast frames count */
        struct {
            __IOM uint32_t RGUFC               : 32; /*!< [31..0] Receive good unicast frames counter - Good unicast frame receive count */
        } MMCRGUFCR_f;
    };
    __IO uint32_t RESERVED7[142];
    union {
        __IOM uint32_t MACL3L4C0R          ; /*!< Offset: 0x0400 (read-write) MAC L3/L4 control register 0 - Layer 3/4 filter control 0 */
        struct {
            __IOM uint32_t L3PEN0              : 1; /*!< [0..0] L3PEN0 - MAC Layer 3 port enable 0 */
            __IM  uint32_t          : 1;
            __IOM uint32_t L3SAM0              : 1; /*!< [2..2] L3SAM0 - MAC Layer 3 source addr mask 0 */
            __IOM uint32_t L3SAIM0             : 1; /*!< [3..3] L3SAIM0 - MAC Layer 3 source addr IP match 0 */
            __IOM uint32_t L3DAM0              : 1; /*!< [4..4] L3DAM0 - MAC Layer 3 destination addr mask 0 */
            __IOM uint32_t L3DAIM0             : 1; /*!< [5..5] L3DAIM0 - MAC Layer 3 dest addr IP match 0 */
            __IOM uint32_t L3HSBM0             : 5; /*!< [10..6] L3HSBM0 - MAC Layer 3 half-byte status base match 0 */
            __IOM uint32_t L3HDBM0             : 5; /*!< [15..11] L3HDBM0 - MAC Layer 3 half-byte data base match 0 */
            __IOM uint32_t L4PEN0              : 1; /*!< [16..16] L4PEN0 - MAC Layer 4 port enable 0 */
            __IM  uint32_t          : 1;
            __IOM uint32_t L4SPM0              : 1; /*!< [18..18] L4SPM0 - MAC Layer 4 source port mask 0 */
            __IOM uint32_t L4SPIM0             : 1; /*!< [19..19] L4SPIM0 - MAC Layer 4 source port inv mask 0 */
            __IOM uint32_t L4DPM0              : 1; /*!< [20..20] L4DPM0 - MAC Layer 4 destination port mask 0 */
            __IOM uint32_t L4DPIM0             : 1; /*!< [21..21] L4DPIM0 - MAC Layer 4 dest port inv mask 0 */
            __IM  uint32_t          : 10;
        } MACL3L4C0R_f;
    };
    union {
        __IOM uint32_t MACL4A0R            ; /*!< Offset: 0x0404 (read-write) MACL4A0R - MAC Layer 3/4 address register */
        struct {
            __IOM uint32_t L4SP0               : 16; /*!< [15..0] L4SP0 - MAC Layer 4 source port 0 */
            __IOM uint32_t L4DP0               : 16; /*!< [31..16] L4DP0 - MAC Layer 4 destination port 0 */
        } MACL4A0R_f;
    };
    __IO uint32_t RESERVED8[2];
    union {
        __IOM uint32_t MACL3A00R           ; /*!< Offset: 0x0410 (read-write) MACL3A00R - MAC Layer 3/4 address register */
        struct {
            __IOM uint32_t L3A00               : 32; /*!< [31..0] L3A00 - MAC Layer 3 address 0 byte 0 */
        } MACL3A00R_f;
    };
    union {
        __IOM uint32_t MACL3A10R           ; /*!< Offset: 0x0414 (read-write) MACL3A10R - MAC Layer 3/4 address register */
        struct {
            __IOM uint32_t L3A10               : 32; /*!< [31..0] L3A10 - MAC Layer 3 address 1 byte 0 */
        } MACL3A10R_f;
    };
    union {
        __IOM uint32_t MACL3A20R           ; /*!< Offset: 0x0418 (read-write) MACL3A20R - MAC Layer 3/4 address register */
        struct {
            __IOM uint32_t L3A20               : 32; /*!< [31..0] L3A20 - MAC Layer 3 address 2 byte 0 */
        } MACL3A20R_f;
    };
    union {
        __IOM uint32_t MACL3A30R           ; /*!< Offset: 0x041C (read-write) MACL3A30R - MAC Layer 3/4 address register */
        struct {
            __IOM uint32_t L3A30               : 32; /*!< [31..0] L3A30 - MAC Layer 3 address 3 byte 0 */
        } MACL3A30R_f;
    };
    __IO uint32_t RESERVED9[4];
    union {
        __IOM uint32_t MACL3L4C1R          ; /*!< Offset: 0x0430 (read-write) MAC L3/L4 control register 1 - Layer 3/4 filter control 1 */
        struct {
            __IOM uint32_t L3PEN1              : 1; /*!< [0..0] L3PEN1 - MAC Layer 3 port enable 1 */
            __IM  uint32_t          : 1;
            __IOM uint32_t L3SAM1              : 1; /*!< [2..2] L3SAM1 - MAC Layer 3 source addr mask 1 */
            __IOM uint32_t L3SAIM1             : 1; /*!< [3..3] L3SAIM1 - MAC Layer 3 source addr IP match 1 */
            __IOM uint32_t L3DAM1              : 1; /*!< [4..4] L3DAM1 - MAC Layer 3 destination addr mask 1 */
            __IOM uint32_t L3DAIM1             : 1; /*!< [5..5] L3DAIM1 - MAC Layer 3 dest addr IP match 1 */
            __IOM uint32_t L3HSBM1             : 5; /*!< [10..6] L3HSBM1 - MAC Layer 3 half-byte status base match 1 */
            __IOM uint32_t L3HDBM1             : 5; /*!< [15..11] L3HDBM1 - MAC Layer 3 half-byte data base match 1 */
            __IOM uint32_t L4PEN1              : 1; /*!< [16..16] L4PEN1 - MAC Layer 4 port enable 1 */
            __IM  uint32_t          : 1;
            __IOM uint32_t L4SPM1              : 1; /*!< [18..18] L4SPM1 - MAC Layer 4 source port mask 1 */
            __IOM uint32_t L4SPIM1             : 1; /*!< [19..19] L4SPIM1 - MAC Layer 4 source port inv mask 1 */
            __IOM uint32_t L4DPM1              : 1; /*!< [20..20] L4DPM1 - MAC Layer 4 destination port mask 1 */
            __IOM uint32_t L4DPIM1             : 1; /*!< [21..21] L4DPIM1 - MAC Layer 4 dest port inv mask 1 */
            __IM  uint32_t          : 10;
        } MACL3L4C1R_f;
    };
    union {
        __IOM uint32_t MACL4A1R            ; /*!< Offset: 0x0434 (read-write) MACL4A1R - MAC Layer 3/4 address register */
        struct {
            __IOM uint32_t L4SP1               : 16; /*!< [15..0] L4SP1 - MAC Layer 4 source port 1 */
            __IOM uint32_t L4DP1               : 16; /*!< [31..16] L4DP1 - MAC Layer 4 destination port 1 */
        } MACL4A1R_f;
    };
    __IO uint32_t RESERVED10[2];
    union {
        __IOM uint32_t MACL3A01R           ; /*!< Offset: 0x0440 (read-write) MACL3A01R - MAC Layer 3/4 address register */
        struct {
            __IOM uint32_t L3A01               : 32; /*!< [31..0] L3A01 - MAC Layer 3 address 0 byte 1 */
        } MACL3A01R_f;
    };
    union {
        __IOM uint32_t MACL3A11R           ; /*!< Offset: 0x0444 (read-write) MACL3A11R - MAC Layer 3/4 address register */
        struct {
            __IOM uint32_t L3A11               : 32; /*!< [31..0] L3A11 - MAC Layer 3 address 1 byte 1 */
        } MACL3A11R_f;
    };
    union {
        __IOM uint32_t MACL3A21R           ; /*!< Offset: 0x0448 (read-write) MACL3A21R - MAC Layer 3/4 address register */
        struct {
            __IOM uint32_t L3A21               : 32; /*!< [31..0] L3A21 - MAC Layer 3 address 2 byte 1 */
        } MACL3A21R_f;
    };
    union {
        __IOM uint32_t MACL3A31R           ; /*!< Offset: 0x044C (read-write) MACL3A31R - MAC Layer 3/4 address register */
        struct {
            __IOM uint32_t L3A31               : 32; /*!< [31..0] L3A31 - MAC Layer 3 address 3 byte 1 */
        } MACL3A31R_f;
    };
    __IO uint32_t RESERVED11[77];
    union {
        __IOM uint32_t MACVTIRR            ; /*!< Offset: 0x0584 (read-write) MAC VLAN tag inclusion/reflection register - VLAN tag config */
        struct {
            __IOM uint32_t VLT                 : 16; /*!< [15..0] VLT - VLT value */
            __IOM uint32_t VLC                 : 2; /*!< [17..16] VLC - VLC value */
            __IOM uint32_t VLP                 : 1; /*!< [18..18] VLP - VLP value */
            __IOM uint32_t CSVL                : 1; /*!< [19..19] Clock security violation lock - Locks CSS detection status */
            __IM  uint32_t          : 12;
        } MACVTIRR_f;
    };
    union {
        __IOM uint32_t MACVHTR             ; /*!< Offset: 0x0588 (read-write) MAC VLAN hash table register - VLAN hash table */
        struct {
            __IOM uint32_t VLHT                : 16; /*!< [15..0] VLHT - VLHT value */
            __IM  uint32_t          : 16;
        } MACVHTR_f;
    };
    __IO uint32_t RESERVED12[93];
    union {
        __IOM uint32_t PTPTSCR             ; /*!< Offset: 0x0700 (read-write) PTP timestamp control - PTP timestamp control */
        struct {
            __IOM uint32_t TE                  : 1; /*!< [0..0] Transmitter enable - Enables UART transmitter */
            __IOM uint32_t TFCU                : 1; /*!< [1..1] TFCU - TFCU value */
            __IOM uint32_t TI                  : 1; /*!< [2..2] TI - TI value */
            __IOM uint32_t TU                  : 1; /*!< [3..3] TU - TU value */
            __IOM uint32_t TITE                : 1; /*!< [4..4] TITE - TITE value */
            __IOM uint32_t TARU                : 1; /*!< [5..5] TARU - TARU value */
            __IM  uint32_t          : 2;
            __IOM uint32_t TSARFE              : 1; /*!< [8..8] TSARFE - TSARFE value */
            __IOM uint32_t TSR                 : 1; /*!< [9..9] Timestamp register - RTC timestamp value */
            __IOM uint32_t TSPTPPSV2E          : 1; /*!< [10..10] TS PTP PPS v2 enable - Enables PTP PPS v2 mode */
            __IOM uint32_t EPPEF               : 1; /*!< [11..11] Endpoint packet error flag - USB EP error flag */
            __IOM uint32_t EPPFSIP6U           : 1; /*!< [12..12] EP packet frame/SIP6U - USB EP frame status */
            __IOM uint32_t EPPFSIP4U           : 1; /*!< [13..13] EP packet frame/SIP4U - USB EP frame status */
            __IOM uint32_t ETSFEM              : 1; /*!< [14..14] Ethernet timestamp filter mode - Timestamp filter mode */
            __IOM uint32_t ESFMRTM             : 1; /*!< [15..15] ESFM RTM - ESFM real-time monitor */
            __IOM uint32_t SPPFTS              : 2; /*!< [17..16] SPPFTS - SPPFTS value */
            __IOM uint32_t EMAFPFF             : 1; /*!< [18..18] EMA frame format - EMA frame format selection */
            __IM  uint32_t          : 5;
            __IOM uint32_t ASFC                : 1; /*!< [24..24] Analog switch fast charge - Enables fast charge for analog switch */
            __IOM uint32_t ASEN0               : 1; /*!< [25..25] Analog switch enable 0 - Enables analog switch for IO 0 */
            __IOM uint32_t ASEN1               : 1; /*!< [26..26] Analog switch enable 1 - Enables analog switch for IO 1 */
            __IOM uint32_t ASEN2               : 1; /*!< [27..27] Analog switch enable 2 - Enables analog switch for IO 2 */
            __IOM uint32_t ASEN3               : 1; /*!< [28..28] Analog switch enable 3 - Enables analog switch for IO 3 */
            __IM  uint32_t          : 3;
        } PTPTSCR_f;
    };
    union {
        __IOM uint32_t PTPSSIR             ; /*!< Offset: 0x0704 (read-write) PTP subsecond increment - PTP subsecond increment */
        struct {
            __IOM uint32_t SSIV                : 8; /*!< [7..0] SSIV - SSIV value */
            __IM  uint32_t          : 24;
        } PTPSSIR_f;
    };
    union {
        __IM  uint32_t PTPTSHR             ; /*!< Offset: 0x0708 (read-only) PTP timestamp high - PTP timestamp high bits */
        struct {
            __IM  uint32_t TS                  : 32; /*!< [31..0] Trigger selection - Selects trigger input for slave mode */
        } PTPTSHR_f;
    };
    union {
        __IM  uint32_t PTPTSLR             ; /*!< Offset: 0x070C (read-only) PTP timestamp low - PTP timestamp low bits */
        struct {
            __IM  uint32_t TSS                 : 31; /*!< [30..0] TSS - TSS value */
            __IM  uint32_t          : 1;
        } PTPTSLR_f;
    };
    union {
        __IOM uint32_t PTPTSHUR            ; /*!< Offset: 0x0710 (read-write) PTP timestamp high update - PTP update high */
        struct {
            __IOM uint32_t TSU                 : 32; /*!< [31..0] TSU - TSU value */
        } PTPTSHUR_f;
    };
    union {
        __IOM uint32_t PTPTSLUR            ; /*!< Offset: 0x0714 (read-write) PTP timestamp low update - PTP update low */
        struct {
            __IOM uint32_t TSSU                : 31; /*!< [30..0] TSSU - TSSU value */
            __IOM uint32_t AST                 : 1; /*!< [31..31] Analog switch timing - Configures analog switch timing */
        } PTPTSLUR_f;
    };
    union {
        __IOM uint32_t PTPTSAR             ; /*!< Offset: 0x0718 (read-write) PTP timestamp addend - PTP timestamp addend */
        struct {
            __IOM uint32_t TAR                 : 32; /*!< [31..0] TAR - TAR value */
        } PTPTSAR_f;
    };
    union {
        __IOM uint32_t PTPTTHR             ; /*!< Offset: 0x071C (read-write) PTP target time high - PTP target high */
        struct {
            __IOM uint32_t TTSR                : 32; /*!< [31..0] TTSR - TTSR value */
        } PTPTTHR_f;
    };
    union {
        __IOM uint32_t PTPTTLR             ; /*!< Offset: 0x0720 (read-write) PTP target time low - PTP target low */
        struct {
            __IOM uint32_t TTLR                : 31; /*!< [30..0] TTLR - TTLR value */
            __IOM uint32_t TTRB                : 1; /*!< [31..31] TTRB - TTRB value */
        } PTPTTLR_f;
    };
    __IO uint32_t RESERVED13;
    union {
        __IM  uint32_t PTPTSSR             ; /*!< Offset: 0x0728 (read-only) PTP timestamp status register - PTP timestamp status */
        struct {
            __IM  uint32_t TSO                 : 1; /*!< [0..0] TSO - TSO value */
            __IM  uint32_t TTTR                : 1; /*!< [1..1] TTTR - TTTR value */
            __IM  uint32_t ATTS                : 1; /*!< [2..2] Analog test signal - Analog test signal configuration */
            __IM  uint32_t TTTE                : 1; /*!< [3..3] TTTE - TTTE value */
            __IM  uint32_t          : 12;
            __IM  uint32_t ATSTI               : 4; /*!< [19..16] Analog test input - Analog test input selection */
            __IM  uint32_t          : 4;
            __IM  uint32_t ATSTM               : 1; /*!< [24..24] Analog test mode - Enables analog test mode */
            __IM  uint32_t ATSNS               : 3; /*!< [27..25] Analog temperature sensor - Analog temperature sensor control */
            __IM  uint32_t          : 4;
        } PTPTSSR_f;
    };
    union {
        __IOM uint32_t PTPPPSCR            ; /*!< Offset: 0x072C (read-write) PTP PPS control register - PTP PPS control */
        struct {
            __IOM uint32_t PPSCTRL             : 4; /*!< [3..0] PPS control - PPS control register */
            __IOM uint32_t PPSEN               : 1; /*!< [4..4] PPS enable - Enables PPS output */
            __IOM uint32_t TRGTMODSEL          : 2; /*!< [6..5] Trigger mode selection - Trigger operating mode selection */
            __IM  uint32_t          : 25;
        } PTPPPSCR_f;
    };
    union {
        __IM  uint32_t PTPATSNR            ; /*!< Offset: 0x0730 (read-only) PTP auxiliary timestamp nanoseconds - PTP aux timestamp ns */
        struct {
            __IM  uint32_t AUSTSLO             : 31; /*!< [30..0] Auto-shutdown low - Low threshold for auto-shutdown */
            __IM  uint32_t          : 1;
        } PTPATSNR_f;
    };
    union {
        __IM  uint32_t PTPATSSR            ; /*!< Offset: 0x0734 (read-only) PTP auxiliary timestamp seconds - PTP aux timestamp s */
        struct {
            __IM  uint32_t AUSTSHI             : 31; /*!< [30..0] Auto-shutdown high - High threshold for auto-shutdown */
            __IM  uint32_t          : 1;
        } PTPATSSR_f;
    };
    __IO uint32_t RESERVED14[10];
    union {
        __IOM uint32_t PTPPPSIR            ; /*!< Offset: 0x0760 (read-write) PTP PPS interval register - PTP PPS interval */
        struct {
            __IOM uint32_t PPSINT              : 32; /*!< [31..0] PPS interrupt - PPS interrupt control */
        } PTPPPSIR_f;
    };
    union {
        __IOM uint32_t PTPPPSWR            ; /*!< Offset: 0x0764 (read-write) PTP PPS width register - PTP PPS width */
        struct {
            __IOM uint32_t PPSWIDTH            : 32; /*!< [31..0] PPS width - PPS pulse width */
        } PTPPPSWR_f;
    };
    __IO uint32_t RESERVED15[550];
    union {
        __IOM uint32_t DMABMR              ; /*!< Offset: 0x1000 (read-write) DMA bus master register - DMA bus master configuration */
        struct {
            __IOM uint32_t SWR                 : 1; /*!< [0..0] SWR - SWR value */
            __IOM uint32_t DA                  : 1; /*!< [1..1] Device address - Device address for I2C/SPI */
            __IOM uint32_t DSL                 : 5; /*!< [6..2] Data select - Data source selection */
            __IOM uint32_t EDFE                : 1; /*!< [7..7] Edge filter enable - Enables edge filter */
            __IOM uint32_t PBL                 : 6; /*!< [13..8] PBL - Programmable burst length */
            __IOM uint32_t PM                  : 2; /*!< [15..14] AM/PM indicator - Indicates AM or PM in 12-hour mode */
            __IOM uint32_t FB                  : 1; /*!< [16..16] Feedback - Feedback control */
            __IOM uint32_t RDP                 : 6; /*!< [22..17] RDP - RDP value */
            __IOM uint32_t USP                 : 1; /*!< [23..23] USP - USP value */
            __IOM uint32_t EPM                 : 1; /*!< [24..24] Endpoint mode - USB endpoint mode select */
            __IOM uint32_t AAB                 : 1; /*!< [25..25] Alarm A binary - Alarm A counter value in binary format */
            __IOM uint32_t MB                  : 1; /*!< [26..26] Mailbox - Mailbox register */
            __IOM uint32_t TXPR                : 1; /*!< [27..27] TX priority - Transmit priority level */
            __IM  uint32_t          : 3;
            __IOM uint32_t RIB                 : 1; /*!< [31..31] RIB - RIB value */
        } DMABMR_f;
    };
    union {
        __IM  uint32_t DMATPDR             ; /*!< Offset: 0x1004 (read-only) DMA transmit poll demand register - Transmit poll demand */
        struct {
            __IM  uint32_t TPD                 : 32; /*!< [31..0] TPD - TPD value */
        } DMATPDR_f;
    };
    union {
        __IM  uint32_t DMARPDR             ; /*!< Offset: 0x1008 (read-only) DMA read poll demand register - Read poll demand */
        struct {
            __IM  uint32_t RPD                 : 32; /*!< [31..0] RPD - RPD value */
        } DMARPDR_f;
    };
    union {
        __IOM uint32_t DMARDLAR            ; /*!< Offset: 0x100C (read-write) DMA read list address register - DMA read linked list address */
        struct {
            __IOM uint32_t SRL                 : 32; /*!< [31..0] SRL - SRL value */
        } DMARDLAR_f;
    };
    union {
        __IOM uint32_t DMATDLAR            ; /*!< Offset: 0x1010 (read-write) DMA transmit list address register - Transmit linked list address */
        struct {
            __IOM uint32_t STL                 : 32; /*!< [31..0] STL - STL value */
        } DMATDLAR_f;
    };
    union {
        __IM  uint32_t DMASR               ; /*!< Offset: 0x1014 (read-only) DMA status register - DMA status flags */
        struct {
            __IM  uint32_t TS                  : 1; /*!< [0..0] Trigger selection - Selects trigger input for slave mode */
            __IM  uint32_t TPSS                : 1; /*!< [1..1] TPSS - TPSS value */
            __IM  uint32_t TBUS                : 1; /*!< [2..2] TBUS - TBUS value */
            __IM  uint32_t TJTS                : 1; /*!< [3..3] TJTS - TJTS value */
            __IM  uint32_t ROS                 : 1; /*!< [4..4] ROS - ROS value */
            __IM  uint32_t TUS                 : 1; /*!< [5..5] TUS - TUS value */
            __IM  uint32_t RS                  : 1; /*!< [6..6] RS - RS value */
            __IM  uint32_t RBUS                : 1; /*!< [7..7] RBUS - RBUS status */
            __IM  uint32_t RPSS                : 1; /*!< [8..8] RPSS - RPSS value */
            __IM  uint32_t RWTS                : 1; /*!< [9..9] RWTS - RWTS value */
            __IM  uint32_t ETS                 : 1; /*!< [10..10] Event source - Event source selection */
            __IM  uint32_t          : 2;
            __IM  uint32_t FBES                : 1; /*!< [13..13] FIFO buffer error status - FIFO buffer error status */
            __IM  uint32_t ERS                 : 1; /*!< [14..14] Error status - Error status flag */
            __IM  uint32_t AIS                 : 1; /*!< [15..15] Alarm A interrupt selection - Selects alarm A interrupt source */
            __IM  uint32_t NIS                 : 1; /*!< [16..16] N interrupt status - NIS */
            __IM  uint32_t RPS                 : 3; /*!< [19..17] RPS - RPS value */
            __IM  uint32_t TPS                 : 3; /*!< [22..20] TPS - TPS value */
            __IM  uint32_t EBS                 : 3; /*!< [25..23] Endpoint buffer size - USB endpoint buffer size */
            __IM  uint32_t          : 1;
            __IM  uint32_t MMCS                : 1; /*!< [27..27] MMC status - MMC status register */
            __IM  uint32_t PMTS                : 1; /*!< [28..28] PMTS - PMT status */
            __IM  uint32_t TSTS                : 1; /*!< [29..29] TSTS - TSTS value */
            __IM  uint32_t LPIS                : 1; /*!< [30..30] Low power input status - Low power input status */
            __IM  uint32_t          : 1;
        } DMASR_f;
    };
    union {
        __IOM uint32_t DMAOMR              ; /*!< Offset: 0x1018 (read-write) DMA operation mode register - DMA operation mode */
        struct {
            __IM  uint32_t          : 1;
            __IOM uint32_t SR                  : 1; /*!< [1..1] Status register - Indicates timer event flags and status */
            __IOM uint32_t OSF                 : 1; /*!< [2..2] OSF - Oscillator status flag */
            __IOM uint32_t RTC                 : 2; /*!< [4..3] RTC - RTC value */
            __IOM uint32_t DGF                 : 1; /*!< [5..5] Digital glitch filter - Digital glitch filter configuration */
            __IOM uint32_t FUGF                : 1; /*!< [6..6] FIFO underrun/general flag - FIFO underrun/general flag */
            __IOM uint32_t FEF                 : 1; /*!< [7..7] FIFO error flag - FIFO error status flag */
            __IM  uint32_t          : 5;
            __IOM uint32_t ST                  : 1; /*!< [13..13] Second tens - BCD tens digit of second value */
            __IOM uint32_t TTC                 : 3; /*!< [16..14] TTC - TTC value */
            __IM  uint32_t          : 3;
            __IOM uint32_t FTF                 : 1; /*!< [20..20] FIFO trigger flag - FT flag */
            __IOM uint32_t TSF                 : 1; /*!< [21..21] Timestamp flag - RTC timestamp event flag */
            __IM  uint32_t          : 2;
            __IOM uint32_t DFRF                : 1; /*!< [24..24] Data frame format - Data frame format selection */
            __IOM uint32_t RSF                 : 1; /*!< [25..25] RSF - RSF value */
            __IOM uint32_t DTCEFD              : 1; /*!< [26..26] DTC event flag disable - Disables DTC event flag */
            __IM  uint32_t          : 5;
        } DMAOMR_f;
    };
    union {
        __IOM uint32_t DMAIER              ; /*!< Offset: 0x101C (read-write) DMA interrupt enable register - DMA interrupt enable configuration */
        struct {
            __IOM uint32_t TIE                 : 1; /*!< [0..0] Trigger interrupt enable - Enables trigger event interrupt */
            __IOM uint32_t TPSIE               : 1; /*!< [1..1] TPSIE - TPSIE value */
            __IOM uint32_t TBUIE               : 1; /*!< [2..2] TBUIE - TBUIE value */
            __IOM uint32_t TJTIE               : 1; /*!< [3..3] TJTIE - TJTIE value */
            __IOM uint32_t ROIE                : 1; /*!< [4..4] ROIE - ROIE value */
            __IOM uint32_t TUIE                : 1; /*!< [5..5] TUIE - TUIE value */
            __IOM uint32_t RIE                 : 1; /*!< [6..6] Receive interrupt enable - Enables message reception interrupt */
            __IOM uint32_t RBUIE               : 1; /*!< [7..7] RBU interrupt enable - Enables RBU interrupt */
            __IOM uint32_t RPSIE               : 1; /*!< [8..8] RPSIE - RPSIE value */
            __IOM uint32_t RWTIE               : 1; /*!< [9..9] RWTIE - RWTIE value */
            __IOM uint32_t ETIE                : 1; /*!< [10..10] Ethernet interrupt enable - Enables Ethernet MAC interrupt */
            __IM  uint32_t          : 2;
            __IOM uint32_t FBEIE               : 1; /*!< [13..13] FIFO buffer error interrupt enable - FIFO buffer error IE */
            __IOM uint32_t ERIE                : 1; /*!< [14..14] Error interrupt enable - Error interrupt enable */
            __IOM uint32_t AISE                : 1; /*!< [15..15] Address increment skip enable - Enables skipping address increment */
            __IOM uint32_t NISE                : 1; /*!< [16..16] NISE - NISE control */
            __IM  uint32_t          : 15;
        } DMAIER_f;
    };
    union {
        __IM  uint32_t DMAMFBOCR           ; /*!< Offset: 0x1020 (read-only) DMA MFO buffer offset register - MFO buffer offset */
        struct {
            __IM  uint32_t MFC                 : 16; /*!< [15..0] Match flag clear - Clears MF */
            __IM  uint32_t OBMFC               : 1; /*!< [16..16] OB MFC - Option byte MFC */
            __IM  uint32_t OFC                 : 11; /*!< [27..17] OFC - OFC value */
            __IM  uint32_t OBFOC               : 1; /*!< [28..28] OB FOC - Option byte FOC */
            __IM  uint32_t          : 3;
        } DMAMFBOCR_f;
    };
    union {
        __IOM uint32_t DMARIWTR            ; /*!< Offset: 0x1024 (read-write) DMA read interrupt wait token register - Read interrupt wait token */
        struct {
            __IOM uint32_t RIWTC               : 8; /*!< [7..0] RIWTC - RIWTC value */
            __IM  uint32_t          : 24;
        } DMARIWTR_f;
    };
    __IO uint32_t RESERVED16[8];
    union {
        __IM  uint32_t DMACHTDR            ; /*!< Offset: 0x1048 (read-only) DMA channel table data - Channel table data */
        struct {
            __IM  uint32_t HTDAP               : 32; /*!< [31..0] HRNG test data - HRNG test data */
        } DMACHTDR_f;
    };
    union {
        __IM  uint32_t DMACHRDR            ; /*!< Offset: 0x104C (read-only) DMA channel register data - Channel register data */
        struct {
            __IM  uint32_t HRDAP               : 32; /*!< [31..0] HRNG data ready - HRNG data ready flag */
        } DMACHRDR_f;
    };
    union {
        __IM  uint32_t DMACHTBAR           ; /*!< Offset: 0x1050 (read-only) DMA channel table base address - Channel table base address */
        struct {
            __IM  uint32_t HTBAP               : 32; /*!< [31..0] HRNG test baud rate - HRNG test baud rate */
        } DMACHTBAR_f;
    };
    union {
        __IM  uint32_t DMACHRBAR           ; /*!< Offset: 0x1054 (read-only) DMA channel register base address - Channel register base address */
        struct {
            __IM  uint32_t HRBAP               : 32; /*!< [31..0] HRNG baud rate - HRNG baud rate divider */
        } DMACHRBAR_f;
    };
} ETH_TypeDef;

/**
 * @brief GPIO - GPIO Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t MD                  ; /*!< Offset: 0x00 (read-write) GPIO Mode register - Configures the operating mode for each GPIO pin: Input, General output, Alternate function or Analog */
        struct {
            __IOM uint32_t PIN0                : 2; /*!< [1..0] Mode configuration for pin 0: 00=Input, 01=General output, 10=Alternate function, 11=Analog */
            __IOM uint32_t PIN1                : 2; /*!< [3..2] Mode configuration for pin 1: 00=Input, 01=General output, 10=Alternate function, 11=Analog */
            __IOM uint32_t PIN2                : 2; /*!< [5..4] Mode configuration for pin 2: 00=Input, 01=General output, 10=Alternate function, 11=Analog */
            __IOM uint32_t PIN3                : 2; /*!< [7..6] Mode configuration for pin 3: 00=Input, 01=General output, 10=Alternate function, 11=Analog */
            __IOM uint32_t PIN4                : 2; /*!< [9..8] Mode configuration for pin 4: 00=Input, 01=General output, 10=Alternate function, 11=Analog */
            __IOM uint32_t PIN5                : 2; /*!< [11..10] Mode configuration for pin 5: 00=Input, 01=General output, 10=Alternate function, 11=Analog */
            __IOM uint32_t PIN6                : 2; /*!< [13..12] Mode configuration for pin 6: 00=Input, 01=General output, 10=Alternate function, 11=Analog */
            __IOM uint32_t PIN7                : 2; /*!< [15..14] Mode configuration for pin 7: 00=Input, 01=General output, 10=Alternate function, 11=Analog */
            __IOM uint32_t PIN8                : 2; /*!< [17..16] Mode configuration for pin 8: 00=Input, 01=General output, 10=Alternate function, 11=Analog */
            __IOM uint32_t PIN9                : 2; /*!< [19..18] Mode configuration for pin 9: 00=Input, 01=General output, 10=Alternate function, 11=Analog */
            __IOM uint32_t PIN10               : 2; /*!< [21..20] Mode configuration for pin 10: 00=Input, 01=General output, 10=Alternate function, 11=Analog */
            __IOM uint32_t PIN11               : 2; /*!< [23..22] Mode configuration for pin 11: 00=Input, 01=General output, 10=Alternate function, 11=Analog */
            __IOM uint32_t PIN12               : 2; /*!< [25..24] Mode configuration for pin 12: 00=Input, 01=General output, 10=Alternate function, 11=Analog */
            __IOM uint32_t PIN13               : 2; /*!< [27..26] Mode configuration for pin 13: 00=Input, 01=General output, 10=Alternate function, 11=Analog */
            __IOM uint32_t PIN14               : 2; /*!< [29..28] Mode configuration for pin 14: 00=Input, 01=General output, 10=Alternate function, 11=Analog */
            __IOM uint32_t PIN15               : 2; /*!< [31..30] Mode configuration for pin 15: 00=Input, 01=General output, 10=Alternate function, 11=Analog */
        } MD_f;
    };
    union {
        __IOM uint32_t OTYP                ; /*!< Offset: 0x04 (read-write) GPIO Output type register - Configures output mode as push-pull or open-drain for each GPIO pin */
        struct {
            __IOM uint32_t PIN0                : 1; /*!< [0..0] Output type for pin 0: 0=Push-pull, 1=Open-drain */
            __IOM uint32_t PIN1                : 1; /*!< [1..1] Output type for pin 1: 0=Push-pull, 1=Open-drain */
            __IOM uint32_t PIN2                : 1; /*!< [2..2] Output type for pin 2: 0=Push-pull, 1=Open-drain */
            __IOM uint32_t PIN3                : 1; /*!< [3..3] Output type for pin 3: 0=Push-pull, 1=Open-drain */
            __IOM uint32_t PIN4                : 1; /*!< [4..4] Output type for pin 4: 0=Push-pull, 1=Open-drain */
            __IOM uint32_t PIN5                : 1; /*!< [5..5] Output type for pin 5: 0=Push-pull, 1=Open-drain */
            __IOM uint32_t PIN6                : 1; /*!< [6..6] Output type for pin 6: 0=Push-pull, 1=Open-drain */
            __IOM uint32_t PIN7                : 1; /*!< [7..7] Output type for pin 7: 0=Push-pull, 1=Open-drain */
            __IOM uint32_t PIN8                : 1; /*!< [8..8] Output type for pin 8: 0=Push-pull, 1=Open-drain */
            __IOM uint32_t PIN9                : 1; /*!< [9..9] Output type for pin 9: 0=Push-pull, 1=Open-drain */
            __IOM uint32_t PIN10               : 1; /*!< [10..10] Output type for pin 10: 0=Push-pull, 1=Open-drain */
            __IOM uint32_t PIN11               : 1; /*!< [11..11] Output type for pin 11: 0=Push-pull, 1=Open-drain */
            __IOM uint32_t PIN12               : 1; /*!< [12..12] Output type for pin 12: 0=Push-pull, 1=Open-drain */
            __IOM uint32_t PIN13               : 1; /*!< [13..13] Output type for pin 13: 0=Push-pull, 1=Open-drain */
            __IOM uint32_t PIN14               : 1; /*!< [14..14] Output type for pin 14: 0=Push-pull, 1=Open-drain */
            __IOM uint32_t PIN15               : 1; /*!< [15..15] Output type for pin 15: 0=Push-pull, 1=Open-drain */
            __IM  uint32_t          : 16;
        } OTYP_f;
    };
    union {
        __IOM uint32_t PUPD                ; /*!< Offset: 0x08 (read-write) GPIO Pull-up/Pull-down register - Configures pull-up/pull-down resistors for each GPIO pin */
        struct {
            __IOM uint32_t PIN0                : 2; /*!< [1..0] Pull-up/pull-down for pin 0: 00=No pull, 01=Pull-up, 10=Pull-down, 11=Reserved */
            __IOM uint32_t PIN1                : 2; /*!< [3..2] Pull-up/pull-down for pin 1: 00=No pull, 01=Pull-up, 10=Pull-down, 11=Reserved */
            __IOM uint32_t PIN2                : 2; /*!< [5..4] Pull-up/pull-down for pin 2: 00=No pull, 01=Pull-up, 10=Pull-down, 11=Reserved */
            __IOM uint32_t PIN3                : 2; /*!< [7..6] Pull-up/pull-down for pin 3: 00=No pull, 01=Pull-up, 10=Pull-down, 11=Reserved */
            __IOM uint32_t PIN4                : 2; /*!< [9..8] Pull-up/pull-down for pin 4: 00=No pull, 01=Pull-up, 10=Pull-down, 11=Reserved */
            __IOM uint32_t PIN5                : 2; /*!< [11..10] Pull-up/pull-down for pin 5: 00=No pull, 01=Pull-up, 10=Pull-down, 11=Reserved */
            __IOM uint32_t PIN6                : 2; /*!< [13..12] Pull-up/pull-down for pin 6: 00=No pull, 01=Pull-up, 10=Pull-down, 11=Reserved */
            __IOM uint32_t PIN7                : 2; /*!< [15..14] Pull-up/pull-down for pin 7: 00=No pull, 01=Pull-up, 10=Pull-down, 11=Reserved */
            __IOM uint32_t PIN8                : 2; /*!< [17..16] Pull-up/pull-down for pin 8: 00=No pull, 01=Pull-up, 10=Pull-down, 11=Reserved */
            __IOM uint32_t PIN9                : 2; /*!< [19..18] Pull-up/pull-down for pin 9: 00=No pull, 01=Pull-up, 10=Pull-down, 11=Reserved */
            __IOM uint32_t PIN10               : 2; /*!< [21..20] Pull-up/pull-down for pin 10: 00=No pull, 01=Pull-up, 10=Pull-down, 11=Reserved */
            __IOM uint32_t PIN11               : 2; /*!< [23..22] Pull-up/pull-down for pin 11: 00=No pull, 01=Pull-up, 10=Pull-down, 11=Reserved */
            __IOM uint32_t PIN12               : 2; /*!< [25..24] Pull-up/pull-down for pin 12: 00=No pull, 01=Pull-up, 10=Pull-down, 11=Reserved */
            __IOM uint32_t PIN13               : 2; /*!< [27..26] Pull-up/pull-down for pin 13: 00=No pull, 01=Pull-up, 10=Pull-down, 11=Reserved */
            __IOM uint32_t PIN14               : 2; /*!< [29..28] Pull-up/pull-down for pin 14: 00=No pull, 01=Pull-up, 10=Pull-down, 11=Reserved */
            __IOM uint32_t PIN15               : 2; /*!< [31..30] Pull-up/pull-down for pin 15: 00=No pull, 01=Pull-up, 10=Pull-down, 11=Reserved */
        } PUPD_f;
    };
    union {
        __IM  uint32_t IDATA               ; /*!< Offset: 0x0C (read-only) GPIO Input data register - Reflects the logic value of each GPIO pin when configured as input */
        struct {
            __IM  uint32_t PIN0                : 1; /*!< [0..0] Input data for pin 0 */
            __IM  uint32_t PIN1                : 1; /*!< [1..1] Input data for pin 1 */
            __IM  uint32_t PIN2                : 1; /*!< [2..2] Input data for pin 2 */
            __IM  uint32_t PIN3                : 1; /*!< [3..3] Input data for pin 3 */
            __IM  uint32_t PIN4                : 1; /*!< [4..4] Input data for pin 4 */
            __IM  uint32_t PIN5                : 1; /*!< [5..5] Input data for pin 5 */
            __IM  uint32_t PIN6                : 1; /*!< [6..6] Input data for pin 6 */
            __IM  uint32_t PIN7                : 1; /*!< [7..7] Input data for pin 7 */
            __IM  uint32_t PIN8                : 1; /*!< [8..8] Input data for pin 8 */
            __IM  uint32_t PIN9                : 1; /*!< [9..9] Input data for pin 9 */
            __IM  uint32_t PIN10               : 1; /*!< [10..10] Input data for pin 10 */
            __IM  uint32_t PIN11               : 1; /*!< [11..11] Input data for pin 11 */
            __IM  uint32_t PIN12               : 1; /*!< [12..12] Input data for pin 12 */
            __IM  uint32_t PIN13               : 1; /*!< [13..13] Input data for pin 13 */
            __IM  uint32_t PIN14               : 1; /*!< [14..14] Input data for pin 14 */
            __IM  uint32_t PIN15               : 1; /*!< [15..15] Input data for pin 15 */
            __IM  uint32_t          : 16;
        } IDATA_f;
    };
    union {
        __IOM uint32_t ODATA               ; /*!< Offset: 0x10 (read-write) GPIO Output data register - Sets the output logic value for each GPIO pin when configured as output */
        struct {
            __IOM uint32_t PIN0                : 1; /*!< [0..0] Output data for pin 0 */
            __IOM uint32_t PIN1                : 1; /*!< [1..1] Output data for pin 1 */
            __IOM uint32_t PIN2                : 1; /*!< [2..2] Output data for pin 3 */
            __IOM uint32_t PIN3                : 1; /*!< [3..3] Output data for pin 3 */
            __IOM uint32_t PIN4                : 1; /*!< [4..4] Output data for pin 4 */
            __IOM uint32_t PIN5                : 1; /*!< [5..5] Output data for pin 5 */
            __IOM uint32_t PIN6                : 1; /*!< [6..6] Output data for pin 6 */
            __IOM uint32_t PIN7                : 1; /*!< [7..7] Output data for pin 7 */
            __IOM uint32_t PIN8                : 1; /*!< [8..8] Output data for pin 8 */
            __IOM uint32_t PIN9                : 1; /*!< [9..9] Output data for pin 9 */
            __IOM uint32_t PIN10               : 1; /*!< [10..10] Output data for pin 10 */
            __IOM uint32_t PIN11               : 1; /*!< [11..11] Output data for pin 11 */
            __IOM uint32_t PIN12               : 1; /*!< [12..12] Output data for pin 12 */
            __IOM uint32_t PIN13               : 1; /*!< [13..13] Output data for pin 13 */
            __IOM uint32_t PIN14               : 1; /*!< [14..14] Output data for pin 14 */
            __IOM uint32_t PIN15               : 1; /*!< [15..15] Output data for pin 15 */
            __IM  uint32_t          : 16;
        } ODATA_f;
    };
    union {
        __OM  uint32_t BSC                 ; /*!< Offset: 0x14 (write-only) GPIO Bit set/clear register - Allows atomic bit set and clear operations on output data */
        struct {
            __OM  uint32_t SET                 : 16; /*!< [15..0] Set bits for pins 15-0: Writing 1 sets the corresponding output pin high */
            __OM  uint32_t CLR                 : 16; /*!< [31..16] Clear bits for pins 15-0: Writing 1 clears the corresponding output pin low */
        } BSC_f;
    };
    union {
        __IOM uint32_t AF0                 ; /*!< Offset: 0x18 (read-write) GPIO Alternate function register 0 - Selects alternate function for pins 7-0 (AF0 to AF8) */
        struct {
            __IOM uint32_t AFSEL0              : 4; /*!< [3..0] Alternate function selection for pin 0: 0000=AF0 to 1000=AF8 */
            __IOM uint32_t AFSEL1              : 4; /*!< [7..4] Alternate function selection for pin 1: 0000=AF0 to 1000=AF8 */
            __IOM uint32_t AFSEL2              : 4; /*!< [11..8] Alternate function selection for pin 2: 0000=AF0 to 1000=AF8 */
            __IOM uint32_t AFSEL3              : 4; /*!< [15..12] Alternate function selection for pin 3: 0000=AF0 to 1000=AF8 */
            __IOM uint32_t AFSEL4              : 4; /*!< [19..16] Alternate function selection for pin 4: 0000=AF0 to 1000=AF8 */
            __IOM uint32_t AFSEL5              : 4; /*!< [23..20] Alternate function selection for pin 5: 0000=AF0 to 1000=AF8 */
            __IOM uint32_t AFSEL6              : 4; /*!< [27..24] Alternate function selection for pin 6: 0000=AF0 to 1000=AF8 */
            __IOM uint32_t AFSEL7              : 4; /*!< [31..28] Alternate function selection for pin 7: 0000=AF0 to 1000=AF8 */
        } AF0_f;
    };
    union {
        __IOM uint32_t AF1                 ; /*!< Offset: 0x1C (read-write) GPIO Alternate function register 1 - Selects alternate function for pins 15-8 (AF0 to AF8) */
        struct {
            __IOM uint32_t AFSEL8              : 4; /*!< [3..0] Alternate function selection for pin 8: 0000=AF0 to 1000=AF8 */
            __IOM uint32_t AFSEL9              : 4; /*!< [7..4] Alternate function selection for pin 9: 0000=AF0 to 1000=AF8 */
            __IOM uint32_t AFSEL10             : 4; /*!< [11..8] Alternate function selection for pin 10: 0000=AF0 to 1000=AF8 */
            __IOM uint32_t AFSEL11             : 4; /*!< [15..12] Alternate function selection for pin 11: 0000=AF0 to 1000=AF8 */
            __IOM uint32_t AFSEL12             : 4; /*!< [19..16] Alternate function selection for pin 12: 0000=AF0 to 1000=AF8 */
            __IOM uint32_t AFSEL13             : 4; /*!< [23..20] Alternate function selection for pin 13: 0000=AF0 to 1000=AF8 */
            __IOM uint32_t AFSEL14             : 4; /*!< [27..24] Alternate function selection for pin 14: 0000=AF0 to 1000=AF8 */
            __IOM uint32_t AFSEL15             : 4; /*!< [31..28] Alternate function selection for pin 15: 0000=AF0 to 1000=AF8 */
        } AF1_f;
    };
    union {
        __IOM uint32_t DS0                 ; /*!< Offset: 0x20 (read-write) GPIO Drive strength register 0 - Configures output drive strength for pins 7-0 */
        struct {
            __IOM uint32_t DS0                 : 4; /*!< [3..0] Drive strength for pin 0: 0000=2mA to 1111=16mA (FT) / 0000=11mA to 1111=32mA (TC) at VDD=3.3V */
            __IOM uint32_t DS1                 : 4; /*!< [7..4] Drive strength for pin 1: 0000=2mA to 1111=16mA (FT) / 0000=11mA to 1111=32mA (TC) at VDD=3.3V */
            __IOM uint32_t DS2                 : 4; /*!< [11..8] Drive strength for pin 2: 0000=2mA to 1111=16mA (FT) / 0000=11mA to 1111=32mA (TC) at VDD=3.3V */
            __IOM uint32_t DS3                 : 4; /*!< [15..12] Drive strength for pin 3: 0000=2mA to 1111=16mA (FT) / 0000=11mA to 1111=32mA (TC) at VDD=3.3V */
            __IOM uint32_t DS4                 : 4; /*!< [19..16] Drive strength for pin 4: 0000=2mA to 1111=16mA (FT) / 0000=11mA to 1111=32mA (TC) at VDD=3.3V */
            __IOM uint32_t DS5                 : 4; /*!< [23..20] Drive strength for pin 5: 0000=2mA to 1111=16mA (FT) / 0000=11mA to 1111=32mA (TC) at VDD=3.3V */
            __IOM uint32_t DS6                 : 4; /*!< [27..24] Drive strength for pin 6: 0000=2mA to 1111=16mA (FT) / 0000=11mA to 1111=32mA (TC) at VDD=3.3V */
            __IOM uint32_t DS7                 : 4; /*!< [31..28] Drive strength for pin 7: 0000=2mA to 1111=16mA (FT) / 0000=11mA to 1111=32mA (TC) at VDD=3.3V */
        } DS0_f;
    };
    union {
        __IOM uint32_t DS1                 ; /*!< Offset: 0x24 (read-write) GPIO Drive strength register 1 - Configures output drive strength for pins 15-8 */
        struct {
            __IOM uint32_t DS8                 : 4; /*!< [3..0] Drive strength for pin 8: 0000=2mA to 1111=16mA (FT) / 0000=11mA to 1111=32mA (TC) at VDD=3.3V */
            __IOM uint32_t DS9                 : 4; /*!< [7..4] Drive strength for pin 9: 0000=2mA to 1111=16mA (FT) / 0000=11mA to 1111=32mA (TC) at VDD=3.3V */
            __IOM uint32_t DS10                : 4; /*!< [11..8] Drive strength for pin 10: 0000=2mA to 1111=16mA (FT) / 0000=11mA to 1111=32mA (TC) at VDD=3.3V */
            __IOM uint32_t DS11                : 4; /*!< [15..12] Drive strength for pin 11: 0000=2mA to 1111=16mA (FT) / 0000=11mA to 1111=32mA (TC) at VDD=3.3V */
            __IOM uint32_t DS12                : 4; /*!< [19..16] Drive strength for pin 12: 0000=2mA to 1111=16mA (FT) / 0000=11mA to 1111=32mA (TC) at VDD=3.3V */
            __IOM uint32_t DS13                : 4; /*!< [23..20] Drive strength for pin 13: 0000=2mA to 1111=16mA (FT) / 0000=11mA to 1111=32mA (TC) at VDD=3.3V */
            __IOM uint32_t DS14                : 4; /*!< [27..24] Drive strength for pin 14: 0000=2mA to 1111=16mA (FT) / 0000=11mA to 1111=32mA (TC) at VDD=3.3V */
            __IOM uint32_t DS15                : 4; /*!< [31..28] Drive strength for pin 15: 0000=2mA to 1111=16mA (FT) / 0000=11mA to 1111=32mA (TC) at VDD=3.3V */
        } DS1_f;
    };
    union {
        __IOM uint32_t SMIT                ; /*!< Offset: 0x28 (read-write) GPIO Schmitt trigger enable register - Enables or disables the Schmitt trigger input for each GPIO pin */
        struct {
            __IOM uint32_t PIN0                : 1; /*!< [0..0] Schmitt trigger enable for pin 0: 0=Disabled, 1=Enabled */
            __IOM uint32_t PIN1                : 1; /*!< [1..1] Schmitt trigger enable for pin 1: 0=Disabled, 1=Enabled */
            __IOM uint32_t PIN2                : 1; /*!< [2..2] Schmitt trigger enable for pin 2: 0=Disabled, 1=Enabled */
            __IOM uint32_t PIN3                : 1; /*!< [3..3] Schmitt trigger enable for pin 3: 0=Disabled, 1=Enabled */
            __IOM uint32_t PIN4                : 1; /*!< [4..4] Schmitt trigger enable for pin 4: 0=Disabled, 1=Enabled */
            __IOM uint32_t PIN5                : 1; /*!< [5..5] Schmitt trigger enable for pin 5: 0=Disabled, 1=Enabled */
            __IOM uint32_t PIN6                : 1; /*!< [6..6] Schmitt trigger enable for pin 6: 0=Disabled, 1=Enabled */
            __IOM uint32_t PIN7                : 1; /*!< [7..7] Schmitt trigger enable for pin 7: 0=Disabled, 1=Enabled */
            __IOM uint32_t PIN8                : 1; /*!< [8..8] Schmitt trigger enable for pin 8: 0=Disabled, 1=Enabled */
            __IOM uint32_t PIN9                : 1; /*!< [9..9] Schmitt trigger enable for pin 9: 0=Disabled, 1=Enabled */
            __IOM uint32_t PIN10               : 1; /*!< [10..10] Schmitt trigger enable for pin 10: 0=Disabled, 1=Enabled */
            __IOM uint32_t PIN11               : 1; /*!< [11..11] Schmitt trigger enable for pin 11: 0=Disabled, 1=Enabled */
            __IOM uint32_t PIN12               : 1; /*!< [12..12] Schmitt trigger enable for pin 12: 0=Disabled, 1=Enabled */
            __IOM uint32_t PIN13               : 1; /*!< [13..13] Schmitt trigger enable for pin 13: 0=Disabled, 1=Enabled */
            __IOM uint32_t PIN14               : 1; /*!< [14..14] Schmitt trigger enable for pin 14: 0=Disabled, 1=Enabled */
            __IOM uint32_t PIN15               : 1; /*!< [15..15] Schmitt trigger enable for pin 15: 0=Disabled, 1=Enabled */
            __IM  uint32_t          : 16;
        } SMIT_f;
    };
    union {
        __IOM uint32_t LOCK                ; /*!< Offset: 0x2C (read-write) GPIO Lock register - Locks the configuration of selected GPIO pins until next reset */
        struct {
            __IOM uint32_t PIN0                : 1; /*!< [0..0] Lock enable for pin 0: Writing sequence (1->0->1->read->1) locks the corresponding pin configuration */
            __IOM uint32_t PIN1                : 1; /*!< [1..1] Lock enable for pin 1: Writing sequence (1->0->1->read->1) locks the corresponding pin configuration */
            __IOM uint32_t PIN2                : 1; /*!< [2..2] Lock enable for pin 2: Writing sequence (1->0->1->read->1) locks the corresponding pin configuration */
            __IOM uint32_t PIN3                : 1; /*!< [3..3] Lock enable for pin 3: Writing sequence (1->0->1->read->1) locks the corresponding pin configuration */
            __IOM uint32_t PIN4                : 1; /*!< [4..4] Lock enable for pin 4: Writing sequence (1->0->1->read->1) locks the corresponding pin configuration */
            __IOM uint32_t PIN5                : 1; /*!< [5..5] Lock enable for pin 5: Writing sequence (1->0->1->read->1) locks the corresponding pin configuration */
            __IOM uint32_t PIN6                : 1; /*!< [6..6] Lock enable for pin 6: Writing sequence (1->0->1->read->1) locks the corresponding pin configuration */
            __IOM uint32_t PIN7                : 1; /*!< [7..7] Lock enable for pin 7: Writing sequence (1->0->1->read->1) locks the corresponding pin configuration */
            __IOM uint32_t PIN8                : 1; /*!< [8..8] Lock enable for pin 8: Writing sequence (1->0->1->read->1) locks the corresponding pin configuration */
            __IOM uint32_t PIN9                : 1; /*!< [9..9] Lock enable for pin 9: Writing sequence (1->0->1->read->1) locks the corresponding pin configuration */
            __IOM uint32_t PIN10               : 1; /*!< [10..10] Lock enable for pin 10: Writing sequence (1->0->1->read->1) locks the corresponding pin configuration */
            __IOM uint32_t PIN11               : 1; /*!< [11..11] Lock enable for pin 11: Writing sequence (1->0->1->read->1) locks the corresponding pin configuration */
            __IOM uint32_t PIN12               : 1; /*!< [12..12] Lock enable for pin 12: Writing sequence (1->0->1->read->1) locks the corresponding pin configuration */
            __IOM uint32_t PIN13               : 1; /*!< [13..13] Lock enable for pin 13: Writing sequence (1->0->1->read->1) locks the corresponding pin configuration */
            __IOM uint32_t PIN14               : 1; /*!< [14..14] Lock enable for pin 14: Writing sequence (1->0->1->read->1) locks the corresponding pin configuration */
            __IOM uint32_t PIN15               : 1; /*!< [15..15] Lock enable for pin 15: Writing sequence (1->0->1->read->1) locks the corresponding pin configuration */
            __IOM uint32_t KEY                 : 1; /*!< [16..16] Lock key bit: Set to indicate lock is active, clear for unlocked */
            __IM  uint32_t          : 15;
        } LOCK_f;
    };
    union {
        __IOM uint32_t AIEN                ; /*!< Offset: 0x30 (read-write) GPIO Analog switch enable register - Enables or disables the analog switch for each GPIO pin in analog mode */
        struct {
            __IOM uint32_t PIN0                : 1; /*!< [0..0] Analog switch enable for pin0,0=Switch open, 1=Switch closed (valid when pin is in analog mode) */
            __IOM uint32_t PIN1                : 1; /*!< [1..1] Analog switch enable for pin1,0=Switch open, 1=Switch closed (valid when pin is in analog mode) */
            __IOM uint32_t PIN2                : 1; /*!< [2..2] Analog switch enable for pin2,0=Switch open, 1=Switch closed (valid when pin is in analog mode) */
            __IOM uint32_t PIN3                : 1; /*!< [3..3] Analog switch enable for pin3,0=Switch open, 1=Switch closed (valid when pin is in analog mode) */
            __IOM uint32_t PIN4                : 1; /*!< [4..4] Analog switch enable for pin4,0=Switch open, 1=Switch closed (valid when pin is in analog mode) */
            __IOM uint32_t PIN5                : 1; /*!< [5..5] Analog switch enable for pin5,0=Switch open, 1=Switch closed (valid when pin is in analog mode) */
            __IOM uint32_t PIN6                : 1; /*!< [6..6] Analog switch enable for pin6,0=Switch open, 1=Switch closed (valid when pin is in analog mode) */
            __IOM uint32_t PIN7                : 1; /*!< [7..7] Analog switch enable for pin7,0=Switch open, 1=Switch closed (valid when pin is in analog mode) */
            __IOM uint32_t PIN8                : 1; /*!< [8..8] Analog switch enable for pin8,0=Switch open, 1=Switch closed (valid when pin is in analog mode) */
            __IOM uint32_t PIN9                : 1; /*!< [9..9] Analog switch enable for pin9,0=Switch open, 1=Switch closed (valid when pin is in analog mode) */
            __IOM uint32_t PIN10               : 1; /*!< [10..10] Analog switch enable for pin10,0=Switch open, 1=Switch closed (valid when pin is in analog mode) */
            __IOM uint32_t PIN11               : 1; /*!< [11..11] Analog switch enable for pin11,0=Switch open, 1=Switch closed (valid when pin is in analog mode) */
            __IOM uint32_t PIN12               : 1; /*!< [12..12] Analog switch enable for pin12,0=Switch open, 1=Switch closed (valid when pin is in analog mode) */
            __IOM uint32_t PIN13               : 1; /*!< [13..13] Analog switch enable for pin13,0=Switch open, 1=Switch closed (valid when pin is in analog mode) */
            __IOM uint32_t PIN14               : 1; /*!< [14..14] Analog switch enable for pin14,0=Switch open, 1=Switch closed (valid when pin is in analog mode) */
            __IOM uint32_t PIN15               : 1; /*!< [15..15] Analog switch enable for pin15,0=Switch open, 1=Switch closed (valid when pin is in analog mode) */
            __IM  uint32_t          : 16;
        } AIEN_f;
    };
} GPIO_TypeDef;

/**
 * @brief ADC - ADC Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t SR                  ; /*!< Offset: 0x00 (read-write) Status register - Power management status flags */
        struct {
            __IOM uint32_t ADCRDY              : 1; /*!< [0..0] ADC ready flag - Set when ADC is ready for operation */
            __IOM uint32_t EOSMP               : 1; /*!< [1..1] End of sampling flag - Set when ADC sampling phase ends */
            __IOM uint32_t EOC                 : 1; /*!< [2..2] End of conversion flag - Set when ADC conversion completes */
            __IOM uint32_t EOG                 : 1; /*!< [3..3] End of regular sequence flag - Set when ADC regular group conversion completes */
            __IOM uint32_t OVERF               : 1; /*!< [4..4] Overrun flag - Set when ADC data overrun occurs */
            __IOM uint32_t JEOC                : 1; /*!< [5..5] Injected end of conversion flag - Set when injected conversion completes */
            __IOM uint32_t JEOG                : 1; /*!< [6..6] Injected end of sequence flag - Set when injected group conversion completes */
            __IOM uint32_t AWD                 : 1; /*!< [7..7] Analog watchdog - Analog watchdog status flag */
            __IOM uint32_t AFE_EOC             : 1; /*!< [8..8] AFE end of conversion - Analog front-end conversion complete */
            __IM  uint32_t          : 23;
        } SR_f;
    };
    union {
        __IOM uint32_t IE                  ; /*!< Offset: 0x04 (read-write) Interrupt enable register - Enables or disables various interrupt sources */
        struct {
            __IM  uint32_t          : 1;
            __IOM uint32_t EOSMPIE             : 1; /*!< [1..1] End of sampling interrupt enable - EOSMP interrupt enable */
            __IOM uint32_t EOCIE               : 1; /*!< [2..2] EOC interrupt enable - Enables EOC interrupt */
            __IOM uint32_t EOGIE               : 1; /*!< [3..3] End of group interrupt enable - EOG interrupt enable */
            __IOM uint32_t OVERFIE             : 1; /*!< [4..4] Overflow interrupt enable - Enables ADC overflow interrupt */
            __IOM uint32_t JEOCIE              : 1; /*!< [5..5] Injected end of conversion interrupt enable - Enables injected EOC int */
            __IOM uint32_t JEOGIE              : 1; /*!< [6..6] Injected end of group interrupt enable - Enables injected EOG int */
            __IOM uint32_t AWDIE               : 1; /*!< [7..7] Analog watchdog interrupt enable - Enables analog watchdog interrupt */
            __IM  uint32_t          : 24;
        } IE_f;
    };
    union {
        __IOM uint32_t CR1                 ; /*!< Offset: 0x08 (read-write) Control register 1 - ADC control register 1 */
        struct {
            __IOM uint32_t AWDCH               : 5; /*!< [4..0] Analog watchdog channel - Selects channel for analog watchdog */
            __IOM uint32_t AWDJCH              : 5; /*!< [9..5] Analog watchdog injected channel - Selects injected channel for watchdog */
            __IOM uint32_t AWDSGL              : 1; /*!< [10..10] Analog watchdog single - Enables single channel watchdog mode */
            __IOM uint32_t JAWDEN              : 1; /*!< [11..11] Injected analog watchdog enable - Enables injected AWD */
            __IOM uint32_t AWDEN               : 1; /*!< [12..12] Analog watchdog enable - Enables analog watchdog */
            __IOM uint32_t DMA                 : 1; /*!< [13..13] DMA - DMA control and status */
            __IOM uint32_t EXTEN               : 2; /*!< [15..14] External enable - Enables external triggering */
            __IOM uint32_t EXTSEL              : 5; /*!< [20..16] External selection - External signal selection */
            __IM  uint32_t          : 1;
            __IOM uint32_t JSWSTART            : 1; /*!< [22..22] Injected software start - Software trigger for injected group */
            __IOM uint32_t SWSTART             : 1; /*!< [23..23] SWSTART - SWSTART value */
            __IOM uint32_t CONT                : 1; /*!< [24..24] Continuous mode - Enables continuous conversion mode */
            __IOM uint32_t JAUTO               : 1; /*!< [25..25] Injected auto conversion - Auto injected conversion */
            __IOM uint32_t DISCEN              : 1; /*!< [26..26] Discontinuous enable - Enables discontinuous mode */
            __IOM uint32_t JDISCEN             : 1; /*!< [27..27] Injected discontinuous enable - Injected discontinuous mode */
            __IOM uint32_t DISCNUM             : 3; /*!< [30..28] Discontinuous mode channel count - Number of channels in discontinuous mode */
            __IM  uint32_t          : 1;
        } CR1_f;
    };
    union {
        __IOM uint32_t CR2                 ; /*!< Offset: 0x0c (read-write) Control register 2 - ADC control register 2 */
        struct {
            __IOM uint32_t ADC_EN              : 1; /*!< [0..0] ADC enable - Enables ADC conversion */
            __IOM uint32_t OVRMOD              : 1; /*!< [1..1] Overrun mode - ADC overrun behavior configuration */
            __IOM uint32_t ADC_STP             : 1; /*!< [2..2] ADC stop - ADC stop conversion control */
            __IOM uint32_t ALIGN               : 1; /*!< [3..3] Alignment - Data alignment mode (right/left aligned) */
            __IOM uint32_t RES                 : 2; /*!< [5..4] RES - RES value */
            __IOM uint32_t TRIMLDO12           : 4; /*!< [9..6] Trim LDO 1.2V - LDO 1.2V output trim value */
            __IM  uint32_t          : 6;
            __IOM uint32_t OVSE                : 1; /*!< [16..16] Oversampling enable - Enables ADC oversampling */
            __IOM uint32_t OVR                 : 3; /*!< [19..17] Oversampling ratio - ADC oversampling ratio (2x to 256x) */
            __IOM uint32_t OVSS                : 4; /*!< [23..20] Oversampling shift - ADC oversampling data shift */
            __IOM uint32_t TROVS               : 1; /*!< [24..24] TROVS - TROVS value */
            __IOM uint32_t JOVSE               : 1; /*!< [25..25] Injected oversampling enable - Injected oversampling mode */
            __IM  uint32_t          : 2;
            __IOM uint32_t ADCVREGEN           : 1; /*!< [28..28] ADC voltage regulator enable - Enables ADC internal voltage regulator */
            __IOM uint32_t ADCRSTN             : 1; /*!< [29..29] ADC reset - Resets the ADC peripheral */
            __IOM uint32_t ADCCALDIF           : 1; /*!< [30..30] ADC differential calibration - Enables differential mode calibration */
            __IOM uint32_t ADCCAL              : 1; /*!< [31..31] ADC calibration - Initiates ADC calibration when set */
        } CR2_f;
    };
    union {
        __IOM uint32_t SMPR1               ; /*!< Offset: 0x10 (read-write) Sampling time register 1 - ADC sampling time for channels */
        struct {
            __IOM uint32_t SMP0                : 4; /*!< [3..0] Sampling time 0 - Sampling time for channel 0 */
            __IOM uint32_t SMP1                : 4; /*!< [7..4] Sampling time 1 - Sampling time for channel 1 */
            __IOM uint32_t SMP2                : 4; /*!< [11..8] Sampling time 2 - Sampling time for channel 2 */
            __IOM uint32_t SMP3                : 4; /*!< [15..12] Sampling time 3 - Sampling time for channel 3 */
            __IOM uint32_t SMP4                : 4; /*!< [19..16] Sampling time 4 - Sampling time for channel 4 */
            __IOM uint32_t SMP5                : 4; /*!< [23..20] Sampling time 5 - Sampling time for channel 5 */
            __IOM uint32_t SMP6                : 4; /*!< [27..24] Sampling time 6 - Sampling time for channel 6 */
            __IOM uint32_t SMP7                : 4; /*!< [31..28] Sampling time 7 - Sampling time for channel 7 */
        } SMPR1_f;
    };
    union {
        __IOM uint32_t SMPR2               ; /*!< Offset: 0x14 (read-write) Sampling time register 2 - ADC sampling time for channels */
        struct {
            __IOM uint32_t SMP8                : 4; /*!< [3..0] Sampling time 8 - Sampling time for channel 8 */
            __IOM uint32_t SMP9                : 4; /*!< [7..4] Sampling time 9 - Sampling time for channel 9 */
            __IOM uint32_t SMP10               : 4; /*!< [11..8] Sampling time 10 - Sampling for channel 10 */
            __IOM uint32_t SMP11               : 4; /*!< [15..12] Sampling time 11 - Sampling for channel 11 */
            __IOM uint32_t SMP12               : 4; /*!< [19..16] Sampling time 12 - Sampling for channel 12 */
            __IOM uint32_t SMP13               : 4; /*!< [23..20] Sampling time 13 - Sampling for channel 13 */
            __IOM uint32_t SMP14               : 4; /*!< [27..24] Sampling time 14 - Sampling for channel 14 */
            __IOM uint32_t SMP15               : 4; /*!< [31..28] Sampling time 15 - Sampling for channel 15 */
        } SMPR2_f;
    };
    union {
        __IOM uint32_t SMPR3               ; /*!< Offset: 0x18 (read-write) Sampling time register 3 - Extended sampling time configuration */
        struct {
            __IOM uint32_t SMP16               : 4; /*!< [3..0] Sampling time 16 - Sampling for channel 16 */
            __IOM uint32_t SMP17               : 4; /*!< [7..4] Sampling time 17 - Sampling for channel 17 */
            __IOM uint32_t SMP18               : 4; /*!< [11..8] Sampling time 18 - Sampling for channel 18 */
            __IM  uint32_t          : 8;
            __IOM uint32_t SMP_PLUS            : 2; /*!< [21..20] Sampling plus - Extended sampling time addition */
            __IM  uint32_t          : 10;
        } SMPR3_f;
    };
    union {
        __IOM uint32_t HTR                 ; /*!< Offset: 0x1c (read-write) High threshold register - ADC watchdog high threshold register */
        struct {
            __IOM uint32_t HT                  : 12; /*!< [11..0] High threshold [11:0] - Single-ended channel watchdog high threshold */
            __IM  uint32_t          : 4;
            __IOM uint32_t DHT                 : 12; /*!< [27..16] Differential high threshold [11:0] - Differential channel watchdog high threshold */
            __IM  uint32_t          : 4;
        } HTR_f;
    };
    union {
        __IOM uint32_t LTR                 ; /*!< Offset: 0x20 (read-write) Low threshold register - ADC watchdog low threshold register */
        struct {
            __IOM uint32_t LT                  : 12; /*!< [11..0] Low threshold [11:0] - Single-ended channel watchdog low threshold */
            __IM  uint32_t          : 4;
            __IOM uint32_t DLT                 : 12; /*!< [27..16] Differential low threshold [11:0] - Differential channel watchdog low threshold */
            __IM  uint32_t          : 4;
        } LTR_f;
    };
    union {
        __IOM uint32_t SQR1                ; /*!< Offset: 0x24 (read-write) Sequence register 1 - Regular channel sequence 1 */
        struct {
            __IOM uint32_t L                   : 4; /*!< [3..0] L - Low/Length indicator */
            __IM  uint32_t          : 1;
            __IOM uint32_t SQ1                 : 5; /*!< [9..5] Sequence 1 - Regular channel 1 in sequence */
            __IOM uint32_t SQ2                 : 5; /*!< [14..10] Sequence 2 - Regular channel 2 in sequence */
            __IOM uint32_t SQ3                 : 5; /*!< [19..15] Sequence 3 - Regular channel 3 in sequence */
            __IOM uint32_t SQ4                 : 5; /*!< [24..20] Sequence 4 - Regular channel 4 in sequence */
            __IOM uint32_t SQ5                 : 5; /*!< [29..25] Sequence 5 - Regular channel 5 in sequence */
            __IM  uint32_t          : 2;
        } SQR1_f;
    };
    union {
        __IOM uint32_t SQR2                ; /*!< Offset: 0x28 (read-write) Sequence register 2 - Regular channel sequence 2 */
        struct {
            __IOM uint32_t SQ6                 : 5; /*!< [4..0] Sequence 6 - Regular channel 6 in sequence */
            __IOM uint32_t SQ7                 : 5; /*!< [9..5] Sequence 7 - Regular channel 7 in sequence */
            __IOM uint32_t SQ8                 : 5; /*!< [14..10] Sequence 8 - Regular channel 8 in sequence */
            __IOM uint32_t SQ9                 : 5; /*!< [19..15] Sequence 9 - Regular channel 9 in sequence */
            __IOM uint32_t SQ10                : 5; /*!< [24..20] Sequence 10 - Regular channel 10 in sequence */
            __IOM uint32_t SQ11                : 5; /*!< [29..25] Sequence 11 - Regular channel 11 in sequence */
            __IM  uint32_t          : 2;
        } SQR2_f;
    };
    union {
        __IOM uint32_t SQR3                ; /*!< Offset: 0x2c (read-write) Sequence register 3 - Regular channel sequence 3 */
        struct {
            __IOM uint32_t SQ12                : 5; /*!< [4..0] Sequence 12 - Regular channel 12 in sequence */
            __IOM uint32_t SQ13                : 5; /*!< [9..5] Sequence 13 - Regular channel 13 in sequence */
            __IOM uint32_t SQ14                : 5; /*!< [14..10] Sequence 14 - Regular channel 14 in sequence */
            __IOM uint32_t SQ15                : 5; /*!< [19..15] Sequence 15 - Regular channel 15 in sequence */
            __IOM uint32_t SQ16                : 5; /*!< [24..20] Sequence 16 - Regular channel 16 in sequence */
            __IM  uint32_t          : 7;
        } SQR3_f;
    };
    union {
        __IOM uint32_t JSQR                ; /*!< Offset: 0x30 (read-write) Injected sequence register - ADC injected channel sequence configuration */
        struct {
            __IOM uint32_t JL                  : 2; /*!< [1..0] Injected sequence length - Injected channel sequence length */
            __IM  uint32_t          : 3;
            __IOM uint32_t JSQ1                : 5; /*!< [9..5] Injected sequence 1 - Injected channel 1 selection */
            __IOM uint32_t JSQ2                : 5; /*!< [14..10] Injected sequence 2 - Injected channel 2 selection */
            __IOM uint32_t JSQ3                : 5; /*!< [19..15] Injected sequence 3 - Injected channel 3 selection */
            __IOM uint32_t JSQ4                : 5; /*!< [24..20] Injected sequence 4 - Injected channel 4 in sequence */
            __IOM uint32_t JEXTEN              : 2; /*!< [26..25] Injected external trigger enable - Enables injected external trigger */
            __IOM uint32_t JEXTSEL             : 5; /*!< [31..27] Injected external trigger select - Selects injected trigger */
        } JSQR_f;
    };
    union {
        __IM  uint32_t JDR1                ; /*!< Offset: 0x34 (read-only) Injected data register 1 - Injected conversion result 1 */
        struct {
            __IM  uint32_t JDATA               : 16; /*!< [15..0] J data - Injected conversion data */
            __IM  uint32_t JCH                 : 5; /*!< [20..16] J channel - Injected channel select */
            __IM  uint32_t          : 11;
        } JDR1_f;
    };
    union {
        __IM  uint32_t JDR2                ; /*!< Offset: 0x38 (read-only) Injected data register 2 - Injected conversion result 2 */
        struct {
            __IM  uint32_t JDATA               : 16; /*!< [15..0] J data - Injected conversion data */
            __IM  uint32_t JCH                 : 5; /*!< [20..16] J channel - Injected channel select */
            __IM  uint32_t          : 11;
        } JDR2_f;
    };
    union {
        __IM  uint32_t JDR3                ; /*!< Offset: 0x3c (read-only) Injected data register 3 - Injected conversion result 3 */
        struct {
            __IM  uint32_t JDATA               : 16; /*!< [15..0] J data - Injected conversion data */
            __IM  uint32_t JCH                 : 5; /*!< [20..16] J channel - Injected channel select */
            __IM  uint32_t          : 11;
        } JDR3_f;
    };
    union {
        __IM  uint32_t JDR4                ; /*!< Offset: 0x40 (read-only) Injected data register 4 - Injected conversion result 4 */
        struct {
            __IM  uint32_t JDATA               : 16; /*!< [15..0] J data - Injected conversion data */
            __IM  uint32_t JCH                 : 5; /*!< [20..16] J channel - Injected channel select */
            __IM  uint32_t          : 11;
        } JDR4_f;
    };
    __IO uint32_t RESERVED0;
    union {
        __IM  uint32_t DR                  ; /*!< Offset: 0x48 (read-only) Data register - CRC calculation data input and result */
        struct {
            __IM  uint32_t DATA                : 16; /*!< [15..0] Data value - Data to be transferred or received data value */
            __IM  uint32_t CH                  : 5; /*!< [20..16] Channel - Channel selection field */
            __IM  uint32_t          : 11;
        } DR_f;
    };
    union {
        __IOM uint32_t DIFSEL              ; /*!< Offset: 0x4c (read-write) Differential mode selection - Selects differential ADC mode */
        struct {
            __IOM uint32_t DIFSEL              : 19; /*!< [18..0] Differential mode selection - Selects differential ADC mode */
            __IM  uint32_t          : 13;
        } DIFSEL_f;
    };
    __IO uint32_t RESERVED1[4];
    union {
        __IOM uint32_t OFR1                ; /*!< Offset: 0x60 (read-write) Offset register 1 - Offset for channel 1 */
        struct {
            __IOM uint32_t OFFSETX             : 12; /*!< [11..0] Offset Y - ADC offset Y value */
            __IM  uint32_t          : 12;
            __IOM uint32_t OFFSETX_POS         : 1; /*!< [24..24] Offset Y position - ADC offset Y sign */
            __IOM uint32_t OFFSETX_SAT         : 1; /*!< [25..25] Offset Y saturation - ADC offset Y saturation */
            __IOM uint32_t OFFSETX_CH          : 5; /*!< [30..26] Offset Y channel - ADC offset Y channel selection */
            __IOM uint32_t OFFSETX_EN          : 1; /*!< [31..31] Offset Y enable - Enables ADC offset Y */
        } OFR1_f;
    };
    union {
        __IOM uint32_t OFR2                ; /*!< Offset: 0x64 (read-write) Offset register 2 - Offset for channel 2 */
        struct {
            __IOM uint32_t OFFSETX             : 12; /*!< [11..0] Offset Y - ADC offset Y value */
            __IM  uint32_t          : 12;
            __IOM uint32_t OFFSETX_POS         : 1; /*!< [24..24] Offset Y position - ADC offset Y sign */
            __IOM uint32_t OFFSETX_SAT         : 1; /*!< [25..25] Offset Y saturation - ADC offset Y saturation */
            __IOM uint32_t OFFSETX_CH          : 5; /*!< [30..26] Offset Y channel - ADC offset Y channel selection */
            __IOM uint32_t OFFSETX_EN          : 1; /*!< [31..31] Offset Y enable - Enables ADC offset Y */
        } OFR2_f;
    };
    union {
        __IOM uint32_t OFR3                ; /*!< Offset: 0x68 (read-write) Offset register 3 - Offset for channel 3 */
        struct {
            __IOM uint32_t OFFSETX             : 12; /*!< [11..0] Offset Y - ADC offset Y value */
            __IM  uint32_t          : 12;
            __IOM uint32_t OFFSETX_POS         : 1; /*!< [24..24] Offset Y position - ADC offset Y sign */
            __IOM uint32_t OFFSETX_SAT         : 1; /*!< [25..25] Offset Y saturation - ADC offset Y saturation */
            __IOM uint32_t OFFSETX_CH          : 5; /*!< [30..26] Offset Y channel - ADC offset Y channel selection */
            __IOM uint32_t OFFSETX_EN          : 1; /*!< [31..31] Offset Y enable - Enables ADC offset Y */
        } OFR3_f;
    };
    union {
        __IOM uint32_t OFR4                ; /*!< Offset: 0x6c (read-write) Offset register 4 - Offset for channel 4 */
        struct {
            __IOM uint32_t OFFSETX             : 12; /*!< [11..0] Offset Y - ADC offset Y value */
            __IM  uint32_t          : 12;
            __IOM uint32_t OFFSETX_POS         : 1; /*!< [24..24] Offset Y position - ADC offset Y sign */
            __IOM uint32_t OFFSETX_SAT         : 1; /*!< [25..25] Offset Y saturation - ADC offset Y saturation */
            __IOM uint32_t OFFSETX_CH          : 5; /*!< [30..26] Offset Y channel - ADC offset Y channel selection */
            __IOM uint32_t OFFSETX_EN          : 1; /*!< [31..31] Offset Y enable - Enables ADC offset Y */
        } OFR4_f;
    };
    __IO uint32_t RESERVED2[5];
    union {
        __IOM uint32_t CALFACT             ; /*!< Offset: 0x84 (read-write) Calibration factor - ADC calibration factor value */
        struct {
            __IOM uint32_t CALFACT_S           : 7; /*!< [6..0] Calibration factor single-ended - Single-ended mode calibration factor */
            __IM  uint32_t          : 9;
            __IOM uint32_t CALFACT_D           : 7; /*!< [22..16] Calibration factor differential - Differential mode calibration factor */
            __IM  uint32_t          : 9;
        } CALFACT_f;
    };
    __IO uint32_t RESERVED3[159];
    union {
        __IOM uint32_t CCR                 ; /*!< Offset: 0x304 (read-write) Common control register - ADC common configuration register */
        struct {
            __IM  uint32_t          : 13;
            __IOM uint32_t CKMODE              : 1; /*!< [13..13] Clock mode - Clock operating mode selection */
            __IM  uint32_t          : 2;
            __IOM uint32_t ADCDIV              : 6; /*!< [21..16] ADC clock divider - Configures ADC clock division factor */
            __IOM uint32_t TSEN                : 1; /*!< [22..22] TSEN - TSEN value */
            __IOM uint32_t VREFINTEN           : 1; /*!< [23..23] VREFINT enable - Enables internal voltage reference */
            __IOM uint32_t VBATEN              : 1; /*!< [24..24] VBAT enable - Enables VBAT power supply */
            __IM  uint32_t          : 7;
        } CCR_f;
    };
    __IO uint32_t RESERVED4;
    union {
        __IOM uint32_t CVRB                ; /*!< Offset: 0x30C (read-write) CVR buffer - ADC voltage reference buffer register */
        struct {
            __IOM uint32_t ENVR                : 1; /*!< [0..0] Enable voltage regulator - Voltage regulator enable */
            __IOM uint32_t HIZ                 : 1; /*!< [1..1] High-impedance - High-impedance output control */
            __IOM uint32_t VRS                 : 2; /*!< [3..2] VRS - VRS value */
            __IOM uint32_t VRBTRIM             : 5; /*!< [8..4] VRB trim - Voltage reference buffer trim value */
            __IM  uint32_t          : 23;
        } CVRB_f;
    };
} ADC_TypeDef;

/**
 * @brief DAC - DAC Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t CR                  ; /*!< Offset: 0x00 (read-write) Control register - CRC calculation control */
        struct {
            __IOM uint32_t EN1                 : 1; /*!< [0..0] DAC channel 1 enable - Enables DAC channel 1 */
            __IOM uint32_t TEN1                : 1; /*!< [1..1] DAC channel 1 trigger enable - Enables external trigger for channel 1 */
            __IOM uint32_t TSEL1               : 4; /*!< [5..2] DAC channel 1 trigger selection - Selects trigger source for channel 1 */
            __IOM uint32_t WAVE1               : 2; /*!< [7..6] DAC channel 1 LFSR noise/triangle waveform generation enable */
            __IOM uint32_t MAMP1               : 4; /*!< [11..8] DAC channel 1 mask/amplitude - Configures noise/triangle amplitude for channel 1 */
            __IOM uint32_t DMAEN1              : 1; /*!< [12..12] DAC channel 1 DMA enable - Enables DMA for channel 1 */
            __IOM uint32_t DMAUDIE1            : 1; /*!< [13..13] DMA underrun interrupt enable 1 - DMA underrun interrupt 1 */
            __IOM uint32_t CEN1                : 1; /*!< [14..14] DAC channel 1 calibration enable */
            __IM  uint32_t          : 1;
            __IOM uint32_t EN2                 : 1; /*!< [16..16] DAC channel 2 enable - Enables DAC channel 2 */
            __IOM uint32_t TEN2                : 1; /*!< [17..17] DAC channel 2 trigger enable */
            __IOM uint32_t TSEL2               : 4; /*!< [21..18] DAC channel 2 trigger selection */
            __IOM uint32_t WAVE2               : 2; /*!< [23..22] DAC channel 2 LFSR noise/triangle waveform generation enable */
            __IOM uint32_t MAMP2               : 4; /*!< [27..24] MAMP2 - DAC2 noise/triangle amplitude */
            __IOM uint32_t DMAEN2              : 1; /*!< [28..28] DMA enable 2 - Secondary DMA enable control */
            __IOM uint32_t DMAUDIE2            : 1; /*!< [29..29] DMA underrun interrupt enable 2 - DMA underrun interrupt 2 */
            __IOM uint32_t CEN2                : 1; /*!< [30..30] DAC channel 2 calibration enable */
            __IM  uint32_t          : 1;
        } CR_f;
    };
    union {
        __OM  uint32_t SWTRIGR             ; /*!< Offset: 0x04 (write-only) DAC software trigger register */
        struct {
            __OM  uint32_t SWTRIG1             : 1; /*!< [0..0] DAC channel 1 software trigger */
            __OM  uint32_t SWTRIG2             : 1; /*!< [1..1] DAC channel 2 software trigger */
            __IM  uint32_t          : 14;
            __OM  uint32_t SWTRIGB1            : 1; /*!< [16..16] DAC channel 1 sawtooth increment software trigger */
            __OM  uint32_t SWTRIGB2            : 1; /*!< [17..17] DAC channel 2 sawtooth increment software trigger */
            __IM  uint32_t          : 14;
        } SWTRIGR_f;
    };
    union {
        __IOM uint32_t DHR12R1             ; /*!< Offset: 0x08 (read-write) 12-bit right-aligned data holding register 1 - DAC channel 1 12-bit right-aligned data */
        struct {
            __IOM uint32_t DACC1DHR            : 12; /*!< [11..0] DAC channel 1 data holding register - DAC channel 1 data for conversion */
            __IM  uint32_t          : 4;
            __IOM uint32_t DACC1DHRB           : 12; /*!< [27..16] DAC channel 1 data holding register B - DAC channel 1 buffer data */
            __IM  uint32_t          : 4;
        } DHR12R1_f;
    };
    union {
        __IOM uint32_t DHR12L1             ; /*!< Offset: 0x0c (read-write) 12-bit left-aligned data holding register 1 - DAC channel 1 12-bit left-aligned data */
        struct {
            __IM  uint32_t          : 4;
            __IOM uint32_t DACC1DHR            : 12; /*!< [15..4] DAC channel 1 data holding register - DAC channel 1 data for conversion */
            __IM  uint32_t          : 4;
            __IOM uint32_t DACC1DHRB           : 12; /*!< [31..20] DAC channel 1 data holding register B - DAC channel 1 buffer data */
        } DHR12L1_f;
    };
    union {
        __IOM uint32_t DHR8R1              ; /*!< Offset: 0x10 (read-write) 8-bit right-aligned data holding register 1 - DAC channel 1 8-bit right-aligned data */
        struct {
            __IOM uint32_t DACC1DHR            : 8; /*!< [7..0] DAC channel 1 data holding register - DAC channel 1 data for conversion */
            __IOM uint32_t DACC1DHRB           : 8; /*!< [15..8] DAC channel 1 data holding register B - DAC channel 1 buffer data */
            __IM  uint32_t          : 16;
        } DHR8R1_f;
    };
    union {
        __IOM uint32_t DHR12R2             ; /*!< Offset: 0x14 (read-write) 12-bit right-aligned data holding register 2 - DAC channel 2 12-bit right-aligned data */
        struct {
            __IOM uint32_t DACC2DHR            : 12; /*!< [11..0] DAC channel 2 data holding register - DAC channel 2 data for conversion */
            __IM  uint32_t          : 4;
            __IOM uint32_t DACC2DHRB           : 12; /*!< [27..16] DAC channel 2 data holding register B - DAC channel 2 buffer data */
            __IM  uint32_t          : 4;
        } DHR12R2_f;
    };
    union {
        __IOM uint32_t DHR12L2             ; /*!< Offset: 0x18 (read-write) 12-bit left-aligned data holding register 2 - DAC channel 2 12-bit left-aligned data */
        struct {
            __IM  uint32_t          : 4;
            __IOM uint32_t DACC2DHR            : 12; /*!< [15..4] DAC channel 2 data holding register - DAC channel 2 data for conversion */
            __IM  uint32_t          : 4;
            __IOM uint32_t DACC2DHRB           : 12; /*!< [31..20] DAC channel 2 data holding register B - DAC channel 2 buffer data */
        } DHR12L2_f;
    };
    union {
        __IOM uint32_t DHR8R2              ; /*!< Offset: 0x1c (read-write) 8-bit right-aligned data holding register 2 - DAC channel 2 8-bit right-aligned data */
        struct {
            __IOM uint32_t DACC2DHR            : 8; /*!< [7..0] DAC channel 2 data holding register - DAC channel 2 data for conversion */
            __IOM uint32_t DACC2DHRB           : 8; /*!< [15..8] DAC channel 2 data holding register B - DAC channel 2 buffer data */
            __IM  uint32_t          : 16;
        } DHR8R2_f;
    };
    union {
        __IOM uint32_t DHR12RD             ; /*!< Offset: 0x20 (read-write) 12-bit right-aligned data holding register - DAC data holding (right-aligned) */
        struct {
            __IOM uint32_t DACC1DHR            : 12; /*!< [11..0] DAC channel 1 data holding register - DAC channel 1 data for conversion */
            __IM  uint32_t          : 4;
            __IOM uint32_t DACC2DHR            : 12; /*!< [27..16] DAC channel 2 data holding register - DAC channel 2 data for conversion */
            __IM  uint32_t          : 4;
        } DHR12RD_f;
    };
    union {
        __IOM uint32_t DHR12LD             ; /*!< Offset: 0x24 (read-write) 12-bit left-aligned data holding register - DAC data holding (left-aligned) */
        struct {
            __IM  uint32_t          : 4;
            __IOM uint32_t DACC1DHR            : 12; /*!< [15..4] DAC channel 1 data holding register - DAC channel 1 data for conversion */
            __IM  uint32_t          : 4;
            __IOM uint32_t DACC2DHR            : 12; /*!< [31..20] DAC channel 2 data holding register - DAC channel 2 data for conversion */
        } DHR12LD_f;
    };
    union {
        __IOM uint32_t DHR8RD              ; /*!< Offset: 0x28 (read-write) 8-bit right-aligned data holding register - DAC 8-bit data holding */
        struct {
            __IOM uint32_t DACC1DHR            : 8; /*!< [7..0] DAC channel 1 data holding register - DAC channel 1 data for conversion */
            __IM  uint32_t          : 8;
            __IOM uint32_t DACC2DHR            : 8; /*!< [23..16] DAC channel 2 data holding register - DAC channel 2 data for conversion */
            __IM  uint32_t          : 8;
        } DHR8RD_f;
    };
    union {
        __IM  uint32_t DOR1                ; /*!< Offset: 0x2c (read-only) Data output register 1 - DAC channel 1 output value */
        struct {
            __IM  uint32_t DACC1DOR            : 12; /*!< [11..0] DAC channel 1 data output register - DAC channel 1 output value */
            __IM  uint32_t          : 4;
            __IM  uint32_t DACC1DORB           : 12; /*!< [27..16] DAC channel 1 data output register B - DAC channel 1 output buffer */
            __IM  uint32_t          : 4;
        } DOR1_f;
    };
    union {
        __IM  uint32_t DOR2                ; /*!< Offset: 0x30 (read-only) Data output register 2 - DAC channel 2 output value */
        struct {
            __IM  uint32_t DACC2DOR            : 12; /*!< [11..0] DAC channel 2 data output register - DAC channel 2 output value */
            __IM  uint32_t          : 4;
            __IM  uint32_t DACC2DORB           : 12; /*!< [27..16] DAC channel 2 data output register B - DAC channel 2 output buffer */
            __IM  uint32_t          : 4;
        } DOR2_f;
    };
    union {
        __IOM uint32_t SR                  ; /*!< Offset: 0x34 (read-write) DAC status register */
        struct {
            __IM  uint32_t          : 8;
            __IOM uint32_t SAMOV1              : 1; /*!< [8..8] DAC channel 1 sample event synchronization complete flag */
            __IM  uint32_t          : 3;
            __IOM uint32_t DORSTAT1            : 1; /*!< [12..12] DOR status 1 - Data output register status 1 */
            __IOM uint32_t DMAUDR1             : 1; /*!< [13..13] DMA underrun 1 - DMA underrun flag 1 */
            __IOM uint32_t CAL_FLAG1           : 1; /*!< [14..14] Calibration flag 1 - Calibration status flag 1 */
            __IM  uint32_t          : 9;
            __IOM uint32_t SAMOV2              : 1; /*!< [24..24] DAC channel 2 sample event synchronization complete flag */
            __IM  uint32_t          : 3;
            __IOM uint32_t DORSTAT2            : 1; /*!< [28..28] DOR status 2 - Data output register status 2 */
            __IOM uint32_t DMAUDR2             : 1; /*!< [29..29] DMA underrun 2 - DMA underrun flag 2 */
            __IOM uint32_t CAL_FLAG2           : 1; /*!< [30..30] Calibration flag 2 - Calibration status flag 2 */
            __IM  uint32_t          : 1;
        } SR_f;
    };
    union {
        __IOM uint32_t CCR                 ; /*!< Offset: 0x38 (read-write) DAC calibration control register */
        struct {
            __IOM uint32_t OTRIM1              : 5; /*!< [4..0] DAC channel 1 TRIM value */
            __IM  uint32_t          : 11;
            __IOM uint32_t OTRIM2              : 5; /*!< [20..16] DAC channel 2 TRIM value */
            __IM  uint32_t          : 11;
        } CCR_f;
    };
    union {
        __IOM uint32_t MCR                 ; /*!< Offset: 0x3c (read-write) Mode control register - Configures DAC operating mode */
        struct {
            __IOM uint32_t MODE1               : 3; /*!< [2..0] Mode 1 - Operating mode 1 */
            __IM  uint32_t          : 5;
            __IOM uint32_t DMADOUBLE1          : 1; /*!< [8..8] DMA double buffer 1 - DMA double buffer enable 1 */
            __IOM uint32_t SINFORMAT1          : 1; /*!< [9..9] DAC channel 1 signed format select */
            __IM  uint32_t          : 6;
            __IOM uint32_t MODE2               : 3; /*!< [18..16] Mode 2 - Operating mode 2 */
            __IM  uint32_t          : 5;
            __IOM uint32_t DMADOUBLE2          : 1; /*!< [24..24] DMA double buffer 2 - DMA double buffer enable 2 */
            __IOM uint32_t SINFORMAT2          : 1; /*!< [25..25] DAC channel 2 signed format select */
            __IM  uint32_t          : 6;
        } MCR_f;
    };
    union {
        __IOM uint32_t SHSR1               ; /*!< Offset: 0x40 (read-write) DAC channel 1 sample time register */
        struct {
            __IOM uint32_t TSAMPLE1            : 10; /*!< [9..0] TSAMPLE1 - TSAMPLE1 value */
            __IM  uint32_t          : 22;
        } SHSR1_f;
    };
    union {
        __IOM uint32_t SHSR2               ; /*!< Offset: 0x44 (read-write) DAC channel 2 sample time register */
        struct {
            __IOM uint32_t TSAMPLE2            : 10; /*!< [9..0] TSAMPLE2 - TSAMPLE2 value */
            __IM  uint32_t          : 22;
        } SHSR2_f;
    };
    union {
        __IOM uint32_t SHHR                ; /*!< Offset: 0x48 (read-write) DAC hold time register */
        struct {
            __IOM uint32_t THOLD1              : 10; /*!< [9..0] THOLD1 - THOLD1 value */
            __IM  uint32_t          : 6;
            __IOM uint32_t THOLD2              : 10; /*!< [25..16] THOLD2 - THOLD2 value */
            __IM  uint32_t          : 6;
        } SHHR_f;
    };
    union {
        __IOM uint32_t SHRR                ; /*!< Offset: 0x4c (read-write) DAC refresh time register */
        struct {
            __IOM uint32_t TREFRESH1           : 8; /*!< [7..0] TREFRESH1 - TREFRESH1 value */
            __IM  uint32_t          : 8;
            __IOM uint32_t TREFRESH2           : 8; /*!< [23..16] TREFRESH2 - TREFRESH2 value */
            __IM  uint32_t          : 8;
        } SHRR_f;
    };
    __IO uint32_t RESERVED0[2];
    union {
        __IOM uint32_t STR1                ; /*!< Offset: 0x58 (read-write) DAC channel 1 sawtooth register */
        struct {
            __IOM uint32_t STRSTDATA1          : 12; /*!< [11..0] STRSTDATA1 - STRSTDATA1 value */
            __IOM uint32_t STDIR1              : 1; /*!< [12..12] STDIR1 - STDIR1 value */
            __IM  uint32_t          : 3;
            __IOM uint32_t STINCDATA1          : 16; /*!< [31..16] STINCDATA1 - STINCDATA1 value */
        } STR1_f;
    };
    union {
        __IOM uint32_t STR2                ; /*!< Offset: 0x5c (read-write) DAC channel 2 sawtooth register */
        struct {
            __IOM uint32_t STRSTDATA2          : 12; /*!< [11..0] STRSTDATA2 - STRSTDATA2 value */
            __IOM uint32_t STDIR2              : 1; /*!< [12..12] STDIR2 - STDIR2 value */
            __IM  uint32_t          : 3;
            __IOM uint32_t STINCDATA2          : 16; /*!< [31..16] STINCDATA2 - STINCDATA2 value */
        } STR2_f;
    };
    union {
        __IOM uint32_t STMODR              ; /*!< Offset: 0x60 (read-write) DAC sawtooth mode register */
        struct {
            __IOM uint32_t STRSTTRIGSEL1       : 4; /*!< [3..0] STRSTTRIGSEL1 - STRSTTRIGSEL1 value */
            __IM  uint32_t          : 4;
            __IOM uint32_t STINCTRIGSEL1       : 4; /*!< [11..8] STINCTRIGSEL1 - STINCTRIGSEL1 value */
            __IM  uint32_t          : 4;
            __IOM uint32_t STRSTTRIGSEL2       : 4; /*!< [19..16] STRSTTRIGSEL2 - STRSTTRIGSEL2 value */
            __IM  uint32_t          : 4;
            __IOM uint32_t STINCTRIGSEL2       : 4; /*!< [27..24] STINCTRIGSEL2 - STINCTRIGSEL2 value */
            __IM  uint32_t          : 4;
        } STMODR_f;
    };
} DAC_TypeDef;

/**
 * @brief DLYB - DLYB Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t CR                  ; /*!< Offset: 0x00 (read-write) Control register - CRC calculation control */
        struct {
            __IOM uint32_t DEN                 : 1; /*!< [0..0] DMA enable - Enables DMA requests */
            __IOM uint32_t SEN                 : 1; /*!< [1..1] SEN - SEN value */
            __IM  uint32_t          : 30;
        } CR_f;
    };
    union {
        __IOM uint32_t CFGR                ; /*!< Offset: 0x04 (read-write) Configuration register - ADC operating mode configuration */
        struct {
            __IOM uint32_t SEL                 : 4; /*!< [3..0] Select - Selection value */
            __IM  uint32_t          : 4;
            __IOM uint32_t UNIT                : 6; /*!< [13..8] UNIT - UNIT value */
            __IM  uint32_t          : 2;
            __IOM uint32_t LEN                 : 12; /*!< [27..16] Length - Length value */
            __IM  uint32_t          : 3;
            __IOM uint32_t LENF                : 1; /*!< [31..31] Length flag - Length error flag */
        } CFGR_f;
    };
} DLYB_TypeDef;

/**
 * @brief USB - USB Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t WORKING_MODE        ; /*!< Offset: 0x00 (read-write) Working mode register */
        struct {
            __IOM uint32_t SPEED_MODE          : 1; /*!< [0..0] USB speed mode select: 0=low-speed, 1=full-speed */
            __IOM uint32_t USB_SUSPEND         : 1; /*!< [1..1] USB suspend control */
            __IOM uint32_t USB_FORCE_RST       : 1; /*!< [2..2] Force reset controller address, state machine, FIFO */
            __IOM uint32_t USB_BUS_AUTO_RST_EN : 1; /*!< [3..3] USB bus reset can reset controller address, state machine, FIFO */
            __IOM uint32_t USB_DPPU            : 1; /*!< [4..4] USB Dp 2.1k pull-up resistor control */
            __IM  uint32_t          : 1;
            __IOM uint32_t USB_DPPU_LO         : 1; /*!< [6..6] USB Dp 2.8k pull-up resistor control */
            __IM  uint32_t          : 1;
            __IOM uint32_t USB_LINE_STATE      : 2; /*!< [9..8] USB Dp/Dm signal status bits [9:8] */
            __IOM uint32_t USB_CRCERR_NAK_EN   : 1; /*!< [10..10] NAK response enable on CRC error */
            __IOM uint32_t USB_IN_TIMEOUT_NAK_EN: 1; /*!< [11..11] NAK response enable when host ACK timeout on IN operation */
            __IOM uint32_t USB_MORETHAN64_NAK_EN: 1; /*!< [12..12] NAK response enable when data packet exceeds 64 bytes */
            __IM  uint32_t          : 7;
            __IOM uint32_t USB_REMOTE_WAKEUP   : 1; /*!< [20..20] USB remote wakeup control bit */
            __IOM uint32_t USB_TOKEN_NOEOP_NAK_EN: 1; /*!< [21..21] NAK response enable when token packet EOP loss detected */
            __IOM uint32_t USB_DATA_NOEOP_NAK_EN: 1; /*!< [22..22] NAK response enable when data packet EOP loss detected */
            __IOM uint32_t USB_TOKEN_NOEOP_EN  : 1; /*!< [23..23] USB token packet EOP loss detection enable */
            __IOM uint32_t USB_DATA_NOEOP_EN   : 1; /*!< [24..24] USB data packet EOP loss detection enable */
            __IOM uint32_t USB_EPO_ZOD_INTR_EN : 1; /*!< [25..25] USB OUT zero-length data packet interrupt enable */
            __IOM uint32_t LPM_EN              : 1; /*!< [26..26] Enable LPM function support */
            __IOM uint32_t LPM_TOKEN_NYET_EN   : 1; /*!< [27..27] Enable NYET response after receiving LPM Token */
            __IM  uint32_t          : 4;
        } WORKING_MODE_f;
    };
    __IO uint32_t RESERVED0[3];
    union {
        __IM  uint32_t LPM_ATTR            ; /*!< Offset: 0x10 (read-only) LPM attribute register */
        struct {
            __IM  uint32_t LPM_ATTR            : 11; /*!< [10..0] USB LPM ATTR value */
            __IM  uint32_t          : 21;
        } LPM_ATTR_f;
    };
    __IO uint32_t RESERVED1;
    union {
        __IOM uint32_t USB_ADDR            ; /*!< Offset: 0x18 (read-write) USB address register */
        struct {
            __IOM uint32_t USB_ADDR            : 7; /*!< [6..0] USB device address */
            __IM  uint32_t          : 25;
        } USB_ADDR_f;
    };
    union {
        __IM  uint32_t SETUP_0_3_DATA      ; /*!< Offset: 0x1C (read-only) SETUP data packet register 0 */
        struct {
            __IM  uint32_t SETUP_0_3_DATA      : 32; /*!< [31..0] SETUP Data packet Byte0~Byte3 */
        } SETUP_0_3_DATA_f;
    };
    union {
        __IM  uint32_t SETUP_4_7_DATA      ; /*!< Offset: 0x20 (read-only) SETUP data packet register 1 */
        struct {
            __IM  uint32_t SETUP_4_7_DATA      : 32; /*!< [31..0] SETUP Data packet Byte4~Byte7 */
        } SETUP_4_7_DATA_f;
    };
    union {
        __IOM uint32_t EP_ADDR             ; /*!< Offset: 0x24 (read-write) End point address configuration register */
        struct {
            __IOM uint32_t EP1_ADDR            : 4; /*!< [3..0] EP1 address configuration */
            __IOM uint32_t EP2_ADDR            : 4; /*!< [7..4] EP2 address configuration */
            __IOM uint32_t EP3_ADDR            : 4; /*!< [11..8] EP3 address configuration */
            __IOM uint32_t EP4_ADDR            : 4; /*!< [15..12] EP4 address configuration */
            __IOM uint32_t EP5_ADDR            : 4; /*!< [19..16] EP5 address configuration */
            __IOM uint32_t EP6_ADDR            : 4; /*!< [23..20] EP6 address configuration */
            __IM  uint32_t          : 8;
        } EP_ADDR_f;
    };
    __IO uint32_t RESERVED2;
    union {
        __IM  uint32_t CURRENT_PID         ; /*!< Offset: 0x2C (read-only) Current USB bus packet PID register */
        struct {
            __IM  uint32_t CURRENT_PID         : 4; /*!< [3..0] Current received USB packet PID value */
            __IM  uint32_t          : 28;
        } CURRENT_PID_f;
    };
    union {
        __IM  uint32_t CURRENT_FRAME_NUMBER; /*!< Offset: 0x30 (read-only) Frame number register */
        struct {
            __IM  uint32_t CURRENT_FRAME_NUMBER: 11; /*!< [10..0] Current frame number */
            __IM  uint32_t          : 21;
        } CURRENT_FRAME_NUMBER_f;
    };
    union {
        __IM  uint32_t CRC_ERROR_CNT       ; /*!< Offset: 0x34 (read-only) CRC error counter register */
        struct {
            __IM  uint32_t CRC_ERROR_CNT       : 8; /*!< [7..0] CRC error packet count, reset on USB Reset */
            __IM  uint32_t          : 24;
        } CRC_ERROR_CNT_f;
    };
    union {
        __IOM uint32_t USB_STATUS_DETECT_CNT; /*!< Offset: 0x38 (read-write) Suspend/Resume/Reset detection time register */
        struct {
            __IOM uint32_t USB_STATUS_DETECT_CNT: 10; /*!< [9..0] Reset/Resume/Suspend detection threshold: time = CNT*5.3us + 2.5us */
            __IM  uint32_t          : 22;
        } USB_STATUS_DETECT_CNT_f;
    };
    union {
        __IOM uint32_t USB_CFGR            ; /*!< Offset: 0x3C (read-write) USB configuration register */
        struct {
            __IOM uint32_t TIMOUT_CFG          : 1; /*!< [0..0] Timeout configuration */
            __IOM uint32_t SETADDR_CFG         : 1; /*!< [1..1] SET address configuration */
            __IOM uint32_t SOFT_CFG            : 1; /*!< [2..2] 0: only update Frame Number on SOF TOKEN, 1: original design */
            __IM  uint32_t          : 29;
        } USB_CFGR_f;
    };
    union {
        __IOM uint32_t EP0CSR              ; /*!< Offset: 0x40 (read-write) EP0 transfer control register */
        struct {
            __IOM uint32_t EP0_RECEIVED_BYTE   : 8; /*!< [7..0] EP0 received data byte count */
            __IOM uint32_t EP0_EN              : 1; /*!< [8..8] EP0 endpoint enable */
            __IOM uint32_t EP0_FIFOCLR         : 1; /*!< [9..9] EP0 FIFO pointer reset control */
            __IOM uint32_t EP0_DATA_START      : 1; /*!< [10..10] EP0 data send start */
            __IOM uint32_t EP0_RECEIVED_DONE   : 1; /*!< [11..11] EP0 receive done control */
            __IOM uint32_t EP0_SEND_STALL      : 1; /*!< [12..12] EP0 STALL send control */
            __IOM uint32_t EP0_SEND_STALL_DONE : 1; /*!< [13..13] EP0 STALL send complete flag */
            __IOM uint32_t EP0_IN_TOGGLE_VALUE : 1; /*!< [14..14] EP0 IN operation toggle control */
            __IOM uint32_t EP0_IN_TOGGLE_CTRL_EN: 1; /*!< [15..15] EP0_IN_TOGGLE_VALUE effective enable */
            __IOM uint32_t EP0_OUT_TOGGLE_VALUE: 1; /*!< [16..16] EP0 received data packet toggle value */
            __IOM uint32_t EP0_OUT_TOGGLE_WANT : 1; /*!< [17..17] EP0 OUT/Setup data packet toggle comparison value */
            __IOM uint32_t EP0_OUT_TOGGLE_CTRL_EN: 1; /*!< [18..18] EP0_OUT_TOGGLE_WANT value effective enable */
            __IOM uint32_t EP0_OUT_TOGGLE_STATE: 1; /*!< [19..19] EP0 OUT/Setup toggle error flag */
            __IOM uint32_t EP0_OUT_VALID       : 1; /*!< [20..20] EP0 OUT valid flag, set when valid data enters FIFO */
            __IM  uint32_t          : 3;
            __IOM uint32_t EP0_RECEIVED_ACK    : 1; /*!< [24..24] EP0 received ACK handshake from host */
            __IOM uint32_t EP0_RECEIVED_NAK    : 1; /*!< [25..25] EP0 received NAK handshake from host */
            __IOM uint32_t EP0_RECEIVED_STALL  : 1; /*!< [26..26] EP0 received STALL handshake from host */
            __IOM uint32_t EP0_SEND_HALT       : 1; /*!< [27..27] EP0 send data halt control */
            __IM  uint32_t          : 4;
        } EP0CSR_f;
    };
    union {
        __IOM uint32_t EP1CSR              ; /*!< Offset: 0x44 (read-write) EP1 transfer control register */
        struct {
            __IOM uint32_t EP1_RECEIVED_BYTE   : 8; /*!< [7..0] EP1 received data byte count */
            __IOM uint32_t EP1_EN              : 1; /*!< [8..8] EP1 endpoint enable */
            __IOM uint32_t EP1_FIFOCLR         : 1; /*!< [9..9] EP1 FIFO pointer reset control */
            __IOM uint32_t EP1_DATA_START      : 1; /*!< [10..10] EP1 data send start */
            __IOM uint32_t EP1_RECEIVED_DONE   : 1; /*!< [11..11] EP1 receive done control */
            __IOM uint32_t EP1_SEND_STALL      : 1; /*!< [12..12] EP1 STALL send control */
            __IOM uint32_t EP1_SEND_STALL_DONE : 1; /*!< [13..13] EP1 STALL send complete flag */
            __IOM uint32_t EP1_IN_TOGGLE_VALUE : 1; /*!< [14..14] EP1 IN operation toggle control */
            __IOM uint32_t EP1_IN_TOGGLE_CTRL_EN: 1; /*!< [15..15] EP1_IN_TOGGLE_VALUE effective enable */
            __IOM uint32_t EP1_OUT_TOGGLE_VALUE: 1; /*!< [16..16] EP1 received data packet toggle value */
            __IOM uint32_t EP1_OUT_TOGGLE_WANT : 1; /*!< [17..17] EP1 OUT/Setup data packet toggle comparison value */
            __IOM uint32_t EP1_OUT_TOGGLE_CTRL_EN: 1; /*!< [18..18] EP1_OUT_TOGGLE_WANT value effective enable */
            __IOM uint32_t EP1_OUT_TOGGLE_STATE: 1; /*!< [19..19] EP1 OUT/Setup toggle error flag */
            __IOM uint32_t EP1_OUT_VALID       : 1; /*!< [20..20] EP1 OUT valid flag */
            __IOM uint32_t EP1_ISOCHRONOUS_EN  : 1; /*!< [21..21] EP1 isochronous mode enable */
            __IM  uint32_t          : 2;
            __IOM uint32_t EP1_RECEIVED_ACK    : 1; /*!< [24..24] EP1 received ACK handshake from host */
            __IOM uint32_t EP1_RECEIVED_NAK    : 1; /*!< [25..25] EP1 received NAK handshake from host */
            __IOM uint32_t EP1_RECEIVED_STALL  : 1; /*!< [26..26] EP1 received STALL handshake from host */
            __IOM uint32_t EP1_SEND_HALT       : 1; /*!< [27..27] EP1 send data halt control */
            __IM  uint32_t          : 4;
        } EP1CSR_f;
    };
    union {
        __IOM uint32_t EP2CSR              ; /*!< Offset: 0x48 (read-write) EP2 transfer control register */
        struct {
            __IOM uint32_t EP2_RECEIVED_BYTE   : 8; /*!< [7..0] EP2 received data byte count */
            __IOM uint32_t EP2_EN              : 1; /*!< [8..8] EP2 endpoint enable */
            __IOM uint32_t EP2_FIFOCLR         : 1; /*!< [9..9] EP2 FIFO pointer reset control */
            __IOM uint32_t EP2_DATA_START      : 1; /*!< [10..10] EP2 data send start */
            __IOM uint32_t EP2_RECEIVED_DONE   : 1; /*!< [11..11] EP2 receive done control */
            __IOM uint32_t EP2_SEND_STALL      : 1; /*!< [12..12] EP2 STALL send control */
            __IOM uint32_t EP2_SEND_STALL_DONE : 1; /*!< [13..13] EP2 STALL send complete flag */
            __IOM uint32_t EP2_IN_TOGGLE_VALUE : 1; /*!< [14..14] EP2 IN operation toggle control */
            __IOM uint32_t EP2_IN_TOGGLE_CTRL_EN: 1; /*!< [15..15] EP2_IN_TOGGLE_VALUE effective enable */
            __IOM uint32_t EP2_OUT_TOGGLE_VALUE: 1; /*!< [16..16] EP2 received data packet toggle value */
            __IOM uint32_t EP2_OUT_TOGGLE_WANT : 1; /*!< [17..17] EP2 OUT/Setup data packet toggle comparison value */
            __IOM uint32_t EP2_OUT_TOGGLE_CTRL_EN: 1; /*!< [18..18] EP2_OUT_TOGGLE_WANT value effective enable */
            __IOM uint32_t EP2_OUT_TOGGLE_STATE: 1; /*!< [19..19] EP2 OUT/Setup toggle error flag */
            __IOM uint32_t EP2_OUT_VALID       : 1; /*!< [20..20] EP2 OUT valid flag */
            __IOM uint32_t EP2_ISOCHRONOUS_EN  : 1; /*!< [21..21] EP2 isochronous mode enable */
            __IM  uint32_t          : 2;
            __IOM uint32_t EP2_RECEIVED_ACK    : 1; /*!< [24..24] EP2 received ACK handshake from host */
            __IOM uint32_t EP2_RECEIVED_NAK    : 1; /*!< [25..25] EP2 received NAK handshake from host */
            __IOM uint32_t EP2_RECEIVED_STALL  : 1; /*!< [26..26] EP2 received STALL handshake from host */
            __IOM uint32_t EP2_SEND_HALT       : 1; /*!< [27..27] EP2 send data halt control */
            __IM  uint32_t          : 4;
        } EP2CSR_f;
    };
    union {
        __IOM uint32_t EP3CSR              ; /*!< Offset: 0x4C (read-write) EP3 transfer control register */
        struct {
            __IOM uint32_t EP3_RECEIVED_BYTE   : 8; /*!< [7..0] EP3 received data byte count */
            __IOM uint32_t EP3_EN              : 1; /*!< [8..8] EP3 endpoint enable */
            __IOM uint32_t EP3_FIFOCLR         : 1; /*!< [9..9] EP3 FIFO pointer reset control */
            __IOM uint32_t EP3_DATA_START      : 1; /*!< [10..10] EP3 data send start */
            __IOM uint32_t EP3_RECEIVED_DONE   : 1; /*!< [11..11] EP3 receive done control */
            __IOM uint32_t EP3_SEND_STALL      : 1; /*!< [12..12] EP3 STALL send control */
            __IOM uint32_t EP3_SEND_STALL_DONE : 1; /*!< [13..13] EP3 STALL send complete flag */
            __IOM uint32_t EP3_IN_TOGGLE_VALUE : 1; /*!< [14..14] EP3 IN operation toggle control */
            __IOM uint32_t EP3_IN_TOGGLE_CTRL_EN: 1; /*!< [15..15] EP3_IN_TOGGLE_VALUE effective enable */
            __IOM uint32_t EP3_OUT_TOGGLE_VALUE: 1; /*!< [16..16] EP3 received data packet toggle value */
            __IOM uint32_t EP3_OUT_TOGGLE_WANT : 1; /*!< [17..17] EP3 OUT/Setup data packet toggle comparison value */
            __IOM uint32_t EP3_OUT_TOGGLE_CTRL_EN: 1; /*!< [18..18] EP3_OUT_TOGGLE_WANT value effective enable */
            __IOM uint32_t EP3_OUT_TOGGLE_STATE: 1; /*!< [19..19] EP3 OUT/Setup toggle error flag */
            __IOM uint32_t EP3_OUT_VALID       : 1; /*!< [20..20] EP3 OUT valid flag */
            __IOM uint32_t EP3_ISOCHRONOUS_EN  : 1; /*!< [21..21] EP3 isochronous mode enable */
            __IM  uint32_t          : 2;
            __IOM uint32_t EP3_RECEIVED_ACK    : 1; /*!< [24..24] EP3 received ACK handshake from host */
            __IOM uint32_t EP3_RECEIVED_NAK    : 1; /*!< [25..25] EP3 received NAK handshake from host */
            __IOM uint32_t EP3_RECEIVED_STALL  : 1; /*!< [26..26] EP3 received STALL handshake from host */
            __IOM uint32_t EP3_SEND_HALT       : 1; /*!< [27..27] EP3 send data halt control */
            __IM  uint32_t          : 4;
        } EP3CSR_f;
    };
    union {
        __IOM uint32_t EP4CSR              ; /*!< Offset: 0x50 (read-write) EP4 transfer control register */
        struct {
            __IOM uint32_t EP4_RECEIVED_BYTE   : 8; /*!< [7..0] EP4 received data byte count */
            __IOM uint32_t EP4_EN              : 1; /*!< [8..8] EP4 endpoint enable */
            __IOM uint32_t EP4_FIFOCLR         : 1; /*!< [9..9] EP4 FIFO pointer reset control */
            __IOM uint32_t EP4_DATA_START      : 1; /*!< [10..10] EP4 data send start */
            __IOM uint32_t EP4_RECEIVED_DONE   : 1; /*!< [11..11] EP4 receive done control */
            __IOM uint32_t EP4_SEND_STALL      : 1; /*!< [12..12] EP4 STALL send control */
            __IOM uint32_t EP4_SEND_STALL_DONE : 1; /*!< [13..13] EP4 STALL send complete flag */
            __IOM uint32_t EP4_IN_TOGGLE_VALUE : 1; /*!< [14..14] EP4 IN operation toggle control */
            __IOM uint32_t EP4_IN_TOGGLE_CTRL_EN: 1; /*!< [15..15] EP4_IN_TOGGLE_VALUE effective enable */
            __IOM uint32_t EP4_OUT_TOGGLE_VALUE: 1; /*!< [16..16] EP4 received data packet toggle value */
            __IOM uint32_t EP4_OUT_TOGGLE_WANT : 1; /*!< [17..17] EP4 OUT/Setup data packet toggle comparison value */
            __IOM uint32_t EP4_OUT_TOGGLE_CTRL_EN: 1; /*!< [18..18] EP4_OUT_TOGGLE_WANT value effective enable */
            __IOM uint32_t EP4_OUT_TOGGLE_STATE: 1; /*!< [19..19] EP4 OUT/Setup toggle error flag */
            __IOM uint32_t EP4_OUT_VALID       : 1; /*!< [20..20] EP4 OUT valid flag */
            __IOM uint32_t EP4_ISOCHRONOUS_EN  : 1; /*!< [21..21] EP4 isochronous mode enable */
            __IM  uint32_t          : 2;
            __IOM uint32_t EP4_RECEIVED_ACK    : 1; /*!< [24..24] EP4 received ACK handshake from host */
            __IOM uint32_t EP4_RECEIVED_NAK    : 1; /*!< [25..25] EP4 received NAK handshake from host */
            __IOM uint32_t EP4_RECEIVED_STALL  : 1; /*!< [26..26] EP4 received STALL handshake from host */
            __IOM uint32_t EP4_SEND_HALT       : 1; /*!< [27..27] EP4 send data halt control */
            __IM  uint32_t          : 4;
        } EP4CSR_f;
    };
    union {
        __IOM uint32_t EP5CSR              ; /*!< Offset: 0x54 (read-write) EP5 transfer control register */
        struct {
            __IOM uint32_t EP5_RECEIVED_BYTE   : 8; /*!< [7..0] EP5 received data byte count */
            __IOM uint32_t EP5_EN              : 1; /*!< [8..8] EP5 endpoint enable */
            __IOM uint32_t EP5_FIFOCLR         : 1; /*!< [9..9] EP5 FIFO pointer reset control */
            __IOM uint32_t EP5_DATA_START      : 1; /*!< [10..10] EP5 data send start */
            __IOM uint32_t EP5_RECEIVED_DONE   : 1; /*!< [11..11] EP5 receive done control */
            __IOM uint32_t EP5_SEND_STALL      : 1; /*!< [12..12] EP5 STALL send control */
            __IOM uint32_t EP5_SEND_STALL_DONE : 1; /*!< [13..13] EP5 STALL send complete flag */
            __IOM uint32_t EP5_IN_TOGGLE_VALUE : 1; /*!< [14..14] EP5 IN operation toggle control */
            __IOM uint32_t EP5_IN_TOGGLE_CTRL_EN: 1; /*!< [15..15] EP5_IN_TOGGLE_VALUE effective enable */
            __IOM uint32_t EP5_OUT_TOGGLE_VALUE: 1; /*!< [16..16] EP5 received data packet toggle value */
            __IOM uint32_t EP5_OUT_TOGGLE_WANT : 1; /*!< [17..17] EP5 OUT/Setup data packet toggle comparison value */
            __IOM uint32_t EP5_OUT_TOGGLE_CTRL_EN: 1; /*!< [18..18] EP5_OUT_TOGGLE_WANT value effective enable */
            __IOM uint32_t EP5_OUT_TOGGLE_STATE: 1; /*!< [19..19] EP5 OUT/Setup toggle error flag */
            __IOM uint32_t EP5_OUT_VALID       : 1; /*!< [20..20] EP5 OUT valid flag */
            __IOM uint32_t EP5_ISOCHRONOUS_EN  : 1; /*!< [21..21] EP5 isochronous mode enable */
            __IM  uint32_t          : 2;
            __IOM uint32_t EP5_RECEIVED_ACK    : 1; /*!< [24..24] EP5 received ACK handshake from host */
            __IOM uint32_t EP5_RECEIVED_NAK    : 1; /*!< [25..25] EP5 received NAK handshake from host */
            __IOM uint32_t EP5_RECEIVED_STALL  : 1; /*!< [26..26] EP5 received STALL handshake from host */
            __IOM uint32_t EP5_SEND_HALT       : 1; /*!< [27..27] EP5 send data halt control */
            __IM  uint32_t          : 4;
        } EP5CSR_f;
    };
    union {
        __IOM uint32_t EP6CSR              ; /*!< Offset: 0x58 (read-write) EP6 transfer control register */
        struct {
            __IOM uint32_t EP6_RECEIVED_BYTE   : 8; /*!< [7..0] EP6 received data byte count */
            __IOM uint32_t EP6_EN              : 1; /*!< [8..8] EP6 endpoint enable */
            __IOM uint32_t EP6_FIFOCLR         : 1; /*!< [9..9] EP6 FIFO pointer reset control */
            __IOM uint32_t EP6_DATA_START      : 1; /*!< [10..10] EP6 data send start */
            __IOM uint32_t EP6_RECEIVED_DONE   : 1; /*!< [11..11] EP6 receive done control */
            __IOM uint32_t EP6_SEND_STALL      : 1; /*!< [12..12] EP6 STALL send control */
            __IOM uint32_t EP6_SEND_STALL_DONE : 1; /*!< [13..13] EP6 STALL send complete flag */
            __IOM uint32_t EP6_IN_TOGGLE_VALUE : 1; /*!< [14..14] EP6 IN operation toggle control */
            __IOM uint32_t EP6_IN_TOGGLE_CTRL_EN: 1; /*!< [15..15] EP6_IN_TOGGLE_VALUE effective enable */
            __IOM uint32_t EP6_OUT_TOGGLE_VALUE: 1; /*!< [16..16] EP6 received data packet toggle value */
            __IOM uint32_t EP6_OUT_TOGGLE_WANT : 1; /*!< [17..17] EP6 OUT/Setup data packet toggle comparison value */
            __IOM uint32_t EP6_OUT_TOGGLE_CTRL_EN: 1; /*!< [18..18] EP6_OUT_TOGGLE_WANT value effective enable */
            __IOM uint32_t EP6_OUT_TOGGLE_STATE: 1; /*!< [19..19] EP6 OUT/Setup toggle error flag */
            __IOM uint32_t EP6_OUT_VALID       : 1; /*!< [20..20] EP6 OUT valid flag */
            __IOM uint32_t EP6_ISOCHRONOUS_EN  : 1; /*!< [21..21] EP6 isochronous mode enable */
            __IM  uint32_t          : 2;
            __IOM uint32_t EP6_RECEIVED_ACK    : 1; /*!< [24..24] EP6 received ACK handshake from host */
            __IOM uint32_t EP6_RECEIVED_NAK    : 1; /*!< [25..25] EP6 received NAK handshake from host */
            __IOM uint32_t EP6_RECEIVED_STALL  : 1; /*!< [26..26] EP6 received STALL handshake from host */
            __IOM uint32_t EP6_SEND_HALT       : 1; /*!< [27..27] EP6 send data halt control */
            __IM  uint32_t          : 4;
        } EP6CSR_f;
    };
    __IO uint32_t RESERVED3[9];
    union {
        __IOM uint32_t EP0SENDBN           ; /*!< Offset: 0x80 (read-write) EP0 send data byte count register */
        struct {
            __IOM uint32_t EP0_SEND_BYTE       : 8; /*!< [7..0] EP0 send data byte count */
            __IOM uint32_t EP0_FIFO_LEVEL      : 5; /*!< [12..8] EP0 FIFO valid data count (in words) */
            __IOM uint32_t EP0_UP_OVFL         : 1; /*!< [13..13] EP0 FIFO overflow */
            __IOM uint32_t EP0_DOWN_OVFL       : 1; /*!< [14..14] EP0 FIFO underflow */
            __IM  uint32_t          : 17;
        } EP0SENDBN_f;
    };
    union {
        __IOM uint32_t EP1SENDBN           ; /*!< Offset: 0x84 (read-write) EP1 send data byte count register */
        struct {
            __IOM uint32_t EP1_SEND_BYTE       : 8; /*!< [7..0] EP1 send data byte count */
            __IOM uint32_t EP1_FIFO_LEVEL      : 5; /*!< [12..8] EP1 FIFO valid data count (in words) */
            __IOM uint32_t EP1_UP_OVFL         : 1; /*!< [13..13] EP1 FIFO overflow */
            __IOM uint32_t EP1_DOWN_OVFL       : 1; /*!< [14..14] EP1 FIFO underflow */
            __IM  uint32_t          : 17;
        } EP1SENDBN_f;
    };
    union {
        __IOM uint32_t EP2SENDBN           ; /*!< Offset: 0x88 (read-write) EP2 send data byte count register */
        struct {
            __IOM uint32_t EP2_SEND_BYTE       : 8; /*!< [7..0] EP2 send data byte count */
            __IOM uint32_t EP2_FIFO_LEVEL      : 5; /*!< [12..8] EP2 FIFO valid data count (in words) */
            __IOM uint32_t EP2_UP_OVFL         : 1; /*!< [13..13] EP2 FIFO overflow */
            __IOM uint32_t EP2_DOWN_OVFL       : 1; /*!< [14..14] EP2 FIFO underflow */
            __IM  uint32_t          : 17;
        } EP2SENDBN_f;
    };
    union {
        __IOM uint32_t EP3SENDBN           ; /*!< Offset: 0x8C (read-write) EP3 send data byte count register */
        struct {
            __IOM uint32_t EP3_SEND_BYTE       : 8; /*!< [7..0] EP3 send data byte count */
            __IOM uint32_t EP3_FIFO_LEVEL      : 5; /*!< [12..8] EP3 FIFO valid data count (in words) */
            __IOM uint32_t EP3_UP_OVFL         : 1; /*!< [13..13] EP3 FIFO overflow */
            __IOM uint32_t EP3_DOWN_OVFL       : 1; /*!< [14..14] EP3 FIFO underflow */
            __IM  uint32_t          : 17;
        } EP3SENDBN_f;
    };
    union {
        __IOM uint32_t EP4SENDBN           ; /*!< Offset: 0x90 (read-write) EP4 send data byte count register */
        struct {
            __IOM uint32_t EP4_SEND_BYTE       : 8; /*!< [7..0] EP4 send data byte count */
            __IOM uint32_t EP4_FIFO_LEVEL      : 5; /*!< [12..8] EP4 FIFO valid data count (in words) */
            __IOM uint32_t EP4_UP_OVFL         : 1; /*!< [13..13] EP4 FIFO overflow */
            __IOM uint32_t EP4_DOWN_OVFL       : 1; /*!< [14..14] EP4 FIFO underflow */
            __IM  uint32_t          : 17;
        } EP4SENDBN_f;
    };
    union {
        __IOM uint32_t EP5SENDBN           ; /*!< Offset: 0x94 (read-write) EP5 send data byte count register */
        struct {
            __IOM uint32_t EP5_SEND_BYTE       : 8; /*!< [7..0] EP5 send data byte count */
            __IOM uint32_t EP5_FIFO_LEVEL      : 5; /*!< [12..8] EP5 FIFO valid data count (in words) */
            __IOM uint32_t EP5_UP_OVFL         : 1; /*!< [13..13] EP5 FIFO overflow */
            __IOM uint32_t EP5_DOWN_OVFL       : 1; /*!< [14..14] EP5 FIFO underflow */
            __IM  uint32_t          : 17;
        } EP5SENDBN_f;
    };
    union {
        __IOM uint32_t EP6SENDBN           ; /*!< Offset: 0x98 (read-write) EP6 send data byte count register */
        struct {
            __IOM uint32_t EP6_SEND_BYTE       : 8; /*!< [7..0] EP6 send data byte count */
            __IOM uint32_t EP6_FIFO_LEVEL      : 5; /*!< [12..8] EP6 FIFO valid data count (in words) */
            __IOM uint32_t EP6_UP_OVFL         : 1; /*!< [13..13] EP6 FIFO overflow */
            __IOM uint32_t EP6_DOWN_OVFL       : 1; /*!< [14..14] EP6 FIFO underflow */
            __IM  uint32_t          : 17;
        } EP6SENDBN_f;
    };
    __IO uint32_t RESERVED4[9];
    union {
        __IOM uint32_t INT_ISR             ; /*!< Offset: 0xC0 (read-write) Global interrupt status register */
        struct {
            __IOM uint32_t BUS_RESET_RAW       : 1; /*!< [0..0] Host Reset */
            __IOM uint32_t SUSPEND_RAW         : 1; /*!< [1..1] Host Suspend */
            __IOM uint32_t RESUME_RAW          : 1; /*!< [2..2] Host Resume */
            __IOM uint32_t SOF_RAW             : 1; /*!< [3..3] SOF packet received */
            __IOM uint32_t SETUPTOK_RAW        : 1; /*!< [4..4] Setup token packet received */
            __IOM uint32_t SUDAV_RAW           : 1; /*!< [5..5] Setup data packet received */
            __IM  uint32_t          : 15;
            __IOM uint32_t TURNAROUND_ERROR_RAW: 1; /*!< [21..21] Host ACK packet timeout */
            __IOM uint32_t SETADDR_RAW         : 1; /*!< [22..22] Host set USB device address completed */
            __IOM uint32_t CRC_ERR_RAW         : 1; /*!< [23..23] Token or data packet CRC error */
            __IOM uint32_t DATA_BYTE_MORETHAN_64_RAW: 1; /*!< [24..24] DATA packet length exceeds 64 bytes */
            __IOM uint32_t LPM_ATTR_UPDATA     : 1; /*!< [25..25] LPM Token received */
            __IM  uint32_t          : 4;
            __IOM uint32_t NOEOP_ERR_RAW       : 1; /*!< [30..30] Token packet length exceeds protocol limit or data packet exceeds 64+8 bytes */
            __IOM uint32_t TOGGLE_STATE_ERR_RAW: 1; /*!< [31..31] IN/OUT/Setup toggle error flag */
        } INT_ISR_f;
    };
    union {
        __IOM uint32_t EPINT_ISR           ; /*!< Offset: 0xC4 (read-write) Endpoint interrupt status register */
        struct {
            __IOM uint32_t EP0_IN_RAW          : 1; /*!< [0..0] EP0 IN token received */
            __IOM uint32_t EP0_OUT_RAW         : 1; /*!< [1..1] EP0 OUT interrupt */
            __IOM uint32_t EP0_ACK_RAW         : 1; /*!< [2..2] EP0 ACK status */
            __IOM uint32_t EP0_IN_HANDSHAKE_ERR_RAW: 1; /*!< [3..3] EP0 IN operation host ACK timeout */
            __IOM uint32_t EP1_IN_RAW          : 1; /*!< [4..4] EP1 IN token received */
            __IOM uint32_t EP1_OUT_RAW         : 1; /*!< [5..5] EP1 OUT interrupt */
            __IOM uint32_t EP1_ACK_RAW         : 1; /*!< [6..6] EP1 ACK status */
            __IOM uint32_t EP1_IN_HANDSHAKE_ERR_RAW: 1; /*!< [7..7] EP1 IN operation host ACK timeout */
            __IOM uint32_t EP2_IN_RAW          : 1; /*!< [8..8] EP2 IN token received */
            __IOM uint32_t EP2_OUT_RAW         : 1; /*!< [9..9] EP2 OUT interrupt */
            __IOM uint32_t EP2_ACK_RAW         : 1; /*!< [10..10] EP2 ACK status */
            __IOM uint32_t EP2_IN_HANDSHAKE_ERR_RAW: 1; /*!< [11..11] EP2 IN operation host ACK timeout */
            __IOM uint32_t EP3_IN_RAW          : 1; /*!< [12..12] EP3 IN token received */
            __IOM uint32_t EP3_OUT_RAW         : 1; /*!< [13..13] EP3 OUT interrupt */
            __IOM uint32_t EP3_ACK_RAW         : 1; /*!< [14..14] EP3 ACK status */
            __IOM uint32_t EP3_IN_HANDSHAKE_ERR_RAW: 1; /*!< [15..15] EP3 IN operation host ACK timeout */
            __IOM uint32_t EP4_IN_RAW          : 1; /*!< [16..16] EP4 IN token received */
            __IOM uint32_t EP4_OUT_RAW         : 1; /*!< [17..17] EP4 OUT interrupt */
            __IOM uint32_t EP4_ACK_RAW         : 1; /*!< [18..18] EP4 ACK status */
            __IOM uint32_t EP4_IN_HANDSHAKE_ERR_RAW: 1; /*!< [19..19] EP4 IN operation host ACK timeout */
            __IOM uint32_t EP5_IN_RAW          : 1; /*!< [20..20] EP5 IN token received */
            __IOM uint32_t EP5_OUT_RAW         : 1; /*!< [21..21] EP5 OUT interrupt */
            __IOM uint32_t EP5_ACK_RAW         : 1; /*!< [22..22] EP5 ACK status */
            __IOM uint32_t EP5_IN_HANDSHAKE_ERR_RAW: 1; /*!< [23..23] EP5 IN operation host ACK timeout */
            __IOM uint32_t EP6_IN_RAW          : 1; /*!< [24..24] EP6 IN token received */
            __IOM uint32_t EP6_OUT_RAW         : 1; /*!< [25..25] EP6 OUT interrupt */
            __IOM uint32_t EP6_ACK_RAW         : 1; /*!< [26..26] EP6 ACK status */
            __IOM uint32_t EP6_IN_HANDSHAKE_ERR_RAW: 1; /*!< [27..27] EP6 IN operation host ACK timeout */
            __IM  uint32_t          : 4;
        } EPINT_ISR_f;
    };
    __IO uint32_t RESERVED5;
    union {
        __IOM uint32_t EPINT_ISR2          ; /*!< Offset: 0xCC (read-write) Endpoint interrupt status register 2 */
        struct {
            __IM  uint32_t          : 1;
            __IOM uint32_t EP1_IN_DATA_RAW     : 1; /*!< [1..1] EP1 IN DATA interrupt, device send IN DATA packet completed */
            __IOM uint32_t EP2_IN_DATA_RAW     : 1; /*!< [2..2] EP2 IN DATA interrupt, device send IN DATA packet completed */
            __IOM uint32_t EP3_IN_DATA_RAW     : 1; /*!< [3..3] EP3 IN DATA interrupt, device send IN DATA packet completed */
            __IOM uint32_t EP4_IN_DATA_RAW     : 1; /*!< [4..4] EP4 IN DATA interrupt, device send IN DATA packet completed */
            __IOM uint32_t EP5_IN_DATA_RAW     : 1; /*!< [5..5] EP5 IN DATA interrupt, device send IN DATA packet completed */
            __IOM uint32_t EP6_IN_DATA_RAW     : 1; /*!< [6..6] EP6 IN DATA interrupt, device send IN DATA packet completed */
            __IM  uint32_t          : 25;
        } EPINT_ISR2_f;
    };
    union {
        __IOM uint32_t INT_EN              ; /*!< Offset: 0xD0 (read-write) Global interrupt enable register */
        struct {
            __IOM uint32_t BUS_RESET_EN        : 1; /*!< [0..0] Host Reset interrupt enable */
            __IOM uint32_t SUSPEND_EN          : 1; /*!< [1..1] Host Suspend interrupt enable */
            __IOM uint32_t RESUME_EN           : 1; /*!< [2..2] Host Resume interrupt enable */
            __IOM uint32_t SOF_EN              : 1; /*!< [3..3] SOF packet received interrupt enable */
            __IOM uint32_t SETUPTOK_EN         : 1; /*!< [4..4] Setup token packet received interrupt enable */
            __IOM uint32_t SUDAV_EN            : 1; /*!< [5..5] Setup data packet received interrupt enable */
            __IM  uint32_t          : 15;
            __IOM uint32_t TURNAROUND_ERROR_EN : 1; /*!< [21..21] Host ACK packet timeout interrupt enable */
            __IOM uint32_t SETADDR_EN          : 1; /*!< [22..22] Host set USB device address completed interrupt enable */
            __IOM uint32_t CRC_ERR_EN          : 1; /*!< [23..23] CRC error interrupt enable */
            __IOM uint32_t DATA_BYTE_MORETHAN_64_EN: 1; /*!< [24..24] DATA packet exceeds 64 bytes interrupt enable */
            __IOM uint32_t LPM_ATTR_UPDATE_EN  : 1; /*!< [25..25] LPM_ATTR_UPDATE interrupt enable */
            __IM  uint32_t          : 4;
            __IOM uint32_t NOEOP_ERR_EN        : 1; /*!< [30..30] NOEOP_ERR interrupt enable */
            __IOM uint32_t TOGGLE_STATE_ERR_EN : 1; /*!< [31..31] TOGGLE_STATE_ERR interrupt enable */
        } INT_EN_f;
    };
    union {
        __IOM uint32_t EPINT_EN            ; /*!< Offset: 0xD4 (read-write) Endpoint interrupt enable register */
        struct {
            __IOM uint32_t EP0_IN_EN           : 1; /*!< [0..0] EP0 IN token received interrupt enable */
            __IOM uint32_t EP0_OUT_EN          : 1; /*!< [1..1] EP0 OUT interrupt enable */
            __IOM uint32_t EP0_ACK_EN          : 1; /*!< [2..2] EP0 ACK interrupt enable */
            __IOM uint32_t EP0_IN_HANDSHAKE_ERR_EN: 1; /*!< [3..3] EP0_IN_HANDSHAKE_ERR interrupt enable */
            __IOM uint32_t EP1_IN_EN           : 1; /*!< [4..4] EP1 IN token received interrupt enable */
            __IOM uint32_t EP1_OUT_EN          : 1; /*!< [5..5] EP1 OUT interrupt enable */
            __IOM uint32_t EP1_ACK_EN          : 1; /*!< [6..6] EP1 ACK interrupt enable */
            __IOM uint32_t EP1_IN_HANDSHAKE_ERR_EN: 1; /*!< [7..7] EP1_IN_HANDSHAKE_ERR interrupt enable */
            __IOM uint32_t EP2_IN_EN           : 1; /*!< [8..8] EP2 IN token received interrupt enable */
            __IOM uint32_t EP2_OUT_EN          : 1; /*!< [9..9] EP2 OUT interrupt enable */
            __IOM uint32_t EP2_ACK_EN          : 1; /*!< [10..10] EP2 ACK interrupt enable */
            __IOM uint32_t EP2_IN_HANDSHAKE_ERR_EN: 1; /*!< [11..11] EP2_IN_HANDSHAKE_ERR interrupt enable */
            __IOM uint32_t EP3_IN_EN           : 1; /*!< [12..12] EP3 IN token received interrupt enable */
            __IOM uint32_t EP3_OUT_EN          : 1; /*!< [13..13] EP3 OUT interrupt enable */
            __IOM uint32_t EP3_ACK_EN          : 1; /*!< [14..14] EP3 ACK interrupt enable */
            __IOM uint32_t EP3_IN_HANDSHAKE_ERR_EN: 1; /*!< [15..15] EP3_IN_HANDSHAKE_ERR interrupt enable */
            __IOM uint32_t EP4_IN_EN           : 1; /*!< [16..16] EP4 IN token received interrupt enable */
            __IOM uint32_t EP4_OUT_EN          : 1; /*!< [17..17] EP4 OUT interrupt enable */
            __IOM uint32_t EP4_ACK_EN          : 1; /*!< [18..18] EP4 ACK interrupt enable */
            __IOM uint32_t EP4_IN_HANDSHAKE_ERR_EN: 1; /*!< [19..19] EP4_IN_HANDSHAKE_ERR interrupt enable */
            __IOM uint32_t EP5_IN_EN           : 1; /*!< [20..20] EP5 IN token received interrupt enable */
            __IOM uint32_t EP5_OUT_EN          : 1; /*!< [21..21] EP5 OUT interrupt enable */
            __IOM uint32_t EP5_ACK_EN          : 1; /*!< [22..22] EP5 ACK interrupt enable */
            __IOM uint32_t EP5_IN_HANDSHAKE_ERR_EN: 1; /*!< [23..23] EP5_IN_HANDSHAKE_ERR interrupt enable */
            __IOM uint32_t EP6_IN_EN           : 1; /*!< [24..24] EP6 IN token received interrupt enable */
            __IOM uint32_t EP6_OUT_EN          : 1; /*!< [25..25] EP6 OUT interrupt enable */
            __IOM uint32_t EP6_ACK_EN          : 1; /*!< [26..26] EP6 ACK interrupt enable */
            __IOM uint32_t EP6_IN_HANDSHAKE_ERR_EN: 1; /*!< [27..27] EP6_IN_HANDSHAKE_ERR interrupt enable */
            __IM  uint32_t          : 4;
        } EPINT_EN_f;
    };
    __IO uint32_t RESERVED6;
    union {
        __IOM uint32_t EPINT_EN2           ; /*!< Offset: 0xDC (read-write) Endpoint interrupt enable register 2 */
        struct {
            __IM  uint32_t          : 1;
            __IOM uint32_t EP1_IN_DATA_EN      : 1; /*!< [1..1] EP1 IN DATA interrupt enable */
            __IOM uint32_t EP2_IN_DATA_EN      : 1; /*!< [2..2] EP2 IN DATA interrupt enable */
            __IOM uint32_t EP3_IN_DATA_EN      : 1; /*!< [3..3] EP3 IN DATA interrupt enable */
            __IOM uint32_t EP4_IN_DATA_EN      : 1; /*!< [4..4] EP4 IN DATA interrupt enable */
            __IOM uint32_t EP5_IN_DATA_EN      : 1; /*!< [5..5] EP5 IN DATA interrupt enable */
            __IOM uint32_t EP6_IN_DATA_EN      : 1; /*!< [6..6] EP6 IN DATA interrupt enable */
            __IM  uint32_t          : 25;
        } EPINT_EN2_f;
    };
    __IO uint32_t RESERVED7[8];
    union {
        __IOM uint32_t EP0FIFO             ; /*!< Offset: 0x100 (read-write) EP0 FIFO access entry */
        struct {
            __IOM uint32_t EP0FIFO             : 32; /*!< [31..0] EP0 FIFO data */
        } EP0FIFO_f;
    };
    union {
        __IOM uint32_t EP1FIFO             ; /*!< Offset: 0x104 (read-write) EP1 FIFO access entry */
        struct {
            __IOM uint32_t EP1FIFO             : 32; /*!< [31..0] EP1 FIFO data */
        } EP1FIFO_f;
    };
    union {
        __IOM uint32_t EP2FIFO             ; /*!< Offset: 0x108 (read-write) EP2 FIFO access entry */
        struct {
            __IOM uint32_t EP2FIFO             : 32; /*!< [31..0] EP2 FIFO data */
        } EP2FIFO_f;
    };
    union {
        __IOM uint32_t EP3FIFO             ; /*!< Offset: 0x10C (read-write) EP3 FIFO access entry */
        struct {
            __IOM uint32_t EP3FIFO             : 32; /*!< [31..0] EP3 FIFO data */
        } EP3FIFO_f;
    };
    union {
        __IOM uint32_t EP4FIFO             ; /*!< Offset: 0x110 (read-write) EP4 FIFO access entry */
        struct {
            __IOM uint32_t EP4FIFO             : 32; /*!< [31..0] EP4 FIFO data */
        } EP4FIFO_f;
    };
    union {
        __IOM uint32_t EP5FIFO             ; /*!< Offset: 0x114 (read-write) EP5 FIFO access entry */
        struct {
            __IOM uint32_t EP5FIFO             : 32; /*!< [31..0] EP5 FIFO data */
        } EP5FIFO_f;
    };
    union {
        __IOM uint32_t EP6FIFO             ; /*!< Offset: 0x118 (read-write) EP6 FIFO access entry */
        struct {
            __IOM uint32_t EP6FIFO             : 32; /*!< [31..0] EP6 FIFO data */
        } EP6FIFO_f;
    };
    __IO uint32_t RESERVED8[57];
    union {
        __IOM uint32_t EP0MEM              ; /*!< Offset: 0x200 (read-write) EP0 Memory access entry (64 bytes) */
        struct {
            __IOM uint32_t EP0MEM              : 32; /*!< [31..0] EP0 Memory data */
        } EP0MEM_f;
    };
    __IO uint32_t RESERVED9[15];
    union {
        __IOM uint32_t EP1MEM              ; /*!< Offset: 0x240 (read-write) EP1 Memory access entry (64 bytes) */
        struct {
            __IOM uint32_t EP1MEM              : 32; /*!< [31..0] EP1 Memory data */
        } EP1MEM_f;
    };
    __IO uint32_t RESERVED10[15];
    union {
        __IOM uint32_t EP2MEM              ; /*!< Offset: 0x280 (read-write) EP2 Memory access entry (64 bytes) */
        struct {
            __IOM uint32_t EP2MEM              : 32; /*!< [31..0] EP2 Memory data */
        } EP2MEM_f;
    };
    __IO uint32_t RESERVED11[15];
    union {
        __IOM uint32_t EP3MEM              ; /*!< Offset: 0x2C0 (read-write) EP3 Memory access entry (64 bytes) */
        struct {
            __IOM uint32_t EP3MEM              : 32; /*!< [31..0] EP3 Memory data */
        } EP3MEM_f;
    };
    __IO uint32_t RESERVED12[15];
    union {
        __IOM uint32_t EP4MEM              ; /*!< Offset: 0x300 (read-write) EP4 Memory access entry (64 bytes) */
        struct {
            __IOM uint32_t EP4MEM              : 32; /*!< [31..0] EP4 Memory data */
        } EP4MEM_f;
    };
    __IO uint32_t RESERVED13[15];
    union {
        __IOM uint32_t EP5MEM              ; /*!< Offset: 0x340 (read-write) EP5 Memory access entry (64 bytes) */
        struct {
            __IOM uint32_t EP5MEM              : 32; /*!< [31..0] EP5 Memory data */
        } EP5MEM_f;
    };
    __IO uint32_t RESERVED14[15];
    union {
        __IOM uint32_t EP6MEM              ; /*!< Offset: 0x380 (read-write) EP6 Memory access entry (64 bytes) */
        struct {
            __IOM uint32_t EP6MEM              : 32; /*!< [31..0] EP6 Memory data */
        } EP6MEM_f;
    };
} USB_TypeDef;

/**
 * @brief SDMMC - SDMMC Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t CTRL                ; /*!< Offset: 0x00 (read-write) SDMMC control register */
        struct {
            __IOM uint32_t CONTROLLER_RESET    : 1; /*!< [0..0] Controller reset, auto-clear */
            __IOM uint32_t FIFO_RESET          : 1; /*!< [1..1] FIFO reset, auto-clear */
            __IOM uint32_t DMA_RESET           : 1; /*!< [2..2] DMA reset, auto-clear */
            __IM  uint32_t          : 1;
            __IOM uint32_t INT_ENABLE          : 1; /*!< [4..4] Global interrupt enable */
            __IM  uint32_t          : 1;
            __IOM uint32_t READ_WAIT           : 1; /*!< [6..6] Read wait */
            __IOM uint32_t SEND_IRQ_RESPONSE   : 1; /*!< [7..7] Send IRQ response */
            __IOM uint32_t ABORT_READ_DATA     : 1; /*!< [8..8] Abort read data */
            __IOM uint32_t SEND_CCSD           : 1; /*!< [9..9] Send command completion signal disable */
            __IOM uint32_t SEND_AUTO_STP_CCSD  : 1; /*!< [10..10] Send auto stop CCSD */
            __IOM uint32_t CEATA_DEV_INT_STS   : 1; /*!< [11..11] CE-ATA device interrupt status */
            __IM  uint32_t          : 13;
            __IOM uint32_t USE_INTERNAL_DMAC   : 1; /*!< [25..25] Use IDMA: 0=AHB slave, 1=IDMA */
            __IM  uint32_t          : 6;
        } CTRL_f;
    };
    union {
        __IOM uint32_t PWREN               ; /*!< Offset: 0x04 (read-write) SDMMC power enable register */
        struct {
            __IOM uint32_t POWER_EN            : 2; /*!< [1..0] Power switch: bit[0]=card0, bit[1]=card1 */
            __IM  uint32_t          : 30;
        } PWREN_f;
    };
    union {
        __IOM uint32_t CLKDIV              ; /*!< Offset: 0x08 (read-write) SDMMC clock divider register */
        struct {
            __IOM uint32_t CLK_DIV0            : 8; /*!< [7..0] Clock divider 0, freq=source/(2*n) */
            __IOM uint32_t CLK_DIV1            : 8; /*!< [15..8] Clock divider 1 */
            __IOM uint32_t CLK_DIV2            : 8; /*!< [23..16] Clock divider 2 */
            __IOM uint32_t CLK_DIV3            : 8; /*!< [31..24] Clock divider 3 */
        } CLKDIV_f;
    };
    union {
        __IOM uint32_t CLKSRC              ; /*!< Offset: 0x0C (read-write) SDMMC clock source register */
        struct {
            __IOM uint32_t CLK_SOURCE          : 32; /*!< [31..0] Card clock source select */
        } CLKSRC_f;
    };
    union {
        __IOM uint32_t CLKENA              ; /*!< Offset: 0x10 (read-write) SDMMC clock enable register */
        struct {
            __IOM uint32_t CLK_ENABLE          : 2; /*!< [1..0] Clock enable: bit[0]=card0, bit[1]=card1 */
            __IM  uint32_t          : 14;
            __IOM uint32_t CLK_LOW_POWER       : 2; /*!< [17..16] Clock low power: bit[16]=card0, bit[17]=card1 */
            __IM  uint32_t          : 14;
        } CLKENA_f;
    };
    union {
        __IOM uint32_t TMOUT               ; /*!< Offset: 0x14 (read-write) SDMMC timeout register */
        struct {
            __IOM uint32_t RESPONSE_TIMEOUT    : 8; /*!< [7..0] Response timeout in card clock cycles */
            __IOM uint32_t DATA_TIMEOUT        : 24; /*!< [31..8] Data read timeout in card clock cycles */
        } TMOUT_f;
    };
    union {
        __IOM uint32_t CTYPE               ; /*!< Offset: 0x18 (read-write) SDMMC card type register */
        struct {
            __IOM uint32_t WIDTH0              : 2; /*!< [1..0] Card width 0: bit[0]=card0, bit[1]=card1, 1=4-bit mode */
            __IM  uint32_t          : 14;
            __IOM uint32_t WIDTH1              : 2; /*!< [17..16] Card width 1: bit[16]=card0, bit[17]=card1, 1=8-bit mode */
            __IM  uint32_t          : 14;
        } CTYPE_f;
    };
    union {
        __IOM uint32_t BLKSIZ              ; /*!< Offset: 0x1C (read-write) SDMMC block size register */
        struct {
            __IOM uint32_t BLOCK_SIZE          : 16; /*!< [15..0] Block size in bytes */
            __IM  uint32_t          : 16;
        } BLKSIZ_f;
    };
    union {
        __IOM uint32_t BYTCNT              ; /*!< Offset: 0x20 (read-write) SDMMC byte count register */
        struct {
            __IOM uint32_t BYTE_COUNT          : 32; /*!< [31..0] Transfer byte count */
        } BYTCNT_f;
    };
    union {
        __IOM uint32_t INTMASK             ; /*!< Offset: 0x24 (read-write) SDMMC interrupt mask register */
        struct {
            __IOM uint32_t INT_MASK            : 16; /*!< [15..0] Interrupt mask bits */
            __IOM uint32_t SDIO_INT_MASK       : 2; /*!< [17..16] SDIO interrupt mask: bit[16]=card0, bit[17]=card1 */
            __IM  uint32_t          : 14;
        } INTMASK_f;
    };
    union {
        __IOM uint32_t CMDARG              ; /*!< Offset: 0x28 (read-write) SDMMC command argument register */
        struct {
            __IOM uint32_t CMD_ARG             : 32; /*!< [31..0] Command argument */
        } CMDARG_f;
    };
    union {
        __IOM uint32_t CMD                 ; /*!< Offset: 0x2C (read-write) SDMMC command register */
        struct {
            __IOM uint32_t CMD_INDEX           : 6; /*!< [5..0] Command index */
            __IOM uint32_t RESP_EXPECT         : 1; /*!< [6..6] Response expected */
            __IOM uint32_t RESP_LENGTH         : 1; /*!< [7..7] Response length: 0=short, 1=long */
            __IOM uint32_t CHECK_RESP_CRC      : 1; /*!< [8..8] Check response CRC */
            __IOM uint32_t DATA_EXPECTED       : 1; /*!< [9..9] Data expected from card */
            __IOM uint32_t READ_WRITE          : 1; /*!< [10..10] Read/write: 0=read, 1=write */
            __IOM uint32_t TRANSFER_MODE       : 1; /*!< [11..11] Transfer mode: 0=block, 1=stream */
            __IOM uint32_t SEND_AUTO_STOP      : 1; /*!< [12..12] Send auto stop after data transfer */
            __IOM uint32_t WAIT_PRVDATA_COMPLETE: 1; /*!< [13..13] Wait for previous data transfer complete */
            __IOM uint32_t STOP_ABORT_CMD      : 1; /*!< [14..14] Stop/abort command */
            __IOM uint32_t SEND_INI_SEQ        : 1; /*!< [15..15] Send 80 clocks initialization sequence */
            __IOM uint32_t CARD_NUM            : 5; /*!< [20..16] Card number (physical slot) */
            __IOM uint32_t UPDATE_CLK          : 1; /*!< [21..21] Update clock registers only */
            __IOM uint32_t READ_CEATA_DEVICE   : 1; /*!< [22..22] Read CE-ATA device */
            __IOM uint32_t CCS_EXPECTED        : 1; /*!< [23..23] Command completion signal expected */
            __IOM uint32_t ENA_BOOT            : 1; /*!< [24..24] Enable boot */
            __IOM uint32_t EXPECT_BOOT_ACK     : 1; /*!< [25..25] Expect boot ACK */
            __IOM uint32_t DIS_BOOT            : 1; /*!< [26..26] Disable boot */
            __IOM uint32_t BOOT_MODE           : 1; /*!< [27..27] Boot mode: 0=forced boot, 1=alternative boot */
            __IOM uint32_t VOLT_SWITCH         : 1; /*!< [28..28] Voltage switch for CMD11 */
            __IOM uint32_t USE_HOLD_REG        : 1; /*!< [29..29] Use hold register for CMD/DATA output */
            __IM  uint32_t          : 1;
            __IOM uint32_t START_CMD           : 1; /*!< [31..31] Start command, cleared by hardware when CIU accepts */
        } CMD_f;
    };
    union {
        __IM  uint32_t RESP0               ; /*!< Offset: 0x30 (read-only) SDMMC response 0 register */
        struct {
            __IM  uint32_t RESPONSE0           : 32; /*!< [31..0] Response bits [31:0] */
        } RESP0_f;
    };
    union {
        __IM  uint32_t RESP1               ; /*!< Offset: 0x34 (read-only) SDMMC response 1 register */
        struct {
            __IM  uint32_t RESPONSE1           : 32; /*!< [31..0] Response bits [63:32] */
        } RESP1_f;
    };
    union {
        __IM  uint32_t RESP2               ; /*!< Offset: 0x38 (read-only) SDMMC response 2 register */
        struct {
            __IM  uint32_t RESPONSE2           : 32; /*!< [31..0] Response bits [95:64] */
        } RESP2_f;
    };
    union {
        __IM  uint32_t RESP3               ; /*!< Offset: 0x3C (read-only) SDMMC response 3 register */
        struct {
            __IM  uint32_t RESPONSE3           : 32; /*!< [31..0] Response bits [127:96] */
        } RESP3_f;
    };
    union {
        __IM  uint32_t MINTSTS             ; /*!< Offset: 0x40 (read-only) SDMMC masked interrupt status register */
        struct {
            __IM  uint32_t INT_STATUS          : 16; /*!< [15..0] Masked interrupt status */
            __IM  uint32_t SDIO_INT            : 2; /*!< [17..16] SDIO interrupt: bit[16]=card0, bit[17]=card1 */
            __IM  uint32_t          : 14;
        } MINTSTS_f;
    };
    union {
        __IOM uint32_t RINTSTS             ; /*!< Offset: 0x44 (read-write) SDMMC raw interrupt status register */
        struct {
            __IOM uint32_t RAW_INT_STATUS      : 16; /*!< [15..0] Raw interrupt status, write 1 to clear */
            __IOM uint32_t RAW_SDIO_INT        : 2; /*!< [17..16] Raw SDIO interrupt, write 1 to clear */
            __IM  uint32_t          : 14;
        } RINTSTS_f;
    };
    union {
        __IM  uint32_t STATUS              ; /*!< Offset: 0x48 (read-only) SDMMC status register */
        struct {
            __IM  uint32_t FIFO_RX_LEVEL       : 1; /*!< [0..0] FIFO RX level */
            __IM  uint32_t FIFO_TX_LEVEL       : 1; /*!< [1..1] FIFO TX level */
            __IM  uint32_t FIFO_EMPTY          : 1; /*!< [2..2] FIFO empty */
            __IM  uint32_t FIFO_FULL           : 1; /*!< [3..3] FIFO full */
            __IM  uint32_t CMD_FSM_STATUS      : 4; /*!< [7..4] Command FSM state */
            __IM  uint32_t DATA_3_STATUS       : 1; /*!< [8..8] DATA[3] raw value for card detect */
            __IM  uint32_t DATA_BUSY           : 1; /*!< [9..9] Card data busy (DATA[0] inverted) */
            __IM  uint32_t DATA_STATE_MC_BUSY  : 1; /*!< [10..10] Data state machine busy */
            __IM  uint32_t RESP_INDEX          : 6; /*!< [16..11] Previous response index */
            __IM  uint32_t FIFO_COUNT          : 13; /*!< [29..17] FIFO count */
            __IM  uint32_t          : 2;
        } STATUS_f;
    };
    union {
        __IOM uint32_t FIFOTH              ; /*!< Offset: 0x4C (read-write) SDMMC FIFO threshold register */
        struct {
            __IOM uint32_t TX_LEVEL            : 12; /*!< [11..0] TX FIFO threshold level */
            __IM  uint32_t          : 4;
            __IOM uint32_t RX_LEVEL            : 12; /*!< [27..16] RX FIFO threshold level */
            __IM  uint32_t          : 4;
        } FIFOTH_f;
    };
    __IO uint32_t RESERVED0[3];
    union {
        __IM  uint32_t TCBCNT              ; /*!< Offset: 0x5C (read-only) SDMMC transferred card byte count register */
        struct {
            __IM  uint32_t TRANS_CARD_BYTE_CNT : 32; /*!< [31..0] Transferred card byte count */
        } TCBCNT_f;
    };
    union {
        __IM  uint32_t TBBCNT              ; /*!< Offset: 0x60 (read-only) SDMMC transferred FIFO byte count register */
        struct {
            __IM  uint32_t TRANS_FIFO_BYTE_CNT : 32; /*!< [31..0] Transferred FIFO byte count */
        } TBBCNT_f;
    };
    union {
        __IOM uint32_t DEBNCE              ; /*!< Offset: 0x64 (read-write) SDMMC debounce register */
        struct {
            __IOM uint32_t DEBOUNCE_COUNT      : 24; /*!< [23..0] Debounce clock cycles for card detect */
            __IM  uint32_t          : 8;
        } DEBNCE_f;
    };
    __IO uint32_t RESERVED1[3];
    union {
        __IOM uint32_t UHS_REG             ; /*!< Offset: 0x74 (read-write) SDMMC UHS register */
        struct {
            __IM  uint32_t          : 16;
            __IOM uint32_t DDR_REG             : 2; /*!< [17..16] DDR mode: bit[16]=card0, bit[17]=card1 */
            __IM  uint32_t          : 14;
        } UHS_REG_f;
    };
    union {
        __IOM uint32_t RSTN                ; /*!< Offset: 0x78 (read-write) SDMMC hardware reset register */
        struct {
            __IOM uint32_t CARD_RSTN           : 2; /*!< [1..0] Card reset: bit[0]=card0, bit[1]=card1 */
            __IM  uint32_t          : 30;
        } RSTN_f;
    };
    __IO uint32_t RESERVED2;
    union {
        __IOM uint32_t BMOD                ; /*!< Offset: 0x80 (read-write) SDMMC bus mode register (IDMAC) */
        struct {
            __IOM uint32_t SWR                 : 1; /*!< [0..0] Software reset, auto-clear */
            __IOM uint32_t FB                  : 1; /*!< [1..1] Fixed burst: 0=SINGLE+INCR, 1=SINGLE+INCR4/8/16 */
            __IOM uint32_t DSL                 : 5; /*!< [6..2] Descriptor skip length (words) */
            __IOM uint32_t DE                  : 1; /*!< [7..7] IDMAC enable */
            __IOM uint32_t PBL                 : 3; /*!< [10..8] Programmable burst length */
            __IM  uint32_t          : 21;
        } BMOD_f;
    };
    union {
        __OM  uint32_t PLDMND              ; /*!< Offset: 0x84 (write-only) SDMMC poll demand register */
        struct {
            __OM  uint32_t PD                  : 32; /*!< [31..0] Poll demand */
        } PLDMND_f;
    };
    union {
        __IOM uint32_t DBADDR              ; /*!< Offset: 0x88 (read-write) SDMMC descriptor list base address register */
        struct {
            __IOM uint32_t SDL                 : 32; /*!< [31..0] Start of descriptor list */
        } DBADDR_f;
    };
    union {
        __IOM uint32_t IDSTS               ; /*!< Offset: 0x8C (read-write) SDMMC IDMA status register */
        struct {
            __IOM uint32_t TI                  : 1; /*!< [0..0] Transmit interrupt, write 1 to clear */
            __IOM uint32_t RI                  : 1; /*!< [1..1] Receive interrupt, write 1 to clear */
            __IOM uint32_t FBE                 : 1; /*!< [2..2] Fatal bus error, write 1 to clear */
            __IM  uint32_t          : 1;
            __IOM uint32_t DU                  : 1; /*!< [4..4] Descriptor unavailable, write 1 to clear */
            __IOM uint32_t CES                 : 1; /*!< [5..5] Card error summary, write 1 to clear */
            __IM  uint32_t          : 2;
            __IOM uint32_t NIS                 : 1; /*!< [8..8] Normal interrupt summary, write 1 to clear */
            __IOM uint32_t AIS                 : 1; /*!< [9..9] Abnormal interrupt summary, write 1 to clear */
            __IOM uint32_t FBE_CODE            : 3; /*!< [12..10] Fatal bus error code */
            __IOM uint32_t FSM_STATUS          : 4; /*!< [16..13] IDMAC FSM state */
            __IM  uint32_t          : 15;
        } IDSTS_f;
    };
    union {
        __IOM uint32_t IDINTEN             ; /*!< Offset: 0x90 (read-write) SDMMC IDMA interrupt enable register */
        struct {
            __IOM uint32_t TI_EN               : 1; /*!< [0..0] Transmit interrupt enable */
            __IOM uint32_t RI_EN               : 1; /*!< [1..1] Receive interrupt enable */
            __IOM uint32_t FBE_EN              : 1; /*!< [2..2] Fatal bus error enable */
            __IM  uint32_t          : 1;
            __IOM uint32_t DU_EN               : 1; /*!< [4..4] Descriptor unavailable enable */
            __IOM uint32_t CES_EN              : 1; /*!< [5..5] Card error summary enable */
            __IM  uint32_t          : 2;
            __IOM uint32_t NIS_EN              : 1; /*!< [8..8] Normal interrupt summary enable */
            __IOM uint32_t AIS_EN              : 1; /*!< [9..9] Abnormal interrupt summary enable */
            __IM  uint32_t          : 22;
        } IDINTEN_f;
    };
    union {
        __IM  uint32_t DSCADDR             ; /*!< Offset: 0x94 (read-only) SDMMC current host descriptor address register */
        struct {
            __IM  uint32_t HDA                 : 32; /*!< [31..0] Host descriptor address pointer */
        } DSCADDR_f;
    };
    union {
        __IM  uint32_t BUFADDR             ; /*!< Offset: 0x98 (read-only) SDMMC current host buffer address register */
        struct {
            __IM  uint32_t HBA                 : 32; /*!< [31..0] Host buffer address pointer */
        } BUFADDR_f;
    };
    __IO uint32_t RESERVED3[25];
    union {
        __IOM uint32_t CTHCTL              ; /*!< Offset: 0x100 (read-write) SDMMC card threshold control register */
        struct {
            __IOM uint32_t CARD_RD_THR_EN      : 1; /*!< [0..0] Card read threshold enable */
            __IOM uint32_t BUSY_CLR_INT_EN     : 1; /*!< [1..1] Busy clear interrupt enable */
            __IM  uint32_t          : 14;
            __IOM uint32_t CARD_THR            : 7; /*!< [22..16] Card threshold size */
            __IM  uint32_t          : 9;
        } CTHCTL_f;
    };
    __IO uint32_t RESERVED4[2];
    union {
        __IOM uint32_t EMMC_DDR            ; /*!< Offset: 0x10C (read-write) SDMMC eMMC DDR register */
        struct {
            __IOM uint32_t HALF_START_BIT      : 2; /*!< [1..0] Half start bit: bit[0]=card0, bit[1]=card1 */
            __IM  uint32_t          : 30;
        } EMMC_DDR_f;
    };
    union {
        __IOM uint32_t ENA_SHIFT           ; /*!< Offset: 0x110 (read-write) SDMMC phase shift control register */
        struct {
            __IOM uint32_t EN_SHIFT            : 4; /*!< [3..0] Phase shift: bit[1:0]=card0, bit[3:2]=card1 */
            __IM  uint32_t          : 28;
        } ENA_SHIFT_f;
    };
    __IO uint32_t RESERVED5[59];
    union {
        __IOM uint32_t DATA                ; /*!< Offset: 0x200 (read-write) SDMMC FIFO data access register */
        struct {
            __IOM uint32_t DATA                : 32; /*!< [31..0] FIFO data access channel */
        } DATA_f;
    };
} SDMMC_TypeDef;

/**
 * @brief HRNG - HRNG Register Structure
 */
typedef struct {
    union {
        __IOM uint32_t CTRL                ; /*!< Offset: 0x00 (read-write) HRNG control register */
        struct {
            __IOM uint32_t RBG0_EN             : 1; /*!< [0..0] Random source 0 enable: 1=enable, 0=disable */
            __IOM uint32_t RBG1_EN             : 1; /*!< [1..1] Random source 1 enable: 1=enable, 0=disable */
            __IOM uint32_t HRNG_LFSRCLK_SEL    : 1; /*!< [2..2] LFSR clock select: 0=system clock, 1=HRNG divided clock */
            __IM  uint32_t          : 1;
            __IOM uint32_t RBG0_TESTEN         : 1; /*!< [4..4] RBG0 test enable: 1=enable, 0=disable */
            __IOM uint32_t RBG1_TESTEN         : 1; /*!< [5..5] RBG1 test enable: 1=enable, 0=disable */
            __IM  uint32_t          : 26;
        } CTRL_f;
    };
    union {
        __IOM uint32_t LFSR                ; /*!< Offset: 0x04 (read-write) HRNG linear feedback shift register (random data) */
        struct {
            __IOM uint32_t LFSR                : 32; /*!< [31..0] Linear feedback shift register data */
        } LFSR_f;
    };
} HRNG_TypeDef;

/* =========================================  End of section using anonymous unions  ========================================= */
#if defined (__CC_ARM)
  #pragma pop
#elif defined (__ICCARM__)
  /* leave anonymous unions enabled */
#elif (__ARMCC_VERSION >= 6010050)
  #pragma clang diagnostic pop
#elif defined (__GNUC__)
  /* anonymous unions are enabled by default */
#elif defined (__TMS470__)
  /* anonymous unions are enabled by default */
#elif defined (__TASKING__)
  #pragma warning restore
#elif defined (__CSMC__)
  /* anonymous unions are enabled by default */
#else
  #warning Not supported compiler type
#endif

/* ================================================================================ */
/* ================       Peripheral Base Address Definitions     ================ */
/* ================================================================================ */

#define SPI1_BASE                           (0x40030000UL)
#define SPI2_BASE                           (0x40030400UL)
#define SPI3_BASE                           (0x40030800UL)
#define SPI4_BASE                           (0x40030C00UL)
#define SPI7_BASE                           (0x52005000UL)
#define OSPI1_BASE                          (0x520D1400UL)
#define OSPI2_BASE                          (0x520D1800UL)
#define TIM1_BASE                           (0x40012C00UL)
#define TIM8_BASE                           (0x40013400UL)
#define TIM2_BASE                           (0x40000000UL)
#define TIM3_BASE                           (0x40000400UL)
#define TIM4_BASE                           (0x40000800UL)
#define TIM5_BASE                           (0x40000C00UL)
#define TIM6_BASE                           (0x40001000UL)
#define TIM7_BASE                           (0x40001400UL)
#define TIM10_BASE                          (0x40018400UL)
#define TIM9_BASE                           (0x40018000UL)
#define TIM26_BASE                          (0x4000A400UL)
#define RTC_BASE                            (0x40002800UL)
#define WDT_BASE                            (0x40002C00UL)
#define IWDT_BASE                           (0x40003000UL)
#define I2S1_BASE                           (0x40003400UL)
#define I2S2_BASE                           (0x40003800UL)
#define UART1_BASE                          (0x40013800UL)
#define UART2_BASE                          (0x40004400UL)
#define UART3_BASE                          (0x40004800UL)
#define UART4_BASE                          (0x40004C00UL)
#define UART5_BASE                          (0x40005000UL)
#define UART6_BASE                          (0x40013C00UL)
#define UART7_BASE                          (0x40009800UL)
#define UART8_BASE                          (0x40009C00UL)
#define I2C1_BASE                           (0x40005400UL)
#define I2C2_BASE                           (0x40005800UL)
#define FDCAN1_BASE                         (0x40022000UL)
#define FDCAN2_BASE                         (0x40022400UL)
#define PMU_BASE                            (0x40007000UL)
#define LPTIM1_BASE                         (0x40007C00UL)
#define LPUART1_BASE                        (0x40008000UL)
#define LPUART2_BASE                        (0x40008400UL)
#define EFUSE1_BASE                         (0x4000E000UL)
#define SYSCFG_BASE                         (0x40010000UL)
#define COMP1_BASE                          (0x40010200UL)
#define EXTI_BASE                           (0x40010400UL)
#define DMAC1_BASE                          (0x40020000UL)
#define DMAC2_BASE                          (0x40020400UL)
#define RCC_BASE                            (0x40021000UL)
#define CRC_BASE                            (0x40023000UL)
#define ETH_BASE                            (0x40028000UL)
#define GPIOA_BASE                          (0x48000000UL)
#define GPIOB_BASE                          (0x48000400UL)
#define GPIOC_BASE                          (0x48000800UL)
#define GPIOD_BASE                          (0x48000C00UL)
#define GPIOE_BASE                          (0x48001000UL)
#define GPIOF_BASE                          (0x48001400UL)
#define GPIOG_BASE                          (0x48001800UL)
#define GPIOH_BASE                          (0x48001C00UL)
#define ADC_BASE                            (0x50000400UL)
#define DAC1_BASE                           (0x50000800UL)
#define ETH_DLYB_BASE                       (0x52006C00UL)
#define OSPI1_DLYB_BASE                     (0x520D2000UL)
#define OSPI2_DLYB_BASE                     (0x520D2400UL)
#define SDMMC_DLYBS_BASE                    (0x520CA000UL)
#define SDMMC_DLYBD_BASE                    (0x520CA800UL)
#define USB1_BASE                           (0x40040000UL)
#define SDMMC_BASE                          (0x520C8000UL)
#define HRNG_BASE                           (0x50060800UL)

/* ================================================================================ */
/* ================       Peripheral Declarations                 ================ */
/* ================================================================================ */

#define SPI1                           ((SPI_TypeDef *) SPI1_BASE)
#define SPI2                           ((SPI_TypeDef *) SPI2_BASE)
#define SPI3                           ((SPI_TypeDef *) SPI3_BASE)
#define SPI4                           ((SPI_TypeDef *) SPI4_BASE)
#define SPI7                           ((SPI_TypeDef *) SPI7_BASE)
#define OSPI1                          ((OSPI_TypeDef *) OSPI1_BASE)
#define OSPI2                          ((OSPI_TypeDef *) OSPI2_BASE)
#define TIM1                           ((TIM1_TypeDef *) TIM1_BASE)
#define TIM8                           ((TIM1_TypeDef *) TIM8_BASE)
#define TIM2                           ((TIM2_TypeDef *) TIM2_BASE)
#define TIM3                           ((TIM2_TypeDef *) TIM3_BASE)
#define TIM4                           ((TIM2_TypeDef *) TIM4_BASE)
#define TIM5                           ((TIM2_TypeDef *) TIM5_BASE)
#define TIM6                           ((TIM6_TypeDef *) TIM6_BASE)
#define TIM7                           ((TIM6_TypeDef *) TIM7_BASE)
#define TIM10                          ((TIM10_TypeDef *) TIM10_BASE)
#define TIM9                           ((TIM9_TypeDef *) TIM9_BASE)
#define TIM26                          ((TIM64B_TypeDef *) TIM26_BASE)
#define RTC                            ((RTC_TypeDef *) RTC_BASE)
#define WDT                            ((WDT_TypeDef *) WDT_BASE)
#define IWDT                           ((IWDT_TypeDef *) IWDT_BASE)
#define I2S1                           ((I2S_TypeDef *) I2S1_BASE)
#define I2S2                           ((I2S_TypeDef *) I2S2_BASE)
#define UART1                          ((UART_TypeDef *) UART1_BASE)
#define UART2                          ((UART_TypeDef *) UART2_BASE)
#define UART3                          ((UART_TypeDef *) UART3_BASE)
#define UART4                          ((UART_TypeDef *) UART4_BASE)
#define UART5                          ((UART_TypeDef *) UART5_BASE)
#define UART6                          ((UART_TypeDef *) UART6_BASE)
#define UART7                          ((UART_TypeDef *) UART7_BASE)
#define UART8                          ((UART_TypeDef *) UART8_BASE)
#define I2C1                           ((I2C_TypeDef *) I2C1_BASE)
#define I2C2                           ((I2C_TypeDef *) I2C2_BASE)
#define FDCAN1                         ((FDCAN_TypeDef *) FDCAN1_BASE)
#define FDCAN2                         ((FDCAN_TypeDef *) FDCAN2_BASE)
#define PMU                            ((PMU_TypeDef *) PMU_BASE)
#define LPTIM1                         ((LPTIM_TypeDef *) LPTIM1_BASE)
#define LPUART1                        ((LPUART_TypeDef *) LPUART1_BASE)
#define LPUART2                        ((LPUART_TypeDef *) LPUART2_BASE)
#define EFUSE1                         ((EFUSE_TypeDef *) EFUSE1_BASE)
#define SYSCFG                         ((SYSCFG_TypeDef *) SYSCFG_BASE)
#define COMP1                          ((COMP_TypeDef *) COMP1_BASE)
#define EXTI                           ((EXTI_TypeDef *) EXTI_BASE)
#define DMAC1                          ((DMAC_TypeDef *) DMAC1_BASE)
#define DMAC2                          ((DMAC_TypeDef *) DMAC2_BASE)
#define RCC                            ((RCC_TypeDef *) RCC_BASE)
#define CRC                            ((CRC_TypeDef *) CRC_BASE)
#define ETH                            ((ETH_TypeDef *) ETH_BASE)
#define GPIOA                          ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB                          ((GPIO_TypeDef *) GPIOB_BASE)
#define GPIOC                          ((GPIO_TypeDef *) GPIOC_BASE)
#define GPIOD                          ((GPIO_TypeDef *) GPIOD_BASE)
#define GPIOE                          ((GPIO_TypeDef *) GPIOE_BASE)
#define GPIOF                          ((GPIO_TypeDef *) GPIOF_BASE)
#define GPIOG                          ((GPIO_TypeDef *) GPIOG_BASE)
#define GPIOH                          ((GPIO_TypeDef *) GPIOH_BASE)
#define ADC                            ((ADC_TypeDef *) ADC_BASE)
#define DAC1                           ((DAC_TypeDef *) DAC1_BASE)
#define ETH_DLYB                       ((DLYB_TypeDef *) ETH_DLYB_BASE)
#define OSPI1_DLYB                     ((DLYB_TypeDef *) OSPI1_DLYB_BASE)
#define OSPI2_DLYB                     ((DLYB_TypeDef *) OSPI2_DLYB_BASE)
#define SDMMC_DLYBS                    ((DLYB_TypeDef *) SDMMC_DLYBS_BASE)
#define SDMMC_DLYBD                    ((DLYB_TypeDef *) SDMMC_DLYBD_BASE)
#define USB1                           ((USB_TypeDef *) USB1_BASE)
#define SDMMC                          ((SDMMC_TypeDef *) SDMMC_BASE)
#define HRNG                           ((HRNG_TypeDef *) HRNG_BASE)

#ifdef __cplusplus
}
#endif

#endif /* __ACM32P4XX_H__ */
