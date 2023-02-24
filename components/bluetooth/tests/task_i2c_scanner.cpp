#include <esp_log.h>
#include <FreeRTOS.h>
#include <string>
#include <Task.h>
#include <I2C.h>

#include "sdkconfig.h"

#define DEVICE_ADDRESS 0
#define SDA_PIN 25
#define SCL_PIN 26

//static char tag[] = "task_cpp_utils";

class I2CScanner: public Task {
	void run(void *data) override {
		I2C i2c;
		i2c.init((uint8_t)DEVICE_ADDRESS, (gpio_num_t)SDA_PIN, (gpio_num_t)SCL_PIN);
		i2c.scan();
	} // End run
};


static I2CScanner i2cScanner = I2CScanner();

void task_i2c_scanner(void *ignore) {
	i2cScanner.start();
	FreeRTOS::deleteTask();
}
