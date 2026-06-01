#include "device_init.h"

#include "i2c_bus.h"
#include "spi_bus.h"

static const char* TAG = "Device";

void DeviceInit::Init()
{
    if (!I2CBusManager::GetInstance().Init())
    {
        ESP_LOGE(TAG, "I2C bus init failed");
        return;
    }

    if (!I2CBusManager::GetInstance().RegisterSHT40(0x44))
    {
        ESP_LOGE(TAG, "SHT40 0x44 register failed");
    }

    if (!SPIBusManager::GetInstance().Init())
    {
        ESP_LOGE(TAG, "SPI bus init failed");
        return;
    }

    ESP_LOGI(TAG, "Init device successfull!");
}