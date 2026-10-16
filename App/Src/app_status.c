/*
 * app_status.c —— 共享状态的“保险柜”
 *
 * 新手提示：
 *   多任务改同一结构体时，必须先拿锁（statusMutex），改完再放锁。
 *   下面 lock/unlock 封装了这件事，其它代码请只调 App_Status_* 函数。
 *
 * 为什么需要锁？
 *   比如错误计数：两个任务同时 ++，可能丢一次。
 *   锁保证同一时刻只有一个任务在读写 g_status。
 */

#include "app_status.h"
#include "app_tasks.h"
#include "app_config.h"
#include <string.h>

/*---------- 全局变量（都在本文件里，外部只能用函数访问） ----------*/

/*
 * g_status：整份系统状态的唯一实例
 *   包含：系统状态、运行时间、各种错误计数、心跳、最新传感器数据
 */
static AppStatus_t g_status;

/*
 * s_local_mutex：备用锁
 *   正常应使用 app_tasks 创建的 statusMutex；
 *   若调度器还没建好对象，就用本地锁兜底，避免空指针。
 */
static SemaphoreHandle_t s_local_mutex;

/*---------- 内部函数 ----------*/

/* status_lock 用于获取互斥锁，优先尝试获取statusMutex，
 * 若为空则尝试获取 s_local_mutex，等待时间为 20 毫秒，目的是保证对共享资源 g_status 访问的互斥性
 * 拿锁（最多等 20ms）。
 * xSemaphoreTake：成功拿到锁返回 pdTRUE
 */
static void status_lock(void)
{
    if (statusMutex != NULL)
    {
        xSemaphoreTake(statusMutex, pdMS_TO_TICKS(20));
    }
    else if (s_local_mutex != NULL)
    {
        xSemaphoreTake(s_local_mutex, pdMS_TO_TICKS(20));
    }
}

/* 放锁，别的任务可以继续访问 */
static void status_unlock(void)
{
    if (statusMutex != NULL)
    {
        xSemaphoreGive(statusMutex);
    }
    else if (s_local_mutex != NULL)
    {
        xSemaphoreGive(s_local_mutex);
    }
}

/*---------- 对外接口 ----------*/

/*
 * App_GetTick：取当前系统 tick（1 tick = 1ms，见 FreeRTOSConfig）
 * 返回值：开机到现在大约多少毫秒（uint32 约 49 天后溢出）
 */
uint32_t App_GetTick(void)
{
    return (uint32_t)xTaskGetTickCount();
}

/*
 * 初始化：整块 g_status 清零，并设默认值
 * logging_enabled = 1：默认打开 INFO 日志
 * sensor_ready = 0：传感器还没初始化成功
 */
void App_Status_Init(void)
{
    memset(&g_status, 0, sizeof(g_status));
    g_status.system_state = SYSTEM_INIT;
    g_status.logging_enabled = 1U;
    g_status.sensor_ready = 0U;
    if (s_local_mutex == NULL)
    {
        s_local_mutex = xSemaphoreCreateMutex();
    }
}

/*
 * 设置系统大状态
 * 参数 state：SYSTEM_INIT / RUNNING / WARNING / ERROR
 */
void App_Status_SetState(SystemState_t state)
{
    status_lock();
    g_status.system_state = state;
    status_unlock();
}

/* 读出系统状态，给别的任务/CLI 用 */
SystemState_t App_Status_GetState(void)
{
    SystemState_t s;
    status_lock();
    s = g_status.system_state;
    status_unlock();
    return s;
}

/* ready：1=传感器 OK，0=失败或未初始化 */
void App_Status_SetSensorReady(uint8_t ready)
{
    status_lock();
    g_status.sensor_ready = ready;
    status_unlock();
}

/* enabled：1=打印 INFO/DEBUG，0=只保留 WARN/ERROR */
void App_Status_SetLoggingEnabled(uint8_t enabled)
{
    status_lock();
    g_status.logging_enabled = enabled;
    status_unlock();
}

uint8_t App_Status_IsLoggingEnabled(void)
{
    uint8_t e;
    status_lock();
    e = g_status.logging_enabled;
    status_unlock();
    return e;
}

/*--------- 错误计数：只做 ++，谁调用谁负责在合适的时机调用 ---------*/

void App_Status_RecordSensorError(void)
{
    status_lock();
    g_status.sensor_error_count++;   /* 读传感器失败次数 */
    status_unlock();
}

void App_Status_RecordUartError(void)
{
    status_lock();
    g_status.uart_error_count++;     /* 串口异常（溢出等）次数 */
    status_unlock();
}

void App_Status_RecordLogDrop(void)
{
    status_lock();
    g_status.log_dropped_count++;    /* 日志队列满被丢弃的条数 */
    status_unlock();
}

void App_Status_RecordSensorQueueFull(void)
{
    status_lock();
    g_status.sensor_queue_full_count++; /* 传感器队列满次数 */
    status_unlock();
}

/*--------- 心跳：每个任务把“当前毫秒时间”写进去 ---------*/

/* 同下 */
void App_Status_BeatSensor(uint32_t now_ms)
{
    status_lock();  //获取锁，防止多个任务同时访问和修改g_status相关状态数据
    g_status.heartbeat.sensor_heartbeat = now_ms;
    status_unlock();   //释放锁
}

/* 更新通信任务的 “心跳” 时间，通过加锁和解锁操作，
 * 将当前毫秒时间 now_ms 记录到 g_status.heartbeat.comm_heartbeat 中，以便监控任务运行状态 */
void App_Status_BeatComm(uint32_t now_ms)
{
    status_lock();
    g_status.heartbeat.comm_heartbeat = now_ms;
    status_unlock();
}

void App_Status_BeatLogger(uint32_t now_ms)
{
    status_lock();
    g_status.heartbeat.logger_heartbeat = now_ms;
    status_unlock();
}

void App_Status_BeatMonitor(uint32_t now_ms)
{
    status_lock();
    g_status.heartbeat.monitor_heartbeat = now_ms;
    status_unlock();
}

/*--------- 设置（全局）最新传感器数据：整包拷贝，避免读到“半份”数据 ---------*/
/* 在 data前加 const ——> 只会读取传入的数据，不会修改它 */
void App_Status_SetLatestSensor(const SensorData_t *data)
{
    /* 空指针校验
       入参防御性检查：如果传进来的是空指针，直接返回，不往下执行 */
    if (data == NULL)
    {
        return;
    }
    /* 操作全局变量前先加锁，加锁进入临界区
     * 内部加锁保护，避免多任务并发读写造成数据撕裂（）
     */
    status_lock();
    g_status.latest_sensor = *data;  /* 结构体整体赋值 = 一次拷贝 */
    status_unlock();
}

void App_Status_GetLatestSensor(SensorData_t *out)
{
    if (out == NULL)
    {
        return;
    }
    status_lock();
    *out = g_status.latest_sensor;
    status_unlock();
}

/* 拷贝整份状态，CLI 一次拿走慢慢显示 */
void App_Status_Copy(AppStatus_t *out)
{
    if (out == NULL)
    {
        return;
    }
    status_lock();
    *out = g_status;
    status_unlock();
}

/* 运行时间（毫秒），由 MonitorTask 定期更新 */
void App_Status_SetUptime(uint32_t ms)
{
    status_lock();
    g_status.uptime_ms = ms;
    status_unlock();
}

/* run：1=调度器已启动 */
void App_Status_SetSchedulerRunning(uint8_t run)
{
    status_lock();
    g_status.scheduler_running = run;
    status_unlock();
}
