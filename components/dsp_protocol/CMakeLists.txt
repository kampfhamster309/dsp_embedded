idf_component_register(
    SRCS "dsp_protocol.c"
    INCLUDE_DIRS "include"
    REQUIRES dsp_config dsp_shared dsp_identity dsp_mbedtls dsp_http
             dsp_catalog dsp_neg dsp_xfer
)
