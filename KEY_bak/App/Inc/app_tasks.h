#ifndef APP_TASKS_H
#define APP_TASKS_H

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "app_types.h"
#include "app_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Global FreeRTOS objects (created in App_CreateKernelObjects) */
extern QueueHandle_t  sensorQueue;
extern QueueHandle_t  logQueue;
extern SemaphoreHandle_t statusMutex;
extern SemaphoreHandle_t i2cMutex;

/* Create queues/mutexes/tasks and start the scheduler.
 * Call once from main.c after HAL + MX peripheral init. */
void App_StartScheduler(void);

/* Create only kernel objects (queues/mutexes). Useful for tests. */
BaseType_t App_CreateKernelObjects(void);

/* Structured logging helper — never blocks forever. */
void App_Log(LogLevel_t level, const char *fmt, ...);

#define LOG_DEBUG(...) App_Log(LOG_DEBUG, __VA_ARGS__)
#define LOG_INFO(...)  App_Log(LOG_INFO,  __VA_ARGS__)
#define LOG_WARN(...)  App_Log(LOG_WARN,  __VA_ARGS__)
#define LOG_ERROR(...) App_Log(LOG_ERROR, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* APP_TASKS_H */
