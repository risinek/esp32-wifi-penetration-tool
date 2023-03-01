#ifndef BT_LOGS_FORWARDER_H_
#define BT_LOGS_FORWARDER_H_

// This class enables all logs, printed by ESP32, to be also copied to Bluetooth
class BtLogsForwarder {
 public:
  BtLogsForwarder() = default;
  void startForwarding();
  void stopForwarding();
};

#endif  // BT_LOGS_FORWARDER_H_
