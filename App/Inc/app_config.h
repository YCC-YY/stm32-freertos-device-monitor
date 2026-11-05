#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/*
 * app_config.h —— 项目“可调参数”集中地
 *
 * 新手提示：
 *   - 想改采集周期、队列深度、任务栈大小，改这个文件即可
 *   - 不要把逻辑写在这里，这里只有宏常量
 *   - BME280 模块没到货时用仿真；到货后把 SENSOR_USE_SIMULATION 改成 0
 */

#include <stdint.h>

/* ========== 1. 时间周期（单位：毫秒） ========== */
#define SENSOR_PERIOD_MS          1000U   /* 传感器多久采一次 */
#define MONITOR_PERIOD_MS         2000U   /* 监控任务多久检查一次 */
#define COMM_POLL_MS              5U      /* 串口任务空转间隔（后续可改成通知唤醒） */
#define STATUS_HEARTBEAT_PRINT_MS 10000U

/* ========== 2. 队列与缓冲区大小 ========== */
#define SENSOR_QUEUE_LENGTH       8U      /* 传感器数据队列最多暂存几条 */
#define LOG_QUEUE_LENGTH          16U     /* 日志队列最多暂存几条 */
#define UART_RX_BUFFER_SIZE       128U    /* 串口接收环形缓冲字节数 */
#define CLI_MAX_COMMAND_LENGTH    64U     /* 一行命令最大长度 */
#define LOG_MESSAGE_MAX_LEN       64U     /* 单条日志文本最大长度 */

/* ========== 3. 健康监控 ========== */
#define HEARTBEAT_TIMEOUT_MS      5000U   /* 心跳超过 5 秒没更新则告警 */
#define SENSOR_ERROR_WARN_LIMIT   5U      /* 传感器错误累计多少次进入 WARNING */

/* ========== 4. 命令行 ========== */
#define CLI_PROMPT                "> "    /* 提示符（系统已经就绪，可以输入命令） */
#define UART_BAUDRATE             115200U
/* 串口工具若不自动补 \r\n，静默多久把当前行当命令提交 */
#define CLI_LINE_TIMEOUT_MS       400U

/* ========== 5. 传感器后端选择 ========== */
/*
 * 1 = 软件仿真（不用接硬件，方便先调框架）
 * 0 = 真实 BME280（I2C2，SCL=PB10，SDA=PB11）
 */
#ifndef SENSOR_USE_SIMULATION
#define SENSOR_USE_SIMULATION     0   /* 0=真实 BME280（已 chipid+bmeread 验证） */
#endif

/* ========== 6. 任务优先级（数字越大优先级越高） ========== */
#define PRIORITY_LOGGER           1U
#define PRIORITY_SENSOR           2U
#define PRIORITY_MONITOR          2U
#define PRIORITY_COMM             3U      /* 命令响应优先，避免用户觉得卡 */

/* ========== 7. 任务栈大小（单位：FreeRTOS 的 word，1 word = 4 字节） ========== */
#define STACK_SENSOR              256U
#define STACK_COMM                256U
#define STACK_LOGGER              256U
#define STACK_MONITOR             256U

#endif /* APP_CONFIG_H */
