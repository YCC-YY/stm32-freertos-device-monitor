#include "sensor_task.h"
#include "app_tasks.h"
#include "app_status.h"
#include "app_config.h"
#include "sensor_driver.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <string.h>

static TaskHandle_t s_sensor_handle = NULL;

void SensorTask_Start(void)
{
    if (xTaskCreate(SensorTask_Function,
                    "SensorTask",
                    STACK_SENSOR,
                    NULL,
                    PRIORITY_SENSOR,
                    &s_sensor_handle) != pdPASS)
    {
        for (;;)
        {
        }
    }
}

void SensorTask_Function(void *argument)
{
    SensorData_t data;
    (void)argument;

    if (Sensor_Init() == SENSOR_OK)
    {
        App_Status_SetSensorReady(1U);
        LOG_INFO("Sensor init OK");
    }
    else
    {
        App_Status_SetSensorReady(0U);
        LOG_ERROR("Sensor init failed");
    }

    for (;;)
    {
        uint32_t now = App_GetTick();
        App_Status_BeatSensor(now);

        memset(&data, 0, sizeof(data));

        if (Sensor_Read(&data) == SENSOR_OK)
        {
            data.timestamp = now;
            data.valid = 1U;
            App_Status_SetLatestSensor(&data);

            if (sensorQueue != NULL)
            {
                if (xQueueSend(sensorQueue, &data, 0) != pdTRUE)
                {
                    App_Status_RecordSensorQueueFull();
                }
            }
        }
        else
        {
            App_Status_RecordSensorError();
            LOG_WARN("Sensor read error");
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_PERIOD_MS));
    }
}
