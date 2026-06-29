/**
 * @file wifi_example.c
 * @brief Wi-Fi 连接示例（BCM43438 + WHD + ThreadX）
 *
 * 功能流程：
 *  1. 初始化 WHD 驱动（SDIO + BCM43438 + ThreadX）
 *  2. 打印 Wi-Fi MAC 地址
 *  3. 扫描周边 AP 并打印列表（最多 WIFI_SCAN_MAX_AP 个）
 *  4. 使用 WPA2-AES-PSK 连接预配置的 SSID
 *  5. 连接成功后每 5 秒打印一次 RSSI
 *  6. 断开连接并挂起任务
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

//===========================================
// 头文件包含
//===========================================

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "acm32p4.h"
#include "hardware/system.h"
#include "tx_api.h"

#include "shell.h"
#include "adapter_shell_port.h"
#include "adapter_whd.h"
#include "whd_wifi_api.h"
#include "whd_types.h"

//===========================================
// 配置选项
//===========================================

/** @brief 通过 Letter-shell 输出日志（shell 任务初始化后才可用） */
#define printf(...)  shellPrint(&shell, __VA_ARGS__)

/** @brief 目标 AP SSID（根据实际环境修改） */
#define WIFI_SSID                "YourSSID"

/** @brief 目标 AP 密码（WPA2-PSK） */
#define WIFI_PASSWORD            "YourPassword"

/** @brief 扫描结果最大缓存条目数 */
#define WIFI_SCAN_MAX_AP         10U

/** @brief 连接成功后 RSSI 打印间隔（ms） */
#define WIFI_RSSI_INTERVAL_MS    5000U

/** @brief Wi-Fi 任务优先级 */
#define TASK_WIFI_PRIO           10U

/** @brief Wi-Fi 任务栈大小（字节）*/
#define TASK_WIFI_STK_SIZE       4096U

//===========================================
// 内部状态变量
//===========================================

static TX_THREAD s_task_wifi_tcb;
static uint64_t  s_task_wifi_stk[TASK_WIFI_STK_SIZE / 8];

//===========================================
// 内部辅助函数
//===========================================

static void task_wifi_entry(ULONG arg);
static void s_print_mac(const whd_mac_t *mac);
static void s_do_scan(whd_interface_t ifp);
static bool s_do_join(whd_interface_t ifp);

//===========================================
// 主函数与内核入口
//===========================================

/**
 * @brief 程序入口
 */
int main(void)
{
    system_init();
    tx_kernel_enter();

    while (1) {
        /* 正常情况下不会执行到这里 */
    }
}

/**
 * @brief ThreadX 应用定义
 *
 * @details 由 ThreadX 内核在启动时调用一次，用于创建所有任务。
 *
 * @param first_unused_memory 内核空闲内存起始地址（本示例不使用字节池）
 */
void tx_application_define(void *first_unused_memory)
{
    (void)first_unused_memory;

    shell_port_init();

    tx_thread_create(
        &s_task_wifi_tcb,
        "task_wifi",
        task_wifi_entry,
        0U,
        &s_task_wifi_stk[0],
        TASK_WIFI_STK_SIZE,
        TASK_WIFI_PRIO,
        TASK_WIFI_PRIO,
        TX_NO_TIME_SLICE,
        TX_AUTO_START
    );
}

//===========================================
// 辅助函数实现
//===========================================

/**
 * @brief 打印 MAC 地址（XX:XX:XX:XX:XX:XX 格式）
 */
static void s_print_mac(const whd_mac_t *mac)
{
    printf("%02X:%02X:%02X:%02X:%02X:%02X\r\n",
           mac->octet[0], mac->octet[1], mac->octet[2],
           mac->octet[3], mac->octet[4], mac->octet[5]);
}

/**
 * @brief 扫描周边 AP 并打印结果
 *
 * @param ifp  WHD STA 接口句柄
 */
static void s_do_scan(whd_interface_t ifp)
{
    static whd_sync_scan_result_t s_scan_buf[WIFI_SCAN_MAX_AP];
    uint32_t count = WIFI_SCAN_MAX_AP;
    whd_result_t ret;

    printf("[WIFI] 开始扫描 AP...\r\n");

    ret = whd_wifi_scan_synch(ifp, s_scan_buf, &count);
    if (ret != WHD_SUCCESS) {
        printf("[WIFI] 扫描失败，ret=0x%08lX\r\n", (unsigned long)ret);
        return;
    }

    printf("[WIFI] 扫描到 %lu 个 AP：\r\n", (unsigned long)count);
    printf("  %-32s  %-3s  %-4s\r\n", "SSID", "CH", "RSSI");
    printf("  %-32s  %-3s  %-4s\r\n", "--------------------------------", "---", "----");

    for (uint32_t i = 0U; i < count; i++) {
        const whd_sync_scan_result_t *ap = &s_scan_buf[i];

        /* SSID 末尾补零，安全打印 */
        char ssid_str[SSID_NAME_SIZE + 1U];
        uint8_t ssid_len = ap->SSID.length;
        if (ssid_len > SSID_NAME_SIZE) {
            ssid_len = SSID_NAME_SIZE;
        }
        (void)memcpy(ssid_str, ap->SSID.value, ssid_len);
        ssid_str[ssid_len] = '\0';

        printf("  %-32s  %-3u  %d dBm\r\n",
               ssid_str,
               (unsigned int)ap->channel,
               (int)ap->signal_strength);
    }
}

