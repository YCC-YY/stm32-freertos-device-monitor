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

/*
 * Sensor_Init：初始化传感器（任务启动时调用一次）
 * 返回 SENSOR_OK / SENSOR_ERROR
 * 真机：调 BME280_Init() 走 I2C（读 ID → 软复位 → 校准 → 配置）
 * 仿真：直接标记就绪，零硬件也能跑通整个框架
 */
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

/*
 * Sensor_Read：读一帧传感器数据，结果填进 data（SensorTask 每周期调用）
 * 参数 data：输出参数，成功后里面是有效数据（见 app_types.h 字段说明）
 * 返回 SENSOR_OK / SENSOR_ERROR / SENSOR_TIMEOUT
 * 注意：data 为 NULL 时直接返回错误（防御性编程，防止空指针崩溃）
 */
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
        /* 生成缓慢变化的仿真值：约 24~26°C，湿度 47~56%，气压约 101325
         * 原理：用一个 0~39 循环的计数，做成"三角波"，
         *       温度在 24.0~25.9 之间来回走，看起来像真实缓慢变化的读数
         * phase：0~39 循环；tri：先升(0~19)后降(20~39)，形成来回波
         */
        int32_t phase = (int32_t)(s_sim_counter % 40U);
        int32_t tri = (phase < 20) ? phase : (40 - phase);
        data->temperature_x10 = (int16_t)(250 + tri - 10);   /* 240~259 → 24.0~25.9°C */
        data->humidity_x10 = (uint16_t)(500 + (tri * 3));    /* 500~557 → 50.0~55.7%RH */
        data->pressure_pa = (uint32_t)(101325 + tri * 10);   /* 101325~101515 Pa */
        data->timestamp = 0;    /* 仿真数据：时间戳由任务层填写 */
        data->valid = 1;        /* 有效标志 */
    }
    return SENSOR_OK;
#else
    return BME280_Read(data);   /* 真机：走 bme280.c 的 I2C 读取 + 补偿算法 */
#endif
}

/*
 * Sensor_GetTemperature：只取温度（简化接口）
 * 参数 temperature_x10：输出指针，成功时写入"放大 10 倍的温度"（253=25.3°C）
 * 返回：SENSOR_OK / SENSOR_ERROR / SENSOR_TIMEOUT
 * 注意：内部会完整读一帧（连带湿度/气压也读了），只是把温度单独返回
 */
SensorResult_t Sensor_GetTemperature(int16_t *temperature_x10)
{
    SensorData_t d;
    SensorResult_t r = Sensor_Read(&d);
    if (r == SENSOR_OK && temperature_x10)   /* 读成功 且 输出指针非空 */
    {
        *temperature_x10 = d.temperature_x10;   /* 通过指针把结果带出去 */
    }
    return r;
}

/*
 * Sensor_GetHumidity：只取湿度（简化接口）
 * 参数 humidity_x10：输出指针，成功时写入"放大 10 倍的湿度"（521=52.1%RH）
 * 返回：SENSOR_OK / SENSOR_ERROR / SENSOR_TIMEOUT
 */
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

/*
 * Sensor_IsReady：查询传感器当前是否可用
 * 返回 1=初始化成功、可读；0=未初始化/初始化失败
 * （注意：它反映的是"初始化状态"，不代表这一刻读一定成功）
 */
uint8_t Sensor_IsReady(void)
{
    return s_ready;
}
