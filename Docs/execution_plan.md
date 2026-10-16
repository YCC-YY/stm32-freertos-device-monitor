# STM32 FreeRTOS Device Monitor — 执行计划

## 0. 项目原则

1. **先架构，后功能**：先搭好 App / Tasks / Drivers / Middleware 分层与 FreeRTOS 对象，再逐个填充真实逻辑。
2. **CubeMX 与业务代码分离**：`Core/` 只放 CubeMX 生成物；自己写的代码放在 `App/`、`Tasks/`、`Drivers/`、`Middleware/`、`BSP/`，避免重新生成工程被覆盖。
3. **单一所有者**：UART 日志输出只由 `LoggerTask` 负责；任务不直接 `printf` 到串口。
4. **MVP 优先**：第一版只做 3 个核心任务 + Queue + Mutex + `help`/`status`，监控与 DMA 等后置。

---

## 1. 目标与范围

| 项 | 内容 |
|---|---|
| MCU | STM32（CubeMX + CLion + FreeRTOS + C） |
| 周期 | 约 1 个月，按模块递进 |
| 核心能力 | 周期采集、UART CLI、队列通信、互斥锁、统一日志、状态监控 |
| 第一版不做 | Wi-Fi / GUI / MQTT / 完整 BLE / 完整 DMA 闭环 |

### MVP 清单（本仓库已落地）

- [x] FreeRTOS 创建任务骨架（Sensor / Comm / Logger / Monitor）
- [x] `SensorDataQueue` 传递传感器数据
- [x] UART 接收 → RingBuffer → CLI 组行
- [x] `help` / `status` / `sensor` / `log on|off` / `stats`
- [x] `LoggerTask` 统一日志输出 + 日志等级
- [x] Mutex 保护共享状态 / I2C 总线
- [x] 任务心跳 + MonitorTask 检查
- [x] README + 架构说明

---

## 2. 分层架构

```
┌─────────────────────────────────────────────────────┐
│ Tasks/     SensorTask  CommTask  LoggerTask  Monitor│
├─────────────────────────────────────────────────────┤
│ App/       配置、类型、全局状态、任务创建与对象初始化   │
├─────────────────────────────────────────────────────┤
│ Drivers/   sensor_driver  uart_driver               │
│ Middleware/ RingBuffer  CLI                          │
│ BSP/       LED / 板级封装                            │
├─────────────────────────────────────────────────────┤
│ Core/      CubeMX 生成：HAL、时钟、FreeRTOS 移植      │
└─────────────────────────────────────────────────────┘
```

**依赖方向**：`Tasks → App/Drivers/Middleware/BSP → Core(HAL)`。  
任务层不直接写寄存器、不直接散落 `printf`。

---

## 3. 任务与通信设计

| 任务 | 职责 | 优先级建议 | 阻塞方式 |
|---|---|---|---|
| SensorTask | 周期读传感器，入队 | Normal (2) | `vTaskDelay` 周期 |
| CommTask | 取环缓、组命令、执行 | AboveNormal (3) | 短延时 / 后续任务通知 |
| LoggerTask | 出日志队列，写 UART | Low (1) | `xQueueReceive` 阻塞 |
| MonitorTask | 心跳/错误/堆栈检查 | Normal (2) | 周期延时 |

### 同步原语

| 对象 | 类型 | 用途 |
|---|---|---|
| `sensorQueue` | Queue(8) | SensorData_t：采集结果 |
| `logQueue` | Queue(16) | LogMessage_t：结构化日志 |
| `statusMutex` | Mutex | 保护 `AppStatus_t` |
| `i2cMutex` | Mutex | 保护 I2C 传感器总线 |
| 心跳字段 | 共享结构（statusMutex 内） | MonitorTask 超时检查 |

### 数据流

```
SensorTask ──xQueueSend──► sensorQueue ──(最新值缓存/AppStatus)
CommTask   ──RingBuffer◄── UART ISR
    │
    ▼
  CLI 解析 ──► Command Handler ──► AppStatus / Sensor / Log 控制
    │
LoggerTask ◄──xQueueReceive── logQueue ◄── 各任务 LOG_* 宏
MonitorTask ──读心跳/错误计数──► 更新 SystemState / 可选喂狗
```

**队列语义说明**：FreeRTOS 队列不是广播。第一版由 App 层维护“最新传感器数据”副本（`statusMutex` 保护），`sensor` 命令与 Logger 展示都读该副本；`sensorQueue` 用于解耦采集节奏与后续消费者扩展。

---

## 4. 目录落地

```
DeviceMonitor/
├── Core/           # CubeMX 生成（本仓库提供接入说明与 freertos 钩子示例）
├── App/Inc|Src     # app_config / app_types / app_status / app_tasks
├── Tasks/Inc|Src   # sensor/comm/logger/monitor
├── Drivers/Inc|Src # sensor_driver / uart_driver
├── Middleware/     # RingBuffer / CLI
├── BSP/Inc|Src     # LED
├── Tests/          # host-side ring buffer + CLI 解析思路（pytest）
├── Docs/           # architecture / task_design
├── README.md
└── index.html      # 手机可试的交互式系统演示 App
```

---

## 5. 分阶段实施（7 阶段）

