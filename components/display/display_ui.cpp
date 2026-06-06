#include "display_ui.h"

static const char* TAG = "DisplayUI";

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

    // ====== 1. 十球物理碰撞 + 中心引力井 ======
    {
        const int N = 10;
        struct
        {
            float x, y, vx, vy;
            int r;
            uint16_t c;
        } b[N];
        uint16_t colors[] = {C_RED, C_GREEN, C_BLUE, C_YELLOW, C_CYAN, C_MAGENTA, C_ORANGE, C_WHITE, 0x07E0, 0xF80F};
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
            d.Fill(C_BLACK);
            uint16_t* fb = d.GetFramebuffer();
            // 引力井波纹
            for (int ring = 0; ring < 3; ++ring)
            {
                int rr2 = 30 + ring * 15 + (f % 15);
                int cx2 = 0, cy2 = rr2, e2 = 3 - 2 * rr2;
                while (cy2 >= cx2)
                {
                    uint16_t rc = (ring == 0) ? C_DARK : C_DARK;
                    DrawHLine(fb, w, h, (int)gx - cx2, (int)gy + cy2, 2 * cx2 + 1, rc);
                    DrawHLine(fb, w, h, (int)gx - cx2, (int)gy - cy2, 2 * cx2 + 1, rc);
                    DrawHLine(fb, w, h, (int)gx - cy2, (int)gy + cx2, 2 * cy2 + 1, rc);
                    DrawHLine(fb, w, h, (int)gx - cy2, (int)gy - cx2, 2 * cy2 + 1, rc);
                    if (e2 < 0)
                        e2 += 4 * cx2 + 6;
                    else
                    {
                        e2 += 4 * (cx2 - cy2) + 10;
                        cy2--;
                    }
                    cx2++;
                }
            }
            // 球体
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
                // 高光
                int hs = rr / 3;
                if (hs > 1) FillRect(fb, w, h, cx - rr / 3, cy - rr / 3, hs, hs, C_WHITE);
            }
            d.Flush();
            vTaskDelay(pdMS_TO_TICKS(16));
        }
    }

    // ====== 2. 星空视差（十字星芒） ======
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
                    if (sx > 1) fb[sy * w + sx - 2] = C_DARK;
                    if (sx < w - 2) fb[sy * w + sx + 2] = C_DARK;
                    if (sy > 1) fb[(sy - 2) * w + sx] = C_DARK;
                    if (sy < h - 2) fb[(sy + 2) * w + sx] = C_DARK;
                }
            }
            d.Flush();
            vTaskDelay(pdMS_TO_TICKS(16));
        }
    }

    // ====== 4. 双立方体交织旋转 + 色彩循环 ======
    {
        const float verts[8][3] = {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
                                   {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}};
        const int edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
                                  {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
        uint16_t palette[] = {C_RED, C_ORANGE, C_YELLOW, C_GREEN, C_CYAN, C_BLUE, C_MAGENTA, C_WHITE};
        for (int f = 0; f < 600; ++f)
        {
            d.Fill(C_BLACK);
            uint16_t* fb = d.GetFramebuffer();
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
            for (int i = 0; i < 8; ++i) DrawLine(fb, w, h, ox[i], oy[i], ix[i], iy[i], C_DARK);
            // 外顶点光晕
            for (int i = 0; i < 8; ++i)
            {
                FillRect(fb, w, h, ox[i] - 4, oy[i] - 4, 9, 9, C_WHITE);
                FillRect(fb, w, h, ox[i] - 2, oy[i] - 2, 5, 5, palette[(cs + i) % 8]);
            }
            // 内顶点
            for (int i = 0; i < 8; ++i)
            {
                FillRect(fb, w, h, ix[i] - 2, iy[i] - 2, 5, 5, C_YELLOW);
            }
            d.Flush();
            vTaskDelay(pdMS_TO_TICKS(16));
        }
    }

    // ====== 5. 多彩方块碰撞混战 ======
    {
        const int NS = 5;
        struct
        {
            int x, y, vx, vy, s;
            uint16_t c;
        } sq[NS] = {
            {w / 2 - 40, h / 2 - 40, 5, 3, 80, C_RED}, {100, 200, -4, 6, 50, C_CYAN},    {300, 500, 6, -5, 60, C_GREEN},
            {200, 100, -5, -6, 45, C_YELLOW},          {380, 300, 4, -7, 70, C_MAGENTA},
        };
        uint16_t pal[] = {C_RED, C_CYAN, C_GREEN, C_YELLOW, C_MAGENTA, C_ORANGE, C_BLUE, C_WHITE};
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
            d.Fill(C_BLACK);
            uint16_t* fb = d.GetFramebuffer();
            for (int i = 0; i < NS; ++i)
            {
                FillRect(fb, w, h, sq[i].x, sq[i].y, sq[i].s, sq[i].s, sq[i].c);
                // 白色边框
                DrawHLine(fb, w, h, sq[i].x, sq[i].y, sq[i].s, C_WHITE);
                DrawHLine(fb, w, h, sq[i].x, sq[i].y + sq[i].s - 1, sq[i].s, C_WHITE);
                for (int dy = 1; dy < sq[i].s - 1; ++dy)
                {
                    int row = sq[i].y + dy;
                    if (row >= 0 && row < h)
                    {
                        if (sq[i].x >= 0 && sq[i].x < w) fb[row * w + sq[i].x] = C_WHITE;
                        int rx = sq[i].x + sq[i].s - 1;
                        if (rx >= 0 && rx < w) fb[row * w + rx] = C_WHITE;
                    }
                }
            }
            d.Flush();
            vTaskDelay(pdMS_TO_TICKS(16));
        }
    }
}

void DisplayUI::Init()
{
    Display::GetInstance().Init({});
    DisplayTest();
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
