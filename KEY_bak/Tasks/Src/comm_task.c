#include "comm_task.h"
#include "app_tasks.h"
#include "app_status.h"
#include "app_config.h"
#include "cli.h"
#include "ring_buffer.h"
#include "uart_driver.h"

#include "FreeRTOS.h"
#include "task.h"

static TaskHandle_t s_comm_handle = NULL;

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

void CommTask_Function(void *argument)
{
    char line[CLI_MAX_COMMAND_LENGTH];
    (void)argument;

    CLI_Init();
    LOG_INFO("CommTask ready");

    for (;;)
    {
        App_Status_BeatComm(App_GetTick());

        /* Drain ring buffer into CLI line assembler */
        uint8_t byte;
        while (UART_Driver_PopRx(&byte))
        {
            CLI_InputChar((char)byte);
        }

        if (CLI_ReadLine(line, sizeof(line)) == CLI_OK)
        {
            CLI_Execute(line);
        }

        vTaskDelay(pdMS_TO_TICKS(COMM_POLL_MS));
    }
}
