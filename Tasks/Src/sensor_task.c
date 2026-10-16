#include "sensor_task.h"
#include "app_tasks.h"
#include "app_status.h"
#include "app_config.h"
#include "sensor_driver.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <string.h>

/*
 * sensor_task.c —— 传感器采集任务
 *
 * 每隔 SENSOR_PERIOD_MS 做一次：
 *   1. 更新心跳（告诉监控任务“我还活着”）
 *   2. Sensor_Read() 读一帧数据
 *   3. 成功：记最新值，并尝试放进 sensorQueue
 *   4. 失败：错误计数 +1，打一条 WARN 日志
 *
 * 设计要点：任务不碰 I2C 寄存器，只调用 sensor_driver。
 */

/* s_sensor_handle是当前文件内部的静态变量
 * TaskHandle_t 是 FreeRTOS 实时操作系统里定义的任务句柄类型，用来标识和操作一个任务 ——> 通过它暂停、删除任务，或者获取任务状态
 * 初始化为 NULL，意思是一开始这个任务句柄还没有指向任何实际的任务
 */
static TaskHandle_t s_sensor_handle = NULL; //初始化


/* 给 CLI/监控用：拿到本任务句柄（查栈水位等），是一个对外的接口函数
 * 返回 s_sensor_handle 的值，相当于给外部一个 “访问通道”，这样其他模块就能通过调用这个函数拿到任务句柄了。
 */
TaskHandle_t SensorTask_Handle(void)
{
    return s_sensor_handle;
}

/* xTaskCreate创建任务；栈和优先级在 app_config.h 里配置
 * SensorTask_Function任务函数
 * SensorTask任务名
 * STACK_SENSOR（栈深度也就是栈空间大小）,NULL（所需传递的参数，由于没有参数所以为NULL）
 * PRIORITY_SENSOR传感器任务优先级值
 *
 * xTaskCreate函数成功时返回 pdPASS（任务创建成功的返回值），这里将任务句柄地址传入，若返回值不等于 pdPASS，说明任务创建失败，进入 for (;;)死循环，避免错误执行后续代码
 */
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

/* 生产者任务
 * 任务主体：FreeRTOS 会把这个函数当成一个死循环线程来跑 */
void SensorTask_Function(void *argument)
{
    SensorData_t data;  /* 本帧采样结果
                           定义传感器数据结构体变量 data*/

    (void)argument;     /* FreeRTOS 固定参数，本任务不用 */

    /* 启动时初始化传感器（真实 BME280 或仿真，见 app_config.h） */
    //初始化传感器，若成功进入分支
    if (Sensor_Init() == SENSOR_OK)
    {
        /* 设置传感器就绪状态为 1，记录初始化成功日志
         * 1U  表示传感器初始化成功，设置其就绪状态为 “已就绪”
         * 0U  表示初始化失败时设置为 “未就绪”
         */
        App_Status_SetSensorReady(1U);
        /* 记录初始化成功日志 */
        LOG_INFO("Sensor init OK");
    }
    else
    {
        /* 设置传感器就绪状态为 0 */
        App_Status_SetSensorReady(0U);
        //记录失败日志
        LOG_ERROR("Sensor init failed");
    }

    for (;;)
    {
        /* 获取当前时间 */
        uint32_t now = App_GetTick();

        /* 心跳打卡
        * 进行传感器心跳打卡(把当前的时间戳`now`，更新到全局状态里的`sensor_heartbeat`字段，相当于 SensorTask 说一句 “我还活着，没卡死”。
          后面 MonitorTask 每 2 秒会检查一次所有任务的心跳时间，如果发现某个任务的心跳时间超过 5 秒没更新，就判定任务卡死了，输出警告日志。)
         */
        App_Status_BeatSensor(now);//更新传感器任务的心跳时间戳；

        /* 清空传感器数据结构体，做初始化清理 */
        memset(&data, 0, sizeof(data));

        if (Sensor_Read(&data) == SENSOR_OK)
        {
            /*（timestamp）采样时刻 */
            data.timestamp = now;
            /* Valid 判断是否采集到有效的结构体内容；1U为采集到有效结构体内容，0U则代表未采集到有效结构体内容 */
            data.valid = 1U;

            /* 缓存一份最新数据到全局状态（data存储传感器数据），供 status/sensor 命令显示 */
            App_Status_SetLatestSensor(&data);

            /* 尝试入队（队列满则丢弃并计数，绝不阻塞采集）
             * 队列有效性保护，如果不等于NULL则说明队列成功创建且避免空指针操作；
             * 同样，系统启动时队列可能创建失败，加这个判断可以防止程序崩溃
             */
            if (sensorQueue != NULL)
            {
                /* 尝试把数据发送到队列（非阻塞）；xQueueSend为队列发送函数（三个参数分别为队列句柄、要发送的数据地址、阻塞等待时间）
                 * 此处阻塞等待时间为 0：代表如果队列满了，立刻返回失败，绝不等待
                 * 返回`pdTRUE`代表发送成功；不等于`pdTRUE`就代表队列满了，发送失败
                 */
                if (xQueueSend(sensorQueue, &data, 0) != pdTRUE)
                {
                    /* 记录队列满的次数
                       当队列满、数据丢弃时，调用这个函数给 “队列满” 的计数器加 1 */
                    App_Status_RecordSensorQueueFull();
                }
            }
        }
        else
        {
            /* 记录传感器错误状态 */
            App_Status_RecordSensorError();
            /* 打印警告日志 */
            LOG_WARN("Sensor read error");
        }

        /* 延时函数，将任务挂起指定的时间
         * SENSOR_PERIOD_MS是传感器任务执行的周期
         * pdMS_TO_TICKS（宏）用于把以毫秒为单位的时间值转换为系统时钟节拍数，方便任务延时等
         * 周期延时；更精确的调度可用 vTaskDelayUntil */
        vTaskDelay(pdMS_TO_TICKS(SENSOR_PERIOD_MS));
    }
}
