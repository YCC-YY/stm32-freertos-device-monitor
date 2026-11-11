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

/*
 * 以下三个函数是 LoggerTask 对外的全部接口：
 *
 * LoggerTask_Start()         创建任务（App_StartScheduler 里调用）
 * LoggerTask_Function(arg)   任务函数本体：阻塞等队列 → 格式化 → 串口输出
 * LoggerTask_Handle()        返回任务句柄（stats 命令查栈水位用）
 */
void LoggerTask_Start(void);
void LoggerTask_Function(void *argument);
TaskHandle_t LoggerTask_Handle(void);

#ifdef __cplusplus
}
#endif

#endif /* LOGGER_TASK_H */
