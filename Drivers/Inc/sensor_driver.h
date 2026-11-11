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

/*
 * Sensor_Init：初始化传感器（真机走 BME280_Init，仿真模式下直接成功）
 * 返回：SENSOR_OK / SENSOR_ERROR / SENSOR_TIMEOUT
 */
SensorResult_t Sensor_Init(void);

/*
 * Sensor_Read：读一帧数据（任务层最常用的函数）
 * 参数 data：输出结构体，成功后 valid=1 且字段有效
 * 返回：SENSOR_OK / SENSOR_ERROR / SENSOR_TIMEOUT
 */
SensorResult_t Sensor_Read(SensorData_t *data);

/*
 * Sensor_GetTemperature：只取温度
 * 参数 temperature_x10：输出指针，253 表示 25.3°C（放大 10 倍避免小数）
 * 返回：同 Sensor_Read
 */
SensorResult_t Sensor_GetTemperature(int16_t *temperature_x10);

/*
 * Sensor_GetHumidity：只取湿度
 * 参数 humidity_x10：输出指针，521 表示 52.1%RH
 * 返回：同 Sensor_Read
 */
SensorResult_t Sensor_GetHumidity(uint16_t *humidity_x10);

/*
 * Sensor_IsReady：查询初始化是否成功
 * 返回：1=可以读；0=未初始化或初始化失败
 */
uint8_t Sensor_IsReady(void);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_DRIVER_H */
