idf_component_register(SRCS "sniffer.cpp" "ap_scanner.cpp" "wifi_controller.cpp"
                    INCLUDE_DIRS "interface"
                    REQUIRES esp_wifi esp_event # For using "ESP_EVENT_DECLARE_BASE" in header file
                    PRIV_REQUIRES device_id)