#include "buzzer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "Buzzer";

bool Buzzer::Init(gpio_num_t pin)
{
    pin_ = pin;

    // 配置 LEDC 定时器
    ledc_timer_config_t timer_cfg = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_RESOLUTION,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQ_HZ,  // 1MHz 基频
        .clk_cfg = LEDC_AUTO_CLK,
    };
    if (ledc_timer_config(&timer_cfg) != ESP_OK)
    {
        ESP_LOGE(TAG, "LEDC timer config failed");
        return false;
    }

    // 配置 LEDC 通道
    ledc_channel_config_t ch_cfg = {
        .gpio_num = pin,
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER,
        .duty = 0,  // 初始静音
        .hpoint = 0,
    };
    if (ledc_channel_config(&ch_cfg) != ESP_OK)
    {
        ESP_LOGE(TAG, "LEDC channel config failed");
        return false;
    }

    // 创建命令队列 + 监听 Task
    cmd_queue_ = xQueueCreate(CMD_QUEUE_LEN, sizeof(Command));
    if (!cmd_queue_)
    {
        ESP_LOGE(TAG, "Queue create failed");
        return false;
    }

    xTaskCreate(BuzzerTask, "BuzzerTask", 2048, this, 3, nullptr);

    initialized_ = true;
    ESP_LOGI(TAG, "Init buzzer on GPIO %d", pin);
    return true;
}

void Buzzer::SetVolume(uint8_t percent)
{
    if (percent > 100)
    {
        percent = 100;
    }
    volume_ = percent;
    ESP_LOGI(TAG, "Volume: %u%%", percent);
}

void Buzzer::Play(uint16_t freq_hz)
{
    // 无源蜂鸣器最大音量在 50% 占空比，100%=0% 都静音
    // volume 100 → MAX_DUTY/2 (最响), volume 0 → 0 (静音)
    uint32_t duty = (static_cast<uint32_t>(volume_) * MAX_DUTY) / 200;
    Play(freq_hz, duty);
}

void Buzzer::Play(Note note)
{
    Play(static_cast<uint16_t>(note));
}

void Buzzer::Play(uint16_t freq_hz, uint32_t duty)
{
    if (!initialized_)
    {
        ESP_LOGE(TAG, "Not initialized");
        return;
    }

    if (duty > MAX_DUTY)
    {
        duty = MAX_DUTY;
    }

    if (freq_hz == 0)
    {
        Stop();
        return;
    }

    // 固定 10-bit 分辨率配置定时器，避免 ledc_set_freq 自动改变分辨率导致占空比错乱
    ledc_timer_config_t timer_cfg = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_RESOLUTION,
        .timer_num = LEDC_TIMER,
        .freq_hz = freq_hz,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer_cfg);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

void Buzzer::Stop()
{
    if (!initialized_)
    {
        return;
    }

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

void Buzzer::Beep(uint32_t duration_ms, uint16_t freq_hz)
{
    Play(freq_hz);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    Stop();
}

void Buzzer::Alarm(uint32_t duration_ms)
{
    uint32_t start = esp_timer_get_time() / 1000;

    do
    {
        Play(2700);
        vTaskDelay(pdMS_TO_TICKS(100));
        Stop();  // 静音间隙
        vTaskDelay(pdMS_TO_TICKS(80));
    } while (duration_ms == 0 || (esp_timer_get_time() / 1000 - start) < duration_ms);

    Stop();
}

void Buzzer::SelfTest()
{
    ESP_LOGI(TAG, "=== SelfTest Start ===");

    // 1. 短促提示音
    Beep(100);
    vTaskDelay(pdMS_TO_TICKS(200));

    // 2. 上行音阶
    Note notes[] = {Note::C4, Note::D4, Note::E4, Note::F4, Note::G4, Note::A4, Note::B4};
    for (auto n : notes)
    {
        Play(n);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    Stop();
    vTaskDelay(pdMS_TO_TICKS(300));

    // 3. 警报测试 (1秒)
    Alarm(1000);

    ESP_LOGI(TAG, "=== SelfTest End ===");
}

// ====== 异步通知（其他 Task 通过这些方法发送命令） ======

bool Buzzer::SendCommand(const Command& cmd)
{
    if (!cmd_queue_)
    {
        return false;
    }
    return xQueueSend(cmd_queue_, &cmd, 0) == pdTRUE;
}

bool Buzzer::NotifyBeep(uint32_t duration_ms, uint16_t freq_hz)
{
    return SendCommand({Cmd::BEEP, freq_hz, duration_ms});
}

bool Buzzer::NotifyAlarm(uint32_t duration_ms)
{
    return SendCommand({Cmd::ALARM, 2700, duration_ms});
}

bool Buzzer::NotifyStop()
{
    return SendCommand({Cmd::STOP});
}

bool Buzzer::NotifySelfTest()
{
    return SendCommand({Cmd::SELFTEST});
}

// ====== 内部监听 Task ======

void Buzzer::BuzzerTask(void* pvParameters)
{
    static_cast<Buzzer*>(pvParameters)->TaskLoop();
}

void Buzzer::TaskLoop()
{
    Command cmd;

    while (true)
    {
        if (xQueueReceive(cmd_queue_, &cmd, portMAX_DELAY) == pdTRUE)
        {
            switch (cmd.type)
            {
                case Cmd::BEEP:
                    Beep(cmd.duration_ms, cmd.freq_hz);
                    break;
                case Cmd::ALARM:
                    Alarm(cmd.duration_ms);
                    break;
                case Cmd::STOP:
                    Stop();
                    break;
                case Cmd::SELFTEST:
                    SelfTest();
                    break;
                default:
                    break;
            }
        }
    }
}
