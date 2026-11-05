# 测试报告模板

> 焊接完成后填写实测值。版本号、串口号、日期每次保留。

## 1. 测试环境

| 项 | 值 |
|----|-----|
| 固件版本 / Git tag | |
| 硬件 | STM32F103C8T6 + BME280 PCB |
| 串口 | COM___ 115200 8N1 |
| 测试人 / 日期 | |

## 2. 功能用例

| ID | 用例 | 步骤 | 预期 | 结果 | 备注 |
|----|------|------|------|------|------|
| T1 | CLI 基础 | `help` `status` `sensor` | 关键词正确 | | |
| T2 | I2C | `chipid` | `0x76` / `0x60` | | |
| T3 | 驱动 | `bmeread` 与 `sensor` | 数值合理且接近 | | |
| T4 | 正常采集 | 跑 10min | SensorErr=0 | | |
| T5 | 拔线 | 拔 BME280 | NOT READY + WARNING，CLI 可用 | | |
| T6 | 插回 | 接回 BME280 | 5s 内恢复 OK | | |
| T7 | 日志开关 | `log off` / `log on` | INFO 停/启 | | |
| T8 | 看门狗 | 长时间运行 | 无无故复位 | | |

## 3. 故障注入

| ID | 命令 | 预期 | 结果 |
|----|------|------|------|
| F1 | `fault sensor` | SensorErr 涨 / NOT READY / WARNING | |
| F2 | `fault clear` | 恢复 OK | |
| F3 | `fault logger` | 超时后 WARNING（心跳） | |
| F4 | `fault comm` | 命令暂无响应；`clear` 后恢复（需先 clear 或复位） | |
| F5 | `fault show` | 显示 mask | |

## 4. 压力测试

```bash
python Tests/stress_cli.py --port COMx --baud 115200 --n 200 --log stress.log
```

| 指标 | 目标 | 实测 |
|------|------|------|
| Commands OK | ≥ 95% | |
| Commands FAIL | ≤ 5% | |
| MCU Reset | 0 | |
| 乱码/死锁 | 无 | |
| 超长命令后 status | 仍应答 | |
| 快速连发后 status | 仍应答 | |

粘贴脚本摘要：

```text

```

## 5. 边界

| ID | 项 | 结果 |
|----|----|------|
| E1 | 超长命令 120 字节 | |
| E2 | 非法命令 `!!!` `foo` | |
| E3 | 环缓溢出计数 `stats` | |
| E4 | 日志队列满 LogDrop | |

## 6. 结论

- [ ] 通过（可打 tag `v0.2.0-board`）
- [ ] 有条件通过（列遗留）
- [ ] 不通过

**遗留问题：**

1.  

**备注：**
