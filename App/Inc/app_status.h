#ifndef APP_STATUS_H
#define APP_STATUS_H

/*
 * app_status.h —— 系统共享状态
 *
 * 新手提示：
 *   - 多个任务会同时读写“错误次数、最新传感器值、心跳”等
 *   - 所有读写都必须走下面这些函数，函数内部会拿 statusMutex 加锁
 *   - 千万不要自己直接改全局状态结构体
 */

#include "app_types.h"
#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 内部使用的完整状态（一般任务不要直接操作，请调用下面的 App_Status_* 函数）
 *
 * 这个结构体是整个系统的"信息中心"，所有任务共享一份（g_status），
 * 内部由 app_status.c 用 statusMutex 加锁保护。
 *
 * 字段说明：
 *   system_state            系统大状态：INIT/RUNNING/WARNING/ERROR（MonitorTask 负责切换）
 *   uptime_ms               开机运行时长，单位 ms（MonitorTask 每周期更新）
 *   sensor_error_count      传感器读失败累计次数（SensorTask 每次失败 +1；
 *                           达到 SENSOR_ERROR_WARN_LIMIT 后系统转 WARNING）
 *   uart_error_count        串口异常累计次数（如环缓溢出）
 *   log_dropped_count       日志队列满导致被丢弃的条数（App_Log 统计）
 *   sensor_queue_full_count 传感器队列满的次数（当前设计已改为"最新值缓存"，基本不用）
 *   sensor_ready            1=传感器当前可用；0=未初始化/连续失败/故障注入中
 *   logging_enabled         1=允许输出 DEBUG/INFO 日志；0=只保留 WARN/ERROR
 *   scheduler_running       1=调度器已启动（调试用）
 *   heartbeat               四个任务的心跳时间戳（见 TaskHeartbeat_t）
 *   latest_sensor           最近一帧有效传感器数据（status/sensor 命令显示它）
 */
typedef struct
{
    SystemState_t system_state;

    uint32_t uptime_ms;
    uint32_t sensor_error_count;
    uint32_t uart_error_count;
    uint32_t log_dropped_count;
    uint32_t sensor_queue_full_count;

    uint8_t sensor_ready;
    uint8_t logging_enabled;
    uint8_t scheduler_running;

    TaskHeartbeat_t heartbeat;
    SensorData_t    latest_sensor;
} AppStatus_t;

/* 初始化状态（调度器启动前调用一次） */
void App_Status_Init(void);

/* 读写系统状态 */
void App_Status_SetState(SystemState_t state);
SystemState_t App_Status_GetState(void);

/* 传感器是否就绪 / 日志开关 */
void App_Status_SetSensorReady(uint8_t ready);
void App_Status_SetLoggingEnabled(uint8_t enabled);
uint8_t App_Status_IsLoggingEnabled(void);

/* 记录各类错误或丢弃事件（内部会 ++，线程安全） */
void App_Status_RecordSensorError(void);
void App_Status_RecordUartError(void);
void App_Status_RecordLogDrop(void);
void App_Status_RecordSensorQueueFull(void);

/* 任务心跳打卡：传入当前毫秒时间 */
void App_Status_BeatSensor(uint32_t now_ms);
void App_Status_BeatComm(uint32_t now_ms);
void App_Status_BeatLogger(uint32_t now_ms);
void App_Status_BeatMonitor(uint32_t now_ms);

/* 最新一帧传感器数据的存取（拷贝，线程安全） */
void App_Status_SetLatestSensor(const SensorData_t *data);
void App_Status_GetLatestSensor(SensorData_t *out);

/* 一次拷贝出整份状态，方便 CLI 显示 */
void App_Status_Copy(AppStatus_t *out);
void App_Status_SetUptime(uint32_t ms);
void App_Status_SetSchedulerRunning(uint8_t run);

/* 故障注入（演示用）：按 FAULT_* 位或组合 */
void App_Fault_Set(uint32_t mask);
void App_Fault_Clear(uint32_t mask);
uint32_t App_Fault_Get(void);
uint8_t App_Fault_IsSet(uint32_t mask);

/* 当前系统 tick（毫秒），等价于开机后的运行时间 */
uint32_t App_GetTick(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_STATUS_H */
