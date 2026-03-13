idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES dsp_config dsp_mbedtls dsp_http dsp_mem dsp_wifi dsp_identity dsp_jwt dsp_psk dsp_daps dsp_json dsp_jsonld dsp_msg dsp_build dsp_catalog dsp_neg dsp_xfer dsp_shared dsp_protocol dsp_application dsp_rtc_state
)
