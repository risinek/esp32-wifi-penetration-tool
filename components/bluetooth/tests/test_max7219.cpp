#include <esp_log.h>
#include <FreeRTOS.h>
#include <SPI.h>
#include <string>
#include <stdio.h>
#include <Task.h>
#include "MAX7219.h"

#include "sdkconfig.h"

//static char tag[] = "task_cpp_utils";



class TestMAX7219: public Task {
	void test8x8(MAX7219 &max7219) {
		int dir=1;
		int y=0;
		while(1) {
			for (auto x=0; x<8; x++) {
				for (auto i=0; i<8; i++) {
					max7219.setLed(x, y, true);
					FreeRTOS::sleep(100);
					max7219.setLed(x, y, false);
					y=y+dir;
				}
				dir = -dir;
				y=y+dir;
			}
		}
	}

	void test7SegDisplay(MAX7219 &max7219) {
		int i=0;
		while(1) {
			max7219.setNumber(i);
			i++;
			FreeRTOS::sleep(100);
		}
	}

	void run(void *data) override {
		SPI spi;
		spi.init();

		MAX7219 max7219 = MAX7219(&spi, 1);
		max7219.shutdown(false);
		max7219.setIntensity(4);
		//test7SegDisplay(max7219);
		test8x8(max7219);

	} // End run
};


static TestMAX7219 testMAX7219 = TestMAX7219();

void task_max7219(void *ignore) {
	testMAX7219.start();
	FreeRTOS::deleteTask();
}
