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
 * 数据流：
 *   环缓字节 → CLI_InputChar 组装一行 → CLI_Execute 执行
 *
 * 回显规则：只由 CLI_InputChar 回显一次，
 * 千万不要在本文件再 HAL_UART_Transmit 一遍，否则会看到命令重复。
 */

/* TaskHandle_t 用来标识和操作一个任务 ——> 通过它暂停、删除任务，或者获取任务状态
 * 保存任务句柄 */
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

/* 消费者任务
 * CommTask实际执行的内容 */
void CommTask_Function(void *argument)
{
    char line[CLI_MAX_COMMAND_LENGTH]; /* 定义一个字符数组 line，缓存完整命令行 */
    uint8_t byte;                      /* 临时存放环形缓冲区取出的 1 字节 */
    (void)argument;

    /* 打印提示符，初始化命令行和通信通道 */
    CLI_Init();
    Comm_Init();
    /* 记录任务已准备就绪的日志 */
    LOG_INFO("CommTask ready");

    for (;;)
    {
        /* 获取系统当前时间（以 tick 为单位），更新与通信任务相关的状态信息 */
        App_Status_BeatComm(App_GetTick());

        /* 1. 取字节：从串口环形缓冲区弹出 1 个字节，喂给输入处理函数；环缓空则循环结束 */
        while (UART_Driver_PopRx(&byte))
        {
            CLI_InputChar((char)byte);
        }
        /* 2. 超时检查：用户不发换行、静默超过阈值时，自动把当前行提交（避免命令卡住） */
        CLI_PollLineTimeout();

        /* 3. 取已就绪的整行并执行：行未就绪返回非 OK，不做处理；就绪才送入解析执行 */
        if (CLI_ReadLine(line, sizeof(line)) == CLI_OK)
        {
            CLI_Execute(line);
        }

        /* 短延时；后续可改成任务通知唤醒 */
        vTaskDelay(pdMS_TO_TICKS(COMM_POLL_MS));
    }
}
