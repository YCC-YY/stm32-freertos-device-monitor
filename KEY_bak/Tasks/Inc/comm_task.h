#ifndef COMM_TASK_H
#define COMM_TASK_H

#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

void CommTask_Start(void);
void CommTask_Function(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* COMM_TASK_H */
