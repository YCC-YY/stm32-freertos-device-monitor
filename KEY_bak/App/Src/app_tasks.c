#include "app_tasks.h"
#include "app_status.h"
#include "app_status_extra.h"
#include "app_config.h"
#include "sensor_task.h"
#include "comm_task.h"
#include "logger_task.h"
#include "monitor_task.h"
#include "uart_driver.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

QueueHandle_t     sensorQueue = NULL;
QueueHandle_t     logQueue    = NULL;
SemaphoreHandle_t statusMutex = NULL;
SemaphoreHandle_t i2cMutex    = NULL;

BaseType_t App_CreateKernelObjects(void)
{
    if (sensorQueue == NULL)
    {
        sensorQueue = xQueueCreate(SENSOR_QUEUE_LENGTH, sizeof(SensorData_t));
    }
    if (logQueue == NULL)
    {
        logQueue = xQueueCreate(LOG_QUEUE_LENGTH, sizeof(LogMessage_t));
    }
    if (statusMutex == NULL)
    {
        statusMutex = xSemaphoreCreateMutex();
    }
    if (i2cMutex == NULL)
    {
        i2cMutex = xSemaphoreCreateMutex();
    }

    if (sensorQueue == NULL || logQueue == NULL || statusMutex == NULL || i2cMutex == NULL)
    {
        return pdFAIL;
    }
    return pdPASS;
}

void App_Log(LogLevel_t level, const char *fmt, ...)
{
    LogMessage_t msg;
    va_list args;
    TickType_t ticks;

    if (logQueue == NULL || fmt == NULL)
    {
        return;
    }

    /* Filter INFO/DEBUG when logging disabled; WARN/ERROR always queued. */
    if (!App_Status_IsLoggingEnabled() && level <= LOG_INFO)
    {
        return;
    }

    ticks = xTaskGetTickCount();
    msg.level = level;
    msg.timestamp = (uint32_t)ticks;

    va_start(args, fmt);
    vsnprintf(msg.text, sizeof(msg.text), fmt, args);
    va_end(args);
    msg.text[sizeof(msg.text) - 1] = '\0';

    /* Never block high-priority work for long on log backpressure. */
    if (xQueueSend(logQueue, &msg, pdMS_TO_TICKS(10)) != pdTRUE)
    {
        App_Status_RecordLogDrop();
    }
}

void App_StartScheduler(void)
{
    App_Status_Init();

    if (App_CreateKernelObjects() != pdPASS)
    {
        /* Fatal: cannot continue without RTOS objects */
        for (;;)
        {
            __NOP();
        }
    }

    UART_Driver_Init();

    App_Status_SetState(SYSTEM_INIT);

    SensorTask_Start();
    CommTask_Start();
    LoggerTask_Start();
    MonitorTask_Start();

    App_Status_SetSchedulerRunning(1U);
    App_Status_SetState(SYSTEM_RUNNING);

    LOG_INFO("DeviceMonitor boot, scheduler start");

    vTaskStartScheduler();

    /* Should not reach here */
    for (;;)
    {
    }
}
