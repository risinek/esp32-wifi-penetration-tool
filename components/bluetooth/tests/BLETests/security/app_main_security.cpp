/**
 * Main file for running the BLE samples.
 */
extern "C" {
	void app_main(void);
}


// The list of sample entry points.

void SampleClient_Encryption(void);
void SampleServer_Encryption(void);
void SampleClient_authentication_passkey(void);
void SampleServer_authentication_passkey(void);
void SampleClient_authentication_numeric_confirmation(void);
void SampleServer_authentication_numeric_confirmation(void);

void SampleServer_Authorization(void);

//
// Un-comment ONE of the following
//            ---
void app_main(void) {

	SampleClient_Encryption();
//	SampleServer_Encryption();
//	SampleClient_authentication_passkey();
//	SampleServer_authentication_passkey();
//	SampleClient_authentication_numeric_confirmation();
//	SampleServer_authentication_numeric_confirmation();
//
//	SampleServer_Authorization();
} // app_main
