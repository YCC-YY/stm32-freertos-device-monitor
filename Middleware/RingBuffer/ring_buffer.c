#include "ring_buffer.h"

/*
 * ring_buffer.c —— 环形缓冲区实现（SPSC，无锁）
 *
 * 一、SPSC 是什么
 *   Single Producer Single Consumer：单生产者、单消费者。
 *   本工程用法：串口接收中断是唯一生产者（Push），CommTask 是唯一消费者（Pop）。
 *
 * 二、两个下标（不是 C指针，是数组下标，类型 uint16_t）
 *   head：下一个要“写入”的位置，只有生产者会改它
 *   tail：下一个要“读出”的位置，只有消费者会改它
 *
 * 三、为什么不用锁
 *   1. head 只由生产者改，tail 只由消费者改，没有 “两个执行流同时改同一个变量 ”
 *   2. 一方读对方下标时，uint16_t 在 Cortex-M3 上是一条指令完成的原子访问，
 *      不会读到一半，所以不用关中断、也不用互斥锁
 *
 * 四、满/空怎么区分
 *   空：head == tail
 *   满：head 再往前走一格就撞上 tail（故意留一格空位，否则空和满无法区分）
 *
 * 五、限制
 *   只能一个生产者、一个消费者；多个任务同时写或同时读的场景必须加锁。
 */

/*
 * next_index（通用函数）：计算下一个下标
 * 下标先 +1，一旦走到数组末尾（ >= capacity）就绕回 0，
 * 下标在数组里循环使用，缓冲区因此形成 “环形 ”。
 * Capacity 是容量，代表这个环缓的总大小
 */
static uint16_t next_index(uint16_t idx, uint16_t capacity)
{
    idx++;                       /* 先往后挪一格 */
    if (idx >= capacity)         /* 挪完发现已经到数组末尾（下标编号等于容量） */
    {
        idx = 0;                 /* 绕回开头，下标在数组里循环，缓冲区因此形成“环形” */
    }
    return idx;
}

/*
 * RingBuffer_Init：初始化环缓
 * 环缓自己不分配内存，由调用方传入一个字节数组 buffer 和容量 capacity，
 * 环缓只负责按 head/tail 管理这个数组。
 * buffer 是真正存放字节数据的那块内存数组。
 * head/tail 都从 0 开始。
 * capacity 至少为 2：要留一格空位区分空和满，容量 1 存不下任何数据。
 */
void RingBuffer_Init(RingBuffer_t *rb, uint8_t *buffer, uint16_t capacity)
{
    /* 参数检查，任一不合法就直接返回、不做初始化：
     * rb == NULL      结构体指针为空，没地方写
     * buffer == NULL  没给数据数组，存不了字节
     * capacity < 2    容量太小，无法留一格区分空和满 */
    if (rb == NULL || buffer == NULL || capacity < 2U)
    {
        return;
    }
    rb->buffer = buffer;       /* 记住数据数组的首地址，之后 Push/Pop 靠它找数组 */
    rb->capacity = capacity;   /* 记住总容量，绕回判断和算空位都要用 */
    rb->head = 0;              /* 写下标从第 0格开始 */
    rb->tail = 0;              /* 读下标也从第 0格开始，此时 head==tail 表示空 */
}

/*
 * RingBuffer_Push：生产者写入一个字节
 * 返回 true  = 写入成功
 *      false = 参数非法，或缓冲区已满（数据被丢弃）
 *
 * 步骤：
 *   1. 把 head/tail 拷到局部变量
 *   2. 算出 head 的下一个位置，若撞上 tail 说明满了，放弃
 *   3. 数据写入 buffer[head]，再更新 head
 */
bool RingBuffer_Push(RingBuffer_t *rb, uint8_t data)
{
    uint16_t head;
    uint16_t tail;
    uint16_t next;

    if (rb == NULL || rb->buffer == NULL || rb->capacity < 2U)
    {
        return false;   /* 参数非法，无法写入 */
    }

    /* head 由生产者改、tail 由消费者改，判断满不满要同时看两个值。
     * 先拷到局部变量，本次判断用的是同一时刻的快照，
     * 不会出现“判断到一半 tail 被对方改了”的情况。这是无锁 SPSC 的常用手法。
     * “->”只是说 rb 在访问结构体的内容，具体得到的内容要看访问的变量类型（指针变量 / 普通变量）
     */
    head = rb->head;
    tail = rb->tail;
    next = next_index(head, rb->capacity);   /* 算出写位置的下一格 */

    /* 满：head 的下一个位置已经等于 tail，再写就会覆盖还没读的数据 */
    if (next == tail)
    {
        return false;   /* 满了，放弃这个字节（调用方负责计数丢包） */
    }

    rb->buffer[head] = data;   /* 先把字节写进数组当前位置 */
    /* 再更新 head：head 更新前消费者看不到这个位置，不会读到只写了一半的数据。
     * head/tail 在头文件里带 volatile，防止编译器把它优化成寄存器缓存，
     * 保证每次都从内存重新读对方的最新值。 */
    rb->head = next;
    return true;
}

