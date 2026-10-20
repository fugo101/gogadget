# Accept about 2C charging of the 250 mAh cell

The board's TP4065 charger is fixed by a 2 kΩ resistor (R13) at about 525 mA (425-575 mA across parts), which is about 2.1C for the 250 mAh protected LiPo the owner uses, against the usual 1C limit. The alternatives were a 500-600 mAh cell (about 1C, twice the runtime) or replacing R13 with about 4.3 kΩ (about 245 mA, fine SMD rework); the owner chose to keep the current hardware and accepted the risk. The cell's protection circuit guards against overcharge voltage, over-discharge and short circuits but does not limit charge current, so the first charges must be watched for heat or swelling, and this decision should be revisited if either appears.

## Consequences

- Firmware cannot see the charge state (the charger's status pin drives only the red LED), so it never claims "charging" or "full", only "on USB".
