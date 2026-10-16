#include "monitor_task.h"
#include "app_tasks.h"
#include "app_status.h"
#include "app_status_extra.h"
#include "app_config.h"
#include "bsp_led.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

static TaskHandle_t s_monitor_handle = NULL;

static int heartbeat_stale(uint32_t last_ms, uint32_t now_ms, uint32_t timeout_ms)
{
    /* handle tick wrap roughly */
    if (last_ms == 0U)
    {
        return 1;
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
    AppStatus_t st;
    (void)argument;

    BSP_LED_Init();
    LOG_INFO("MonitorTask ready");

    for (;;)
    {
        uint32_t now = App_GetTick();
        App_Status_BeatMonitor(now);
        App_Status_SetUptime(now);

        App_Status_Copy(&st);

        int unhealthy = 0;
        if (heartbeat_stale(st.heartbeat.sensor_heartbeat, now, HEARTBEAT_TIMEOUT_MS) ||
            heartbeat_stale(st.heartbeat.comm_heartbeat, now, HEARTBEAT_TIMEOUT_MS) ||
            heartbeat_stale(st.heartbeat.logger_heartbeat, now, HEARTBEAT_TIMEOUT_MS))
        {
            unhealthy = 1;
        }

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

        /* Optional IWDG refresh would go here when st.system_state != SYSTEM_ERROR */

        vTaskDelay(pdMS_TO_TICKS(MONITOR_PERIOD_MS));
    }
}
