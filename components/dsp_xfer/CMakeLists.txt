idf_component_register(
    SRCS "dsp_xfer.c"
    INCLUDE_DIRS "include"
    REQUIRES dsp_config dsp_build dsp_http dsp_json dsp_jsonld dsp_msg dsp_neg
)
