#include "display.h"
#include <cstring>

#include "driver/gpio.h"
#include "esp_ldo_regulator.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "Display";

// ST7701S 初始化序列（来自官方例程，480×854 MIPI DSI）
static const st7701_lcd_init_cmd_t s_init_cmds[] = {
    //  {cmd, data, data_size, delay_ms}
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x13}, 5, 0},
    {0xEF, (uint8_t[]){0x08}, 1, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x10}, 5, 0},
    {0xC0, (uint8_t[]){0xE9, 0x03}, 2, 0},
    {0xC1, (uint8_t[]){0x10, 0x0C}, 2, 0},
    {0xC2, (uint8_t[]){0x20, 0x0A}, 2, 0},
    {0xCC, (uint8_t[]){0x10}, 2, 0},
    {0xB0, (uint8_t[]){0x0F, 0x1F, 0x28, 0x1C, 0x13, 0x07, 0x15, 0x0A, 0x08, 0x2F, 0x04, 0x13, 0x0F, 0x2D, 0x33, 0x1F},
     16, 0},
    {0xB1, (uint8_t[]){0x00, 0x1F, 0x25, 0x0F, 0x0F, 0x05, 0x0D, 0x07, 0x08, 0x23, 0x03, 0x0E, 0x0F, 0x27, 0x30, 0x1F},
     16, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x11}, 5, 0},
    {0xB0, (uint8_t[]){0x4D}, 1, 0},
    {0xB1, (uint8_t[]){0x66}, 1, 0},
    {0xB2, (uint8_t[]){0x84}, 1, 0},
    {0xB3, (uint8_t[]){0x80}, 1, 0},
    {0xB5, (uint8_t[]){0x4A}, 1, 0},
    {0xB7, (uint8_t[]){0x85}, 1, 0},
    {0xB8, (uint8_t[]){0x33}, 1, 0},
    {0xB9, (uint8_t[]){0x00, 0x1F}, 2, 0},
    {0xC1, (uint8_t[]){0x78}, 2, 0},
    {0xC2, (uint8_t[]){0x78}, 2, 0},
    {0xD0, (uint8_t[]){0x88}, 2, 0},
    {0xE0, (uint8_t[]){0x00, 0x00, 0x02}, 3, 0},
    {0xE1, (uint8_t[]){0x06, 0xA0, 0x08, 0xA0, 0x05, 0xA0, 0x07, 0xA0, 0x00, 0x44, 0x44}, 11, 0},
    {0xE2, (uint8_t[]){0x30, 0x30, 0x44, 0x44, 0x6E, 0xA0, 0x00, 0x00, 0x6E, 0xA0, 0x00, 0x00}, 12, 0},
    {0xE3, (uint8_t[]){0x00, 0x00, 0x33, 0x33}, 4, 0},
    {0xE4, (uint8_t[]){0x44, 0x44}, 2, 0},
    {0xE5, (uint8_t[]){0x0D, 0x69, 0x0A, 0xA0, 0x0F, 0x6B, 0x0A, 0xA0, 0x09, 0x65, 0x0A, 0xA0, 0x0B, 0x67, 0x0A, 0xA0},
     16, 0},
    {0xE6, (uint8_t[]){0x00, 0x00, 0x33, 0x33}, 4, 0},
    {0xE7, (uint8_t[]){0x44, 0x44}, 2, 0},
    {0xE8, (uint8_t[]){0x0C, 0x68, 0x0A, 0xA0, 0x0E, 0x6A, 0x0A, 0xA0, 0x08, 0x64, 0x0A, 0xA0, 0x0A, 0x66, 0x0A, 0xA0},
     16, 0},
    {0xE9, (uint8_t[]){0x36, 0x00}, 2, 0},
    {0xEB, (uint8_t[]){0x00, 0x01, 0xE4, 0xE4, 0x44, 0x88, 0x40}, 7, 0},
    {0xED, (uint8_t[]){0xFF, 0x45, 0x67, 0xFA, 0x01, 0x2B, 0xCF, 0xFF, 0xFF, 0xFC, 0xB2, 0x10, 0xAF, 0x76, 0x54, 0xFF},
     16, 0},
    {0xEF, (uint8_t[]){0x10, 0x0D, 0x04, 0x08, 0x3F, 0x1F}, 6, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x13}, 5, 0},
    {0xE8, (uint8_t[]){0x00, 0x0E}, 2, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x00}, 5, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x13}, 5, 0},
    {0xE8, (uint8_t[]){0x00, 0x0C}, 2, 10},
    {0xE8, (uint8_t[]){0x00, 0x00}, 2, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x00}, 5, 0},
    {0x35, (uint8_t[]){0x00}, 1, 0},
    {0x11, (uint8_t[]){0x00}, 0, 1500},
    {0x29, (uint8_t[]){0x00}, 0, 120},
};

