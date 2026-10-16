#include "monitor_task.h"
#include "app_tasks.h"
#include "app_status.h"
#include "app_status_extra.h"
#include "app_config.h"
#include "bsp_led.h"
#include "iwdg.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/*
 * monitor_task.c —— 系统健康监控
 *
 * 职责：
 *   1. 检查各任务心跳是否超时（卡死/饿死）
 *   2. 根据错误次数调整 SystemState（RUNNING / WARNING）
 *   3. 更新运行时间
 *   4. 翻转 LED（肉眼可见“活着”）
 *   5. 喂独立看门狗 IWDG（不喂会复位）
 *
 * 注意：IWDG 约 4 秒超时，本任务 2 秒跑一次，正常不会复位。
 */

static TaskHandle_t s_monitor_handle = NULL;

TaskHandle_t MonitorTask_Handle(void)
{
    return s_monitor_handle;
}

/* 心跳是否“过期”：超过 timeout 毫秒没更新 */
static int heartbeat_stale(uint32_t last_ms, uint32_t now_ms, uint32_t timeout_ms)
{
    if (last_ms == 0U)
    {
        return 1; /* 还没打过卡 */
    }
    return ((now_ms - last_ms) > timeout_ms) ? 1 : 0;
}

void MonitorTask_Start(void)
{
    if (xTaskCreate(MonitorTask_Function,
                    "MonitorTask",
                    STACK_MONITOR,
                    NULL,
                    PRIORITY_MONITOR,
                    &s_monitor_handle) != pdPASS)
    {
        for (;;)
        {
        }
    }
}

void MonitorTask_Function(void *argument)
{
    AppStatus_t st;  /* 状态快照 */
    (void)argument;

    BSP_LED_Init();
    LOG_INFO("MonitorTask ready");

    for (;;)
    {
        uint32_t now = App_GetTick();
        int unhealthy = 0;

        App_Status_BeatMonitor(now);
        App_Status_SetUptime(now);

        App_Status_Copy(&st);

        /* 任一任务心跳超时 → 不健康 */
        if (heartbeat_stale(st.heartbeat.sensor_heartbeat, now, HEARTBEAT_TIMEOUT_MS) ||
            heartbeat_stale(st.heartbeat.comm_heartbeat, now, HEARTBEAT_TIMEOUT_MS) ||
            heartbeat_stale(st.heartbeat.logger_heartbeat, now, HEARTBEAT_TIMEOUT_MS))
        {
            unhealthy = 1;
        }

        /* 传感器错误太多 → 不健康 */
        if (st.sensor_error_count >= SENSOR_ERROR_WARN_LIMIT)
        {
            unhealthy = 1;
        }

        if (unhealthy)
        {
            App_Status_SetState(SYSTEM_WARNING);
        }
        else if (st.system_state == SYSTEM_WARNING || st.system_state == SYSTEM_INIT)
        {
            App_Status_SetState(SYSTEM_RUNNING);
        }

        BSP_LED_Toggle();

        /* 看门狗：系统不是 ERROR 时才喂狗 */
        if (st.system_state != SYSTEM_ERROR)
        {
            HAL_IWDG_Refresh(&hiwdg);
        }

        vTaskDelay(pdMS_TO_TICKS(MONITOR_PERIOD_MS));
    }
}
