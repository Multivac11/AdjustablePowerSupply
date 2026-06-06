#include "display_gfx.h"

#include <cstdlib>
#include <cstring>

// ====== 字体渲染（C51 逐行格式，MSB=左） ======
// 字库按实际像素宽度选择存储类型：
//   w≤8  → uint8_t [95][h]，  每行 1 字节
//   w≤16 → uint16_t[95][h]，  每行 1 个 uint16_t
//   w≤32 → uint32_t[95][h]，  每行 1 个 uint32_t（左对齐）

static void DrawGlyph(uint16_t* fb, int fb_w, int fb_h, int x, int y, char ch, const Font& font, uint16_t color,
                      uint16_t bg)
{
    if (ch < 0x20 || ch > 0x7F) ch = '?';
    int fw = font.w, fh = font.h;
    if (x + fw <= 0 || x >= fb_w || y + fh <= 0 || y >= fb_h) return;

    // 先填充背景
    if (bg != color) DisplayGFX::FillRect(fb, fb_w, fb_h, x, y, fw, fh, bg);

    int idx = ch - 0x20;

    if (fw <= 8)
    {
        const uint8_t* glyphs = (const uint8_t*)font.data;
        const uint8_t* glyph = glyphs + idx * fh;
        for (int row = 0; row < fh; ++row)
        {
            int py = y + row;
            if (py < 0 || py >= fb_h) continue;
            for (int col = 0; col < fw; ++col)
            {
                int px = x + col;
                if (px < 0 || px >= fb_w) continue;
                if (glyph[row] & (0x80 >> col)) fb[py * fb_w + px] = color;
            }
        }
    }
    else if (fw <= 16)
    {
        const uint16_t* glyphs = (const uint16_t*)font.data;
        const uint16_t* glyph = glyphs + idx * fh;
        for (int row = 0; row < fh; ++row)
        {
            int py = y + row;
            if (py < 0 || py >= fb_h) continue;
            for (int col = 0; col < fw; ++col)
            {
                int px = x + col;
                if (px < 0 || px >= fb_w) continue;
                if (glyph[row] & (0x8000 >> col)) fb[py * fb_w + px] = color;
            }
        }
    }
    else
    {
        const uint32_t* glyphs = (const uint32_t*)font.data;
        const uint32_t* glyph = glyphs + idx * fh;
        for (int row = 0; row < fh; ++row)
        {
            int py = y + row;
            if (py < 0 || py >= fb_h) continue;
            for (int col = 0; col < fw; ++col)
            {
                int px = x + col;
                if (px < 0 || px >= fb_w) continue;
                if (glyph[row] & (0x80000000 >> col)) fb[py * fb_w + px] = color;
            }
        }
    }
}

// ====== 基础绘图原语 ======

void DisplayGFX::DrawPixel(uint16_t* fb, int w, int h, int x, int y, uint16_t color)
{
    if (x < 0 || x >= w || y < 0 || y >= h) return;
    fb[y * w + x] = color;
}

void DisplayGFX::DrawHLine(uint16_t* fb, int w, int h, int x, int y, int len, uint16_t color)
{
    if (y < 0 || y >= h || len <= 0) return;
    if (x < 0)
    {
        len += x;
        x = 0;
    }
    if (x + len > w) len = w - x;
    if (len <= 0) return;
    for (int i = 0; i < len; ++i) fb[y * w + x + i] = color;
}

void DisplayGFX::DrawVLine(uint16_t* fb, int w, int h, int x, int y, int len, uint16_t color)
{
    if (x < 0 || x >= w || len <= 0) return;
    if (y < 0)
    {
        len += y;
        y = 0;
    }
    if (y + len > h) len = h - y;
    if (len <= 0) return;
    for (int i = 0; i < len; ++i) fb[(y + i) * w + x] = color;
}

