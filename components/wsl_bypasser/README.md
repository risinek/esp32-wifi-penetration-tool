# ESP32 Wi-Fi Penetration Tool
## Wi-Fi Stack Libraries (WSL) Bypasser component

This components main purpose is to bypass Wi-Fi Stack Libaries blocking mechanism that disallows sending some types of raw 802.11 frames. 

It's based on [ESP32-Deauther](https://github.com/GANESH-ICMC/esp32-deauther) project, where the function used to check type of frame in frame buffer [was decompiled by Ghidra tool](https://github.com/GANESH-ICMC/esp32-deauther/issues/9) and it's name `ieee80211_raw_frame_sanity_check` was found.

The princip of this bypass is to use linker flag during compilation that allows multiple function definitions - `-Wl,-zmuldefs`. This allows this component to override default function behaviour and always return value that allows further transmittion of frame buffer by Wi-Fi Stack Libraries.

This is done in [CMakeLists.txt](CMakeLists.txt) by following line:
```cmake
target_link_libraries(${COMPONENT_LIB} -Wl,-zmuldefs)
```

And the function itself is defined in [wsl_bypasser.c](wsl_bypasser.c) as:
```c
int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3){
    return 0;
}
```