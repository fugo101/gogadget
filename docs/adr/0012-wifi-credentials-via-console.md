# Enter Wi-Fi credentials only through the USB console; add BLE provisioning later

The repository is public and CI builds firmware from it, so no build may contain credentials. Wi-Fi networks are added at runtime over the USB-Serial-JTAG console (`wifi add <ssid> <password>`, stored as a multi-network list in NVS), fed from a local, gitignored file by a helper script. BLE provisioning comes later through `espressif/network_provisioning` (IDF 6.0 removed `wifi_provisioning`) with NimBLE and Security 2 and a per-session password shown as a QR code, in a dedicated provisioning boot entered by holding KEY at boot, so normal boots never start Bluetooth (about 110 KB of RAM). It works with Espressif's iPhone app; SoftAP provisioning is not pursued because the iOS app cannot join the device's network by itself.

## Consequences

- Console output masks SSIDs and never prints passwords.
- CI runs a secrets check; a tracked `sdkconfig` or credentials file fails the build.