/*
 * RingBuffer_Pop：消费者读出一个字节
 * 返回 true  = 读到数据
 *      false = 参数非法，或缓冲区为空
 * head == tail 表示没有数据可读。
 * 先从 buffer[tail] 取数据，再把 tail 推进到下一个位置。
 */
bool RingBuffer_Pop(RingBuffer_t *rb, uint8_t *data)
{
    uint16_t head;
    uint16_t tail;

    if (rb == NULL || rb->buffer == NULL || data == NULL)
    {
        return false;   /* 参数非法：data 是输出指针，为空就没地方放读到的字节 */
    }

    head = rb->head;
    tail = rb->tail;

    /* 空：两个下标重合，生产者还没写，或数据已被取完 */
    if (head == tail)
    {
        return false;   /* 没有数据可读 */
    }

    *data = rb->buffer[tail];              /* 从读位置取出字节，通过输出指针带出去 */
    rb->tail = next_index(tail, rb->capacity);  /* 读位置往后挪一格（末尾自动绕回） */
    return true;
}

/* const 只读不写数据；
 * RingBuffer_Size：当前存了多少字节（诊断用，stats 命令显示）
 * 分两种情况：
 *   head >= tail：没绕回，直接 head - tail
 *   head <  tail：已经绕回，tail 到数组尾的字节 + 数组头到 head 的字节
 */
uint16_t RingBuffer_Size(const RingBuffer_t *rb)
{
    uint16_t head;
    uint16_t tail;
    if (rb == NULL || rb->capacity == 0U)
    {
        return 0;
    }

    head = rb->head;     /* 把 head（写入下标） 当前位置存入 "head" 变量 */
    tail = rb->tail;     /* 把 tail（读入下标） 当前位置存入 "tail" 变量 */

    if (head >= tail)    /* 写入下标大于等于读入下标 */
    {
        /* 没绕回：head 在 tail 后面（相等时为空，差为0）
         * 例：tail=2, head=5 → 已有 5-2=3 个字节
         */
        return (uint16_t)(head - tail);
    }
    /* 绕回了：head 走到数组末尾又回到开头（head < tail）
     * 例：capacity=8, tail=6, head=2
     *     → 已用 = (8-6) + 2 = 4 个字节
     *       即 tail 到数组尾有 2 个，数组头到 head 有 2 个
     */
    return (uint16_t)(rb->capacity - tail + head);
}

/* *rb只读不写
 * RingBuffer_Free：还能再放多少字节
 * 计算：总容量 - 1（永远保留一格） - 已有字节数
 * 为什么减 1：空和满都是 head/tail 重合附近的状态，
 * 必须故意留一格空位，才能用“下一个 head 会碰到 tail”来表示满。
 */
uint16_t RingBuffer_Free(const RingBuffer_t *rb)
{
    if (rb == NULL || rb->capacity < 2U)    //检查传进来的指针是不是空的 或 容量 < 2U
                                            //（capacity 至少为 2：要留一格空位区分空和满，容量 1 存不下任何数据。）
    {
        return 0;   //传进来的指针是空的，或者容量不合法；无法计算剩余空位
    }
    return (uint16_t)(rb->capacity - 1U - RingBuffer_Size(rb));    //计算还能再存放多少字节
}

/*
 * RingBuffer_IsEmpty：是否为空
 * head == tail 即空：要么还没写过，要么消费者已经取完追上了生产者。
 */
bool RingBuffer_IsEmpty(const RingBuffer_t *rb)
{
    if (rb == NULL)     //是否为空指针
    {
        return true;
    }
    return rb->head == rb->tail;    //两个下标重合，此时环形缓冲区还没写入 / 消费者已经把数据读完了（读指针已经读完写指针写入的字节）
}

/*
 * RingBuffer_IsFull：是否已满
 * 用 next_index 试探 head 的下一个位置，等于 tail 就是满。
 */
bool RingBuffer_IsFull(const RingBuffer_t *rb)
{
    uint16_t next;
    if (rb == NULL || rb->capacity < 2U)
    {
        return true;
    }
    next = next_index(rb->head, rb->capacity);  /* 试探写位置的下一格，此时 NEXT已经存入 head 当前的位置 */
    return next == rb->tail;                       /* 下一格（NEXT）等于读指针的位置，就是满 */
}

/*  重置
 * RingBuffer_Reset：head/tail 归零，逻辑上清空缓冲区
 * 数组里的旧字节本身没有擦，但下标归零后旧数据不会再被读到。
 * 非线程安全：只能在初始化、或确认没有中断/任务正在访问时调用。
 */
void RingBuffer_Reset(RingBuffer_t *rb)
{
    if (rb == NULL)
    {
        return;
    }
    rb->head = 0;   /* 两个下标都归位，重新重合，逻辑上就是空 */
    rb->tail = 0;
}
