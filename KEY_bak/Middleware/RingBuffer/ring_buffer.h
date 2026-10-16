#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Single-producer single-consumer byte ring buffer.
 * If producer is ISR and consumer is thread, use RingBuffer_* inside
 * a critical section (or ensure architecture guarantees), volatile alone
 * is NOT a full concurrency solution.
 */
typedef struct
{
    uint8_t *buffer;
    uint16_t capacity; /* usable slots; internal storage is capacity+1 optional — we use full/empty via size counter style with head/tail */
    volatile uint16_t head; /* next write index */
    volatile uint16_t tail; /* next read index */
    volatile uint16_t count;
} RingBuffer_t;

void RingBuffer_Init(RingBuffer_t *rb, uint8_t *buffer, uint16_t capacity);
bool RingBuffer_Push(RingBuffer_t *rb, uint8_t data);
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
