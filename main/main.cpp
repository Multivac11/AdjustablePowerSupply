#include <stdio.h>

#include "device_init.h"
#include "temperature_monitor.h"

extern "C" void app_main(void)
{
    DeviceInit::GetInstance().Init();
    TemperatureMonitor::GetInstance().TemperatureMonitorInit();
}
