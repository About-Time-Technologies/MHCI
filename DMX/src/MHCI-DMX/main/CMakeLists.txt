idf_component_register(SRCS "main.c"
                    PRIV_REQUIRES esp_driver_ledc esp_driver_gpio esp_dmx
                    INCLUDE_DIRS ".")
