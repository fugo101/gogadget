/*
 * Interactive viewer: the real core + ui in an SDL window, driven by the
 * keyboard. Right/Down/N = Next (KEY press), Left/Esc/B = Back (KEY hold),
 * Enter/Space = Enter (BOOT press), U = toggle USB power, Q = quit.
 * Run with --usb to start in Always-on.
 */
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "gg/gg_app.h"
#include "lvgl.h"
#include "ui/ui.h"

static gg_state_t s_state;
static bool s_standby;
static bool s_quit;
static lv_obj_t *s_blank;

static uint32_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
}

static void apply(gg_effects_t fx)
{
    if (fx.brightness >= 0) {
        printf("[sim] brightness -> %d/16\n", fx.brightness);
    }
    if (fx.enter_standby) {
        printf("[sim] standby (any key wakes; the wake press is consumed)\n");
        s_standby = true;
        lv_obj_set_hidden(s_blank, false);
        return;
    }
    if (fx.render) {
        ui_render(&s_state);
    }
}

static void post(gg_msg_t m)
{
    m.at_ms = now_ms();
    apply(gg_app_handle(&s_state, &m));
}

static void on_key(lv_event_t *e)
{
    uint32_t key = lv_event_get_key(e);
    if (key == 'q' || key == 'Q') {
        s_quit = true;
        return;
    }
    if (s_standby) {
        s_standby = false;
        lv_obj_set_hidden(s_blank, true);
        gg_app_init(&s_state, &s_state.cfg, s_state.status.usb, now_ms());
        post((gg_msg_t){.kind = GG_MSG_TIME, .time = {.epoch_s = time(NULL), .valid = true}});
        ui_render(&s_state);
        return;
    }
    gg_nav_t nav = GG_NAV_NONE;
    switch (key) {
    case LV_KEY_RIGHT:
    case LV_KEY_DOWN:
    case 'n':
        nav = GG_NAV_NEXT;
        break;
    case LV_KEY_LEFT:
    case LV_KEY_ESC:
    case 'b':
        nav = GG_NAV_BACK;
        break;
    case LV_KEY_ENTER:
    case ' ':
        nav = GG_NAV_ENTER;
        break;
    case 'u':
        post((gg_msg_t){
            .kind = GG_MSG_POWER,
            .power = {.usb = !s_state.status.usb, .vsys_mv = s_state.status.usb ? 3950 : 4610}});
        return;
    default:
        return;
    }
    post((gg_msg_t){.kind = GG_MSG_NAV, .nav = nav});
}

int main(int argc, char **argv)
{
    bool usb = argc > 1 && strcmp(argv[1], "--usb") == 0;

    lv_init();
    lv_tick_set_cb(now_ms);
    lv_display_t *disp = lv_sdl_window_create(UI_HOR_RES, UI_VER_RES);
    lv_sdl_window_set_zoom(disp, 3);
    lv_sdl_window_set_title(disp, "Gogadget (sim)");

    /* A hidden focusable object receives every key from the SDL keyboard. */
    lv_indev_t *kb = lv_sdl_keyboard_create();
    lv_group_t *group = lv_group_create();
    lv_obj_t *sink = lv_obj_create(lv_layer_top());
    lv_obj_set_hidden(sink, true);
    lv_obj_add_event_cb(sink, on_key, LV_EVENT_KEY, NULL);
    lv_group_add_obj(group, sink);
    lv_indev_set_group(kb, group);

    s_blank = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(s_blank);
    lv_obj_set_size(s_blank, UI_HOR_RES, UI_VER_RES);
    lv_obj_set_style_bg_color(s_blank, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_blank, LV_OPA_COVER, 0);
    lv_obj_set_hidden(s_blank, true);

    ui_init(disp, &(ui_config_t){.version = "sim"});
    gg_app_config_t cfg = GG_APP_CONFIG_DEFAULT;
    gg_app_init(&s_state, &cfg, usb, now_ms());
    post((gg_msg_t){.kind = GG_MSG_POWER, .power = {.usb = usb, .vsys_mv = usb ? 4610 : 3950}});
    post((gg_msg_t){.kind = GG_MSG_TIME, .time = {.epoch_s = time(NULL), .valid = true}});
    ui_render(&s_state);

    time_t last = 0;
    while (!s_quit) {
        uint32_t wait = lv_timer_handler();
        if (!s_standby) {
            time_t t = time(NULL);
            if (t != last) {
                last = t;
                post((gg_msg_t){.kind = GG_MSG_TIME, .time = {.epoch_s = t, .valid = true}});
            }
            post((gg_msg_t){.kind = GG_MSG_TICK});
        }
        lv_delay_ms(wait < 20 ? wait : 20);
    }
    return 0;
}
