/**
 * @file main.c
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-03
 * @copyright Copyright (c) 2021
 *
 * @brief Main file used to setup ESP32 into initial state
 *
 * Starts management AP and webserver
 */

#include <stdio.h>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "attack.h"
#include "bluetooth_serial.h"
#include "bt_logs_forwarder.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "serial_command_dispatcher.h"
#include "webserver.h"
#include "wifi_controller.h"

#ifdef CONFIG_ENABLE_UNIT_TESTS
#include "ring_buffer_test.h"
#endif

namespace {
const char* LOG_TAG = "main";
SerialCommandDispatcher gSerialCommandDispatcher;
BtLogsForwarder gBtLogsForwarder;
}  // namespace

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_ENABLE_UNIT_TESTS
void testBT() {
  ESP_LOGD(LOG_TAG, "Testing BT transmition");

  for (int i = 0; i < 10; ++i) {
    std::vector<char> line(10, '0' + i);
    BluetoothSerial::instance().send(std::move(line));
  }

  for (int i = 0; i < 50; ++i) {
    std::vector<char> line(i + 1, '0' + i);
    BluetoothSerial::instance().send(std::move(line));
  }
}
#endif

void app_main(void) {
  BluetoothSerial::instance().init(
      [](std::string receivedData) { gSerialCommandDispatcher.onNewSymbols(std::move(receivedData)); });

  auto startLogsCommandHandler = []() {
    ESP_LOGI(LOG_TAG, "Start redirecting logs to BT");
    gBtLogsForwarder.startForwarding();
    BluetoothSerial::instance().limitBTLogs(true);
  };
  // This function should be called only after Bluetooth initialisation is finished
  startLogsCommandHandler();

  ESP_LOGD(LOG_TAG, "app_main started");
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  wifictl_mgmt_ap_start();
  attack_init();
  webserver_run();

  // Set serial commands handlers
  gSerialCommandDispatcher.setCommandHandler(SerialCommandDispatcher::CommandType::kReset, []() {
    ESP_LOGI(LOG_TAG, "RESETTING ESP32");
    esp_restart();
  });
  gSerialCommandDispatcher.setCommandHandler(SerialCommandDispatcher::CommandType::kStartLogs, startLogsCommandHandler);
  gSerialCommandDispatcher.setCommandHandler(SerialCommandDispatcher::CommandType::kStopLogs, []() {
    ESP_LOGI(LOG_TAG, "Stop redirecting logs to BT");
    gBtLogsForwarder.stopForwarding();
    BluetoothSerial::instance().limitBTLogs(false);
  });
  gSerialCommandDispatcher.setCommandHandler(SerialCommandDispatcher::CommandType::kBtTerminalConnected, []() {
    BluetoothSerial::instance().send(gSerialCommandDispatcher.getSupportedCommands());
  });

#ifdef CONFIG_ENABLE_UNIT_TESTS
  // testBT();
  test_ring_buffer();
#endif
}

#ifdef __cplusplus
}
#endif
