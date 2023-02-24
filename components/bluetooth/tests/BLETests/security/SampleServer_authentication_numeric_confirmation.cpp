/*
 * SampleServer_authentication_numeric_confirmation.cpp
 *
 *  Created on: Dec 23, 2017
 *      Author: chegewara
 */


/**
 * Create a new BLE server.
 */
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"
#include <esp_log.h>
#include <string>
#include <Task.h>


#include "sdkconfig.h"

static char LOG_TAG[] = "SampleServer";

class MySecurity : public BLESecurityCallbacks {

	uint32_t onPassKeyRequest(){
        ESP_LOGI(LOG_TAG, "PassKeyRequest");
		return 123456;
	}
	void onPassKeyNotify(uint32_t pass_key){
        ESP_LOGI(LOG_TAG, "The passkey Notify number:%d", pass_key);
	}
	bool onConfirmPIN(uint32_t pass_key){
        ESP_LOGI(LOG_TAG, "The passkey YES/NO number:%d", pass_key);
	    vTaskDelay(5000);
		return true;
	}
	bool onSecurityRequest(){
	    ESP_LOGI(LOG_TAG, "SecurityRequest");
		return true;
	}

	void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl){
		ESP_LOGI(LOG_TAG, "Starting BLE work!");
		if(cmpl.success){
			uint16_t length;
			esp_ble_gap_get_whitelist_size(&length);
			ESP_LOGD(LOG_TAG, "size: %d", length);
		}
	}
};

class bleCharacteristicCallback: public BLECharacteristicCallbacks {
	void onWrite(BLECharacteristic* pCharacteristic) {
		std::string msg = pCharacteristic->getValue();
		ESP_LOGI(LOG_TAG, "BLE received: %s", msg.c_str());
		esp_log_buffer_char(LOG_TAG, msg.c_str(), msg.length());
		esp_log_buffer_hex(LOG_TAG, msg.c_str(), msg.length());
	}

	void onRead(BLECharacteristic* pCharacteristic) {
		std::string msg = pCharacteristic->getValue();
		ESP_LOGI(LOG_TAG, "BLE received: %s, %i", msg.c_str(), msg.length());
		esp_log_buffer_char(LOG_TAG, msg.c_str(), msg.length());
		esp_log_buffer_hex(LOG_TAG, msg.c_str(), msg.length());
	}
};
class MainBLEServer: public Task {
	void run(void *data) {
		ESP_LOGD(LOG_TAG, "Starting BLE work!");
		esp_log_buffer_char(LOG_TAG, LOG_TAG, sizeof(LOG_TAG));
		esp_log_buffer_hex(LOG_TAG, LOG_TAG, sizeof(LOG_TAG));
		BLEDevice::init("ESP32");
		BLEServer* pServer = BLEDevice::createServer();
		BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_NO_MITM);
		/*
		 * Required in authentication process to provide displaying and/or input passkey or yes/no butttons confirmation
		 */
		BLEDevice::setSecurityCallbacks(new MySecurity());

		BLEService* pService = pServer->createService("91bad492-b950-4226-aa2b-4ede9fa42f59");

		BLECharacteristic* pCharacteristic = pService->createCharacteristic(
			BLEUUID("0d563a58-196a-48ce-ace2-dfec78acc814"),
			BLECharacteristic::PROPERTY_READ  |
			BLECharacteristic::PROPERTY_NOTIFY    |
			BLECharacteristic::PROPERTY_WRITE |
			BLECharacteristic::PROPERTY_INDICATE
		);
		pCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
		pCharacteristic->setCallbacks(new bleCharacteristicCallback());
		pCharacteristic->setValue("Hello World!");

		BLE2902* p2902Descriptor = new BLE2902();
		p2902Descriptor->setNotifications(true);
		pCharacteristic->addDescriptor(p2902Descriptor);
		p2902Descriptor->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);

		pService->start();

		BLEAdvertising* pAdvertising = pServer->getAdvertising();
		pAdvertising->addServiceUUID(BLEUUID(pService->getUUID()));

		BLESecurity *pSecurity = new BLESecurity();
		pSecurity->setKeySize();
		pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_ONLY);
		pSecurity->setCapability(ESP_IO_CAP_IO);
		pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
		//pSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

		pAdvertising->start();

		ESP_LOGD(LOG_TAG, "Advertising started!");
		delay(portMAX_DELAY);
	}
};


void SampleServer_authentication_numeric_confirmation(void)
{
	//esp_log_level_set("*", ESP_LOG_DEBUG);
	MainBLEServer* pMainBleServer = new MainBLEServer();
	pMainBleServer->setStackSize(20000);
	pMainBleServer->start();

} // app_main


