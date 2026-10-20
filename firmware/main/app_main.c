/* Composition root: board up, then hand over to the shell (ADR-0004). */
#include <stdbool.h>

#include "app_console.h"
#include "app_shell.h"
#include "board/board.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "main";

void app_main(void)
{
    board_sleep_release_holds();
    uint64_t wake_buttons = board_wake_buttons();
    ESP_LOGI(TAG, "reset reason %d, wake buttons 0x%llx", esp_reset_reason(), wake_buttons);

    bool usb = false;
    ESP_ERROR_CHECK(board_power_init(&usb));
    ESP_ERROR_CHECK(board_backlight_init());

    lv_display_t *disp = NULL;
    ESP_ERROR_CHECK(board_display_init(false, &disp));

    app_shell_start(disp, usb, wake_buttons);

    /* No host can be attached on battery, so the console would only poll. */
    if (usb) {
        app_console_start();
    }
}
