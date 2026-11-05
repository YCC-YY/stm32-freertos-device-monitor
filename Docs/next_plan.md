# 下一阶段计划（焊接完成后执行）

> 前提：`v0.1-mvp-sim` 已冻结；BME280 已通过 chipid / bmeread / SensorTask；拔插恢复（re-init）已做。
> 焊接 PCB 期间不扩新模块，只完善文档与可并行的主机侧测试。

---

## 0. 当前完成度（简表）

| 项 | 状态 |
|----|------|
| 四任务 + Queue/Mutex + CLI + Monitor + IWDG | 完成 |
| Task Notification 唤醒 CommTask | 完成 |
| BME280 真数据进 SensorTask | 完成（仿真已关） |
| 掉线 NOT READY / 插回 re-init | 完成（待 PCB 复测） |
| chipid / bmeread 诊断命令 | 完成 |
| 故障注入 `fault` | 未做 |
| 串口压力测试 pytest | 未做 |
| README 测试报告 / 架构定稿 | 未做 |
| BLE / DMA / PB0 按键 | 后置 |

---

## 1. 阶段 A — 焊接期间（可做，不依赖新板）

### A1. 文档收口（0.5 天）
- [ ] README 更新：引脚表、命令表、三层验证步骤、拔插恢复说明  
- [ ] `Docs/test_report.md` 模板：写好空表格，焊完填实测值  
- [ ] 标注 `v0.1-mvp-sim` 与当前 master 差异（Notification / 真 BME280 / re-init）

### A2. 主机侧压力测试脚本（1 天）
- [ ] `Tests/stress_cli.py`（pyserial）  
  - 发送 `status` / `sensor` / `stats` × N（默认 500）  
  - 统计：成功/超时/乱码/复位（uptime 回退）  
  - 输出：`Commands OK / FAIL / MCU Reset`  
- [ ] 不依赖 BME280 也可跑（命令回显即可断言）

### A3. CLI 小完善（可选，0.5 天）
- [ ] `status` 增加各任务心跳年龄（更直观）  
- [ ] **不**再堆新命令

---

## 2. 阶段 B — PCB 焊完、上电后（核心验收）

按「上电 → 稳定 → 再折腾」顺序：

### B1. 上电冒烟（0.5 天）
- [ ] 3.3V / GND / 电流是否正常  
- [ ] 烧录后 `status`：`RUNNING`，无异常复位  
- [ ] LED 闪烁、IWDG 不误复位  

### B2. I2C + BME280 复测（0.5 天）
- [ ] `chipid` → `0x76` / `0x60`  
- [ ] `bmeread` 与 `sensor` 数值一致且合理（平原约 1010 hPa）  
- [ ] 连续跑 10 分钟：`SensorErr` 稳定为 0  

### B3. 容错（0.5 天）
- [ ] 拔 BME280 → `NOT READY` / `WARNING`，CLI 仍可用  
- [ ] 插回 → 5 秒内恢复 `OK` + 新数据  
- [ ] 记录恢复耗时与 `SensorErr` 增量  

### B4. 通信稳定（0.5 天）
- [ ] 跑 `stress_cli.py` 500～1000 次  
- [ ] 无乱码、无死机、无环缓溢出（可看 `stats`）  

### B5. 写入测试报告（0.5 天）
- [ ] 填 `Docs/test_report.md`  
- [ ] Git tag：`v0.2.0-board`（板级验收版）  

---

## 3. 阶段 C — 系统能力增强（板稳后再做）

| 优先级 | 项 | 做法简述 | 预估 |
|--------|----|----------|------|
| C1 | **fault 故障注入** | CLI：`fault sensor\|clear`，模拟停心跳/读失败 | 1 天 |
| C2 | status 更像设备 | 任务表 + 最近采样摘要 | 0.5 天 |
| C3 | 心跳年龄 / 错误环 | Monitor 输出更细 | 0.5 天 |

C1 建议最小集：

```text
fault sensor   → SensorTask 读失败并停心跳
fault clear    → 恢复正常
status         → System: WARNING/ERROR
```

用于课堂/答辩演示「监控真的会报警」。

---

## 4. 阶段 D — 加分项（有时间再做）

| 项 | 说明 | 建议 |
|----|------|------|
| BLE / USART1 | 手机发 `status`/`sensor` | 复用 `comm_channel` + CLI，不改业务 |
| UART DMA + IDLE | 降低中断开销 | 在 Notification 架构上替换收数 |
| PB0 按键 | 短按打印 status | BSP 扩展 |
| README 截图/演示 | 串口 log、系统框图 | 放 GitHub 首页 |

**明确不做**：推翻四任务架构、重写 CLI、MQTT/WiFi。

---

## 5. 建议执行顺序（焊完后）

```text
B1 冒烟 → B2 BME280 → B3 拔插 → B4 压测 → B5 报告+tag
                              ↓
                         C1 fault 演示
                              ↓
                    （可选）D：BLE / DMA / 按键
```

**里程碑**

| Tag | 含义 |
|-----|------|
| `v0.1-mvp-sim` | 仿真 MVP（已有） |
| `v0.2.0-board` | 真板 + BME280 + 容错 + 压测 |
| `v0.3.0-fault` | 故障注入与监控演示 |

---

## 6. 你现在可以拍板的 3 件事

1. 阶段 A 先做 **压测脚本** 还是 **README/测试报告**？  
2. `fault` 只做 `sensor` 一项，还是也要 `fault logger/comm`？  
3. BLE 是否进 **v0.3** 必做，还是纯加分？  
