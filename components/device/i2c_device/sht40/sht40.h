#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_device.h"

class SHT40 : public I2CDevice
{
   public:
    explicit SHT40(i2c_master_bus_handle_t bus, uint16_t addr);

    bool Init() override;

    struct EnvParamsStruct
    {
        float temperature;

        float humidity;
    };

    bool ReadEnvParams(EnvParamsStruct& params);
};
