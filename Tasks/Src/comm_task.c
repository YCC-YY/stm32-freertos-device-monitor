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
    uint32_t fault_start = 0;          /* FAULT_COMM 开始时刻，用于自动恢复 */
    (void)argument;

    /* 把自己（当前任务）的句柄登记给串口驱动：
     * 以后串口中断收到字节时，会调用 vTaskNotifyGiveFromISR(这个句柄)，
     * 给本任务发一个"通知"，本任务在下面的 ulTaskNotifyTake 里醒来。
     * 一句话：中断喊"有数据了！"，任务收到喊声就去处理。
     */
    UART_Driver_SetRxTask(xTaskGetCurrentTaskHandle());

    CLI_Init();     /* 命令行初始化：清空输入缓冲 + 打印一次提示符 */
    Comm_Init();    /* 通信通道注册：默认注册 usb（USART2）通道 */
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

        /*
         * 故障注入 FAULT_COMM：暂停消费环缓（CLI 假死）。
         * 8 秒后自动恢复，避免只能硬件复位；期间 Monitor 可能报 WARNING。
         */
        if (App_Fault_IsSet(FAULT_COMM))
        {
            if (fault_start == 0U)
            {
                fault_start = App_GetTick();
            }
            else if ((App_GetTick() - fault_start) >= 8000U)
            {
                App_Fault_Clear(FAULT_COMM);
                fault_start = 0;
                LOG_WARN("FAULT_COMM auto-cleared");
            }
            continue; /* 故障期间：不读环缓、不执行命令 */
        }
        fault_start = 0;

        /* 醒来后"尽量掏空"环缓：
         * 中断可能连续收到很多字节（比如你粘贴一段文字），
         * 每来一个字节就通知一次太浪费；所以一次醒来就把环缓里
         * 现有的字节全部取出来喂给 CLI_InputChar，取空为止。
         * UART_Driver_PopRx(&byte)：从环缓取 1 字节，取到返回 true
         */
        while (UART_Driver_PopRx(&byte))
        {
            CLI_InputChar((char)byte);   /* 喂一个字符给命令行（组行/退格/回显） */
        }

        /* 兜底：如果串口工具没发换行符，安静超时后也提交当前行 */
        CLI_PollLineTimeout();

        /* 有没有凑齐一整行？凑齐了 → 解析并执行；没凑齐 → 下一轮继续 */
        if (CLI_ReadLine(line, sizeof(line)) == CLI_OK)
        {
            CLI_Execute(line);
        }
    }
}
