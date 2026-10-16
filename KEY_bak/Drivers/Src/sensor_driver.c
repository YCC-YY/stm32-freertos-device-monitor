#include "sensor_driver.h"
#include "app_tasks.h"
#include "app_status.h"
#include "app_types.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include <string.h>

/*
 * Board-agnostic sensor driver.
 * - Replace the body of Sensor_Read() with your real I2C transaction
 *   (HTS221 / SHT3x / AHT2x / BMP280 ...).
 * - I2C bus is protected by i2cMutex (multi-task safety).
 * - Without hardware, returns a plausible simulation so CLI/queues can be exercised.
 */

#ifndef SENSOR_USE_SIMULATION
#define SENSOR_USE_SIMULATION 1
#endif

static uint32_t s_sim_counter;
static uint8_t s_ready;

static SensorResult_t sensor_bus_lock(TickType_t *woken)
{
    TickType_t start = xTaskGetTickCount();
    if (i2cMutex == NULL)
    {
        return SENSOR_OK;
    }
    if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(50)) != pdTRUE)
    {
        return SENSOR_TIMEOUT;
    }
    if (woken)
    {
        *woken = xTaskGetTickCount() - start;
    }
    return SENSOR_OK;
}

static void sensor_bus_unlock(void)
{
    if (i2cMutex != NULL)
    {
        xSemaphoreGive(i2cMutex);
    }
}

SensorResult_t Sensor_Init(void)
{
#if SENSOR_USE_SIMULATION
    s_ready = 1U;
    s_sim_counter = 0;
    return SENSOR_OK;
#else
    if (sensor_bus_lock(NULL) != SENSOR_OK)
    {
        return SENSOR_TIMEOUT;
    }
    /* TODO: HAL_I2C_Mem_Read WHO_AM_I / soft reset */
    sensor_bus_unlock();
    s_ready = 1U;
    return SENSOR_OK;
#endif
}

SensorResult_t Sensor_Read(SensorData_t *data)
{
    if (data == NULL)
    {
        return SENSOR_ERROR;
    }
    if (!s_ready)
    {
        return SENSOR_ERROR;
    }

    if (sensor_bus_lock(NULL) != SENSOR_OK)
    {
        return SENSOR_TIMEOUT;
    }

#if SENSOR_USE_SIMULATION
    s_sim_counter++;
    /* Gentle sine-like variation without libm: triangle around 25.0C / 50%RH */
    {
        int32_t phase = (int32_t)(s_sim_counter % 40U);
        int32_t tri = (phase < 20) ? phase : (40 - phase); /* 0..20 */
        data->temperature_x10 = (int16_t)(250 + tri - 10);  /* 24.0 .. 26.0 */
        data->humidity_x10 = (uint16_t)(500 + (tri * 3));   /* ~47..56 */
        data->light = (uint16_t)(300 + tri * 10);
        data->timestamp = 0;
        data->valid = 1;
    }
    sensor_bus_unlock();
    return SENSOR_OK;
#else
    /* TODO: real register read; on NACK return SENSOR_ERROR */
    sensor_bus_unlock();
    return SENSOR_ERROR;
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
