#include "sht40.h"

static const char* TAG = "SHT40";

SHT40::SHT40(i2c_master_bus_handle_t bus, uint16_t addr) : I2CDevice(bus, addr, TAG)
{
}

bool SHT40::Init()
{
    if (!I2CDevice::Init())
    {
        return false;
    }

    ESP_LOGI(TAG, "Init SHT40 success");

    return true;
}

bool SHT40::ReadEnvParams(EnvParamsStruct& params)
{
    uint8_t cmd = 0xFD;
    uint8_t buffer[6];

    esp_err_t ret = Write(&cmd, 1);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send measure command");
        return false;
    }

    vTaskDelay(pdMS_TO_TICKS(10));

    ret = Read(buffer, 6);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read SHT40 data");
        return false;
    }

    uint16_t recovery_temper = ((uint16_t)buffer[0] << 8) | buffer[1];
    params.temperature = -45 + 175 * ((float)recovery_temper / 65535);
    uint16_t recovery_hum = ((uint16_t)buffer[3] << 8) | buffer[4];
    params.humidity = -6 + 125 * ((float)recovery_hum / 65535);

    if (params.humidity >= 100)  // 根据数据手册编写
    {
        params.humidity = 100;
    }
    else if (params.humidity <= 0)
    {
        params.humidity = 0;
    }

    return true;
}
