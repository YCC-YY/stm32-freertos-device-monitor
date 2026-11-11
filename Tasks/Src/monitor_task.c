#include "monitor_task.h"
#include "app_tasks.h"
#include "app_status.h"
#include "app_status_extra.h"
#include "app_config.h"
#include "bsp_led.h"
#include "iwdg.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/*
 * monitor_task.c —— 系统健康监控
 *
 * 职责：
 *   1. 检查各任务心跳是否超时（卡死/饿死）
 *   2. 根据错误次数调整 SystemState（RUNNING / WARNING）
 *   3. 更新运行时间
 *   4. 翻转 LED（肉眼可见“活着”）
 *   5. 喂独立看门狗 IWDG（不喂会复位）
 *   6. 周期性打印状态摘要（可由 STATUS_HEARTBEAT_PRINT_EN 开关）
 *
 * 注意：IWDG 约 4 秒超时，本任务 2 秒跑一次，正常不会复位。
 */

static TaskHandle_t s_monitor_handle = NULL;

/*
 * MonitorTask_Handle：返回本任务句柄
 * 给 CLI 的 stats 命令查栈水位、或调试时挂起/查看任务用
 */
TaskHandle_t MonitorTask_Handle(void)
{
    return s_monitor_handle;
}

/*
 * heartbeat_stale：判断某个任务的心跳是否”过期”（疑似卡死）
 * 参数：
 *   last_ms    该任务上次打卡的时间（App_GetTick 得到的毫秒数）
 *   now_ms     当前时间
 *   timeout_ms 多久没打卡就算过期（本项目 5000ms）
 * 返回：1 = 过期（任务可能卡死），0 = 正常
 *
 * 原理：last_ms == 0 说明这个任务从开机起一次卡都没打过，
 *       直接认为异常；否则比较”现在 - 上次”是否超过阈值。
 * 注意：任务心跳是”时间戳”而不是”计数”，这是判断卡死的关键——
 *       任务只要还在跑，就会不停刷新这个时间戳。
 */
static int heartbeat_stale(uint32_t last_ms, uint32_t now_ms, uint32_t timeout_ms)
{
    if (last_ms == 0U)
    {
        return 1; /* 还没打过卡 */
    }
    return ((now_ms - last_ms) > timeout_ms) ? 1 : 0;
}

/*
 * MonitorTask_Start：创建监控任务（由 App_StartScheduler 调用）
 * xTaskCreate 的 6 个参数：入口函数、任务名、栈字数、参数、优先级、句柄地址
 * 括号里优先级 MONITOR=2、栈 STACK_MONITOR=256 words（见 app_config.h）
 * 创建失败（堆不够）就死循环，方便调试器抓现场
 */
void MonitorTask_Start(void)
{
    if (xTaskCreate(MonitorTask_Function,
                    "MonitorTask",
                    STACK_MONITOR,
                    NULL,
                    PRIORITY_MONITOR,
                    &s_monitor_handle) != pdPASS)
    {
        for (;;)
        {
        }
    }
}

