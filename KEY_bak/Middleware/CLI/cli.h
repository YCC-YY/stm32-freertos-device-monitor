#ifndef CLI_H
#define CLI_H

#include <stdint.h>
#include <stdbool.h>
#include "app_config.h"

#ifdef __cplusplus
extern "C" {
#endif

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

/* Feed one RX byte (from RingBuffer drain). Returns true when a full line is ready. */
bool CLI_InputChar(char c);

/* Copy completed line; returns CLI_OK if a line is pending. */
CLIResult_t CLI_ReadLine(char *out, uint16_t out_size);

void CLI_ParseCommand(const char *line, ParsedCommand_t *cmd);
void CLI_Execute(const char *line);
void CLI_PrintHelp(void);

/* Push prompt + string into log path via UART driver (CLI uses logger indirectly). */
void CLI_Printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* CLI_H */
