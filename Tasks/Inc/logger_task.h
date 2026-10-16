#ifndef LOGGER_TASK_H
#define LOGGER_TASK_H

/*
 * logger_task.h —— 日志任务接口
 * 职责：从 logQueue 取消息，统一打印（实现见 logger_task.c）
 */

#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

void LoggerTask_Start(void);
void LoggerTask_Function(void *argument);
TaskHandle_t LoggerTask_Handle(void);

#ifdef __cplusplus
}
#endif

#endif /* LOGGER_TASK_H */
