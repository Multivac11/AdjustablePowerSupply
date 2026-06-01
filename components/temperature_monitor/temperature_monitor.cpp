#include "temperature_monitor.h"

static const char *TAG = "TemperatureMonitor";

bool TemperatureMonitor::TemperatureMonitorInit()
{
    monitor_data_.sht40_ = I2CBusManager::GetInstance().GetDeviceByAddr<SHT40>(0x44);
    if (monitor_data_.sht40_ != nullptr)
    {
        monitor_data_.not_found_ = false;
    }
    else
    {
        monitor_data_.not_found_ = true;
        ESP_LOGE(TAG, "SHT40 not found");

        return false;
    }

    xTaskCreate(TemperatureMonitorTask, "TemperatureMonitorTask", 4096, this, 5, nullptr);

    return true;
}

void TemperatureMonitor::TemperatureMonitorTask(void *pvParameters)
{
    static_cast<TemperatureMonitor *>(pvParameters)->Monitor();
}

void TemperatureMonitor::Monitor()
{
    while (true)
    {
        monitor_data_.sht40_->ReadEnvParams(monitor_data_.env_params_);
        // ESP_LOGI(TAG, "Temperature: %f, Humidity: %f", monitor_data_.env_params_.temperature,
        //          monitor_data_.env_params_.humidity);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
