#ifndef TIMER_H
#define TIMER_H

#include <atomic>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>

#include "Task.h"
#include "esp_timer.h"

class Timer {
 public:
  using TimeoutCallback = std::function<void(const bool& isStopRequested)>;
  // This version of Task will not simply kill whatever was executing at the moment stop() method is called. It will ask
  // callback to stop and wait till it will be stopped.
  // So, if you make some heavy processing in callback, make sure to properly react on isStopRequested, which is passed
  // as parameter
  class GentlyStoppableTimerTask : public Task {
   public:
    GentlyStoppableTimerTask() : Task("GentlyStoppableTimerTask") {}
    void run(void* data) override;
    void stop();

   private:
    std::atomic<bool> mIsTaskFinished{false};
    bool mIsStopRequested{false};  // Have to use non-atomic type, because this variable is passed into C code
    std::promise<void> mFinishedPromise;
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
  std::unique_ptr<GentlyStoppableTimerTask> mTimerTask;
};

#endif  // TIMER_H
