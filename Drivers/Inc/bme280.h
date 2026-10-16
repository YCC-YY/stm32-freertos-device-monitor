#ifndef BME280_H
#define BME280_H

/*
 * bme280.h —— BME280 芯片驱动对外接口
 *
 * 普通代码请调用 sensor_driver.h，不要直接用 BME280_*，
 * 以便以后换传感器时少改任务层。
 */

#include <stdint.h>
#include "app_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* BME280 on I2C2: SCL=PB10, SDA=PB11. Address 0x76 (SDO=GND) or 0x77 (SDO=VDD). */
#ifndef BME280_I2C_ADDR
#define BME280_I2C_ADDR  (0x76U << 1) /* HAL 要求 7bit 地址左移 1 位 */
#endif

SensorResult_t BME280_Init(void);
SensorResult_t BME280_Read(SensorData_t *data);
uint8_t BME280_ChipId(void);

#ifdef __cplusplus
}
#endif

#endif /* BME280_H */

