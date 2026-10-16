#include "uart_driver.h"
#include "ring_buffer.h"
#include "app_config.h"

#include <string.h>
#include <stdbool.h>

/*
 * uart_driver.c —— 串口收发实现
 *
 * 重要规则（初学者最容易踩坑）：
 *   1. 中断函数里：不能 taskENTER_CRITICAL、不能拿互斥锁、不能 printf
 *   2. 中断只负责“把字节丢进环缓”，处理逻辑留给任务
 *   3. 发送可以阻塞，因为本项目约定 Logger/CLI 共用输出，不会很频繁
 *
 * 变量一览（都在本文件静态全局，外部用函数访问）：
 *   s_huart         当前用的串口外设（&huart2）
 *   s_rx_storage    收字节数组，真正存数据的内存
 *   s_rx_rb         环形缓冲区控制块（head/tail 指向 s_rx_storage）
 *   s_rx_overflow   环缓满导致丢字节的次数
 *   s_rx_isr_count  进入接收中断的次数（调试用）
 *   s_rx_pop_count  任务成功取走的字节数（调试用）
 */

static UART_HandleTypeDef *s_huart = NULL;
static uint8_t s_rx_storage[UART_RX_BUFFER_SIZE]; /* 长度 = 128，见 app_config.h */
static RingBuffer_t s_rx_rb;
static volatile uint32_t s_rx_overflow;
static volatile uint32_t s_rx_isr_count;
static volatile uint32_t s_rx_pop_count;

/*
 * 绑定串口句柄。
 * 参数 huart：CubeMX 生成的 huart2 地址，必须活在全局，不能是局部变量
 */
void UART_Driver_BindHandle(UART_HandleTypeDef *huart)
{
    s_huart = huart;
}

/*
 * 初始化环缓：把 s_rx_storage 这块内存“借”给 s_rx_rb 管理
 * 可重复调用（第二次会清空缓冲）
 */
void UART_Driver_Init(void)
{
    RingBuffer_Init(&s_rx_rb, s_rx_storage, (uint16_t)sizeof(s_rx_storage));
    s_rx_overflow = 0;
    s_rx_isr_count = 0;
    s_rx_pop_count = 0;
}

/*
 * 阻塞发送若干字节
 * data   ：要发送的数组
 * length ：字节个数
 * 返回 HAL_OK 表示发完
 */
HAL_StatusTypeDef UART_Driver_Send(const uint8_t *data, uint16_t length)
{
    if (data == NULL || length == 0U || s_huart == NULL)
    {
        return HAL_ERROR;
    }
    return HAL_UART_Transmit(s_huart, data, length, 100); /* 超时 100ms */
}

/* 发送 C 字符串（遇到 \0 结束），内部自己算长度 */
HAL_StatusTypeDef UART_Driver_SendString(const char *str)
{
    if (str == NULL)
    {
        return HAL_ERROR;
    }
    return UART_Driver_Send((const uint8_t *)str, (uint16_t)strlen(str));
}

/*
 * 中断上下文！只做一件事：Push 进环缓。
 * data：本次从 USART DR 寄存器读到的 1 个字节
 */
void UART_Driver_RxISR(uint8_t data)
{
    s_rx_isr_count++;
    if (!RingBuffer_Push(&s_rx_rb, data))
    {
        s_rx_overflow++;   /* 环缓满：丢掉这个字节并计数 */
    }
}

/*
 * 任务上下文：取一个字节
 * byte：输出参数，成功时写入取到的值
 * 返回 true = 取到了；false = 环缓空
 */
bool UART_Driver_PopRx(uint8_t *byte)
{
    bool ok;
    if (byte == NULL)
    {
        return false;
    }
    ok = RingBuffer_Pop(&s_rx_rb, byte);
    if (ok)
    {
        s_rx_pop_count++;
    }
    return ok;
}

uint32_t UART_Driver_RxOverflow(void)
{
    return s_rx_overflow;
}

uint32_t UART_Driver_RxIsrCount(void)
{
    return s_rx_isr_count;
}

uint32_t UART_Driver_RxPopCount(void)
{
    return s_rx_pop_count;
}
