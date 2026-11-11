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
        /* 故障注入 FAULT_LOGGER：停心跳，Monitor 应告警 */
        if (!App_Fault_IsSet(FAULT_LOGGER))
        {
            App_Status_BeatLogger(App_GetTick());
        }

        /* ---- 从日志队列取一条消息（阻塞等待） ----
         * xQueueReceive(队列, 取出的数据放哪, 等待时间)：
         *   - 队列有消息 → 立刻取走，返回 pdTRUE，msg 被填好
         *   - 队列为空   → 挂起等待；pdMS_TO_TICKS(500) 是最多等 500ms，
         *                 500ms 一到无论有没有消息都返回（超时返回 pdFALSE）
         * 为什么用"等 500ms"而不是"永久等"？
         *   → 因为任务还要抽空更新心跳（上面那段 BeatLogger），
         *     如果永久阻塞，监控任务就以为它卡死了。
         */
        if (logQueue != NULL &&
            xQueueReceive(logQueue, &msg, pdMS_TO_TICKS(500)) == pdTRUE)
        {
            /* 把结构化日志拼成一行文本：
             * [时间戳毫秒][级别缩写] 内容\r\n
             * 例：[     136127][WRN] Sensor read error\r\n
             * %10lu：宽度至少 10 的十进制数，右对齐（让日志时间列整齐）
             * %s   ：字符串（级别标签 / 日志内容）
             */
            snprintf(line,
                     sizeof(line),
                     "[%10lu][%s] %s\r\n",
                     (unsigned long)msg.timestamp,
                     level_tag(msg.level),
                     msg.text);

            /* 唯一出口：整行一次性发出去，避免多个任务交错打印乱码 */
            UART_Driver_SendString(line);
        }
    }
}
