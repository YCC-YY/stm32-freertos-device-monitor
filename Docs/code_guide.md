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
```

---

## App/ —— 应用的“总管”

| 文件 | 作用 | 初学者怎么理解 |
|------|------|----------------|
| `app_config.h` | 所有可调参数：周期、队列长度、栈大小、仿真开关 | 想改“多久采一次”“要不要仿真”只改这里 |
| `app_types.h` | 公共数据结构：`SensorData_t`、`LogMessage_t`、状态枚举 | 相当于项目的“词汇表” |
| `app_status.h/.c` | 全局运行状态 + 任务心跳，用互斥锁保护 | 多个任务都要读/写同一份状态时，来这里 |
| `app_tasks.h/.c` | 创建队列/互斥锁；`LOG_INFO(...)` 日志宏 | 任务之间怎么通信、怎么打日志 |
| `app_scheduler.h/.c` | 系统启动入口 `App_StartScheduler()` | 上电后“建对象 → 建任务 → 开调度器” |

---

## Tasks/ —— FreeRTOS 任务（每个都是一个 `for(;;)` 循环）

| 文件 | 作用 | 一句话 |
|------|------|--------|
| `sensor_task.c` | 周期读传感器，写入最新值 / 队列 | “定时采集” |
| `comm_task.c` | 从环缓取字节 → 组命令 → 执行 | “串口键盘” |
| `logger_task.c` | 从日志队列取消息，统一打印到串口 | “唯一打字员” |
| `monitor_task.c` | 查心跳、错误、喂狗、闪灯 | “值班监督员” |

任务层**不直接**写寄存器，只调用 Drivers / App 接口。

---

## Drivers/ —— 和芯片/模块说话

| 文件 | 作用 | 一句话 |
|------|------|--------|
| `sensor_driver.h/.c` | 传感器门面：`Sensor_Init` / `Sensor_Read` | 任务只认这一层 |
| `bme280.c` | BME280 真实 I2C 读写 + 温湿压补偿 | 模块到货后看这里 |
| `uart_driver.h/.c` | 串口收发 + 环缓对接 | 中断收字节 → 环缓；任务发字符串 |

---

## Middleware/ —— 不依赖具体芯片的“积木”

| 文件 | 作用 | 一句话 |
|------|------|--------|
| `ring_buffer.c` | 环形缓冲区（SPSC） | 中断写、任务读的队列缓冲 |
| `cli.c` | 命令行：组行、解析、执行 | `help` / `status` 都在这里 |
| `comm_channel.c` | 通信通道抽象（usb / 以后 ble） | 业务不绑定某一个 UART |

---

## BSP/ —— 板子上的小零件

| 文件 | 作用 |
|------|------|
| `bsp_led.c` | PC13 LED 开/关/翻转 |

---

## Core/ —— CubeMX 生成（一般不动）

| 文件 | 作用 |
|------|------|
| `main.c` | 时钟、外设初始化，然后调用 `App_StartScheduler()` |
| `stm32f1xx_it.c` | 中断入口：USART2 收到字节后调 `UART_Driver_RxISR()` |
| `usart.c` / `i2c.c` / `gpio.c` / `iwdg.c` | 串口 / I2C / GPIO / 看门狗配置 |
| `FreeRTOSConfig.h` | FreeRTOS 堆大小、优先级等内核配置 |

---

## Docs/ 与 Tests/

| 文件 | 作用 |
|------|------|
| `Docs/execution_plan.md` | 分阶段实施计划 |
| `Docs/architecture.md` | 架构与通信关系 |
| `Docs/task_design.md` | 四个任务各自职责 |
| `Docs/pinout.md` | 引脚分配（USART2 / I2C2 / LED） |
| `Tests/test_ring_buffer.py` | 电脑上跑的环缓单元测试 |

---

## 推荐学习路径（30 分钟版）

1. 看 `app_types.h` 里 `SensorData_t` 有哪些字段  
2. 看 `sensor_task.c` 的 `for(;;)` 怎么填数据  
3. 看 `comm_task.c`：收一个字节 → `CLI_InputChar` → 换行后 `CLI_Execute`  
4. 看 `ring_buffer.c` 的 `Push` / `Pop`  
5. 看 `app_scheduler.c`：系统如何从 `main` 跑起来  

有问题的地方改完记得用 CLion 重新编译烧录验证。
