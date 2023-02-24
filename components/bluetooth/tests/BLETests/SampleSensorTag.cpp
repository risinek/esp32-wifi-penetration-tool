/**
 * Create a sample BLE client that connects to a BLE server and then retrieves the current
 * characteristic value.  It will then periodically update the value of the characteristic on the
 * remote server with the current time since boot.
 */
#include <esp_log.h>
#include <string>
#include <sstream>
#include <sys/time.h>
#include "BLEDevice.h"

#include "BLEAdvertisedDevice.h"
#include "BLEClient.h"
#include "BLEScan.h"
#include "BLEUtils.h"
#include "Task.h"

#include "sdkconfig.h"

static const char* LOG_TAG = "SampleSensorTag";

static BLEUUID luxometerServiceUUID("f000aa70-0451-4000-b000-000000000000");
static BLEUUID    luxometerDataUUID("f000aa71-0451-4000-b000-000000000000");
static BLEUUID  luxometerConfigUUID("f000aa72-0451-4000-b000-000000000000");
static BLEUUID  luxometerPeriodUUID("f000aa73-0451-4000-b000-000000000000");

static BLEUUID irTempServiceUUID("f000aa00-0451-4000-b000-000000000000");
static BLEUUID    irTempDataUUID("f000aa01-0451-4000-b000-000000000000");
static BLEUUID  irTempConfigUUID("f000aa02-0451-4000-b000-000000000000");
static BLEUUID  irTempPeriodUUID("f000aa03-0451-4000-b000-000000000000");

static BLEUUID keyPressServiceUUID("0000ffe0-0000-1000-8000-00805f9b34fb");
static BLEUUID   keyPressStateUUID("0000ffe1-0000-1000-8000-00805f9b34fb");


void notifyCallback(
   BLERemoteCharacteristic* pBLERemoteCharacteristic,
   uint8_t*                 data,
   size_t                   length,
   bool                     isNotify)
{
	ESP_LOGD(LOG_TAG, "Notified!");
}

/**
 * Become a BLE client to a remote BLE server.  We are passed in the address of the BLE server
 * as the input parameter when the task is created.
 */
