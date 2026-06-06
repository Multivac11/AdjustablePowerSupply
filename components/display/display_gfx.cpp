#include "display_gfx.h"

#include <cstdlib>
#include <cstring>

static const char* TAG = "DisplayGFX";

// ====== 6×8 ASCII 字库 (5×7 点阵，每字符 5 字节，码点 0x20–0x7F) ======
// 每字节对应一列 (MSB = 顶部像素)，在 6×8 单元格内左对齐，右侧留 1px 间距
static const uint8_t kFont5x7[96][5] = {
    /* 0x20 ' ' */ {0x00, 0x00, 0x00, 0x00, 0x00},
    /* 0x21 '!' */ {0x00, 0x00, 0x5F, 0x00, 0x00},
    /* 0x22 '"' */ {0x00, 0x07, 0x00, 0x07, 0x00},
    /* 0x23 '#' */ {0x14, 0x7F, 0x14, 0x7F, 0x14},
    /* 0x24 '$' */ {0x24, 0x2A, 0x7F, 0x2A, 0x12},
    /* 0x25 '%' */ {0x23, 0x13, 0x08, 0x64, 0x62},
    /* 0x26 '&' */ {0x36, 0x49, 0x55, 0x22, 0x50},
    /* 0x27 ''' */ {0x00, 0x05, 0x03, 0x00, 0x00},
    /* 0x28 '(' */ {0x00, 0x1C, 0x22, 0x41, 0x00},
    /* 0x29 ')' */ {0x00, 0x41, 0x22, 0x1C, 0x00},
    /* 0x2A '*' */ {0x08, 0x2A, 0x1C, 0x2A, 0x08},
    /* 0x2B '+' */ {0x08, 0x08, 0x3E, 0x08, 0x08},
    /* 0x2C ',' */ {0x00, 0x50, 0x30, 0x00, 0x00},
    /* 0x2D '-' */ {0x08, 0x08, 0x08, 0x08, 0x08},
    /* 0x2E '.' */ {0x00, 0x60, 0x60, 0x00, 0x00},
    /* 0x2F '/' */ {0x20, 0x10, 0x08, 0x04, 0x02},
    /* 0x30 '0' */ {0x3E, 0x51, 0x49, 0x45, 0x3E},
    /* 0x31 '1' */ {0x00, 0x42, 0x7F, 0x40, 0x00},
    /* 0x32 '2' */ {0x42, 0x61, 0x51, 0x49, 0x46},
    /* 0x33 '3' */ {0x21, 0x41, 0x45, 0x4B, 0x31},
    /* 0x34 '4' */ {0x18, 0x14, 0x12, 0x7F, 0x10},
    /* 0x35 '5' */ {0x27, 0x45, 0x45, 0x45, 0x39},
    /* 0x36 '6' */ {0x3C, 0x4A, 0x49, 0x49, 0x30},
    /* 0x37 '7' */ {0x01, 0x71, 0x09, 0x05, 0x03},
    /* 0x38 '8' */ {0x36, 0x49, 0x49, 0x49, 0x36},
    /* 0x39 '9' */ {0x06, 0x49, 0x49, 0x29, 0x1E},
    /* 0x3A ':' */ {0x00, 0x36, 0x36, 0x00, 0x00},
    /* 0x3B ';' */ {0x00, 0x56, 0x36, 0x00, 0x00},
    /* 0x3C '<' */ {0x00, 0x08, 0x14, 0x22, 0x41},
    /* 0x3D '=' */ {0x14, 0x14, 0x14, 0x14, 0x14},
    /* 0x3E '>' */ {0x41, 0x22, 0x14, 0x08, 0x00},
    /* 0x3F '?' */ {0x02, 0x01, 0x51, 0x09, 0x06},
    /* 0x40 '@' */ {0x32, 0x49, 0x79, 0x41, 0x3E},
    /* 0x41 'A' */ {0x7E, 0x11, 0x11, 0x11, 0x7E},
    /* 0x42 'B' */ {0x7F, 0x49, 0x49, 0x49, 0x36},
    /* 0x43 'C' */ {0x3E, 0x41, 0x41, 0x41, 0x22},
    /* 0x44 'D' */ {0x7F, 0x41, 0x41, 0x22, 0x1C},
    /* 0x45 'E' */ {0x7F, 0x49, 0x49, 0x49, 0x41},
    /* 0x46 'F' */ {0x7F, 0x09, 0x09, 0x01, 0x01},
    /* 0x47 'G' */ {0x3E, 0x41, 0x41, 0x51, 0x32},
    /* 0x48 'H' */ {0x7F, 0x08, 0x08, 0x08, 0x7F},
    /* 0x49 'I' */ {0x00, 0x41, 0x7F, 0x41, 0x00},
    /* 0x4A 'J' */ {0x20, 0x40, 0x41, 0x3F, 0x01},
    /* 0x4B 'K' */ {0x7F, 0x08, 0x14, 0x22, 0x41},
    /* 0x4C 'L' */ {0x7F, 0x40, 0x40, 0x40, 0x40},
    /* 0x4D 'M' */ {0x7F, 0x02, 0x04, 0x02, 0x7F},
    /* 0x4E 'N' */ {0x7F, 0x04, 0x08, 0x10, 0x7F},
    /* 0x4F 'O' */ {0x3E, 0x41, 0x41, 0x41, 0x3E},
    /* 0x50 'P' */ {0x7F, 0x09, 0x09, 0x09, 0x06},
    /* 0x51 'Q' */ {0x3E, 0x41, 0x51, 0x21, 0x5E},
    /* 0x52 'R' */ {0x7F, 0x09, 0x19, 0x29, 0x46},
    /* 0x53 'S' */ {0x46, 0x49, 0x49, 0x49, 0x31},
    /* 0x54 'T' */ {0x01, 0x01, 0x7F, 0x01, 0x01},
    /* 0x55 'U' */ {0x3F, 0x40, 0x40, 0x40, 0x3F},
    /* 0x56 'V' */ {0x1F, 0x20, 0x40, 0x20, 0x1F},
    /* 0x57 'W' */ {0x7F, 0x20, 0x18, 0x20, 0x7F},
    /* 0x58 'X' */ {0x63, 0x14, 0x08, 0x14, 0x63},
    /* 0x59 'Y' */ {0x03, 0x04, 0x78, 0x04, 0x03},
    /* 0x5A 'Z' */ {0x61, 0x51, 0x49, 0x45, 0x43},
    /* 0x5B '[' */ {0x00, 0x00, 0x7F, 0x41, 0x41},
    /* 0x5C '\' */ {0x02, 0x04, 0x08, 0x10, 0x20},
    /* 0x5D ']' */ {0x41, 0x41, 0x7F, 0x00, 0x00},
    /* 0x5E '^' */ {0x04, 0x02, 0x01, 0x02, 0x04},
    /* 0x5F '_' */ {0x40, 0x40, 0x40, 0x40, 0x40},
    /* 0x60 '`' */ {0x00, 0x01, 0x02, 0x04, 0x00},
    /* 0x61 'a' */ {0x20, 0x54, 0x54, 0x54, 0x78},
    /* 0x62 'b' */ {0x7F, 0x48, 0x44, 0x44, 0x38},
    /* 0x63 'c' */ {0x38, 0x44, 0x44, 0x44, 0x20},
    /* 0x64 'd' */ {0x38, 0x44, 0x44, 0x48, 0x7F},
    /* 0x65 'e' */ {0x38, 0x54, 0x54, 0x54, 0x18},
    /* 0x66 'f' */ {0x08, 0x7E, 0x09, 0x01, 0x02},
    /* 0x67 'g' */ {0x08, 0x14, 0x54, 0x54, 0x3C},
    /* 0x68 'h' */ {0x7F, 0x08, 0x04, 0x04, 0x78},
    /* 0x69 'i' */ {0x00, 0x44, 0x7D, 0x40, 0x00},
    /* 0x6A 'j' */ {0x20, 0x40, 0x44, 0x3D, 0x00},
    /* 0x6B 'k' */ {0x00, 0x7F, 0x10, 0x28, 0x44},
    /* 0x6C 'l' */ {0x00, 0x41, 0x7F, 0x40, 0x00},
    /* 0x6D 'm' */ {0x7C, 0x04, 0x18, 0x04, 0x78},
    /* 0x6E 'n' */ {0x7C, 0x08, 0x04, 0x04, 0x78},
    /* 0x6F 'o' */ {0x38, 0x44, 0x44, 0x44, 0x38},
    /* 0x70 'p' */ {0x7C, 0x14, 0x14, 0x14, 0x08},
    /* 0x71 'q' */ {0x08, 0x14, 0x14, 0x18, 0x7C},
    /* 0x72 'r' */ {0x7C, 0x08, 0x04, 0x04, 0x08},
    /* 0x73 's' */ {0x48, 0x54, 0x54, 0x54, 0x20},
    /* 0x74 't' */ {0x04, 0x3F, 0x44, 0x40, 0x20},
    /* 0x75 'u' */ {0x3C, 0x40, 0x40, 0x20, 0x7C},
    /* 0x76 'v' */ {0x1C, 0x20, 0x40, 0x20, 0x1C},
    /* 0x77 'w' */ {0x3C, 0x40, 0x30, 0x40, 0x3C},
    /* 0x78 'x' */ {0x44, 0x28, 0x10, 0x28, 0x44},
    /* 0x79 'y' */ {0x0C, 0x50, 0x50, 0x50, 0x3C},
    /* 0x7A 'z' */ {0x44, 0x64, 0x54, 0x4C, 0x44},
    /* 0x7B '{' */ {0x00, 0x08, 0x36, 0x41, 0x00},
    /* 0x7C '|' */ {0x00, 0x00, 0x7F, 0x00, 0x00},
    /* 0x7D '}' */ {0x00, 0x41, 0x36, 0x08, 0x00},
    /* 0x7E '~' */ {0x08, 0x04, 0x08, 0x10, 0x08},
    /* 0x7F DEL */ {0x00, 0x00, 0x00, 0x00, 0x00},
};

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

