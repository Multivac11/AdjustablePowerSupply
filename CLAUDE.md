# CLAUDE.md

本文件为 Claude Code (claude.ai/code) 在此仓库中工作时提供指导。

## 项目概述

ESP-IDF v5.5.3 固件，目标平台为 ESP32-P4 (RISC-V)，实现基于 MP4201 的大功率可调数字电源。C++17，编译器 esp-clang 19.1.2。sdkconfig 设为 Debug 优化，但组件级 CMakeLists.txt 覆写为 `-O3 -ffast-math`。

## 构建/运行

```bash
idf.py build              # 编译 (Ninja)；输出 build/AdjustablePowerSupply.bin
idf.py flash              # UART 烧录，端口 /dev/ttyUSB0
idf.py monitor            # 串口监视器 (115200 波特率)
idf.py build flash monitor  # 完整流程
```

- IDF 目标已设置为 `esp32p4`（已写入 sdkconfig）。
- 构建会生成 `build/compile_commands.json`，clangd 直接使用该文件（路径见 `.vscode/settings.json`）。
- 本项目暂无测试框架。

## 硬件引脚分配

| GPIO | 功能 | 说明 |
|------|------|------|
| 1–5 | 按键输入 | 内部上拉，低电平有效 |
| 7 | I2C SDA | 100kHz |
| 8 | I2C SCL | |
| 10 | 编码器 A 相 / SD CS | EC11 旋钮 (PCNT) + SD 卡 SPI 片选 |
| 11 | 编码器 B 相 / SPI MOSI | EC11 旋钮 (PCNT) + SPI 数据 |
| 12 | 按键输入 / SPI CLK | key6 + SPI 时钟 |
| 13 | SPI MISO | |
| 21 | 蜂鸣器 PWM | LEDC 无源蜂鸣器 |
| 37/38 | 调试 UART | |

> **注意：** GPIO10/11/12 存在复用 — 编码器使用硬件 PCNT 外设（仅需 GPIO 输入），SPI 也使用这些引脚。当前 SPI (SD 卡) 初始化已被注释掉。

## 架构

### 启动顺序 (`main/main.cpp`)

```
app_main
  ├── StatusKey::InitKeys()         — 按键扫描 Task (core1, prio 2, stack 4096)
  ├── Encoder::Init()               — EC11 编码器 PCNT 正交解码 (prio 4, stack 2048)
  ├── DeviceInit::Init()            — I2C 总线 + 全部 I2C 设备注册
  │     ├── I2CBusManager::Init()   — I2C_NUM_0, GPIO7/8, 100kHz
  │     ├── RegisterSHT40(0x44)
  │     ├── RegisterMP4201(0x3F)
  │     ├── RegisterMCP4725(0x60)   — FREQ DAC (A0=GND)
  │     └── RegisterMCP4725(0x61)   — MODE DAC (A0=VDD)
  ├── TemperatureMonitor::Init()    — SHT40 温湿度采集 (prio 5, stack 2048, 500ms)
  ├── AdjustablePSU::Init()         — 电源应用层 (prio 5, stack 8192)
  ├── PerfMonitor::Init()           — CPU/内存监控 (core1, prio 1, stack 4096, 1s)
  ├── Buzzer::Init()                — 蜂鸣器 PWM + 命令监听 Task (prio 3, stack 2048)
  └── Display::Init() + DisplayTest — ST7701S MIPI DSI 480×854 LCD + 动画测试
```

### 设备驱动层次

```
Device (纯虚基类: Probe / Init / Deinit)
  ├── I2CDevice (+ Write / Read / WriteThenRead)
  │     ├── SHT40       — 温湿度传感器 (0x44), 发送 0xFD 读 6 字节含 CRC
  │     ├── MP4201      — 电源控制器 (0x3F), PMBus 协议, WriteByte/Word + ReadByte/Word
  │     └── MCP4725     — 12-bit DAC (0x60/0x61), WriteVolatile + WriteEEPROM
  └── SPIDevice (+ Transmit)
        └── SDCard      — SPI FAT 文件系统, 字模/动画帧读取 (36×16 像素矩阵)
```

### 总线管理器（单例）

- **I2CBusManager** (`components/device/i2c_device/i2c_bus.h`) — 管理 `std::vector<std::unique_ptr<Device>>`，通过 `GetDeviceByAddr<T>(addr)` 按 I2C 地址查找设备。
- **SPIBusManager** (`components/device/spi_device/spi_bus.h`) — SPI2_HOST，通过 `GetDeviceByCSPin<T>(cs_pin)` 按 CS 引脚查找设备。当前 SD 卡初始化已被注释。

### 应用层组件

**AdjustablePSU** (`components/adjustable_psu/`) — 电源控制应用层。从 I2CBusManager 获取 MP4201 (0x3F) 和两个 MCP4725 DAC 指针。通过 DAC 设置 MP4201 的 FREQ/MODE 引脚电压来控制开关频率和 PFM/FCCM 模式。内部 Task 每 500ms 读取一次 ADC。支持 `SaveConfig()` 将 DAC 值写入 EEPROM。

