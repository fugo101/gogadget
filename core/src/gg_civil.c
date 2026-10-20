#include "gg/gg_civil.h"

/* Howard Hinnant's days_from_civil / civil_from_days algorithms. */

int32_t gg_days_from_civil(int32_t y, uint32_t m, uint32_t d)
{
    y -= m <= 2;
    const int32_t era = (y >= 0 ? y : y - 399) / 400;
    const uint32_t yoe = (uint32_t)(y - era * 400);
    const uint32_t doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    const uint32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (int32_t)doe - 719468;
}

static void civil_from_days(int32_t z, int32_t *y, uint32_t *m, uint32_t *d)
{
    z += 719468;
    const int32_t era = (z >= 0 ? z : z - 146096) / 146097;
    const uint32_t doe = (uint32_t)(z - era * 146097);
    const uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const uint32_t mp = (5 * doy + 2) / 153;
    *d = doy - (153 * mp + 2) / 5 + 1;
    *m = mp < 10 ? mp + 3 : mp - 9;
    *y = (int32_t)yoe + era * 400 + (*m <= 2);
}

void gg_civil_from_epoch(int64_t epoch_s, int32_t utc_offset_s, gg_civil_t *out)
{
    int64_t t = epoch_s + utc_offset_s;
    int64_t days = t / 86400;
    int64_t secs = t % 86400;
    if (secs < 0) {
        secs += 86400;
        days -= 1;
    }

    int32_t y;
    uint32_t m, d;
    civil_from_days((int32_t)days, &y, &m, &d);

    out->year = (int16_t)y;
    out->month = (uint8_t)m;
    out->day = (uint8_t)d;
    /* 1970-01-01 was a Thursday (4). */
    int64_t wd = (days + 4) % 7;
    out->weekday = (uint8_t)(wd < 0 ? wd + 7 : wd);
    out->hour = (uint8_t)(secs / 3600);
    out->minute = (uint8_t)(secs / 60 % 60);
    out->second = (uint8_t)(secs % 60);
}

bool gg_is_leap_year(int32_t y)
{
    return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

uint8_t gg_days_in_month(int32_t y, uint8_t m)
{
    static const uint8_t k_days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m == 2 && gg_is_leap_year(y)) {
        return 29;
    }
    return k_days[m - 1];
}
