/**
 * @file filex_example.c
 * @brief ACM32P4 FileX + LevelX NOR Flash 文件系统示例
 *
 * @details
 * 演示 5 个场景，均运行在 ThreadX 任务中：
 * 1. 格式化并挂载媒体（首次上电必须格式化）
 * 2. 创建文件并写入数据
 * 3. 打开文件并读取数据
 * 4. 目录遍历（列出根目录所有文件）
 * 5. 删除文件并验证媒体空间
 *
 * 使用说明：
 * - 修改 DEFAULT_SCENARIO 切换场景
 * - 场景 1 会格式化 Flash，首次使用或恢复出厂时执行
 * - 场景 2~5 需要已格式化的媒体，直接挂载即可
 *
 * 硬件配置（来自 adapter_levelx_nor.h）：
 * - Flash 偏移基地址：0x00020000
 * - 总块数：8 块
 * - 每块可用扇区：15（共 120 个逻辑扇区）
 * - 逻辑扇区大小：512 字节
 * - 可用总容量：120 × 512 = 60 KB
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 * @date 2026-03-23
 */

//===========================================
// 头文件包含
//===========================================

#include "adapter_shell_port.h"
#include "acm32p4.h"
#include "fx_api.h"
#include "hardware/system.h"
#include "lx_api.h"
#include "adapter_levelx_nor.h"
#include "tx_api.h"
#include <string.h>

//===========================================
// 配置选项
//===========================================

#define printf(...) shellPrint(&shell, __VA_ARGS__)

/** @brief 选择要演示的场景（1~5） */
#define DEFAULT_SCENARIO 1

/** @brief FileX 任务优先级 */
#define TASK_FX_PRIO 10U

/** @brief FileX 任务栈大小（字节，须足够大以容纳 filex_example 的 4 KB
 * 局部缓冲） */
#define TASK_FX_STK_SIZE 8192U

/** @brief 媒体缓冲区大小（须 >= FILEX_SECTOR_SIZE，一般 512 字节即可） */
#define MEDIA_BUF_SIZE FILEX_SECTOR_SIZE

/** @brief 文件读写测试缓冲区大小（字节） */
#define FILE_BUF_SIZE 256U

/** @brief 测试文件名 */
#define TEST_FILE_NAME "TEST.TXT"

/** @brief 目录遍历时每次获取的文件名缓冲区长度 */
#define DIR_NAME_LEN FX_MAX_LONG_NAME_LEN

//===========================================
// 驱动函数声明（实现在 adapter_filex_driver.c）
//===========================================

VOID fx_nor_flash_ACM32P4xx_driver(FX_MEDIA *media_ptr);

//===========================================
// 静态变量
//===========================================

/* ThreadX 任务 */
static TX_THREAD task_fx_tcb;
static uint64_t task_fx_stk[TASK_FX_STK_SIZE / 8];

/* FileX 媒体对象与缓冲区 */
static FX_MEDIA nor_disk;
static UCHAR media_buf[MEDIA_BUF_SIZE];

/* 文件对象（跨场景复用） */
static FX_FILE file_obj;

//===========================================
// 内部函数声明
//===========================================

static void task_fx_entry(ULONG thread_input);

static void scenario_1_format_and_open(void);
static void scenario_2_write_file(void);
static void scenario_3_read_file(void);
static void scenario_4_list_dir(void);
static void scenario_5_delete_and_stat(void);

//===========================================
// 主函数与内核入口
//===========================================

/**
 * @brief 程序入口
 *
 * @details 依次完成系统初始化、LevelX 初始化、FileX 初始化，
 *          然后进入 ThreadX 内核，由任务执行示例。
 */
int main(void) {
  system_init();
  norflash_init_default();
  tx_kernel_enter();

  while (1) {
    /* 正常情况下不会执行到这里 */
  }
}

/**
 * @brief ThreadX 应用定义
 *
 * @param first_unused_memory 未使用（本示例不使用动态内存池）
 */
