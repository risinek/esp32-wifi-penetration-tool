idf_component_register(SRCS "webserver.c"
                    INCLUDE_DIRS "interface"
                    PRIV_REQUIRES hccapx_serializer pcap_serializer esp_http_server wifi_controller main)