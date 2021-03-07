idf_component_register(SRCS "frame_analyzer.c" "data_frame_parser.c"
                    INCLUDE_DIRS "interface"
                    PRIV_REQUIRES wifi_controller)