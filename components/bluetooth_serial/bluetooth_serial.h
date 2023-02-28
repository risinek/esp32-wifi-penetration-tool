#ifndef BT_SERIAL_H_
#define BT_SERIAL_H_

// #define PRINT_RX_SPEED
// #define PRINT_TX_SPEED

#include <functional>
#include <shared_mutex>
#include <string>
#include <vector>

#include "esp_spp_api.h"

#if defined(PRINT_RX_SPEED) || defined(PRINT_TX_SPEED)
#include "sys/time.h"
#endif

class BluetoothSerial {
 public:
  using OnBtDataReceviedCallbackType = std::function<void(std::string receivedData)>;
  static BluetoothSerial& instance();
  bool init(OnBtDataReceviedCallbackType dataReceviedCallback);
  bool send(std::string message);

 private:
  BluetoothSerial() = default;
  void onBtDataRecevied(std::string receivedData);
  friend void esp_spp_callback(esp_spp_cb_event_t event, esp_spp_cb_param_t* param);
  void transmitChunkOfData();
  // NOTE!
  // 1. Requires external lock
  // 2. Moves extracted data to mCurrentTransmittedChunk
  void extractDataChunkToTransmit();
  // NOTE!
  // 1. Requires external lock
  // 2. mTotalTxDataLength must be set to current size of mTxData
  void freeOldTXData();

  OnBtDataReceviedCallbackType mDataReceviedCallback{nullptr};

  // This data can be accessed from multiple threads, so it is protected by mutex
  std::shared_timed_mutex mMutex;
  uint32_t mTerminalConnectionHandle{0};
  std::vector<std::string> mTxData;
  uint32_t mTotalTxDataLength{0};
  std::string mCurrentTransmittedChunk;
  bool mIsTransmissionEvenSequenceInProgress{false};

#ifdef PRINT_RX_SPEED
  void printRXSpeed();
  timeval mReceiveTimeNew;
  timeval mReceiveTimeOld;
  uint32_t mRXDataSizeTransSend{0};
#endif
#ifdef PRINT_RX_SPEED
  void printTXSpeed();
  timeval mTransmitTimeNew;
  timeval mTransmitTimeOld;
  uint32_t mTXDataSizeTransSend{0};
#endif
};

#endif  // BT_SERIAL_HPP_