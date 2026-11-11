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

/*
 * 驱动层读写结果：任务层据此决定"成功"还是"记错误"
 *
 * 谁返回它：
 *   Sensor_Init / Sensor_Read（sensor_driver.c，门面）
 *   以及 BME280_Init / BME280_Read（bme280.c，真机驱动）
 *
 * 三个值的区别（新手重点）：
 *   SENSOR_OK      成功，data 里是有效数据
 *   SENSOR_ERROR   一般性失败：芯片没应答/ID 不对/I2C 返回错误/
 *                  故障注入 FAULT_I2C 打开时也会直接返回它
 *   SENSOR_TIMEOUT 超时类失败：等 i2cMutex（I2C 总线锁）超时，
 *                  说明总线被别的任务长时间占着，或 HAL 等待超时
 * 任务层对三者的处理基本相同（计数 + 打 WARN + 定期重试 Init），
 * 分开定义是为了日志/排查时能看出到底是"坏了"还是"忙/超时"。
 */
typedef enum
{
    SENSOR_OK = 0,      /* 成功 */
    SENSOR_ERROR,       /* 一般错误（无应答/ID 不符/读写失败） */
    SENSOR_TIMEOUT      /* 超时（拿不到总线锁或 HAL 超时） */
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

/*
 * 一条结构化日志（先进 logQueue，再由 LoggerTask 打印）
 *
 * 为什么用结构体而不是直接打印字符串？
 *   - 任务只管把消息丢进队列，不会因为串口慢而卡住
 *   - LoggerTask 统一格式化输出，避免多个任务同时打印交错成乱码
 *
 * 字段说明：
 *   level      日志级别（DEBUG/INFO/WARN/ERROR），LoggerTask 会把它
 *              转成 DBG/INF/WRN/ERR 三个字母；日志关闭时 INFO 及以下不入队
 *   timestamp  产生这条日志的时刻，单位 ms（App_GetTick），
 *              打印时作为 "[   136127][WRN]" 里的时间前缀
 *   text       已经格式化好的日志正文，最长 LOG_MESSAGE_MAX_LEN（64）字节，
 *              含结尾 '\0'；例如 "Sensor read error"
 */
typedef struct
{
    LogLevel_t level;
    uint32_t   timestamp;
    char       text[LOG_MESSAGE_MAX_LEN];
} LogMessage_t;

/*
 * 任务心跳：每个任务定期"打卡"，证明自己还活着
 *
 * 机制（新手重点）：
 *   每个任务在自己的循环里，每跑一圈就把"当前毫秒时间"写进对应的字段；
 *   MonitorTask 每 2 秒读一次这四个时间戳，用
 *       "现在 - 上次打卡时间 > 5000ms"
 *   判断对应任务是不是卡死（详见 monitor_task.c 的 heartbeat_stale）。
 *
 * 为什么存"时间戳"而不是"计数"？
 *   存计数的做法只能知道"它曾经打过卡"，无法判断多久没打卡；
 *   存时间戳才能算出"已经多久没打卡了"，才能真正发现卡死。
 *
 * 字段说明（单位都是 ms，来自 App_GetTick）：
 *   sensor_heartbeat   SensorTask 的打卡时间（每 1 秒刷新）
 *   comm_heartbeat     CommTask 的打卡时间（每轮循环刷新，最长 50ms 一次）
 *   logger_heartbeat   LoggerTask 的打卡时间（每 500ms 左右刷新）
 *   monitor_heartbeat  MonitorTask 自己的打卡时间（每 2 秒刷新）
 *
 * 特殊值：0 表示"从开机起一次都没打过卡"，MonitorTask 会直接判为异常。
 *         （所以上电初期出现一次 WARNING 是正常的，任务跑起来就恢复）
 */
typedef struct
{
    uint32_t sensor_heartbeat;
    uint32_t comm_heartbeat;
    uint32_t logger_heartbeat;
    uint32_t monitor_heartbeat;
} TaskHeartbeat_t;

/*
 * 故障注入掩码：CLI 的 fault 命令用来"人为制造故障"，演示监控与容错
 *
 * 什么是位掩码（新手必读）？
 *   每个故障只占 1 个二进制位，用 1U << n 表示"把第 n 位置 1"：
 *     FAULT_SENSOR = 1U << 0 = 0x01  → 二进制 0001
 *     FAULT_COMM   = 1U << 1 = 0x02  → 二进制 0010
 *     FAULT_LOGGER = 1U << 2 = 0x04  → 二进制 0100
 *     FAULT_I2C    = 1U << 3 = 0x08  → 二进制 1000
 *   好处：用 1 个 uint32 就能同时记录多个故障，互不干扰。
 *     - 判断某故障是否打开：App_Fault_IsSet(FAULT_SENSOR)
 *       内部就是 (mask & FAULT_SENSOR) != 0
 *     - 同时打开多个：App_Fault_Set(FAULT_SENSOR | FAULT_I2C)
 *     - 全部清除：    App_Fault_Clear(0x0F) 或各业务位按位或
 *
 * 各故障的效果：
 *   FAULT_SENSOR  任务层：SensorTask 强制读失败 + 停心跳 → 触发 WARNING
 *   FAULT_COMM    任务层：CommTask 停止消费环缓（CLI 假死约 8 秒后自动恢复）
 *   FAULT_LOGGER  任务层：LoggerTask 停心跳（模拟卡死）
 *   FAULT_I2C     驱动层：BME280_Init/Read 直接返回 SENSOR_ERROR
 */
#define FAULT_NONE    0U
#define FAULT_SENSOR  (1U << 0)  /* 任务层：读失败 + 停心跳 */
#define FAULT_COMM    (1U << 1)  /* 串口处理“卡住”：不消费环缓 */
#define FAULT_LOGGER  (1U << 2)  /* 日志任务停心跳（模拟卡死） */
#define FAULT_I2C     (1U << 3)  /* 驱动层：BME280_Init/Read 返回 SENSOR_ERROR */

#endif /* APP_TYPES_H */
