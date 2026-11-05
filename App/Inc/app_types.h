#ifndef APP_TYPES_H
#define APP_TYPES_H

/*
 * app_types.h —— 项目公共数据类型（“词汇表”）
 *
 * 新手提示：
 *   - 整个项目提到“传感器数据”“日志”“系统状态”时，用的都是这里定义的结构体/枚举
 *   - 改这里会影响所有文件，改之前想清楚字段含义
 */

#include <stdint.h>
#include <stdbool.h>
#include "app_config.h"

/* 系统大状态：监控任务会根据错误情况切换 */
typedef enum
{
    SYSTEM_INIT = 0,   /* 正在启动 */
    SYSTEM_RUNNING,    /* 正常运行 */
    SYSTEM_WARNING,    /* 有异常但仍可工作（如心跳超时、错误偏多） */
    SYSTEM_ERROR       /* 严重错误 */
} SystemState_t;

/* 日志级别：数字越大越严重 */
typedef enum
{
    LOG_DEBUG = 0,     /* 调试细节，可关 */
    LOG_INFO,          /* 一般信息 */
    LOG_WARN,          /* 警告 */
    LOG_ERROR          /* 错误 */
} LogLevel_t;

/* 驱动层读写结果：任务层据此决定成功还是记错误 */
typedef enum
{
    SENSOR_OK = 0,
    SENSOR_ERROR,
    SENSOR_TIMEOUT
} SensorResult_t;

/*
 * 一次传感器采样结果（SensorTask 填好，别人来读）
 *
 * 字段说明：
 *   timestamp        采样时刻，单位 ms（App_GetTick）
 *   temperature_x10  温度放大 10 倍：253 → 25.3°C，负温度用负数
 *   humidity_x10     湿度放大 10 倍：521 → 52.1%RH
 *   pressure_pa      气压，单位帕斯卡（BME280），约 101325 = 标准大气压
 *   valid            1=本结构体内容有效；0=还没采到过
 */
typedef struct
{
    uint32_t timestamp;
    int16_t  temperature_x10;
    uint16_t humidity_x10;
    uint32_t pressure_pa;
    uint8_t  valid;
} SensorData_t;

/* 一条结构化日志（先进 logQueue，再由 LoggerTask 打印） */
typedef struct
{
    LogLevel_t level;
    uint32_t   timestamp;
    char       text[LOG_MESSAGE_MAX_LEN];
} LogMessage_t;

/*
 * 任务心跳：每个任务定期“打卡”
 * 监控任务比较当前时间与上次打卡时间，判断有没有卡死
 */
typedef struct
{
    uint32_t sensor_heartbeat;
    uint32_t comm_heartbeat;
    uint32_t logger_heartbeat;
    uint32_t monitor_heartbeat;
} TaskHeartbeat_t;

/* 故障注入掩码：CLI fault 命令使用，便于演示监控与容错 */
#define FAULT_NONE    0U
#define FAULT_SENSOR  (1U << 0)  /* 传感器读失败 + 停心跳 */
#define FAULT_COMM    (1U << 1)  /* 串口处理“卡住”：不消费环缓 */
#define FAULT_LOGGER  (1U << 2)  /* 日志任务停心跳（模拟卡死） */

#endif /* APP_TYPES_H */
