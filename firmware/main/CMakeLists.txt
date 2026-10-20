idf_component_register(
    SRCS app_main.c app_console.c app_shell.c
    INCLUDE_DIRS .
    REQUIRES core ui board
    PRIV_REQUIRES console esp_app_format esp_timer esp_system esp_hw_support heap freertos log
)
