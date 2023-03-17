#ifndef LED_H
#define LED_H

#include <memory>

#include "esp_timer.h"

class Led {
 public:
  Led();
  ~Led();
  void init();

  void on();
  void off();
  void toggle();
  void startBlinking();
  void stopBlinking();

 private:
  static void espTimerCallback(void* arg);

  uint8_t mState{0};
  esp_timer_handle_t mHandle{nullptr};
};

#endif  // LED_H