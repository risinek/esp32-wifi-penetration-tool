idf_component_register(SRCS "frame_analyzer.c" "frame_analyzer_parser.c"
                    INCLUDE_DIRS "interface"
                    PRIV_REQUIRES wifi_controller REQUIRES esp_event esp_wifi)