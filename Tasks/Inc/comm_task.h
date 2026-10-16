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

void CommTask_Start(void);
void CommTask_Function(void *argument);
TaskHandle_t CommTask_Handle(void);

#ifdef __cplusplus
}
#endif

#endif /* COMM_TASK_H */
