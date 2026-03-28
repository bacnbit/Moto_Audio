idf_component_register(
    SRCS "wired_radio.c"
    INCLUDE_DIRS "."
    REQUIRES driver freertos esp_common
)
