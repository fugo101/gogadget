#include "gg/gg_input.h"

#include <string.h>

void gg_input_init(gg_input_t *in, const gg_input_config_t *cfg)
{
    memset(in, 0, sizeof(*in));
    in->cfg = *cfg;
}

void gg_input_consume_wake_press(gg_input_t *in, gg_button_t b, uint32_t now_ms)
{
    gg_input_button_t *s = &in->btn[b];
    s->raw = true;
    s->stable = true;
    s->suppressed = true;
    s->hold_fired = false;
    s->raw_since = now_ms;
    s->down_since = now_ms;
}

gg_nav_t gg_input_sample(gg_input_t *in, gg_button_t b, bool pressed, uint32_t now_ms)
{
    gg_input_button_t *s = &in->btn[b];

    if (pressed != s->raw) {
        s->raw = pressed;
        s->raw_since = now_ms;
    }

    if (s->raw != s->stable && (uint32_t)(now_ms - s->raw_since) >= in->cfg.debounce_ms) {
        s->stable = s->raw;
        if (s->stable) {
            s->down_since = now_ms;
            s->hold_fired = false;
            /* BOOT acts on the press itself: it has no hold meaning. */
            if (b == GG_BTN_BOOT && !s->suppressed) {
                return GG_NAV_ENTER;
            }
        } else {
            bool was_suppressed = s->suppressed;
            bool was_hold = s->hold_fired;
            s->suppressed = false;
            s->hold_fired = false;
            if (b == GG_BTN_KEY && !was_suppressed && !was_hold) {
                return GG_NAV_NEXT;
            }
        }
        return GG_NAV_NONE;
    }

    if (b == GG_BTN_KEY && s->stable && !s->suppressed && !s->hold_fired &&
        (uint32_t)(now_ms - s->down_since) >= in->cfg.hold_ms) {
        s->hold_fired = true;
        return GG_NAV_BACK;
    }

    return GG_NAV_NONE;
}

bool gg_input_busy(const gg_input_t *in)
{
    for (int i = 0; i < GG_BTN_COUNT; i++) {
        const gg_input_button_t *s = &in->btn[i];
        if (s->raw || s->stable || s->raw != s->stable) {
            return true;
        }
    }
    return false;
}
