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

/*
 * 以下三个函数是 MonitorTask 对外的全部接口：
 *
 * MonitorTask_Start()         创建任务（App_StartScheduler 里调用）
 * MonitorTask_Function(arg)   任务函数本体：查心跳/错误 → 切状态 → 喂狗/翻 LED
 * MonitorTask_Handle()        返回任务句柄（stats 命令查栈水位用）
 */
void MonitorTask_Start(void);
void MonitorTask_Function(void *argument);
TaskHandle_t MonitorTask_Handle(void);

#ifdef __cplusplus
}
#endif

#endif /* MONITOR_TASK_H */
