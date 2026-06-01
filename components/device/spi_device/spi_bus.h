#pragma once
#include <memory>
#include <vector>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "sd_card.h"
#include "spi_device.h"

#define SPI_MOSI_GPIO_NUM GPIO_NUM_11
#define SPI_CLK_GPIO_NUM GPIO_NUM_12
#define SPI_MISO_GPIO_NUM GPIO_NUM_13
#define SPI_HOST_ID SPI2_HOST

class SPIBusManager
{
   public:
    static SPIBusManager& GetInstance()
    {
        static SPIBusManager instance;
        return instance;
    }

    bool Init();

    void Deinit();

    bool RegisterSpiSdCard(gpio_num_t cs_pin);

    template <typename T>
    T* GetDeviceByCSPin(gpio_num_t cs_pin)
    {
        for (auto& dev : devices_)
        {
            auto* spi = static_cast<SPIDevice*>(dev.get());
            if (spi->GetCSPin() == cs_pin) return static_cast<T*>(dev.get());
        }
        return nullptr;
    }

    spi_host_device_t GetHost() const { return host_; }

   private:
    SPIBusManager() = default;

    ~SPIBusManager() = default;

    spi_host_device_t host_ = SPI_HOST_ID;

    std::vector<std::unique_ptr<Device>> devices_;
};
