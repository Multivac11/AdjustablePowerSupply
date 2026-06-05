#include "encoder.h"

static const char* TAG = "Encoder";

bool Encoder::Init(gpio_num_t pin_a, gpio_num_t pin_b)
{
    // PCNT 单元配置
    pcnt_unit_config_t unit_cfg = {
        .low_limit = -PCNT_LIMIT,
        .high_limit = PCNT_LIMIT,
        .intr_priority = 0,
    };
    if (pcnt_new_unit(&unit_cfg, &pcnt_unit_) != ESP_OK)
    {
        ESP_LOGE(TAG, "PCNT unit create failed");
        return false;
    }

    pcnt_glitch_filter_config_t filter_cfg = {
        .max_glitch_ns = 5000,  // 5µs，滤除高频毛刺
    };
    pcnt_unit_set_glitch_filter(pcnt_unit_, &filter_cfg);

    // PCNT 通道 — 正交解码
    pcnt_chan_config_t chan_cfg = {
        .edge_gpio_num = pin_a,
        .level_gpio_num = pin_b,
    };
    if (pcnt_new_channel(pcnt_unit_, &chan_cfg, &pcnt_chan_) != ESP_OK)
    {
        ESP_LOGE(TAG, "PCNT channel create failed");
        return false;
    }

    // 单沿触发：上升沿 HOLD，下降沿 INCREASE
    pcnt_channel_set_edge_action(pcnt_chan_,
                                 PCNT_CHANNEL_EDGE_ACTION_INCREASE,  // 上升沿：不动作
                                 PCNT_CHANNEL_EDGE_ACTION_HOLD);     // 下降沿：基础 +1

    // B 相电平决定方向
    pcnt_channel_set_level_action(pcnt_chan_,
                                  PCNT_CHANNEL_LEVEL_ACTION_KEEP,      // B=高: 保持 (+1)
                                  PCNT_CHANNEL_LEVEL_ACTION_INVERSE);  // B=低: 反转 (-1)

    // 上拉
    gpio_set_pull_mode(pin_a, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(pin_b, GPIO_PULLUP_ONLY);

    if (pcnt_unit_enable(pcnt_unit_) != ESP_OK || pcnt_unit_clear_count(pcnt_unit_) != ESP_OK ||
        pcnt_unit_start(pcnt_unit_) != ESP_OK)
    {
        ESP_LOGE(TAG, "PCNT start failed");
        return false;
    }

    xTaskCreate(EncoderTask, "EncoderTask", 2048, this, 4, nullptr);

    ESP_LOGI(TAG, "Init encoder A=%d B=%d", pin_a, pin_b);
    return true;
}

Encoder::~Encoder()
{
    if (pcnt_unit_)
    {
        pcnt_unit_stop(pcnt_unit_);
        pcnt_unit_disable(pcnt_unit_);
    }
    if (pcnt_chan_)
    {
        pcnt_del_channel(pcnt_chan_);
    }
    if (pcnt_unit_)
    {
        pcnt_del_unit(pcnt_unit_);
    }
}

void Encoder::ResetPosition()
{
    position_ = 0;
    pcnt_unit_clear_count(pcnt_unit_);
}

bool Encoder::RegisterListener(QueueHandle_t queue)
{
    if (!queue || listener_count_ >= MAX_LISTENERS) return false;

    for (int i = 0; i < listener_count_; ++i)
    {
        if (listeners_[i] == queue) return true;
    }

    listeners_[listener_count_++] = queue;
    return true;
}

bool Encoder::UnregisterListener(QueueHandle_t queue)
{
    for (int i = 0; i < listener_count_; ++i)
    {
        if (listeners_[i] == queue)
        {
            for (int j = i; j < listener_count_ - 1; ++j) listeners_[j] = listeners_[j + 1];
            listeners_[--listener_count_] = nullptr;
            return true;
        }
    }
    return false;
}

void Encoder::EncoderTask(void* pvParameters)
{
    static_cast<Encoder*>(pvParameters)->TaskLoop();
}

void Encoder::TaskLoop()
{
    while (true)
    {
        int count = 0;
        if (cooldown_ > 0)
        {
            cooldown_--;
        }
        if (pcnt_unit_get_count(pcnt_unit_, &count) == ESP_OK && count != 0)
        {
            int32_t delta = count;
            int32_t dir = (delta > 0) ? 1 : -1;
            pcnt_unit_clear_count(pcnt_unit_);

            // 冷却期内：同向放行、反向丢弃（抑制回弹）
            if (cooldown_ > 0 && dir != last_dir_)
            {
                // 反向 → 回弹，忽略
            }
            else
            {
                int32_t steps = (delta > 0) ? delta : -delta;
                position_ += delta;
                cooldown_ = COOLDOWN_MAX;
                last_dir_ = dir;

                ev_.position = dir;  // +1 或 -1
                ESP_LOGI(TAG, "Encoder %d position %d", ev_.position, position_);

                for (int s = 0; s < steps; ++s)
                {
                    for (int i = 0; i < listener_count_; ++i)
                    {
                        if (listeners_[i])
                        {
                            Event* p = &ev_;
                            xQueueOverwrite(listeners_[i], &p);
                        }
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
