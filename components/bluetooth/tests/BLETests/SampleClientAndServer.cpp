/**
 * Create a sample BLE client that connects to a BLE server and then retrieves the current
 * characteristic value.  It will then periodically update the value of the characteristic on the
 * remote server with the current time since boot.
 */
#include <esp_log.h>
#include <string>
#include <sstream>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
extern "C" {                       // See issue 1069:  https://github.com/espressif/esp-idf/issues/1069
	#include <esp_vfs_dev.h>
}
#include "BLEDevice.h"
#include "Task.h"

#include "sdkconfig.h"

static const char* LOG_TAG = "SampleClientAndServer";

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
// The remote service we wish to connect to.
static BLEUUID serviceUUID("91bad492-b950-4226-aa2b-4ede9fa42f59");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("0d563a58-196a-48ce-ace2-dfec78acc814");

class MyServer: public Task {
	void run(void *data) {
		ESP_LOGD(LOG_TAG, "Starting to be a BLE Server");

		BLEDevice::init("MYDEVICE");
		BLEServer* pServer = BLEDevice::createServer();

		BLEService* pService = pServer->createService(serviceUUID);

		BLECharacteristic* pCharacteristic = pService->createCharacteristic(
			charUUID,
			BLECharacteristic::PROPERTY_BROADCAST | BLECharacteristic::PROPERTY_READ  |
			BLECharacteristic::PROPERTY_NOTIFY    | BLECharacteristic::PROPERTY_WRITE |
			BLECharacteristic::PROPERTY_INDICATE
		);

		pCharacteristic->setValue("Hello World!");

		pService->start();

		BLEAdvertising* pAdvertising = pServer->getAdvertising();
		pAdvertising->addServiceUUID(pService->getUUID());
		pAdvertising->start();

		ESP_LOGD(LOG_TAG, "Advertising started!");
		delay(1000000);
	}
};

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

		// Obtain a reference to the service we are after in the remote BLE server.
		BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
		if (pRemoteService == nullptr) {
			ESP_LOGD(LOG_TAG, "Failed to find our service UUID: %s", serviceUUID.toString().c_str());
			return;
		}


		// Obtain a reference to the characteristic in the service of the remote BLE server.
		BLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
		if (pRemoteCharacteristic == nullptr) {
			ESP_LOGD(LOG_TAG, "Failed to find our characteristic UUID: %s", charUUID.toString().c_str());
			return;
		}

		// Read the value of the characteristic.
		std::string value = pRemoteCharacteristic->readValue();
		ESP_LOGD(LOG_TAG, "The characteristic value was: %s", value.c_str());

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

		pClient->disconnect();

		ESP_LOGD(LOG_TAG, "%s", pClient->toString().c_str());
		ESP_LOGD(LOG_TAG, "-- End of task");
	} // run
}; // MyClient


/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
	/**
	 * Called for each advertising BLE server.
	 */
	void onResult(BLEAdvertisedDevice advertisedDevice) {
		ESP_LOGD(LOG_TAG, "Advertised Device: %s", advertisedDevice.toString().c_str());

		if (!advertisedDevice.haveServiceUUID()) {
			ESP_LOGD(LOG_TAG, "Found device is not advertising a service ... ignoring");
			return;
		}
		if (advertisedDevice.getServiceUUID().equals(serviceUUID)) {
			advertisedDevice.getScan()->stop();

			ESP_LOGD(LOG_TAG, "Found our device!  address: %s", advertisedDevice.getAddress().toString().c_str());
			MyClient* pMyClient = new MyClient();
			pMyClient->setStackSize(18000);
			pMyClient->start(new BLEAddress(*advertisedDevice.getAddress().getNative()));
			return;
		} // Found our server
		ESP_LOGD(LOG_TAG, "Found device advertised %s which is not %s ... ignoring",
			advertisedDevice.getServiceUUID().toString().c_str(),
			serviceUUID.toString().c_str());
	} // onResult
}; // MyAdvertisedDeviceCallbacks


/**
 * Perform the work of a sample BLE client.
 */
void SampleClientAndServer(void) {
	ESP_LOGD(LOG_TAG, "SampleClientAndServer starting");
	esp_vfs_dev_uart_register();
	int fd = open("/dev/uart/0", O_RDONLY);
	if (fd == -1) {
		ESP_LOGE(LOG_TAG, "Failed to open file %s", strerror(errno));
		return;
	}

	ESP_LOGD(LOG_TAG, "Enter:");
	ESP_LOGD(LOG_TAG, "C - Client");
	ESP_LOGD(LOG_TAG, "S - Server");
	bool isServer;
	while(1) {
		uint8_t val;
		ssize_t rc;
		// Read a character from the UART.  Since the UART is non-blocking, we loop until we have
		// a character available.
		do {
			rc = read(fd, &val, 1);
			if (rc == -1) {
				//ESP_LOGE(LOG_TAG, "Failed to read file %s", strerror(errno));
				FreeRTOS::sleep(100);
				//return;
			}
		} while(rc == -1);

		// See if the character is an indication of being a server or a client.

		if (val == 'c' || val == 'C') {
			isServer = false;
			break;
		}
		if (val == 's' || val == 'S') {
			isServer = true;
			break;
		}
	}
	close(fd);  // Close the UART file as we don't need it any more.
	ESP_LOGD(LOG_TAG, "Chosen: %s", isServer?"Server":"Client");

	if (isServer) {
		MyServer* pMyServer = new MyServer();
		pMyServer->setStackSize(20000);
		pMyServer->start();
	} else {
		BLEDevice::init("");
		BLEScan *pBLEScan = BLEDevice::getScan();
		pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
		pBLEScan->setActiveScan(true);
		pBLEScan->start(15);
	}

} // SampleClient
