/*
 * bme280.c —— BME280 温湿度气压传感器驱动（I2C）
 *
 * 接线（本项目）：
 *   PB10 = I2C2_SCL
 *   PB11 = I2C2_SDA
 *   模块地址：0x76（SDO 接地）或 0x77（SDO 接 3.3V），见 bme280.h
 *
 * 新手提示：
 *   1. 先读芯片 ID（0xD0 应为 0x60），确认通信成功
 *   2. 再读校准系数（出厂标定），芯片温度/湿度/气压要靠这些系数换算
 *   3. 最后读原始 ADC，用 datasheet 公式补偿成可读数值
 *   I2C 总线用 i2cMutex 保护，防止多任务同时访问。
 */

#include "bme280.h"
#include "i2c.h"
#include "app_tasks.h"
#include "app_status.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include <string.h>

/*
 * Bosch BME280 driver (I2C, blocking HAL + i2cMutex).
 * Pins (this board): PB10 = I2C2_SCL, PB11 = I2C2_SDA.
 * Compensation math follows Bosch datasheet integer formulas.
 */

#define BME280_REG_ID          0xD0U
#define BME280_REG_RESET       0xE0U
#define BME280_REG_STATUS      0xF3U
#define BME280_REG_CTRL_HUM    0xF2U
#define BME280_REG_CTRL_MEAS   0xF4U
#define BME280_REG_CONFIG      0xF5U
#define BME280_REG_PRESS_MSB   0xF7U
#define BME280_REG_CALIB00     0x88U
#define BME280_REG_CALIB26     0xE1U

#define BME280_CHIP_ID         0x60U   /* BME280 芯片 ID */
#define BME280_RESET_VALUE     0xB6U   /* 软复位魔数 */
#define BME280_TIMEOUT_MS      50U     /* 单次 I2C 超时 */

typedef struct
{
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4;
    int16_t  dig_H5;
    int8_t   dig_H6;
} BME280_Calib_t;

static BME280_Calib_t s_calib;   /* 出厂校准系数，Init 时读入 */
static int32_t s_t_fine;         /* 温度补偿中间量 t_fine */
static uint8_t s_ready;          /* 1=初始化成功 */

/* 拿 I2C 总线锁：同一时刻只允许一个任务访问 I2C2（防数据打架） */
static SensorResult_t bus_lock(void)
{
    if (i2cMutex == NULL)
    {
        return SENSOR_OK;
    }
    if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(BME280_TIMEOUT_MS)) != pdTRUE)
    {
        return SENSOR_TIMEOUT;
    }
    return SENSOR_OK;
}

/* 放锁：别的任务可以继续用 I2C 总线 */
static void bus_unlock(void)
{
    if (i2cMutex != NULL)
    {
        xSemaphoreGive(i2cMutex);
    }
}

/*
 * write_reg：向 BME280 的某个寄存器写 1 字节（I2C 内存写）
 * reg ：寄存器地址（如 0xF4 控制寄存器）
 * val ：要写入的值
 * 返回 SENSOR_OK / SENSOR_ERROR
 * 前置：调用者已持有 i2cMutex（bus_lock）
 */
static SensorResult_t write_reg(uint8_t reg, uint8_t val)
{
    HAL_StatusTypeDef st = HAL_I2C_Mem_Write(&hi2c2,
                                             BME280_I2C_ADDR,
                                             reg,
                                             I2C_MEMADD_SIZE_8BIT,
                                             &val,
                                             1,
                                             BME280_TIMEOUT_MS);
    return (st == HAL_OK) ? SENSOR_OK : SENSOR_ERROR;
}

/*
 * read_regs：从 BME280 连续读 len 个字节（I2C 内存读）
 * reg ：起始寄存器地址
 * buf ：读到的数据放这里
 * len ：要读的字节数
 * 返回 SENSOR_OK / SENSOR_ERROR
 * 前置：调用者已持有 i2cMutex（bus_lock）
 */
static SensorResult_t read_regs(uint8_t reg, uint8_t *buf, uint16_t len)
{
    HAL_StatusTypeDef st = HAL_I2C_Mem_Read(&hi2c2,
                                            BME280_I2C_ADDR,
                                            reg,
                                            I2C_MEMADD_SIZE_8BIT,
                                            buf,
                                            len,
                                            BME280_TIMEOUT_MS);
    return (st == HAL_OK) ? SENSOR_OK : SENSOR_ERROR;
}

/*
 * load_calib：读取 BME280 出厂校准系数（每个芯片不同）
 * 温度校准 12 字节（0x88 起）+ 湿度校准 7 字节（0xE1 起），共 19 项。
 * 这些系数存进 s_calib，后续补偿算法全靠它们，读错则数值全偏。
 * 返回 SENSOR_OK / SENSOR_ERROR
 */
