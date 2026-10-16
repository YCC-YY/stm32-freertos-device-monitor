#include "app_scheduler.h"
#include "app_tasks.h"
#include "app_status.h"
#include "app_status_extra.h"
#include "app_config.h"
#include "uart_driver.h"
#include "sensor_task.h"
#include "comm_task.h"
#include "logger_task.h"
#include "monitor_task.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/*
 * app_scheduler.c —— 整个程序的“点火开关”
 *
 * 启动顺序（新手记住这四步）：
 *   1. 初始化共享状态
 *   2. 创建队列、互斥锁
 *   3. 初始化串口驱动（环缓）
 *   4. 创建四个任务，然后 vTaskStartScheduler() 永不返回
 *
 * main.c 在外设初始化完成后只调用一次 App_StartScheduler()。
 */
void App_StartScheduler(void)
{
    /* 1. 状态结构清零 */
    App_Status_Init();

    /* 2. 内核对象：队列 + 互斥锁 */
    if (App_CreateKernelObjects() != pdPASS)
    {
        /* 失败：多半是 FreeRTOS 堆太小，卡在这里方便调试 */
        for (;;)
        {
            __NOP();
        }
    }

    /* 3. 串口接收环缓 */
    UART_Driver_Init();
    App_Status_SetState(SYSTEM_INIT);

    /* 4. 创建四个任务（优先级、栈大小见 app_config.h） */
    SensorTask_Start();
    CommTask_Start();
    LoggerTask_Start();
    MonitorTask_Start();

    App_Status_SetSchedulerRunning(1U);
    App_Status_SetState(SYSTEM_RUNNING);

    LOG_INFO("DeviceMonitor boot, scheduler start");

    /* 5. 启动调度器；成功后此函数不再返回 */
    vTaskStartScheduler();

    /* 能执行到这里 = 调度器启动失败（常见原因：堆不够） */
    configASSERT(0);
    for (;;)
    {
    }
}
