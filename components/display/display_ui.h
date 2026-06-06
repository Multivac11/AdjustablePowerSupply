#pragma once

#include "display.h"
#include "display_gfx.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// UI 应用层 — 单例，持有硬件驱动 (Display) 和软件图形层 (DisplayGFX)
//
// 外部获取方式：
//   auto& ui = DisplayUI::GetInstance();
//   ui.GetDisplay().Fill(GFX_BLACK);
//   ui.GetGFX().DrawString(10, 10, "Hello", GFX_WHITE, GFX_BLACK);
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

    Display display_;   // 硬件驱动
    DisplayGFX gfx_;    // 软件图形层（引用 display_）
};
