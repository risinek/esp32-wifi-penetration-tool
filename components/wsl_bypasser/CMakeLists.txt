idf_component_register(
# "wsl_bypasser" file MUST be C, not C++ !!! Otherwise trick with replacing ieee80211_raw_frame_sanity_check() function
# from ESP IDF by our imnplementation will not work
    SRCS "wsl_bypasser.c" "WiFiFramesSender.cpp" 

    INCLUDE_DIRS "interface"
    PRIV_REQUIRES esp_wifi freertos_utils
)
target_link_libraries(${COMPONENT_LIB} -Wl,-zmuldefs)