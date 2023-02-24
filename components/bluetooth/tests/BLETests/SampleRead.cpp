/**
 * Create a BLE Server that when it receive a read request from a BLE client for the value
 * of a characteristic will have the BLECharacteristicCallback invoked in its onRead() method.
 * This can be then used to set the value of the corresponding characteristic which will then
 * be returned back to the client.
 */
#include "BLEUtils.h"
#include "BLEServer.h"
#include <esp_log.h>
#include <string>
#include <sys/time.h>
#include <sstream>
#include "BLEDevice.h"

#include "sdkconfig.h"

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

static uint8_t  SERVICE_UUID_BIN[] = {0x4f, 0xaf, 0xc2, 0x01, 0x1f, 0xb5, 0x45, 0x9e, 0x8f, 0xcc, 0xc5, 0xc9, 0xc3, 0x31, 0x91, 0x4b};

class MyCallbackHandler: public BLECharacteristicCallbacks {
	void onRead(BLECharacteristic *pCharacteristic) {
		struct timeval tv;
		gettimeofday(&tv, nullptr);
		std::ostringstream os;
		os << "Time: " << tv.tv_sec;
		pCharacteristic->setValue(os.str());
	}
};

static void run() {
	BLEDevice::init("MYDEVICE");
	BLEServer *pServer = BLEDevice::createServer();

	BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID_BIN, 16, true));

	BLECharacteristic *pCharacteristic = pService->createCharacteristic(
		BLEUUID(CHARACTERISTIC_UUID),
		BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
	);

	pCharacteristic->setCallbacks(new MyCallbackHandler());

	pCharacteristic->setValue("Hello World");

	pService->start();

	BLEAdvertising *pAdvertising = pServer->getAdvertising();
	pAdvertising->start();
}

void SampleRead(void)
{
	run();
} // app_main
