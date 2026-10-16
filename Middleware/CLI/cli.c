/*
* cli.c —— 串口命令行实现
 *
 * 整体思路（一句话）：
 *   串口字符逐个收进来 -> 攒成一行 -> 按空格切开 -> 认命令名 -> 调用对应处理函数
 *
 * 新手提示：
 *   1. 一次只处理一行命令，不搞嵌套解析
 *   2. 认命令名用 strcmp 逐个比较，参数用空格 / Tab 切开
 *   3. 所有输出统一走 UART_Driver_SendString，不直接用 printf
 *
 * 状态变量（都是 static，只在本文件内使用）：
 *   s_line         正在输入中的行缓冲（用户还在打字，随时可能退格修改）
 *   s_line_len     当前行已经输入了几个字符
 *   s_line_ready   1 = 一行已敲完，等待 CLI_ReadLine 取走；0 = 还在输入
 *   s_ready_line   定稿后的命令副本（给 Execute 用，和输入缓冲分开，避免冲突）
 *   s_last_rx_tick 上次收到字符的时刻，用于"安静太久就自动按回车"
 */

#include "cli.h"
#include "app_tasks.h"
#include "app_status.h"
#include "app_status_extra.h"
#include "app_types.h"
#include "uart_driver.h"
#include "sensor_driver.h"
#include "sensor_task.h"
#include "comm_task.h"
#include "logger_task.h"
#include "monitor_task.h"
#include "stm32f1xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

static char s_line[CLI_MAX_COMMAND_LENGTH];         /* 正在输入的临时行 */
static uint16_t s_line_len;                        /* 临时行已存字符数 */
static volatile int s_line_ready;                 /* 1=有一行就绪待取 */
static char s_ready_line[CLI_MAX_COMMAND_LENGTH]; /* 定稿命令的存放处 */
static uint32_t s_last_rx_tick;                  /* 最近一次收字符的时刻 */

/*   cli_commit_line：把当前输入行"定稿"
 *   作用：临时行 s_line -> 复制到 s_ready_line，然后清空临时行准备打下一条
 *   前置条件：非空行，且当前没有已就绪未取走的行
 */
static void cli_commit_line(void)
{
    /* 空行不提交；上一行还没被取走也不覆盖 */
    if (s_line_len == 0U || s_line_ready)
    {
        return;
    }
    s_line[s_line_len] = '\0';                            /* C 字符串必须以 \0 结尾 */
    memcpy(s_ready_line, s_line, sizeof(s_ready_line));  /* 整行复制到定稿区 */
    s_line_len = 0;                                      /* 清空临时行长度 */
    s_line_ready = 1;                                    /* 通知外部：有一行就绪了 */
    UART_Driver_SendString("\r\n");                   /* 回车换行，光标到下一行开头 */
}

/* CLI_Init：开机初始化命令行，清空缓冲并打印一次提示符 */
void CLI_Init(void)
{
    s_line_len = 0;
    s_line_ready = 0;
    s_last_rx_tick = 0;
    memset(s_line, 0, sizeof(s_line));              /* memset：把一块内存全部填 0（清缓冲） */
    memset(s_ready_line, 0, sizeof(s_ready_line));
    UART_Driver_SendString(CLI_PROMPT);          /* 打印提示符，如 "> " */
}

/*   CLI_PollLineTimeout：超时自动提交
 *   有些串口工具按回车不发换行符，这里判断：如果一直在打字但超过
 *   CLI_LINE_TIMEOUT_MS 没新字符，就自动当作按了回车提交本行
 */
void CLI_PollLineTimeout(void)
{
    if (s_line_ready || s_line_len == 0U)  /* 已有就绪行 / 没在打字，都不用处理 */
    {
        return;
    }
    /* `App_GetTick()`：现在系统跑了多少毫秒（当前时刻）。
        s_last_rx_tick`：上一次收到你敲字的时刻，每敲一个字，`CLI_InputChar` 开头都会刷新它 */
    if ((App_GetTick() - s_last_rx_tick) >= CLI_LINE_TIMEOUT_MS)  // >= CLI_LINE_TIMEOUT_MS （判断这个差值有没有超过超时阈值）
    {
        cli_commit_line();  //把当前这行定稿提交（存到 `s_ready_line`、置 `s_line_ready=1`），效果等同于你手动按了回车。
    }
}

/*  CLI_InputChar：处理收到的一个字符（由 comm_task 每取一个字节调一次）
 *  参数 c：一个 ASCII 字符
 *  返回：true = 本字符让一行命令凑齐了；false = 还在输入中（或该字符被忽略）
 *
 * 分类处理：
 *   \r / \n   ：回车。空行只重显提示符；非空则提交本行
 *   0x08/0x7F ：退格（ASCII 里 BS=0x08，DEL=0x7F，两种都按退格处理）
 *   可打印字符：存进 s_line 并回显到串口
 *   其他控制字符：直接丢弃
 */