void tx_application_define(void *first_unused_memory) {
  (void)first_unused_memory;

  /* 初始化 LevelX 和 FileX 子系统 */
  if (lx_nor_flash_initialize() != LX_SUCCESS) {
    printf("LevelX 初始化失败\r\n");
    return;
  }

  fx_system_initialize();

  tx_thread_create(&task_fx_tcb, "task_filex", task_fx_entry, DEFAULT_SCENARIO,
                   &task_fx_stk[0], TASK_FX_STK_SIZE, TASK_FX_PRIO,
                   TASK_FX_PRIO, TX_NO_TIME_SLICE, TX_AUTO_START);
}

//===========================================
// 工具辅助函数
//===========================================

/**
 * @brief 打印 FileX / LevelX 操作结果
 *
 * @param op     操作描述字符串
 * @param status 操作返回值（FX_SUCCESS = 0 表示成功）
 * @return bool  true = 成功
 */
static bool check_status(const char *op, UINT status) {
  if (status == FX_SUCCESS) {
    printf("[OK]  %s\r\n", op);
    return true;
  }
  printf("[ERR] %s 失败，错误码: 0x%02X\r\n", op, status);
  return false;
}

/**
 * @brief 确保媒体已挂载（非格式化场景公用入口）
 *
 * @return bool true = 挂载成功
 */
static bool open_media(void) {
  UINT status = fx_media_open(&nor_disk, "NOR", fx_nor_flash_ACM32P4xx_driver,
                              FX_NULL, media_buf, sizeof(media_buf));
  return check_status("fx_media_open", status);
}

/**
 * @brief 刷新并关闭媒体
 */
static void close_media(void) {
  fx_media_flush(&nor_disk);
  fx_media_close(&nor_disk);
  printf("媒体已关闭\r\n");
}

//===========================================
// 场景 1：格式化并重新挂载媒体
//===========================================

/**
 * @brief 格式化 NOR Flash 并验证挂载成功
 *
 * @details 调用 fx_media_format() 写入 FAT 引导扇区和目录表，
 *          之后立即重新 open 验证文件系统可正常读写。
 *          每次格式化会清除所有文件，谨慎使用。
 */
static void scenario_1_format_and_open(void) {
  UINT status;

  printf("\r\n======== 场景 1：格式化媒体 ========\r\n");
  printf("配置: %u 个扇区 x %u 字节 = %u 字节\r\n",
         (unsigned)FILEX_TOTAL_SECTORS, (unsigned)FILEX_SECTOR_SIZE,
         (unsigned)(FILEX_TOTAL_SECTORS * FILEX_SECTOR_SIZE));

  /* 格式化前先物理擦除整个 LevelX 区域，清除残留元数据 */
  printf("预擦除 LevelX 区域 0x%08X 共 %u 字节...\r\n",
         (unsigned)NORFLASH_BASE_ADDRESS, (unsigned)NORFLASH_TOTAL_SIZE);
  if (!norflash_erase(NORFLASH_BASE_ADDRESS, NORFLASH_TOTAL_SIZE)) {
    printf("[ERR] 预擦除失败\r\n");
    return;
  }
  printf("预擦除完成\r\n");

  /* 格式化（会触发 FX_DRIVER_INIT -> LevelX 打开 Flash） */
  status = fx_media_format(
      &nor_disk, fx_nor_flash_ACM32P4xx_driver, /* 驱动入口 */
      FX_NULL,                                  /* 驱动信息（本驱动不使用） */
      media_buf, sizeof(media_buf), "NOR_DISK", /* 卷标 */
      FILEX_NUMBER_OF_FATS, FILEX_DIRECTORY_ENTRIES, FILEX_HIDDEN_SECTORS,
      FILEX_TOTAL_SECTORS, FILEX_SECTOR_SIZE, FILEX_SECTORS_PER_CLUSTER,
      FILEX_HEADS, FILEX_SECTORS_PER_TRACK);
  if (!check_status("fx_media_format", status)) {
    return;
  }

  /* 格式化后重新挂载 */
  if (!open_media()) {
    return;
  }

  /* 读取媒体信息验证 */
  ULONG available;
  status = fx_media_space_available(&nor_disk, &available);
  if (check_status("fx_media_space_available", status)) {
    printf("可用空间: %lu 字节\r\n", available);
  }

  close_media();
  printf("结果: 成功\r\n");
}