class MyClient: public Task {
	void run(void* data) {
		BLEAddress* pAddress = (BLEAddress*)data;
		BLEClient*  pClient  = BLEDevice::createClient();

		// Connect to the remove BLE Server.
		pClient->connect(*pAddress);

		BLERemoteService* pIRTempService = pClient->getService(irTempServiceUUID);
		BLERemoteCharacteristic* pIRTempDataCharacteristic = pIRTempService->getCharacteristic(irTempDataUUID);
		BLERemoteCharacteristic* pIRTempConfigCharacteristic = pIRTempService->getCharacteristic(irTempConfigUUID);
		pIRTempConfigCharacteristic->writeValue(1);
		while(1) {
			uint32_t rawValue = pIRTempDataCharacteristic->readUInt32();
			float obj = ((rawValue & 0x0000ffff) >> 2) * 0.03125;
			float amb = ((rawValue & 0xffff0000) >> 18) * 0.03125;
			ESP_LOGD(LOG_TAG, "IT Values: obj: %f, amb: %f", obj, amb);
			FreeRTOS::sleep(1000);
		}

		/*
		BLERemoteService* pKeyPressService = pClient->getService(keyPressServiceUUID);
		BLERemoteCharacteristic* pKeyStateCharacteristic = pKeyPressService->getCharacteristic(keyPressStateUUID);
		BLERemoteDescriptor *p2902Descriptor = pKeyStateCharacteristic->getDescriptor(BLEUUID("00002902-0000-1000-8000-00805f9b34fb"));
		p2902Descriptor->writeValue(1);
		pKeyStateCharacteristic->registerForNotify(notifyCallback);
		*/

		/*
		// Obtain a reference to the service we are after in the remote BLE server.
		BLERemoteService* pRemoteService = pClient->getService(luxometerServiceUUID);
		if (pRemoteService == nullptr) {
			ESP_LOGD(LOG_TAG, "Failed to find our service UUID: %s", luxometerServiceUUID.toString().c_str());
			return;
		}


		// Obtain a reference to the characteristic in the service of the remote BLE server.
		BLERemoteCharacteristic* pLuxometerConfigCharacteristic = pRemoteService->getCharacteristic(luxometerConfigUUID);
		if (pLuxometerConfigCharacteristic == nullptr) {
			ESP_LOGD(LOG_TAG, "Failed to find our characteristic UUID: %s", luxometerDataUUID.toString().c_str());
			return;
		}
		BLERemoteCharacteristic* pLuxometerPeriodCharacteristic = pRemoteService->getCharacteristic(luxometerPeriodUUID);
		if (pLuxometerPeriodCharacteristic == nullptr) {
			ESP_LOGD(LOG_TAG, "Failed to find our characteristic UUID: %s", luxometerDataUUID.toString().c_str());
			return;
		}
		BLERemoteCharacteristic* pLuxometerDataCharacteristic = pRemoteService->getCharacteristic(luxometerDataUUID);
		if (pLuxometerDataCharacteristic == nullptr) {
			ESP_LOGD(LOG_TAG, "Failed to find our characteristic UUID: %s", luxometerDataUUID.toString().c_str());
			return;
		}

		// Read the value of the characteristic.
		uint16_t configValue = pLuxometerConfigCharacteristic->readUInt16();
		ESP_LOGD(LOG_TAG, "The pLuxometerConfigCharacteristic value was: %d", configValue);

		pLuxometerConfigCharacteristic->writeValue(1);

		while(1) {
			uint16_t lightValue = pLuxometerDataCharacteristic->readUInt16();
			ESP_LOGD(LOG_TAG, "Light value: %d", lightValue);
			FreeRTOS::sleep(1000);
		}
		*/

		/*
		while(1) {
			// Set a new value of the characteristic
			ESP_LOGD(LOG_TAG, "Setting the new value");
			std::ostringstream stringStream;
			struct timeval tv;
			gettimeofday(&tv, nullptr);
			stringStream << "Time since boot: " << tv.tv_sec;
			pRemoteCharacteristic->writeValue(stringStream.str());

			FreeRTOS::sleep(1000);
		}
		*/

		while(1) {
			FreeRTOS::sleep(5000);
		}
		pClient->disconnect();

		ESP_LOGD(LOG_TAG, "%s", pClient->toString().c_str());
		ESP_LOGD(LOG_TAG, "-- End of task");
	} // run
}; // MyClient


/**
 * Scan for BLE servers and find the first one that advertises the SensorTag.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
	/**
	 * Called for each advertising BLE server.
	 */
	void onResult(BLEAdvertisedDevice advertisedDevice) {
		ESP_LOGD(LOG_TAG, "Advertised Device: %s", advertisedDevice.toString().c_str());
		if (advertisedDevice.getName() == "CC2650 SensorTag") {
			ESP_LOGD(LOG_TAG, "Found our SensorTag device!  address: %s", advertisedDevice.getAddress().toString().c_str());
			advertisedDevice.getScan()->stop();
			MyClient* pMyClient = new MyClient();
			pMyClient->setStackSize(18000);
			pMyClient->start(new BLEAddress(*advertisedDevice.getAddress().getNative()));
		} // Found our server
	} // onResult
}; // MyAdvertisedDeviceCallbacks


/**
 * Perform the work of a sample BLE client.
 */
void SampleSensorTag(void) {
	ESP_LOGD(LOG_TAG, "SampleSensorTag starting");
	BLEDevice::init("");
	BLEScan *pBLEScan = BLEDevice::getScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
	pBLEScan->setActiveScan(true);
	pBLEScan->start(15);
} // SampleClient
