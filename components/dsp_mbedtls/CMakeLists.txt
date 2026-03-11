idf_component_register(
    SRCS "dsp_tls.c"
    INCLUDE_DIRS "include"
    REQUIRES dsp_config mbedtls esp-tls
)