### 阶段 1 — 骨架可跑（本仓库完成）
1. 建目录与头文件边界  
2. 定义 `app_types.h`、任务头文件  
3. `App_InitObjects()` 创建 Queue/Mutex/任务  
4. 各任务先周期打一条日志，确认调度  
5. CLion/CMake 或 CubeMX 工程能编译链接（需本机 CubeMX 生成 `Core/`）

### 阶段 2 — CLI 通路
1. UART RX 中断写入 RingBuffer  
2. `CLI_ReadLine` 组行（处理 `\r`/`\n`、退格）  
3. 实现 `help`、`status`  
4. CLI 与 HAL 解耦：结果经 Logger 输出  

### 阶段 3 — 传感器驱动
1. I2C + `Sensor_Init` / `Sensor_Read`（本仓库提供 DHT 类逻辑桩 + HTS221 风格接口，按板替换）  
2. 读 ID / 校验失败计数  
3. 错误路径返回 `SENSOR_ERROR/TIMEOUT`  

### 阶段 4 — 采集入队
1. SensorTask 周期 1000ms  
2. 成功：打时间戳 → `xQueueSend` → 更新最新值  
3. 失败：`App_Status_RecordSensorError()`  
4. 队列满：非阻塞发送 + 计数  

### 阶段 5 — 日志子系统
1. `LogMessage_t` + `logQueue`  
2. `LOG_INFO/WARN/ERROR` 宏  
3. `log on/off` 控制等级过滤  
4. 仅 LoggerTask 写 UART  

### 阶段 6 — 监控完善
1. 心跳更新时间戳（不是只置位）  
2. MonitorTask 检查超时 → `SYSTEM_WARNING/ERROR`  
3. `stats` 显示队列水位、堆、运行时长  
4. 可选 IWDG  

### 阶段 7 — 增强与测试
1. UART DMA + Idle 中断  
2. Task Notification 唤醒 CommTask  
3. `Tests/test_ring_buffer.py` 主机侧测试  
4. README / 架构图 / 测试报告  

---

## 6. 关键配置（起点，非终值）

```c
#define SENSOR_PERIOD_MS       1000U
#define MONITOR_PERIOD_MS      2000U
#define CLI_MAX_COMMAND_LENGTH 64U
#define SENSOR_QUEUE_LENGTH    8U
#define LOG_QUEUE_LENGTH       16U
#define UART_RX_BUFFER_SIZE    128U
#define LOG_MESSAGE_MAX_LEN    64U
#define HEARTBEAT_TIMEOUT_MS   5000U
```

栈大小：按实际调用链与 `uxTaskGetStackHighWaterMark` 调整，不盲目抄固定值。

---

## 7. CubeMX + CLion 接入步骤（本机执行）

1. CubeMX 新建工程 → 使能 FreeRTOS → USARTx + I2Cx +（可选）IWDG  
2. FreeRTOSConfig：`configUSE_MUTEXES=1`、`configUSE_TASK_NOTIFICATIONS=1`、堆足够  
3. 将本仓库 `App/ Tasks/ Drivers/ Middleware/ BSP/` 拷入工程根  
4. CLion 打开 CubeMX 生成工程；在 CMake 中 `include_directories` 增加上述 `Inc` 路径，并加入对应 `.c`  
5. `main.c` 中 HAL_Init/时钟/MX_*_Init 后调用 `App_StartScheduler()`（内部创建对象并 `vTaskStartScheduler`）  
6. `stm32f1xx_it.c`（按系列）USART IRQ 中调用 `UART_Driver_RxISR(uint8_t byte)`  
7. 编译烧录 → 串口 115200 8N1 → 输入 `help`

---

## 8. 验收标准

| 阶段 | 验收 |
|---|---|
| P1 | 三任务周期日志交错输出，无硬错 |
| P2 | 串口输入 `help`/`status` 有回显与结构化输出 |
| P3–4 | `sensor` 显示递增 timestamp 的温湿度；拔线时 error count 上升 |
| P5 | `log off` 后 INFO 不再刷屏，ERROR 仍可出 |
| P6 | 杀死 SensorTask 或卡住后，Monitor 报 WARNING |
| P7 | pytest 覆盖 ring buffer 边界；README 可复现构建 |

---

## 9. 风险与对策

| 风险 | 对策 |
|---|---|
| CubeMX 覆盖用户代码 | 用户代码写在 `App/Tasks/...`，`USER CODE` 区只留调用一行 |
| 队列被单消费者取走 | 最新值进 `AppStatus`，命令侧读副本 |
| 高频任务等日志队列卡死 | `xQueueSend` 超时 0~10ms，满则丢弃并计数 |
| RingBuffer ISR+任务竞态 | ISR 写入用临界区/单生产者单消费者约定；volatile 不足以为凭 |
| 栈溢出 | 提高水位监控，`stats` 打印 high water mark |

---

## 10. 本仓库交付物

1. **可编译集成的 C 源码分层**（MVP + Monitor + CLI 全命令）  
2. **Docs/**：架构与任务设计  
3. **Tests/**：主机侧 ring buffer 测试  
4. **index.html**：手机模拟器可打开的交互演示 App（任务面板 + 虚拟 CLI + 架构图）  
5. **README.md**：克隆后如何在 CubeMX/CLion 接线  

**建议执行顺序**：先完成阶段 1 在板载跑通心跳日志 → 阶段 2 CLI → 阶段 3–4 传感器 → 阶段 5–6 日志与监控 → 阶段 7 增强。
