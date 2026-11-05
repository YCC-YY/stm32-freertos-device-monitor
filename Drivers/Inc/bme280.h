#ifndef BME280_H
#define BME280_H

/*
 * bme280.h —— BME280 芯片驱动对外接口
 *
 * 普通代码请调用 sensor_driver.h；本头文件给 I2C 单测 / 驱动层用。
 */

#include <stdint.h>
#include "app_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* HAL 用 8bit 地址 = 7bit 地址左移 1 */
#define BME280_ADDR_76   (0x76U << 1)  /* SDO 接地 */
#define BME280_ADDR_77   (0x77U << 1)  /* SDO 接 3.3V */

#ifndef BME280_I2C_ADDR
#define BME280_I2C_ADDR  BME280_ADDR_76
#endif

SensorResult_t BME280_Init(void);
SensorResult_t BME280_Read(SensorData_t *data);
uint8_t BME280_ChipId(void);

/* P0 单测：探测 0x76/0x77，读 CHIP_ID（期望 0x60） */
uint8_t BME280_Probe(uint8_t *addr_7bit, uint8_t *chip_id);

#ifdef __cplusplus
}
#endif

#endif /* BME280_H */
