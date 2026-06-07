#pragma once

#include "adjustable_psu.h"
#include "display.h"
#include "display_gfx.h"
#include "esp_log.h"
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

    void Init();

    Display& GetDisplay() { return display_; }
    DisplayGFX& GetGFX() { return gfx_; }

   private:
    DisplayUI() : gfx_(display_) {}

    static void DisplayUITask(void*);

    void UpdateUI();

    static void AdjustableListenerTask(void*);

    void AdjustableListener();

   private:
    Display display_;  // 硬件驱动
    DisplayGFX gfx_;   // 软件图形层（引用 display_）

    AdjustablePSU::Event adjustable_ev_;
};
