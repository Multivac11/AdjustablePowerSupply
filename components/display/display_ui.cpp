#include "display_ui.h"

static const char* TAG = "DisplayUI";

void DisplayUI::Init()
{
    display_.Init();
    gfx_.SetRotation(Rotation::k270);

    xTaskCreatePinnedToCore(DisplayUITask, "DisplayUITask", 8192, this, 1, nullptr, 1);

    ESP_LOGI(TAG, "Init DisplayUI");
}

void DisplayUI::DisplayUITask(void* pvParameters)
{
    static_cast<DisplayUI*>(pvParameters)->UpdateUI();
}

void DisplayUI::UpdateUI()
{
    QueueHandle_t q = xQueueCreate(1, sizeof(AdjustablePSU::Event));
    AdjustablePSU::GetInstance().RegisterListener(q);

    int w = gfx_.GetWidth();
    char buf[128];

    while (true)
    {
        if (xQueueReceive(q, &adjustable_ev_, portMAX_DELAY) == pdTRUE)
        {
            auto& e = adjustable_ev_;
            ESP_LOGI(TAG, "VIN:%.2f VOUT:%.2f IIN:%.2f IOUT:%.2f T:%.0fC  STS:0x%04X TSTS:0x%02X",
                     e.adc.Vin, e.adc.Vout, e.adc.Iin, e.adc.Iout, e.adc.Temp,
                     e.status.raw, e.temp_status.raw);

            gfx_.GetDisplay().Fill(GFX_BLACK);

            // === 大字：主要数据 (24x48) ===
            gfx_.SetFont(&kFont24x48);

            snprintf(buf, sizeof(buf), "VIN: %5.2fV   VOUT: %5.2fV", e.adc.Vin, e.adc.Vout);
            gfx_.DrawString(10, 10, buf, GFX_WHITE, GFX_BLACK);

            snprintf(buf, sizeof(buf), "IIN: %5.2fA   IOUT: %5.2fA", e.adc.Iin, e.adc.Iout);
            gfx_.DrawString(10, 62, buf, GFX_WHITE, GFX_BLACK);

            snprintf(buf, sizeof(buf), "TEMP: %.0f C", e.adc.Temp);
            gfx_.DrawString(10, 114, buf, GFX_WHITE, GFX_BLACK);

            // 分隔线
            gfx_.DrawHLine(0, 175, w, GFX_GRAY);

            // === 小字：状态寄存器 (8x16) ===
            gfx_.SetFont(&kFont8x16);

            uint16_t st = e.status.raw;
            snprintf(buf, sizeof(buf), "STATUS:0x%04X SCP=%d IOV=%d PG=%d IIC=%d VOV=%d IOC=%d TEMP=%d CRC=%d",
                     st, e.status.scp_fault(), e.status.input_ov_fault(), e.status.pg_fault(),
                     e.status.iin_oc_fault(), e.status.vout_ov_fault(), e.status.iout_oc_fault(),
                     e.status.temperature_fault(), e.status.crc_error());
            gfx_.DrawString(10, 185, buf, GFX_WHITE, GFX_BLACK);

            uint8_t ts = e.temp_status.raw;
            snprintf(buf, sizeof(buf), "TEMP_ST:0x%02X OT=%d WRN=%d NTC=%d",
                     ts, e.temp_status.ot_fault(), e.temp_status.ot_warning(), e.temp_status.ntc_fault());
            gfx_.DrawString(10, 205, buf, GFX_WHITE, GFX_BLACK);

            gfx_.GetDisplay().Flush();
        }
    }
}
