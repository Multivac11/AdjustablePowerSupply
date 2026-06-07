#include "adjustable_psu.h"

static const char *TAG = "AdjustablePSU";

bool AdjustablePSU::AdjustablePSUInit()
{
    freq_dac_ = I2CBusManager::GetInstance().GetDeviceByAddr<MCP4725>(DAC_FREQ_ADDR);
    mode_dac_ = I2CBusManager::GetInstance().GetDeviceByAddr<MCP4725>(DAC_MODE_ADDR);
    mp4201_ = I2CBusManager::GetInstance().GetDeviceByAddr<MP4201>(0x3F);
    if (freq_dac_ != nullptr && mode_dac_ != nullptr)
    {
        ESP_LOGI(TAG, "MCP4725 found");
    }
    else
    {
        ESP_LOGE(TAG, "MCP4725 not found");
        return false;
    }

    if (mp4201_ != nullptr)
    {
        ESP_LOGI(TAG, "MP4201 found");
    }
    else
    {
        ESP_LOGE(TAG, "MP4201 not found");
        return false;
    }

    // 上电初始化为默认配置
    SetFrequency(current_freq_);
    SetMode(current_mode_);
    mp4201_->SetInputRegulation(0.0f);  // 禁掉输入电压调节
    mp4201_->DisableOutput();
    mp4201_->SetDirection(MP4201::Direction::VIN_TO_OUT);
    mp4201_->SetSenseResistors(MP4201::SenseResistor::R5_MOHM, MP4201::SenseResistor::R5_MOHM);

    xTaskCreate(PowerSupplyTask, "PowerSupplyTask", 8192, this, 5, nullptr);

    return true;
}

void AdjustablePSU::PowerSupplyTask(void *pvParameters)
{
    static_cast<AdjustablePSU *>(pvParameters)->PowerSupply();
}

void AdjustablePSU::PowerSupply()
{
    while (true)
    {
        MP4201::ADCReadings adc;
        mp4201_->ReadADC(adc);

        for (int i = 0; i < listener_count_; ++i)
        {
            if (listeners_[i])
            {
                Event *p = &ev_;
                p->adc = adc;
                xQueueOverwrite(listeners_[i], p);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

bool AdjustablePSU::RegisterListener(QueueHandle_t queue)
{
    if (queue == nullptr || listener_count_ >= MAX_LISTENERS) return false;

    for (int i = 0; i < listener_count_; ++i)
    {
        if (listeners_[i] == queue) return true;
    }

    listeners_[listener_count_++] = queue;
    return true;
}

bool AdjustablePSU::UnregisterListener(QueueHandle_t queue)
{
    for (int i = 0; i < listener_count_; ++i)
    {
        if (listeners_[i] == queue)
        {
            for (int j = i; j < listener_count_ - 1; ++j) listeners_[j] = listeners_[j + 1];
            listeners_[--listener_count_] = nullptr;
            return true;
        }
    }
    return false;
}

bool AdjustablePSU::SetFrequency(MP4201::SwitchingFrequency freq)
{
    if (!freq_dac_)
    {
        return false;
    }

    float v = MP4201::FreqToVoltage(freq, DAC_VDD);
    if (!freq_dac_->SetOutputVoltage(v, DAC_VDD))
    {
        ESP_LOGE(TAG, "Set FREQ failed");
        return false;
    }

    current_freq_ = freq;
    ESP_LOGI(TAG, "FREQ set: %.2fV", v);
    return true;
}

bool AdjustablePSU::SetMode(MP4201::OperationMode mode)
{
    if (!mode_dac_)
    {
        return false;
    }

    float v = MP4201::ModeToVoltage(mode, DAC_VDD);
    if (!mode_dac_->SetOutputVoltage(v, DAC_VDD))
    {
        ESP_LOGE(TAG, "Set MODE failed");
        return false;
    }

    current_mode_ = mode;
    ESP_LOGI(TAG, "MODE set: %.2fV", v);
    return true;
}

bool AdjustablePSU::SaveConfig()
{
    if (!freq_dac_ || !mode_dac_)
    {
        return false;
    }

    freq_dac_->SaveToEEPROM();
    mode_dac_->SaveToEEPROM();
    ESP_LOGI(TAG, "Config saved to EEPROM");
    return true;
}
