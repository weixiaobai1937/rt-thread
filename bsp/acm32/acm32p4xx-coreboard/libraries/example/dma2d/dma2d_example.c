/**
 * @file dma2d_example.c
 * @brief DMA2D模块功能示例程序
 *
 * @details
 * 演示8个场景：
 * 1. 寄存器到存储器（颜色填充）
 * 2. 存储器到存储器（数据复制）
 * 3. 存储器到存储器+PFC（像素格式转换）
 * 4. 存储器到存储器+PFC+混合（双图层混合）
 * 5. 手动CLUT填充与索引颜色转换
 * 6. 结构体配置方式
 * 7. 传输挂起与恢复
 * 8. 颜色格式与字节计算
 *
 * @note 需确保DMA2D时钟已使能
 */

#include "acm32p4.h"
#include "SEGGER_RTT.h"

//===========================================
// 配置选项
//===========================================

#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)
#define DEFAULT_SCENARIO 1

// 测试缓冲区定义（位于SRAM）
#define TEST_BUF_WIDTH   64U
#define TEST_BUF_HEIGHT  48U
#define TEST_BUF_SIZE    (TEST_BUF_WIDTH * TEST_BUF_HEIGHT * 4U)  // ARGB8888

//===========================================
// 全局变量
//===========================================

// 测试缓冲区
static __ALIGNED(4) uint8_t  src_buffer[TEST_BUF_SIZE];         ///< 源缓冲区
static __ALIGNED(4) uint8_t  bg_buffer[TEST_BUF_SIZE];          ///< 背景缓冲区
static __ALIGNED(4) uint8_t  dst_buffer[TEST_BUF_SIZE];         ///< 目标缓冲区
static __ALIGNED(4) uint32_t clut_data[256];                    ///< CLUT数据

static volatile bool    transfer_done = false;   ///< 传输完成标志
static volatile uint32_t irq_flags     = 0U;     ///< 中断标志记录

//===========================================
// 辅助函数
//===========================================

void print_separator(void)
{
    printf("========================================\n");
}

/**
 * @brief 中断回调函数
 */
static void dma2d_example_callback(uint32_t flags)
{
    irq_flags = flags;
    if (flags & DMA2D_FLAG_TCIF) {
        transfer_done = true;
    }
    dma2d_clear_flag(flags);
}

/**
 * @brief 初始化测试数据
 */
static void init_test_patterns(void)
{
    // 用渐变色填充源缓冲区
    for (uint32_t y = 0U; y < TEST_BUF_HEIGHT; y++) {
        for (uint32_t x = 0U; x < TEST_BUF_WIDTH; x++) {
            uint32_t idx = (y * TEST_BUF_WIDTH + x) * 4U;
            src_buffer[idx + 0U] = (uint8_t)(x * 4U);           // B
            src_buffer[idx + 1U] = (uint8_t)(y * 5U);           // G
            src_buffer[idx + 2U] = (uint8_t)((x + y) * 2U);     // R
            src_buffer[idx + 3U] = 0xFFU;                       // A
        }
    }

    // 用固定色填充背景缓冲区（半透明绿色）
    for (uint32_t i = 0U; i < TEST_BUF_SIZE; i += 4U) {
        bg_buffer[i + 0U] = 0x00U;  // B
        bg_buffer[i + 1U] = 0x80U;  // G
        bg_buffer[i + 2U] = 0x00U;  // R
        bg_buffer[i + 3U] = 0x80U;  // A（半透明）
    }

    // 清零目标缓冲区
    for (uint32_t i = 0U; i < TEST_BUF_SIZE; i++) {
        dst_buffer[i] = 0x00U;
    }
}

/**
 * @brief 初始化CLUT（灰度渐变）
 */
