# ESP32 Wi-Fi Penetration Tool
## PCAP Serializer component

This component formats provided frames into PCAP binary format.

It's based on [Wiresharks LibPCAP file format referenc](https://gitlab.com/wireshark/wireshark/-/wikis/Development/LibpcapFileFormat).
It simply appends new frames to a structured buffer and it can be obtained on demand.

## Usage
1. First initialise new PCAP file buffer by calling `pcap_serializer_init()`.
1. Then `pcap_serializer_append_frame()` is used to append more frames into the file.
1. To get the buffer, call `pcap_serializer_get_buffer()` and `pcap_serializer_get_size()`.

## Reference
Doxygen API reference available