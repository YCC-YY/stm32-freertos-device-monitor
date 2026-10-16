#include "ring_buffer.h"

void RingBuffer_Init(RingBuffer_t *rb, uint8_t *buffer, uint16_t capacity)
{
    if (rb == NULL || buffer == NULL || capacity == 0U)
    {
        return;
    }
    rb->buffer = buffer;
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

bool RingBuffer_Push(RingBuffer_t *rb, uint8_t data)
{
    if (rb == NULL || rb->buffer == NULL || RingBuffer_IsFull(rb))
    {
        return false;
    }
    rb->buffer[rb->head] = data;
    rb->head = (uint16_t)((rb->head + 1U) % rb->capacity);
    rb->count++;
    return true;
}

bool RingBuffer_Pop(RingBuffer_t *rb, uint8_t *data)
{
    if (rb == NULL || rb->buffer == NULL || data == NULL || RingBuffer_IsEmpty(rb))
    {
        return false;
    }
    *data = rb->buffer[rb->tail];
    rb->tail = (uint16_t)((rb->tail + 1U) % rb->capacity);
    rb->count--;
    return true;
}

uint16_t RingBuffer_Size(const RingBuffer_t *rb)
{
    if (rb == NULL)
    {
        return 0;
    }
    return rb->count;
}

uint16_t RingBuffer_Free(const RingBuffer_t *rb)
{
    if (rb == NULL)
    {
        return 0;
    }
    return (uint16_t)(rb->capacity - rb->count);
}

bool RingBuffer_IsEmpty(const RingBuffer_t *rb)
{
    return (rb == NULL) || (rb->count == 0U);
}

bool RingBuffer_IsFull(const RingBuffer_t *rb)
{
    return (rb != NULL) && (rb->count >= rb->capacity);
}

void RingBuffer_Reset(RingBuffer_t *rb)
{
    if (rb == NULL)
    {
        return;
    }
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}
