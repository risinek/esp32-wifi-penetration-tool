#include "BLEUtils.h"
#include "BLEServer.h"
#include <esp_log.h>
#include <string>
#include <stdio.h>

#include "BLEDevice.h"
#include "sdkconfig.h"

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID         "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID2 "54059634-9448-404f-9af4-7d14556f3ad8"
#define CHARACTERISTIC_UUID3 "78f8a814-7b20-40ca-b970-0aba448c53b1"
#define CHARACTERISTIC_UUID4 "03a55273-c1ef-4eab-a6c0-7ff11509122f"
#define CHARACTERISTIC_UUID5 "0d19566d-2144-4443-9779-19d42e283439"

static void run() {
	BLEDevice::init("MYDEVICE");

	BLEServer*  pServer  = BLEDevice::createServer();

	BLEService* pService = pServer->createService(BLEUUID(SERVICE_UUID));

	BLECharacteristic* pCharacteristic = pService->createCharacteristic(
		BLEUUID(CHARACTERISTIC_UUID),
		BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
	);

	pCharacteristic = pService->createCharacteristic(
		BLEUUID(CHARACTERISTIC_UUID2),
		BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
	);

	pCharacteristic = pService->createCharacteristic(
		BLEUUID(CHARACTERISTIC_UUID3),
		BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
	);

	pCharacteristic = pService->createCharacteristic(
		BLEUUID(CHARACTERISTIC_UUID4),
		BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
	);

	pCharacteristic = pService->createCharacteristic(
		BLEUUID(CHARACTERISTIC_UUID5),
		BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
	);


	pCharacteristic->setValue("Hello World says Neil");

	pService->start();

	BLEAdvertising* pAdvertising = pServer->getAdvertising();
	pAdvertising->start();
}

void Sample1(void)
{
	//esp_log_level_set("*", ESP_LOG_ERROR);
	run();
} // app_main
