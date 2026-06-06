#pragma once

#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ====== RGB565 颜色定义 ======
#define GFX_RED 0xF800
#define GFX_GREEN 0x07E0
#define GFX_BLUE 0x001F
#define GFX_WHITE 0xFFFF
#define GFX_BLACK 0x0000
#define GFX_YELLOW 0xFFE0
#define GFX_CYAN 0x07FF
#define GFX_MAGENTA 0xF81F
#define GFX_GRAY 0x8410
#define GFX_DARK 0x4208
#define GFX_ORANGE 0xFD20

// 软件图形驱动层 — 封装 Display 硬件驱动，提供画点、画线、画圆、写字符串等绘图原语
//
// 使用方式：
//   DisplayGFX gfx(display);
//   gfx.SetRotation(Rotation::k90);  // 逆时针旋转 90°
//   gfx.FillRect(10, 10, 100, 50, GFX_RED);
//   gfx.DrawString(10, 10, "Hello", GFX_WHITE, GFX_BLACK);
//
// 同时保留静态方法用于直接操作帧缓冲（无旋转，原始物理坐标）。
enum class Rotation
{
    k0 = 0,
    k90 = 90,    // 逆时针 90°
    k180 = 180,
    k270 = 270   // 逆时针 270°（等价于顺时针 90°）
};

class DisplayGFX
{
   public:
    explicit DisplayGFX(Display& display) : display_(display) {}

    // ====== 屏幕旋转 ======

    void SetRotation(Rotation rot);
    Rotation GetRotation() const { return rotation_; }

    // 逻辑宽高（自动跟随旋转交换）
    int GetWidth() const;
    int GetHeight() const;

    // ====== 实例方法（推荐）— 自动应用旋转、自动获取帧缓冲和尺寸 ======

    void DrawPixel(int x, int y, uint16_t color);
    void DrawHLine(int x, int y, int len, uint16_t color);
    void DrawVLine(int x, int y, int len, uint16_t color);
    void DrawLine(int x0, int y0, int x1, int y1, uint16_t color);
    void DrawRect(int x, int y, int rw, int rh, uint16_t color);
    void FillRect(int x, int y, int rw, int rh, uint16_t color);
    void DrawCircle(int cx, int cy, int r, uint16_t color);
    void FillCircle(int cx, int cy, int r, uint16_t color);

    // 绘制 ASCII 字符（6×8 点阵，码点 0x20–0x7F）
    // bg == color 时背景透明
    void DrawChar(int x, int y, char ch, uint16_t color, uint16_t bg);
    void DrawString(int x, int y, const char* str, uint16_t color, uint16_t bg);

    // ====== 静态方法 — 直接操作帧缓冲 ======

    static void DrawPixel(uint16_t* fb, int w, int h, int x, int y, uint16_t color);
    static void DrawHLine(uint16_t* fb, int w, int h, int x, int y, int len, uint16_t color);
    static void DrawVLine(uint16_t* fb, int w, int h, int x, int y, int len, uint16_t color);
    static void DrawLine(uint16_t* fb, int w, int h, int x0, int y0, int x1, int y1, uint16_t color);
    static void DrawRect(uint16_t* fb, int w, int h, int x, int y, int rw, int rh, uint16_t color);
    static void FillRect(uint16_t* fb, int w, int h, int x, int y, int rw, int rh, uint16_t color);
    static void DrawCircle(uint16_t* fb, int w, int h, int cx, int cy, int r, uint16_t color);
    static void FillCircle(uint16_t* fb, int w, int h, int cx, int cy, int r, uint16_t color);

    static constexpr int kFontWidth = 6;
    static constexpr int kFontHeight = 8;

    static void DrawChar(uint16_t* fb, int w, int h, int x, int y, char ch, uint16_t color, uint16_t bg);
    static void DrawString(uint16_t* fb, int w, int h, int x, int y, const char* str, uint16_t color, uint16_t bg);

    // ====== 测试程序 ======

    void TestFont();
    void TestBallCollision();
    void TestStarfield();
    void TestCubeRotation();
    void TestColorSquares();
    void TestAll();  // 依次运行全部测试

    Display& GetDisplay() { return display_; }

   private:
    void MapToPhysical(int lx, int ly, int* px, int* py) const;
    int HwWidth() const { return display_.GetWidth(); }
    int HwHeight() const { return display_.GetHeight(); }

    Display& display_;
    Rotation rotation_ = Rotation::k0;
};