static void init_clut_grayscale(void)
{
    for (uint32_t i = 0U; i < 256U; i++) {
        uint8_t gray = (uint8_t)i;
        clut_data[i] = ((uint32_t)0xFFU << 24U)        // Alpha
                     | ((uint32_t)gray << 16U)          // R
                     | ((uint32_t)gray << 8U)           // G
                     |  (uint32_t)gray;                 // B
    }
}

/**
 * @brief 等待DMA2D传输完成
 */
static bool wait_transfer_done(uint32_t timeout_ms)
{
    transfer_done = false;
    while (!transfer_done) {
        if (dma2d_is_flag_set(DMA2D_FLAG_TCIF)) {
            dma2d_clear_flag(DMA2D_FLAG_TCIF);
            transfer_done = true;
            break;
        }
        if (timeout_ms == 0U) {
            return false;
        }
        delay_ms(1U);
        timeout_ms--;
    }
    return true;
}

/**
 * @brief 打印示例标题
 */
static void print_demo_header(uint8_t scenario, const char *title)
{
    printf("\n");
    print_separator();
    printf("=== 场景%d：%s ===\n\n", scenario, title);
}

/**
 * @brief 打印示例结束
 */
static void print_demo_end(void)
{
    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 打印缓冲区前若干个像素用于对比
 */
static void print_buffer_sample(const char *name, const uint8_t *buf, uint32_t count)
{
    printf("  %s 前%u像素:\n", name, (unsigned int)count);
    for (uint32_t i = 0U; i < count && i < 8U; i++) {
        uint32_t idx = i * 4U;
        printf("    [%u] A=%02X R=%02X G=%02X B=%02X\n",
               (unsigned int)i,
               buf[idx + 3U], buf[idx + 2U],
               buf[idx + 1U], buf[idx + 0U]);
    }
}

//===========================================
// 场景1：寄存器到存储器（颜色填充）
//===========================================

/**
 * @brief 场景1：用指定颜色填充目标区域
 *
 * 演示DMA2D的R2M模式，用固定颜色（红色）填充整个目标缓冲区。
 */
void scenario_1_fill_color(void)
{
    print_demo_header(1, "寄存器到存储器（颜色填充）");

    printf("步骤1：初始化目标缓冲区为0\n");
    for (uint32_t i = 0U; i < TEST_BUF_SIZE; i++) {
        dst_buffer[i] = 0x00U;
    }
    printf("  结果: dst_buffer 已清零\n");

    printf("\n步骤2：用红色(R=FF,G=00,B=00,A=FF)填充整个目标区域\n");
    printf("  区域: %u x %u 像素, 输出格式 ARGB8888\n",
           (unsigned int)TEST_BUF_WIDTH, (unsigned int)TEST_BUF_HEIGHT);

    uint32_t red_color = (0xFFU << 24U) | (0xFFU << 16U) | (0x00U << 8U) | 0x00U;
    dma2d_init_r2m((uint32_t)dst_buffer, TEST_BUF_WIDTH,
                   TEST_BUF_WIDTH, TEST_BUF_HEIGHT,
                   DMA2D_OUT_COLOR_ARGB8888, red_color);

    // 等待传输完成
    while (dma2d_is_busy()) {
        __NOP();
    }
    printf("  结果: 填充完成\n");

    printf("\n步骤3：验证填充结果\n");
    print_buffer_sample("dst_buffer", dst_buffer, 4);

    printf("\n步骤4：用半透明蓝色填充上半部分\n");
    uint32_t blue_color = (0x80U << 24U) | (0x00U << 16U) | (0x00U << 8U) | 0xFFU;
    dma2d_init_r2m((uint32_t)dst_buffer, TEST_BUF_WIDTH,
                   TEST_BUF_WIDTH, TEST_BUF_HEIGHT / 2U,
                   DMA2D_OUT_COLOR_ARGB8888, blue_color);
    while (dma2d_is_busy()) {
        __NOP();
    }
    printf("  结果: 上半部分已填充为半透明蓝色\n");
    print_buffer_sample("dst_buffer", dst_buffer, 4);

    print_demo_end();
}

//===========================================
// 场景2：存储器到存储器（数据复制）
//===========================================

/**
 * @brief 场景2：从源缓冲区复制数据到目标缓冲区
 *
 * 演示DMA2D的M2M模式，不执行像素格式转换，直接复制。
 */
void scenario_2_copy_buffer(void)
{
    print_demo_header(2, "存储器到存储器（数据复制）");

    init_test_patterns();

    printf("步骤1：检查源数据\n");
    print_buffer_sample("src_buffer", src_buffer, 4);

    printf("\n步骤2：清零目标缓冲区\n");
    for (uint32_t i = 0U; i < TEST_BUF_SIZE; i++) {
        dst_buffer[i] = 0x00U;
    }
    printf("  结果: dst_buffer 已清零\n");

    printf("\n步骤3：M2M模式复制（ARGB8888格式）\n");
    printf("  源地址: 0x%08X, 目标地址: 0x%08X\n",
           (unsigned int)(uintptr_t)src_buffer,
           (unsigned int)(uintptr_t)dst_buffer);
    printf("  区域: %u x %u\n", (unsigned int)TEST_BUF_WIDTH,
           (unsigned int)TEST_BUF_HEIGHT);

    dma2d_init_m2m((uint32_t)src_buffer, (uint32_t)dst_buffer,
                   TEST_BUF_WIDTH, TEST_BUF_HEIGHT,
                   TEST_BUF_WIDTH, TEST_BUF_WIDTH,
                   DMA2D_COLOR_ARGB8888);

    while (dma2d_is_busy()) {
        __NOP();
    }
    printf("  结果: 复制完成\n");

    printf("\n步骤4：验证复制结果\n");
    print_buffer_sample("dst_buffer", dst_buffer, 4);

    // 对比第一个像素
    bool match = true;
    for (uint32_t i = 0U; i < TEST_BUF_SIZE; i++) {
        if (dst_buffer[i] != src_buffer[i]) {
            match = false;
            printf("  不匹配位置: offset=%u\n", (unsigned int)i);
            break;
        }
    }
    printf("  数据校验: %s\n", match ? "全部一致" : "存在差异");

    print_demo_end();
}

//===========================================
// 场景3：存储器到存储器+PFC（像素格式转换）
//===========================================

/**
 * @brief 场景3：将ARGB8888源数据转换为RGB565格式输出
 *
 * 演示DMA2D的M2M+PFC模式，执行像素格式转换。
 */
void scenario_3_pfc_convert(void)
{
    print_demo_header(3, "存储器到存储器+PFC（像素格式转换）");

    init_test_patterns();

    printf("步骤1：查看源数据（ARGB8888格式，每像素4字节）\n");
    print_buffer_sample("src_buffer", src_buffer, 4);

    printf("\n步骤2：清零目标缓冲区\n");
    for (uint32_t i = 0U; i < TEST_BUF_SIZE; i++) {
        dst_buffer[i] = 0x00U;
    }

    printf("\n步骤3：PFC转换 ARGB8888 -> RGB565\n");
    printf("  源格式: ARGB8888（4字节/像素）\n");
    printf("  目标格式: RGB565（2字节/像素）\n");
    printf("  源每行偏移: %u 像素\n", (unsigned int)TEST_BUF_WIDTH);
    printf("  目标每行偏移: %u 像素\n", (unsigned int)TEST_BUF_WIDTH);

    dma2d_init_m2m_pfc((uint32_t)src_buffer, (uint32_t)dst_buffer,
                       TEST_BUF_WIDTH, TEST_BUF_HEIGHT,
                       TEST_BUF_WIDTH, TEST_BUF_WIDTH,
                       DMA2D_COLOR_ARGB8888,
                       DMA2D_OUT_COLOR_RGB565,
                       DMA2D_ALPHA_NO_MODIF, 0xFFU);

    while (dma2d_is_busy()) {
        __NOP();
    }
    printf("  结果: 转换完成\n");

    printf("\n步骤4：查看转换后的数据\n");
    printf("  目标格式 RGB565 每像素2字节:\n");
    uint16_t *dst16 = (uint16_t *)dst_buffer;
    for (uint32_t i = 0U; i < 4U; i++) {
        printf("    [%u] 0x%04X\n", (unsigned int)i, dst16[i]);
    }

    printf("\n步骤5：反向转换 RGB565 -> ARGB8888\n");
    // 先用刚转换的RGB565数据作为源
    uint16_t *final16 = (uint16_t *)src_buffer;
    dma2d_init_m2m_pfc((uint32_t)dst_buffer, (uint32_t)final16,
                       TEST_BUF_WIDTH, TEST_BUF_HEIGHT,
                       TEST_BUF_WIDTH, TEST_BUF_WIDTH,
                       DMA2D_COLOR_RGB565,
                       DMA2D_OUT_COLOR_ARGB8888,
                       DMA2D_ALPHA_NO_MODIF, 0xFFU);

    while (dma2d_is_busy()) {
        __NOP();
    }
    printf("  结果: 反向转换完成（转换存在精度损失）\n");
    print_buffer_sample("src_buffer(还原)", src_buffer, 4);

    print_demo_end();
}

//===========================================
// 场景4：存储器到存储器+PFC+混合
//===========================================

/**
 * @brief 场景4：将前景层和背景层混合输出
 *
 * 演示DMA2D的M2M_BLEND模式，两张图像Alpha混合。
 */
void scenario_4_blend_images(void)
{
    print_demo_header(4, "存储器到存储器+PFC+混合（双图层混合）");

    init_test_patterns();

    printf("步骤1：查看前景层数据（渐变图案）\n");
    print_buffer_sample("src_buffer(FG)", src_buffer, 3);

    printf("\n步骤2：查看背景层数据（半透明绿色）\n");
    print_buffer_sample("bg_buffer(BG)", bg_buffer, 3);

    printf("\n步骤3：清零目标缓冲区\n");
    for (uint32_t i = 0U; i < TEST_BUF_SIZE; i++) {
        dst_buffer[i] = 0x00U;
    }

    printf("\n步骤4：执行混合操作（FG + BG -> OUT）\n");
    printf("  混合公式: OUT = FG*FG_alpha + BG*(1-FG_alpha)\n");

    dma2d_init_m2m_blend((uint32_t)src_buffer, (uint32_t)bg_buffer,
                         (uint32_t)dst_buffer,
                         TEST_BUF_WIDTH, TEST_BUF_HEIGHT,
                         TEST_BUF_WIDTH, TEST_BUF_WIDTH, TEST_BUF_WIDTH,
                         DMA2D_COLOR_ARGB8888, DMA2D_COLOR_ARGB8888,
                         DMA2D_OUT_COLOR_ARGB8888);

    while (dma2d_is_busy()) {
        __NOP();
    }
    printf("  结果: 混合完成\n");

    printf("\n步骤5：查看混合结果\n");
    print_buffer_sample("dst_buffer(混合)", dst_buffer, 5);

    print_demo_end();
}

//===========================================
// 场景5：手动CLUT填充与索引颜色转换
//===========================================

/**
 * @brief 场景5：手动填充CLUT，用L8索引颜色进行转换
 *
 * 演示手动操作CLUT和索引颜色格式转换。
 */
void scenario_5_clut_manual(void)
{
    print_demo_header(5, "手动CLUT填充与索引颜色转换");

    printf("步骤1：初始化灰度CLUT（256个条目）\n");
    init_clut_grayscale();

    printf("步骤2：手动写入FG CLUT（前32个条目）\n");
    for (uint32_t i = 0U; i < 32U; i++) {
        dma2d_clut_write_fg((uint8_t)i, clut_data[i]);
    }
    printf("  结果: 已写入32个CLUT条目\n");

    printf("\n步骤3：验证CLUT读取\n");
    for (uint32_t i = 0U; i < 4U; i++) {
        uint32_t val = dma2d_clut_read_fg((uint8_t)i);
        printf("  CLUT[%u] = 0x%08X\n", (unsigned int)i, (unsigned int)val);
    }

    printf("\n步骤4：手动写入FG CLUT剩余条目（32-255）\n");
    for (uint32_t i = 32U; i < 256U; i++) {
        dma2d_clut_write_fg((uint8_t)i, clut_data[i]);
    }
    printf("  结果: 全部256个条目已写入\n");

    printf("\n步骤5：用L8索引数据填充源缓冲区\n");
    // L8格式：每像素1字节，作为CLUT索引
    for (uint32_t i = 0U; i < TEST_BUF_WIDTH * TEST_BUF_HEIGHT; i++) {
        src_buffer[i] = (uint8_t)(i % 256U);  // 循环使用所有CLUT条目
    }

    printf("\n步骤6：配置FG PFC为L8格式并启动转换\n");
    DMA2D->FGPFCCR_f.CM = (uint32_t)DMA2D_COLOR_L8;
    DMA2D->FGPFCCR_f.CCM = (uint32_t)DMA2D_CLUT_CM_ARGB8888;
    DMA2D->FGPFCCR_f.CS = 255U;  // 256条CLUT条目

    dma2d_init_m2m_pfc((uint32_t)src_buffer, (uint32_t)dst_buffer,
                       TEST_BUF_WIDTH, TEST_BUF_HEIGHT,
                       TEST_BUF_WIDTH, TEST_BUF_WIDTH,
                       DMA2D_COLOR_L8,
                       DMA2D_OUT_COLOR_ARGB8888,
                       DMA2D_ALPHA_NO_MODIF, 0xFFU);

    while (dma2d_is_busy()) {
        __NOP();
    }
    printf("  结果: L8索引转换完成\n");
    print_buffer_sample("dst_buffer(CLUT索引)", dst_buffer, 8);

    print_demo_end();
}

//===========================================
// 场景6：结构体配置方式
//===========================================

/**
 * @brief 场景6：使用dma2d_config_t结构体进行完整配置
 *
 * 演示结构体配置和参数校验。
 */
void scenario_6_config_struct(void)
{
    print_demo_header(6, "结构体配置方式");

    init_test_patterns();

    printf("步骤1：使用结构体配置M2M+PFC+混合模式\n");
    dma2d_config_t cfg = {
        .mode = DMA2D_MODE_M2M_BLEND,
        .fg_mar = (uint32_t)src_buffer,
        .fg_or = TEST_BUF_WIDTH,
        .fg_cm = DMA2D_COLOR_ARGB8888,
        .fg_am = DMA2D_ALPHA_NO_MODIF,
        .fg_alpha = 0xFFU,
        .fg_clut_cm = DMA2D_CLUT_CM_ARGB8888,
        .fg_clut_size = 0U,
        .fg_clut_mar = 0U,
        .fg_color = 0U,
        .bg_mar = (uint32_t)bg_buffer,
        .bg_or = TEST_BUF_WIDTH,
        .bg_cm = DMA2D_COLOR_ARGB8888,
        .bg_am = DMA2D_ALPHA_NO_MODIF,
        .bg_alpha = 0xFFU,
        .bg_clut_cm = DMA2D_CLUT_CM_ARGB8888,
        .bg_clut_size = 0U,
        .bg_clut_mar = 0U,
        .bg_color = 0U,
        .omar = (uint32_t)dst_buffer,
        .o_or = TEST_BUF_WIDTH,
        .out_cm = DMA2D_OUT_COLOR_ARGB8888,
        .out_color = 0U,
        .nlr_pl = TEST_BUF_WIDTH,
        .nlr_nl = TEST_BUF_HEIGHT,
        .watermark = TEST_BUF_HEIGHT / 2U,
        .dead_time_en = false,
        .dead_time = 0U,
        .ahb_lock = false,
        .teie = true,
        .tcie = true,
        .twie = true,
        .caeie = false,
        .ctcie = false,
        .ceie = true,
    };

    if (dma2d_config(&cfg)) {
        printf("  结果: 配置成功\n");
        printf("  模式: M2M_BLEND, 区域: %u x %u\n",
               cfg.nlr_pl, cfg.nlr_nl);
        printf("  水印行: %u, 中断: TE/TC/TW/CE\n",
               cfg.watermark);
    } else {
        printf("  结果: 配置失败\n");
        return;
    }

    printf("\n步骤2：查询当前配置\n");
    printf("  模式: %d\n", dma2d_get_mode());
    printf("  FG地址: 0x%08X, 偏移: %u\n",
           (unsigned int)dma2d_get_fg_address(),
           (unsigned int)dma2d_get_fg_offset());
    printf("  BG地址: 0x%08X, 偏移: %u\n",
           (unsigned int)dma2d_get_bg_address(),
           (unsigned int)dma2d_get_bg_offset());
    printf("  OUT地址: 0x%08X, 偏移: %u\n",
           (unsigned int)dma2d_get_out_address(),
           (unsigned int)dma2d_get_out_offset());
    printf("  每行像素: %u, 行数: %u\n",
           (unsigned int)dma2d_get_pixels_per_line(),
           (unsigned int)dma2d_get_num_lines());
    printf("  水印行: %u\n", (unsigned int)dma2d_get_watermark());

    printf("\n步骤3：启动传输并等待完成\n");
    dma2d_register_callback(dma2d_example_callback);
    NVIC_EnableIRQ(DMA2D_IRQn);

    dma2d_start();
    if (wait_transfer_done(1000U)) {
        printf("  结果: 传输完成\n");
    } else {
        printf("  结果: 传输超时\n");
    }

    NVIC_DisableIRQ(DMA2D_IRQn);
    dma2d_register_callback(NULL);

    printf("\n步骤4：参数校验测试\n");
    dma2d_config_t invalid_cfg = cfg;
    invalid_cfg.fg_cm = 15U;  // 无效颜色模式
    if (!dma2d_config(&invalid_cfg)) {
        printf("  结果: 无效颜色模式被正确拒绝\n");
    }

    invalid_cfg = cfg;
    invalid_cfg.nlr_pl = 0U;  // PL不能为0
    if (!dma2d_config(&invalid_cfg)) {
        printf("  结果: PL=0被正确拒绝\n");
    }

    print_demo_end();
}

//===========================================
// 场景7：传输挂起与恢复
//===========================================

/**
 * @brief 场景7：演示DMA2D传输的挂起和恢复
 *
 * 启动一个大区域传输，中途挂起然后恢复。
 */
void scenario_7_suspend_resume(void)
{
    print_demo_header(7, "传输挂起与恢复");

    printf("步骤1：初始化测试数据\n");
    init_test_patterns();
    for (uint32_t i = 0U; i < TEST_BUF_SIZE; i++) {
        dst_buffer[i] = 0x00U;
    }

    printf("\n步骤2：使用结构体配置M2M模式（不使用自动启动）\n");
    dma2d_config_t cfg = {
        .mode = DMA2D_MODE_M2M,
        .fg_mar = (uint32_t)src_buffer,
        .fg_or = TEST_BUF_WIDTH,
        .fg_cm = DMA2D_COLOR_ARGB8888,
        .fg_am = DMA2D_ALPHA_NO_MODIF,
        .fg_alpha = 0xFFU,
        .fg_clut_cm = DMA2D_CLUT_CM_ARGB8888,
        .fg_clut_size = 0U,
        .fg_clut_mar = 0U,
        .fg_color = 0U,
        .bg_mar = 0U,
        .bg_or = 0U,
        .bg_cm = DMA2D_COLOR_ARGB8888,
        .bg_am = DMA2D_ALPHA_NO_MODIF,
        .bg_alpha = 0xFFU,
        .bg_clut_cm = DMA2D_CLUT_CM_ARGB8888,
        .bg_clut_size = 0U,
        .bg_clut_mar = 0U,
        .bg_color = 0U,
        .omar = (uint32_t)dst_buffer,
        .o_or = TEST_BUF_WIDTH,
        .out_cm = DMA2D_OUT_COLOR_ARGB8888,
        .out_color = 0U,
        .nlr_pl = TEST_BUF_WIDTH,
        .nlr_nl = TEST_BUF_HEIGHT,
        .watermark = 0U,
        .dead_time_en = false,
        .dead_time = 0U,
        .ahb_lock = false,
        .teie = false,
        .tcie = false,
        .twie = false,
        .caeie = false,
        .ctcie = false,
        .ceie = false,
    };
    dma2d_config(&cfg);

    printf("\n步骤3：启动传输\n");
    dma2d_start();
    printf("  正在传输: %s\n", dma2d_is_busy() ? "是" : "否");

    printf("\n步骤4：立即挂起传输\n");
    dma2d_suspend();
    printf("  已挂起\n");

    // 短暂延迟确保挂起生效
    delay_ms(10);

    printf("\n步骤5：恢复传输\n");
    dma2d_resume();
    printf("  已恢复\n");

    printf("\n步骤6：等待传输完成\n");
    uint32_t timeout = 1000U;
    while (dma2d_is_busy() && --timeout) {
        __NOP();
    }
    printf("  传输完成: %s\n", timeout > 0U ? "是" : "超时");

    printf("\n步骤7：演示中止操作\n");
    // 重新配置并启动
    dma2d_config(&cfg);
    dma2d_start();
    delay_ms(1);
    printf("  传输中: %s\n", dma2d_is_busy() ? "是" : "否");
    dma2d_abort();
    // 等待中止生效
    for (volatile uint32_t i = 0U; i < 1000U; i++) {
        __NOP();
    }
    printf("  中止后传输中: %s\n", dma2d_is_busy() ? "是" : "否");
    printf("  中止后TCIF: %s\n",
           dma2d_is_flag_set(DMA2D_FLAG_TCIF) ? "置位" : "未置位（正确，中止不触发TCIF）");

    print_demo_end();
}

//===========================================
// 场景8：颜色格式与字节计算
//===========================================

/**
 * @brief 场景8：演示颜色格式的字节计算和地址对齐检查
 *
 * 使用辅助函数验证各种颜色格式的属性。
 */
void scenario_8_color_calculation(void)
{
    print_demo_header(8, "颜色格式与字节计算");

    printf("步骤1：输入颜色格式每像素字节数\n");
    printf("  ┌──────────────┬──────────┬────────┐\n");
    printf("  │ 颜色模式     │ 枚举值   │ 字节   │\n");
    printf("  ├──────────────┼──────────┼────────┤\n");

    static const struct {
        dma2d_color_mode_t cm;
        const char *name;
    } input_modes[] = {
        { DMA2D_COLOR_ARGB8888, "ARGB8888" },
        { DMA2D_COLOR_RGB888,   "RGB888  " },
        { DMA2D_COLOR_RGB565,   "RGB565  " },
        { DMA2D_COLOR_ARGB1555, "ARGB1555" },
        { DMA2D_COLOR_ARGB4444, "ARGB4444" },
        { DMA2D_COLOR_L8,       "L8      " },
        { DMA2D_COLOR_AL44,     "AL44    " },
        { DMA2D_COLOR_AL88,     "AL88    " },
        { DMA2D_COLOR_L4,       "L4      " },
        { DMA2D_COLOR_A8,       "A8      " },
        { DMA2D_COLOR_A4,       "A4      " },
    };

    for (uint32_t i = 0U; i < 11U; i++) {
        uint8_t bpp = dma2d_bytes_per_pixel(input_modes[i].cm);
        printf("  │ %-12s │ %-8d │ %-6u │\n",
               input_modes[i].name,
               (int)input_modes[i].cm,
               (unsigned int)bpp);
    }
    printf("  └──────────────┴──────────┴────────┘\n");

    printf("\n步骤2：输出颜色格式每像素字节数\n");
    printf("  ┌──────────────┬──────────┬────────┐\n");
    printf("  │ 输出模式     │ 枚举值   │ 字节   │\n");
    printf("  ├──────────────┼──────────┼────────┤\n");

    static const struct {
        dma2d_out_color_mode_t cm;
        const char *name;
    } out_modes[] = {
        { DMA2D_OUT_COLOR_ARGB8888, "ARGB8888" },
        { DMA2D_OUT_COLOR_RGB888,   "RGB888  " },
        { DMA2D_OUT_COLOR_RGB565,   "RGB565  " },
        { DMA2D_OUT_COLOR_ARGB1555, "ARGB1555" },
        { DMA2D_OUT_COLOR_ARGB4444, "ARGB4444" },
    };

    for (uint32_t i = 0U; i < 5U; i++) {
        uint8_t bpp = dma2d_out_bytes_per_pixel(out_modes[i].cm);
        printf("  │ %-12s │ %-8d │ %-6u │\n",
               out_modes[i].name,
               (int)out_modes[i].cm,
               (unsigned int)bpp);
    }
    printf("  └──────────────┴──────────┴────────┘\n");

    printf("\n步骤3：地址对齐检查\n");
    uint32_t test_addr = (uint32_t)src_buffer;
    printf("  测试地址: 0x%08X\n", (unsigned int)test_addr);
    printf("  ARGB8888对齐: %s\n",
           dma2d_check_address_alignment(test_addr, DMA2D_COLOR_ARGB8888) ? "OK" : "FAIL");
    printf("  RGB565对齐: %s\n",
           dma2d_check_address_alignment(test_addr, DMA2D_COLOR_RGB565) ? "OK" : "FAIL");
    printf("  L8对齐: %s\n",
           dma2d_check_address_alignment(test_addr, DMA2D_COLOR_L8) ? "OK" : "FAIL");

    // 测试奇数地址
    printf("  奇地址(0x%08X) RGB565对齐: %s\n",
           (unsigned int)(test_addr + 1U),
           dma2d_check_address_alignment(test_addr + 1U, DMA2D_COLOR_RGB565) ? "OK" : "FAIL（预期失败）");

    print_demo_end();
}

//===========================================
// 主函数
//===========================================

int main(void)
{
    // 系统初始化
    sys_status_t status = system_init();
    if (status != SYS_OK) {
        while (1) { __NOP(); }
    }
    SEGGER_RTT_Init();

    printf("\n");
    print_separator();
    printf("DMA2D 模块示例程序\n");
    printf("当前场景: %d\n", DEFAULT_SCENARIO);
    print_separator();
    printf("\n");

    // 使能DMA2D时钟
    clock_periph_enable(CLK_DMA2D);

    // 根据场景号运行对应示例
    switch (DEFAULT_SCENARIO) {
        case 1:
            scenario_1_fill_color();
            break;
        case 2:
            scenario_2_copy_buffer();
            break;
        case 3:
            scenario_3_pfc_convert();
            break;
        case 4:
            scenario_4_blend_images();
            break;
        case 5:
            scenario_5_clut_manual();
            break;
        case 6:
            scenario_6_config_struct();
            break;
        case 7:
            scenario_7_suspend_resume();
            break;
        case 8:
            scenario_8_color_calculation();
            break;
        default:
            printf("错误：无效的场景编号 %d\n", DEFAULT_SCENARIO);
            printf("有效值: 1-8\n");
            break;
    }

    // 主循环
    while (1) {
        __WFI();
    }
}
