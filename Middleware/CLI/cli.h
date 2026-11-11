#ifndef CLI_H
#define CLI_H

/*
 * cli.h —— 命令行接口（Command Line Interface）
 *
 * 把串口收到的一行文本，变成具体命令动作：
 *   help / status / sensor / log on|off / stats / reset
 *
 * 使用流程：
 *   CLI_InputChar() 喂字节 → 凑齐一行 → CLI_ReadLine() → CLI_Execute()
 */

#include <stdint.h>
#include <stdbool.h>
#include "app_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 命令类型：解析后得到，方便 switch 处理 */
typedef enum
{
    CMD_UNKNOWN = 0,
    CMD_HELP,
    CMD_STATUS,
    CMD_SENSOR,
    CMD_LOG,
    CMD_STATS,
    CMD_RESET,
    CMD_CHIPID,  /* I2C/BME280 通信单测：读 CHIP_ID */
    CMD_BMEREAD, /* 驱动单测：Init + 一次温湿度气压 */
    CMD_FAULT    /* 故障注入：fault sensor|comm|logger|clear */
} CommandType_t;

/*
 * 解析结果：CLI_ParseCommand 把一行文本拆开后的"成品"
 *
 * 字段说明（以输入 "log on" 为例）：
 *   type     命令编号，如 CMD_LOG（用 strcmp 把 name 与命令表比对后得出）
 *   name     第 1 个词 = 命令名，如 "log"（最长 16 字节，含结尾符 '\0'）
 *   arg      第 2 个词 = 参数，如 "on"（没有第二个词时是空字符串）
 *   has_arg  是否带了参数：true=有 arg，false=只有命令名；
 *            它的作用是让处理函数能明确区分"没给参数"和"参数恰好是空"
 */
typedef struct
{
    CommandType_t type;
    char name[16];
    char arg[16];
    bool has_arg;
} ParsedCommand_t;

/*
 * CLI_ReadLine 的返回值：告诉调用者"这一轮有没有拿到整行"
 *
 *   CLI_OK    取到了一条完整命令（out 里已经是这一行的内容）
 *   CLI_BUSY  还没有凑齐一行（用户还在打字），调用者继续等下一轮
 *   CLI_ERR   参数非法（out 是空指针，或 out_size 为 0）
 *
 * 典型用法（见 comm_task.c）：
 *   if (CLI_ReadLine(line, sizeof(line)) == CLI_OK) { CLI_Execute(line); }
 * 即：只有拿到 CLI_OK 才去执行命令，BUSY 时什么都不做。
 */
typedef enum
{
    CLI_OK = 0,
    CLI_BUSY,
    CLI_ERR
} CLIResult_t;

void CLI_Init(void);

/* 喂一个字符；返回 true 表示刚好凑齐一行 */
bool CLI_InputChar(char c);

/* 无换行的串口工具：静默超时后把当前行当命令提交 */
void CLI_PollLineTimeout(void);

/* 取走已就绪的一行；没有则 CLI_BUSY */
CLIResult_t CLI_ReadLine(char *out, uint16_t out_size);

void CLI_ParseCommand(const char *line, ParsedCommand_t *cmd);
void CLI_Execute(const char *line);
void CLI_PrintHelp(void);

void CLI_Printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* CLI_H */
