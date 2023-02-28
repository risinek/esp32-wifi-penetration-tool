#include "bluetooth.h"

#include "bluetooth_serial.h"
#include "serial_command_dispatcher.h"

namespace {
SerialCommandDispatcher serialCommandDispatcher;
}  // namespace

void bluetooth_init(void) {
  BluetoothSerial::instance().init(
      [](std::string receivedData) { serialCommandDispatcher.onNewSymbols(std::move(receivedData)); });
}
