idf_component_register(
    SRCS
        board_backlight.c
        board_buttons.c
        board_display.c
        board_power.c
        board_sleep.c
    INCLUDE_DIRS include
    REQUIRES core esp_driver_gpio
    PRIV_REQUIRES esp_adc esp_lcd esp_timer esp_hw_support esp_rom freertos log
)
