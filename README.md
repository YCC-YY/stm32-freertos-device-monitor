# STM32 FreeRTOS Device Monitor

基于 **STM32F103C8T6 + FreeRTOS** 的多任务设备监测与命令控制系统（CubeMX + CLion）。

周期性采集 BME280（温/湿/压），UART 命令行交互，队列与互斥锁进行任务协作，统一日志与系统健康监控，并支持**故障注入**验证容错。

| 标签 | 含义 |
|------|------|
| `v0.1-mvp-sim` | 四任务 + CLI + 仿真传感器 |
| `v0.2.0-board` | 真板 + BME280 + 故障注入 + 串口压测通过 |

---

## 1. 核心功能

- FreeRTOS **四任务**：Sensor / Comm / Logger / Monitor  
- **Task Notification** 唤醒串口任务（事件驱动，非空转）  
- BME280 I2C 采集（拔线检测、自动 re-init）  
- CLI：查询状态、传感器、日志开关、诊断、故障注入  
- IWDG 看门狗、任务心跳、LED 运行指示  
- 主机侧 **pytest 环缓测试** + **串口压力测试脚本**

---

## 2. 系统架构

```text
                    ┌─────────────────────┐
                    │   STM32F103C8T6     │
                    │     FreeRTOS        │
                    └──────────┬──────────┘
          ┌────────────────────┼────────────────────┐
          ▼                    ▼                    ▼
     SensorTask            CommTask            LoggerTask
          │                    │                    ▲
          ▼                    ▼                    │
    latest_sensor ←──CLI──  RingBuffer         logQueue
    (Mutex 共享)       │         ▲                  │
          │            ▼         │                  │
          │        USART2 IRQ────┘            UART 输出
          │            │
          ▼            ▼
       BME280        PC 终端
          I2C2
          │
     MonitorTask → 心跳/错误/IWDG/LED
```

设计原则：任务不直接散落 HAL；日志单一出口；共享数据用 Mutex，**不保留无消费者队列**。

---

## 3. 四任务职责

| 任务 | 优先级 | 职责 |
|------|--------|------|
| SensorTask | 2 | 1s 周期读 BME280，更新最新值，掉线 re-init |
| CommTask | 3 | 通知唤醒，收字节组命令，执行 CLI |
| LoggerTask | 1 | `logQueue` 格式化输出串口 |
| MonitorTask | 2 | 心跳超时、错误统计、IWDG、LED |

---

## 4. 硬件连接

| 功能 | 引脚 | 说明 |
|------|------|------|
| USB-UART | PA2=TX / PA3=RX（USART2） | 115200 8N1 |
| BME280 SCL/SDA | PB10 / PB11（I2C2） | 地址 0x76 |
| LED | PC13 | 低电平亮 |
| BLE（后续） | PA9/PA10 USART1 | 手机命令通道 |
| 按键（后续） | PB0 | |

供电 3.3V，共地；SCL/SDA 建议 4.7k 上拉。

---

## 5. CLI 命令演示

以下为真实串口运行输出（节选）：

```text
> status
System: RUNNING
Uptime: 966001 ms
Sensor: OK
UART: OK
Log: ON
SensorErr: 0
LogDrop: 0
Fault: 0x00

> sensor
ts=967981  temp=29.3 C  rh=65.7 %  press=100737 Pa

> chipid
I2C OK  addr=0x76  CHIP_ID=0x60  (BME280)

> help
Available commands:
  help          Show this help
  status        System status
  sensor        Latest sensor sample
  log on|off    Enable/disable INFO logs
  stats         Queues / heap / uptime
  chipid        BME280 I2C chip-id probe
  bmeread       BME280 init + one sample
  fault sensor|comm|logger|clear|show
  reset         Software reset
```

---

## 6. 故障注入与恢复

```text
> fault sensor
FAULT sensor ON (read fail + stop hb)

> status
System: WARNING
Sensor: NOT READY
Fault: 0x01

> fault clear
FAULT cleared
```

| 命令 | 效果 | 恢复 |
|------|------|------|
| `fault sensor` | 读失败 + 停心跳 | `fault clear`，无需复位 |
| `fault logger` | 停日志心跳 | `fault clear` |
| `fault comm` | 约 8s 内 CLI 暂停 | **自动** clear |
| 拔掉 BME280 | NOT READY / WARNING | 插回后 re-init |

---

## 7. 自动化测试

### 环缓单元测试（无硬件）

```bash
python Tests/test_ring_buffer.py
```

### 串口压力测试（n=200 实测）

```bash
pip install pyserial
python Tests/stress_cli.py --port COM10 --baud 115200 --n 200 --log stress.log
```

| 指标 | 结果 |
|------|------|
| 连发 200 | visible_fail = 0 |
| 用例 | 14/14 OK |
| 超时 | 0 |
| MCU 非预期复位 | 0 |
| Heap 前/后 | 2688 → 2688 |

详见 `Docs/test_report.md`。

---

## 8. 编译、烧录与运行

1. **CubeMX** 打开 `Key.ioc`（或用已有生成代码）  
2. **CLion** 打开工程，选 `Key` 目标编译  
3. 烧录 `build/Debug/Key.elf`  
4. 串口 115200 打开 COMx，输入 `help`  
5. 传感器相关：`chipid` → `bmeread` → `sensor`  

仿真开关：`App/Inc/app_config.h` 中 `SENSOR_USE_SIMULATION`（`0` = 真 BME280）。

---

## 9. 目录结构

```text
App/         配置、类型、状态、调度入口
Tasks/       Sensor / Comm / Logger / Monitor
Drivers/     bme280、sensor_driver、uart_driver
Middleware/  ring_buffer、cli、comm_channel
BSP/         LED
Core/        CubeMX 生成（HAL / 启动 / 中断）
Tests/       环缓测试、串口压测
Docs/        架构、计划、测试报告、代码导读
```

代码导读：`Docs/code_guide.md`。

---

## 10. 后续计划

- [ ] 烧录无 `sensorQueue` 版本并复核 `stats`  
- [ ] README 持续补真实截图  
- [ ] Python 自动 fault 断言（亮点）  
- [ ] **BLE / USART1** 手机命令（加分）  
- [ ] UART DMA + IDLE（加分）  

---

## License

MIT
