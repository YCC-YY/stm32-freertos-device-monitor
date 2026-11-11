#include "comm_channel.h"
#include "uart_driver.h"

#include <string.h>

/*
 * comm_channel.c —— 通道注册表
 *
 * 初学者理解：
 *   CommChannel_t 是一个“结构体函数指针”包，
 *   像遥控器按键：读、写两个功能先填好，再插进系统。
 */

static const CommChannel_t *s_channels[2];
static uint8_t s_count;

/* usb 通道的“读一个字节”：从串口环缓取 */
static bool usb_read(uint8_t *b)
{
    return UART_Driver_PopRx(b);
}

/* usb 通道的“写一串字节”：阻塞发送 */
static int usb_write(const uint8_t *data, size_t len)
{
    if (UART_Driver_Send(data, (uint16_t)len) != HAL_OK)
    {
        return -1;
    }
    return (int)len;
}

/* 预先填好的 usb 通道对象 */
static const CommChannel_t s_usb = {
    .name = "usb",
    .read_byte = usb_read,
    .write = usb_write,
};

/*
 * Comm_Init：通信通道初始化，任务启动时调用一次
 * 现在只注册一个 "usb" 通道（USART2 调试串口）；
 * 以后接了 BLE，再 Comm_RegisterChannel 一个 "ble" 通道即可。
 */
void Comm_Init(void)
{
    s_count = 0;
    Comm_RegisterChannel(&s_usb);
}

/*
 * Comm_RegisterChannel：把一个通道登记进通道表
 * 参数 ch：通道对象地址（内部只保存指针，不拷贝）
 * 通道表最多 2 个（数组大小），满了/空指针直接忽略
 */
void Comm_RegisterChannel(const CommChannel_t *ch)
{
    if (ch == NULL || s_count >= 2U)
    {
        return;
    }
    s_channels[s_count++] = ch;
}

/*
 * Comm_GetChannel：按名字找一个通道
 * 参数 name：要查找的通道名（如 "usb"）
 * 返回：找到的通道指针；找不到返回 NULL
 * 内部用 strcmp 逐个比较名字
 */
const CommChannel_t *Comm_GetChannel(const char *name)
{
    uint8_t i;
    if (name == NULL)
    {
        return NULL;
    }
    for (i = 0; i < s_count; i++)
    {
        if (s_channels[i] != NULL && strcmp(s_channels[i]->name, name) == 0)
        {
            return s_channels[i];
        }
    }
    return NULL;
}

/*
 * Comm_Default：返回默认通道（usb）
 * CLI / 业务代码直接用它收发，不需要关心通道名
 */
const CommChannel_t *Comm_Default(void)
{
    return &s_usb;
}
