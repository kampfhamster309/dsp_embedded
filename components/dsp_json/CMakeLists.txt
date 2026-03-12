idf_component_register(
    SRCS "dsp_json.c" "cjson/cJSON.c"
    INCLUDE_DIRS "include" "cjson"
    REQUIRES dsp_config
)
