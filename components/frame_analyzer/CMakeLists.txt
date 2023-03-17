idf_component_register(SRCS "frame_analyzer.cpp" "frame_analyzer_parser.cpp"
                    INCLUDE_DIRS "interface"
                    REQUIRES esp_wifi esp_event # For using "ESP_EVENT_DECLARE_BASE" in header file
                    PRIV_REQUIRES wifi_controller)