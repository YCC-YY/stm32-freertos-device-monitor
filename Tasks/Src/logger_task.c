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

/*
 * logger_task.c —— 日志输出任务（串口输出的“唯一出口”之一）
 *
 * 为什么单独做一个任务？
 *   多个任务同时往串口打印会交错成乱码。
 *   约定：别人只把消息放进 logQueue，由本任务统一格式化打印。
 *
 * 输出格式：[tick毫秒][级别] 文本
 */

static TaskHandle_t s_logger_handle = NULL;

TaskHandle_t LoggerTask_Handle(void)
{
    return s_logger_handle;
}

/* 日志级别 → 三个字母 */
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
    LogMessage_t msg;  /* 队列里的一条日志 */
    char line[96];     /* 格式化后的输出行 */
    (void)argument;

    LOG_INFO("LoggerTask ready");

    for (;;)
    {
        /* 心跳：即使很久没有日志，也要证明任务没卡死 */
        App_Status_BeatLogger(App_GetTick());

        /* 最多等 500ms；有消息就打印，没消息继续打心跳 */
        if (logQueue != NULL &&
            xQueueReceive(logQueue, &msg, pdMS_TO_TICKS(500)) == pdTRUE)
        {
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
