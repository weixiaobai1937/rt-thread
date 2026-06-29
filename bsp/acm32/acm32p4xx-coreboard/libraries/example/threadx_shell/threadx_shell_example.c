/**
 * @file threadx_shell_example.c
 * @brief letter-shell + ThreadX 集成示例（ACM32P4，UART1 DMA）
 *
 * @details
 * 演示如何在 ThreadX RTOS 中集成 letter-shell：
 *   - 系统初始化与内核启动
 *   - shell 硬件初始化（UART1 + DMA1）
 *   - 创建 shell 任务
 *   - 创建用户业务任务（以 1 Hz LED 闪烁为例）
 *   - 使用 SHELL_EXPORT_CMD 注册自定义命令
 *
 * 硬件资源
 * ---------
 *   UART1  ：TX=PA9(AF1)，RX=PA10(AF1)，115200 bps
 *   DMA1   ：CH0=UART1 RX（循环链表），CH1=UART1 TX（一次性）
 *   NVIC   ：UART1_IRQn 优先级 4（仅空闲中断）
 *
 * 项目配置要求
 * -----------
 *  1. 将以下文件加入 Keil 工程：
 *       src/main/shell/shell_port.c
 *       src/lib/letter-shell/src/shell.c
 *       src/lib/letter-shell/src/shell_cmd_list.c
 *  2. 将以下路径加入包含目录：
 *       src/main/shell
 *       src/lib/letter-shell/src
 *  3. 在编译器预定义宏中添加：
 *       SHELL_CFG_USER="shell_cfg_user.h"
 *  4. 在链接脚本（.sct）中保留 shellCommand section（用于命令导出）：
 *       *(shellCommand)
 */

#include "acm32p4.h"
#include "hardware/system.h"
#include "hardware/gpio.h"
#include "tx_api.h"
#include "shell.h"
#include "adapter_shell_port.h"

//=============================================================================
// 配置选项
//=============================================================================

/** @brief Shell 任务优先级（高于业务任务，确保交互响应） */
#define TASK_SHELL_PRIO         6U

/** @brief Shell 任务栈大小（字节，建议 ≥ 2048） */
#define TASK_SHELL_STK_SIZE     2048U

/** @brief 业务任务优先级 */
#define TASK_BLINK_PRIO         20U

/** @brief 业务任务栈大小（字节） */
#define TASK_BLINK_STK_SIZE     512U

//=============================================================================
// 静态变量（任务控制块与栈）
//=============================================================================

static TX_THREAD task_shell_tcb;
static uint64_t task_shell_stk[TASK_SHELL_STK_SIZE / 8];

static TX_THREAD task_blink_tcb;
static uint64_t task_blink_stk[TASK_BLINK_STK_SIZE / 8];

//=============================================================================
// 函数声明
//=============================================================================

static void task_shell_entry(ULONG arg);
static void task_blink_entry(ULONG arg);

/* 自定义 Shell 命令 */
static int  cmd_hello(void);
static int  cmd_version(void);
static int  cmd_reset(void);
static int  cmd_gpio_read(int port, int pin);

//=============================================================================
// 主函数与内核入口
//=============================================================================

/**
 * @brief 程序入口
 */
int main(void)
{
    system_init();      /* 系统时钟、Cache、中断等初始化 */
    tx_kernel_enter();  /* 进入 ThreadX 内核，不再返回  */

    while (1) {
        /* 正常情况下不会执行到这里 */
    }
}

/**
 * @brief ThreadX 应用定义
 *
 * @details 由 ThreadX 内核在启动时调用一次，用于创建所有任务。
 *          建议在此函数末尾调用 shell_port_init()（此时内核已准备好
 *          接受 tx_semaphore_create / tx_mutex_create 等调用）。
 *
 * @param first_unused_memory 内核空闲内存起始地址（本示例不使用字节池）
 */