static SensorResult_t load_calib(void)
{
    uint8_t c[26];
    uint8_t h[7];

    if (read_regs(BME280_REG_CALIB00, c, 26) != SENSOR_OK)
    {
        return SENSOR_ERROR;
    }

    s_calib.dig_T1 = (uint16_t)(c[1] << 8 | c[0]);
    s_calib.dig_T2 = (int16_t)(c[3] << 8 | c[2]);
    s_calib.dig_T3 = (int16_t)(c[5] << 8 | c[4]);
    s_calib.dig_P1 = (uint16_t)(c[7] << 8 | c[6]);
    s_calib.dig_P2 = (int16_t)(c[9] << 8 | c[8]);
    s_calib.dig_P3 = (int16_t)(c[11] << 8 | c[10]);
    s_calib.dig_P4 = (int16_t)(c[13] << 8 | c[12]);
    s_calib.dig_P5 = (int16_t)(c[15] << 8 | c[14]);
    s_calib.dig_P6 = (int16_t)(c[17] << 8 | c[16]);
    s_calib.dig_P7 = (int16_t)(c[19] << 8 | c[18]);
    s_calib.dig_P8 = (int16_t)(c[21] << 8 | c[20]);
    s_calib.dig_P9 = (int16_t)(c[23] << 8 | c[22]);
    s_calib.dig_H1 = c[25];

    if (read_regs(BME280_REG_CALIB26, h, 7) != SENSOR_OK)
    {
        return SENSOR_ERROR;
    }
    s_calib.dig_H2 = (int16_t)(h[1] << 8 | h[0]);
    s_calib.dig_H3 = h[2];
    s_calib.dig_H4 = (int16_t)((h[3] << 4) | (h[4] & 0x0FU));
    s_calib.dig_H5 = (int16_t)((h[5] << 4) | (h[4] >> 4));
    s_calib.dig_H6 = (int8_t)h[6];
    return SENSOR_OK;
}

/*
 * comp_temp：原始温度 ADC → 实际温度（单位 0.01°C）
 * 算法来自 Bosch datasheet 整数公式，必须按顺序算；
 * 中间量 t_fine 存到全局，气压/湿度补偿也要用到它。
 * 返回：如 2530 = 25.30°C
 */
static int32_t comp_temp(int32_t adc_T)
{
    int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((int32_t)s_calib.dig_T1 << 1))) * ((int32_t)s_calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)s_calib.dig_T1)) * ((adc_T >> 4) - ((int32_t)s_calib.dig_T1))) >> 12) *
            ((int32_t)s_calib.dig_T3)) >> 14;
    s_t_fine = var1 + var2;
    T = (s_t_fine * 5 + 128) >> 8;
    return T; /* 0.01 degC */
}

/*
 * comp_press：原始气压 ADC → 实际气压（单位 Pa）
 * 依赖 comp_temp 算好的 s_t_fine，返回约 101325（标准大气压附近）
 */
static uint32_t comp_press(int32_t adc_P)
{
    int64_t var1, var2, p;
    var1 = ((int64_t)s_t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)s_calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)s_calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)s_calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)s_calib.dig_P3) >> 8) + ((var1 * (int64_t)s_calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)s_calib.dig_P1) >> 33;
    if (var1 == 0)
    {
        return 0;
    }
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)s_calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)s_calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)s_calib.dig_P7) << 4);
    /* Bosch: Q24.8 Pa; >>8 yields Pa. */
    return (uint32_t)(p >> 8);
}

/*
 * comp_hum：原始湿度 ADC → 实际湿度（单位 0.01%RH）
 * 依赖 s_t_fine；返回如 5210 = 52.10%RH
 */