void DisplayGFX::DrawChar(uint16_t* fb, int w, int h, int x, int y, char ch, uint16_t color, uint16_t bg)
{
    if (ch < 0x20 || ch > 0x7F) return;
    // 整字符在屏幕外则跳过
    if (x + kFontWidth <= 0 || x >= w || y + kFontHeight <= 0 || y >= h) return;

    const uint8_t* glyph = kFont5x7[ch - 0x20];
    for (int col = 0; col < 5; ++col)
    {
        int px = x + col;
        if (px < 0 || px >= w) continue;
        uint8_t line = glyph[col];
        for (int row = 0; row < 8; ++row)
        {
            int py = y + row;
            if (py < 0 || py >= h) continue;
            if (line & (1 << (7 - row)))
                fb[py * w + px] = color;
            else if (bg != color)  // bg == color 时跳过背景（透明模式）
                fb[py * w + px] = bg;
        }
    }
    // 第 6 列（间距列）：填充背景色
    int px6 = x + 5;
    if (px6 >= 0 && px6 < w && bg != color)
    {
        for (int row = 0; row < 8; ++row)
        {
            int py = y + row;
            if (py >= 0 && py < h) fb[py * w + px6] = bg;
        }
    }
}

void DisplayGFX::DrawString(uint16_t* fb, int w, int h, int x, int y, const char* str, uint16_t color, uint16_t bg)
{
    if (!str) return;
    int cx = x;
    while (*str)
    {
        DrawChar(fb, w, h, cx, y, *str, color, bg);
        cx += kFontWidth;
        if (cx >= w) break;  // 超出屏幕，不再绘制
        str++;
    }
}

