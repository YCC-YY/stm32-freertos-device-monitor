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
    CMD_RESET
} CommandType_t;

/* 解析结果：命令名 + 可选参数（如 log on 里的 on） */
typedef struct
{
    CommandType_t type;
    char name[16];
    char arg[16];
    bool has_arg;
} ParsedCommand_t;

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
