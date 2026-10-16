#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

/*
 * sensor_task.h —— 传感器任务接口
 * 职责：周期读传感器并发布数据（实现见 sensor_task.c）
 */

#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

void SensorTask_Start(void);                 /* 创建任务 */
void SensorTask_Function(void *argument);    /* 任务入口 */
TaskHandle_t SensorTask_Handle(void);        /* 取句柄（诊断用） */

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_TASK_H */
