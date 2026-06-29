#include "acm32p4.h"
#include "tx_api.h"

#include "board_st7789.h"
#include "adapter_guix_display.h"
#include "adapter_shell_port.h"

#include "acm_tool_resources.h"
#include "acm_tool_specifications.h"
#include "gx_api.h"
#include "shell.h"

#define APP_GUIX_INIT_PRIO            10U
#define APP_GUIX_INIT_STACK_SIZE      4096U
#define APP_LCD_FLUSH_PRIO            28U
#define APP_LCD_FLUSH_STACK_SIZE      2048U
#define APP_SHELL_PRIO                6U
#define APP_SHELL_STACK_SIZE          2048U

static TX_THREAD s_gui_init_thread;
static ULONG s_gui_init_stack[APP_GUIX_INIT_STACK_SIZE / sizeof(ULONG)];
static TX_THREAD s_lcd_flush_thread;
static ULONG s_lcd_flush_stack[APP_LCD_FLUSH_STACK_SIZE / sizeof(ULONG)];
static TX_THREAD s_shell_thread;
static ULONG s_shell_stack[APP_SHELL_STACK_SIZE / sizeof(ULONG)];

extern GX_STUDIO_DISPLAY_INFO acm_tool_display_table[1];

static char s_startup_screen_name[] = "window";

static void app_shell_thread_entry(ULONG thread_input)
{
	(void)thread_input;
	shell_port_init();
	shellTask((void *)&shell);
}

static void app_guix_init_thread_entry(ULONG thread_input)
{
	GX_WIDGET *startup_screen = GX_NULL;
	GX_WINDOW_ROOT *root = GX_NULL;
	UINT status;

	(void)thread_input;

	bsp_st7789_init();
	if (!bsp_st7789_is_ready()) {
		shellPrint(&shell, "[guix] st7789 init failed\r\n");
		return;
	}
	shellPrint(&shell, "[guix] st7789 init ok\r\n");

	gx_system_initialize();

	status = gx_studio_display_configure(DISPLAY_1,
										 st7789_guix_display_driver_setup,
										 LANGUAGE_ENGLISH,
										 DISPLAY_1_THEME_1,
										 &root);
	if (status != GX_SUCCESS || root == GX_NULL) {
		shellPrint(&shell, "[guix] display configure failed, status=%u\r\n", status);
		return;
	}
	shellPrint(&shell, "[guix] display configure ok\r\n");

	st7789_guix_display_bind_canvas(acm_tool_display_table[DISPLAY_1].canvas_memory,
									acm_tool_display_table[DISPLAY_1].canvas_memory_size,
									(USHORT)acm_tool_display_table[DISPLAY_1].x_resolution,
									(USHORT)acm_tool_display_table[DISPLAY_1].y_resolution);

	status = gx_studio_named_widget_create(s_startup_screen_name,
										   (GX_WIDGET *)root,
										   &startup_screen);
	if (status != GX_SUCCESS || startup_screen == GX_NULL) {
		shellPrint(&shell, "[guix] widget create failed, status=%u\r\n", status);
		return;
	}
	shellPrint(&shell, "[guix] widget create ok, starting system\r\n");

	gx_widget_show((GX_WIDGET *)root);

	{
		GX_CANVAS *canvas = acm_tool_display_table[DISPLAY_1].canvas;
		shellPrint(&shell, "[guix] before start: dirty_count=%u draw_count=%u\r\n",
				   canvas->gx_canvas_dirty_count, canvas->gx_canvas_draw_count);
	}

	status = gx_system_start();
	if (status != GX_SUCCESS) {
		shellPrint(&shell, "[guix] gx_system_start failed, status=%u\r\n", status);
		return;
	}
	shellPrint(&shell, "[guix] gx_system_start ok\r\n");

	/* 主动触发一次刷新，用于判断 GUIX 内部状态是否正常 */
	status = gx_system_canvas_refresh();
	shellPrint(&shell, "[guix] manual canvas_refresh result=%u\r\n", status);
}

static UINT app_guix_create_threads(void)
{
	UINT status;

	status = st7789_guix_display_runtime_init();
	if (status != TX_SUCCESS) {
		return status;
	}

	status = tx_thread_create(&s_shell_thread,
							  "shell",
							  app_shell_thread_entry,
							  0,
							  s_shell_stack,
							  APP_SHELL_STACK_SIZE,
							  APP_SHELL_PRIO,
							  APP_SHELL_PRIO,
							  TX_NO_TIME_SLICE,
							  TX_AUTO_START);
	if (status != TX_SUCCESS) {
		return status;
	}

	status = tx_thread_create(&s_gui_init_thread,
							  "gui_init",
							  app_guix_init_thread_entry,
							  0,
							  s_gui_init_stack,
							  APP_GUIX_INIT_STACK_SIZE,
							  APP_GUIX_INIT_PRIO,
							  APP_GUIX_INIT_PRIO,
							  TX_NO_TIME_SLICE,
							  TX_AUTO_START);
	if (status != TX_SUCCESS) {
		return status;
	}

	status = tx_thread_create(&s_lcd_flush_thread,
							  "lcd_flush",
							  st7789_guix_display_flush_thread_entry,
							  0,
							  s_lcd_flush_stack,
							  APP_LCD_FLUSH_STACK_SIZE,
							  APP_LCD_FLUSH_PRIO,
							  APP_LCD_FLUSH_PRIO,
							  TX_NO_TIME_SLICE,
							  TX_AUTO_START);

	return status;
}

int main(void)
{
	system_init();
	tx_kernel_enter();

	while (1) {
	}
}

void tx_application_define(void *first_unused_memory)
{
	(void)first_unused_memory;
	(void)app_guix_create_threads();
}
