# BLE C++ classes

# BLE Server
## BLE Characteristic callbacks
When a client connects to us as a BLE Server, it can change a characteristics value or read from a characteristic.  The BLECharacteristicCallbacks class provides a handler for such.
# BLE Client
# Advertising
When we are being a BLE server we have the opportunity to perform advertising.  Advertising means that we broadcast information about ourselves such that interested parties (presumably clients) can receive that information.  The BLE advertising is described in the BLE specifications.

Some key facts:

* Advertisements can be sent periodically with the period being from 20ms to 10.24 seconds in steps of 0.625 ms.
* An advertisement payload can be up to 31 bytes in length.
* A payload can contain a sequence of records where a record is composed of a length, type and data.

We can build a BLE advertising structure and specify that it should be sent for an advertisement using `esp_ble_gap_config_adv_data_raw` or sent using a scan response message using `esp_ble_gap_config_scan_rsp_data_raw`.

Looking at the data structure for advertising we find it consists of records where each record is:

* 1 byte length
* 1 byte record type
* length-1 bytes of data

Trailing data (assuming we don't use all 31 bytes) shall be zeros.  The advert types that include payload data are:

* ADV_IND
* ADV\_NONCONN\_IND
* ADV\_SCAN\_IND

The following advertising types are supported:

| Type | Description |
|--------|------
|0x01| Flags |
|0x02|Incomplete list of 16 bit service UUIDs
|0x03|Complete list of 16 bit service UUIDs
|0x04|Incomplete list of 32 bit service UUIDs
|0x05|Complete list of 32 bit UUIDs
|0x06|Incomplete list of 128 bit service UUIDs
|0x07|Complete list of 128 bit service UUIDs
|0x08|Shortened local name
|0x09|Complete local name
|0x16|Service data (16 bit)
|0x19|Appearance
|0x20|Service data (32 bit)
|0x21|Service data (128 bit)
|0xFF|Manufacturer data



See also:

* BLE Spec 4.2 Vol 3 Part C Chapter 11 - Advertising and Scan response data format.
* [Advertisement record types](https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile)
* [A BLE Advertising Primer](http://www.argenox.com/a-ble-advertising-primer/)

## Exposed functions
|Type|Function
|-|-
|0x01|`setFlags(uint8_t flag)`
|0x02, 0x04, 0x06|`setPartialServices(vector<BLEUUID>)`
|0x02, 0x04, 0x06|`setPartialServices(BLEUUID)`
|0x03, 0x05, 0x07|`setCompleteServices(vector<BLEUUID>)`
|0x03, 0x05, 0x07|`setCompleteServices(BLEUUID)`
|0x08|`setShortName(std::string)`
|0x09|`setName(std::string)`
|0x16, 0x20, 0x21|`setServiceData(BLEUUID, std::string)`
|0x19|`setAppearance(uint16_t)`
|0xFF|`setManufacturerData(std::string)`