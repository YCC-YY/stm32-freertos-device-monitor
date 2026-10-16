#include "cli.h"
#include "app_tasks.h"
#include "app_status.h"
#include "app_status_extra.h"
#include "app_types.h"
#include "uart_driver.h"
#include "sensor_driver.h"
#include "stm32f1xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

static char s_line[CLI_MAX_COMMAND_LENGTH];
static uint16_t s_line_len;
static volatile int s_line_ready;
static char s_ready_line[CLI_MAX_COMMAND_LENGTH];

void CLI_Init(void)
{
    s_line_len = 0;
    s_line_ready = 0;
    memset(s_line, 0, sizeof(s_line));
    memset(s_ready_line, 0, sizeof(s_ready_line));
    UART_Driver_SendString(CLI_PROMPT);
}

bool CLI_InputChar(char c)
{
    if (s_line_ready)
    {
        return true;
    }

    if (c == '\r' || c == '\n')
    {
        if (s_line_len == 0U)
        {
            UART_Driver_SendString(CLI_PROMPT);
            return false;
        }
        s_line[s_line_len] = '\0';
        memcpy(s_ready_line, s_line, sizeof(s_ready_line));
        s_line_len = 0;
        s_line_ready = 1;
        return true;
    }

    if (c == 0x08 || c == 0x7F) /* backspace */
    {
        if (s_line_len > 0U)
        {
            s_line_len--;
            s_line[s_line_len] = '\0';
            UART_Driver_SendString("\b \b");
        }
        return false;
    }

    if (c < 32 || c > 126)
    {
        return false;
    }

    if (s_line_len < (CLI_MAX_COMMAND_LENGTH - 1U))
    {
        s_line[s_line_len++] = c;
        /* echo */
        UART_Driver_Send((const uint8_t *)&c, 1);
    }
    return false;
}

CLIResult_t CLI_ReadLine(char *out, uint16_t out_size)
{
    if (out == NULL || out_size == 0U)
    {
        return CLI_ERR;
    }
    if (!s_line_ready)
    {
        return CLI_BUSY;
    }
    strncpy(out, s_ready_line, out_size - 1U);
    out[out_size - 1U] = '\0';
    s_line_ready = 0;
    memset(s_ready_line, 0, sizeof(s_ready_line));
    return CLI_OK;
}

static void rtrim(char *s)
{
    size_t n;
    if (s == NULL)
    {
        return;
    }
    n = strlen(s);
    while (n > 0U && (s[n - 1U] == ' ' || s[n - 1U] == '\t'))
    {
        s[--n] = '\0';
    }
}

void CLI_ParseCommand(const char *line, ParsedCommand_t *cmd)
{
    char buf[CLI_MAX_COMMAND_LENGTH];
    char *tok;
    char *save = NULL;

    if (cmd == NULL)
    {
        return;
    }
    memset(cmd, 0, sizeof(*cmd));
    cmd->type = CMD_UNKNOWN;

    if (line == NULL)
    {
        return;
    }

    strncpy(buf, line, sizeof(buf) - 1U);
    buf[sizeof(buf) - 1U] = '\0';
    rtrim(buf);

#if defined(_WIN32)
    tok = strtok_s(buf, " \t", &save);
#else
    tok = strtok(buf, " \t");
#endif
    if (tok == NULL)
    {
        return;
    }

    strncpy(cmd->name, tok, sizeof(cmd->name) - 1U);

#if defined(_WIN32)
    tok = strtok_s(NULL, " \t", &save);
#else
    tok = strtok(NULL, " \t");
#endif
    if (tok != NULL)
    {
        strncpy(cmd->arg, tok, sizeof(cmd->arg) - 1U);
        cmd->has_arg = true;
    }

    if (strcmp(cmd->name, "help") == 0)
    {
        cmd->type = CMD_HELP;
    }
    else if (strcmp(cmd->name, "status") == 0)
    {
        cmd->type = CMD_STATUS;
    }
    else if (strcmp(cmd->name, "sensor") == 0)
    {
        cmd->type = CMD_SENSOR;
    }
    else if (strcmp(cmd->name, "log") == 0)
    {
        cmd->type = CMD_LOG;
    }
    else if (strcmp(cmd->name, "stats") == 0)
    {
        cmd->type = CMD_STATS;
    }
    else if (strcmp(cmd->name, "reset") == 0)
    {
        cmd->type = CMD_RESET;
    }
    else
    {
        cmd->type = CMD_UNKNOWN;
    }
}

void CLI_PrintHelp(void)
{
    UART_Driver_SendString(
        "\r\nAvailable commands:\r\n"
        "  help          Show this help\r\n"
        "  status        System status\r\n"
        "  sensor        Latest sensor sample\r\n"
        "  log on|off    Enable/disable INFO logs\r\n"
        "  stats         Queues / heap / uptime\r\n"
        "  reset         Software reset\r\n\r\n");
}

