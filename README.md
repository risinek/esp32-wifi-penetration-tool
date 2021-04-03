# ESP32 Wi-Fi Penetration Tool

This project implements attack on WPA/WPA2 handshake. 
It tries to get PMKID if AP provides it in plaintext during WPA authentication, otherwise it runs deauthentication attack and gather WPA handshake. 
Cracking of PMK from PMKID or handshake itself is not part of this project, as ESP32 doesn't have sufficient power to crash hashes in meaningful time period.

<p align="center">
    <img src="doc/images/soucastky_8b.png" alt="Hw components" width="400">
</p>

Each component has single header file in `interface` folder that provides "all-in-one" interface for this component. It provides all functions intended to be used outside
of the component.

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
This project consists of multiple components, that should be reusable with none or minimal code changes.

#### Main
Main component is entry point for this project. All neccessary initialisation steps are done here. **Management AP** is started and the controll is handed to other components.

#### Wifi Controller
This component wraps all Wi-Fi related operations. It's used to start AP, connect as STA, scan nearby APs etc. 

#### Webserver
Webserver component provides web UI to configure attacks. It expects that AP is started and no additional security features like SSL encryption are enabled.

<p align="center">
    <img src="doc/images/mini.jpg" alt="Hw components" width="300">
    <img src="doc/images/mini2.jpg" alt="Hw components" width="300">
</p>

## Disclaimer
This project demonstrates vulnerabilities of Wi-Fi networks and its underlaying 802.11 standard. It's not meant to be used for illegal activities.

## License
Even though this project is licensed under MIT license (see [LICENSE](LICENSE) file for details), don't be shy or greedy and share your work.