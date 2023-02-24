#include <stdio.h>
#include <Task.h>
extern void dumpInfo();


extern "C" {
	void app_main();
}

class MyTask: public Task {
	void run(void* data) {
		int count = 0;
		/*
		while(1) {
			printf("count: %d\n", count);
			count++;
			vTaskDelay(1000/portTICK_PERIOD_MS);
		}
		*/
		/*
		char* x = "hello";
		uint32_t i = 0;
		printf("Hello world!");
		printf("Hello world! again!");
		char* p = (char*) i;
		*p = 123;
		*p */
		dumpInfo();
		printf("Done\n");
	}
};

void app_main() {
	MyTask* pMyTask = new MyTask();
	pMyTask->setStackSize(20000);
	pMyTask->start();

}
