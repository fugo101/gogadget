#include "app_shell.h"

#include <stdio.h>
#include <sys/time.h>

#include "board/board.h"
#include "esp_app_desc.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "gg/gg_battery.h"
#include "ui/ui.h"

static const char *TAG = "shell";

#define QUEUE_LEN 16
#define PUMP_PERIOD_MS 20
#define SECOND_US (1000 * 1000)

/* Set once a Sync succeeded; survives deep sleep (RTC memory). */
RTC_DATA_ATTR static bool s_time_valid;

static QueueHandle_t s_queue;
static gg_state_t s_state;
static esp_timer_handle_t s_second_timer;
static volatile bool s_stay_awake; /* Dev console: power.stay on */

static uint32_t now_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

void app_post(gg_msg_t msg)
{
    msg.at_ms = now_ms();
    if (xQueueSend(s_queue, &msg, 0) != pdTRUE) {
        ESP_LOGW(TAG, "queue full, dropped message %d", msg.kind);
    }
}

/* Runs in the LVGL task. */
static void execute(gg_effects_t fx)
{
    if (fx.enter_standby && !s_stay_awake) {
        ESP_LOGI(TAG, "standby");
        board_sleep_enter(0);
    }
    if (fx.brightness >= 0) {
        board_backlight_set((uint8_t)fx.brightness);
    }
    if (fx.render) {
        ui_render(&s_state);
    }
}

static void pump(lv_timer_t *timer)
{
    (void)timer;
    gg_msg_t msg;
    while (xQueueReceive(s_queue, &msg, 0) == pdTRUE) {
        execute(gg_app_handle(&s_state, &msg));
    }
    const gg_msg_t tick = {.kind = GG_MSG_TICK, .at_ms = now_ms()};
    execute(gg_app_handle(&s_state, &tick));
}

static gg_msg_t time_msg(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (gg_msg_t){.kind = GG_MSG_TIME, .time = {.epoch_s = tv.tv_sec, .valid = s_time_valid}};
}

static gg_msg_t power_msg(void)
{
    uint16_t mv = board_power_vsys_mv();
    return (gg_msg_t){.kind = GG_MSG_POWER,
                      .power = {.usb = mv >= GG_BATTERY_USB_MV, .vsys_mv = mv}};
}

static void on_second(void *arg)
{
    (void)arg;
    app_post(time_msg());
    app_post(power_msg());
}

static void on_button(gg_nav_t nav, void *ctx)
{
    (void)ctx;
    app_post((gg_msg_t){.kind = GG_MSG_NAV, .nav = nav});
}

void app_shell_start(lv_display_t *disp, bool usb, uint64_t wake_buttons)
{
    s_queue = xQueueCreate(QUEUE_LEN, sizeof(gg_msg_t));
    configASSERT(s_queue);

    const gg_app_config_t cfg = GG_APP_CONFIG_DEFAULT;
    gg_app_init(&s_state, &cfg, usb, now_ms());
    gg_msg_t msg = time_msg();
    msg.at_ms = now_ms();
    gg_app_handle(&s_state, &msg);
    msg = power_msg();
    msg.power.usb = usb; /* the boot-time GPIO15 LOW test is the reliable one */
    msg.at_ms = now_ms();
    gg_app_handle(&s_state, &msg);

    /* First frame before DISPON, backlight last: the panel still holds the
     * previous frame in its RAM (ADR-0006). */
    board_display_lock();
    ui_init(disp, &(ui_config_t){.version = esp_app_get_description()->version});
    ui_render(&s_state);
    lv_refr_now(disp);
    ESP_ERROR_CHECK(board_display_on());
    lv_timer_create(pump, PUMP_PERIOD_MS, NULL);
    board_display_unlock();
    board_backlight_set(gg_app_brightness(&s_state));

    ESP_ERROR_CHECK(board_buttons_init(wake_buttons, on_button, NULL));
    const esp_timer_create_args_t args = {.callback = on_second, .name = "second"};
    ESP_ERROR_CHECK(esp_timer_create(&args, &s_second_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_second_timer, SECOND_US));
}

void app_shell_stay_awake(bool on)
{
    s_stay_awake = on;
}

void app_shell_set_time(int64_t epoch_s)
{
    const struct timeval tv = {.tv_sec = (time_t)epoch_s};
    settimeofday(&tv, NULL);
    s_time_valid = true;
    app_post(time_msg());
}

void app_shell_print_state(void)
{
    /* The LVGL lock also guards s_state: only the pump (inside it) writes it. */
    board_display_lock();
    const gg_state_t st = s_state;
    board_display_unlock();

    static const char *const k_screens[GG_SCREEN_COUNT] = {"watchface", "menu", "sysinfo", "test"};
    printf("mode       %s%s\n", st.mode == GG_MODE_ALWAYS_ON ? "always-on" : "glance",
           s_stay_awake ? " (power.stay on)" : "");
    printf("screen     %s\n", st.screen < GG_SCREEN_COUNT ? k_screens[st.screen] : "?");
    printf("power      %s, VSYS %u mV", st.status.usb ? "USB" : "battery", st.status.vsys_mv);
    if (!st.status.usb && st.status.battery_known) {
        printf(", %u%%", st.status.battery_pct);
    }
    printf("\n");
    if (st.clock.valid) {
        const gg_civil_t *c = &st.clock.now;
        printf("time       %04d-%02u-%02u %02u:%02u:%02u (UTC+7)\n", c->year, c->month, c->day,
               c->hour, c->minute, c->second);
    } else {
        printf("time       unknown (never synced)\n");
    }
}