bool CLI_InputChar(char c)
{
    /* 上一行还没被 ReadLine 取走，先不接收新字符，避免覆盖 */
    if (s_line_ready)
    {
        return true; /* 当 s_line_ready 为真时，说明上一行还没被取走，先不处理新字符 */
    }

    s_last_rx_tick = App_GetTick();  /* 记下本次收字时刻，刷新超时计时 */

    /* ---- 回车：提交命令 ---- */
    if (c == '\r' || c == '\n')
    {
        if (s_line_len == 0U)  /* s_line_len == 0U（即当前行输入字符数为 0）；直接按了回车、什么都没打：只重显提示符 */
        {
            UART_Driver_SendString("\r\n" CLI_PROMPT); /* 空行：只重新提示符（提示可以继续重新输入） */
            return false;   //表明还在输入中
        }
        cli_commit_line();  /* 非空行：定稿提交 */
        return true;
    }

    /* ---- 退格：删掉最后一个字符 ---- */
    if (c == 0x08 || c == 0x7F) /* 判断 C 是否为退格键（backspace） */
    {
        /* 当前已输入字符数 s_line_len大于 0，
         * 就删除前一个字符，s_line_len减 1 */
        if (s_line_len > 0U)
        {
            s_line_len--;               /* 长度减 1 */
            /* 将当前位置设为字符串结束符'\0'；结尾符往前挪一位 = 最后一个字消失
             * 字符数组有一个硬性要求：必须以 '\0'（空字符，ASCII 码值为 0）作为结尾标记；通过识别'\0'来判断 “字符串到这里结束了”
             */
            s_line[s_line_len] = '\0';
            UART_Driver_SendString("\b \b");  /* 串口回显：光标左移、打空格擦掉、再左移 */
        }
        return false;
    }

    /* 其他不可打印的控制字符（如 Ctrl 组合键）直接忽略 */
    if (c < 32 || c > 126)
    {
        return false;
    }

    /* ---- 普通可打印字符：存起来并回显 ---- */
    if (s_line_len < (CLI_MAX_COMMAND_LENGTH - 1U))         /* 留 1 个位置给 '\0'，防止越界 */
    {
        s_line[s_line_len++] = c;                           /* 存入数组，长度加 1（先存后加） */
        /* echo */
        UART_Driver_Send((const uint8_t *)&c, 1);    /* （回显）echo：把这个字原样发回，让用户看到   (const uint8_t *)-->强制类型转换 */
    }
    return false;
}

/*
 * CLI_ReadLine：取走一条就绪的命令行
 *   out    ：输出缓冲，命令写到这里
 *   out_size：缓冲大小
 * 返回：CLI_OK=取到了；CLI_BUSY=还没有就绪行；CLI_ERR=参数非法
 * 取走后会复位 s_line_ready 并清空定稿区，为下一条命令腾位置
 */
CLIResult_t CLI_ReadLine(char *out, uint16_t out_size)
{
    if (out == NULL || out_size == 0U)
    {
        return CLI_ERR;
    }
    if (!s_line_ready)                /* 还没攒成行 */
    {
        return CLI_BUSY;
    }
    strncpy(out, s_ready_line, out_size - 1U);
    out[out_size - 1U] = '\0';      /* 强制结尾符，防止越界读出 */
    s_line_ready = 0;
    memset(s_ready_line, 0, sizeof(s_ready_line));
    return CLI_OK;
}

/* rtrim：去掉字符串末尾的空格和 Tab（只清尾部，不动前面） */
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

/*
 * CLI_ParseCommand：解析一行命令文本
 * 输入 line，例如 "log on"
 * 输出 cmd 结构体：
 *   type    = 命令编号（CMD_LOG 等，用 strcmp 匹配出来）
 *   name    = 第一个词，即命令名 "log"
 *   arg     = 第二个词，即参数 "on"，没有则为空
 *   has_arg = 是否带了参数
 * 做法：先复制到临时 buf -> rtrim 去尾空格 -> strtok 按空格/Tab 切词 -> 逐个 strcmp 认命令
 */
void CLI_ParseCommand(const char *line, ParsedCommand_t *cmd)
{
    char buf[CLI_MAX_COMMAND_LENGTH];
    char *tok;
#if defined(_WIN32)
    char *save = NULL;
#endif

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
    rtrim(buf);         /* 先去掉行尾多余空格 */

/* strtok：按 " \t"（空格/Tab）把字符串切成一个个词；第一次调用传 buf，之后传 NULL */
#if defined(_WIN32)
    tok = strtok_s(buf, " \t", &save);
#else
    tok = strtok(buf, " \t");
#endif
    if (tok == NULL)
    {
        return;         /* 空行，什么都没切到 */
    }

    strncpy(cmd->name, tok, sizeof(cmd->name) - 1U);    /* 第 1 个词 = 命令名 */

/* 再取下一个词 = 参数（没有就保持空） */
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

    /* 命令名逐个比较，命中就给出对应的命令编号 */
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
        cmd->type = CMD_UNKNOWN;        /* 都不匹配 = 未知命令 */
    }
}

