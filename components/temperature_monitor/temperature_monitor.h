#pragma once

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_bus.h"

class TemperatureMonitor
{
   public:
    static TemperatureMonitor &GetInstance()
    {
        static TemperatureMonitor instance;
        return instance;
    }

    struct MonitorData
    {
        SHT40 *sht40_ = nullptr;
        SHT40::EnvParamsStruct env_params_;
        bool not_found_ = true;
    };

    TemperatureMonitor() = default;

    ~TemperatureMonitor() = default;

    bool TemperatureMonitorInit();

    static void TemperatureMonitorTask(void *pvParameters);

    void Monitor();

   private:
    MonitorData monitor_data_;
};