// ====== 测试程序 ======

// 十球物理碰撞 + 中心引力井
void DisplayGFX::TestBallCollision(Display& display)
{
    int w = display.GetWidth(), h = display.GetHeight();
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
        // 引力 + 阻尼
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
            if (b[i].x - b[i].r < 0)
            {
                b[i].x = (float)b[i].r;
                b[i].vx = -b[i].vx * 0.7f;
            }
            if (b[i].x + b[i].r > w)
            {
                b[i].x = w - b[i].r;
                b[i].vx = -b[i].vx * 0.7f;
            }
            if (b[i].y - b[i].r < 0)
            {
                b[i].y = (float)b[i].r;
                b[i].vy = -b[i].vy * 0.7f;
            }
            if (b[i].y + b[i].r > h)
            {
                b[i].y = h - b[i].r;
                b[i].vy = -b[i].vy * 0.7f;
            }
        }
        // 碰撞
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
                        b[i].vx -= dvn * nx;
                        b[i].vy -= dvn * ny;
                        b[j].vx += dvn * nx;
                        b[j].vy += dvn * ny;
                    }
                    float overlap = md - dist;
                    b[i].x += nx * overlap * 0.5f;
                    b[i].y += ny * overlap * 0.5f;
                    b[j].x -= nx * overlap * 0.5f;
                    b[j].y -= ny * overlap * 0.5f;
                }
            }
        display.Fill(GFX_BLACK);
        uint16_t* fb = display.GetFramebuffer();
        // 引力井波纹
        for (int ring = 0; ring < 3; ++ring)
        {
            int rr2 = 30 + ring * 15 + (f % 15);
            FillCircle(fb, w, h, (int)gx, (int)gy, rr2, GFX_DARK);
        }
        // 球体
        for (int i = 0; i < N; ++i)
        {
            int cx = (int)b[i].x, cy = (int)b[i].y, rr = b[i].r;
            FillCircle(fb, w, h, cx, cy, rr, b[i].c);
            // 高光
            int hs = rr / 3;
            if (hs > 1) FillRect(fb, w, h, cx - rr / 3, cy - rr / 3, hs, hs, GFX_WHITE);
        }
        display.Flush();
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}

