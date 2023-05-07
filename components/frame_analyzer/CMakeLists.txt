idf_component_register(SRCS "frame_analyzer.c" "frame_analyzer_parser.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_event esp_wifi wifi_controller)