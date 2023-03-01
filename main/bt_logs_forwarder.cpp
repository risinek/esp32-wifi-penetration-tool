#include "bt_logs_forwarder.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <vector>

#include "bluetooth_serial.h"
#include "esp_log.h"

namespace {
const char* LOG_TAG{"BT_FWD"};
vprintf_like_t gSerialVprintf{nullptr};

std::vector<char> vformat(const char* format, va_list args) {
  va_list copy;
  va_copy(copy, args);
  int len = std::vsnprintf(nullptr, 0, format, copy);
  va_end(copy);

  if (len >= 0) {
    std::vector<char> data(std::size_t(len) + 2, '\0');
    std::vsnprintf(&data[0], data.size(), format, args);
    data[len] = '\r';
    data[len + 1] = '\0';
    return data;
  }

  return {};
}

int btVprintf(const char* format, va_list args) {
  auto message = vformat(format, args);
  if (!message.empty()) {
    BluetoothSerial::instance().send(std::move(message));
  }
  return gSerialVprintf(format, args);
}

}  // namespace

void BtLogsForwarder::startForwarding() {
  if (gSerialVprintf != nullptr) {
    ESP_LOGE(LOG_TAG, "BT logs forwarding is already enabled");
    return;
  }
  gSerialVprintf = esp_log_set_vprintf(btVprintf);
}

void BtLogsForwarder::stopForwarding() {
  if (gSerialVprintf == nullptr) {
    ESP_LOGE(LOG_TAG, "BT logs forwarding is already disabled");
    return;
  }
  esp_log_set_vprintf(gSerialVprintf);
  gSerialVprintf = nullptr;
}
