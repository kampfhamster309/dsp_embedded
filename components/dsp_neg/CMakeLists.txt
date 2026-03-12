idf_component_register(
    SRCS "dsp_neg.c"
    INCLUDE_DIRS "include"
    REQUIRES dsp_config dsp_build dsp_http dsp_msg dsp_json dsp_jsonld
)
