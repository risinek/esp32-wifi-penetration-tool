ESP32 WPA Handshake attack
====================

This project implements attack on WPA/WPA2 handshake. 
It tries to get PMKID if AP provides it in plaintext during WPA authentication, otherwise it runs deauthentication attack and gather WPA handshake. 
Cracking of PMK from PMKID or handshake itself is not part of this project, as ESP32 doesn't have sufficient power to crash hashes in meaningful time period.
