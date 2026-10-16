#ifndef COMM_CHANNEL_H
#define COMM_CHANNEL_H

/*
 * comm_channel.h —— 通信通道抽象
 *
 * 目的：业务逻辑（CLI、命令）不绑死在“某一个串口”上。
 *
 * 现在：注册 "usb" = USART2 调试串口
 * 以后：BLE 模块接到 USART1 后，再注册 "ble"，命令处理不用重写
 *
 * 类比：插座标准固定，换的是插头（USB 或 BLE）。
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 一个“通道”需要提供：名字 + 收一个字节 + 发一串字节 */
typedef struct CommChannel
{
    const char *name;
    bool (*read_byte)(uint8_t *b);
    int  (*write)(const uint8_t *data, size_t len);
} CommChannel_t;

void Comm_Init(void);   /* 注册默认 usb 通道，任务启动时调用一次 */
void Comm_RegisterChannel(const CommChannel_t *ch);
const CommChannel_t *Comm_GetChannel(const char *name);

/* 当前默认通道（usb） */
const CommChannel_t *Comm_Default(void);

#ifdef __cplusplus
}
#endif

#endif /* COMM_CHANNEL_H */
