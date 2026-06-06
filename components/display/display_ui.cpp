#include "display_ui.h"

#include "display_gfx.h"

static const char* TAG = "DisplayUI";

void DisplayUI::Init()
{
    Display::GetInstance().Init({});

    // 运行 GFX 层自带测试动画
    DisplayGFX::TestAll(Display::GetInstance());

    xTaskCreatePinnedToCore(DisplayUITask, "DisplayUITask", 8192, this, 2, nullptr, 1);

    ESP_LOGI(TAG, "Init DisplayUI");
}

void DisplayUI::DisplayUITask(void* pvParameters)
{
    static_cast<DisplayUI*>(pvParameters)->UpdateUI();
}

void DisplayUI::UpdateUI()
{
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
