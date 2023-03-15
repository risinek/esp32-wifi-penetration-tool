idf_component_register(SRCS "frame_analyzer.c" "frame_analyzer_parser.c"
                    INCLUDE_DIRS "interface"
                    REQUIRES esp_wifi esp_event # For using "ESP_EVENT_DECLARE_BASE" in header file
                    PRIV_REQUIRES wifi_controller)