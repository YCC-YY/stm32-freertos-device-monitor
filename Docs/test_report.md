# 测试报告

| 项 | 值 |
|----|-----|
| 固件 / Tag | `v0.2.0-board` |
| 硬件 | STM32F103C8T6 + BME280（I2C2，0x76） |
| 串口 | COM10 CH340，115200 8N1 |
| 日期 | 2026-11-04 |

## 1. 功能用例

| ID | 用例 | 结果 |
|----|------|------|
| T1 | CLI help/status/sensor | **PASS** |
| T2 | chipid 0x76/0x60 | **PASS** |
| T3 | bmeread / sensor 数值合理（约 1010 hPa） | **PASS** |
| T4 | 连续采集 | **PASS** |
| T5 | 拔线 → NOT READY + WARNING，CLI 可用 | **PASS** |
| T6 | 插回 re-init | **PASS** |
| T7 | log on/off | **PASS** |
| T8 | IWDG / 无无故复位 | **PASS** |

## 2. 故障注入

| ID | 命令 | 结果 |
|----|------|------|
| F1 | `fault sensor` | **PASS** Fault:0x01 / WARNING / NOT READY |
| F2 | `fault clear` | **PASS** 无需复位 |
| F3 | `fault logger` | **PASS** |
| F4 | `fault comm` | **PASS** 约 8s 自动恢复 |
| F5 | `fault show` | **PASS** |

## 3. 压力测试（n=200）

```bash
python Tests/stress_cli.py --port COM10 --baud 115200 --n 200 --log Tests/stress.log
```

### 指标（严格区分）

| 指标 | 数值 | 说明 |
|------|------|------|
| 总请求数（脚本用例 + 连发） | **200 连发 + 14 组用例** | 连发内含 status/sensor/help/stats/badcmd |
| 成功响应数 | **14/14 用例；连发 visible_fail=0** | 有预期关键字或 `>` 提示符 |
| 超时数（无响应/未收到包） | **0** | |
| 错误响应数（CLI 主动 Unknown 等） | **1**（`@@@` 探测） | **不算系统失败** |
| MCU 非预期复位 | **0** | uptime 单调：964s → 1182s |
| 测试前 Heap | **2688 B** | `stats` FreeHeap |
| 测试后 Heap | **2688 B** | 无泄漏迹象 |
| LogQueue 前/后 | **0 → 0** | |
| 队列状态 | 旧固件：SensorQueue 8/8；**新固件已删除该队列** | 见遗留 |

脚本摘要：

```text
Total requests     : 14
OK responses       : 14
Timeout/No-resp    : 0
CLI error replies  : 1
MCU unexpected rst : 0
Heap before/after  : 2688 -> 2688
LogQueue before/after: 0 -> 0
[OK] burst_stress     n=200 visible_fail=0
[OK] alive_after_stress uptime=1182026
```

## 4. 边界

| ID | 项 | 结果 |
|----|----|------|
| E1 | 超长命令 120 字节 | **PASS** |
| E2 | 非法命令 | **PASS** 返回 Unknown，系统继续 |
| E3 | stats 可观测 | **PASS** |
| E4 | LogDrop 字段 | **PASS** |

## 5. 结论

**通过**，维持 `v0.2.0-board`。

**遗留（已改代码，待烧录验证）：**

1. `sensorQueue` 已从源码删除（无消费者）；需烧录新 `Key.elf` 后 `stats` 不再出现 SensorQueue  
2. 大规模连发内部逐条未计入 Total requests，以 `visible_fail=0` 为准  

**区分说明：**

- **Timeout/No-resp**：主机没收到预期回复 → 真失败  
- **CLI error**：设备回复 `Unknown` 等 → 容错正常  
- **MCU reset**：uptime 回退/重启 banner → 非预期复位  
