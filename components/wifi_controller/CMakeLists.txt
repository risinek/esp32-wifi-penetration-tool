idf_component_register(SRCS "sniffer.cpp" "ap_scanner.cpp" "wifi_controller.cpp"
                    INCLUDE_DIRS "interface"
                    PRIV_REQUIRES device_id)