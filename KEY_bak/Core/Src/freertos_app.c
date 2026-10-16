/**
 * 示例：CubeMX freertos.c / main.c 中的最小调用桩
 * 将对应内容拷入生成代码的 USER CODE 区。
 */

#include "freertos_app.h"
#include "app_tasks.h"
#include "uart_driver.h"

/* 假设 CubeMX 生成了 huart1 */
extern UART_HandleTypeDef huart1;

void freertos_app_boot(void)
{
    UART_Driver_BindHandle(&huart1);
    App_StartScheduler(); /* 不返回 */
}