void DisplayGFX::DrawLine(uint16_t* fb, int w, int h, int x0, int y0, int x1, int y1, uint16_t color)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (1)
    {
        if (x0 >= 0 && x0 < w && y0 >= 0 && y0 < h) fb[y0 * w + x0] = color;
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void DisplayGFX::DrawRect(uint16_t* fb, int w, int h, int x, int y, int rw, int rh, uint16_t color)
{
    DrawHLine(fb, w, h, x, y, rw, color);              // top
    DrawHLine(fb, w, h, x, y + rh - 1, rw, color);     // bottom
    DrawVLine(fb, w, h, x, y + 1, rh - 2, color);      // left
    DrawVLine(fb, w, h, x + rw - 1, y + 1, rh - 2, color);  // right
}

void DisplayGFX::FillRect(uint16_t* fb, int w, int h, int x, int y, int rw, int rh, uint16_t color)
{
    if (x < 0)
    {
        rw += x;
        x = 0;
    }
    if (y < 0)
    {
        rh += y;
        y = 0;
    }
    if (x + rw > w) rw = w - x;
    if (y + rh > h) rh = h - y;
    if (rw <= 0 || rh <= 0) return;
    for (int row = 0; row < rh; ++row)
        for (int col = 0; col < rw; ++col) fb[(y + row) * w + x + col] = color;
}

void DisplayGFX::DrawCircle(uint16_t* fb, int w, int h, int cx, int cy, int r, uint16_t color)
{
    if (r <= 0) return;
    int x = 0, y2 = r, err = 3 - 2 * r;
    while (y2 >= x)
    {
        DrawPixel(fb, w, h, cx + x, cy + y2, color);
        DrawPixel(fb, w, h, cx + x, cy - y2, color);
        DrawPixel(fb, w, h, cx - x, cy + y2, color);
        DrawPixel(fb, w, h, cx - x, cy - y2, color);
        DrawPixel(fb, w, h, cx + y2, cy + x, color);
        DrawPixel(fb, w, h, cx + y2, cy - x, color);
        DrawPixel(fb, w, h, cx - y2, cy + x, color);
        DrawPixel(fb, w, h, cx - y2, cy - x, color);
        if (err < 0)
            err += 4 * x + 6;
        else
        {
            err += 4 * (x - y2) + 10;
            y2--;
        }
        x++;
    }
}

void DisplayGFX::FillCircle(uint16_t* fb, int w, int h, int cx, int cy, int r, uint16_t color)
{
    if (r <= 0) return;
    int x = 0, y2 = r, err = 3 - 2 * r;
    while (y2 >= x)
    {
        DrawHLine(fb, w, h, cx - x, cy + y2, 2 * x + 1, color);
        DrawHLine(fb, w, h, cx - x, cy - y2, 2 * x + 1, color);
        DrawHLine(fb, w, h, cx - y2, cy + x, 2 * y2 + 1, color);
        DrawHLine(fb, w, h, cx - y2, cy - x, 2 * y2 + 1, color);
        if (err < 0)
            err += 4 * x + 6;
        else
        {
            err += 4 * (x - y2) + 10;
            y2--;
        }
        x++;
    }
}

// ====== 文字渲染 ======

// 静态版本 — 需显式传入字体
void DisplayGFX::DrawChar(uint16_t* fb, int w, int h, int x, int y, char ch, const Font& font, uint16_t color,
                           uint16_t bg)
{
    DrawGlyph(fb, w, h, x, y, ch, font, color, bg);
}

void DisplayGFX::DrawString(uint16_t* fb, int w, int h, int x, int y, const char* str, const Font& font, uint16_t color,
                             uint16_t bg)
{
    if (!str) return;
    int cx = x;
    while (*str)
    {
        DrawGlyph(fb, w, h, cx, y, *str, font, color, bg);
        cx += font.w;
        if (cx >= w) break;
        str++;
    }
}

// ====== 旋转 / 坐标映射 ======

void DisplayGFX::SetRotation(Rotation rot)
{
    rotation_ = rot;
}

int DisplayGFX::GetWidth() const
{
    return (rotation_ == Rotation::k90 || rotation_ == Rotation::k270) ? HwHeight() : HwWidth();
}

int DisplayGFX::GetHeight() const
{
    return (rotation_ == Rotation::k90 || rotation_ == Rotation::k270) ? HwWidth() : HwHeight();
}

void DisplayGFX::MapToPhysical(int lx, int ly, int* px, int* py) const
{
    switch (rotation_)
    {
        default:
        case Rotation::k0:
            *px = lx;
            *py = ly;
            break;
        case Rotation::k90:
            // 逆时针 90°: 物理右边缘 → 逻辑上边缘, 物理上边缘 → 逻辑左边缘
            *px = HwWidth() - 1 - ly;
            *py = lx;
            break;
        case Rotation::k180:
            *px = HwWidth() - 1 - lx;
            *py = HwHeight() - 1 - ly;
            break;
        case Rotation::k270:
            // 逆时针 270°: 物理左边缘 → 逻辑上边缘, 物理下边缘 → 逻辑左边缘
            *px = ly;
            *py = HwHeight() - 1 - lx;
            break;
    }
}

// ====== 实例方法（自动应用旋转） ======

void DisplayGFX::DrawPixel(int x, int y, uint16_t color)
{
    if (rotation_ == Rotation::k0)
    {
        DrawPixel(display_.GetFramebuffer(), HwWidth(), HwHeight(), x, y, color);
        return;
    }
    int px, py;
    MapToPhysical(x, y, &px, &py);
    DrawPixel(display_.GetFramebuffer(), HwWidth(), HwHeight(), px, py, color);
}

void DisplayGFX::DrawHLine(int x, int y, int len, uint16_t color)
{
    if (len <= 0) return;
    if (rotation_ == Rotation::k0)
    {
        DrawHLine(display_.GetFramebuffer(), HwWidth(), HwHeight(), x, y, len, color);
        return;
    }
    // 旋转时映射首尾端点后走 Bresenham（90° 时自动退化为垂直线）
    int px0, py0, px1, py1;
    MapToPhysical(x, y, &px0, &py0);
    MapToPhysical(x + len - 1, y, &px1, &py1);
    DrawLine(display_.GetFramebuffer(), HwWidth(), HwHeight(), px0, py0, px1, py1, color);
}

void DisplayGFX::DrawVLine(int x, int y, int len, uint16_t color)
{
    if (len <= 0) return;
    if (rotation_ == Rotation::k0)
    {
        DrawVLine(display_.GetFramebuffer(), HwWidth(), HwHeight(), x, y, len, color);
        return;
    }
    int px0, py0, px1, py1;
    MapToPhysical(x, y, &px0, &py0);
    MapToPhysical(x, y + len - 1, &px1, &py1);
    DrawLine(display_.GetFramebuffer(), HwWidth(), HwHeight(), px0, py0, px1, py1, color);
}

void DisplayGFX::DrawLine(int x0, int y0, int x1, int y1, uint16_t color)
{
    if (rotation_ == Rotation::k0)
    {
        DrawLine(display_.GetFramebuffer(), HwWidth(), HwHeight(), x0, y0, x1, y1, color);
        return;
    }
    int px0, py0, px1, py1;
    MapToPhysical(x0, y0, &px0, &py0);
    MapToPhysical(x1, y1, &px1, &py1);
    DrawLine(display_.GetFramebuffer(), HwWidth(), HwHeight(), px0, py0, px1, py1, color);
}

void DisplayGFX::DrawRect(int x, int y, int rw, int rh, uint16_t color)
{
    if (rw <= 0 || rh <= 0) return;
    switch (rotation_)
    {
        case Rotation::k0:
            DrawRect(display_.GetFramebuffer(), HwWidth(), HwHeight(), x, y, rw, rh, color);
            break;
        case Rotation::k90:
            DrawRect(display_.GetFramebuffer(), HwWidth(), HwHeight(), HwWidth() - y - rh, x, rh, rw, color);
            break;
        case Rotation::k180:
            DrawRect(display_.GetFramebuffer(), HwWidth(), HwHeight(), HwWidth() - x - rw, HwHeight() - y - rh, rw, rh,
                     color);
            break;
        case Rotation::k270:
            DrawRect(display_.GetFramebuffer(), HwWidth(), HwHeight(), y, HwHeight() - x - rw, rh, rw, color);
            break;
    }
}

void DisplayGFX::FillRect(int x, int y, int rw, int rh, uint16_t color)
{
    if (rw <= 0 || rh <= 0) return;
    switch (rotation_)
    {
        case Rotation::k0:
            FillRect(display_.GetFramebuffer(), HwWidth(), HwHeight(), x, y, rw, rh, color);
            break;
        case Rotation::k90:
            FillRect(display_.GetFramebuffer(), HwWidth(), HwHeight(), HwWidth() - y - rh, x, rh, rw, color);
            break;
        case Rotation::k180:
            FillRect(display_.GetFramebuffer(), HwWidth(), HwHeight(), HwWidth() - x - rw, HwHeight() - y - rh, rw, rh,
                     color);
            break;
        case Rotation::k270:
            FillRect(display_.GetFramebuffer(), HwWidth(), HwHeight(), y, HwHeight() - x - rw, rh, rw, color);
            break;
    }
}

void DisplayGFX::DrawCircle(int cx, int cy, int r, uint16_t color)
{
    if (rotation_ == Rotation::k0)
    {
        DrawCircle(display_.GetFramebuffer(), HwWidth(), HwHeight(), cx, cy, r, color);
        return;
    }
    int px, py;
    MapToPhysical(cx, cy, &px, &py);
    DrawCircle(display_.GetFramebuffer(), HwWidth(), HwHeight(), px, py, r, color);
}

void DisplayGFX::FillCircle(int cx, int cy, int r, uint16_t color)
{
    if (rotation_ == Rotation::k0)
    {
        FillCircle(display_.GetFramebuffer(), HwWidth(), HwHeight(), cx, cy, r, color);
        return;
    }
    int px, py;
    MapToPhysical(cx, cy, &px, &py);
    FillCircle(display_.GetFramebuffer(), HwWidth(), HwHeight(), px, py, r, color);
}

void DisplayGFX::DrawChar(int x, int y, char ch, const Font& font, uint16_t color, uint16_t bg)
{
    if (ch < 0x20 || ch > 0x7F) ch = '?';
    int fw = font.w, fh = font.h;

    // k0 快速路径：直接写物理帧缓冲
    if (rotation_ == Rotation::k0)
    {
        DrawGlyph(display_.GetFramebuffer(), HwWidth(), HwHeight(), x, y, ch, font, color, bg);
        return;
    }

    // 旋转路径：每个像素走实例 DrawPixel / FillRect（自动映射坐标）
    if (bg != color) FillRect(x, y, fw, fh, bg);

    int idx = ch - 0x20;

    if (fw <= 8)
    {
        const uint8_t* glyphs = (const uint8_t*)font.data;
        const uint8_t* glyph = glyphs + idx * fh;
        for (int row = 0; row < fh; ++row)
            for (int col = 0; col < fw; ++col)
                if (glyph[row] & (0x80 >> col))
                    DrawPixel(x + col, y + row, color);
    }
    else if (fw <= 16)
    {
        const uint16_t* glyphs = (const uint16_t*)font.data;
        const uint16_t* glyph = glyphs + idx * fh;
        for (int row = 0; row < fh; ++row)
            for (int col = 0; col < fw; ++col)
                if (glyph[row] & (0x8000 >> col))
                    DrawPixel(x + col, y + row, color);
    }
    else
    {
        const uint32_t* glyphs = (const uint32_t*)font.data;
        const uint32_t* glyph = glyphs + idx * fh;
        for (int row = 0; row < fh; ++row)
            for (int col = 0; col < fw; ++col)
                if (glyph[row] & (0x80000000 >> col))
                    DrawPixel(x + col, y + row, color);
    }
}

void DisplayGFX::DrawString(int x, int y, const char* str, const Font& font, uint16_t color, uint16_t bg)
{
    if (!str) return;
    int cx = x;
    while (*str)
    {
        DrawChar(cx, y, *str, font, color, bg);
        cx += font.w;
        if (cx >= GetWidth()) break;
        str++;
    }
}

// ---- 使用默认字体的重载 ----

void DisplayGFX::DrawChar(int x, int y, char ch, uint16_t color, uint16_t bg)
{
    DrawChar(x, y, ch, *font_, color, bg);
}

void DisplayGFX::DrawString(int x, int y, const char* str, uint16_t color, uint16_t bg)
{
    DrawString(x, y, str, *font_, color, bg);
}

// ====== 测试程序（全部使用实例方法，自动跟随旋转） ======

void DisplayGFX::TestBallCollision()
{
    int w = GetWidth(), h = GetHeight();
    const int N = 10;
    struct
    {
        float x, y, vx, vy;
        int r;
        uint16_t c;
    } b[N];
    uint16_t colors[] = {GFX_RED, GFX_GREEN, GFX_BLUE, GFX_YELLOW, GFX_CYAN, GFX_MAGENTA, GFX_ORANGE, GFX_WHITE, 0x07E0, 0xF80F};
    int radii[] = {30, 22, 34, 18, 26, 20, 32, 16, 24, 28};
    for (int i = 0; i < N; ++i)
    {
        b[i].x = 100.0f + (float)(rand() % (w - 200));
        b[i].y = 100.0f + (float)(rand() % (h - 200));
        b[i].vx = (float)(rand() % 8 - 4);
        b[i].vy = (float)(rand() % 8 - 4);
        b[i].r = radii[i];
        b[i].c = colors[i];
    }
    float gx = w * 0.5f, gy = h * 0.5f;
    for (int f = 0; f < 600; ++f)
    {
        for (int i = 0; i < N; ++i)
        {
            float dx2 = gx - b[i].x, dy2 = gy - b[i].y;
            float dist2 = dx2 * dx2 + dy2 * dy2 + 6000.0f;
            float force = 100.0f / dist2;
            b[i].vx += dx2 * force;
            b[i].vy += dy2 * force;
            b[i].vx *= 0.997f;
            b[i].vy *= 0.997f;
            b[i].x += b[i].vx;
            b[i].y += b[i].vy;
            if (b[i].x - b[i].r < 0) { b[i].x = (float)b[i].r; b[i].vx = -b[i].vx * 0.7f; }
            if (b[i].x + b[i].r > w) { b[i].x = w - b[i].r; b[i].vx = -b[i].vx * 0.7f; }
            if (b[i].y - b[i].r < 0) { b[i].y = (float)b[i].r; b[i].vy = -b[i].vy * 0.7f; }
            if (b[i].y + b[i].r > h) { b[i].y = h - b[i].r; b[i].vy = -b[i].vy * 0.7f; }
        }
        for (int i = 0; i < N; ++i)
            for (int j = i + 1; j < N; ++j)
            {
                float dx2 = b[i].x - b[j].x, dy2 = b[i].y - b[j].y;
                float dist = sqrtf(dx2 * dx2 + dy2 * dy2);
                float md = (float)(b[i].r + b[j].r);
                if (dist < md && dist > 0.001f)
                {
                    float nx = dx2 / dist, ny = dy2 / dist;
                    float dvn = (b[i].vx - b[j].vx) * nx + (b[i].vy - b[j].vy) * ny;
                    if (dvn < 0)
                    {
                        b[i].vx -= dvn * nx; b[i].vy -= dvn * ny;
                        b[j].vx += dvn * nx; b[j].vy += dvn * ny;
                    }
                    float overlap = md - dist;
                    b[i].x += nx * overlap * 0.5f; b[i].y += ny * overlap * 0.5f;
                    b[j].x -= nx * overlap * 0.5f; b[j].y -= ny * overlap * 0.5f;
                }
            }
        display_.Fill(GFX_BLACK);
        for (int ring = 0; ring < 3; ++ring)
        {
            int rr2 = 30 + ring * 15 + (f % 15);
            FillCircle((int)gx, (int)gy, rr2, GFX_DARK);
        }
        for (int i = 0; i < N; ++i)
        {
            int cx = (int)b[i].x, cy = (int)b[i].y, rr = b[i].r;
            FillCircle(cx, cy, rr, b[i].c);
            int hs = rr / 3;
            if (hs > 1) FillRect(cx - rr / 3, cy - rr / 3, hs, hs, GFX_WHITE);
        }
        display_.Flush();
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}

void DisplayGFX::TestStarfield()
{
    int w = GetWidth(), h = GetHeight();
    const int N = 100;
    struct
    {
        int x, y, speed;
        uint16_t c;
    } stars[N];
    for (int i = 0; i < N; ++i)
    {
        stars[i].x = rand() % w;
        stars[i].y = rand() % h;
        stars[i].speed = 1 + rand() % 4;
        uint16_t cc[] = {GFX_DARK, GFX_GRAY, GFX_WHITE, GFX_CYAN};
        stars[i].c = cc[stars[i].speed - 1];
    }
    for (int f = 0; f < 400; ++f)
    {
        display_.Fill(GFX_BLACK);
        for (int i = 0; i < N; ++i)
        {
            stars[i].x = (stars[i].x + stars[i].speed) % w;
            int sx = stars[i].x, sy = stars[i].y;
            if (sx < 0 || sx >= w || sy < 0 || sy >= h) continue;
            uint16_t c = stars[i].c;
            int sp = stars[i].speed;
            DrawPixel(sx, sy, c);
            if (sp >= 2)
            {
                if (sx > 0) DrawPixel(sx - 1, sy, c);
                if (sx < w - 1) DrawPixel(sx + 1, sy, c);
                if (sy > 0) DrawPixel(sx, sy - 1, c);
                if (sy < h - 1) DrawPixel(sx, sy + 1, c);
            }
            if (sp >= 3)
            {
                if (sx > 0 && sy > 0) DrawPixel(sx - 1, sy - 1, c);
                if (sx < w - 1 && sy > 0) DrawPixel(sx + 1, sy - 1, c);
                if (sx > 0 && sy < h - 1) DrawPixel(sx - 1, sy + 1, c);
                if (sx < w - 1 && sy < h - 1) DrawPixel(sx + 1, sy + 1, c);
            }
            if (sp >= 4)
            {
                if (sx > 1) DrawPixel(sx - 2, sy, GFX_DARK);
                if (sx < w - 2) DrawPixel(sx + 2, sy, GFX_DARK);
                if (sy > 1) DrawPixel(sx, sy - 2, GFX_DARK);
                if (sy < h - 2) DrawPixel(sx, sy + 2, GFX_DARK);
            }
        }
        display_.Flush();
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}

void DisplayGFX::TestCubeRotation()
{
    int w = GetWidth(), h = GetHeight();
    const float verts[8][3] = {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
                                {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}};
    const int edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
                               {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
    uint16_t palette[] = {GFX_RED, GFX_ORANGE, GFX_YELLOW, GFX_GREEN, GFX_CYAN, GFX_BLUE, GFX_MAGENTA, GFX_WHITE};
    for (int f = 0; f < 600; ++f)
    {
        display_.Fill(GFX_BLACK);
        float pulse = 1.0f + 0.12f * sinf(f * 0.04f);
        float a1 = f * 0.025f, ca1 = cosf(a1), sa1 = sinf(a1);
        float a1b = f * 0.018f, ca1b = cosf(a1b), sa1b = sinf(a1b);
        int ox[8], oy[8];
        for (int i = 0; i < 8; ++i)
        {
            float rx = verts[i][0] * ca1 - verts[i][2] * sa1;
            float rz = verts[i][0] * sa1 + verts[i][2] * ca1;
            float ry = verts[i][1] * ca1b - rz * sa1b;
            rz = verts[i][1] * sa1b + rz * ca1b;
            ox[i] = (int)(rx * 150 * pulse + w / 2);
            oy[i] = (int)(-ry * 150 * pulse + h / 2) + (int)(rz * 20);
        }
        float a2 = -f * 0.04f, ca2 = cosf(a2), sa2 = sinf(a2);
        float a2b = -f * 0.03f, ca2b = cosf(a2b), sa2b = sinf(a2b);
        int ix[8], iy[8];
        for (int i = 0; i < 8; ++i)
        {
            float rx = verts[i][0] * ca2 - verts[i][2] * sa2;
            float rz = verts[i][0] * sa2 + verts[i][2] * ca2;
            float ry = verts[i][1] * ca2b - rz * sa2b;
            rz = verts[i][1] * sa2b + rz * ca2b;
            ix[i] = (int)(rx * 80 * pulse + w / 2);
            iy[i] = (int)(-ry * 80 * pulse + h / 2) + (int)(rz * 12);
        }
        int cs = (f / 8) % 8;
        for (int e = 0; e < 12; ++e)
        {
            int ci = (cs + (edges[e][0] + edges[e][1]) % 4) % 8;
            DrawLine(ox[edges[e][0]], oy[edges[e][0]], ox[edges[e][1]], oy[edges[e][1]], palette[ci]);
        }
        for (int e = 0; e < 12; ++e)
        {
            int ci = (cs + 4 + e % 3) % 8;
            DrawLine(ix[edges[e][0]], iy[edges[e][0]], ix[edges[e][1]], iy[edges[e][1]], palette[ci]);
        }
        for (int i = 0; i < 8; ++i) DrawLine(ox[i], oy[i], ix[i], iy[i], GFX_DARK);
        for (int i = 0; i < 8; ++i)
        {
            FillRect(ox[i] - 4, oy[i] - 4, 9, 9, GFX_WHITE);
            FillRect(ox[i] - 2, oy[i] - 2, 5, 5, palette[(cs + i) % 8]);
        }
        for (int i = 0; i < 8; ++i)
        {
            FillRect(ix[i] - 2, iy[i] - 2, 5, 5, GFX_YELLOW);
        }
        display_.Flush();
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}

void DisplayGFX::TestColorSquares()
{
    int w = GetWidth(), h = GetHeight();
    const int NS = 5;
    struct
    {
        int x, y, vx, vy, s;
        uint16_t c;
    } sq[NS] = {
        {w / 2 - 40, h / 2 - 40, 5, 3, 80, GFX_RED},  {100, 200, -4, 6, 50, GFX_CYAN},   {300, 500, 6, -5, 60, GFX_GREEN},
        {200, 100, -5, -6, 45, GFX_YELLOW},            {380, 300, 4, -7, 70, GFX_MAGENTA},
    };
    uint16_t pal[] = {GFX_RED, GFX_CYAN, GFX_GREEN, GFX_YELLOW, GFX_MAGENTA, GFX_ORANGE, GFX_BLUE, GFX_WHITE};
    for (int f = 0; f < 450; ++f)
    {
        for (int i = 0; i < NS; ++i)
        {
            sq[i].x += sq[i].vx;
            sq[i].y += sq[i].vy;
            if (sq[i].x <= 0) { sq[i].x = 0; sq[i].vx = -sq[i].vx; sq[i].c = pal[(f / 20 + i) % 8]; }
            if (sq[i].x + sq[i].s >= w) { sq[i].x = w - sq[i].s; sq[i].vx = -sq[i].vx; sq[i].c = pal[(f / 20 + i + 2) % 8]; }
            if (sq[i].y <= 0) { sq[i].y = 0; sq[i].vy = -sq[i].vy; sq[i].c = pal[(f / 20 + i + 4) % 8]; }
            if (sq[i].y + sq[i].s >= h) { sq[i].y = h - sq[i].s; sq[i].vy = -sq[i].vy; sq[i].c = pal[(f / 20 + i + 6) % 8]; }
        }
        for (int i = 0; i < NS; ++i)
            for (int j = i + 1; j < NS; ++j)
            {
                if (sq[i].x < sq[j].x + sq[j].s && sq[i].x + sq[i].s > sq[j].x && sq[i].y < sq[j].y + sq[j].s &&
                    sq[i].y + sq[i].s > sq[j].y)
                {
                    int tvx = sq[i].vx; sq[i].vx = sq[j].vx; sq[j].vx = tvx;
                    int tvy = sq[i].vy; sq[i].vy = sq[j].vy; sq[j].vy = tvy;
                    uint16_t tc = sq[i].c; sq[i].c = sq[j].c; sq[j].c = tc;
                    int ox2 = (sq[i].x + sq[i].s / 2) - (sq[j].x + sq[j].s / 2);
                    int oy2 = (sq[i].y + sq[i].s / 2) - (sq[j].y + sq[j].s / 2);
                    ox2 = (ox2 < 0) ? -1 : 1;
                    oy2 = (oy2 < 0) ? -1 : 1;
                    sq[i].x += ox2 * 4; sq[j].x -= ox2 * 4;
                    sq[i].y += oy2 * 4; sq[j].y -= oy2 * 4;
                }
            }
        display_.Fill(GFX_BLACK);
        for (int i = 0; i < NS; ++i)
        {
            FillRect(sq[i].x, sq[i].y, sq[i].s, sq[i].s, sq[i].c);
            DrawRect(sq[i].x, sq[i].y, sq[i].s, sq[i].s, GFX_WHITE);
        }
        display_.Flush();
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}

void DisplayGFX::TestFont()
{
    int w = GetWidth(), h = GetHeight();

    // 渐变背景
    for (int y = 0; y < h; ++y)
    {
        float t = (float)y / h;
        uint8_t r = (uint8_t)(31 * t);
        uint8_t g = (uint8_t)(63 * (1.0f - t));
        uint8_t b = (uint8_t)(31 * (0.5f + 0.5f * sinf(t * 3.14159f)));
        uint16_t bg = (uint16_t)((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
        DrawHLine(0, y, w, bg);
    }

    // ---- 8x16 小字：ASCII 字表 ----
    int fw = kFont8x16.w, fh = kFont8x16.h;
    DrawString(10, 10, "Font: 8x16 Heiti — ASCII 0x20-0x7F", kFont8x16, GFX_WHITE, GFX_BLACK);

    for (int row = 0; row < 6; ++row)
    {
        for (int col = 0; col < 16; ++col)
        {
            char ch = (char)(0x20 + row * 16 + col);
            int cx = 10 + col * (fw + 1);
            int cy = 10 + fh + 4 + row * (fh + 1);
            uint16_t fg = (col % 2 == 0) ? GFX_YELLOW : GFX_CYAN;
            char buf[2] = {ch, '\0'};
            DrawString(cx, cy, buf, kFont8x16, fg, GFX_BLACK);
        }
    }

    int y0 = 10 + fh + 4 + 6 * (fh + 1) + 8;
    DrawHLine(10, y0, w - 20, GFX_WHITE);

    // ---- 16x32 正文 ----
    DrawString(10, y0 + 8, "Hello ESP32-P4!", kFont16x32, GFX_GREEN, GFX_BLACK);
    DrawString(10, y0 + 8 + 34, "0123456789", kFont16x32, GFX_YELLOW, GFX_BLACK);

    // ---- 24x48 标题 ----
    DrawString(10, y0 + 8 + 68 + 8, "24x48 Heiti", kFont24x48, GFX_RED, GFX_BLACK);

    // ---- 32x64 大标题 ----
    DrawString(10, y0 + 8 + 68 + 8 + 52 + 8, "32x64", kFont32x64, GFX_CYAN, GFX_BLACK);

    display_.Flush();
    vTaskDelay(pdMS_TO_TICKS(5000));
}

void DisplayGFX::TestAll()
{
    TestFont();
    TestBallCollision();
    TestStarfield();
    TestCubeRotation();
    TestColorSquares();
}
