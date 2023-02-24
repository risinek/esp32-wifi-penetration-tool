#include "bluetooth.h"

#include "BLE2902.h"
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "esp_system.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <cctype>
#include <string>

#include "esp_log.h"

namespace {
// See the following for generating UUIDs: https://www.uuidgenerator.net/
const char* kServiceUUID = "08c3308c-2bf5-4016-b032-14a7d0a3c010";
const char* kCharacteristicUUID = "9c456f81-a18d-4182-97f9-e140fc1209a7";
const char* kDeviceBLEName = "ManagementAP";
const char* kResetCommand = "reset";
const char* LOG_TAG = "BLE";

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    ESP_LOGD(LOG_TAG, "BLE device connected");
  };

  void onDisconnect(BLEServer* pServer) {
    ESP_LOGD(LOG_TAG, "BLE device disconnected");
  }
};
static MyServerCallbacks gMyServerCallbacks;

class MyCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) override {
    std::string value{pCharacteristic->getValue()};
    if (value.empty()) {
      return;
    }

    ESP_LOGD(LOG_TAG, "BLE characteristic write: '%s'", value.c_str());
    std::string value_lowstr;
    for (auto ch : value) {
      value_lowstr += std::tolower(ch);
    }
    if (value_lowstr == std::string{kResetCommand}) {
      ESP_LOGD(LOG_TAG, "Reset command received. Rebooting...");
      esp_restart();
    }
  }
};
static MyCharacteristicCallbacks kMyCharacteristicCallbacks;

}  // namespace

void bluetooth_init() {
  ESP_LOGD(LOG_TAG, "Initialization of BLE...");

  BLEDevice::init(kDeviceBLEName);
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(&gMyServerCallbacks);

  // TODO(alambin): rework it, because all we need is just to react on
  // onConnected() event. Try not to create any service or Characteristic at all
  BLEService* pService = pServer->createService(kServiceUUID);
  BLECharacteristic* pCharacteristic = pService->createCharacteristic(
      kCharacteristicUUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(&kMyCharacteristicCallbacks);

  pCharacteristic->setValue("Write here 'reset' to reset 'ManagementAP'");
  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(kServiceUUID);
  pAdvertising->setScanResponse(true);
  // pAdvertising->setMinPreferred(
  //     0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  ESP_LOGD(LOG_TAG, "Initialization of BLE is done");
}
