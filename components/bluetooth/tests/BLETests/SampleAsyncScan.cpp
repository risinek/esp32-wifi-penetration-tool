/**
 * Perform an async scanning for BLE advertised servers.
 */
#include "BLEUtils.h"
#include "BLEScan.h"
#include <string>

#include "BLEDevice.h"
#include "BLEAdvertisedDevice.h"
#include "sdkconfig.h"

/**
 * Callback for each detected advertised device.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
	void onResult(BLEAdvertisedDevice advertisedDevice) {
		printf("Advertised Device: %s\n", advertisedDevice.toString().c_str());
	}
};


/**
 * Callback invoked when scanning has completed.
 */
static void scanCompleteCB(BLEScanResults scanResults) {
	printf("Scan complete!\n");
	printf("We found %d devices\n", scanResults.getCount());
	scanResults.dump();
} // scanCompleteCB

/**
 * Run the sample.
 */
static void run() {
	printf("Async Scanning sample starting\n");
	BLEDevice::init("");

	BLEScan* pBLEScan = BLEDevice::getScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), false);
	pBLEScan->setActiveScan(true);
	printf("About to start scanning for 10 seconds\n");
	pBLEScan->start(10, scanCompleteCB);
	printf("Now scanning in the background ... scanCompleteCB() will be called when done.\n");

	//
	// Now going into a loop logging that we are still alive.
	//
	while(1) {
		printf("Tick! - still alive\n");
		FreeRTOS::sleep(1000);
	}
	printf("Scanning sample ended\n");
}

void SampleAsyncScan(void)
{
	run();
} // app_main
