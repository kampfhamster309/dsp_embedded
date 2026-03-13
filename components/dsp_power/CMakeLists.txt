idf_component_register(
    SRCS "dsp_power.c"
    INCLUDE_DIRS "include"
    REQUIRES dsp_config dsp_rtc_state dsp_http dsp_wifi esp_hw_support
)
