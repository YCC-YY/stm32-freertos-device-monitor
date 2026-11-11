#ifndef APP_STATUS_EXTRA_H
#define APP_STATUS_EXTRA_H

/*
 * app_status_extra.h —— 状态接口的"补充声明"（App 层）
 *
 * 这两个函数在 app_status.h 里其实也声明了，这里再单独列一份，
 * 是为了让某些只包含本头文件的模块（如 MonitorTask、app_scheduler.c）
 * 也能直接调用，避免重复包含整个 app_status.h 的场景。
 *
 * 作用：
 *   App_Status_SetUptime(ms)        更新运行时间（MonitorTask 每周期写一次）
 *   App_Status_SetSchedulerRunning()标记调度器是否已启动（调试用）
 */

#include "app_status.h"

#ifdef __cplusplus
extern "C" {
#endif

void App_Status_SetUptime(uint32_t ms);
void App_Status_SetSchedulerRunning(uint8_t run);

#ifdef __cplusplus
}
#endif

#endif /* APP_STATUS_EXTRA_H */
