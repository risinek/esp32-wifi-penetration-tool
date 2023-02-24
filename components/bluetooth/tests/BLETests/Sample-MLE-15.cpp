#include "BLEUtils.h"
#include "BLEScan.h"
#include <esp_log.h>
#include <string>

#include "BLEDevice.h"
#include "BLEAdvertisedDevice.h"
#include "BLEClient.h"
#include "sdkconfig.h"
#include "Task.h"

static const char LOG_TAG[] = "Sample-MLE-15";

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/



static BLEUUID serviceUUID((uint16_t)0x1802);
static BLEUUID    charUUID((uint16_t)0x2a06);
static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) {

}

class MyClient: public Task {
	void run(void *data) {
		BLEAddress* pAddress = (BLEAddress *)data;
		BLEClient*  pClient = BLEDevice::createClient();


		pClient->connect(*pAddress);
		pClient->getServices();

		BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
		if (pRemoteService == nullptr) {
			ESP_LOGD(LOG_TAG, "Failed to find our service UUID: %s", serviceUUID.toString().c_str());
			return;
		}

		BLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
		if (pRemoteCharacteristic == nullptr) {
			ESP_LOGD(LOG_TAG, "Failed to find our characteristic UUID: %s", charUUID.toString().c_str());
			return;
		}
		pRemoteCharacteristic->readValue();

		pRemoteCharacteristic->writeValue("123");
		pRemoteCharacteristic->registerForNotify(notifyCallback);
		pClient->disconnect();

		ESP_LOGD(LOG_TAG, "%s", pClient->toString().c_str());
		ESP_LOGD(LOG_TAG, "-- End of task");
	}
};



static void run() {
	ESP_LOGD(LOG_TAG, "MLE-15 sample starting");
	BLEDevice::init("");
	BLEClient*  pClient = BLEDevice::createClient();


	pClient->connect(BLEAddress("ff:ff:45:19:14:80"));
	BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
	if (pRemoteService == nullptr) {
		ESP_LOGD(LOG_TAG, "Failed to find our service UUID: %s", serviceUUID.toString().c_str());
		return;
	}

	BLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
	if (pRemoteCharacteristic == nullptr) {
		ESP_LOGD(LOG_TAG, "Failed to find our characteristic UUID: %s", charUUID.toString().c_str());
		return;
	}

	pRemoteCharacteristic->writeValue((uint8_t)1);

	//BLEClient *pClient = BLE::createClient();
	//pClient->setClientCallbacks(new MyClientCallbacks());
	//pClient->connect(BLEAddress("00:00:00:00:00:00"));

}

void Sample_MLE_15(void)
{
	run();
} // app_main
