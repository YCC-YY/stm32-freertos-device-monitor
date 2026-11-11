/**
 * ============================================================
 * 【分层导读】Core 层 —— freertos_app.c（CubeMX 集成桩）
 *
 * 作用：给 CubeMX 生成的 freertos.c / main.c 提供一个"最小调用桩"，
 *      演示如何把自写的 App 层启动函数接到生成代码里。
 * 实际工程中 main() 已直接调用 App_StartScheduler()（见 main.c USER CODE 2），
 *      所以本文件当前只是示例/备选方案，未参与实际运行。
 * ============================================================
 */

#include "freertos_app.h"
#include "app_scheduler.h"
#include "app_tasks.h"
#include "uart_driver.h"

/* 假设 CubeMX 生成了 huart1 */
extern UART_HandleTypeDef huart2;

void freertos_app_boot(void)
{
    UART_Driver_BindHandle(&huart2);
    App_StartScheduler(); /* 不返回 */
}
