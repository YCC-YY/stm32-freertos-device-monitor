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

/*
 * 一个”通道”对象：名字 + 收一个字节 + 发一串字节
 *
 * 新手看这里（函数指针）：
 *   bool (*read_byte)(uint8_t *b) 意思是”这个结构体里存了一个函数地址”，
 *   这个函数长得像：bool 某个函数名(uint8_t *b)；
 *   调用时用 ch->read_byte(&byte) 就能收一个字节。
 *
 * 类比：结构体像”插座”，里面的函数指针像”插头的引脚”，
 *       不同的通信方式（USB / 以后的 BLE）都按这个形状实现，
 *       业务代码只认插座，不关心插的是谁。
 */
typedef struct CommChannel
{
    const char *name;                  /* 通道名，如 “usb”、”ble” */
    bool (*read_byte)(uint8_t *b);     /* 读 1 字节：成功填 *b 返回 true */
    int  (*write)(const uint8_t *data, size_t len); /* 发 len 字节，返回发送字节数 */
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
