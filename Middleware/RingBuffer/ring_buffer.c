#include "ring_buffer.h"

/*
 * ring_buffer.c —— 环形缓冲区实现（SPSC，无锁）
 *
 * 新手提示：
 *   - head 只在 Push 里前进
 *   - tail 只在 Pop 里前进
 *   - 这样中断写、任务读时，不需要互斥锁也不会互相踩
 */

/* 计算下一个下标（到了末尾绕回 0） */
static uint16_t next_index(uint16_t idx, uint16_t capacity)
{
    idx++;
    if (idx >= capacity)
    {
        idx = 0;
    }
    return idx;
}

/*
 * 初始化：把外部数组“借”给环缓使用。
 * capacity 至少为 2。
 */
void RingBuffer_Init(RingBuffer_t *rb, uint8_t *buffer, uint16_t capacity)
{
    if (rb == NULL || buffer == NULL || capacity < 2U)
    {
        return;
    }
    rb->buffer = buffer;
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
}

/* 生产者：写入一个字节 */
bool RingBuffer_Push(RingBuffer_t *rb, uint8_t data)
{
    uint16_t head;
    uint16_t tail;
    uint16_t next;

    if (rb == NULL || rb->buffer == NULL || rb->capacity < 2U)
    {
        return false;
    }

    head = rb->head;
    tail = rb->tail;
    next = next_index(head, rb->capacity);

    /* 满：再写就会追上 tail */
    if (next == tail)
    {
        return false;
    }

    rb->buffer[head] = data;
    rb->head = next;   /* 数据写完后再更新 head，消费者不会读到半成品 */
    return true;
}

/* 消费者：读出一个字节 */
bool RingBuffer_Pop(RingBuffer_t *rb, uint8_t *data)
{
    uint16_t head;
    uint16_t tail;

    if (rb == NULL || rb->buffer == NULL || data == NULL)
    {
        return false;
    }

    head = rb->head;
    tail = rb->tail;

    /* 空：生产者和消费者指针重合 */
    if (head == tail)
    {
        return false;
    }

    *data = rb->buffer[tail];
    rb->tail = next_index(tail, rb->capacity);
    return true;
}

/* 当前大约有多少字节（诊断用） */
uint16_t RingBuffer_Size(const RingBuffer_t *rb)
{
    uint16_t head;
    uint16_t tail;

    if (rb == NULL || rb->capacity == 0U)
    {
        return 0;
    }

    head = rb->head;
    tail = rb->tail;

    if (head >= tail)
    {
        return (uint16_t)(head - tail);
    }
    return (uint16_t)(rb->capacity - tail + head);
}

/* 还能放多少字节（不含保留的那一格） */
uint16_t RingBuffer_Free(const RingBuffer_t *rb)
{
    if (rb == NULL || rb->capacity < 2U)
    {
        return 0;
    }
    return (uint16_t)(rb->capacity - 1U - RingBuffer_Size(rb));
}

bool RingBuffer_IsEmpty(const RingBuffer_t *rb)
{
    if (rb == NULL)
    {
        return true;
    }
    return rb->head == rb->tail;
}

bool RingBuffer_IsFull(const RingBuffer_t *rb)
{
    uint16_t next;
    if (rb == NULL || rb->capacity < 2U)
    {
        return true;
    }
    next = next_index(rb->head, rb->capacity);
    return next == rb->tail;
}

void RingBuffer_Reset(RingBuffer_t *rb)
{
    if (rb == NULL)
    {
        return;
    }
    rb->head = 0;
    rb->tail = 0;
}
