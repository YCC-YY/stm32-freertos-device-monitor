#ifndef APP_STATUS_EXTRA_H
#define APP_STATUS_EXTRA_H

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
