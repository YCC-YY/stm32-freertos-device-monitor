# 架构说明（Architecture）

## 1. 分层

| 层 | 目录 | 允许依赖 |
|----|------|----------|
| 应用任务 | `Tasks/` | App、Drivers、Middleware、BSP |
| 应用核心 | `App/` | Drivers、Middleware、FreeRTOS |
| 驱动 | `Drivers/` | HAL、Middleware |
| 中间件 | `Middleware/` | 标准 C（尽量无 HAL） |
| 板级 | `BSP/` | HAL |
| 生成区 | `Core/` | — （不写业务） |

## 2. FreeRTOS 对象

| 名称 | 类型 | 尺寸/说明 |
|------|------|-----------|
| sensorQueue | Queue | 8 × SensorData_t |
| logQueue | Queue | 16 × LogMessage_t |
| statusMutex | Mutex | AppStatus_t |
| i2cMutex | Mutex | I2C 总线事务 |

## 3. 任务通信

```
                    ┌────────────┐
   UART RX IRQ ───► │ RingBuffer │──► CommTask ──► CLI_Execute
                    └────────────┘        │
                                          ├── 读/写 AppStatus (statusMutex)
                                          └── 控制 logging_enabled

   SensorTask ──(queue, 可选消费者)──► sensorQueue
        │
        └── App_Status_SetLatestSensor (statusMutex)  ◄── status/sensor 命令

   任意任务 LOG_* ──► logQueue ──► LoggerTask ──► UART TX
```

## 4. 心跳模型

各任务每周期写入 `heartbeat.xxx = now_ms`（在 statusMutex 内）。  
`MonitorTask` 比较 `now - last > HEARTBEAT_TIMEOUT_MS` 则进入 `SYSTEM_WARNING`。

仅“置位不更新时间”无法发现卡死；本项目以**时间戳超时**为准。

## 5. 日志路径

- `App_Log()`：格式化到栈上 `LogMessage_t`，非永久阻塞入队。
- `logging_enabled == 0` 时丢弃 `DEBUG/INFO`，仍保留 `WARN/ERROR`。
- 队列满：`log_dropped_count++`。

## 6. 并发注意

| 资源 | 策略 |
|------|------|
| AppStatus | statusMutex |
| I2C | i2cMutex |
| UART TX | 单一所有者（Logger / CLI 路径约定） |
| RX RingBuffer | ISR 写 / 任务读，临界区保护 |

`volatile` 不构成完整的并发协议。

## 7. 扩展点

- **DMA + Idle**：`UART_StartReceiveDMA` + `RxCplt/Idle` 回调写入更大环缓。
- **Task Notification**：ISR `vTaskNotifyGiveFromISR` 唤醒 CommTask。
- **IWDG**：仅在 Monitor 确认各心跳健康后 `HAL_IWDG_Refresh`。
- **多消费者**：不要复用同一队列弹出；用状态副本或分发任务。
