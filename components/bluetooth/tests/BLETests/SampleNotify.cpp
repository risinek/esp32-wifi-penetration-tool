/**
 * Create a BLE server that, once we receive a connection, will send periodic notifications.
 * The service advertises itself as: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
 * And has a characteristic of: beb5483e-36e1-4688-b7f5-ea07361b26a8
 *
 * The design of creating the BLE server is:
 * 1. Create a BLE Server
 * 2. Create a BLE Service
 * 3. Create a BLE Characteristic on the Service
 * 4. Create a BLE Descriptor on the characteristic
 * 5. Start the service.
 * 6. Start advertising.
 *
 * A connect hander associated with the server starts a background task that performs notification
 * every couple of seconds.
 *
 * @author: Neil Kolban, July 2017
 *
 */
#include "sdkconfig.h"

#include <esp_log.h>
#include <string>
#include <sstream>
#include <sys/time.h>
#include "BLEDevice.h"

#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"
#include "Task.h"


static char LOG_TAG[] = "SampleNotify";

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic *pCharacteristic;

class MyNotifyTask: public Task {
	void run(void *data) {
		uint8_t value = 0;
		while(1) {
			delay(2000);
			ESP_LOGD(LOG_TAG, "*** NOTIFY: %d ***", value);
			pCharacteristic->setValue(&value, 1);
			pCharacteristic->notify();
			//pCharacteristic->indicate();
			value++;
		} // While 1
	} // run
}; // MyNotifyTask

MyNotifyTask *pMyNotifyTask;

class MyServerCallbacks: public BLEServerCallbacks {
	void onConnect(BLEServer* pServer) {
		pMyNotifyTask->start();
	};

	void onDisconnect(BLEServer* pServer) {
		pMyNotifyTask->stop();
	}
};

static void run() {
	pMyNotifyTask = new MyNotifyTask();
	pMyNotifyTask->setStackSize(8000);

	// Create the BLE Device
	BLEDevice::init("MYDEVICE");

	// Create the BLE Server
	BLEServer *pServer = BLEDevice::createServer();
	pServer->setCallbacks(new MyServerCallbacks());

	// Create the BLE Service
	BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID));

	// Create a BLE Characteristic
	pCharacteristic = pService->createCharacteristic(
		BLEUUID(CHARACTERISTIC_UUID),
		BLECharacteristic::PROPERTY_READ   |
		BLECharacteristic::PROPERTY_WRITE  |
		BLECharacteristic::PROPERTY_NOTIFY |
		BLECharacteristic::PROPERTY_INDICATE
	);

	// https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
	// Create a BLE Descriptor
	pCharacteristic->addDescriptor(new BLE2902());

	// Start the service
	pService->start();

	// Start advertising
	pServer->getAdvertising()->start();
}

void SampleNotify(void)
{
	run();
} // app_main