// 星空视差（十字星芒）
void DisplayGFX::TestStarfield(Display& display)
{
    int w = display.GetWidth(), h = display.GetHeight();
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
        display.Fill(GFX_BLACK);
        uint16_t* fb = display.GetFramebuffer();
        for (int i = 0; i < N; ++i)
        {
            stars[i].x = (stars[i].x + stars[i].speed) % w;
            int sx = stars[i].x, sy = stars[i].y;
            if (sx < 0 || sx >= w || sy < 0 || sy >= h) continue;
            uint16_t c = stars[i].c;
            int sp = stars[i].speed;
            // 中心点
            fb[sy * w + sx] = c;
            // 十字
            if (sp >= 2)
            {
                if (sx > 0) fb[sy * w + sx - 1] = c;
                if (sx < w - 1) fb[sy * w + sx + 1] = c;
                if (sy > 0) fb[(sy - 1) * w + sx] = c;
                if (sy < h - 1) fb[(sy + 1) * w + sx] = c;
            }
            // 对角线（星芒）
            if (sp >= 3)
            {
                if (sx > 0 && sy > 0) fb[(sy - 1) * w + sx - 1] = c;
                if (sx < w - 1 && sy > 0) fb[(sy - 1) * w + sx + 1] = c;
                if (sx > 0 && sy < h - 1) fb[(sy + 1) * w + sx - 1] = c;
                if (sx < w - 1 && sy < h - 1) fb[(sy + 1) * w + sx + 1] = c;
            }
            // 外层暗色光晕
            if (sp >= 4)
            {
                if (sx > 1) fb[sy * w + sx - 2] = GFX_DARK;
                if (sx < w - 2) fb[sy * w + sx + 2] = GFX_DARK;
                if (sy > 1) fb[(sy - 2) * w + sx] = GFX_DARK;
                if (sy < h - 2) fb[(sy + 2) * w + sx] = GFX_DARK;
            }
        }
        display.Flush();
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}

