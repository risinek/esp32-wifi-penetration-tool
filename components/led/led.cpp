#include "led.h"

#include "driver/gpio.h"

namespace {
// NOTE! There is no way to change state of red LED on ESP32 because it is hard-wired
constexpr gpio_num_t gLedPin{GPIO_NUM_2};
}  // namespace

Led::Led() {
  ESP_ERROR_CHECK(gpio_reset_pin(gLedPin));
  ESP_ERROR_CHECK(gpio_set_direction(gLedPin, GPIO_MODE_OUTPUT));  // Set the GPIO as a push/pull output
}

void Led::init() {
  esp_timer_create_args_t args{};
  args.callback = &espTimerCallback;
  args.arg = (void*)this;
  ESP_ERROR_CHECK(esp_timer_create(&args, &mHandle));
}

Led::~Led() { ESP_ERROR_CHECK(esp_timer_delete(mHandle)); }

void Led::on() {
  stopBlinking();
  mState = 1;
  ESP_ERROR_CHECK(gpio_set_level(gLedPin, mState));
}

void Led::off() {
  stopBlinking();
  mState = 0;
  ESP_ERROR_CHECK(gpio_set_level(gLedPin, mState));
}

void Led::toggle() {
  mState++;
  mState %= 2;
  ESP_ERROR_CHECK(gpio_set_level(gLedPin, mState));
}

void Led::startBlinking() {
  stopBlinking();
  ESP_ERROR_CHECK(esp_timer_start_periodic(mHandle, 1 * 1000000 / 2));  // Period is 0.5 sec
}

void Led::stopBlinking() { esp_timer_stop(mHandle); }

void Led::espTimerCallback(void* arg) { reinterpret_cast<Led*>(arg)->toggle(); }
