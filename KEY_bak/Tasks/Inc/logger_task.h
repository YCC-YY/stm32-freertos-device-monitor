#ifndef LOGGER_TASK_H
#define LOGGER_TASK_H

#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

void LoggerTask_Start(void);
void LoggerTask_Function(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* LOGGER_TASK_H */
