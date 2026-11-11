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

/*
 * BME280_Init：初始化芯片（软复位 → 读校准系数 → 配置测量模式）
 * 参数：无
 * 返回：SENSOR_OK=成功；SENSOR_ERROR=芯片无应答/ID 不是 0x60；
 *       SENSOR_TIMEOUT=等 I2C 总线锁超时
 * 调用时机：上电后一次；连续读失败时 SensorTask 会定期重试
 */
SensorResult_t BME280_Init(void);

/*
 * BME280_Read：读一帧温湿度气压（触发一次测量 → 读原始值 → 补偿计算）
 * 参数 data：输出结构体，成功后填好 temperature_x10/humidity_x10/pressure_pa
 * 返回：同 BME280_Init 的三种结果；失败时 data 内容不可信
 */
SensorResult_t BME280_Read(SensorData_t *data);

/*
 * BME280_ChipId：直接读寄存器 0xD0 的芯片 ID
 * 返回：读到的 ID（BME280 正常是 0x60）；读失败返回 0
 * 用途：快速判断 I2C 通不通、接的是不是 BME280
 */
uint8_t BME280_ChipId(void);

/* P0 单测：探测 0x76/0x77，读 CHIP_ID（期望 0x60） */
uint8_t BME280_Probe(uint8_t *addr_7bit, uint8_t *chip_id);

#ifdef __cplusplus
}
#endif

#endif /* BME280_H */