bool Display::Init(const Config& cfg)
{
    width_ = cfg.h_res;
    height_ = cfg.v_res;

    // MIPI DSI PHY 供电（LDO）
    esp_ldo_channel_handle_t ldo = nullptr;
    esp_ldo_channel_config_t ldo_cfg = {
        .chan_id = cfg.ldo_chan,
        .voltage_mv = cfg.ldo_voltage_mv,
    };
    esp_ldo_acquire_channel(&ldo_cfg, &ldo);
    ESP_LOGI(TAG, "MIPI DSI PHY powered on");

    if (!InitPanel(cfg))
    {
        return false;
    }

    // 双缓冲 + 刷新回调（消除撕裂）
    refresh_done_ = xSemaphoreCreateBinary();
    esp_lcd_dpi_panel_event_callbacks_t cbs = {
        .on_refresh_done = OnRefreshDone,
    };
    esp_lcd_dpi_panel_register_event_callbacks(panel_, &cbs, refresh_done_);

    // 获取帧缓冲
    esp_lcd_dpi_panel_get_frame_buffer(panel_, 2, &fb_back_, &fb_front_);
    esp_lcd_panel_draw_bitmap(panel_, 0, 0, width_, height_, fb_front_);
    xSemaphoreTake(refresh_done_, pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "Init OK, %dx%d", width_, height_);
    return true;
}

Display::~Display()
{
    if (panel_) esp_lcd_panel_del(panel_);
    if (io_) esp_lcd_panel_io_del(io_);
    if (dsi_bus_) esp_lcd_del_dsi_bus(dsi_bus_);
}

bool Display::InitPanel(const Config& cfg)
{
    // 1. DSI 总线
    esp_lcd_dsi_bus_config_t bus_cfg = {
        .bus_id = cfg.dsi_bus_id,
        .num_data_lanes = (uint8_t)cfg.num_data_lanes,
        .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
        .lane_bit_rate_mbps = cfg.lane_bit_rate_mbps,
    };

    if (esp_lcd_new_dsi_bus(&bus_cfg, &dsi_bus_) != ESP_OK)
    {
        ESP_LOGE(TAG, "DSI bus create failed");
        return false;
    }

    // 2. Panel IO（DBI over DSI，用于发送 DCS 命令）
    esp_lcd_dbi_io_config_t io_cfg = ST7701_PANEL_IO_DBI_CONFIG();
    if (esp_lcd_new_panel_io_dbi(dsi_bus_, &io_cfg, &io_) != ESP_OK)
    {
        ESP_LOGE(TAG, "Panel IO create failed");
        return false;
    }

    // 3. DPI 配置（视频模式时序，字段按 struct 声明顺序）
    esp_lcd_dpi_panel_config_t dpi_cfg = {};
    dpi_cfg.virtual_channel = 0;
    dpi_cfg.dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT;
    dpi_cfg.dpi_clock_freq_mhz = cfg.dpi_clock_freq_mhz;
    dpi_cfg.pixel_format = cfg.px_format;
    dpi_cfg.in_color_format = LCD_COLOR_FMT_RGB565;
    dpi_cfg.out_color_format = LCD_COLOR_FMT_RGB565;
    dpi_cfg.num_fbs = 2;  // 双缓冲
    dpi_cfg.video_timing.h_size = cfg.h_res;
    dpi_cfg.video_timing.v_size = cfg.v_res;
    dpi_cfg.video_timing.hsync_pulse_width = cfg.hsync_pulse_width;
    dpi_cfg.video_timing.hsync_back_porch = cfg.hsync_back_porch;
    dpi_cfg.video_timing.hsync_front_porch = cfg.hsync_front_porch;
    dpi_cfg.video_timing.vsync_pulse_width = cfg.vsync_pulse_width;
    dpi_cfg.video_timing.vsync_back_porch = cfg.vsync_back_porch;
    dpi_cfg.video_timing.vsync_front_porch = cfg.vsync_front_porch;
    dpi_cfg.flags.use_dma2d = true;

    // 4. ST7701 vendor config
    st7701_vendor_config_t vendor_cfg = {
        .init_cmds = s_init_cmds,
        .init_cmds_size = sizeof(s_init_cmds) / sizeof(st7701_lcd_init_cmd_t),
        .mipi_config =
            {
                .dsi_bus = dsi_bus_,
                .dpi_config = &dpi_cfg,
            },
        .flags =
            {
                .use_mipi_interface = 1,
            },
    };

    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = cfg.rst_pin,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config = &vendor_cfg,
    };

    if (esp_lcd_new_panel_st7701(io_, &panel_cfg, &panel_) != ESP_OK)
    {
        ESP_LOGE(TAG, "Panel create failed");
        return false;
    }

    esp_lcd_panel_reset(panel_);
    esp_lcd_panel_init(panel_);
    esp_lcd_panel_disp_on_off(panel_, true);

    return true;
}

void Display::Flush()
{
    if (!panel_ || !fb_back_) return;

    esp_lcd_panel_draw_bitmap(panel_, 0, 0, width_, height_, fb_back_);
    xSemaphoreTake(refresh_done_, pdMS_TO_TICKS(1000));

    void* tmp = fb_back_;
    fb_back_ = fb_front_;
    fb_front_ = tmp;
}

bool Display::OnRefreshDone(esp_lcd_panel_handle_t panel, esp_lcd_dpi_panel_event_data_t* edata, void* user_ctx)
{
    SemaphoreHandle_t sem = (SemaphoreHandle_t)user_ctx;
    BaseType_t high_task_wakeup = pdFALSE;
    xSemaphoreGiveFromISR(sem, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}

void Display::Fill(uint16_t color)
{
    if (!fb_back_) return;
    size_t pixels = width_ * height_;
    uint16_t* buf = (uint16_t*)fb_back_;
    for (size_t i = 0; i < pixels; ++i)
    {
        buf[i] = color;
    }
}

void Display::DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (!fb_back_) return;
    if (x >= (uint16_t)width_ || y >= (uint16_t)height_) return;
    ((uint16_t*)fb_back_)[y * width_ + x] = color;
}