//===========================================
// 场景 2：创建文件并写入数据
//===========================================

/**
 * @brief 创建测试文件并写入递增数据
 *
 * @details 流程：挂载 -> 创建文件 -> 写入 256 字节 -> 关闭文件 -> 卸载媒体
 */
static void scenario_2_write_file(void) {
  UINT status;
  UCHAR tx_buf[FILE_BUF_SIZE];

  printf("\r\n======== 场景 2：写入文件 ========\r\n");
  printf("文件名: %s  写入: %u 字节\r\n", TEST_FILE_NAME,
         (unsigned)sizeof(tx_buf));

  if (!open_media()) {
    return;
  }

  /* 准备测试数据（递增字节） */
  for (uint32_t i = 0; i < sizeof(tx_buf); i++) {
    tx_buf[i] = (UCHAR)(i & 0xFFU);
  }

  /* 创建文件（若已存在则删除重建，保持幂等） */
  fx_file_delete(&nor_disk, TEST_FILE_NAME);

  status = fx_file_create(&nor_disk, TEST_FILE_NAME);
  if (!check_status("fx_file_create", status)) {
    close_media();
    return;
  }

  /* 打开文件（写） */
  status =
      fx_file_open(&nor_disk, &file_obj, TEST_FILE_NAME, FX_OPEN_FOR_WRITE);
  if (!check_status("fx_file_open (write)", status)) {
    close_media();
    return;
  }

  /* 写入数据 */
  status = fx_file_write(&file_obj, tx_buf, sizeof(tx_buf));
  check_status("fx_file_write", status);

  if (status == FX_SUCCESS) {
    printf("已写入: %u 字节\r\n", (unsigned)sizeof(tx_buf));
  }

  fx_file_close(&file_obj);
  close_media();
  printf("结果: %s\r\n", (status == FX_SUCCESS) ? "成功" : "失败");
}

//===========================================
// 场景 3：读取文件并验证数据
//===========================================

/**
 * @brief 读取场景 2 写入的文件并与预期数据比对
 *
 * @details 流程：挂载 -> 打开文件（只读）-> 读取 -> 比对 -> 卸载
 */
static void scenario_3_read_file(void) {
  UINT status;
  UCHAR rx_buf[FILE_BUF_SIZE];
  ULONG actual_read;
  bool data_ok = true;

  printf("\r\n======== 场景 3：读取文件 ========\r\n");

  if (!open_media()) {
    return;
  }

  status = fx_file_open(&nor_disk, &file_obj, TEST_FILE_NAME, FX_OPEN_FOR_READ);
  if (!check_status("fx_file_open (read)", status)) {
    close_media();
    return;
  }

  memset(rx_buf, 0, sizeof(rx_buf));

  status = fx_file_read(&file_obj, rx_buf, sizeof(rx_buf), &actual_read);
  if (!check_status("fx_file_read", status)) {
    fx_file_close(&file_obj);
    close_media();
    return;
  }

  printf("实际读取: %lu 字节\r\n", actual_read);

  /* 验证每字节是否与写入时一致 */
  for (ULONG i = 0; i < actual_read; i++) {
    if (rx_buf[i] != (UCHAR)(i & 0xFFU)) {
      printf("数据错误：偏移 %lu，期望 0x%02X，实际 0x%02X\r\n", i,
             (unsigned)(i & 0xFFU), rx_buf[i]);
      data_ok = false;
      break;
    }
  }

  fx_file_close(&file_obj);
  close_media();
  printf("数据验证: %s\r\n", data_ok ? "通过" : "失败");
  printf("结果: %s\r\n", (status == FX_SUCCESS && data_ok) ? "成功" : "失败");
}

//===========================================
// 场景 4：遍历根目录
//===========================================

/**
 * @brief 遍历根目录并打印所有文件的名称、大小、属性
 *
 * @details 使用 fx_directory_first_full_entry_find /
 *          fx_directory_next_full_entry_find 迭代目录项
 */