void MonitorTask_Function(void *argument)
{
    AppStatus_t st;  /* 状态快照：每轮从 g_status 拷贝一份，本轮判断都基于这份副本 */
    /* last_summary：上次打印状态摘要的时刻（毫秒时间戳）。
     * 初始 0 = 开机后第一轮就满足“距上次已超过10秒”，进循环后立即打第一行摘要。
     * 它只服务于摘要打印，与心跳检测是两套独立计时，互不影响。 */
    uint32_t last_summary = 0;
    (void)argument;

    BSP_LED_Init();
    LOG_INFO("MonitorTask ready");

    for (;;)
    {
        uint32_t now = App_GetTick();
        int unhealthy = 0;

        App_Status_BeatMonitor(now);   /* 自己的心跳也要打卡（不能只查别人） */
        App_Status_SetUptime(now);     /* 更新开机运行时长（status 命令显示） */

        /* 拷一份状态快照：拿锁→复制→放锁，之后本任务自己慢慢看这份副本 */
        App_Status_Copy(&st);

        /* ---- 检查健康：任一条件成立就算"不健康" ----
         * ① Sensor/Comm/Logger 三个任务的心跳超时（它们卡死或没起来）
         * ② 传感器错误累计达到阈值（SENSOR_ERROR_WARN_LIMIT=5）
         * unhealthy=1 → 系统状态切到 WARNING；否则保持 RUNNING
         */
        if (heartbeat_stale(st.heartbeat.sensor_heartbeat, now, HEARTBEAT_TIMEOUT_MS) ||
            heartbeat_stale(st.heartbeat.comm_heartbeat, now, HEARTBEAT_TIMEOUT_MS) ||
            heartbeat_stale(st.heartbeat.logger_heartbeat, now, HEARTBEAT_TIMEOUT_MS))
        {
            unhealthy = 1;
        }

        /* 传感器错误太多 → 不健康 */
        if (st.sensor_error_count >= SENSOR_ERROR_WARN_LIMIT)
        {
            unhealthy = 1;
        }

        /* ---- 根据健康结果切换系统大状态 ----
         * 不健康 → WARNING（有异常但还能跑）
         * 健康且之前是 WARNING/INIT → 恢复到 RUNNING（自动恢复）
         * 这样插回传感器/清除故障后，状态能自己回正常，不用复位
         */
        if (unhealthy)
        {
            App_Status_SetState(SYSTEM_WARNING);
        }
        else if (st.system_state == SYSTEM_WARNING || st.system_state == SYSTEM_INIT)
        {
            App_Status_SetState(SYSTEM_RUNNING);
        }

        BSP_LED_Toggle();   /* LED 翻转：肉眼能确认系统在跑（每 2 秒闪一次） */

        /* ---- 喂看门狗 ----
         * 独立看门狗 IWDG 一旦启动就不能软件关闭，超时（约 4 秒）
         * 没被喂就强制复位芯片。这里每 2 秒喂一次（本任务 2 秒周期），
         * 只有系统不是 ERROR 时才喂：如果业务代码把自己判成 ERROR，
         * 就故意不喂狗让系统复位，作为最后的兜底恢复手段。
         */
        if (st.system_state != SYSTEM_ERROR)
        {
            HAL_IWDG_Refresh(&hiwdg);
        }

        /*
         * ====== 周期性状态摘要（无人值守时的“自动汇报”）======
         * 只负责“展示”，不参与心跳判定、不影响系统状态切换。
         * 用 LOG_INFO 把摘要送进日志队列，由 LoggerTask 统一写串口：
         *   本任务不直接操作 UART，不会因为串口慢而阻塞，喂狗节奏不受影响。
         */
#if STATUS_HEARTBEAT_PRINT_EN   /* 条件编译：宏值为0时，#if到#endif之间整块不编译，零开销 */
        /* 判断距上次打印是否已满10秒。
         * now和last_summary都是无符号数，直接相减即使遇到tick计数溢出回绕，
         * 结果仍然是正确的时间差，不需要额外处理。 */
        if ((now - last_summary) >= STATUS_HEARTBEAT_PRINT_MS)
        {
            /* 先刷新时间戳，再打日志：保证下一轮计时从现在开始，
             * 即使日志队列满被丢弃，打印节奏也不会乱。 */
            last_summary = now;

            /* 一行摘要包含4项信息：
             * uptime ：开机运行秒数（now是毫秒，除以1000换成秒）
             * sensor ：传感器是否已就绪（READY=已采到数据，NOT READY=还没采到）
             * hb     ：本轮健康检查结果（OK=健康，WARNING=有异常，用上面的unhealthy）
             * err    ：传感器错误累计次数（对应status命令里的SensorErr） */
            LOG_INFO("uptime=%lus sensor=%s hb=%s err=%lu",
                     (unsigned long)(now / 1000U),
                     st.sensor_ready ? "READY" : "NOT READY",
                     unhealthy ? "WARNING" : "OK",
                     (unsigned long)st.sensor_error_count);
        }
#endif

        vTaskDelay(pdMS_TO_TICKS(MONITOR_PERIOD_MS));
    }
}
