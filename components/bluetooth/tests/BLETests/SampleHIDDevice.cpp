/**
 * Create a new BLE server.
 */
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"
#include "BLEHIDDevice.h"
#include "SampleKeyboardTypes.h"
#include <esp_log.h>
#include <string>
#include <Task.h>

#include "sdkconfig.h"

static char LOG_TAG[] = "SampleHIDDevice";

static BLEHIDDevice* hid;

class MyTask : public Task {
	void run(void*){
    	vTaskDelay(10000);
    	const char* hello = "Hello world from esp32 hid keyboard!!!";
		while(*hello){
			KEYMAP map = keymap[(uint8_t)*hello];
			uint8_t a[] = {map.modifier, 0x0, map.usage, 0x0,0x0,0x0,0x0,0x0};
			hid->inputReport(NULL)->setValue(a,sizeof(a));
			hid->inputReport(NULL)->notify();

			hello++;
		}
			uint8_t v[] = {0x0, 0x0, 0x0, 0x0,0x0,0x0,0x0,0x0};
			hid->inputReport(NULL)->setValue(v, sizeof(v));
			hid->inputReport(NULL)->notify();
		vTaskDelete(NULL);
	}
};
  MyTask *task;

  class MyCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer){
    	task->start();
    }

    void onDisconnect(BLEServer* pServer){

    }
  };
uint32_t passKey = 0;
  class MySecurity : public BLESecurityCallbacks {

  	uint32_t onPassKeyRequest(){
        ESP_LOGE(LOG_TAG, "The passkey request %d", passKey);

  		vTaskDelay(25000);
  		return passKey;
  	}
  	void onPassKeyNotify(uint32_t pass_key){
          ESP_LOGE(LOG_TAG, "The passkey Notify number:%d", pass_key);
          passKey = pass_key;
  	}
  	bool onSecurityRequest(){
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

class MainBLEServer: public Task {
	void run(void *data) {
		ESP_LOGD(LOG_TAG, "Starting BLE work!");

		task = new MyTask();
		BLEDevice::init("ESP32");
		BLEServer *pServer = BLEDevice::createServer();
		pServer->setCallbacks(new MyCallbacks());
		pServer->setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_NO_MITM);
		pServer->setSecurityCallbacks(new MySecurity());

		/*
		 * Instantiate hid device
		 */
		hid = new BLEHIDDevice(pServer);

		/*
		 * Set manufacturer name
		 * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.manufacturer_name_string.xml
		 */
		std::string name = "esp-community";
		hid->manufacturer()->setValue(name);

		/*
		 * Set pnp parameters
		 * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.pnp_id.xml
		 */
		const uint8_t pnp[] = {0x01,0x02,0xe5,0xab,0xcd,0x01,0x10};
		hid->pnp()->setValue((uint8_t*)pnp, sizeof(pnp));

		/*
		 * Set hid informations
		 * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.hid_information.xml
		 */
		const uint8_t val1[] = {0x01,0x11,0x00,0x03};
		hid->hidInfo()->setValue((uint8_t*)val1, 4);

		/*
		 * Mouse
		 */
		const uint8_t reportMap2[] = {
			USAGE_PAGE(1), 			0x01,
			USAGE(1), 				0x02,
			 COLLECTION(1),			0x01,
			 USAGE(1),				0x01,
			 COLLECTION(1),			0x00,
			 USAGE_PAGE(1),			0x09,
			 USAGE_MINIMUM(1),		0x1,
			 USAGE_MAXIMUM(1),		0x3,
			 LOGICAL_MINIMUM(1),	0x0,
			 LOGICAL_MAXIMUM(1),	0x1,
			 REPORT_COUNT(1),		0x3,
			 REPORT_SIZE(1),		0x1,
			 INPUT(1), 				0x2,		// (Data, Variable, Absolute), ;3 button bits
			 REPORT_COUNT(1),		0x1,
			 REPORT_SIZE(1),		0x5,
			 INPUT(1), 				0x1,		//(Constant), ;5 bit padding
			 USAGE_PAGE(1), 		0x1,		//(Generic Desktop),
			 USAGE(1),				0x30,
			 USAGE(1),				0x31,
			 LOGICAL_MINIMUM(1),	0x81,
			 LOGICAL_MAXIMUM(1),	0x7f,
			 REPORT_SIZE(1),		0x8,
			 REPORT_COUNT(1),		0x2,
			 INPUT(1), 				0x6,		//(Data, Variable, Relative), ;2 position bytes (X & Y)
			 END_COLLECTION(0),
			END_COLLECTION(0)
		};
		/*
		 * Keyboard
		 */
		const uint8_t reportMap[] = {
			USAGE_PAGE(1),      0x01,       // Generic Desktop Ctrls
			USAGE(1),           0x06,       // Keyboard
			COLLECTION(1),      0x01,       // Application
			USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
			USAGE_MINIMUM(1),   0xE0,
			USAGE_MAXIMUM(1),   0xE7,
			LOGICAL_MINIMUM(1), 0x00,
			LOGICAL_MAXIMUM(1), 0x01,
			REPORT_SIZE(1),     0x01,       //   1 byte (Modifier)
			REPORT_COUNT(1),    0x08,
			INPUT(1),           0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position
			REPORT_COUNT(1),    0x01,       //   1 byte (Reserved)
			REPORT_SIZE(1),     0x08,
			INPUT(1),           0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
			REPORT_COUNT(1),    0x05,       //   5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
			REPORT_SIZE(1),     0x01,
			USAGE_PAGE(1),      0x08,       //   LEDs
			USAGE_MINIMUM(1),   0x01,       //   Num Lock
			USAGE_MAXIMUM(1),   0x05,       //   Kana
			OUTPUT(1),          0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
			REPORT_COUNT(1),    0x01,       //   3 bits (Padding)
			REPORT_SIZE(1),     0x03,
			OUTPUT(1),          0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
			REPORT_COUNT(1),    0x06,       //   6 bytes (Keys)
			REPORT_SIZE(1),     0x08,
			LOGICAL_MINIMUM(1), 0x00,
			LOGICAL_MAXIMUM(1), 0x65,       //   101 keys
			USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
			USAGE_MINIMUM(1),   0x00,
			USAGE_MAXIMUM(1),   0x65,
			INPUT(1),           0x00,       //   Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
			END_COLLECTION(0)
		};
		/*
		 * Set report map (here is initialized device driver on client side)
		 * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.report_map.xml
		 */
		hid->setReportMap((uint8_t*)reportMap, sizeof(reportMap));

		/*
		 * We are prepared to start hid device services. Before this point we can change all values and/or set parameters we need.
		 * Also before we start, if we want to provide battery info, we need to prepare battery service.
		 * We can setup characteristics authorization
		 */
		hid->startServices();

		/*
		 * Its good to setup advertising by providing appearance and advertised service. This will let clients find our device by type
		 */
		BLEAdvertising *pAdvertising = pServer->getAdvertising();
		pAdvertising->setAppearance(HID_KEYBOARD);
		pAdvertising->addServiceUUID(hid->hidService()->getUUID());
		pAdvertising->start();


		BLESecurity *pSecurity = new BLESecurity();
		pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
		pSecurity->setCapability(ESP_IO_CAP_NONE);
		pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

		ESP_LOGD(LOG_TAG, "Advertising started!");
		delay(1000000);
	}
};


void SampleHID(void)
{
	//esp_log_level_set("*", ESP_LOG_DEBUG);
	MainBLEServer* pMainBleServer = new MainBLEServer();
	pMainBleServer->setStackSize(20000);
	pMainBleServer->start();

} // app_main
