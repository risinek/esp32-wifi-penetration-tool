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
#include "Timer.h"
#include "WiFiFramesSender.h"
#include "attack.h"
#include "attack_dos.h"
#include "bluetooth_serial.h"
#include "bt_logs_forwarder.h"
#include "device_id.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "led.h"
#include "ota.h"
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
Ota gOta;
constexpr uint32_t kInactivityTimeoutS{10 * 60};
Timer gInactivityTimer;
Led gLed;
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

void setLogLevel(const std::string& levelStr) {
  if (levelStr == "n")
    esp_log_level_set("*", ESP_LOG_NONE);
  else if (levelStr == "e")
    esp_log_level_set("*", ESP_LOG_ERROR);
  else if (levelStr == "w")
    esp_log_level_set("*", ESP_LOG_WARN);
  else if (levelStr == "i")
    esp_log_level_set("*", ESP_LOG_INFO);
  else if (levelStr == "d")
    esp_log_level_set("*", ESP_LOG_DEBUG);
  else if (levelStr == "v")
    esp_log_level_set("*", ESP_LOG_VERBOSE);
  else
    ESP_LOGE(LOG_TAG, "Unsupported log level '%s'", levelStr.c_str());
}

void setSerialCommandsHandlers() {
  gSerialCommandDispatcher.setCommandHandler(SerialCommandDispatcher::CommandType::kReset,
                                             [](const std::string& param) {
                                               ESP_LOGI(LOG_TAG, "RESETTING ESP32");
                                               esp_restart();
                                             });
  gSerialCommandDispatcher.setCommandHandler(SerialCommandDispatcher::CommandType::kStartLogs,
                                             [](const std::string& param) {
                                               ESP_LOGI(LOG_TAG, "Start redirecting logs to BT");
                                               gBtLogsForwarder.startForwarding();
                                             });
  gSerialCommandDispatcher.setCommandHandler(SerialCommandDispatcher::CommandType::kStopLogs,
                                             [](const std::string& param) {
                                               ESP_LOGI(LOG_TAG, "Stop redirecting logs to BT");
                                               gBtLogsForwarder.stopForwarding();
                                             });
  gSerialCommandDispatcher.setCommandHandler(SerialCommandDispatcher::CommandType::kLimitLogs,
                                             [](const std::string& param) {
                                               bool shouldLimit{false};
                                               if (param == "1") {
                                                 ESP_LOGI(LOG_TAG, "Limitting logs ");
                                                 shouldLimit = true;
                                               } else {
                                                 ESP_LOGI(LOG_TAG, "Unlimitting logs ");
                                                 shouldLimit = false;
                                               }
                                               attack_limit_logs(shouldLimit);
                                             });
  gSerialCommandDispatcher.setCommandHandler(SerialCommandDispatcher::CommandType::kSetLogLevel,
                                             [](const std::string& param) { setLogLevel(param); });
  gSerialCommandDispatcher.setCommandHandler(SerialCommandDispatcher::CommandType::kLedOn,
                                             [&gLed](const std::string& param) { gLed.on(); });
  gSerialCommandDispatcher.setCommandHandler(SerialCommandDispatcher::CommandType::kLedOff,
                                             [&gLed](const std::string& param) { gLed.off(); });
  gSerialCommandDispatcher.setCommandHandler(SerialCommandDispatcher::CommandType::kLedBlink,
                                             [&gLed](const std::string& param) { gLed.startBlinking(); });
  gSerialCommandDispatcher.setCommandHandler(SerialCommandDispatcher::CommandType::kHelp, [](const std::string& param) {
    BluetoothSerial::instance().send(gSerialCommandDispatcher.getSupportedCommands());
  });
  gSerialCommandDispatcher.setCommandHandler(
      SerialCommandDispatcher::CommandType::kGetAttackStatus,
      [](const std::string& param) { BluetoothSerial::instance().send(attack_get_status_json()); });
  gSerialCommandDispatcher.setCommandHandler(
      SerialCommandDispatcher::CommandType::kGetDosAttackStatus,
      [](const std::string& param) { BluetoothSerial::instance().send(attack_dos_get_status_json()); });
  gSerialCommandDispatcher.setCommandHandler(SerialCommandDispatcher::CommandType::kStopAttack,
                                             [](const std::string& param) { stopAttack(); });
  gSerialCommandDispatcher.setCommandHandler(
      SerialCommandDispatcher::CommandType::kBtTerminalConnected, [&gInactivityTimer](const std::string& param) {
        if (param == "1") {
          gInactivityTimer.stop();  // Stop inactivity timer in case of any BT activities

          // Hide it if you don't wan't people to know what is this Bluetooth device about
          std::string greeting{
              "\n\r\n\r\n\r\n\r\n\rWelcome to ESP32 WiFi penetration tool\n\r"
              "Supported commands: "};
          BluetoothSerial::instance().send(greeting + gSerialCommandDispatcher.getSupportedCommands());
        }
      });
}

void app_main(void) {
  // This line is required in ESP IDF v5.0.1, because regular way of increasing log level in particular files
  // (by defining LOG_LOCAL_LEVEL) doesn't work anymore. It is considered as bug, but not fixed yet
  esp_log_level_set("*", ESP_LOG_VERBOSE);

  BluetoothSerial::instance().init(&gBtLogsForwarder, [](std::string receivedData) {
    gSerialCommandDispatcher.onNewSymbols(std::move(receivedData));
  });
  gBtLogsForwarder.startForwarding();

  ESP_LOGD(LOG_TAG, "app_main() started. Device ID='%d'", CONFIG_DEVICE_ID);
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  wifictl_mgmt_ap_start();
  init_wifi_frame_sender();
  attack_init();
  setSerialCommandsHandlers();

  webserver_run();
  setWebserverOtaRequestHandler([](const std::string& url) { gOta.connectToServer(url); });

  attack_limit_logs(true);

  gInactivityTimer.start(kInactivityTimeoutS, [&gInactivityTimer](const bool& isStopRequested) {
    if (runDefaultAttack()) {
      // This is one shot timer, so clean up its resourses when it is not needed.
      // NOTE! Calling of stop() will lead to destroy of task (current lambda)
      gInactivityTimer.stop();
    } else {
      // Need to retry again later
      gInactivityTimer.reset();
    }
  });
  setHTTPActivityHandler([&gInactivityTimer]() {
    gInactivityTimer.stop();  // Stop inactivity timer in case any request is arrived from HTTP
  });

  // Configure LED to blink in case no active attack
  gLed.init();
  gLed.startBlinking();
  setAttackProgressHandler([&gLed](bool isStarted) {
    if (isStarted) {
      gLed.off();
    } else {
      gLed.startBlinking();
    }
  });

#ifdef CONFIG_ENABLE_UNIT_TESTS
  // testBT();
  test_ring_buffer();
#endif
}

#ifdef __cplusplus
}
#endif
