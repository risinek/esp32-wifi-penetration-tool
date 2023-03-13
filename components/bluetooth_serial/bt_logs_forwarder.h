#ifndef BT_LOGS_FORWARDER_H_
#define BT_LOGS_FORWARDER_H_

#include <stdarg.h>

// This class enables all logs, printed by ESP32, to be copied to Bluetooth
class BtLogsForwarder {
 public:
  BtLogsForwarder();
  void startForwarding();
  void stopForwarding();

  void printToSerial(const char* format, va_list& args);

 private:
  using ESPLogFn = int (*)(const char*, va_list);

  // This function have to be static so that we can use it in ESP Log API
  static int btVprintf(const char* format, va_list args);

  static ESPLogFn mSerialVprintf;
  bool mIsForwarding{false};
};

#endif  // BT_LOGS_FORWARDER_H_
