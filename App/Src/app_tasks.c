#include "app_tasks.h"
#include "app_status.h"
#include "app_status_extra.h"
#include "app_config.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/*
 * app_tasks.c —— 创建队列/互斥锁，并实现 LOG_* 宏的底层
 *
 * 新手提示：
 *   - Queue 像“传送带”：一个任务放进去，另一个任务取走
 *   - Mutex 像“厕所钥匙”：同一时间只有一个任务能用共享资源
 */

/* ===== 全局对象定义（头文件里是 extern 声明） ===== */
QueueHandle_t     logQueue    = NULL;
SemaphoreHandle_t statusMutex = NULL;
SemaphoreHandle_t i2cMutex    = NULL;

/*
 * 创建所有内核对象。
 * 每个对象创建失败都返回 pdFAIL（通常是堆内存不够）。
 */
BaseType_t App_CreateKernelObjects(void)
{
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

    if (logQueue == NULL || statusMutex == NULL || i2cMutex == NULL)
    {
        return pdFAIL;
    }
    return pdPASS;
}

/*
 * 结构化日志：把 level + 时间 + 文本打包后放进 logQueue。
 * 真正打印由 LoggerTask 完成（保证串口输出不被多个任务打断）。
 *
 * 注意：
 *   - 日志关闭时，DEBUG/INFO 不再入队，WARN/ERROR 仍会保留
 *   - 队列满最多等 10ms，等不到就丢弃并计数，不能卡住采集任务
 */
void App_Log(LogLevel_t level, const char *fmt, ...)
{
    LogMessage_t msg;
    va_list args;
    TickType_t ticks;

    if (logQueue == NULL || fmt == NULL)
    {
        return;
    }

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

    if (xQueueSend(logQueue, &msg, pdMS_TO_TICKS(10)) != pdTRUE)
    {
        App_Status_RecordLogDrop();
    }
}

/* App_StartScheduler() 在 app_scheduler.c 中实现 */
