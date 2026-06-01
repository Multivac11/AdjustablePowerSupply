#pragma once

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_bus.h"
#include "mp4201/mp4201.h"

// 可调电源应用层 — 通过 MCP4725 DAC 配置 MP4201 的 FREQ / MODE 引脚
class AdjustablePSU
{
   public:
    static AdjustablePSU& GetInstance()
    {
        static AdjustablePSU instance;
        return instance;
    }

    AdjustablePSU() = default;

    ~AdjustablePSU() = default;

    bool AdjustablePSUInit();

    // 设置开关频率（通过 0x60 MCP4725 → FREQ 引脚）
    bool SetFrequency(MP4201::SwitchingFrequency freq);

    // 设置工作模式（通过 0x61 MCP4725 → MODE 引脚）
    bool SetMode(MP4201::OperationMode mode);

    // 将当前 FREQ / MODE 配置写入 EEPROM，掉电后自动恢复
    bool SaveConfig();

    static void PowerSupplyTask(void* pvParameters);

    void PowerSupply();

   private:
    static constexpr float DAC_VDD = 5.0f;           // MCP4725 供电电压
    static constexpr uint16_t DAC_FREQ_ADDR = 0x60;  // A0=GND → FREQ
    static constexpr uint16_t DAC_MODE_ADDR = 0x61;  // A0=VDD → MODE

    MCP4725* freq_dac_ = nullptr;
    MCP4725* mode_dac_ = nullptr;
    MP4201* mp4201_ = nullptr;

    MP4201::SwitchingFrequency current_freq_ = MP4201::SwitchingFrequency::KHZ_400;
    MP4201::OperationMode current_mode_ = MP4201::OperationMode::FCCM;
};
