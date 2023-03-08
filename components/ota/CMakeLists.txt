idf_component_register(SRCS "ota.cpp"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES freertos_utils wifi_controller esp_https_ota app_update)
                    # This line is needed in case HTTPS is used for OTA and we need to store server certificate
                    # Refer to "simple_ota_example"
                    # EMBED_TXTFILES ./server_certs/ca_cert.pem)