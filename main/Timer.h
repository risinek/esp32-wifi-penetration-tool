#ifndef TIMER_H
#define TIMER_H

#include <cstdint>
#include <functional>
#include <memory>

#include "Task.h"
#include "esp_timer.h"

class Timer {
 public:
  using TimeoutCallback = std::function<void()>;
  class TimerTask : public Task {
   public:
    TimerTask() : Task("TimerTask") {}
    void run(void* data) override;
  };

  Timer() = default;
  ~Timer();

  void start(uint32_t timeoutS, TimeoutCallback timeoutCallback, bool isPeriodic = false);
  void stop();
  void reset();

 private:
  static void espTimerCallback(void* arg);

  uint32_t mTimeoutS;
  esp_timer_handle_t mHandle{nullptr};
  TimeoutCallback mTimeoutCallback{nullptr};
  bool mIsPeriodic{false};
  std::unique_ptr<TimerTask> mTimerTask;
};

#endif  // TIMER_H
