#include "app_status.h"
#include "app_tasks.h"
#include "app_config.h"
#include <string.h>

static AppStatus_t g_status;
static SemaphoreHandle_t s_local_mutex; /* fallback if statusMutex not yet created */

static void status_lock(void)
{
    if (statusMutex != NULL)
    {
        xSemaphoreTake(statusMutex, pdMS_TO_TICKS(20));
    }
    else if (s_local_mutex != NULL)
    {
        xSemaphoreTake(s_local_mutex, pdMS_TO_TICKS(20));
    }
}

static void status_unlock(void)
{
    if (statusMutex != NULL)
    {
        xSemaphoreGive(statusMutex);
    }
    else if (s_local_mutex != NULL)
    {
        xSemaphoreGive(s_local_mutex);
    }
}

uint32_t App_GetTick(void)
{
    return (uint32_t)xTaskGetTickCount();
}

void App_Status_Init(void)
{
    memset(&g_status, 0, sizeof(g_status));
    g_status.system_state = SYSTEM_INIT;
    g_status.logging_enabled = 1U;
    g_status.sensor_ready = 0U;
    if (s_local_mutex == NULL)
    {
        s_local_mutex = xSemaphoreCreateMutex();
    }
}

void App_Status_SetState(SystemState_t state)
{
    status_lock();
    g_status.system_state = state;
    status_unlock();
}

SystemState_t App_Status_GetState(void)
{
    SystemState_t s;
    status_lock();
    s = g_status.system_state;
    status_unlock();
    return s;
}

void App_Status_SetSensorReady(uint8_t ready)
{
    status_lock();
    g_status.sensor_ready = ready;
    status_unlock();
}

void App_Status_SetLoggingEnabled(uint8_t enabled)
{
    status_lock();
    g_status.logging_enabled = enabled;
    status_unlock();
}

uint8_t App_Status_IsLoggingEnabled(void)
{
    uint8_t e;
    status_lock();
    e = g_status.logging_enabled;
    status_unlock();
    return e;
}

void App_Status_RecordSensorError(void)
{
    status_lock();
    g_status.sensor_error_count++;
    status_unlock();
}

void App_Status_RecordUartError(void)
{
    status_lock();
    g_status.uart_error_count++;
    status_unlock();
}

void App_Status_RecordLogDrop(void)
{
    status_lock();
    g_status.log_dropped_count++;
    status_unlock();
}

void App_Status_RecordSensorQueueFull(void)
{
    status_lock();
    g_status.sensor_queue_full_count++;
    status_unlock();
}

void App_Status_BeatSensor(uint32_t now_ms)
{
    status_lock();
    g_status.heartbeat.sensor_heartbeat = now_ms;
    status_unlock();
}

void App_Status_BeatComm(uint32_t now_ms)
{
    status_lock();
    g_status.heartbeat.comm_heartbeat = now_ms;
    status_unlock();
}

void App_Status_BeatLogger(uint32_t now_ms)
{
    status_lock();
    g_status.heartbeat.logger_heartbeat = now_ms;
    status_unlock();
}

void App_Status_BeatMonitor(uint32_t now_ms)
{
    status_lock();
    g_status.heartbeat.monitor_heartbeat = now_ms;
    status_unlock();
}

void App_Status_SetLatestSensor(const SensorData_t *data)
{
    if (data == NULL)
    {
        return;
    }
    status_lock();
    g_status.latest_sensor = *data;
    status_unlock();
}

void App_Status_GetLatestSensor(SensorData_t *out)
{
    if (out == NULL)
    {
        return;
    }
    status_lock();
    *out = g_status.latest_sensor;
    status_unlock();
}

void App_Status_Copy(AppStatus_t *out)
{
    if (out == NULL)
    {
        return;
    }
    status_lock();
    *out = g_status;
    status_unlock();
}

void App_Status_SetUptime(uint32_t ms)
{
    status_lock();
    g_status.uptime_ms = ms;
    status_unlock();
}

void App_Status_SetSchedulerRunning(uint8_t run)
{
    status_lock();
    g_status.scheduler_running = run;
    status_unlock();
}
