#include "sensor_driver.h"
#include "bme280.h"
#include "app_tasks.h"
#include "app_status.h"
#include "app_types.h"
#include "app_config.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include <string.h>

/*
 * sensor_driver.c —— 传感器门面实现
 *
 * 由 SENSOR_USE_SIMULATION（app_config.h）决定：
 *   1 = 内部造假数据（方便没接 BME280 时先跑通框架）
 *   0 = 调用 bme280.c 走真实 I2C
 *
 * 任务层不需要改任何代码，只切换这个宏即可。
 */

#if SENSOR_USE_SIMULATION
static uint32_t s_sim_counter;  /* 仿真用的相位计数，让数值缓慢变化 */
#endif
static uint8_t s_ready;

SensorResult_t Sensor_Init(void)
{
#if SENSOR_USE_SIMULATION
    s_ready = 1U;
    s_sim_counter = 0;
    return SENSOR_OK;
#else
    SensorResult_t r = BME280_Init();
    s_ready = (r == SENSOR_OK) ? 1U : 0U;
    return r;
#endif
}

SensorResult_t Sensor_Read(SensorData_t *data)
{
    if (data == NULL)
    {
        return SENSOR_ERROR;
    }

#if SENSOR_USE_SIMULATION
    if (!s_ready)
    {
        return SENSOR_ERROR;
    }
    s_sim_counter++;
    {
        /* 生成缓慢变化的仿真值：约 24~26°C，湿度 47~56%，气压约 101325 */
        int32_t phase = (int32_t)(s_sim_counter % 40U);
        int32_t tri = (phase < 20) ? phase : (40 - phase);
        data->temperature_x10 = (int16_t)(250 + tri - 10);
        data->humidity_x10 = (uint16_t)(500 + (tri * 3));
        data->pressure_pa = (uint32_t)(101325 + tri * 10);
        data->timestamp = 0;
        data->valid = 1;
    }
    return SENSOR_OK;
#else
    return BME280_Read(data);
#endif
}

SensorResult_t Sensor_GetTemperature(int16_t *temperature_x10)
{
    SensorData_t d;
    SensorResult_t r = Sensor_Read(&d);
    if (r == SENSOR_OK && temperature_x10)
    {
        *temperature_x10 = d.temperature_x10;
    }
    return r;
}

SensorResult_t Sensor_GetHumidity(uint16_t *humidity_x10)
{
    SensorData_t d;
    SensorResult_t r = Sensor_Read(&d);
    if (r == SENSOR_OK && humidity_x10)
    {
        *humidity_x10 = d.humidity_x10;
    }
    return r;
}

uint8_t Sensor_IsReady(void)
{
    return s_ready;
}
