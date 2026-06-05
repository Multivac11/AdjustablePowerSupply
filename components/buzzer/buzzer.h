#pragma once

#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

// 无源蜂鸣器 — LEDC PWM 驱动，内部 Task 监听命令队列
class Buzzer
{
   public:
    // 预定义音符频率 (Hz)
    enum class Note : uint16_t
    {
        C4 = 262,   // Do
        D4 = 294,   // Re
        E4 = 330,   // Mi
        F4 = 349,   // Fa
        G4 = 392,   // Sol
        A4 = 440,   // La
        B4 = 494,   // Si
        C5 = 523,
        D5 = 587,
        E5 = 659,
        F5 = 698,
        G5 = 784,
        SILENCE = 0,
    };

    // 命令类型（其他 Task 通过 Notify* 发送）
    enum class Cmd : uint8_t
    {
        NONE = 0,
        BEEP,
        ALARM,
        STOP,
        SELFTEST,
    };

    struct Command
    {
        Cmd type = Cmd::NONE;
        uint16_t freq_hz = 2000;
        uint32_t duration_ms = 0;  // Alarm: 0=无限
    };

    static Buzzer& GetInstance()
    {
        static Buzzer instance;
        return instance;
    }

    // 初始化 PWM（GPIO + LEDC 通道配置），同时启动监听 Task
    bool Init(gpio_num_t pin = GPIO_NUM_21);

    // 音量（0~100，0=静音 100=最大）
    void SetVolume(uint8_t percent);
    uint8_t GetVolume() const { return volume_; }

    // ====== 同步方法（调用者 Task 阻塞） ======
    void Play(uint16_t freq_hz);
    void Play(Note note);
    void Play(uint16_t freq_hz, uint32_t duty);
    void Stop();
    void Beep(uint32_t duration_ms, uint16_t freq_hz = 2000);
    void Alarm(uint32_t duration_ms = 0);
    void SelfTest();

    // ====== 异步方法（发命令到队列，立即返回，由内部 Task 执行） ======
    // 其他 Task 通过这些方法通知 Buzzer
    bool NotifyBeep(uint32_t duration_ms, uint16_t freq_hz = 2000);
    bool NotifyAlarm(uint32_t duration_ms = 0);
    bool NotifyStop();
    bool NotifySelfTest();

   private:
    Buzzer() = default;
    ~Buzzer() = default;

    static void BuzzerTask(void* pvParameters);
    void TaskLoop();

    bool SendCommand(const Command& cmd);

    bool initialized_ = false;
    gpio_num_t pin_ = GPIO_NUM_NC;
    uint8_t volume_ = 50;

    QueueHandle_t cmd_queue_ = nullptr;
    static constexpr UBaseType_t CMD_QUEUE_LEN = 8;

    static constexpr ledc_mode_t LEDC_MODE = LEDC_LOW_SPEED_MODE;
    static constexpr ledc_timer_t LEDC_TIMER = LEDC_TIMER_0;
    static constexpr ledc_channel_t LEDC_CHANNEL = LEDC_CHANNEL_0;
    static constexpr uint32_t LEDC_FREQ_HZ = 1000;
    static constexpr ledc_timer_bit_t LEDC_RESOLUTION = LEDC_TIMER_10_BIT;
    static constexpr uint32_t MAX_DUTY = 1023;
};
