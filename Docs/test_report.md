# 测试报告

| 项 | 值 |
|----|-----|
| 固件版本 / Git tag | `v0.2.0-board` |
| 硬件 | STM32F103C8T6 + BME280（I2C2，0x76） |
| 串口 | COM10（CH340）115200 8N1 |
| 测试人 / 日期 | 自动化 + 手工联调 / 2026-11-04 |

## 1. 功能用例

| ID | 用例 | 预期 | 结果 | 备注 |
|----|------|------|------|------|
| T1 | CLI 基础 | help/status/sensor 正常 | **PASS** | 回显正常 |
| T2 | I2C | chipid 0x76/0x60 | **PASS** | |
| T3 | 驱动 | bmeread 与 sensor 合理 | **PASS** | 平原约 1010 hPa |
| T4 | 正常采集 | 持续出数 | **PASS** | |
| T5 | 拔线 | NOT READY + WARNING，CLI 可用 | **PASS** | |
| T6 | 插回 | re-init 恢复 OK | **PASS** | |
| T7 | log on/off | INFO 停/启 | **PASS** | |
| T8 | 长跑/IWDG | 无无故复位 | **PASS** | uptime 连续增长 |

## 2. 故障注入

| ID | 命令 | 预期 | 结果 |
|----|------|------|------|
| F1 | `fault sensor` | SensorErr 涨 / NOT READY / WARNING / Fault:0x01 | **PASS** |
| F2 | `fault clear` | 恢复 OK，无需复位 | **PASS** |
| F3 | `fault logger` | 停心跳 → WARNING | **PASS** |
| F4 | `fault comm` | 约 8s 自动恢复 | **PASS** |
| F5 | `fault show` | 显示 mask | **PASS** |

## 3. 压力测试

```bash
python Tests/stress_cli.py --port COM10 --baud 115200 --n 50 --log Tests/stress.log
```

| 指标 | 目标 | 实测 |
|------|------|------|
| Commands OK | ≥ 95% | **100%（14/14 用例全过）** |
| Commands FAIL | ≤ 5% | **0** |
| MCU Reset | 0 | **0** |
| burst n=50 visible_fail | ≤ 5% | **0** |
| 超长命令后 status | 仍应答 | **PASS** |
| 乱码/死锁 | 无 | **无** |

摘要：

```text
Commands sent : 14
Commands OK   : 14
Commands FAIL : 0
MCU Reset     : 0
[OK] burst_stress  n=50 visible_fail=0
[OK] alive_after_stress  uptime=184001
```

## 4. 边界

| ID | 项 | 结果 |
|----|----|------|
| E1 | 超长命令 120 字节 | **PASS** 无复位 |
| E2 | 非法命令 foo/!!! | **PASS** Unknown |
| E3 | stats 队列/堆 | **PASS** 可观测 |
| E4 | LogDrop | **PASS** 计数字段存在 |

## 5. 结论

**通过** — 打 tag `v0.2.0-board`。

**遗留 / 改进（非阻塞）：**

1. `sensorQueue` 当前无消费者，`stats` 可能长期 8/8（仅诊断用；可后续加 Logger 消费或不入队）  
2. 压测默认建议 `--n 200`；本次联调用 n=50 已覆盖逻辑  
3. BLE / DMA / PB0 仍为加分项  

**备注：** 与 `fault comm` 交互时 8s 内 CLI 无响应属预期。
