# STM32_FreeRTOS_DeviceMonitor

基于 **STM32 + FreeRTOS** 的多任务设备监测与命令控制系统（CubeMX + CLion + C）。

实现周期性传感器采集、UART 命令行交互、队列通信、互斥锁资源保护、统一日志输出与系统健康监控。业务代码与 CubeMX 生成区分离，可直接放入 GitHub 作为结构清晰的嵌入式项目模板。

## 功能（MVP）

| 能力 | 实现 |
|------|------|
| 传感器周期采集 | `SensorTask` + `sensor_driver` |
| 多任务调度 | FreeRTOS 四任务 |
| 任务间通信 | `sensorQueue` / `logQueue` |
| 资源保护 | `statusMutex` / `i2cMutex` |
| 串口命令行 | UART RX → RingBuffer → CLI |
| 状态监控 | `MonitorTask` 心跳与错误计数 |
| 日志输出 | `LoggerTask` 单一 UART 所有者 |

## 目录结构

```
DeviceMonitor/
├── Core/           # CubeMX 生成区（见 Core/README_CubeMX.md）
├── App/            # 配置、类型、全局状态、调度入口
├── Tasks/          # Sensor / Comm / Logger / Monitor
├── Drivers/        # sensor_driver、uart_driver
├── Middleware/      # RingBuffer、CLI
├── BSP/            # LED 等板级封装
├── Tests/          # 主机侧 pytest
├── Docs/           # 执行计划、架构、任务设计
└── index.html      # 手机可打开的交互演示 App
```

## 软件架构

```text
SensorTask ──xQueueSend──► sensorQueue ──► 最新值(AppStatus)
CommTask   ◄──RingBuffer◄── UART ISR
    │
    ▼
  CLI 解析 ──► status / sensor / log / stats ...
    │
LoggerTask ◄── logQueue ◄── LOG_*()
    │
    ▼
  UART TX

MonitorTask ── 心跳超时 / 错误计数 ──► SYSTEM_* 状态
```

**设计原则**

1. 任务层不直接散落 HAL / `printf`。
2. 仅 `LoggerTask`（及 CLI 的直接回显路径）写 UART。
3. 队列不是广播：最新传感器数据缓存在 `AppStatus`（Mutex 保护）。
4. 日志队列有界，高频任务 `xQueueSend(..., 10ms)`，满则丢弃计数。

## 任务一览

| 任务 | 优先级 | 周期/阻塞 | 职责 |
|------|--------|-----------|------|
| SensorTask | 2 | 1000ms | 读传感器、入队、更新最新值 |
| CommTask | 3 | 5ms 轮询（可改通知） | 收命令、CLI 执行 |
| LoggerTask | 1 | 阻塞收日志 | 格式化并输出 |
| MonitorTask | 2 | 2000ms | 心跳、错误、LED |

## CLI 命令

```
help
status
sensor
log on | log off
stats
reset
```

串口：**115200 8N1**。

## 在 CubeMX + CLion 中接入

详细步骤见 [`Docs/execution_plan.md`](Docs/execution_plan.md) 与 [`Core/README_CubeMX.md`](Core/README_CubeMX.md)。

摘要：

1. CubeMX 打开/新建 `.ioc`：使能 **FreeRTOS、USART、I2C**（可选 IWDG）。
2. 将本仓库 `App/ Tasks/ Drivers/ Middleware/ BSP/` 拷入工程根目录。
3. CMake / CubeMX CLion 插件：添加上述源文件与 include 路径。
4. `main.c`：

```c
/* USER CODE BEGIN 2 */
UART_Driver_BindHandle(&huart1);
App_StartScheduler();
/* USER CODE END 2 */
```

5. USART 中断调用 `UART_Driver_RxISR(byte)`（见 Core 文档）。
6. 编译烧录，打开串口终端输入 `help`。

> 将 `Drivers/Src/sensor_driver.c` 中 `SENSOR_USE_SIMULATION` 设为 `0`，并填入真实 I2C 读数逻辑。

## 测试

```bash
pytest Tests/ -q
```

## 阶段路线

- **P1** 骨架与任务心跳（本仓库）
- **P2** CLI 通路
- **P3–4** 真实传感器与入队
- **P5** 日志等级
- **P6** 监控完善 / IWDG
- **P7** UART DMA、Task Notification、文档完善

## 演示 App

根目录 `index.html` 为手机模拟器可打开的**交互式系统演示**（虚拟终端 + 任务面板 + 架构图），用于无硬件时理解数据流。

## License

MIT（可按需修改）。
