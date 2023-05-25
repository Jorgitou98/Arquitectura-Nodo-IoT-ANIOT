idf_component_register(SRCS "si7021.c" "sampling.c" 
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES crc nvs_flash)