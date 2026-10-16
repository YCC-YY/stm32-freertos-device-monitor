#ifndef APP_TASKS_H
#define APP_TASKS_H

/*
 * app_tasks.h —— 全局 FreeRTOS 对象 + 日志宏
 *
 * 新手提示：
 *   - sensorQueue / logQueue / statusMutex / i2cMutex 是整个项目共享的
 *   - 任务之间传数据用队列（xQueueSend / xQueueReceive）
 *   - 保护共享资源用互斥锁（xSemaphoreTake / xSemaphoreGive）
 *   - 打日志请用 LOG_INFO(...)，不要自己 printf 到串口
 */

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "app_types.h"
#include "app_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ===== 全局内核对象（在 app_tasks.c 里创建） ===== */
extern QueueHandle_t  sensorQueue;   /* 传感器数据队列 */
extern QueueHandle_t  logQueue;      /* 日志队列 */
extern SemaphoreHandle_t statusMutex; /* 保护 AppStatus */
extern SemaphoreHandle_t i2cMutex;    /* 保护 I2C 总线 */

/* 创建队列和互斥锁（调度器启动前调用） */
BaseType_t App_CreateKernelObjects(void);

/* 格式化日志并投递到 logQueue（最多等 10ms，避免拖死采集） */
void App_Log(LogLevel_t level, const char *fmt, ...);

/* 四个日志宏：像 printf 一样用，但输出走 LoggerTask */
#define LOG_DEBUG(...) App_Log(LOG_DEBUG, __VA_ARGS__)
#define LOG_INFO(...)  App_Log(LOG_INFO,  __VA_ARGS__)
#define LOG_WARN(...)  App_Log(LOG_WARN,  __VA_ARGS__)
#define LOG_ERROR(...) App_Log(LOG_ERROR, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* APP_TASKS_H */
