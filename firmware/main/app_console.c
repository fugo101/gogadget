#include "app_console.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_shell.h"
#include "board/board.h"
#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_console.h"
#include "esp_heap_caps.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_timer.h"

static const char *TAG = "console";

static int usage(const char *text)
{
    printf("usage: %s\n", text);
    return 1;
}

static int cmd_ui_key(int argc, char **argv)
{
    static const struct {
        const char *name;
        gg_nav_t nav;
    } k_keys[] = {{"next", GG_NAV_NEXT}, {"back", GG_NAV_BACK}, {"enter", GG_NAV_ENTER}};
    if (argc == 2) {
        for (size_t i = 0; i < sizeof(k_keys) / sizeof(k_keys[0]); i++) {
            if (strcmp(argv[1], k_keys[i].name) == 0) {
                app_post((gg_msg_t){.kind = GG_MSG_NAV, .nav = k_keys[i].nav});
                return 0;
            }
        }
    }
    return usage("ui.key next|back|enter");
}

static int cmd_ui_goto(int argc, char **argv)
{
    static const char *const k_screens[GG_SCREEN_COUNT] = {
        [GG_SCREEN_WATCHFACE] = "watchface",
        [GG_SCREEN_MENU] = "menu",
        [GG_SCREEN_SYSINFO] = "sysinfo",
        [GG_SCREEN_TESTPATTERN] = "test",
    };
    if (argc == 2) {
        for (int i = 0; i < GG_SCREEN_COUNT; i++) {
            if (strcmp(argv[1], k_screens[i]) == 0) {
                app_post((gg_msg_t){.kind = GG_MSG_GOTO, .screen = (gg_screen_t)i});
                return 0;
            }
        }
    }
    return usage("ui.goto watchface|menu|sysinfo|test");
}

static int cmd_power_stay(int argc, char **argv)
{
    if (argc == 2 && (strcmp(argv[1], "on") == 0 || strcmp(argv[1], "off") == 0)) {
        app_shell_stay_awake(strcmp(argv[1], "on") == 0);
        printf("power.stay %s\n", argv[1]);
        return 0;
    }
    return usage("power.stay on|off   (on: never enter Standby, for development)");
}

static int cmd_bl(int argc, char **argv)
{
    if (argc == 2) {
        char *end = NULL;
        long level = strtol(argv[1], &end, 10);
        if (end && *end == '\0' && level >= 0 && level <= 16) {
            board_display_lock();
            board_backlight_set((uint8_t)level);
            board_display_unlock();
            return 0;
        }
    }
    return usage("bl <0-16>");
}

static int cmd_time_set(int argc, char **argv)
{
    if (argc == 2) {
        char *end = NULL;
        long long epoch = strtoll(argv[1], &end, 10);
        if (end && *end == '\0' && epoch > 1700000000LL) {
            app_shell_set_time((int64_t)epoch);
            return 0;
        }
    }
    return usage("time.set <unix-epoch-seconds>   (until NTP Sync exists)");
}

static int cmd_sys_info(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    printf("firmware   %s\n", esp_app_get_description()->version);
    printf("esp-idf    %s\n", esp_get_idf_version());
    printf("chip       esp32s3 rev v%d.%d, %d cores\n", chip.revision / 100, chip.revision % 100,
           chip.cores);
    printf("reset      %d, wake causes 0x%" PRIx32 "\n", esp_reset_reason(),
           esp_sleep_get_wakeup_causes());
    printf("uptime     %" PRId64 " ms\n", esp_timer_get_time() / 1000);
    app_shell_print_state();
    return 0;
}

static int cmd_heap(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("internal   %u free, %u min free, %u largest block\n",
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
           (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL),
           (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    printf("psram      %u free\n", (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    return 0;
}

static int cmd_reboot(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    esp_restart();
}

void app_console_start(void)
{
    static const esp_console_cmd_t k_cmds[] = {
        {.command = "ui.key",
         .help = "Inject a navigation action: next|back|enter",
         .func = cmd_ui_key},
        {.command = "ui.goto",
         .help = "Jump to a screen: watchface|menu|sysinfo|test",
         .func = cmd_ui_goto},
        {.command = "power.stay",
         .help = "on|off: keep the device out of Standby",
         .func = cmd_power_stay},
        {.command = "bl", .help = "Set the backlight level 0-16", .func = cmd_bl},
        {.command = "time.set", .help = "Set the clock from a Unix epoch", .func = cmd_time_set},
        {.command = "sys.info",
         .help = "Firmware, chip, reset reason and state",
         .func = cmd_sys_info},
        {.command = "heap", .help = "Free heap", .func = cmd_heap},
        {.command = "reboot", .help = "Restart the chip", .func = cmd_reboot},
    };

    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_cfg.prompt = "gg>";
    esp_console_dev_usb_serial_jtag_config_t dev_cfg =
        ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    if (esp_console_new_repl_usb_serial_jtag(&dev_cfg, &repl_cfg, &repl) != ESP_OK) {
        ESP_LOGW(TAG, "console unavailable");
        return;
    }
    esp_console_register_help_command();
    for (size_t i = 0; i < sizeof(k_cmds) / sizeof(k_cmds[0]); i++) {
        ESP_ERROR_CHECK(esp_console_cmd_register(&k_cmds[i]));
    }
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
