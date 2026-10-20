/*
 * Civil (Gregorian) date and time from epoch seconds at a fixed UTC offset.
 * Core does this itself instead of using TZ strings so that device and host
 * give identical results (ADR-0011).
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int16_t year;
    uint8_t month;   /* 1-12 */
    uint8_t day;     /* 1-31 */
    uint8_t weekday; /* 0 = Sunday ... 6 = Saturday */
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} gg_civil_t;

/* Days since 1970-01-01 for a proleptic Gregorian date. */
int32_t gg_days_from_civil(int32_t year, uint32_t month, uint32_t day);

void gg_civil_from_epoch(int64_t epoch_s, int32_t utc_offset_s, gg_civil_t *out);

bool gg_is_leap_year(int32_t year);

uint8_t gg_days_in_month(int32_t year, uint8_t month);
