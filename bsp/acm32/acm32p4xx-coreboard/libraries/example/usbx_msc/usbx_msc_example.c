/**
 * @file threadx_demo.c
 * @brief ACM32P4 ThreadX 基础使用示例
 *
 * @details
 * 演示 ThreadX RTOS 的基本用法：
 * - 系统初始化与内核启动
 * - 任务创建（tx_thread_create）
 * - 任务延时（tx_thread_sleep）
 * - 临界区保护（TX_DISABLE / TX_RESTORE）
 *
 * 当前任务列表：
 * - 空闲任务（优先级 31）：每秒打印一次系统 Tick
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 * @date 2026-03-23
 */

//===========================================
// 头文件包含
//===========================================

#include "SEGGER_RTT.h"
#include "acm32p4.h"
#include "adapter_usbx_device.h"
#include "hardware/system.h"
#include "tx_api.h"
#include "../../main/board/memory/board_aps6408l.h"

//===========================================
// 配置选项
//===========================================

#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)

/** @brief 空闲任务优先级（数值越小优先级越高，31 为最低） */
#define TASK_IDLE_PRIO 31U

/** @brief 空闲任务栈大小（字节） */
#define TASK_IDLE_STK_SIZE 1024U

//===========================================
// 静态变量
//===========================================

static TX_THREAD task_idle_tcb; ///< 空闲任务控制块
static uint64_t
    task_idle_stk[TASK_IDLE_STK_SIZE / 8]; ///< 空闲任务栈（8 字节对齐）

static volatile uint32_t idle_counter; ///< 空闲任务运行计数

//===========================================
// 函数声明
//===========================================

static void task_idle_entry(ULONG thread_input);

//===========================================
// 主函数与内核入口
//===========================================

/**
 * @brief 程序入口
 *
 * @details 完成系统硬件初始化后进入 ThreadX 内核，内核启动后
 *          调用 tx_application_define() 创建所有任务，之后不再返回。
 */
int main(void) {
  /* 系统初始化（时钟、Cache、中断等） */
  system_init();
	bsp_aps6408l_power_up_init();
	bsp_aps6408l_memory_mode_init();
	
  SEGGER_RTT_Init();
  /* 进入 ThreadX 内核，不再返回 */
  tx_kernel_enter();

  while (1) {
    /* 正常情况下不会执行到这里 */
  }
}

/**
 * @brief ThreadX 应用定义函数
 *
 * @details 由 ThreadX 内核在启动时调用一次，用于创建所有任务和通信对象。
 *
 * @param first_unused_memory 内核分配的空闲内存起始地址（本示例不使用字节池）
 */
void tx_application_define(void *first_unused_memory) {
  UINT usb_ret;

  (void)first_unused_memory;

  /* 上电进入 ThreadX 后立即初始化 USBX 设备栈，确保 USB 设备自动枚举。 */
  usb_ret = usb_device_init();
  if (usb_ret != UX_SUCCESS) {
    printf("[USB] usb_device_init 失败，ret=0x%02X\r\n", usb_ret);
    while (1) {
    }
  }

  /* 创建空闲任务 */
  tx_thread_create(
      &task_idle_tcb,     /* 任务控制块 */
      "task_idle",        /* 任务名称 */
      task_idle_entry,    /* 任务入口函数 */
      0,                  /* 传入参数 */
      &task_idle_stk[0],  /* 栈基地址 */
      TASK_IDLE_STK_SIZE, /* 栈大小（字节） */
      TASK_IDLE_PRIO,     /* 任务优先级 */
      TASK_IDLE_PRIO,     /* 抢占阈值（与优先级相同，禁用抢占阈值功能） */
      TX_NO_TIME_SLICE,   /* 不使用时间片轮转 */
      TX_AUTO_START       /* 创建后立即就绪 */
  );
}

//===========================================
// 任务实现
//===========================================

/**
 * @brief 空闲任务入口
 *
 * @details 每隔 1000 个 ThreadX Tick（约 1 秒）打印一次系统当前 Tick 值。
 *          打印前后通过 TX_DISABLE / TX_RESTORE 保护临界区，避免与中断竞争。
 *
 * @param thread_input 创建任务时传入的参数（本任务未使用）
 */
static void task_idle_entry(ULONG thread_input) {
  TX_INTERRUPT_SAVE_AREA(void)
  thread_input;

  while (1) {
    /* 进入临界区，禁止中断，保护 printf 输出 */
    TX_DISABLE
    printf("threadx sysytem tick: %lu\r\n", tx_time_get());
    TX_RESTORE
    /* 进入临界区，禁止中断，保护 printf 输出 */
    tx_thread_sleep(1000); /* 延时 1000 个 Tick */
  }
}
