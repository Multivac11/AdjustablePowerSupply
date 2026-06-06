#pragma once

#include <math.h>
#include <stdio.h>

#include "display.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class DisplayUI
{
   public:
    static DisplayUI& GetInstance()
    {
        static DisplayUI instance;

        return instance;
    }

    DisplayUI() = default;

    ~DisplayUI() = default;

    void Init();

    static void DisplayUITask(void*);

    void UpdateUI();
};