void tx_application_define(void *first_unused_memory)
{
    (void)first_unused_memory;

    /* 注意：此处中断尚未开启（ThreadX 低层初始化已执行 CPSID i），
     * 不可调用任何依赖中断完成的操作（如 shell_port_init）。
     * shell_port_init 移至 task_shell_entry 中执行。 */

    /* 创建 shell 任务 */
    tx_thread_create(
        &task_shell_tcb,
        "task_shell",
        task_shell_entry,
        0,
        &task_shell_stk[0],
        TASK_SHELL_STK_SIZE,
        TASK_SHELL_PRIO,
        TASK_SHELL_PRIO,
        TX_NO_TIME_SLICE,
        TX_AUTO_START
    );

    /* 创建业务任务（LED 闪烁示例） */
    tx_thread_create(
        &task_blink_tcb,
        "task_blink",
        task_blink_entry,
        0,
        &task_blink_stk[0],
        TASK_BLINK_STK_SIZE,
        TASK_BLINK_PRIO,
        TASK_BLINK_PRIO,
        TX_NO_TIME_SLICE,
        TX_AUTO_START
    );
}

//=============================================================================
// 任务入口
//=============================================================================

/**
 * @brief Shell 任务入口
 *
 * @details 直接调用 letter-shell 提供的 shellTask()。
 *          shellTask 内部包含 while(1) 循环，持续读取一个字节并处理。
 */
static void task_shell_entry(ULONG arg)
{
    (void)arg;
    /* 在任务上下文初始化 shell（此时中断已开启，DMA TC 中断可正常触发） */
    shell_port_init();
    shellTask((void *)&shell);  /* 传入全局 Shell 实例指针 */
}

/**
 * @brief 业务任务入口（LED 闪烁，1 Hz，可替换为实际业务逻辑）
 *
 * @note PA5 为常用 LED 引脚，请根据实际硬件修改。
 */
static void task_blink_entry(ULONG arg)
{
    (void)arg;

    gpio_init_output(PA5, false);

    while (1) {
        gpio_toggle(PA5);
        // shellPrint(&shell, "LED Toggled\r\n");
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 2U);  /* 500 ms */
    }
}

//=============================================================================
// Shell 命令实现
//=============================================================================

/**
 * @brief 命令：hello — 打印问候语
 * @usage  hello
 */
static int cmd_hello(void)
{
    shellPrint(&shell, "Hello from ACM32P4 + letter-shell!\r\n");
    return 0;
}

/**
 * @brief 命令：version — 打印固件版本信息
 * @usage  version
 */
static int cmd_version(void)
{
    shellPrint(&shell,
               "Firmware : v1.0.0\r\n"
               "Shell    : " SHELL_VERSION "\r\n"
               "RTOS     : ThreadX\r\n"
               "MCU      : ACM32P4\r\n");
    return 0;
}

/**
 * @brief 命令：reset — 软件复位
 * @usage  reset
 */
static int cmd_reset(void)
{
    shellPrint(&shell, "Resetting...\r\n");
    NVIC_SystemReset();
    return 0;  /* 不会执行到此处 */
}

/**
 * @brief 命令：gpio_read — 读取 GPIO 电平
 *
 * @usage  gpio_read <port> <pin>
 *         port : 0=PA, 1=PB, 2=PC, ...
 *         pin  : 0~15
 *
 * @note 此命令仅作演示，实际读取逻辑可根据 gpio.h API 完善。
 */
static int cmd_gpio_read(int port, int pin)
{
    if (port < 0 || port > 9 || pin < 0 || pin > 15) {
        shellPrint(&shell, "Usage: gpio_read <port(0-9)> <pin(0-15)>\r\n");
        return -1;
    }
    /* 构造 pin id：letter-shell 将参数按整型传递 */
    int pin_id = port * 16 + pin;
    int level = (int)gpio_get((gpio_pin_t)pin_id);
    shellPrint(&shell,
               "P%c%d = %d\r\n",
               (char)('A' + port), pin, level);
    return level;
}

//=============================================================================
// Shell 命令导出（利用 .sct 保留 shellCommand section）
//=============================================================================

SHELL_EXPORT_CMD(
    SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC) | SHELL_CMD_DISABLE_RETURN,
    hello, cmd_hello,
    打印问候语
);

SHELL_EXPORT_CMD(
    SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC) | SHELL_CMD_DISABLE_RETURN,
    version, cmd_version,
    显示固件版本信息
);

SHELL_EXPORT_CMD(
    SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC) | SHELL_CMD_DISABLE_RETURN,
    reset, cmd_reset,
    软件复位
);

SHELL_EXPORT_CMD(
    SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC),
    gpio_read, cmd_gpio_read,
    读取 GPIO 电平 用法: gpio_read <port> <pin>
);
