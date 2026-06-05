# AdjustablePowerSupply

基于 MP4201 的大功率可调数字电源，ESP32-P4 固件。

> ⚠️ **早期开发阶段** — 功能持续完善中，API 可能变动。

## 硬件

| 组件 | 型号 | 接口 | 说明 |
|------|------|------|------|
| 主控 | ESP32-P4 (RISC-V) | — | 360MHz, 32MB PSRAM, 16MB Flash |
| 电源控制器 | MP4201 | I2C (0x3F) | 100V/25A 双向升降压, PMBus 协议 |
| DAC × 2 | MCP4725 | I2C (0x60/0x61) | 12-bit, 5V 供电, 分别接 MP4201 FREQ/MODE 引脚 |
| 温湿度传感器 | SHT40 | I2C (0x44) | 精度 ±0.2°C / ±1.8%RH |
| SD 卡 | — | SPI (CS=GPIO10) | FAT 文件系统, 字模/动画帧读取 |
| 蜂鸣器 | 无源 | GPIO21 | LEDC PWM 驱动 |
| 按键 × 6 | — | GPIO1~5, GPIO14 | 长按/短按检测 |

### 引脚分配

| GPIO | 功能 |
|------|------|
| 1–5 | 按键输入 |
| 7 | I2C SDA |
| 8 | I2C SCL |
| 11 | SPI MOSI |
| 12 | SPI CLK |
| 13 | SPI MISO |
| 14 | 按键输入 |
| 21 | 蜂鸣器 PWM |
| 37/38 | 调试 UART |

## 构建

```bash
# 依赖：ESP-IDF v5.5.3, 目标已设为 esp32p4
idf.py build              # 编译
idf.py flash              # 烧录 (/dev/ttyUSB0)
idf.py monitor            # 串口监视 (115200)
idf.py build flash monitor  # 一键
```

- C++17, `-O3 -ffast-math`
- 编译器: esp-clang 19.1.2
- clangd 配置: `.vscode/settings.json`

## 架构

```
app_main
  ├── StatusKey          — 按键扫描 (FreeRTOS Task)
  ├── DeviceInit          — 总线 + 设备初始化
  │     ├── I2CBusManager — I2C 总线 (GPIO7/8, 100kHz)
  │     │     ├── SHT40   — 温湿度传感器 (0x44)
  │     │     ├── MP4201  — 电源控制器 (0x3F)
  │     │     ├── MCP4725 — DAC FREQ (0x60)
  │     │     └── MCP4725 — DAC MODE (0x61)
  │     └── SPIBusManager — SPI 总线 (GPIO11/12/13)
  │           └── SDCard  — SD 卡
  ├── TemperatureMonitor  — 温度采集 (500ms)
  ├── AdjustablePSU       — 电源应用层
  │     ├── FREQ/MODE 配置 (通过 MCP4725)
  │     ├── MP4201 输出控制
  │     └── ADC 监控 (VIN/IIN/VOUT/IOUT/Temp)
  ├── PerfMonitor         — 性能监控 (堆栈/内存)
  └── Buzzer              — 蜂鸣器 (内部 Task 监听命令队列)
```

### 设备驱动层次

```
Device (纯虚基类: Probe / Init / Deinit)
  ├── I2CDevice (+ Write / Read / WriteThenRead)
  │     ├── SHT40
  │     ├── MP4201    (+ PMBus WriteByte/Word, ReadByte/Word)
  │     └── MCP4725   (+ WriteVolatile, WriteEEPROM)
  └── SPIDevice (+ Transmit)
        └── SDCard
```

## 功能

### 电源控制 (MP4201)

- 输出电压: 3.2V–81.92V (20mV/step, 内部反馈)
- 输出/输入电流限制: 0.5A–25A (50mA/step)
- 双向功率流 (充电/放电)
- 开关频率: 200/400/600/1000 kHz (DAC 设定)
- 工作模式: PFM / FCCM, 展频开关 (DAC 设定)
- OCP/OVP/OTP 保护
- ADC 实时读数: VIN, IIN, VOUT, IOUT, 芯片温度

### 蜂鸣器

其他 Task 可异步通知:

```cpp
Buzzer::GetInstance().NotifyBeep(200, 1000);   // 滴 200ms
Buzzer::GetInstance().NotifyAlarm(3000);        // 警报 3 秒
Buzzer::GetInstance().NotifyStop();
```

### 按键

6 路按键，自动区分短按/长按（默认 2 秒阈值），通过 FreeRTOS Queue 通知监听者。

## 目录

```
AdjustablePowerSupply/
├── main/                  — 应用入口
├── components/
│   ├── device/            — 设备抽象层
│   │   ├── i2c_device/    — I2C 总线 + 设备
│   │   │   ├── sht40/     — SHT40 驱动
│   │   │   ├── mp4201/    — MP4201 驱动
│   │   │   └── mcp4725/   — MCP4725 驱动
│   │   └── spi_device/    — SPI 总线 + SD 卡
│   ├── temperature_monitor/ — 温度监控
│   ├── adjustable_psu/    — 电源应用层
│   ├── key/               — 按键驱动
│   ├── buzzer/            — 蜂鸣器驱动
│   └── perf_monitor/      — 性能监控
├── shouce/                — 数据手册 + 驱动文档
└── sdkconfig              — ESP-IDF 配置
```

## 文档

- `shouce/MP4201_驱动文档.md` — 寄存器完整说明 + API 参考
- `shouce/MP4201_寄存器手册_文字版.pdf` — 可搜索寄存器手册
- `shouce/MP4201_逐页原文整理.md` — 功能简介逐页整理
- `shouce/sht40.pdf` — SHT40 数据手册
- `shouce/mcp4725.pdf` — MCP4725 数据手册
- `CLAUDE.md` — Claude Code 上下文指引
