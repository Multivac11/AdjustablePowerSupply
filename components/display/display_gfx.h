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

// 软件图形驱动层 — 提供画点、画线、画圆、写字符串等绘图原语
// 所有函数直接操作帧缓冲 (RGB565, w*h 像素)，不依赖具体硬件
class DisplayGFX
{
   public:
    // ====== 基础绘图原语 ======

    // 画像素点（自动边界裁剪）
    static void DrawPixel(uint16_t* fb, int w, int h, int x, int y, uint16_t color);

    // 水平线（比 DrawLine 快，内部无乘法）
    static void DrawHLine(uint16_t* fb, int w, int h, int x, int y, int len, uint16_t color);

    // 垂直线
    static void DrawVLine(uint16_t* fb, int w, int h, int x, int y, int len, uint16_t color);

    // 任意直线 (Bresenham 算法)
    static void DrawLine(uint16_t* fb, int w, int h, int x0, int y0, int x1, int y1, uint16_t color);

    // 矩形边框
    static void DrawRect(uint16_t* fb, int w, int h, int x, int y, int rw, int rh, uint16_t color);

    // 填充矩形
    static void FillRect(uint16_t* fb, int w, int h, int x, int y, int rw, int rh, uint16_t color);

    // 圆形边框 (Midpoint 算法)
    static void DrawCircle(uint16_t* fb, int w, int h, int cx, int cy, int r, uint16_t color);

    // 填充圆形
    static void FillCircle(uint16_t* fb, int w, int h, int cx, int cy, int r, uint16_t color);

    // ====== 文字渲染 ======

    static constexpr int kFontWidth = 6;   // 字符宽度（像素）
    static constexpr int kFontHeight = 8;  // 字符高度（像素）

    // 绘制单个 ASCII 字符（6×8 点阵，码点 0x20–0x7F）
    // bg 为 GFX_TRANSPARENT (0xFFFF) 时不绘制背景
    static void DrawChar(uint16_t* fb, int w, int h, int x, int y, char ch, uint16_t color, uint16_t bg);

    // 绘制字符串（不支持中文，不自动换行，超出屏幕部分被裁剪）
    static void DrawString(uint16_t* fb, int w, int h, int x, int y, const char* str, uint16_t color, uint16_t bg);

    // ====== 测试程序 ======

    // 十球物理碰撞 + 中心引力井
    static void TestBallCollision(Display& display);

    // 星空视差（十字星芒）
    static void TestStarfield(Display& display);

    // 双立方体交织旋转 + 色彩循环
    static void TestCubeRotation(Display& display);

    // 多彩方块碰撞混战
    static void TestColorSquares(Display& display);

    // 字体展示测试 — 显示全部可打印 ASCII 字符
    static void TestFont(Display& display);

    // 运行全部测试
    static void TestAll(Display& display);

   private:
    DisplayGFX() = default;
};
