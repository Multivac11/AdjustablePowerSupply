#include "i2c_bus.h"

static const char* TAG = "I2C";

bool I2CBusManager::Init()
{
    i2c_master_bus_config_t bus_config = {};
    bus_config.i2c_port = I2C_MASTER_NUM, bus_config.sda_io_num = I2C_MASTER_SDA_IO,
    bus_config.scl_io_num = I2C_MASTER_SCL_IO, bus_config.clk_source = I2C_CLK_SRC_DEFAULT,
    bus_config.glitch_ignore_cnt = 7, bus_config.intr_priority = 0, bus_config.trans_queue_depth = 0,
    bus_config.flags = {.enable_internal_pullup = false, .allow_pd = false};

    if (i2c_new_master_bus(&bus_config, &bus_handle_) != ESP_OK)
    {
        ESP_LOGE(TAG, "Bus init failed");
        return false;
    }
    ESP_LOGI(TAG, "Bus init OK");

    return true;
}

void I2CBusManager::Deinit()
{
    devices_.clear();
    if (bus_handle_)
    {
        i2c_del_master_bus(bus_handle_);
        bus_handle_ = nullptr;
    }
}

bool I2CBusManager::RegisterSHT40(uint16_t addr)
{
    for (const auto& dev : devices_)
    {
        auto* i2c = static_cast<I2CDevice*>(dev.get());
        if (i2c->GetAddress() == addr)
        {
            ESP_LOGE(TAG, "Device already registered at 0x%02X", addr);
            return false;
        }
    }

    auto dev = std::make_unique<SHT40>(bus_handle_, addr);
    if (!dev->Init())
    {
        ESP_LOGE(TAG, "SHT40 init failed at 0x%02X", addr);
        return false;
    }

    devices_.push_back(std::move(dev));
    ESP_LOGI(TAG, "SHT40 registered at 0x%02X", addr);
    return true;
}

bool I2CBusManager::RegisterMP4201(uint16_t addr)
{
    for (const auto& dev : devices_)
    {
        auto* i2c = static_cast<I2CDevice*>(dev.get());
        if (i2c->GetAddress() == addr)
        {
            ESP_LOGE(TAG, "Device already registered at 0x%02X", addr);
            return false;
        }
    }

    auto dev = std::make_unique<MP4201>(bus_handle_, addr);
    if (!dev->Init())
    {
        ESP_LOGE(TAG, "MP4201 init failed at 0x%02X", addr);
        return false;
    }

    devices_.push_back(std::move(dev));
    ESP_LOGI(TAG, "MP4201 registered at 0x%02X", addr);
    return true;
}

bool I2CBusManager::RegisterMCP4725(uint16_t addr)
{
    for (const auto& dev : devices_)
    {
        auto* i2c = static_cast<I2CDevice*>(dev.get());
        if (i2c->GetAddress() == addr)
        {
            ESP_LOGE(TAG, "Device already registered at 0x%02X", addr);
            return false;
        }
    }

    auto dev = std::make_unique<MCP4725>(bus_handle_, addr);
    if (!dev->Init())
    {
        ESP_LOGE(TAG, "MCP4725 init failed at 0x%02X", addr);
        return false;
    }

    devices_.push_back(std::move(dev));
    ESP_LOGI(TAG, "MCP4725 registered at 0x%02X", addr);
    return true;
}
