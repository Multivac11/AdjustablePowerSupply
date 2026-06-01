#include "spi_device.h"

SPIDevice::SPIDevice(spi_host_device_t host, gpio_num_t cs_pin, const char* tag, int clock_speed_hz)
    : host_(host), cs_pin_(cs_pin), clock_speed_hz_(clock_speed_hz), tag_(tag)
{
}

SPIDevice::~SPIDevice()
{
    Deinit();
}

bool SPIDevice::Probe()
{
    // SPI 没有类似 I2C 的探测机制，直接返回 true
    ESP_LOGI(tag_, "Probe OK (CS=%d)", cs_pin_);
    return true;
}

bool SPIDevice::Init()
{
    if (!Probe()) return false;

    spi_device_interface_config_t dev_config = {};
    dev_config.mode = 0;
    dev_config.clock_speed_hz = clock_speed_hz_;
    dev_config.spics_io_num = cs_pin_;
    dev_config.queue_size = 1;

    esp_err_t ret = spi_bus_add_device(host_, &dev_config, &dev_handle_);
    if (ret != ESP_OK)
    {
        ESP_LOGE(tag_, "Failed to add to bus (CS=%d)", cs_pin_);
        return false;
    }
    ESP_LOGI(tag_, "Registered on bus (CS=%d)", cs_pin_);
    return true;
}

void SPIDevice::Deinit()
{
    if (dev_handle_)
    {
        spi_bus_remove_device(dev_handle_);
        dev_handle_ = nullptr;
    }
}

esp_err_t SPIDevice::Transmit(const uint8_t* tx, size_t tx_len, uint8_t* rx, size_t rx_len)
{
    spi_transaction_t trans = {};
    trans.length = tx_len * 8;
    trans.rxlength = rx_len * 8;
    trans.tx_buffer = tx;
    trans.rx_buffer = rx;
    return spi_device_transmit(dev_handle_, &trans);
}
