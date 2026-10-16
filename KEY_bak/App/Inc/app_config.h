#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>

/* ---- Timing ---- */
#define SENSOR_PERIOD_MS          1000U
#define MONITOR_PERIOD_MS         2000U
#define COMM_POLL_MS              5U
#define STATUS_HEARTBEAT_PRINT_MS 10000U

/* ---- Queues / buffers ---- */
#define SENSOR_QUEUE_LENGTH       8U
#define LOG_QUEUE_LENGTH          16U
#define UART_RX_BUFFER_SIZE       128U
#define CLI_MAX_COMMAND_LENGTH    64U
#define LOG_MESSAGE_MAX_LEN       64U

/* ---- Health ---- */
#define HEARTBEAT_TIMEOUT_MS      5000U
#define SENSOR_ERROR_WARN_LIMIT   5U

/* ---- CLI ---- */
#define CLI_PROMPT                "> "
#define UART_BAUDRATE             115200U

/* ---- Task priorities (relative) ---- */
#define PRIORITY_LOGGER           1U
#define PRIORITY_SENSOR           2U
#define PRIORITY_MONITOR          2U
#define PRIORITY_COMM             3U

/* ---- Stack words (adjust with high-water mark) ---- */
#define STACK_SENSOR              256U
#define STACK_COMM                256U
#define STACK_LOGGER              256U
#define STACK_MONITOR             256U

#endif /* APP_CONFIG_H */
