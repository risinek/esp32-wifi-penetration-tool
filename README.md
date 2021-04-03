# ESP32 Wi-Fi Penetration Tool

This project implements attack on WPA/WPA2 handshake. 
It tries to get PMKID if AP provides it in plaintext during WPA authentication, otherwise it runs deauthentication attack and gather WPA handshake. 
Cracking of PMK from PMKID or handshake itself is not part of this project, as ESP32 doesn't have sufficient power to crash hashes in meaningful time period.

<p align="center">
    <img src="doc/images/soucastky_8b.png" alt="Hw components" width="400">
</p>

## Build
This project is currently developed using ESP-IDF 4.1 (commit `5ef1b390026270503634ac3ec9f1ec2e364e23b2`).

Project can be build the usual way:

```shell
idf.py build
idf.py flash
```

Legacy method using `make` is not supported by this project.

## Contributing
Feel free to contribute. Don't hestitate to refactor current code base. Please stick to Doxygen notation when commenting new functions and files. This project is mainly build for educational and demonstration purposes of ESP32 platform and Wi-Fi attacks implementations, so verbose documentation is welcome.

## Documentation
### API reference
This project uses Doxygen notation for documenting components API and implementation. Doxyfile is included so if you want to generate API reference, just run `doxygex` from root directory. It will generate HTML API reference into `doc/api/html`.

### Components
This project consists of multiple components, that can be reused in other projects. Each component has it's own README with detailed description. Here comes the list of components:

- **Main** component is entry point for this project. All neccessary initialisation steps are done here. Management AP is started and the controll is handed to other components.
- **Wifi Controller** component wraps all Wi-Fi related operations. It's used to start AP, connect as STA, scan nearby APs etc. 
- **Webserver** component provides web UI to configure attacks. It expects that AP is started and no additional security features like SSL encryption are enabled.
- **Deauther**
- **Frame Analyzer**
- **PCAP Serializer**
- **HCCAPX Serializer**

## Hardware 
This project was mostly build and tested on **ESP32-DEVKITC-32E**
but there should not be any differences for any **ESP32-WROOM-32** modules.

On the following pictures you can see
- **ESP32-DEVKITC-32E**
- 220mAh Li-Pol 3.7V accumulator (weights ±5g)
- MCP1702-3302ET voltage regulator
- Czech 5-koruna coin for scale
<p align="center">
    <img src="doc/images/mini.jpg" alt="Hw components" width="300">
    <img src="doc/images/mini2.jpg" alt="Hw components" width="300">
</p>

Altogether this setup weights around 17g. This can be further downsized by using smaller Li-Pol accumulator and using ESP32-WROOM-32 modul instead of whole dev board.

### Power consumption
Based on [Radioshuttle - Battery-Powered ESP32](https://www.radioshuttle.de/en/media-en/tech-infos-en/battery-powered-esp32/) article, ESP32 consumes around *80-180 mA* while operating on Wi-Fi interface. 

## Disclaimer
This project demonstrates vulnerabilities of Wi-Fi networks and its underlaying 802.11 standard and ESP32 platform can be utilised to attack on those vulnerable spots. Use responsibly against networks you have permission to attack on.

## License
Even though this project is licensed under MIT license (see [LICENSE](LICENSE) file for details), don't be shy or greedy and share your work.