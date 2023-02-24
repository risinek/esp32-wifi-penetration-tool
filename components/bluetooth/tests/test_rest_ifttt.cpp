/*
 * Test the REST API client.
 */
#include <curl/curl.h>
#include <esp_log.h>
#include <IFTTT.h>
#include <RESTClient.h>
#include <string>
#include <Task.h>
#include <WiFi.h>
#include <WiFiEventHandler.h>

#include "sdkconfig.h"

static char tag[] = "test_rest";

extern "C" {
	void app_main(void);
}


static WiFi *wifi;

class IFTTTTestTask: public Task {
	void run(void *data) {
		IFTTT iftt = IFTTT("dG-QmEUwCUyBBLHm1owPtq");
		iftt.trigger("test", "A", "BCD", "Val3");
		ESP_LOGD(tag, "trigger done!");
	}
};


class MyWiFiEventHandler: public WiFiEventHandler {

	esp_err_t staGotIp(system_event_sta_got_ip_t event_sta_got_ip) {
		ESP_LOGD(tag, "MyWiFiEventHandler(Class): staGotIp");


		IFTTTTestTask *pTestTask= new IFTTTTestTask();
		pTestTask->setStackSize(12000);
		pTestTask->start();
		return ESP_OK;
	}
};


void app_main(void) {
	ESP_LOGD(tag, "app_main: libcurl starting");
	MyWiFiEventHandler *eventHandler = new MyWiFiEventHandler();

	wifi = new WiFi();
	wifi->setWifiEventHandler(eventHandler);

	wifi->connectAP("sweetie", "l16wint!");
}
