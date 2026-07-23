#ifndef RT_CONFIG_H__
#define RT_CONFIG_H__

/* RT-Thread Kernel */

/* klibc options */

/* rt_vsnprintf options */

/* end of rt_vsnprintf options */

/* rt_vsscanf options */

/* end of rt_vsscanf options */

/* rt_memset options */

/* end of rt_memset options */

/* rt_memcpy options */

/* end of rt_memcpy options */

/* rt_memmove options */

/* end of rt_memmove options */

/* rt_memcmp options */

/* end of rt_memcmp options */

/* rt_strstr options */

/* end of rt_strstr options */

/* rt_strcasecmp options */

/* end of rt_strcasecmp options */

/* rt_strncpy options */

/* end of rt_strncpy options */

/* rt_strcpy options */

/* end of rt_strcpy options */

/* rt_strncmp options */

/* end of rt_strncmp options */

/* rt_strcmp options */

/* end of rt_strcmp options */

/* rt_strlen options */

/* end of rt_strlen options */

/* rt_strnlen options */

/* end of rt_strnlen options */
/* end of klibc options */
#define RT_NAME_MAX 12
#define RT_CPUS_NR 1
#define RT_ALIGN_SIZE 8
#define RT_THREAD_PRIORITY_32
#define RT_THREAD_PRIORITY_MAX 32
#define RT_TICK_PER_SECOND 100
#define RT_USING_OVERFLOW_CHECK
#define RT_USING_HOOK
#define RT_HOOK_USING_FUNC_PTR
#define RT_USING_IDLE_HOOK
#define RT_IDLE_HOOK_LIST_SIZE 4
#define IDLE_THREAD_STACK_SIZE 256
#define RT_USING_TIMER_SOFT
#define RT_TIMER_THREAD_PRIO 4
#define RT_TIMER_THREAD_STACK_SIZE 512

/* kservice options */

/* end of kservice options */
#define RT_USING_DEBUG
#define RT_DEBUGING_ASSERT
#define RT_DEBUGING_COLOR
#define RT_DEBUGING_CONTEXT

/* Inter-Thread communication */

#define RT_USING_SEMAPHORE
#define RT_USING_MUTEX
#define RT_USING_EVENT
#define RT_USING_MAILBOX
#define RT_USING_MESSAGEQUEUE
/* end of Inter-Thread communication */

/* Memory Management */

#define RT_USING_MEMPOOL
#define RT_USING_SMALL_MEM
#define RT_USING_SMALL_MEM_AS_HEAP
#define RT_USING_HEAP
/* end of Memory Management */
#define RT_USING_DEVICE
#define RT_USING_DEVICE_OPS
#define RT_USING_CONSOLE
#define RT_CONSOLEBUF_SIZE 128
#define RT_CONSOLE_DEVICE_NAME "uart1"
#define RT_USING_CONSOLE_OUTPUT_CTL
#define RT_VER_NUM 0x50300
#define RT_BACKTRACE_LEVEL_MAX_NR 32
/* end of RT-Thread Kernel */

/* RT-Thread Components */

#define RT_USING_COMPONENTS_INIT
#define RT_USING_USER_MAIN
#define RT_MAIN_THREAD_STACK_SIZE 2048
#define RT_MAIN_THREAD_PRIORITY 10
#define RT_USING_MSH
#define RT_USING_FINSH
#define FINSH_USING_MSH
#define FINSH_THREAD_NAME "tshell"
#define FINSH_THREAD_PRIORITY 20
#define FINSH_THREAD_STACK_SIZE 2048
#define FINSH_USING_HISTORY
#define FINSH_HISTORY_LINES 5
#define FINSH_USING_SYMTAB
#define FINSH_CMD_SIZE 80
#define MSH_USING_BUILT_IN_COMMANDS
#define FINSH_USING_DESCRIPTION
#define FINSH_ARG_MAX 10
#define FINSH_USING_OPTION_COMPLETION

/* DFS: device virtual file system */

#define RT_USING_DFS
#define DFS_USING_POSIX
#define DFS_USING_WORKDIR
#define DFS_FD_MAX 16
#define RT_USING_DFS_V1
#define DFS_FILESYSTEMS_MAX 4
#define DFS_FILESYSTEM_TYPES_MAX 4
#define RT_USING_DFS_DEVFS
/* end of DFS: device virtual file system */

/* Device Drivers */

#define RT_USING_DEVICE_IPC
#define RT_UNAMED_PIPE_NUMBER 64
#define RT_USING_SERIAL
#define RT_USING_SERIAL_V2
#define RT_SERIAL_BUF_STRATEGY_OVERWRITE
#define RT_SERIAL_USING_DMA
#define RT_USING_SPI
#define RT_USING_SPI_ISR
#define RT_USING_PIN
/* end of Device Drivers */

/* C/C++ and POSIX layer */

/* ISO-ANSI C layer */

/* Timezone and Daylight Saving Time */

#define RT_LIBC_USING_LIGHT_TZ_DST
#define RT_LIBC_TZ_DEFAULT_HOUR 8
#define RT_LIBC_TZ_DEFAULT_MIN 0
#define RT_LIBC_TZ_DEFAULT_SEC 0
/* end of Timezone and Daylight Saving Time */
/* end of ISO-ANSI C layer */

/* POSIX (Portable Operating System Interface) layer */


/* Interprocess Communication (IPC) */


/* Socket is in the 'Network' category */

/* end of Interprocess Communication (IPC) */
/* end of POSIX (Portable Operating System Interface) layer */
/* end of C/C++ and POSIX layer */

/* Network */

