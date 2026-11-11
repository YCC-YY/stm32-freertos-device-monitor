#ifndef COMM_TASK_H
#define COMM_TASK_H

/*
 * comm_task.h —— 串口命令任务接口
 * 职责：收字节、组命令、执行 CLI（实现见 comm_task.c）
 */

#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 以下三个函数是 CommTask 对外的全部接口：
 *
 * CommTask_Start()         创建任务（App_StartScheduler 里调用）
 * CommTask_Function(arg)   任务函数本体：收字节 → 组行 → CLI 执行
 * CommTask_Handle()        返回任务句柄（stats 命令查栈水位用）
 */
void CommTask_Start(void);
void CommTask_Function(void *argument);
TaskHandle_t CommTask_Handle(void);

#ifdef __cplusplus
}
#endif

#endif /* COMM_TASK_H */
