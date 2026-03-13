idf_component_register(
    SRCS "dsp_application.c"
    INCLUDE_DIRS "include"
    REQUIRES dsp_config dsp_shared esp_timer
)
