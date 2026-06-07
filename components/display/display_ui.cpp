#include "display_ui.h"

static const char* TAG = "DisplayUI";

void DisplayUI::Init()
{
    display_.Init();
    gfx_.SetRotation(Rotation::k270);

    // 运行 GFX 层自带测试动画
    // gfx_.TestAll();

    xTaskCreatePinnedToCore(DisplayUITask, "DisplayUITask", 8192, this, 1, nullptr, 1);
    xTaskCreatePinnedToCore(AdjustableListenerTask, "AdjustableListenerTask", 4096, this, 1, nullptr, 1);

    ESP_LOGI(TAG, "Init DisplayUI");
}

void DisplayUI::DisplayUITask(void* pvParameters)
{
    static_cast<DisplayUI*>(pvParameters)->UpdateUI();
}

void DisplayUI::AdjustableListenerTask(void* pvParameters)
{
    static_cast<DisplayUI*>(pvParameters)->AdjustableListener();
}

void DisplayUI::AdjustableListener()
{
    QueueHandle_t q = xQueueCreate(1, sizeof(AdjustablePSU::Event));
    AdjustablePSU::GetInstance().RegisterListener(q);

    while (true)
    {
        if (xQueueReceive(q, &adjustable_ev_, portMAX_DELAY) == pdTRUE)
        {
            ESP_LOGI(TAG, "AdjustableListener: Vin: %.2f, Vout: %.2f, Iin: %.2f,Iout: %.2f, Temp: %.2f",
                     adjustable_ev_.adc.Vin, adjustable_ev_.adc.Vout, adjustable_ev_.adc.Iin, adjustable_ev_.adc.Iout,
                     adjustable_ev_.adc.Temp);
        }
    }
}

void DisplayUI::UpdateUI()
{
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
