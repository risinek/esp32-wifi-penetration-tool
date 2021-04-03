# ESP32 Wi-Fi Penetration Tool
## HCCAPX Serializer component

This component formats provided frames into HCCAPX ([hashcat](https://hashcat.net/hashcat/)) binary format.

It's based on [Hashcats HCCAPX file format reference](https://hashcat.net/wiki/doku.php?id=hccapx).
It parses provided EAPOL-Key packets (using [Frame Analyzer component](../frame_analyzer)) that are part of WPA handshake and builds HCCAPX formatted file that can be 
later supplied directly to hashcat to crack PSK (Pre-Shared Key, commonly referred to as *network password*).

## Usage
1. First initialise the serializer by providing SSID of target AP by calling `hccapx_serializer_init`
1. Add more handshakes frames by calling `hccapx_serializer_add_frame()`
1. Get the pointer to buffer where HCCAPX binary is stored `hccapx_serializer_get()`

## Reference
Doxygen API reference available