static const char *state_name(SystemState_t s)
{
    switch (s)
    {
    case SYSTEM_INIT:    return "INIT";
    case SYSTEM_RUNNING: return "RUNNING";
    case SYSTEM_WARNING: return "WARNING";
    case SYSTEM_ERROR:   return "ERROR";
    default:             return "?";
    }
}

static void cmd_status(void)
{
    AppStatus_t st;
    char line[128];

    App_Status_Copy(&st);
    snprintf(line, sizeof(line),
             "\r\nSystem: %s\r\nUptime: %lu ms\r\nSensor: %s\r\nUART: %s\r\n"
             "Log: %s\r\nSensorErr: %lu\r\nLogDrop: %lu\r\n\r\n",
             state_name(st.system_state),
             (unsigned long)st.uptime_ms,
             st.sensor_ready ? "OK" : "NOT READY",
             "OK",
             st.logging_enabled ? "ON" : "OFF",
             (unsigned long)st.sensor_error_count,
             (unsigned long)st.log_dropped_count);
    UART_Driver_SendString(line);
}

static void cmd_sensor(void)
{
    SensorData_t d;
    char line[128];

    App_Status_GetLatestSensor(&d);
    if (!d.valid)
    {
        UART_Driver_SendString("\r\nNo sensor sample yet.\r\n\r\n");
        return;
    }
    snprintf(line, sizeof(line),
             "\r\nts=%lu  temp=%d.%d C  rh=%u.%u %%  light=%u\r\n\r\n",
             (unsigned long)d.timestamp,
             (int)(d.temperature_x10 / 10),
             (int)((d.temperature_x10 < 0 ? -d.temperature_x10 : d.temperature_x10) % 10),
             (unsigned)(d.humidity_x10 / 10),
             (unsigned)(d.humidity_x10 % 10),
             (unsigned)d.light);
    UART_Driver_SendString(line);
}

static void cmd_log(const ParsedCommand_t *cmd)
{
    if (!cmd->has_arg)
    {
        UART_Driver_SendString("\r\nUsage: log on | log off\r\n\r\n");
        return;
    }
    if (strcmp(cmd->arg, "on") == 0)
    {
        App_Status_SetLoggingEnabled(1U);
        UART_Driver_SendString("\r\nLogging ON\r\n\r\n");
    }
    else if (strcmp(cmd->arg, "off") == 0)
    {
        UART_Driver_SendString("\r\nLogging OFF\r\n\r\n");
        App_Status_SetLoggingEnabled(0U);
    }
    else
    {
        UART_Driver_SendString("\r\nUsage: log on | log off\r\n\r\n");
    }
}

static void cmd_stats(void)
{
    AppStatus_t st;
    char line[160];
    UBaseType_t sq = 0, lq = 0;

    App_Status_Copy(&st);
    if (sensorQueue)
    {
        sq = uxQueueMessagesWaiting(sensorQueue);
    }
    if (logQueue)
    {
        lq = uxQueueMessagesWaiting(logQueue);
    }

    snprintf(line, sizeof(line),
             "\r\nSensorQueue: %u used / %u\r\nLogQueue: %u used / %u\r\n"
             "FreeHeap: %u\r\nMinEverHeap: %u\r\nQueueFull: %lu\r\n\r\n",
             (unsigned)sq, (unsigned)SENSOR_QUEUE_LENGTH,
             (unsigned)lq, (unsigned)LOG_QUEUE_LENGTH,
             (unsigned)xPortGetFreeHeapSize(),
             (unsigned)xPortGetMinimumEverFreeHeapSize(),
             (unsigned long)st.sensor_queue_full_count);
    UART_Driver_SendString(line);
}

void CLI_Execute(const char *line)
{
    ParsedCommand_t cmd;

    if (line == NULL)
    {
        return;
    }

    CLI_ParseCommand(line, &cmd);

    switch (cmd.type)
    {
    case CMD_HELP:
        CLI_PrintHelp();
        break;
    case CMD_STATUS:
        cmd_status();
        break;
    case CMD_SENSOR:
        cmd_sensor();
        break;
    case CMD_LOG:
        cmd_log(&cmd);
        break;
    case CMD_STATS:
        cmd_stats();
        break;
    case CMD_RESET:
        UART_Driver_SendString("\r\nResetting...\r\n");
        NVIC_SystemReset();
        break;
    case CMD_UNKNOWN:
    default:
        if (cmd.name[0] != '\0')
        {
            UART_Driver_SendString("\r\nUnknown command. Type 'help'.\r\n\r\n");
        }
        break;
    }

    UART_Driver_SendString(CLI_PROMPT);
}
