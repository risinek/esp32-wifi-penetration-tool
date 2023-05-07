idf_component_register(SRCS "sniffer.c" "ap_scanner.c" "wifi_controller.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_event esp_wifi)