# ESP32 Wi-Fi Penetration Tool
## Frame Analyzer component

This component encapsulates Wi-Fi related operations and provides simplified API to be used by other components.

### Common
It provides API to for example start and stop AP with given configuration, to control STA connections, change interface MAC addresses etc.

### AP Scanner
AP Scanner provides an API to scan near APs and saves them into an array for further work.

### Sniffer
Sniffer is used to switch ESP32 into promiscuous mode (or off) and capture raw 802.11 frames. It provides filtering options and sends captured frames to event pool as SNIFFER_EVENTS event base.

## Reference
Doxygen API reference available