idf_component_register(SRCS "webserver.cpp"
                    INCLUDE_DIRS "interface"
                    PRIV_REQUIRES hccapx_serializer pcap_serializer esp_http_server wifi_controller main)