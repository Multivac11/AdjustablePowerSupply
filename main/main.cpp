#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "adjustable_psu.h"
#include "buzzer.h"
#include "device_init.h"
#include "display.h"
#include "encoder.h"
#include "key.h"
#include "perf_monitor.h"
#include "temperature_monitor.h"

#define C_RED 0xF800
#define C_GREEN 0x07E0
#define C_BLUE 0x001F
#define C_WHITE 0xFFFF
#define C_BLACK 0x0000
#define C_YELLOW 0xFFE0
#define C_CYAN 0x07FF
#define C_MAGENTA 0xF81F
#define C_GRAY 0x8410
#define C_DARK 0x4208
#define C_ORANGE 0xFD20

static void FillRect(uint16_t* fb, int w, int h, int x, int y, int rw, int rh, uint16_t c)
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
        for (int col = 0; col < rw; ++col) fb[(y + row) * w + x + col] = c;
}

static void DrawHLine(uint16_t* fb, int w, int h, int x, int y, int len, uint16_t c)
{
    if (y < 0 || y >= h || len <= 0) return;
    if (x < 0)
    {
        len += x;
        x = 0;
    }
    if (x + len > w) len = w - x;
    for (int i = 0; i < len; ++i) fb[y * w + x + i] = c;
}

static void DrawLine(uint16_t* fb, int w, int h, int x0, int y0, int x1, int y1, uint16_t c)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (1)
    {
        if (x0 >= 0 && x0 < w && y0 >= 0 && y0 < h) fb[y0 * w + x0] = c;
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

static void DisplayTest()
{
    auto& d = Display::GetInstance();
    int w = d.GetWidth(), h = d.GetHeight();

    // ====== 1. 六球碰撞物理 ======
    {
        const int N = 6;
        struct
        {
            float x, y, vx, vy;
            int r;
            uint16_t c;
        } b[N] = {
            {100, 200, 3, 2, 25, C_RED},        {300, 150, -2.5f, 3, 22, C_GREEN}, {200, 500, 2.5f, -2, 28, C_BLUE},
            {350, 400, -3, 2.5f, 30, C_YELLOW}, {150, 300, 2, -3, 20, C_CYAN},     {250, 600, -2, -2.5f, 24, C_MAGENTA},
        };
        for (int f = 0; f < 500; ++f)
        {
            for (int i = 0; i < N; ++i)
            {
                b[i].x += b[i].vx;
                b[i].y += b[i].vy;
                if (b[i].x - b[i].r < 0)
                {
                    b[i].x = (float)b[i].r;
                    b[i].vx = -b[i].vx;
                }
                if (b[i].x + b[i].r > w)
                {
                    b[i].x = w - b[i].r;
                    b[i].vx = -b[i].vx;
                }
                if (b[i].y - b[i].r < 0)
                {
                    b[i].y = (float)b[i].r;
                    b[i].vy = -b[i].vy;
                }
                if (b[i].y + b[i].r > h)
                {
                    b[i].y = h - b[i].r;
                    b[i].vy = -b[i].vy;
                }
            }
            // 简单碰撞
            for (int i = 0; i < N; ++i)
                for (int j = i + 1; j < N; ++j)
                {
                    float dx = b[i].x - b[j].x, dy = b[i].y - b[j].y;
                    float dist = sqrtf(dx * dx + dy * dy);
                    if (dist < b[i].r + b[j].r && dist > 0)
                    {
                        float nx = dx / dist, ny = dy / dist;
                        float dvx = b[i].vx - b[j].vx, dvy = b[i].vy - b[j].vy;
                        float dvn = dvx * nx + dvy * ny;
                        if (dvn < 0)
                        {
                            b[i].vx -= dvn * nx;
                            b[i].vy -= dvn * ny;
                            b[j].vx += dvn * nx;
                            b[j].vy += dvn * ny;
                        }
                        float overlap = b[i].r + b[j].r - dist;
                        b[i].x += nx * overlap / 2;
                        b[i].y += ny * overlap / 2;
                        b[j].x -= nx * overlap / 2;
                        b[j].y -= ny * overlap / 2;
                    }
                }
            d.Fill(C_BLACK);
            uint16_t* fb = d.GetFramebuffer();
            for (int i = 0; i < N; ++i)
            {
                int cx = (int)b[i].x, cy = (int)b[i].y, rr = b[i].r;
                int x = 0, y2 = rr, err = 3 - 2 * rr;
                while (y2 >= x)
                {
                    DrawHLine(fb, w, h, cx - x, cy + y2, 2 * x + 1, b[i].c);
                    DrawHLine(fb, w, h, cx - x, cy - y2, 2 * x + 1, b[i].c);
                    DrawHLine(fb, w, h, cx - y2, cy + x, 2 * y2 + 1, b[i].c);
                    DrawHLine(fb, w, h, cx - y2, cy - x, 2 * y2 + 1, b[i].c);
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
            d.Flush();
            vTaskDelay(pdMS_TO_TICKS(16));
        }
    }

    // ====== 2. 星空视差 ======
    {
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
            uint16_t cc[] = {C_DARK, C_GRAY, C_WHITE, C_CYAN};
            stars[i].c = cc[stars[i].speed - 1];
        }
        for (int f = 0; f < 400; ++f)
        {
            d.Fill(C_BLACK);
            uint16_t* fb = d.GetFramebuffer();
            for (int i = 0; i < N; ++i)
            {
                stars[i].x = (stars[i].x + stars[i].speed) % w;
                if (stars[i].x >= 0 && stars[i].x < w && stars[i].y >= 0 && stars[i].y < h)
                    fb[stars[i].y * w + stars[i].x] = stars[i].c;
            }
            d.Flush();
            vTaskDelay(pdMS_TO_TICKS(16));
        }
    }

    // ====== 4. 旋转 3D 立方体 ======
    {
        const float verts[8][3] = {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
                                   {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}};
        const int edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
                                  {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
        for (int f = 0; f < 500; ++f)
        {
            d.Fill(C_BLACK);
            uint16_t* fb = d.GetFramebuffer();
            float a = f * 0.03f, ca = cosf(a), sa = sinf(a);
            int sx[8], sy[8];
            for (int i = 0; i < 8; ++i)
            {
                float rx = verts[i][0] * ca - verts[i][2] * sa;
                float rz = verts[i][0] * sa + verts[i][2] * ca;
                float ca2 = cosf(a * 0.7f), sa2 = sinf(a * 0.7f);
                float ry = verts[i][1] * ca2 - rz * sa2;
                rz = verts[i][1] * sa2 + rz * ca2;
                sx[i] = (int)(rx * 150 + w / 2);
                sy[i] = (int)(-ry * 150 + h / 2) + (int)(rz * 30);
            }
            for (auto& e : edges)
                DrawLine(fb, w, h, sx[e[0]], sy[e[0]], sx[e[1]], sy[e[1]], ((e[0] + e[1]) % 2) ? C_CYAN : C_MAGENTA);
            for (int i = 0; i < 8; ++i) FillRect(fb, w, h, sx[i] - 3, sy[i] - 3, 6, 6, C_YELLOW);
            d.Flush();
            vTaskDelay(pdMS_TO_TICKS(16));
        }
    }

    // ====== 5. 炫彩弹跳方块收尾 ======
    {
        int bx = w / 2 - 40, by = h / 2 - 40, bvx = 4, bvy = 3, bs = 80;
        uint16_t cc = C_RED;
        for (int f = 0; f < 300; ++f)
        {
            bx += bvx;
            by += bvy;
            if (bx <= 0 || bx + bs >= w)
            {
                bvx = -bvx;
                cc = (cc == C_RED) ? C_CYAN : (cc == C_CYAN ? C_YELLOW : C_RED);
            }
            if (by <= 0 || by + bs >= h)
            {
                bvy = -bvy;
                cc = (cc == C_GREEN) ? C_MAGENTA : C_GREEN;
            }
            if (bx < 0) bx = 0;
            if (by < 0) by = 0;
            if (bx + bs > w) bx = w - bs;
            if (by + bs > h) by = h - bs;
            d.Fill(C_BLACK);
            FillRect(d.GetFramebuffer(), w, h, bx, by, bs, bs, cc);
            d.Flush();
            vTaskDelay(pdMS_TO_TICKS(16));
        }
    }
}

extern "C" void app_main(void)
{
    StatusKey::GetInstance().InitKeys();
    Encoder::GetInstance().Init();
    DeviceInit::GetInstance().Init();
    TemperatureMonitor::GetInstance().TemperatureMonitorInit();
    AdjustablePSU::GetInstance().AdjustablePSUInit();
    PerfMonitor::GetInstance().Init();
    Buzzer::GetInstance().Init();

    Display::GetInstance().Init({});
    DisplayTest();
}
