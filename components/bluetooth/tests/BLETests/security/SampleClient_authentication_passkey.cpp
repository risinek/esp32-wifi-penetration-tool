/*
 * SampleClient_authentication_passkey.cpp
 *
 *  Created on: Dec 23, 2017
 *      Author: chegewara
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

static const char* LOG_TAG = "SampleClient";

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
// The remote service we wish to connect to.
static BLEUUID serviceUUID("91bad492-b950-4226-aa2b-4ede9fa42f59");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("0d563a58-196a-48ce-ace2-dfec78acc814");

class MySecurity : public BLESecurityCallbacks {

	uint32_t onPassKeyRequest(){
		return 123456;
	}
	void onPassKeyNotify(uint32_t pass_key){
        ESP_LOGE(LOG_TAG, "The passkey Notify number:%d", pass_key);
	}
	bool onConfirmPIN(uint32_t pass_key){
        ESP_LOGI(LOG_TAG, "The passkey YES/NO number:%d", pass_key);
	    vTaskDelay(5000);
		return false;
	}
	bool onSecurityRequest(){
		ESP_LOGI(LOG_TAG, "Security Request");
		return true;
	}
	void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl){
		if(auth_cmpl.success){
			ESP_LOGI(LOG_TAG, "remote BD_ADDR:");
			esp_log_buffer_hex(LOG_TAG, auth_cmpl.bd_addr, sizeof(auth_cmpl.bd_addr));
			ESP_LOGI(LOG_TAG, "address type = %d", auth_cmpl.addr_type);
		}
        ESP_LOGI(LOG_TAG, "pair status = %s", auth_cmpl.success ? "success" : "fail");
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
		BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
		BLEDevice::setSecurityCallbacks(new MySecurity());

		BLESecurity *pSecurity = new BLESecurity();
		pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_ONLY);
		pSecurity->setCapability(ESP_IO_CAP_OUT);
		pSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
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

		if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
			advertisedDevice.getScan()->stop();

			ESP_LOGD(LOG_TAG, "Found our device!  address: %s", advertisedDevice.getAddress().toString().c_str());
			MyClient* pMyClient = new MyClient();
			pMyClient->setStackSize(18000);
			pMyClient->start(new BLEAddress(*advertisedDevice.getAddress().getNative()));
		} // Found our server
	} // onResult
}; // MyAdvertisedDeviceCallbacks


/**
 * Perform the work of a sample BLE client.
 */
void SampleSecureClient(void) {
	ESP_LOGD(LOG_TAG, "Scanning sample starting");
	BLEDevice::init("");
	BLEScan *pBLEScan = BLEDevice::getScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
	pBLEScan->setActiveScan(true);
	pBLEScan->start(5);
} // SampleClient



