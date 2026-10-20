/*
 * Dev console over USB-Serial-JTAG (ADR-0016). Started only on USB power: on
 * battery no host can be attached, and the REPL would poll for nothing.
 * Commands are part of the firmware's interface; keep them stable.
 */
#pragma once

void app_console_start(void);
