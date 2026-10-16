#ifndef FREERTOS_APP_H
#define FREERTOS_APP_H

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
