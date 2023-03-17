#include "bt_logs_forwarder.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <vector>

#include "bluetooth_serial.h"
#include "esp_log.h"

namespace {
const char* LOG_TAG{"BT_FWD"};

std::vector<char> vformat(const char* format, va_list args) {
  va_list copy;
  va_copy(copy, args);
  int len = std::vsnprintf(nullptr, 0, format, copy);
  va_end(copy);

  if (len >= 0) {
    std::vector<char> data(std::size_t(len) + 1);
    std::vsnprintf(&data[0], data.size(), format, args);
    data[len] = '\0';
    return data;
  }

  return {};
}
}  // namespace

BtLogsForwarder::ESPLogFn BtLogsForwarder::mSerialVprintf{nullptr};

BtLogsForwarder::BtLogsForwarder() {
  mSerialVprintf = esp_log_set_vprintf(nullptr);
  esp_log_set_vprintf(mSerialVprintf);
}

void BtLogsForwarder::startForwarding() {
  if (mIsForwarding) {
    ESP_LOGE(LOG_TAG, "BT logs forwarding is already enabled");
    return;
  }
  mIsForwarding = true;
  esp_log_set_vprintf(btVprintf);
}

void BtLogsForwarder::stopForwarding() {
  if (!mIsForwarding) {
    ESP_LOGE(LOG_TAG, "BT logs forwarding is already disabled");
    return;
  }
  mIsForwarding = false;
  esp_log_set_vprintf(mSerialVprintf);
}

void BtLogsForwarder::printToSerial(const char* format, va_list& args) {
  if (mSerialVprintf != nullptr) {
    mSerialVprintf(format, args);
  }
}

int BtLogsForwarder::btVprintf(const char* format, va_list args) {
  auto message = vformat(format, args);
  if (!message.empty()) {
    BluetoothSerial::instance().send(std::move(message));
  }
  return mSerialVprintf(format, args);
}
