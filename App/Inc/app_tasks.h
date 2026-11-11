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

/* ===== 全局内核对象（在 app_tasks.c 里创建） =====
 * 传感器数据不走队列：各任务用 App_Status_GetLatestSensor() 读最新值（Mutex）。
 * 需要历史/OLED/BLE 广播时再加消息机制，避免“有队列无消费者”。
 */
extern QueueHandle_t  logQueue;      /* 日志队列 */
extern SemaphoreHandle_t statusMutex; /* 保护 AppStatus */
extern SemaphoreHandle_t i2cMutex;    /* 保护 I2C 总线 */

/* 创建队列和互斥锁（调度器启动前调用） */
BaseType_t App_CreateKernelObjects(void);

/*
 * App_Log：格式化一条日志并投递到 logQueue（真正的打印由 LoggerTask 做）
 * 参数 level：日志级别（LOG_INFO / LOG_WARN ...）
 * 参数 fmt  ：格式化字符串，和 printf 一样，如 "temp=%d"
 * 参数 ...  ：可变参数，个数由 fmt 里的占位符决定
 * 返回：无。失败（队列满 10ms）只累加丢弃计数，不报错
 *
 * 为什么不用 printf？printf 直接写串口会阻塞，且多任务同时打印会交错。
 */
void App_Log(LogLevel_t level, const char *fmt, ...);

/*
 * 四个日志宏：像 printf 一样用，但输出走 LoggerTask
 * __VA_ARGS__ 的意思是"把括号里所有参数原样传给 App_Log"，
 * 这样 LOG_INFO("temp=%d", t) 就等价于 App_Log(LOG_INFO, "temp=%d", t)。
 * 好处：调用处不用手写级别参数，也不会忘记级别。
 */
#define LOG_DEBUG(...)    App_Log(LOG_DEBUG, __VA_ARGS__)
#define LOG_INFO(...)     App_Log(LOG_INFO,  __VA_ARGS__)
#define LOG_WARN(...)     App_Log(LOG_WARN,  __VA_ARGS__)
#define LOG_ERROR(...)    App_Log(LOG_ERROR, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* APP_TASKS_H */
