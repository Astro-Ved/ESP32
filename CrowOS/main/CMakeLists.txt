idf_component_register(SRCS "main.c" "os_launcher.c"
                    INCLUDE_DIRS "."
                    REQUIRES fatfs sdmmc nvs_flash esp_partition app_update spi_flash)
