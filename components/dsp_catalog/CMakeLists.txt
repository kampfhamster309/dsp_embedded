idf_component_register(
    SRCS "dsp_catalog.c"
    INCLUDE_DIRS "include"
    REQUIRES dsp_config dsp_build dsp_http dsp_json dsp_msg
)