/**
 * @brief 连接预配置的 AP（WPA2-AES-PSK）
 *
 * @param ifp  WHD STA 接口句柄
 * @return true  连接成功
 * @return false 连接失败
 */
static bool s_do_join(whd_interface_t ifp)
{
    whd_ssid_t   ssid;
    whd_result_t ret;

    const char *ssid_str = WIFI_SSID;
    const char *key_str  = WIFI_PASSWORD;
    uint8_t     ssid_len = (uint8_t)strlen(ssid_str);
    uint8_t     key_len  = (uint8_t)strlen(key_str);

    if (ssid_len > SSID_NAME_SIZE) {
        ssid_len = SSID_NAME_SIZE;
    }

    ssid.length = ssid_len;
    (void)memcpy(ssid.value, ssid_str, ssid_len);

    printf("[WIFI] 正在连接 SSID: %s ...\r\n", ssid_str);

    ret = whd_wifi_join(ifp,
                        &ssid,
                        WHD_SECURITY_WPA2_AES_PSK,
                        (const uint8_t *)key_str,
                        key_len);

    if (ret != WHD_SUCCESS) {
        printf("[WIFI] 连接失败，ret=0x%08lX\r\n", (unsigned long)ret);
        return false;
    }

    printf("[WIFI] 连接成功！\r\n");
    return true;
}

//===========================================
// 任务入口
//===========================================

/**
 * @brief Wi-Fi 示例任务入口
 *
 * @details 完整的初始化 → 扫描 → 连接 → 周期性 RSSI 打印流程。
 *
 * @param arg 未使用
 */
static void task_wifi_entry(ULONG arg)
{
    (void)arg;

    /* 等待 shell 任务完成初始化 */
    tx_thread_sleep(200U);

    printf("\r\n[WIFI] ====== Wi-Fi 示例启动 ======\r\n");

    /* 1. 初始化 WHD（SDIO 识别 + BCM43438 固件下载 + 接口创建） */
    printf("[WIFI] 初始化 WHD 驱动...\r\n");
    if (!adapter_whd_init()) {
        printf("[WIFI] WHD 初始化失败，任务退出\r\n");
        tx_thread_suspend(tx_thread_identify());
        return;
    }
    printf("[WIFI] WHD 初始化成功\r\n");

    whd_interface_t ifp = adapter_whd_get_sta_iface();
    if (ifp == NULL) {
        printf("[WIFI] 获取 STA 接口失败，任务退出\r\n");
        tx_thread_suspend(tx_thread_identify());
        return;
    }

    /* 2. 打印 MAC 地址 */
    whd_mac_t mac;
    if (whd_wifi_get_mac_address(ifp, &mac) == WHD_SUCCESS) {
        printf("[WIFI] Wi-Fi MAC: ");
        s_print_mac(&mac);
    }

    /* 3. 扫描周边 AP */
    s_do_scan(ifp);

    /* 4. 连接目标 AP */
    if (!s_do_join(ifp)) {
        tx_thread_suspend(tx_thread_identify());
        return;
    }

    /* 5. 每隔 WIFI_RSSI_INTERVAL_MS 打印一次 RSSI，共打印 12 次（约 1 分钟） */
    const ULONG sleep_ticks =
        (ULONG)(WIFI_RSSI_INTERVAL_MS / (1000U / TX_TIMER_TICKS_PER_SECOND));

    printf("[WIFI] 进入 RSSI 监控（每 %u s，共 12 次）...\r\n",
           (unsigned int)(WIFI_RSSI_INTERVAL_MS / 1000U));

    for (uint32_t i = 0U; i < 12U; i++) {
        tx_thread_sleep(sleep_ticks);

        int32_t rssi = 0;
        if (whd_wifi_get_rssi(ifp, &rssi) == WHD_SUCCESS) {
            printf("[WIFI] RSSI = %ld dBm\r\n", (long)rssi);
        }
    }

    /* 6. 断开连接 */
    printf("[WIFI] 断开连接...\r\n");
    (void)whd_wifi_leave(ifp);
    printf("[WIFI] 已断开，示例结束\r\n");

    tx_thread_suspend(tx_thread_identify());
}