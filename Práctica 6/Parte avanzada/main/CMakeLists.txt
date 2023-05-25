idf_component_register(SRCS "console_example_main.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES si7021 hall console fatfs nvs_flash cmd_system cmd_nvs)
