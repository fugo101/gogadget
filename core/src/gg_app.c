#include "gg/gg_app.h"

#include <string.h>

#include "gg/gg_battery.h"

/* Always-on pixel shift pattern: small enough to be invisible, wide enough to
 * keep static edges from sitting on the same pixels for hours. */
static const int8_t k_shift[][2] = {{0, 0},  {1, 0},  {1, 1},   {0, 1},
                                    {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}};

void gg_app_init(gg_state_t *s, const gg_app_config_t *cfg, bool usb, uint32_t now_ms)
{
    memset(s, 0, sizeof(*s));
    s->cfg = *cfg;
    s->mode = usb ? GG_MODE_ALWAYS_ON : GG_MODE_GLANCE;
    s->screen = GG_SCREEN_WATCHFACE;
    s->status.usb = usb;
    s->last_activity_ms = now_ms;
    s->last_shift_ms = now_ms;
}

uint8_t gg_app_brightness(const gg_state_t *s)
{
    return s->mode == GG_MODE_ALWAYS_ON ? s->cfg.brightness_usb : s->cfg.brightness_battery;
}

static gg_effects_t no_effects(void)
{
    return (gg_effects_t){.render = false, .enter_standby = false, .brightness = -1};
}

static void open_screen(gg_state_t *s, gg_screen_t screen)
{
    s->screen = screen;
    if (screen == GG_SCREEN_MENU) {
        s->menu_focus = 0;
    }
}

static gg_effects_t handle_nav(gg_state_t *s, gg_nav_t nav, uint32_t at_ms)
{
    gg_effects_t fx = no_effects();
    s->last_activity_ms = at_ms;

    switch (s->screen) {
    case GG_SCREEN_WATCHFACE:
        if (nav == GG_NAV_ENTER) {
            open_screen(s, GG_SCREEN_MENU);
            fx.render = true;
        } else if (nav == GG_NAV_BACK) {
            fx.enter_standby = true;
        }
        break;
    case GG_SCREEN_MENU:
        if (nav == GG_NAV_NEXT) {
            s->menu_focus = (uint8_t)((s->menu_focus + 1) % GG_MENU_COUNT);
            fx.render = true;
        } else if (nav == GG_NAV_ENTER) {
            if (s->menu_focus == GG_MENU_SYSINFO) {
                open_screen(s, GG_SCREEN_SYSINFO);
            }
            fx.render = true;
        } else if (nav == GG_NAV_BACK) {
            open_screen(s, GG_SCREEN_WATCHFACE);
            fx.render = true;
        }
        break;
    case GG_SCREEN_SYSINFO:
        if (nav == GG_NAV_BACK) {
            open_screen(s, GG_SCREEN_MENU);
            fx.render = true;
        }
        break;
    case GG_SCREEN_TESTPATTERN:
        if (nav == GG_NAV_BACK) {
            open_screen(s, GG_SCREEN_WATCHFACE);
            fx.render = true;
        }
        break;
    default:
        break;
    }
    return fx;
}

static gg_effects_t handle_tick(gg_state_t *s, uint32_t at_ms)
{
    gg_effects_t fx = no_effects();
    if (s->mode == GG_MODE_GLANCE) {
        if ((uint32_t)(at_ms - s->last_activity_ms) >= s->cfg.glance_ms) {
            fx.enter_standby = true;
        }
    } else if ((uint32_t)(at_ms - s->last_shift_ms) >= s->cfg.shift_period_ms) {
        s->last_shift_ms = at_ms;
        s->shift_step = (uint8_t)((s->shift_step + 1) % GG_ARRAY_LEN(k_shift));
        s->shift_x = k_shift[s->shift_step][0];
        s->shift_y = k_shift[s->shift_step][1];
        fx.render = true;
    }
    return fx;
}

/* Field-wise comparisons: memcmp would also compare padding bytes. */
static bool civil_equal(const gg_civil_t *a, const gg_civil_t *b)
{
    return a->year == b->year && a->month == b->month && a->day == b->day &&
           a->weekday == b->weekday && a->hour == b->hour && a->minute == b->minute &&
           a->second == b->second;
}

static bool status_equal(const gg_status_t *a, const gg_status_t *b)
{
    return a->usb == b->usb && a->battery_known == b->battery_known &&
           a->battery_pct == b->battery_pct && a->vsys_mv == b->vsys_mv && a->wifi == b->wifi &&
           a->unsynced == b->unsynced;
}

static gg_effects_t handle_time(gg_state_t *s, int64_t epoch_s, bool valid)
{
    gg_effects_t fx = no_effects();
    if (!valid) {
        if (s->clock.valid) {
            s->clock.valid = false;
            fx.render = true;
        }
        return fx;
    }
    gg_civil_t now;
    gg_civil_from_epoch(epoch_s, GG_UTC_OFFSET_S, &now);
    if (!s->clock.valid || !civil_equal(&now, &s->clock.now)) {
        s->clock.valid = true;
        s->clock.now = now;
        fx.render = true;
    }
    return fx;
}

static gg_effects_t handle_power(gg_state_t *s, bool usb, uint16_t vsys_mv, uint32_t at_ms)
{
    gg_effects_t fx = no_effects();
    gg_status_t prev = s->status;

    s->status.usb = usb;
    s->status.vsys_mv = vsys_mv;
    if (!usb && vsys_mv > 0) {
        s->status.battery_known = true;
        s->status.battery_pct = gg_battery_percent(vsys_mv);
    }

    gg_mode_t mode = usb ? GG_MODE_ALWAYS_ON : GG_MODE_GLANCE;
    if (mode != s->mode) {
        s->mode = mode;
        /* Unplugging starts a fresh Glance instead of sleeping at once. */
        s->last_activity_ms = at_ms;
        if (mode == GG_MODE_GLANCE) {
            s->shift_x = s->shift_y = 0;
            s->shift_step = 0;
        }
        fx.brightness = (int8_t)gg_app_brightness(s);
    }

    fx.render = !status_equal(&prev, &s->status) || fx.brightness >= 0;
    return fx;
}

gg_effects_t gg_app_handle(gg_state_t *s, const gg_msg_t *m)
{
    switch (m->kind) {
    case GG_MSG_NAV:
        return handle_nav(s, m->nav, m->at_ms);
    case GG_MSG_TICK:
        return handle_tick(s, m->at_ms);
    case GG_MSG_TIME:
        return handle_time(s, m->time.epoch_s, m->time.valid);
    case GG_MSG_POWER:
        return handle_power(s, m->power.usb, m->power.vsys_mv, m->at_ms);
    case GG_MSG_NET: {
        gg_effects_t fx = no_effects();
        if (s->status.wifi != m->net.wifi || s->status.unsynced != m->net.unsynced) {
            s->status.wifi = m->net.wifi;
            s->status.unsynced = m->net.unsynced;
            fx.render = true;
        }
        return fx;
    }
    case GG_MSG_GOTO: {
        gg_effects_t fx = no_effects();
        if (m->screen < GG_SCREEN_COUNT) {
            open_screen(s, m->screen);
            s->last_activity_ms = m->at_ms;
            fx.render = true;
        }
        return fx;
    }
    default:
        return no_effects();
    }
}
