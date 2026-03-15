idf_component_register(
    SRCS "dsp_dht20.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_i2c esp_log
)
