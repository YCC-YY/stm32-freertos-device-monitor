#include "uart_driver.h"
#include "ring_buffer.h"
#include "app_config.h"
#include "app_status.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <string.h>

static UART_HandleTypeDef *s_huart = NULL;
static uint8_t s_rx_storage[UART_RX_BUFFER_SIZE];
static RingBuffer_t s_rx_rb;
static volatile uint32_t s_rx_overflow;

/* Critical section helpers for ISR producer / task consumer */
static inline void rb_enter(void)
{
    taskENTER_CRITICAL();
}

static inline void rb_exit(void)
{
    taskEXIT_CRITICAL();
}

void UART_Driver_BindHandle(UART_HandleTypeDef *huart)
{
    s_huart = huart;
}

void UART_Driver_Init(void)
{
    RingBuffer_Init(&s_rx_rb, s_rx_storage, sizeof(s_rx_storage));
    s_rx_overflow = 0;
    /* If CubeMX already opened the port, optional re-enable IRQ here. */
}

HAL_StatusTypeDef UART_Driver_Send(const uint8_t *data, uint16_t length)
{
    if (data == NULL || length == 0U)
    {
        return HAL_ERROR;
    }
    if (s_huart == NULL)
    {
        return HAL_ERROR;
    }
    /* Blocking send is OK for MVP because only LoggerTask/CLI own TX. */
    return HAL_UART_Transmit(s_huart, data, length, 100);
}

HAL_StatusTypeDef UART_Driver_SendString(const char *str)
{
    if (str == NULL)
    {
        return HAL_ERROR;
    }
    return UART_Driver_Send((const uint8_t *)str, (uint16_t)strlen(str));
}

void UART_Driver_RxISR(uint8_t byte)
{
    rb_enter();
    if (!RingBuffer_Push(&s_rx_rb, byte))
    {
        s_rx_overflow++;
        App_Status_RecordUartError();
    }
    rb_exit();

    /* Phase-2: replace polling with:
     * vTaskNotifyGiveFromISR(commTaskHandle, &xHigherPriorityTaskWoken);
     * portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
     */
}

bool UART_Driver_PopRx(uint8_t *byte)
{
    bool ok;
    if (byte == NULL)
    {
        return false;
    }
    rb_enter();
    ok = RingBuffer_Pop(&s_rx_rb, byte);
    rb_exit();
    return ok;
}

uint32_t UART_Driver_RxOverflow(void)
{
    return s_rx_overflow;
}

/* Weak default IRQ glue — override in user it.c if needed */
__weak void USART1_IRQHandler(void)
{
    if (s_huart != NULL && s_huart->Instance != NULL)
    {
        if (__HAL_UART_GET_FLAG(s_huart, UART_FLAG_RXNE) != RESET)
        {
            uint8_t b = (uint8_t)(s_huart->Instance->DR & 0xFFU);
            UART_Driver_RxISR(b);
        }
        if (__HAL_UART_GET_FLAG(s_huart, UART_FLAG_ORE) != RESET)
        {
            __HAL_UART_CLEAR_OREFLAG(s_huart);
            App_Status_RecordUartError();
        }
    }
}