/* CLI_PrintHelp：打印所有可用命令的说明清单 */
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

/* state_name：把系统状态枚举翻译成可读字符串，方便打印 */
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

/* cmd_status：处理 status 命令，打印系统概要
 * 先从 App_Status 拷贝一份状态快照到本地 st，再用 snprintf 拼字符串发出去
 */
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

/*
 * cmd_sensor：处理 sensor 命令，显示最近一帧传感器数据
 * 温湿度用的是放大 10 倍的整数存储（如 253 表示 25.3），打印时拆成整数位和小数位
 */
static void cmd_sensor(void)
{
    SensorData_t d;
    char line[128];

    App_Status_GetLatestSensor(&d);
    if (!d.valid)               /* 还没采到任何数据 */
    {
        UART_Driver_SendString("\r\nNo sensor sample yet.\r\n\r\n");
        return;
    }
    /* temperature_x10=253 → 25.3°C，用整数拆成整数部分和小数部分打印 */
    snprintf(line, sizeof(line),
             "\r\nts=%lu  temp=%d.%d C  rh=%u.%u %%  press=%lu Pa\r\n\r\n",
             (unsigned long)d.timestamp,
             (int)(d.temperature_x10 / 10),             /* 整数部分 */
             (int)((d.temperature_x10 < 0 ? -d.temperature_x10 : d.temperature_x10) % 10),  /* 小数位，负数先取绝对值 */
             (unsigned)(d.humidity_x10 / 10),
             (unsigned)(d.humidity_x10 % 10),
             (unsigned long)d.pressure_pa);
    UART_Driver_SendString(line);
}

/* cmd_log：处理 log on / log off，切换日志开关；没带参数就提示用法 */
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

/*
 * cmd_stats：处理 stats 命令，打印队列占用、剩余堆内存、各任务栈高水位
 * StackHWM：任务历史上最少剩下多少栈字，越大说明余量越充足
 */
static void cmd_stats(void)
{
    AppStatus_t st;
    char line[200];
    UBaseType_t sq = 0, lq = 0;
    UBaseType_t hwm_s = 0, hwm_c = 0, hwm_l = 0, hwm_m = 0;

    App_Status_Copy(&st);
    /* 两个队列当前已放了多少条消息（队列为空则跳过） */
    if (sensorQueue)
    {
        sq = uxQueueMessagesWaiting(sensorQueue);
    }
    if (logQueue)
    {
        lq = uxQueueMessagesWaiting(logQueue);
    }

    /* 各任务的栈高水位（句柄为空则跳过） */
    if (SensorTask_Handle())
    {
        hwm_s = uxTaskGetStackHighWaterMark(SensorTask_Handle());
    }
    if (CommTask_Handle())
    {
        hwm_c = uxTaskGetStackHighWaterMark(CommTask_Handle());
    }
    if (LoggerTask_Handle())
    {
        hwm_l = uxTaskGetStackHighWaterMark(LoggerTask_Handle());
    }
    if (MonitorTask_Handle())
    {
        hwm_m = uxTaskGetStackHighWaterMark(MonitorTask_Handle());
    }

    snprintf(line, sizeof(line),
             "\r\nSensorQueue: %u used / %u\r\nLogQueue: %u used / %u\r\n"
             "FreeHeap: %u\r\nMinEverHeap: %u\r\nQueueFull: %lu\r\n"
             "StackHWM(words) S=%u C=%u L=%u M=%u\r\n\r\n",
             (unsigned)sq, (unsigned)SENSOR_QUEUE_LENGTH,
             (unsigned)lq, (unsigned)LOG_QUEUE_LENGTH,
             (unsigned)xPortGetFreeHeapSize(),
             (unsigned)xPortGetMinimumEverFreeHeapSize(),
             (unsigned long)st.sensor_queue_full_count,
             (unsigned)hwm_s, (unsigned)hwm_c, (unsigned)hwm_l, (unsigned)hwm_m);
    UART_Driver_SendString(line);
}

/*
 * CLI_Execute：命令分发入口
 * 流程：先 ParseCommand 把行解析成 cmd -> 按 cmd.type 在 switch 里调用对应函数
 * 执行完统一再打印一次提示符，等待下一条命令
 */
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
        NVIC_SystemReset();             /* 软件复位芯片 */
        break;
    case CMD_UNKNOWN:
    default:
        if (cmd.name[0] != '\0')        /* 真的输入了字但认不出，才提示未知命令；空行不提示 */
        {
            UART_Driver_SendString("\r\nUnknown command. Type 'help'.\r\n\r\n");
        }
        break;
    }

    UART_Driver_SendString(CLI_PROMPT);    /* 无论成功失败，最后都重显提示符 */
}
