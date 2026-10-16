#include "logger_task.h"
#include "app_tasks.h"
#include "app_status.h"
#include "app_config.h"
#include "uart_driver.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <stdio.h>
#include <string.h>

static TaskHandle_t s_logger_handle = NULL;

static const char *level_tag(LogLevel_t level)
{
    switch (level)
    {
    case LOG_DEBUG: return "DBG";
    case LOG_INFO:  return "INF";
    case LOG_WARN:  return "WRN";
    case LOG_ERROR: return "ERR";
    default:        return "???";
    }
}

void LoggerTask_Start(void)
{
    if (xTaskCreate(LoggerTask_Function,
                    "LoggerTask",
                    STACK_LOGGER,
                    NULL,
                    PRIORITY_LOGGER,
                    &s_logger_handle) != pdPASS)
    {
        for (;;)
        {
        }
    }
}

void LoggerTask_Function(void *argument)
{
    LogMessage_t msg;
    char line[96];
    (void)argument;

    LOG_INFO("LoggerTask ready");

    for (;;)
    {
        if (logQueue != NULL &&
            xQueueReceive(logQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            App_Status_BeatLogger(App_GetTick());

            snprintf(line,
                     sizeof(line),
                     "[%10lu][%s] %s\r\n",
                     (unsigned long)msg.timestamp,
                     level_tag(msg.level),
                     msg.text);

            UART_Driver_SendString(line);
        }
    }
}