#define RT_USING_NETDEV
#define NETDEV_USING_IFCONFIG
#define NETDEV_USING_PING
#define NETDEV_USING_NETSTAT
#define NETDEV_USING_AUTO_DEFAULT
#define NETDEV_IPV4 1
#define NETDEV_IPV6 0
#define RT_USING_LWIP
#define RT_USING_LWIP212
#define RT_USING_LWIP_VER_NUM 0x20102
#define RT_LWIP_MEM_ALIGNMENT 4
#define RT_LWIP_IGMP
#define RT_LWIP_ICMP
#define RT_LWIP_DNS
#define RT_LWIP_DHCP
#define IP_SOF_BROADCAST 1
#define IP_SOF_BROADCAST_RECV 1

/* Static IPv4 Address */

#define RT_LWIP_IPADDR "192.168.16.50"
#define RT_LWIP_GWADDR "192.168.16.100"
#define RT_LWIP_MSKADDR "255.255.255.0"
/* end of Static IPv4 Address */
#define RT_LWIP_UDP
#define RT_LWIP_TCP
#define RT_LWIP_RAW
#define RT_MEMP_NUM_NETCONN 8
#define RT_LWIP_PBUF_NUM 8
#define RT_LWIP_RAW_PCB_NUM 4
#define RT_LWIP_UDP_PCB_NUM 4
#define RT_LWIP_TCP_PCB_NUM 4
#define RT_LWIP_TCP_SEG_NUM 32
#define RT_LWIP_TCP_SND_BUF 8192
#define RT_LWIP_TCP_WND 8192
#define RT_LWIP_TCPTHREAD_PRIORITY 8
#define RT_LWIP_TCPTHREAD_MBOX_SIZE 32
#define RT_LWIP_TCPTHREAD_STACKSIZE 2048
#define RT_LWIP_ETHTHREAD_PRIORITY 10
#define RT_LWIP_ETHTHREAD_STACKSIZE 2048
#define RT_LWIP_ETHTHREAD_MBOX_SIZE 32
#define LWIP_NETIF_STATUS_CALLBACK 1
#define LWIP_NETIF_LINK_CALLBACK 1
#define RT_LWIP_NETIF_NAMESIZE 6
#define SO_REUSE 1
#define LWIP_SO_RCVTIMEO 1
#define LWIP_SO_SNDTIMEO 1
#define LWIP_SO_RCVBUF 1
#define LWIP_SO_LINGER 0
#define LWIP_NETIF_LOOPBACK 0
#define RT_LWIP_USING_PING
/* end of Network */

/* Memory protection */

/* end of Memory protection */

/* Utilities */

/* end of Utilities */

/* Using USB legacy version */

/* end of Using USB legacy version */
/* end of RT-Thread Components */

/* RT-Thread Utestcases */

/* end of RT-Thread Utestcases */

/* Hardware Drivers Config */

#define SOC_ACM32P40N128CCT

/* ACM32P4xx Chip Configuration */

#define SOC_SRAM_START_ADDR 0x20000000
#define SOC_SRAM_SIZE 0x80
#define SOC_FLASH_START_ADDR 0x08000000
#define SOC_FLASH_SIZE 0x400
/* end of ACM32P4xx Chip Configuration */

/* ACM32P4xx Onboard Peripheral Drivers */

#define BSP_USING_UART1
#define BSP_UART1_PINS_PA9
#define BSP_USING_UART2
#define BSP_UART2_PINS_PD5
#define BSP_USING_UART2_DMA
#define BSP_UART2_DMA_MAP_DEFAULT
#define BSP_UART2_TX_DMA_UNIT 1
#define BSP_UART2_TX_DMA_CH 3
#define BSP_UART2_RX_DMA_UNIT 2
#define BSP_UART2_RX_DMA_CH 0
#define BSP_USING_SPI1
#define BSP_SPI1_PINS_PE
#define BSP_SPI1_CS_PIN 77
#define BSP_USING_SPI1_DMA
#define BSP_SPI1_DMA_MAP_DEFAULT
#define BSP_SPI1_TX_DMA_UNIT 2
#define BSP_SPI1_TX_DMA_CH 1
#define BSP_SPI1_RX_DMA_UNIT 2
#define BSP_SPI1_RX_DMA_CH 3
#define BSP_USING_GPIO1
#define BSP_USING_ETH
#define BSP_ETH_PHY_RST_PIN 30
#define BSP_USING_IPERF
#define LWIP_SUPPORT_CUSTOM_PBUF 1
#define BSP_ETH_STATIC_IP
#define BSP_ETH_STATIC_IP0 192
#define BSP_ETH_STATIC_IP1 168
#define BSP_ETH_STATIC_IP2 16
#define BSP_ETH_STATIC_IP3 50
#define BSP_ETH_STATIC_GW0 192
#define BSP_ETH_STATIC_GW1 168
#define BSP_ETH_STATIC_GW2 16
#define BSP_ETH_STATIC_GW3 100
#define BSP_ETH_STATIC_NM0 255
#define BSP_ETH_STATIC_NM1 255
#define BSP_ETH_STATIC_NM2 255
#define BSP_ETH_STATIC_NM3 0
/* end of ACM32P4xx Onboard Peripheral Drivers */
/* end of Hardware Drivers Config */

/* BEGIN_BSP_RESOURCE_CHECK - auto by SCons, do not edit */
/* ---- resource map ----
 *   DMA map:
 *     UART2_TX   -> DMA1_CH3
 *     UART2_RX   -> DMA2_CH0
 *     SPI1_TX    -> DMA2_CH1
 *     SPI1_RX    -> DMA2_CH3
 *   Pin groups:
 *     UART1    PA9 PA10
 *     UART2    PD5 PD6
 *     SPI1     PE12 PE11 PE10 PE13
 *     ETH      (RMII board pins)
 */
/* BSP resource check OK (no DMA / pin conflicts) */
/* END_BSP_RESOURCE_CHECK */
#endif
