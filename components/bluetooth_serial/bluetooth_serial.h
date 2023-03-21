#ifndef BT_SERIAL_H_
#define BT_SERIAL_H_

#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "bt_logs_forwarder.h"
#include "esp_spp_api.h"
#include "ring_buffer.h"

// Currently no PIN is required to pair ESP32. All you need is to confirm on your PC that displayed code is the same as
// printed by ESP32 in logs. Authentyification by PIN is enabled in case SSP is disabled in menu config. In that case
// default PIN is "1234"
class BluetoothSerial {
 public:
  using OnBtDataReceviedCallbackType = std::function<void(std::string receivedData)>;
  static BluetoothSerial& instance();
  bool init(BtLogsForwarder* btLogsForwarder, OnBtDataReceviedCallbackType dataReceviedCallback,
            uint32_t maxTxBufSize = 16 * 1024);

  bool send(std::string message);
  bool send(std::vector<char> message);

 private:
  BluetoothSerial() = default;
  void onBtDataRecevied(std::string receivedData);
  friend void esp_spp_callback(esp_spp_cb_event_t event, esp_spp_cb_param_t* param);
  void transmitChunkOfData();
  // NOTE!
  // 1. Requires external lock
  // 2. Moves extracted data to mCurrentTransmittedChunk
  void extractDataChunkToTransmit();
  // This function will slightly reduce amount oflogs, printed inside this class
  // It is useful when terminal connection is established via Bluetooth to avoid a lot of echoed messages
  void limitBTLogs(bool isLimited);
  // This function is used in critical parts, where logs should not be printed when terminal is connected to agoid
  // logs floor. Internally it checks are logs limited or not and if yes, it sends them only to Serial
  void limitedPrint(uint8_t logLevel, const char* tag, const char* format, ...);

  BtLogsForwarder* mBtLogsForwarder{nullptr};
  OnBtDataReceviedCallbackType mDataReceviedCallback{nullptr};

  // This data can be accessed from multiple threads, so it is protected by mutex
  std::mutex mMutex;
  uint32_t mTerminalConnectionHandle{0};
  RingBuffer mTxData;
  uint32_t mMaxTxBufSize{16 * 1024};
  std::vector<char> mCurrentTransmittedChunk;
  bool mIsTransmissionRequestInProgress{false};
  bool mIsLimitedLogs{false};
};

#endif  // BT_SERIAL_HPP_