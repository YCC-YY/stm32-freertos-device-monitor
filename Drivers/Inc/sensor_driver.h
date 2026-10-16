#ifndef SENSOR_DRIVER_H
#define SENSOR_DRIVER_H

/*
 * sensor_driver.h —— 传感器“门面”
 *
 * 任务层只调用这几个函数，不关心底下是 BME280 还是仿真：
 *   Sensor_Init()
 *   Sensor_Read()   ← 最常用
 *   Sensor_GetTemperature() / Sensor_GetHumidity()
 *
 * 以后换别的传感器芯片，尽量只改 sensor_driver.c / bme280.c。
 */

#include <stdint.h>
#include "app_types.h"

#ifdef __cplusplus
extern "C" {
#endif

SensorResult_t Sensor_Init(void);
SensorResult_t Sensor_Read(SensorData_t *data);
SensorResult_t Sensor_GetTemperature(int16_t *temperature_x10);
SensorResult_t Sensor_GetHumidity(uint16_t *humidity_x10);
uint8_t Sensor_IsReady(void);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_DRIVER_H */
