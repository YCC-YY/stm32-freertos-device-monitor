#ifndef RING_BUFFER_H
#define RING_BUFFER_H

/*
 * ring_buffer.h —— 环形缓冲区（Ring Buffer）
 *
 * 形象理解：一条环形传送带，只能从一头放、另一头取。
 *
 * 本项目用法（单生产者单消费者 SPSC）：
 *   生产者：串口中断 ISR 调 Push
 *   消费者：CommTask 调 Pop
 *
 * 为什么不用 count 计数？
 *   中断里改 count、任务里也改 count，可能“抢”同一个变量导致重复读/丢数据。
 *   只维护 head/tail 两个写指针，更安全：
 *     空：head == tail
 *     满：下一个 head 会碰到 tail
 *   可用容量 = capacity - 1（故意空出一格区分满/空）。
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint8_t *buffer;         /* 存数据的数组首地址 */
    uint16_t capacity;       /* 数组总长度；可存 capacity-1 个 */
    volatile uint16_t head;  /* 写入下标，仅生产者改 */
    volatile uint16_t tail;  /* 读出下标，仅消费者改 */
} RingBuffer_t;

void RingBuffer_Init(RingBuffer_t *rb, uint8_t *buffer, uint16_t capacity);

/* 放入一个字节。满则返回 false。只能在生产者上下文调用（本项目=中断） */
bool RingBuffer_Push(RingBuffer_t *rb, uint8_t data);

/* 取出一个字节。空则返回 false。只能在消费者上下文调用（本项目=任务） */
bool RingBuffer_Pop(RingBuffer_t *rb, uint8_t *data);

uint16_t RingBuffer_Size(const RingBuffer_t *rb);
uint16_t RingBuffer_Free(const RingBuffer_t *rb);
bool RingBuffer_IsEmpty(const RingBuffer_t *rb);
bool RingBuffer_IsFull(const RingBuffer_t *rb);
void RingBuffer_Reset(RingBuffer_t *rb);

#ifdef __cplusplus
}
#endif

#endif /* RING_BUFFER_H */
