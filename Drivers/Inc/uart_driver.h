#ifndef UART_DRIVER_H
#define UART_DRIVER_H

/*
 * uart_driver.h —— 串口驱动（USART2 = 调试口）
 *
 * 数据流向（新手记住这条线）：
 *
 *   电脑 USB-TTL
 *        │
 *        ▼
 *   USART2 中断（stm32f1xx_it.c）
 *        │  UART_Driver_RxISR(byte)
 *        ▼
 *   环形缓冲区 s_rx_rb
 *        │  UART_Driver_PopRx()
 *        ▼
 *   CommTask → CLI
 *
 * 发送方向：任务调用 UART_Driver_Send / SendString → HAL 阻塞发送
 */

#include <stdint.h>
#include <stdbool.h>
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

void UART_Driver_Init(void);
HAL_StatusTypeDef UART_Driver_Send(const uint8_t *data, uint16_t length);
HAL_StatusTypeDef UART_Driver_SendString(const char *str);

/*
 * 注册“谁关心串口数据”（一般是 CommTask）。
 * 中断收到字节后会 vTaskNotifyGiveFromISR 通知它起床。
 */
void UART_Driver_SetRxTask(TaskHandle_t task);

/* 中断里调用：收字节入环缓 + 通知接收任务 */
void UART_Driver_RxISR(uint8_t byte);

/* 任务里调用：从环缓取一个字节，没有则返回 false */
bool UART_Driver_PopRx(uint8_t *byte);

uint32_t UART_Driver_RxOverflow(void);
uint32_t UART_Driver_RxIsrCount(void);
uint32_t UART_Driver_RxPopCount(void);

/* 把 CubeMX 创建的 huart2 交给驱动（main 里调用一次） */
void UART_Driver_BindHandle(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif /* UART_DRIVER_H */
