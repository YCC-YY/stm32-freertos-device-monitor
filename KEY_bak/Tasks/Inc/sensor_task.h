#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

void SensorTask_Start(void);
void SensorTask_Function(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_TASK_H */
