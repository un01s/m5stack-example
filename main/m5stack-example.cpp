#include <algorithm>
#include <cmath>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <M5GFX.h>
#include <driver/gpio.h>
#include <driver/ledc.h>

namespace MyApp {

constexpr auto PI = 3.14159265358979323846;

struct ButtonState {
  bool button_a, button_b, button_c;
};

class MyDisplay {
public:
  MyDisplay() : display_(), canvas_(&display_) {}

  void initialize() {
    display_.init();
    display_.startWrite();
    canvas_.createSprite(display_.width(), display_.height());
  }

  void render(const ButtonState &button) {

    if (button.button_a) {
      time_ += 4;
    }
    if (!button.button_b) {
      time_ += 1;
    }
    if (button.button_c) {
      time_ -= 6;
    }

    int kW = display_.width();
    int kH = display_.height();
    int kL = std::min(kW, kH) * 4 / 10;
    display_.waitDisplay();
    canvas_.fillRect(0, 0, kW, kH, TFT_BLACK);

    float a1 = 0.03 * time_;
    float a2 = a1 + PI * 2 / 3;
    float a3 = a1 - PI * 2 / 3;
    canvas_.fillTriangle(std::cos(a1) * kL + kW / 2, std::sin(a1) * kL + kH / 2,
                         std::cos(a2) * kL + kW / 2, std::sin(a2) * kL + kH / 2,
                         std::cos(a3) * kL + kW / 2, std::sin(a3) * kL + kH / 2,
                         TFT_WHITE);

    canvas_.pushSprite(0, 0);
    display_.display();
  }

private:
  M5GFX display_;
  M5Canvas canvas_;

  int time_ = 0;

} g_my_display;

class MyIO {
public:
  MyIO() {}

  void initialize() {
    {
      // buttons
      gpio_config_t config = {};
      config.intr_type = GPIO_INTR_DISABLE;
      config.pin_bit_mask =
          (1ull << GPIO_NUM_39) | (1ull << GPIO_NUM_38) | (1ull << GPIO_NUM_37);
      config.mode = GPIO_MODE_INPUT;
      config.pull_up_en = GPIO_PULLUP_ENABLE;
      gpio_config(&config);
    }
    {
      // speaker
      gpio_config_t config;
      config.intr_type = GPIO_INTR_DISABLE;
      config.pin_bit_mask = (1ull << GPIO_NUM_25);
      config.mode = GPIO_MODE_OUTPUT;
      config.pull_up_en = GPIO_PULLUP_DISABLE;
      config.pull_down_en = GPIO_PULLDOWN_DISABLE;
      gpio_config(&config);
    }
  }

  ButtonState update() {
    bool a = gpio_get_level(GPIO_NUM_39);
    bool b = gpio_get_level(GPIO_NUM_38);
    bool c = gpio_get_level(GPIO_NUM_37);
    return ButtonState{a, b, c};
  }

  void setSound(uint32_t freq, bool enable) {
    ledc_timer_config_t timer_config = {};
    timer_config.speed_mode = LEDC_HIGH_SPEED_MODE;
    timer_config.duty_resolution = LEDC_TIMER_8_BIT;
    timer_config.timer_num = LEDC_TIMER_3;
    timer_config.freq_hz = freq;
    timer_config.clk_cfg = LEDC_AUTO_CLK;
    auto tres = ledc_timer_config(&timer_config);

    ledc_channel_config_t channel_config = {};
    channel_config.gpio_num = GPIO_NUM_25;
    channel_config.speed_mode = LEDC_HIGH_SPEED_MODE;
    channel_config.channel = LEDC_CHANNEL_1;
    channel_config.intr_type = LEDC_INTR_DISABLE;
    channel_config.timer_sel = timer_config.timer_num;
    channel_config.duty = enable ? 0x7F : 0x00;
    channel_config.hpoint = 0x0;
    auto cres = ledc_channel_config(&channel_config);
    ESP_LOGI("APP", "XXXX timer_config_res=%d channel_config=%d", tres, cres);
  }

private:
} g_my_io;

void runSpeakerLoop(void *args) {
  vTaskDelay(3000 / portTICK_PERIOD_MS);
  g_my_io.setSound(262, true);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  g_my_io.setSound(294, true);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  g_my_io.setSound(330, true);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  g_my_io.setSound(349, true);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  g_my_io.setSound(392, true);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  g_my_io.setSound(440, true);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  g_my_io.setSound(494, true);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  g_my_io.setSound(523, true);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  g_my_io.setSound(523, false);
  vTaskDelay(3000 / portTICK_PERIOD_MS);
  vTaskDelete(nullptr);
}

void runMainLoop(void *args) {
  g_my_display.initialize();
  g_my_io.initialize();
  xTaskCreate(&runSpeakerLoop, "task2-speaker", 8192, nullptr, 2, nullptr);
  for (;;) {
    auto button = g_my_io.update();
    g_my_display.render(button);
    vTaskDelay(40 / portTICK_PERIOD_MS);
  }
}

void initializeTask() {
  xTaskCreatePinnedToCore(&runMainLoop, "task1-main", 8192, nullptr, 2, nullptr,
                          1);
}

} // namespace MyApp

extern "C" {
void app_main(void) {
  //
  MyApp::initializeTask();
}
}

