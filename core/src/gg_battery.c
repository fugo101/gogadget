#include "gg/gg_battery.h"

#include "gg/gg_types.h"

/* Light-load LiPo curve, descending voltage. Refine from the idle-drain logs
 * (docs/hardware/power-budget.md). */
static const struct {
    uint16_t mv;
    uint8_t pct;
} k_curve[] = {
    {4200, 100}, {4100, 90}, {4000, 79}, {3900, 66}, {3800, 52}, {3750, 42},
    {3700, 31},  {3650, 21}, {3600, 13}, {3500, 5},  {3400, 2},  {3300, 0},
};

uint8_t gg_battery_percent(uint16_t vsys_mv)
{
    if (vsys_mv >= k_curve[0].mv) {
        return 100;
    }
    for (unsigned i = 1; i < GG_ARRAY_LEN(k_curve); i++) {
        if (vsys_mv >= k_curve[i].mv) {
            uint32_t span_mv = k_curve[i - 1].mv - k_curve[i].mv;
            uint32_t span_pct = k_curve[i - 1].pct - k_curve[i].pct;
            uint32_t above = vsys_mv - k_curve[i].mv;
            return (uint8_t)(k_curve[i].pct + (above * span_pct + span_mv / 2) / span_mv);
        }
    }
    return 0;
}

bool gg_battery_wifi_allowed(uint16_t vsys_mv)
{
    return vsys_mv >= GG_BATTERY_WIFI_MIN_MV;
}
