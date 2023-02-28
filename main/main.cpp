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
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "serial_command_dispatcher.h"
#include "webserver.h"
#include "wifi_controller.h"

namespace {
const char* TAG = "main";
SerialCommandDispatcher serialCommandDispatcher;
}  // namespace

#ifdef __cplusplus
extern "C" {
#endif

void app_main(void) {
  ESP_LOGD(TAG, "app_main started");
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  wifictl_mgmt_ap_start();
  attack_init();
  webserver_run();

  BluetoothSerial::instance().init(
      [](std::string receivedData) { serialCommandDispatcher.onNewSymbols(std::move(receivedData)); });

  // Set serial commands handlers
  serialCommandDispatcher.setCommandHandler(SerialCommandDispatcher::CommandType::kReset, []() {
    ESP_LOGD(TAG, "RESETTING ESP32");
    esp_restart();
  });
}

#ifdef __cplusplus
}
#endif
