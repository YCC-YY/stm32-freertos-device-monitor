# 任务设计（Task Design）

## SensorTask

- **文件**：`Tasks/Src/sensor_task.c`
- **周期**：`SENSOR_PERIOD_MS`（1000）
- **步骤**：
  1. 更新心跳
  2. `Sensor_Read`（驱动层，I2C Mutex）
  3. 成功：写入 `AppStatus.latest_sensor`，非阻塞 `xQueueSend`
  4. 失败：`sensor_error_count++`，`LOG_WARN`
- **不直接**打印原始寄存器数据到 UART

## CommTask

- **文件**：`Tasks/Src/comm_task.c`
- **唤醒**：MVP 为 5ms 轮询 RingBuffer；第二阶段改为任务通知
- **流水线**：

```
UART 中断 → RingBuffer → CLI_InputChar → 行结束 → CLI_Execute
```

- 命令集：`help/status/sensor/log/stats/reset`

## LoggerTask

- **文件**：`Tasks/Src/logger_task.c`
- **阻塞**：`xQueueReceive(logQueue, portMAX_DELAY)`
- **输出格式**：`[tick][LVL] message\r\n`
- 是日志的**唯一**格式化出口（CLI 命令回显走 UART 驱动，属于交互路径）

## MonitorTask

- **文件**：`Tasks/Src/monitor_task.c`
- **周期**：2000ms
- **检查**：
  - sensor/comm/logger 心跳是否超时
  - `sensor_error_count` 是否超过阈值
  - 更新 `SystemState_t`、翻转 LED
  - （选做）喂 IWDG

## 优先级建议

| 任务 | prio |
|------|------|
| LoggerTask | 1 |
| SensorTask | 2 |
| MonitorTask | 2 |
| CommTask | 3 |

交互响应优先于周期采集；日志最低避免反压阻塞采集。可按现场再调。

## 栈大小

初始 256 words（视栈单位而 `configSTACK_DEPTH_TYPE`）。  
上板后用 `uxTaskGetStackHighWaterMark` 校验，禁止盲目抄值。

## 演进

1. Comm：轮询 → Task Notification  
2. Sensor：`vTaskDelay` → `vTaskDelayUntil`（稳定周期）  
3. UART：字节中断 → DMA+IDLE  
4. 可选：`reset` 命令后的安全停机序列  
