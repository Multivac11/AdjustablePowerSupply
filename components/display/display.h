#pragma once

#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_st7701.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

// ST7701S MIPI DSI 480×854 显示屏驱动
class Display
{
   public:
    struct Config
    {
        uint16_t h_res = 480;
        uint16_t v_res = 854;
        // DPI 时序（来自官方例程）
        uint16_t hsync_pulse_width = 2;
        uint16_t hsync_back_porch = 30;
        uint16_t hsync_front_porch = 50;
        uint16_t vsync_pulse_width = 8;
        uint16_t vsync_back_porch = 10;
        uint16_t vsync_front_porch = 20;
        float lane_bit_rate_mbps = 940;
        float dpi_clock_freq_mhz = 15;
        int dsi_bus_id = 0;
        int num_data_lanes = 2;
        gpio_num_t rst_pin = GPIO_NUM_NC;
        uint8_t ldo_chan = 3;            // MIPI DSI PHY LDO 通道
        uint16_t ldo_voltage_mv = 2500;  // MIPI DSI PHY 电压 (mV)
        lcd_color_rgb_pixel_format_t px_format = LCD_COLOR_PIXEL_FORMAT_RGB565;
    };

    Display() = default;

    bool Init(const Config& cfg);
    bool Init();  // 使用默认 Config

    // 获取帧缓冲（驱动分配，直接写像素 RGB565）
    uint16_t* GetFramebuffer() { return (uint16_t*)fb_back_; }

    // 刷新到屏幕
    void Flush();

    // 填充纯色
    void Fill(uint16_t color);

    // 画像素点
    void DrawPixel(uint16_t x, uint16_t y, uint16_t color);

    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }

    ~Display();

   private:
    bool InitPanel(const Config& cfg);

    static bool OnRefreshDone(esp_lcd_panel_handle_t panel, esp_lcd_dpi_panel_event_data_t* edata, void* user_ctx);

    esp_lcd_dsi_bus_handle_t dsi_bus_ = nullptr;
    esp_lcd_panel_io_handle_t io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;
    void* fb_back_ = nullptr;   // 后端缓冲（安全写入）
    void* fb_front_ = nullptr;  // 前端缓冲（正在显示）
    SemaphoreHandle_t refresh_done_ = nullptr;

    int width_ = 0;
    int height_ = 0;
};
