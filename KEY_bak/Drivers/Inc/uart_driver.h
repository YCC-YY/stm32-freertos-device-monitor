#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f1xx_hal.h" /* adjust family via CubeMX generated header */

#ifdef __cplusplus
extern "C" {
#endif

void UART_Driver_Init(void);
HAL_StatusTypeDef UART_Driver_Send(const uint8_t *data, uint16_t length);
HAL_StatusTypeDef UART_Driver_SendString(const char *str);

/* Called from USARTx_IRQHandler for each received byte (byte-by-byte RX mode). */
void UART_Driver_RxISR(uint8_t byte);

/* Task-side: pop one byte from RX ring (ISR producer). */
bool UART_Driver_PopRx(uint8_t *byte);
uint32_t UART_Driver_RxOverflow(void);

/* Optional: bind HAL handle set by CubeMX (assign in main after MX_USART init). */
void UART_Driver_BindHandle(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif /* UART_DRIVER_H */
