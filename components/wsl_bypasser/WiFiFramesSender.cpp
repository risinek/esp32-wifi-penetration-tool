#include "WiFiFramesSender.h"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <vector>

#include "Task.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

namespace {
const char* LOG_TAG = "WFS";

class WiFiFrameSenderTask : public Task {
 public:
  WiFiFrameSenderTask() : Task("WiFiFrameSenderTask") {}
  void run(void* data) override;

  void addFrame(const uint8_t* frame_buffer, uint32_t size);

 private:
  std::queue<std::vector<uint8_t>> mFrames;
  std::mutex mMutex;
  std::condition_variable mCondVar;
};
WiFiFrameSenderTask gWiFiFrameSenderTask;
}  // namespace

void init_wifi_frame_sender() { gWiFiFrameSenderTask.start(nullptr); }

void wsl_bypasser_send_raw_frame(const uint8_t* frame_buffer, uint32_t size) {
  gWiFiFrameSenderTask.addFrame(frame_buffer, size);
}

void WiFiFrameSenderTask::run(void* data) {
  while (true) {
    std::unique_lock<std::mutex> lock(mMutex);
    mCondVar.wait(lock, [this]() { return !mFrames.empty(); });
    auto frame = mFrames.front();
    mFrames.pop();
    lock.unlock();

    // Handle all frames in queue
    while (true) {
      auto err = esp_wifi_80211_tx(WIFI_IF_AP, frame.data(), frame.size(), false);
      if (err == ESP_ERR_NO_MEM) {
        // TX buffer is full. Need to sleep and try again
        delay(10);
        continue;
      } else if (err == ESP_OK) {
        // Frame successfully sent. Proceed with the next one
        lock.lock();
        if (mFrames.empty()) {
          // No more frame. Fall asleep on condition_variable
          lock.unlock();
          break;
        }
        frame = mFrames.front();
        mFrames.pop();
        lock.unlock();
        continue;
      } else {
        ESP_ERROR_CHECK(err);
        return;
      }
    }
  }
}

void WiFiFrameSenderTask::addFrame(const uint8_t* frame_buffer, uint32_t size) {
  {
    std::lock_guard<std::mutex> lock(mMutex);
    mFrames.push(std::vector<uint8_t>(frame_buffer, frame_buffer + size));
  }
  mCondVar.notify_one();
}
