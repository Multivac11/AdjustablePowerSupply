#pragma once

#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define MAX_LISTENERS 10

// EC11 旋钮编码器 — 硬件 PCNT 正交解码，A/B 相默认上拉，C 接地
class Encoder
{
   public:
    enum class Direction : int8_t
    {
        NONE = 0,
        CW = 1,
        CCW = -1,
    };

    struct Event
    {
        Direction dir;
        int32_t position;
    };

    static Encoder& GetInstance()
    {
        static Encoder instance;
        return instance;
    }

    // 初始化（默认 A=GPIO10, B=GPIO11），启动监听 Task
    bool Init(gpio_num_t pin_a = GPIO_NUM_10, gpio_num_t pin_b = GPIO_NUM_11);

    // 读取 / 重置当前位置
    int32_t GetPosition() const { return position_; }
    void ResetPosition();

    // 注册 / 注销监听者
    bool RegisterListener(QueueHandle_t queue);
    bool UnregisterListener(QueueHandle_t queue);

   private:
    Encoder() = default;
    ~Encoder();

    static void EncoderTask(void* pvParameters);
    void TaskLoop();

    pcnt_unit_handle_t pcnt_unit_ = nullptr;
    pcnt_channel_handle_t pcnt_chan_ = nullptr;

    volatile int32_t position_ = 0;
    Event ev_ = {Direction::NONE, 0};
    int cooldown_ = 0;  // 方向冷却计数，抑制回弹
    int32_t last_dir_ = 0;

    static constexpr int32_t PCNT_LIMIT = 32767;
    static constexpr int COOLDOWN_MAX = 10;  // 20 × 20ms = 400ms 冷却

    QueueHandle_t listeners_[MAX_LISTENERS] = {};
    uint8_t listener_count_ = 0;
};
