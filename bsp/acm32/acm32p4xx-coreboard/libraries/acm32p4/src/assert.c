/**
 * @file assert.c
 * @brief ACM32P4 断言处理实现
 * 
 * 本文件实现了ARM EABI标准的断言处理函数，提供了以下功能：
 * - 断言失败处理
 * - 调试断点触发
 * - 死循环保护
 * 
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#include <stdio.h>

//===========================================
// 断言处理函数
//===========================================

/**
 * @brief ARM EABI 断言处理函数
 * 
 * 当assert()失败时，编译器会自动调用此函数。
 * 函数会触发断点并进入死循环，便于调试定位问题。
 * 
 * @param[in] expr 断言表达式字符串
 * @param[in] file 源文件名
 * @param[in] line 行号
 * 
 * @note 符合ARM EABI标准
 * @note 此函数不会返回
 * @warning 在发布版本中，建议禁用assert或实现自定义处理
 * 
 * 使用示例：
 * @code
 * // 在代码中使用断言
 * assert(ptr != NULL);
 * 
 * // 断言失败时会自动调用__aeabi_assert()
 * // 触发BKPT断点，调试器会在此处暂停
 * @endcode
 */
void __aeabi_assert(const char *expr, const char *file, int line)
{
    // 避免未使用变量警告
    (void)expr;
    (void)file;
    (void)line;
    
    // 触发断点指令，调试器会在此处暂停
    // 在调试器中可以查看调用堆栈，定位断言失败的位置
    __asm volatile("bkpt #0");
    
    // 进入死循环，防止程序继续执行
    // 便于调试时观察系统状态
    while (1) {
        __asm volatile("nop");
    }
}
