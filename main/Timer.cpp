#include "Timer.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

void Timer::GentlyStoppableTimerTask::run(void* data) {
  auto callback = reinterpret_cast<Timer::TimeoutCallback*>(data);
  mFinishedPromise = std::promise<void>{};
  mIsTaskFinished.store(false);
  (*callback)(mIsStopRequested);
  mIsTaskFinished.store(true);
  mFinishedPromise.set_value();
}

void Timer::GentlyStoppableTimerTask::stop() {
  if (mIsTaskFinished.load()) {
    Task::stop();  // Call method from parent class. Destroys task
    return;
  }

  mIsStopRequested = true;
  mFinishedPromise.get_future().wait();
  Task::stop();  // Call method from parent class. Destroys task
}

Timer::~Timer() { stop(); }

void Timer::start(uint32_t timeoutS, TimeoutCallback timeoutCallback, bool isPeriodic) {
  mTimeoutS = timeoutS;
  mTimeoutCallback = std::move(timeoutCallback);
  mIsPeriodic = isPeriodic;
  esp_timer_create_args_t args{};
  args.callback = &espTimerCallback;
  args.arg = (void*)this;
  ESP_ERROR_CHECK(esp_timer_create(&args, &mHandle));
  if (mIsPeriodic) {
    ESP_ERROR_CHECK(esp_timer_start_periodic(mHandle, mTimeoutS * 1000000));
  } else {
    ESP_ERROR_CHECK(esp_timer_start_once(mHandle, mTimeoutS * 1000000));
  }
}

void Timer::reset() {
  if (mHandle == nullptr) {
    return;
  }
  if (esp_timer_is_active(mHandle)) {
    ESP_ERROR_CHECK(esp_timer_restart(mHandle, mTimeoutS * 1000000));
  } else {
    if (mIsPeriodic) {
      ESP_ERROR_CHECK(esp_timer_start_periodic(mHandle, mTimeoutS * 1000000));
    } else {
      ESP_ERROR_CHECK(esp_timer_start_once(mHandle, mTimeoutS * 1000000));
    }
  }
}

void Timer::stop() {
  if (mHandle == nullptr) {
    return;
  }
  if (esp_timer_is_active(mHandle)) {
    ESP_ERROR_CHECK(esp_timer_stop(mHandle));
  }
  ESP_ERROR_CHECK(esp_timer_delete(mHandle));
  mHandle = nullptr;
  if (mTimerTask) {
    mTimerTask->stop();
    mTimerTask.reset();
  }
}

void Timer::espTimerCallback(void* arg) {
  auto thisTimer = reinterpret_cast<Timer*>(arg);
  if (thisTimer->mTimeoutCallback == nullptr) {
    return;
  }
  thisTimer->mTimerTask = std::make_unique<GentlyStoppableTimerTask>();
  thisTimer->mTimerTask->start(&thisTimer->mTimeoutCallback);
}
