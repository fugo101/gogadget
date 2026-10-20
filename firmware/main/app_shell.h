/*
 * The imperative shell around core (ADR-0004): one queue of gg_msg_t, drained
 * by an LVGL timer inside the LVGL task, which runs the reducer, renders and
 * carries out the effects. Any task may post.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "gg/gg_app.h"
#include "lvgl.h"

/* Builds the UI, shows the first frame (DISPON, then backlight) and starts the
 * button and clock sources. Call once from app_main after the board is up. */
void app_shell_start(lv_display_t *disp, bool usb, uint64_t wake_buttons);

/* Thread-safe; stamps the message with the current time. */
void app_post(gg_msg_t msg);

/* Development aids for the Dev console (ADR-0016). */
void app_shell_stay_awake(bool on);       /* ignore Standby requests while on */
void app_shell_set_time(int64_t epoch_s); /* set the clock and mark it valid */
void app_shell_print_state(void);         /* print mode, Screen and status */
