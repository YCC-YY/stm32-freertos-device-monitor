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

/*
 * 以下三个函数是 SensorTask 对外的全部接口，别处只会用到它们：
 *
 * SensorTask_Start()          创建任务（在 App_StartScheduler 里调用）
 *                             → 内部调 xTaskCreate，把任务函数挂进 FreeRTOS
 * SensorTask_Function(arg)   任务函数本体，FreeRTOS 会把它当死循环跑；
 *                             arg 是创建时传的参数（本项目传 NULL，不用）
 * SensorTask_Handle()         返回本任务句柄，给别人查询用
 *                             （如 stats 命令查栈高水位）
 */
void SensorTask_Start(void);                 /* 创建任务 */
void SensorTask_Function(void *argument);    /* 任务入口 */
TaskHandle_t SensorTask_Handle(void);        /* 取句柄（诊断用） */

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_TASK_H */
