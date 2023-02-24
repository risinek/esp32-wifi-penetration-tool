/**
 * Main file for running the BLE samples.
 */
extern "C" {
	void app_main(void);
}


// The list of sample entry points.
void Sample_MLE_15(void);
void Sample1(void);
void SampleAsyncScan(void);
void SampleClient(void);
void SampleClient_Notify(void);
void SampleClientAndServer(void);
void SampleClientDisconnect(void);
void SampleClientWithWiFi(void);
void SampleNotify(void);
void SampleRead(void);
void SampleScan(void);
void SampleSensorTag(void);
void SampleServer(void);
void SampleWrite(void);

//
// Un-comment ONE of the following
//            ---
void app_main(void) {
	//Sample_MLE_15();
	//Sample1();
	//SampleAsyncScan();
	//SampleClient();
	//SampleClient_Notify();
	//SampleClientAndServer();
	//SampleClientDisconnect();
	//SampleClientWithWiFi();
	//SampleNotify();
	//SampleRead();
	//SampleSensorTag();
	//SampleScan();
	SampleServer();
	//SampleWrite();
} // app_main
