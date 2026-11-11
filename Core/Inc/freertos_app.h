#ifndef FREERTOS_APP_H
#define FREERTOS_APP_H

/*
 * freertos_app.h —— Core 层集成点声明
 *
 * 用途：在 CubeMX 生成代码中调用 App_StartScheduler() 的备选入口。
 * 当前工程 main.c 已直接调用 App_StartScheduler()，本文件为预留接口。
 */
#ifdef __cplusplus
extern "C" {
#endif

/* CubeMX freertos.c hook — call App_StartScheduler() from main after MX init.
 * This header documents the integration point for generated code. */
void freertos_app_boot(void);

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_APP_H */
