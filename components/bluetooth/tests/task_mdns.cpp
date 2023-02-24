#include <esp_log.h>
#include <WiFiEventHandler.h>
#include <WiFi.h>
#include <FreeRTOS.h>
#include <string>
#include <stdio.h>
#include <Task.h>

#include "sdkconfig.h"

static char tag[] = "task_mdns";

class TargetWiFiEventHandler: public WiFiEventHandler {
	 esp_err_t staGotIp(system_event_sta_got_ip_t event_sta_got_ip) {
			ESP_LOGD(tag, "MyWiFiEventHandler(Class): staGotIp");
			MDNS *mdns = new MDNS();
			mdns->setHostname("HHHH");
			mdns->setInstance("IIII");
			mdns->serviceAdd("_test", "_tcp", 80);
			return ESP_OK;
	 }
};

class MDNSTask: public Task {
	void run(void *data) override {

		WiFi wifi;
		TargetWiFiEventHandler *eventHandler = new TargetWiFiEventHandler();
		wifi.setWifiEventHandler(eventHandler);
		wifi.connectAP("sweetie", "l16wint!");
	} // End run
};


static MDNSTask myTask = MDNSTask();

void task_mdns(void *ignore) {
	myTask.setStackSize(8000);
	myTask.start();
	FreeRTOS::deleteTask();
}
