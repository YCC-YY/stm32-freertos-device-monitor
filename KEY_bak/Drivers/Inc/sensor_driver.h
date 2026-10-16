#ifndef SENSOR_DRIVER_H
#define SENSOR_DRIVER_H

#include <stdint.h>
#include "app_types.h"

#ifdef __cplusplus
extern "C" {
#endif

SensorResult_t Sensor_Init(void);
SensorResult_t Sensor_Read(SensorData_t *data);
SensorResult_t Sensor_GetTemperature(int16_t *temperature_x10);
SensorResult_t Sensor_GetHumidity(uint16_t *humidity_x10);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_DRIVER_H */
