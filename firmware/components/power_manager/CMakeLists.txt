idf_component_register(
    SRCS "power_manager.c"
    INCLUDE_DIRS "."
    REQUIRES driver freertos esp_common
)
