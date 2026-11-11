#include "app_tasks.h"
#include "app_status.h"
#include "app_status_extra.h"
#include "app_config.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/*
 * app_tasks.c —— 创建队列/互斥锁，并实现 LOG_* 宏的底层
 *
 * 新手提示：
 *   - Queue 像“传送带”：一个任务放进去，另一个任务取走
 *   - Mutex 像“厕所钥匙”：同一时间只有一个任务能用共享资源
 */

/* ===== 全局对象定义（头文件里是 extern 声明） ===== */
QueueHandle_t     logQueue    = NULL;
SemaphoreHandle_t statusMutex = NULL;
SemaphoreHandle_t i2cMutex    = NULL;

/*
 * 创建所有内核对象（队列 + 互斥锁）。
 * 在 App_StartScheduler() 里、创建任务之前调用一次。
 *
 * 每个对象的作用：
 *   logQueue     日志队列：App_Log 往里放，LoggerTask 往外取
 *   statusMutex  保护全局状态 g_status（读写 AppStatus 都要先拿它）
 *   i2cMutex     保护 I2C2 总线（防止两个任务同时访问 BME280）
 *
 * 返回值：pdPASS=全部创建成功；pdFAIL=有对象创建失败（几乎都是堆内存不够）
 *
 * 为什么要先判断 NULL 再创建？
 *   防止重复创建：如果已经存在就跳过，避免句柄被覆盖、旧对象泄漏。
 */
BaseType_t App_CreateKernelObjects(void)
{
    /* xQueueCreate(队列深度, 每个元素的字节数)
     * → 深度 16、元素是 LogMessage_t 结构体（16 条日志的"传送带"）
     * sizeof(...) 编译期就能算出元素大小，FreeRTOS 会按这个大小分配内存
     */
    if (logQueue == NULL)
    {
        logQueue = xQueueCreate(LOG_QUEUE_LENGTH, sizeof(LogMessage_t));
    }
    /* xSemaphoreCreateMutex()：创建"互斥量"（带优先级继承的锁）
     * 拿到锁的任务才能访问被保护的资源，用完必须 give 还回去
     */
    if (statusMutex == NULL)
    {
        statusMutex = xSemaphoreCreateMutex();
    }
    if (i2cMutex == NULL)
    {
        i2cMutex = xSemaphoreCreateMutex();
    }

    /* 任一对象创建失败就返回 pdFAIL，让上层知道"资源不够，别继续了" */
    if (logQueue == NULL || statusMutex == NULL || i2cMutex == NULL)
    {
        return pdFAIL;
    }
    return pdPASS;
}

/*
 * 结构化日志：把 level + 时间 + 文本打包后放进 logQueue。
 * 真正打印由 LoggerTask 完成（保证串口输出不被多个任务打断）。
 *
 * 注意：
 *   - 日志关闭时，DEBUG/INFO 不再入队，WARN/ERROR 仍会保留
 *   - 队列满最多等 10ms，等不到就丢弃并计数，不能卡住采集任务
 */
void App_Log(LogLevel_t level, const char *fmt, ...)
{
    LogMessage_t msg;
    va_list args;
    TickType_t ticks;

    if (logQueue == NULL || fmt == NULL)
    {
        return;
    }

    if (!App_Status_IsLoggingEnabled() && level <= LOG_INFO)
    {
        return;
    }

    ticks = xTaskGetTickCount();
    msg.level = level;
    msg.timestamp = (uint32_t)ticks;

    /* ---- 把可变参数列表格式化成一个字符串，存进 msg.text ----
     * va_list：C 语言处理"参数个数不固定"的机制（printf 就是这种）
     * va_start(args, fmt)：从 fmt 之后开始取可变参数
     * vsnprintf：像 snprintf 一样格式化，但参数来源是 va_list；
     *            结果写入 msg.text，最多写 sizeof-1 个字符（自动加 \0）
     * va_end(args)：用完必须收尾
     * 例：App_Log(LOG_INFO, "temp=%d", 25) → msg.text = "temp=25"
     */
    va_start(args, fmt);
    vsnprintf(msg.text, sizeof(msg.text), fmt, args);
    va_end(args);
    msg.text[sizeof(msg.text) - 1] = '\0';   /* 保险：确保最后一个字符是 \0 */

    /* ---- 把整条日志放进队列 ----
     * xQueueSend(队列, 要放的数据, 等待时间)：
     *   - 队列有空位 → 拷贝一份 msg 进去，立即返回 pdTRUE
     *   - 队列满了   → 最多等 10ms；还放不进就返回非 pdTRUE
     * 10ms 是"软超时"：宁可丢一条日志，也不能卡住采集任务
     * （LoggerTask 会周期取队列，所以一般不会满）
     */
    if (xQueueSend(logQueue, &msg, pdMS_TO_TICKS(10)) != pdTRUE)
    {
        App_Status_RecordLogDrop();   /* 统计丢了多少条（status 命令可看） */
    }
}

/* App_StartScheduler() 在 app_scheduler.c 中实现 */
