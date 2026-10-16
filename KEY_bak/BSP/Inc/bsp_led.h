#ifndef BSP_LED_H
#define BSP_LED_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void BSP_LED_Init(void);
void BSP_LED_On(void);
void BSP_LED_Off(void);
void BSP_LED_Toggle(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_LED_H */
