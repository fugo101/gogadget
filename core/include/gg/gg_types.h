/* Small types shared by core, ui and the shells. */
#pragma once

#include <stdint.h>

/* Navigation actions (GLOSSARY: Next / Back / Enter). */
typedef enum {
    GG_NAV_NONE = 0,
    GG_NAV_NEXT,
    GG_NAV_BACK,
    GG_NAV_ENTER,
} gg_nav_t;

/* Vietnam has no DST; core converts epoch seconds to local time itself (ADR-0011). */
#define GG_UTC_OFFSET_S (7 * 3600)

#define GG_ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))
