# TODO

PRIORITIES: 16, 1, 2, 3

1. Check if we can have a callback, notifying about hew WiFi client connected. Currently looks like there are 2 layers of such callbacks - system and user-defined. And in some cases (ex. user didn't authentificated yet) user-level handler is not called. Need to check it. If this is true, we are not able to check which WiFi stations are trying to connect to our AP

2. Test if we can send deauth frame to specific MAC. Make sure that device with that MAC will disconnect. Currently broadcast attack doesn't affect many devices (including my phone).

3. DEPENDS ON 1 + 2. If we can get callbacks about WiFi stations, which are trying to connect to our AP, then need to dynamically store their MAC-addresses and when sending deauf, send deauth to them explicitely (not via broadcast). Such a targetted frames will not be ignored by some devices. Not to blow up this list, we can clean it, let's say, once per day if it exceeds specified size.<br>
I REALLY need MAC addresses from 5G network to add them to black list on UI later
    * Make new end-point to read this list

4. "Black list of WiFi client MACs" feature. Add WiFi client MAC addresses list in WebUI. For those addresses need to send personal deauth frame for every Rogue AP during DOS attack. So, specified WiFi clients will not be able to connect to any attacked AP

5. ~~Also need to have black list of router's MACs. If one of them is detected, its SSID should be printed on WebUI. So, if user will change SSID, they will be anyway dosplayed.<br>
Ex. use pre-defined list of such MACs in WebUI. After scanning is done, WebUI can check list of returned SSIDs to find those ones from the black list~~<br>
Looks like this feature doesn't have much sense. Imagine user comes after some time to ESP and wants to reset it and initiate attack again, but he can not see target WiFi. If he remembers its MAC, then fine, he will find in WiFi list network with that MAC and start attack again. This feature is about having special control on UI for storing such MACs. User will not want to spend his time on entering MAC if he can simply pick up that network from list.

6. Need to handle case when AP under attack changes its name. Ex. user of attacked AP starts suspectign something and changing name of his WiFi. We could store list of MAC addresses of those WiFi networks, that user selected to attack. Once per some time (5-10 min?) we could run task which scans available networks and takes updated data for those ones, which MACs are in that list.

7. "Ninja-feature" :) If we scan all networks (as in feature #6) and see that some of networks, selected by user, are missing, we stop creating Rogue Ap for that network. It can be useful if user of AP under attack starts suspectign something and decides to turns off his router. In that case he still can see that his WiFi network is still available. Thus he will start suspecting he is under attack. By hiding Rogue AP in such cases we prolong the time before he starts realizing he is under attack.

8. Need to make sure that none of my changes are breaking existing code. Ex. that proper status of attack will be returned by attack_get_status(), that each attack really has proper status (remember, that now we have infinite attacks and ability to interrupt attacks)

9. Make sure that timeout in WebUI is handled well (even in case of infinite attack).

10. ~~Stability test (aster most of changes are done). Keep ESP32 running as long as possible, running different attacks. The goal is to make sure that after different use cases it is still up and running<br>~~
May be it doesn't make much sense, because most of the time infinite attack will be run, which will always do the same things<br>
It would make sense in case of non-infinit attacks, as was implemented by "risinek"

11. "Stop attack" button in WebUI does NOT make sense in method which includes Rogue AP, because we will simply can not send any request to ESP32. Actually after initiating such attack the only thing we can do in UI is to show message that ESP32's WiFi will be off during all attack. The only way to make it available again - reboot via Bluetooth
    * Need to analyze other cases, when ESP32 will not be available and probably adapt behavior of UI for it

12. BUG: if DOS Braadcast attack is in progress and you refresh page, list of APs looks like is read by ESP (see its logs), but not displayed on WebUI. Connection to ESP is lost. May be ESP kills its AP when tries to send this list to WebUI?<br>
Probably incorrect handling of request to "/status" when we start WebUI during attack?<br>
Not wlways reproducible.

13. Blink red LED few times at startup (FREERTOS task?) and then turn it off

14. New commands for Bluetooth - blink LED, start LED, stop LED. To make device easier to be found (if forgot where is it)

16. How to improve WiFi antenna?<br>
    Can try this:
    1. https://peterneufeld.wordpress.com/2021/10/14/esp32-range-extender-antenna-modification/ It gives just 3 db (+1.5 dB with 4 mm shorter wire - refer to comments)
    2. https://community.home-assistant.io/t/how-to-add-an-external-antenna-to-an-esp-board/131601 Looks more promissing, up to -90 -> -65 !!!. But need solder iron and clue.<br>
    Possible(?) antenna ("esp32 ăng ten") - https://shopee.vn/%C4%82ng-Ten-Khu%E1%BA%BFch-%C4%90%E1%BA%A1i-T%C3%ADn-Hi%E1%BB%87u-FM-AM-G%E1%BA%AFn-N%C3%B3c-Xe-H%C6%A1i-Benz-Bmw-Audi-Toyota-9-11-16-Inch-i.267737919.4436223296?sp_atk=4bac905a-a588-4f02-9af3-0359c20b77a3&xptdk=4bac905a-a588-4f02-9af3-0359c20b77a3
    3. As an option - buy ESP-WHROOM-32**U**. But I don't want to spend even more money for this project.

18. How to report to WebUI about progress of OTA? The only way I know to send messages from client to web-server is web-sockets. Can we avoid such an overcomplication?

19. Is it possible to set config variable via command line as "idf.py build -DDEVICE_ID=2"?
