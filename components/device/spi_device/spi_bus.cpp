#include "spi_bus.h"

static const char* TAG = "SPI";

bool SPIBusManager::Init()
{
    spi_bus_config_t bus_config = {};
    bus_config.flags = SPICOMMON_BUSFLAG_MASTER;
    bus_config.isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO;
    bus_config.max_transfer_sz = 240 * 240 * 2;
    bus_config.miso_io_num = SPI_MISO_GPIO_NUM;
    bus_config.mosi_io_num = SPI_MOSI_GPIO_NUM;
    bus_config.sclk_io_num = SPI_CLK_GPIO_NUM;
    bus_config.quadhd_io_num = -1;
    bus_config.quadwp_io_num = -1;

    if (spi_bus_initialize(host_, &bus_config, SPI_DMA_CH_AUTO) != ESP_OK)
    {
        ESP_LOGE(TAG, "Bus init failed");
        return false;
    }
    ESP_LOGI(TAG, "Bus init OK");
    return true;
}

void SPIBusManager::Deinit()
{
    devices_.clear();
    spi_bus_free(host_);
}

bool SPIBusManager::RegisterSpiSdCard(gpio_num_t cs_pin)
{
    for (const auto& dev : devices_)
    {
        auto* spi = static_cast<SPIDevice*>(dev.get());
        if (spi->GetCSPin() == cs_pin)
        {
            ESP_LOGE(TAG, "Device already registered at CS=%d", cs_pin);
            return false;
        }
    }

    auto dev = std::make_unique<SDCard>(host_, cs_pin);
    if (!dev->Init())
    {
        ESP_LOGE(TAG, "SD card init failed at CS=%d", cs_pin);
        return false;
    }

    devices_.push_back(std::move(dev));
    ESP_LOGI(TAG, "SD card registered at CS=%d", cs_pin);
    return true;
}
