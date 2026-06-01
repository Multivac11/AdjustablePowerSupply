# CLAUDE.md

本文件为 Claude Code (claude.ai/code) 在此仓库中工作时提供指导。

## 项目概述

ESP-IDF v5.5.3 固件，目标平台为 ESP32-P4 (RISC-V)，实现可调电源控制。C++17，O3 优化，启用 fast-math。电源由 MP4201 芯片控制。外设包括 I2C 接口的 SHT40 温湿度传感器、SPI 接口的 SD 卡，并计划支持 RGB LED 矩阵显示。

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

## 架构

### 设备抽象层 (`components/device/`)

**基类：** `Device` (`device.h`) — 纯虚接口：`Probe()`、`Init()`、`Deinit()`。

**总线管理器**（单例，每个物理总线一个实例）：
- `I2CBusManager` (`i2c_device/i2c_bus.h`) — I2C_NUM_0，引脚 GPIO7 (SDA) / GPIO8 (SCL)，频率 100kHz。以 I2C 地址为键管理 `Device` 指针；通过 `GetDeviceByAddr<T>(addr)` 获取设备。
- `SPIBusManager` (`spi_device/spi_bus.h`) — SPI2_HOST，引脚 GPIO11 (MOSI) / GPIO12 (CLK) / GPIO13 (MISO)。以 CS 引脚为键管理设备；通过 `GetDeviceByCSPin<T>(cs_pin)` 获取设备。

**协议封装层：**
- `I2CDevice` (`i2c_device/i2c_device.h`) — 在 `Device` 基础上提供 `Write()`、`Read()`、`WriteThenRead()` 方法。
- `SPIDevice` (`spi_device/spi_device.h`) — 在 `Device` 基础上提供 `Transmit()` 方法。

**具体驱动：**
- `SHT40` (`i2c_device/sht40/sht40.h`) — 温湿度传感器，I2C 地址 0x44。发送命令 0xFD，读取 6 字节数据（含 CRC 校验）。
- `SDCard` (`spi_device/sd_card/sd_card.h`) — 基于 SPI 的 FAT 文件系统（`esp_vfs_fat_sdspi_mount`）。支持文件 I/O、字模读取、RGB 图像帧读取（36×16 像素）。还支持从打包文件或逐帧目录中读取动画。

**初始化编排器：** `DeviceInit`（单例，`device_init.h`）— 依次初始化 I2C 总线、注册 SHT40、初始化 SPI 总线。

### 应用层

**`TemperatureMonitor`** (`components/temperature_monitor/`) — 单例，从 `I2CBusManager` 获取 SHT40 指针，创建 FreeRTOS 任务（2048 字节栈，优先级 5），每 500ms 读取温湿度并通过 ESP_LOGI 输出日志。

**`app_main`** (`main/main.cpp`) — 依次调用 `DeviceInit::GetInstance().Init()` 和 `TemperatureMonitor::GetInstance().TemperatureMonitorInit()`。

### 依赖组件（来自 `REQUIRES`）

`driver`、`esp_timer`、`esp_driver_i2c`、`esp_driver_spi`、`esp_adc`、`esp_event`、`esp_wifi`、`esp_http_server`、`spiffs`、`nvs_flash`、`lwip`、`fatfs`。

WiFi、HTTP 服务器、SPIFFS、NVS 和 ADC 已在构建依赖中声明，但尚未编写应用层代码 — 属于规划中的功能。

### 硬件配置（来自 sdkconfig）

- Flash：16MB，QIO/DIO 模式，80MHz
- PSRAM：外部 SPIRAM，Hex 模式，200MHz
- 分区表：`partitions_singleapp.csv`（单工厂应用，无 OTA）
- JTAG/调试：OpenOCD 配置 `board/esp32p4-builtin.cfg`

## 代码风格

基于 Google 风格的 clang-format，类、枚举、结构体、联合体和函数使用 Allman 风格大括号（`.clang-format` 位于仓库根目录）。API 级别的文档注释使用中文；行内技术说明使用英文。

## 数据手册

- `shouce/sht40.pdf` — SHT40 传感器数据手册
- `shouce/MP4201功能简介与应用指南-仅供MP4201开源复刻活动使用.pdf` — MP4201 功能简介与应用指南
