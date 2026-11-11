# 代码导读：每个文件是干什么的

面向初学者。读代码时可以按「从下到上」的顺序：先 Middleware/Drivers，再 Tasks，最后 App。

---

## 总览图

```text
你想理解程序时，建议顺序：

1. app_types.h / app_config.h     ← 先认识“有哪些数据、有哪些常量”
2. ring_buffer.c                  ← 最简单的通用模块
3. uart_driver.c + cli.c          ← 串口怎么收发、命令怎么解析
4. sensor_task.c / logger_task.c  ← 任务在循环里干什么
5. app_scheduler.c                ← 系统是怎么“点火”启动的
6. Tests/stress_cli.py            ← 电脑上如何自动测这套系统
```

---

## App/ —— 应用的“总管”

| 文件 | 作用 | 初学者怎么理解 |
|------|------|----------------|
| `app_config.h` | 所有可调参数：周期、队列长度、栈大小、仿真开关 | 想改“多久采一次”“要不要仿真”只改这里 |
| `app_types.h` | 公共数据结构：`SensorData_t`、`LogMessage_t`、`FAULT_*` 故障位 | 项目的“词汇表” |
| `app_status.h/.c` | 全局运行状态 + 心跳 + **故障注入掩码**（Mutex 保护） | 多任务读写同一状态时来这里 |
| `app_tasks.h/.c` | 创建队列/互斥锁；`LOG_INFO(...)` 日志宏 | 任务之间怎么通信、怎么打日志 |
| `app_scheduler.h/.c` | 系统启动入口 `App_StartScheduler()` | 上电后“建对象 → 建任务 → 开调度器” |

---

## Tasks/ —— FreeRTOS 任务（每个都是一个 `for(;;)` 循环）

| 文件 | 作用 | 一句话 |
|------|------|--------|
| `sensor_task.c` | 周期读传感器；失败计数、掉线 re-init、`fault sensor` | “定时采集 + 容错” |
| `comm_task.c` | 环缓 → CLI；Task Notification 唤醒；`fault comm` 8s 自动恢复 | “串口键盘” |
| `logger_task.c` | `logQueue` → UART；`fault logger` 停心跳 | “唯一打字员” |
| `monitor_task.c` | 心跳/错误/IWDG/LED | “值班监督员” |

---

## Drivers/ —— 和芯片/模块说话

| 文件 | 作用 | 一句话 |
|------|------|--------|
| `sensor_driver.h/.c` | `Sensor_Init` / `Sensor_Read` 门面 | 任务只认这一层 |
| `bme280.c` | I2C 读写 + 温湿压补偿；`BME280_Probe` | 三层验证用 |
| `uart_driver.h/.c` | 串口 + 环缓 + Task Notification | 中断收数、叫醒 CommTask |

---

## Middleware/ —— 不依赖具体芯片的“积木”

| 文件 | 作用 | 一句话 |
|------|------|--------|
| `ring_buffer.c` | SPSC 环形缓冲 | 中断写、任务读 |
| `cli.c` | 组行、解析、所有命令实现 | 见下方命令表 |
| `comm_channel.c` | 通信通道抽象（usb / 预留 ble） | 业务不绑死某 UART |

---

## BSP/ 与 Core/

| 文件 | 作用 |
|------|------|
| `bsp_led.c` | PC13 LED |
| `main.c` | 时钟外设，调用 `App_StartScheduler()` |
| `stm32f1xx_it.c` | USART2 中断 → `UART_Driver_RxISR` |
| `i2c.c` | I2C2（PB10/PB11）配置 |

---

## Tests/ —— 电脑上跑的测试

| 文件 | 作用 |
|------|------|
| `test_ring_buffer.py` | 环缓单元测试（不接板） |
| `stress_cli.py` | **串口压力测试**：合法/非法命令、超长、连发、复位检测 |
| `check_html.py` | 辅助检查 index.html |

压测示例：

```bash
pip install pyserial
python Tests/stress_cli.py --port COMx --baud 115200 --n 200 --log stress.log
```

---

## Docs/

| 文件 | 作用 |
|------|------|
| `execution_plan.md` | 总体分阶段计划 |
| `next_plan.md` | 焊接/验收/加分项路线 |
| `architecture.md` | 架构与通信 |
| `task_design.md` | 四任务职责 |
| `pinout.md` | 引脚与双串口架构 |
| `code_guide.md` | **本文件** |
| `test_report.md` | 测试报告模板（焊完填实测） |

---

## CLI 命令一览

| 命令 | 作用 | 新手注意 |
|------|------|----------|
| `help` | 命令列表 | |
| `status` | 系统状态 + `Fault` 掩码 | |
| `sensor` | 最近一帧传感器数据 | 拔线后仍是**缓存值** |
| `log on\|off` | 开关 INFO 日志 | |
| `stats` | 队列/堆/栈水位 | |
| `chipid` | **I2C 单测**：探测 0x76/0x77，期望 CHIP_ID=0x60 | BME280 三层验证第 1 层 |
| `bmeread` | **驱动单测**：Init + 读一次温湿度气压 | 第 2 层 |
| `fault sensor\|comm\|logger\|clear\|show` | **故障注入** | 验证监控/容错 |
| `reset` | 软复位 | |

### 故障注入怎么用

```text
fault sensor     → 读失败 + 停心跳 → status: WARNING / NOT READY
fault clear      → 恢复（sensor/logger 不必复位）
fault comm       → CLI 暂停消费约 8 秒后自动恢复
fault show       → 看当前掩码
```

---

## 推荐学习路径（30 分钟版）

1. `app_types.h`：`SensorData_t` 字段  
2. `sensor_task.c`：采集循环 + 错误/re-init  
3. `comm_task.c` + `cli.c`：字节 → 命令  
4. `ring_buffer.c`：Push/Pop  
5. `app_scheduler.c`：系统如何启动  
6. `Tests/stress_cli.py`：如何做自动化验证  
