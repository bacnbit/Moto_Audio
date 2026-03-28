idf_component_register(
    SRCS "ptt_handler.c"
    INCLUDE_DIRS "."
    REQUIRES driver freertos esp_common
)
