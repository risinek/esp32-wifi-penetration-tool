# ESP32 Wi-Fi Penetration Tool
## Frame Analyzer component

This component processes captured frames and parses them. 
It provides parsing functionality to other components as well as frame filtering by searching for specific types of frames.

### Filtering
Filtering functionality is based on listening to event pool for SNIFFER_EVENTS events. Filtering can be started by calling `frame_analyzer_capture_start()` and
providing it search criteria - currently just search type and BSSID.

It then listens to SNIFFER_EVENTS events, parses captured frames and matches them with search criteria. If some frame matches criteria, it forward this frame (or part of it) to event pool as DATA_FRAME_EVENTS event base.

### Parsing
Parsing functionality provides a way for other components to get required data from frame (or its parts). For example `parse_eapol_packet` will parse EAPOL packet from data frame if available.

### Frame structures
This component also provides a header file with structures based on 802.11 standard for parsing purposes.

## Usage
If you want to use this package in your project, just pass captures frames from sniffer logic to event loop and start capture by `frame_analyzer_capture_start()`.

Or use just parsing functionality of this component.

## Reference
Doxygen API reference available