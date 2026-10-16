#ifndef MONITOR_TASK_H
#define MONITOR_TASK_H

#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

void MonitorTask_Start(void);
void MonitorTask_Function(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* MONITOR_TASK_H */