static void scenario_4_list_dir(void) {
  UINT status;
  CHAR name[DIR_NAME_LEN];
  UINT attributes;
  ULONG size;
  UINT year, month, day, hour, minute, second;
  uint32_t count = 0;

  printf("\r\n======== 场景 4：目录遍历 ========\r\n");

  if (!open_media()) {
    return;
  }

  printf("%-24s %8s  %-6s\r\n", "文件名", "大小(B)", "属性");
  printf("--------------------------------------------\r\n");

  /* 获取第一个目录项 */
  status = fx_directory_first_full_entry_find(&nor_disk, name, &attributes,
                                              &size, &year, &month, &day, &hour,
                                              &minute, &second);

  while (status == FX_SUCCESS) {
    /* 跳过 "." 和 ".." 自身目录项 */
    if (name[0] != '.') {
      const char *type = (attributes & FX_DIRECTORY) ? "<DIR>" : "     ";
      printf("%-24s %8lu  %s\r\n", name, size, type);
      count++;
    }

    /* 获取下一个目录项 */
    status = fx_directory_next_full_entry_find(&nor_disk, name, &attributes,
                                               &size, &year, &month, &day,
                                               &hour, &minute, &second);
  }

  if (status != FX_NO_MORE_ENTRIES) {
    printf("遍历中断，错误码: 0x%02X\r\n", status);
  }

  printf("--------------------------------------------\r\n");
  printf("共 %u 个文件\r\n", (unsigned)count);

  close_media();
  printf("结果: 成功\r\n");
}

//===========================================
// 场景 5：删除文件并查询剩余空间
//===========================================

/**
 * @brief 删除测试文件，查询删除前后的可用空间变化
 *
 * @details 演示 fx_file_delete 和 fx_media_space_available
 */
static void scenario_5_delete_and_stat(void) {
  UINT status;
  ULONG space_before, space_after;

  printf("\r\n======== 场景 5：删除文件 & 查询空间 ========\r\n");

  if (!open_media()) {
    return;
  }

  /* 删除前查询可用空间 */
  fx_media_space_available(&nor_disk, &space_before);
  printf("删除前可用空间: %lu 字节\r\n", space_before);

  /* 删除测试文件 */
  status = fx_file_delete(&nor_disk, TEST_FILE_NAME);
  check_status("fx_file_delete", status);

  /* 删除后查询可用空间（需 flush 更新 FAT） */
  fx_media_flush(&nor_disk);
  fx_media_space_available(&nor_disk, &space_after);
  printf("删除后可用空间: %lu 字节\r\n", space_after);
  printf("释放空间:       %lu 字节\r\n", space_after - space_before);

  close_media();
  printf("结果: %s\r\n", (status == FX_SUCCESS) ? "成功" : "失败");
}

//===========================================
// FileX 任务入口
//===========================================

/**
 * @brief FileX 示例任务
 *
 * @details 根据 thread_input（即 DEFAULT_SCENARIO）运行对应场景，
 *          执行完成后任务进入无限等待。
 *
 * @param thread_input DEFAULT_SCENARIO 传入的场景编号
 */
static void task_fx_entry(ULONG thread_input) {
  shell_port_init();

  printf("\r\n========================================\r\n");
  printf("  ACM32P4 FileX + LevelX 示例\r\n");

  for (int scenario_num = 1; scenario_num < 6; scenario_num++) {
    switch ((int)scenario_num) {
    case 1:
      scenario_1_format_and_open();
      break;
    case 2:
      scenario_2_write_file();
      break;
    case 3:
      scenario_3_read_file();
      break;
    case 4:
      scenario_4_list_dir();
      break;
    case 5:
      scenario_5_delete_and_stat();
      break;
    default:
      printf("无效场景编号: %lu（有效范围 1~5）\r\n", scenario_num);
      break;
    }
  }
  printf("\r\n======== 示例结束，任务挂起 ========\r\n");
  tx_thread_suspend(&task_fx_tcb);
}