**MP4201** (`components/device/i2c_device/mp4201/`) — 100V/25A 同步双向升降压控制器，PMBus 接口。核心功能：
- 输出电压 3.2–81.92V (20mV/step, 内部反馈比 32)
- 输出/输入电流限制 0.5–25A (50mA/step)
- 双向功率流 (VIN_TO_OUT / OUT_TO_VIN)
- FREQ 引脚 (4 档: 200/400/600/1000 kHz) 和 MODE 引脚 (PFM/FCCM + 展频) 由外部 DAC 设定
- 10-bit ADC 读数: VIN, IIN, VOUT, IOUT, 芯片温度
- OCP/OVP/OTP 保护, UVLO, 死区时间, 负载线补偿等

**MCP4725** (`components/device/i2c_device/mcp4725/`) — 12-bit 轨到轨 DAC，VREF=VDD。两块分别接 MP4201 的 FREQ (0x60) 和 MODE (0x61) 引脚。支持 volatile 输出和 EEPROM 保存。

**StatusKey** (`components/key/`) — 6 路按键扫描。自动区分短按/长按（默认阈值 2000ms），通过 FreeRTOS Queue 广播 `Event` 给已注册监听者（最多 10 个）。按下时标记 `KEY_LONG`，松开未达阈值标记 `KEY_SHORT`。扫描周期 20ms。

**Encoder** (`components/key/encoder.h`) — EC11 旋钮编码器。使用硬件 PCNT 外设做正交解码，A/B 相默认 GPIO10/11，5µs 毛刺滤波，方向冷却 400ms 抑制回弹。通过 Queue 广播 ±1 步进事件。

**Display** (`components/display/`) — ST7701S MIPI DSI 480×854 LCD 驱动。双缓冲 + DMA2D 消除撕裂。提供 `Fill()`、`DrawPixel()`、`Flush()` 等基础绘图接口。通过 LDO 通道 3 提供 MIPI DSI PHY 供电 (2.5V)。

**Buzzer** (`components/buzzer/`) — 无源蜂鸣器，LEDC PWM 驱动 (GPIO21, 10-bit 分辨率)。内部 Task 监听命令队列 (长 8)，支持同步 (`Beep`/`Alarm`/`Play`) 和异步 (`NotifyBeep`/`NotifyAlarm`/`NotifyStop`) 两种调用方式。预定义 C4–G5 音阶。音量 0–100%。

**PerfMonitor** (`components/perf_monitor/`) — CPU 使用率 + 空闲堆内存监控。通过空闲任务 RunTimeCounter 差值计算 CPU 占用，避免了 `uxTaskGetSystemState` 的长临界区问题。通过 Queue 广播 `Event{cpu0_, cpu1_, free_heap_}`。

**TemperatureMonitor** (`components/temperature_monitor/`) — 从 I2CBusManager 获取 SHT40 指针，每 500ms 读取温湿度并 ESP_LOGI 输出。

### 依赖组件 (REQUIRES)

`driver`, `esp_timer`, `esp_driver_i2c`, `esp_driver_spi`, `esp_lcd`, `esp_lcd_st7701`, `esp_adc`, `esp_event`, `esp_wifi`, `esp_http_server`, `spiffs`, `nvs_flash`, `lwip`, `fatfs`。

WiFi、HTTP 服务器、SPIFFS、NVS 和 ADC 已在构建依赖中声明，但尚未编写应用层代码 — 属于规划中的功能。

### 硬件配置（来自 sdkconfig）

- Flash：16MB，QIO 模式，80MHz
- PSRAM：外部 SPIRAM，Hex 模式，200MHz，通过 malloc 使用
- 分区表：`partitions_singleapp.csv`（单工厂应用，无 OTA）
- FATFS：2 个卷，扇区 4096 字节，优先从 PSRAM 分配
- JTAG/调试：OpenOCD 配置 `board/esp32p4-builtin.cfg`

## 代码风格

基于 Google 风格的 clang-format，Allman 大括号（类/枚举/结构体/联合体/函数前换行，命名空间不换行）。4 空格缩进，120 列宽。指针左对齐 (`int* p`)。Include 按类别 Regroup 排序。API 级别的文档注释使用中文；行内技术说明使用英文。

## 数据手册

- `shouce/sht40.pdf` — SHT40 传感器数据手册
- `shouce/mcp4725.pdf` — MCP4725 DAC 数据手册
- `shouce/MP4201功能简介与应用指南-仅供MP4201开源复刻活动使用.pdf` — MP4201 功能简介与应用指南
- `shouce/MP4201_寄存器手册_文字版.pdf` — 可搜索寄存器手册
- `shouce/MP4201_逐页原文整理.md` — 功能简介逐页整理
- `shouce/MP4201_逐页原文整理.md` — 功能简介逐页整理，含寄存器位描述
