#ifndef APP_STATUS_H
#define APP_STATUS_H

#include "app_types.h"
#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    SystemState_t system_state;

    uint32_t uptime_ms;
    uint32_t sensor_error_count;
    uint32_t uart_error_count;
    uint32_t log_dropped_count;
    uint32_t sensor_queue_full_count;

    uint8_t sensor_ready;
    uint8_t logging_enabled;
    uint8_t scheduler_running;

    TaskHeartbeat_t heartbeat;
    SensorData_t    latest_sensor;
} AppStatus_t;

void App_Status_Init(void);

void App_Status_SetState(SystemState_t state);
SystemState_t App_Status_GetState(void);

void App_Status_SetSensorReady(uint8_t ready);
void App_Status_SetLoggingEnabled(uint8_t enabled);
uint8_t App_Status_IsLoggingEnabled(void);

void App_Status_RecordSensorError(void);
void App_Status_RecordUartError(void);
void App_Status_RecordLogDrop(void);
void App_Status_RecordSensorQueueFull(void);

void App_Status_BeatSensor(uint32_t now_ms);
void App_Status_BeatComm(uint32_t now_ms);
void App_Status_BeatLogger(uint32_t now_ms);
void App_Status_BeatMonitor(uint32_t now_ms);

void App_Status_SetLatestSensor(const SensorData_t *data);
void App_Status_GetLatestSensor(SensorData_t *out);

void App_Status_Copy(AppStatus_t *out);
void App_Status_SetUptime(uint32_t ms);
void App_Status_SetSchedulerRunning(uint8_t run);
uint32_t App_GetTick(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_STATUS_H */
