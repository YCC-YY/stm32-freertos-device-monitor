#ifndef MONITOR_TASK_H
#define MONITOR_TASK_H

/*
 * monitor_task.h —— 监控任务接口
 * 职责：心跳/错误检查、IWDG、LED（实现见 monitor_task.c）
 */

#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

void MonitorTask_Start(void);
void MonitorTask_Function(void *argument);
TaskHandle_t MonitorTask_Handle(void);

#ifdef __cplusplus
}
#endif

#endif /* MONITOR_TASK_H */
