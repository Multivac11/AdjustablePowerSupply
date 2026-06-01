#pragma once

#include "device.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"

class SPIDevice : public Device
{
   public:
    SPIDevice(spi_host_device_t host, gpio_num_t cs_pin, const char* tag, int clock_speed_hz = 10 * 1000 * 1000);

    ~SPIDevice() override;

    bool Probe() override;

    bool Init() override;

    void Deinit() override;

    gpio_num_t GetCSPin() const { return cs_pin_; }

   protected:
    // 派生类可直接用的底层传输
    esp_err_t Transmit(const uint8_t* tx, size_t tx_len, uint8_t* rx, size_t rx_len);

    spi_host_device_t host_;

    spi_device_handle_t dev_handle_ = nullptr;

    gpio_num_t cs_pin_;

    int clock_speed_hz_;

    const char* tag_;
};