// 双立方体交织旋转 + 色彩循环
void DisplayGFX::TestCubeRotation(Display& display)
{
    int w = display.GetWidth(), h = display.GetHeight();
    const float verts[8][3] = {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
                                {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}};
    const int edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
                               {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
    uint16_t palette[] = {GFX_RED, GFX_ORANGE, GFX_YELLOW, GFX_GREEN, GFX_CYAN, GFX_BLUE, GFX_MAGENTA, GFX_WHITE};
    for (int f = 0; f < 600; ++f)
    {
        display.Fill(GFX_BLACK);
        uint16_t* fb = display.GetFramebuffer();
        float pulse = 1.0f + 0.12f * sinf(f * 0.04f);
        // 外立方体 — Y 轴为主旋转
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
        // 内立方体 — 反向 X 轴旋转
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
        // 外立方体棱边（色彩滚动）
        int cs = (f / 8) % 8;
        for (int e = 0; e < 12; ++e)
        {
            int ci = (cs + (edges[e][0] + edges[e][1]) % 4) % 8;
            DrawLine(fb, w, h, ox[edges[e][0]], oy[edges[e][0]], ox[edges[e][1]], oy[edges[e][1]], palette[ci]);
        }
        // 内立方体棱边
        for (int e = 0; e < 12; ++e)
        {
            int ci = (cs + 4 + e % 3) % 8;
            DrawLine(fb, w, h, ix[edges[e][0]], iy[edges[e][0]], ix[edges[e][1]], iy[edges[e][1]], palette[ci]);
        }
        // 内外顶点连线（半透明效果用暗色）
        for (int i = 0; i < 8; ++i) DrawLine(fb, w, h, ox[i], oy[i], ix[i], iy[i], GFX_DARK);
        // 外顶点光晕
        for (int i = 0; i < 8; ++i)
        {
            FillRect(fb, w, h, ox[i] - 4, oy[i] - 4, 9, 9, GFX_WHITE);
            FillRect(fb, w, h, ox[i] - 2, oy[i] - 2, 5, 5, palette[(cs + i) % 8]);
        }
        // 内顶点
        for (int i = 0; i < 8; ++i)
        {
            FillRect(fb, w, h, ix[i] - 2, iy[i] - 2, 5, 5, GFX_YELLOW);
        }
        display.Flush();
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}

// 多彩方块碰撞混战
void DisplayGFX::TestColorSquares(Display& display)
{
    int w = display.GetWidth(), h = display.GetHeight();
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
            if (sq[i].x <= 0)
            {
                sq[i].x = 0;
                sq[i].vx = -sq[i].vx;
                sq[i].c = pal[(f / 20 + i) % 8];
            }
            if (sq[i].x + sq[i].s >= w)
            {
                sq[i].x = w - sq[i].s;
                sq[i].vx = -sq[i].vx;
                sq[i].c = pal[(f / 20 + i + 2) % 8];
            }
            if (sq[i].y <= 0)
            {
                sq[i].y = 0;
                sq[i].vy = -sq[i].vy;
                sq[i].c = pal[(f / 20 + i + 4) % 8];
            }
            if (sq[i].y + sq[i].s >= h)
            {
                sq[i].y = h - sq[i].s;
                sq[i].vy = -sq[i].vy;
                sq[i].c = pal[(f / 20 + i + 6) % 8];
            }
        }
        // AABB 碰撞交换速度 + 颜色
        for (int i = 0; i < NS; ++i)
            for (int j = i + 1; j < NS; ++j)
            {
                if (sq[i].x < sq[j].x + sq[j].s && sq[i].x + sq[i].s > sq[j].x && sq[i].y < sq[j].y + sq[j].s &&
                    sq[i].y + sq[i].s > sq[j].y)
                {
                    int tvx = sq[i].vx;
                    sq[i].vx = sq[j].vx;
                    sq[j].vx = tvx;
                    int tvy = sq[i].vy;
                    sq[i].vy = sq[j].vy;
                    sq[j].vy = tvy;
                    uint16_t tc = sq[i].c;
                    sq[i].c = sq[j].c;
                    sq[j].c = tc;
                    // 弹开避免重叠
                    int ox2 = (sq[i].x + sq[i].s / 2) - (sq[j].x + sq[j].s / 2);
                    int oy2 = (sq[i].y + sq[i].s / 2) - (sq[j].y + sq[j].s / 2);
                    if (ox2 < 0)
                        ox2 = -1;
                    else
                        ox2 = 1;
                    if (oy2 < 0)
                        oy2 = -1;
                    else
                        oy2 = 1;
                    sq[i].x += ox2 * 4;
                    sq[j].x -= ox2 * 4;
                    sq[i].y += oy2 * 4;
                    sq[j].y -= oy2 * 4;
                }
            }
        display.Fill(GFX_BLACK);
        uint16_t* fb = display.GetFramebuffer();
        for (int i = 0; i < NS; ++i)
        {
            FillRect(fb, w, h, sq[i].x, sq[i].y, sq[i].s, sq[i].s, sq[i].c);
            // 白色边框
            DrawRect(fb, w, h, sq[i].x, sq[i].y, sq[i].s, sq[i].s, GFX_WHITE);
        }
        display.Flush();
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}

// 字体展示测试
void DisplayGFX::TestFont(Display& display)
{
    int w = display.GetWidth(), h = display.GetHeight();
    // 彩色渐变背景
    for (int y = 0; y < h; ++y)
    {
        float t = (float)y / h;
        uint8_t r = (uint8_t)(31 * t);
        uint8_t g = (uint8_t)(63 * (1.0f - t));
        uint8_t b = (uint8_t)(31 * (0.5f + 0.5f * sinf(t * 3.14159f)));
        uint16_t bg = (uint16_t)((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
        uint16_t* fb = display.GetFramebuffer();
        DrawHLine(fb, w, h, 0, y, w, bg);
    }

    uint16_t* fb = display.GetFramebuffer();

    // 标题
    DrawString(fb, w, h, 10, 10, "DisplayGFX Font Test", GFX_WHITE, GFX_BLACK);
    DrawString(fb, w, h, 10, 22, "6x8 ASCII 0x20-0x7F", GFX_GRAY, GFX_BLACK);

    // 打印全部 96 个可打印 ASCII 字符 (16 列 × 6 行)
    for (int row = 0; row < 6; ++row)
    {
        for (int col = 0; col < 16; ++col)
        {
            char ch = (char)(0x20 + row * 16 + col);
            int cx = 10 + col * (kFontWidth + 2);   // 额外 2px 间距
            int cy = 40 + row * (kFontHeight + 2);
            // 交替前景色
            uint16_t fg = (col % 2 == 0) ? GFX_YELLOW : GFX_CYAN;
            char buf[2] = {ch, '\0'};
            DrawString(fb, w, h, cx, cy, buf, fg, GFX_BLACK);
        }
    }

    // 底部绘制一条装饰线
    int by = 40 + 6 * (kFontHeight + 2) + 5;
    DrawHLine(fb, w, h, 10, by, w - 20, GFX_WHITE);

    // 多色测试字符串
    DrawString(fb, w, h, 10, by + 10, "Hello from ESP32-P4!", GFX_GREEN, GFX_BLACK);
    DrawString(fb, w, h, 10, by + 22, "0123456789 !@#$%^&*()", GFX_YELLOW, GFX_BLACK);
    DrawString(fb, w, h, 10, by + 34, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", GFX_RED, GFX_BLACK);
    DrawString(fb, w, h, 10, by + 46, "abcdefghijklmnopqrstuvwxyz", GFX_CYAN, GFX_BLACK);
    DrawString(fb, w, h, 10, by + 58, "The quick brown fox jumps", GFX_ORANGE, GFX_BLACK);

    display.Flush();
    vTaskDelay(pdMS_TO_TICKS(3000));
}

// 运行全部测试
void DisplayGFX::TestAll(Display& display)
{
    TestFont(display);
    TestBallCollision(display);
    TestStarfield(display);
    TestCubeRotation(display);
    TestColorSquares(display);
}
