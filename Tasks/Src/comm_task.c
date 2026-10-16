#include "comm_task.h"
#include "app_tasks.h"
#include "app_status.h"
#include "app_config.h"
#include "cli.h"
#include "comm_channel.h"
#include "uart_driver.h"

#include "FreeRTOS.h"
#include "task.h"

/*
 * comm_task.c —— 串口命令处理任务
 *
 * 数据流（事件驱动）：
 *   UART 中断 Push 环缓 + vTaskNotifyGiveFromISR
 *        ↓
 *   CommTask ulTaskNotifyTake 醒来
 *        ↓
 *   一次掏空 RingBuffer → CLI 组行 → Execute
 *
 * 通知只是“有数据了”的信号，字节本身在环缓里。
 * 回显规则：只由 CLI_InputChar 回显一次，不要在本文件再 Transmit。
 */

/* 任务句柄：标识本任务，也交给串口驱动做通知 */
static TaskHandle_t s_comm_handle = NULL;

/* 其他模块能通过调用这个函数拿到任务句柄
 * 返回 s_comm_handle 的值         */
TaskHandle_t CommTask_Handle(void)
{
    return s_comm_handle;
}

/* 创建任务，确保任务创建成功不卡死 */
void CommTask_Start(void)
{
    if (xTaskCreate(CommTask_Function,
                    "CommTask",
                    STACK_COMM,
                    NULL,
                    PRIORITY_COMM,
                    &s_comm_handle) != pdPASS)
    {
        for (;;)
        {
        }
    }
}

/* 消费者任务主体 */
void CommTask_Function(void *argument)
{
    char line[CLI_MAX_COMMAND_LENGTH]; /* 完整命令行缓冲 */
    uint8_t byte;                      /* 环缓取出的 1 字节 */
    (void)argument;

    /* 把句柄交给串口驱动：中断用 Task Notification 唤醒本任务 */
    UART_Driver_SetRxTask(xTaskGetCurrentTaskHandle());

    CLI_Init();
    Comm_Init();
    LOG_INFO("CommTask ready");

    for (;;)
    {
        App_Status_BeatComm(App_GetTick());

        /*
         * 事件驱动等待：
         *   有通知（串口来数据）→ 立刻返回
         *   最长等 50ms → 兜底处理超时提交 / 心跳
         * pdTRUE = 取出后清空通知计数（多次中断合并为一次）
         */
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(50));

        /* 醒来后尽量掏空环缓（不是 1 字节 1 通知） */
        while (UART_Driver_PopRx(&byte))
        {
            CLI_InputChar((char)byte);
        }

        CLI_PollLineTimeout();

        if (CLI_ReadLine(line, sizeof(line)) == CLI_OK)
        {
            CLI_Execute(line);
        }
    }
}
