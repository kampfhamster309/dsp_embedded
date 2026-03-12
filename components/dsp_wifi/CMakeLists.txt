idf_component_register(
    SRCS "dsp_wifi.c"
    INCLUDE_DIRS "include"
    REQUIRES dsp_config esp_wifi esp_event esp_netif nvs_flash esp_timer
)
