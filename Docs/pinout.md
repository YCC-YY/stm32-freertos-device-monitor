# 引脚与通信架构（STM32F103C8T6）

## 硬件引脚

| 功能 | STM32 引脚 | 外设 | 状态 |
|------|------------|------|------|
| USB-UART TXD | PA3 | USART2_RX | 已调通 |
| USB-UART RXD | PA2 | USART2_TX | 已调通 |
| BLE TXD | PA10 | USART1_RX | 后续接入 |
| BLE RXD | PA9 | USART1_TX | 后续接入 |
| BME280 SCL | PB10 | I2C2_SCL | 保持 |
| BME280 SDA | PB11 | I2C2_SDA | 保持 |
| LED | PC13 | GPIO | 保持（低电平点亮） |
| 按键 | PB0 | GPIO | 保持 |

> 模块 TX ↔ STM32 RX，模块 RX ↔ STM32 TX，共地。BLE 供电与电平按型号确认。

## 通信分工

| 通道 | 用途 | 软件入口 |
|------|------|----------|
| USB-UART / USART2 | 调试 CLI、日志 | `CommTask` + `comm_channel`("usb") |
| BLE / USART1 | 手机命令、数据回传 | 后续 `CommChannel`("ble") |

## 关键设计

业务逻辑 **不绑定具体 UART**。命令处理只走 `CLI_Execute` / 公共 handler；物理层用 `CommChannel_t` 注册：

```c
/* Middleware/CLI/comm_channel.h */
typedef struct CommChannel {
    const char *name;
    bool (*read_byte)(uint8_t *b);
    int  (*write)(const uint8_t *data, size_t len);
} CommChannel_t;
```

当前仅注册 `usb`（USART2）。USART1/BLE 就绪后 `Comm_RegisterChannel()` 即可复用整条 CLI，无需改任务业务。

## 第一版策略

1. **只调 USB-UART CLI**（help / status / sensor / log / stats）。
2. BME280@I2C2 稳定后再加 BLE 串口，避免双串口联调。
3. `sensor_driver` → `bme280.c`：任务层只调 `Sensor_Read()`。
