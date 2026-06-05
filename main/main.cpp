#include <stdio.h>

#include "adjustable_psu.h"
#include "buzzer.h"
#include "device_init.h"
#include "key.h"
#include "perf_monitor.h"
#include "temperature_monitor.h"

extern "C" void app_main(void)
{
    StatusKey::GetInstance().InitKeys();
    DeviceInit::GetInstance().Init();
    TemperatureMonitor::GetInstance().TemperatureMonitorInit();
    AdjustablePSU::GetInstance().AdjustablePSUInit();
    PerfMonitor::GetInstance().Init();
    Buzzer::GetInstance().Init();
}
