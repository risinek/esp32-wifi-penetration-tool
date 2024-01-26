idf_component_register(SRCS "sniffer.c" "ap_scanner.c" "wifi_controller.c"
                    INCLUDE_DIRS "interface"
		    REQUIRES esp_timer esp_event esp_wifi)
