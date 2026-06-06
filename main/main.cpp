#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "adjustable_psu.h"
#include "buzzer.h"
#include "device_init.h"
#include "display_ui.h"
#include "encoder.h"
#include "key.h"
#include "perf_monitor.h"
#include "temperature_monitor.h"

extern "C" void app_main(void)
{
    StatusKey::GetInstance().InitKeys();
    Encoder::GetInstance().Init();
    DeviceInit::GetInstance().Init();
    TemperatureMonitor::GetInstance().TemperatureMonitorInit();
    AdjustablePSU::GetInstance().AdjustablePSUInit();
    PerfMonitor::GetInstance().Init();
    Buzzer::GetInstance().Init();
    DisplayUI::GetInstance().Init();
}