static uint32_t comp_hum(int32_t adc_H)
{
    int32_t v_x1_u32r;
    v_x1_u32r = (s_t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)s_calib.dig_H4) << 20) - (((int32_t)s_calib.dig_H5) * v_x1_u32r)) +
                   ((int32_t)16384)) >> 15) *
                 (((((((v_x1_u32r * ((int32_t)s_calib.dig_H6)) >> 10) *
                      (((v_x1_u32r * ((int32_t)s_calib.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
                    ((int32_t)2097152)) * ((int32_t)s_calib.dig_H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)s_calib.dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
    v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
    /* Q22.10: /1024 = %RH. Return 0.01%RH units. */
    return (uint32_t)(((uint32_t)(v_x1_u32r >> 12)) * 100U / 1024U);
}

uint8_t BME280_ChipId(void)
{
    uint8_t id = 0;
    if (read_regs(BME280_REG_ID, &id, 1) != SENSOR_OK)
    {
        return 0;
    }
    return id;
}

/*
 * P0 通信单测：依次试 0x76 / 0x77
 * 成功：*addr_7bit = 7 位地址，*chip_id = 期望 0x60，返回 1
 * 失败：返回 0（无应答或 ID 不对）
 */
uint8_t BME280_Probe(uint8_t *addr_7bit, uint8_t *chip_id)
{
    static const uint8_t addrs[2] = { 0x76U, 0x77U };
    uint8_t i;
    uint8_t id = 0;
    uint8_t tmp;

    if (addr_7bit == NULL || chip_id == NULL)
    {
        return 0;
    }

    for (i = 0; i < 2U; i++)
    {
        /* 只探测总线是否 ACK，不改动全局 BME280_I2C_ADDR */
        if (HAL_I2C_IsDeviceReady(&hi2c2, (uint16_t)(addrs[i] << 1), 2, 50) != HAL_OK)
        {
            continue;
        }

        if (HAL_I2C_Mem_Read(&hi2c2,
                             (uint16_t)(addrs[i] << 1),
                             BME280_REG_ID,
                             I2C_MEMADD_SIZE_8BIT,
                             &tmp,
                             1,
                             BME280_TIMEOUT_MS) != HAL_OK)
        {
            continue;
        }

        id = tmp;
        *addr_7bit = addrs[i];
        *chip_id = id;
        return 1;
    }

    return 0;
}

/*
 * BME280_Init：完整初始化 BME280
 * 流程：读芯片 ID 校验(应为 0x60) → 软复位 → 读校准系数 → 配置过采样/工作模式
 * 成功置 s_ready=1；失败返回 SENSOR_ERROR，调用方(传感器任务)会定期重试
 */
SensorResult_t BME280_Init(void)
{
    uint8_t id;
    SensorResult_t r;

    s_ready = 0;

    /* 驱动层故障注入：模拟 Init/总线异常（不等同真实 HAL 超时） */
    if (App_Fault_IsSet(FAULT_I2C))
    {
        return SENSOR_ERROR;
    }

    if (bus_lock() != SENSOR_OK)
    {
        return SENSOR_TIMEOUT;
    }

    r = read_regs(BME280_REG_ID, &id, 1);
    if (r != SENSOR_OK || id != BME280_CHIP_ID)
    {
        bus_unlock();
        return (r != SENSOR_OK) ? SENSOR_ERROR : SENSOR_ERROR;
    }

    /* Soft reset, wait status clear */
    if (write_reg(BME280_REG_RESET, BME280_RESET_VALUE) != SENSOR_OK)
    {
        bus_unlock();
        return SENSOR_ERROR;
    }
    vTaskDelay(pdMS_TO_TICKS(5));

    if (load_calib() != SENSOR_OK)
    {
        bus_unlock();
        return SENSOR_ERROR;
    }

    /* humidity oversampling x1, temp x1, press x1, normal mode */
    if (write_reg(BME280_REG_CTRL_HUM, 0x01U) != SENSOR_OK)
    {
        bus_unlock();
        return SENSOR_ERROR;
    }
    if (write_reg(BME280_REG_CONFIG, 0x00U) != SENSOR_OK)
    {
        bus_unlock();
        return SENSOR_ERROR;
    }
    if (write_reg(BME280_REG_CTRL_MEAS, 0x27U) != SENSOR_OK) /* osrs_t=1 osrs_p=1 mode=normal */
    {
        bus_unlock();
        return SENSOR_ERROR;
    }

    s_ready = 1;
    bus_unlock();
    return SENSOR_OK;
}

/*
 * BME280_Read：读一次完整的温/湿/压
 * 流程：一次读 8 字节原始 ADC → 拆出 P/T/H 三个原始值
 *       → 依次做温度/气压/湿度补偿 → 填进 data（单位见 app_types.h）
 * 前置：BME280_Init 成功过(s_ready=1)
 */
SensorResult_t BME280_Read(SensorData_t *data)
{
    uint8_t raw[8];
    int32_t adc_P, adc_T, adc_H;
    int32_t t01;
    uint32_t p_q24, h_x100;

    if (data == NULL || !s_ready)
    {
        return SENSOR_ERROR;
    }

    /* 驱动层故障注入：Read 直接失败，验证上层错误计数/re-init 策略 */
    if (App_Fault_IsSet(FAULT_I2C))
    {
        return SENSOR_ERROR;
    }

    if (bus_lock() != SENSOR_OK)
    {
        return SENSOR_TIMEOUT;
    }

    if (read_regs(BME280_REG_PRESS_MSB, raw, 8) != SENSOR_OK)
    {
        bus_unlock();
        return SENSOR_ERROR;
    }

    bus_unlock();

    adc_P = (int32_t)((uint32_t)raw[0] << 12 | (uint32_t)raw[1] << 4 | (uint32_t)raw[2] >> 4);
    adc_T = (int32_t)((uint32_t)raw[3] << 12 | (uint32_t)raw[4] << 4 | (uint32_t)raw[5] >> 4);
    adc_H = (int32_t)((uint32_t)raw[6] << 8 | raw[7]);

    t01 = comp_temp(adc_T);                 /* 0.01 C */
    data->temperature_x10 = (int16_t)(t01 / 10); /* 0.1 C */

    p_q24 = comp_press(adc_P);              /* Pa (integer path) */
    data->pressure_pa = p_q24;

    h_x100 = comp_hum(adc_H);               /* 0.01 %RH path */
    data->humidity_x10 = (uint16_t)((h_x100 + 5U) / 10U); /* 0.1 %RH */

    data->valid = 1;
    return SENSOR_OK;
}
