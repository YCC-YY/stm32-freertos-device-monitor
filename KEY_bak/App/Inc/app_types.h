#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    SYSTEM_INIT = 0,
    SYSTEM_RUNNING,
    SYSTEM_WARNING,
    SYSTEM_ERROR
} SystemState_t;

typedef enum
{
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} LogLevel_t;

typedef enum
{
    SENSOR_OK = 0,
    SENSOR_ERROR,
    SENSOR_TIMEOUT
} SensorResult_t;

typedef struct
{
    uint32_t timestamp;
    int16_t  temperature_x10; /* 0.1 degC units, e.g. 253 == 25.3C */
    uint16_t humidity_x10;    /* 0.1 %RH units */
    uint16_t light;           /* raw / lux-ish */
    uint8_t  valid;
} SensorData_t;

typedef struct
{
    LogLevel_t level;
    uint32_t   timestamp;
    char       text[LOG_MESSAGE_MAX_LEN];
} LogMessage_t;

typedef struct
{
    uint32_t sensor_heartbeat;
    uint32_t comm_heartbeat;
    uint32_t logger_heartbeat;
    uint32_t monitor_heartbeat;
} TaskHeartbeat_t;

#endif /* APP_TYPES_H */
