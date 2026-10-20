# Wi-Fi provisioning options

Date: 2026-10-20

Method: multi-agent research workflow: one research agent, then an adversarial verifier that re-checked each critical claim and the recommendation against primary sources

Status: verified: 9 confirmed / 1 partially / 0 refuted; the verifier says the recommendation holds

Notes: the settled decisions defer this: v1 takes Wi-Fi credentials only over the USB console (`wifi add`, multi-SSID list in NVS). BLE provisioning as recommended here (espressif/network_provisioning, NimBLE, Security 2, per-session password shown as QR, a dedicated provisioning boot via a GPIO14 long press) comes later, and the SoftAP + captive portal fallback is not pursued.

## Research report

Anything I could not confirm from a primary source is marked **UNVERIFIED**. I did not build or measure anything.

### 1. espressif/network_provisioning (and legacy wifi_provisioning)

- **Version:** latest is **1.3.1, released 2026-10-01**. 1.3.0 was yanked. It requires ESP-IDF ≥5.1.
- **Testing:** CI builds the example on ESP-IDF 5.3, 5.4, 5.5, 6.0, 6.1 and master. It is build-only; there are no tests on real hardware ([registry](https://components.espressif.com/components/espressif/network_provisioning), [CHANGELOG](https://github.com/espressif/idf-extra-components/blob/master/network_provisioning/CHANGELOG.md), [CI](https://github.com/espressif/idf-extra-components/blob/master/.github/workflows/build_and_run_apps.yml), [#428](https://github.com/espressif/idf-extra-components/issues/428)).
- **Legacy component:** **ESP-IDF 6.0 removed `wifi_provisioning`.** 5.5 still ships it with maintenance fixes only. On 5.5, use the new component and add `set(EXCLUDE_COMPONENTS wifi_provisioning)`. ESP-IDF 6 turns Security 0 and 1 off by default ([migration guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/migration-guides/release-6.x/6.0/provisioning.html)).
- **Transports:**
  - BLE on NimBLE or Bluedroid (the official example defaults to NimBLE).
  - SoftAP + HTTP, which needs the Espressif app and is not a captive portal.
  - Console.
- **Security schemes:** 0 = none, 1 = X25519 + AES-CTR + proof of possession (PoP), 2 = SRP6a + AES-GCM.
- **Fixes that touch your earlier problems:**
  - 1.2.1: a reset sent from the phone app now succeeds even if the firmware already reset.
  - 1.2.2: `reset_wifi_sm_state_on_failure()` now also resets the retry counter.
- **Open bugs:**
  - The manager takes over the Wi-Fi station: it forces STA mode, clears the config and disconnects ([#793](https://github.com/espressif/idf-extra-components/issues/793), [#745](https://github.com/espressif/idf-extra-components/issues/745)).
  - An empty scan is reported as success ([#787](https://github.com/espressif/idf-extra-components/issues/787)).
  - SoftAP with Security 1 or 2 fails with the iOS SoftAP app ([#751](https://github.com/espressif/idf-extra-components/issues/751)).
- **Footprint:** Espressif only says NimBLE "requires less heap and flash" ([BLE overview](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/ble/overview.html)). A third party claims about 100 KB less RAM and about 50% less flash than Bluedroid ([esp-nimble-cpp](https://github.com/h2zero/esp-nimble-cpp)). **Numbers for this build: UNVERIFIED.** Measure with `idf.py size-components`. The NimBLE host can allocate its memory from PSRAM.
- **Phone apps:**
  - Android BLE and SoftAP apps were updated **6–7 Oct 2026** for Android 16. The Security 2 username setting is now hidden and defaults to `wifiprov` ([Play](https://play.google.com/store/apps/details?id=com.espressif.provble), [commit](https://github.com/espressif/esp-idf-provisioning-android/commit/e1fae063)).
  - iOS ESP BLE Provisioning is at **3.0.4, released 2025-05-26** ([App Store](https://apps.apple.com/app/id1473590141)).
  - iOS ESP SoftAP Provisioning is at **2.1.0, released 2022-10-08**, so it is stale ([App Store](https://apps.apple.com/app/id1474040630)). The component's top committer, who works at Espressif, says to use the BLE app's transport switch instead (#751).
  - The iOS library has 2026 commits that are not in the App Store yet ([repo](https://github.com/espressif/esp-idf-provisioning-ios)).
- **QR payload:** `{"ver":"v1","name":"PROV_xxxx","username":"wifiprov","pop":"<pwd>","transport":"ble"}`. The apps also read `security`, `password` and `network` ([example](https://github.com/espressif/idf-extra-components/blob/master/network_provisioning/examples/wifi_prov/main/app_main.c), [Android parser](https://github.com/espressif/esp-idf-provisioning-android/blob/master/provisioning/src/main/java/com/espressif/provisioning/ESPProvisionManager.java)).

### 2. Improv Wi-Fi

- **Security:** a simple open spec with **no encryption**. Over BLE the only protection is an optional button-press "authorization" with a timeout ([BLE spec](https://www.improv-wifi.com/ble/), [serial spec](https://www.improv-wifi.com/serial/)).
- **ESP-IDF support:** the only package is `improv/improv` 1.2.7 (2026-08-27). It only encodes and decodes packets and is maintained by ESPHome. You write the USB-Serial-JTAG or NimBLE GATT code yourself ([registry](https://components.espressif.com/components/improv/improv)).
- **Web Serial (needed for Improv serial):** Chrome/Edge desktop, Firefox 151+, Chrome Android 148+ ([caniuse](https://caniuse.com/web-serial), [Chrome 148](https://developer.chrome.com/release-notes/148)).
- **Web Bluetooth (needed for Improv BLE):** Chrome/Edge desktop (Linux needs a flag) and Android Chrome. **iOS Safari supports neither API** ([caniuse](https://caniuse.com/web-bluetooth), [status](https://github.com/WebBluetoothCG/web-bluetooth/blob/main/implementation-status.md)).
- **ESP Web Tools 10.4:** flashes the firmware and then sets up Wi-Fi over Improv serial, with a CDC build variant for the S3's native USB. Its docs say Android is "not implemented yet" ([docs](https://esphome.github.io/esp-web-tools/)).

### 3. SoftAP + captive portal

- **ESP-IDF example:** redirects all DNS to the device, with optional DHCP option 114; about 500 lines, licensed Unlicense/CC0 ([example](https://github.com/espressif/esp-idf/tree/release/v6.0/examples/protocols/http_server/captive_portal)). Option 114 relies on the captive-portal API (RFC 8908), which Apple requires to be served over TLS ([Apple](https://developer.apple.com/news/?id=q78sq5rv)). On a SoftAP that is impractical, so you rely on the DNS redirect.
- **Components:**
  - `78/esp-wifi-connect` 3.3.1: MIT, C++, stores up to 10 SSIDs, about 100k downloads per release ([repo](https://github.com/78/esp-wifi-connect)).
  - `espressif/esp_wifi_service` 0.5.3 from ESP-ADF, first released May 2026: saved-network profiles, captive portal and BluFi. Its license allows use only with Espressif chips, and it pulls in ADF dependencies ([README](https://github.com/espressif/esp-adf/tree/master/components/esp_wifi_service)).
  - `thorrak/esp_wifi_config` 0.4.1: all-in-one, including a Vietnamese web UI, Improv, and BLE through network_provisioning with a reboot after success. Only about 190 downloads ([repo](https://github.com/WiFiConfig/esp_wifi_config)).
- **Phone experience:** the phone must leave its own Wi-Fi and loses internet ([RainMaker](https://docs.rainmaker.espressif.com/docs/dev/firmware/firmware_dev_tips)).
  - When the device runs AP and station together, the station's channel wins ([Wi-Fi guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/wifi-driver/overview.html)). The phone can therefore drop while the credentials are tested, so show the result on the LCD.
  - Android keeping mobile data as the default route is **UNVERIFIED**.
- **Power:**
  - SoftAP keeps the radio on: Wi-Fi receive is 88 mA, transmit peaks 283–340 mA ([datasheet v2.2](https://documentation.espressif.com/esp32-s3_datasheet_en.pdf)).
  - On the BLE path the unconnected station turns its radio off when idle ([power-save guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/wifi-driver/wifi-performance-and-power-save.html)).
  - A 3-minute session costs roughly 3–7 mAh either way (my estimate), which is negligible.

### 4. Fit with the deep-sleep design

- **One-way release:** `esp_bt_mem_release()` cannot be undone in the current boot ([esp_bt.h](https://github.com/espressif/esp-idf/blob/release/v6.0/components/bt/include/esp32c3/include/esp_bt.h)). Re-initialising BT after it panics; Espressif's advice is to keep BLE on or reboot ([ESP-IDF #18065](https://github.com/espressif/esp-idf/issues/18065)).
- **Flow:**
  1. Long press restarts the chip with `esp_restart()` into a boot that only does provisioning.
  2. That boot always ends in deep sleep.
  3. Normal boots never start BT.
- **Effect:** BLE teardown, leaks and the one-way memory release stop mattering.
- **Improv serial:** only useful while `usb_serial_jtag_is_connected()` is true ([header](https://github.com/espressif/esp-idf/blob/release/v6.0/components/esp_driver_usb_serial_jtag/include/driver/usb_serial_jtag.h)).

### 5. Multiple networks

- **What the options store:** none of the official ones keeps more than one network.
  - network_provisioning writes one station config into the Wi-Fi driver's NVS.
  - It passes that config (`wifi_sta_config_t`) to your code in the `NETWORK_PROV_WIFI_CRED_RECV` event ([manager.c](https://github.com/espressif/idf-extra-components/blob/master/network_provisioning/src/manager.c)).
- **How to layer it:** keep your own small NVS list (SSID, password, BSSID, channel, last successful connect).
  - Use `WIFI_STORAGE_RAM` in normal boots.
  - On each sync, try the last network that worked (its BSSID and channel) first, then do one scan.

### 6. Pitfalls to design around

1. **One owner of Wi-Fi.** In the provisioning boot your code must never call `esp_wifi_connect()`; only the manager drives Wi-Fi. Set `wifi_conn_attempts=3`. On `WIFI_CRED_FAIL`, call `network_prov_mgr_reset_wifi_sm_state_on_failure()` so the user can retry.
2. **Timeouts.** The manager has no idle timeout, so add a 5-minute session limit. Running BLE and Wi-Fi together slows joining (one user reports 15–50 s in #793).
3. **Exit point.** Exit on `NETWORK_PROV_END`, not on `CRED_SUCCESS`, so the app can read the final status. Sync time over SNTP on the live connection first; the disconnect-and-wait step is no longer needed.
4. **Per-session password.** Generate a fresh Security 2 password each session with `esp_srp_gen_salt_verifier()`. Its documentation suggests this for devices with a display ([esp_srp.h](https://github.com/espressif/esp-idf/blob/release/v6.0/components/protocomm/include/crypto/srp6a/esp_srp.h)).
   - Keep the username `wifiprov`.
   - Keep the QR payload at or under about 100 bytes so the code fits the 170 px screen height.
5. **Source of truth.** Do not use `network_prov_mgr_is_wifi_provisioned()` to decide whether networks exist; your own list is the source of truth.
6. **iPhone testing.** Test on an iPhone early: the iOS app has not had a release since May 2025.

## Recommendation

Use espressif/network_provisioning (pin ~1.3.1) over BLE with NimBLE and Security 2 as the only provisioning path in v1. Run it only in a dedicated boot entered by a long press, always leave that boot via deep sleep or esp_restart(), and show a QR on the LCD containing a fresh SRP password for each session. This keeps the encrypted path Espressif maintains, which has current apps on Android and iOS and which the user already knows. The deep-sleep design removes the teardown leak, the one-way BT memory release, the Wi-Fi races and the "Wi-Fi still connected" problems from the previous project. Keep your own multi-network list in NVS, filled from NETWORK_PROV_WIFI_CRED_RECV/SUCCESS and managed from on-device Settings, and put provisioning behind a module that does not depend on the transport, so a SoftAP + captive-portal page (no app, Vietnamese UI) can be added later as a fallback. Defer Improv (no encryption, no iOS browser support, ESP Web Tools has no Android support) unless firmware is later shared with others through a web flasher.

## Open questions

- Actual RAM and flash cost of network_provisioning + NimBLE + Wi-Fi + LVGL on this ESP32-S3R8 build. Espressif publishes no figures and I built nothing (UNVERIFIED); measure with idf.py size-components and heap logging in the provisioning boot.
- Whether iOS ESP BLE Provisioning 3.0.4 (last released May 2025) works reliably on current iOS (26.x), and when Espressif will ship the 2026 changes already in the iOS library repo.
- Which phone(s) the user will provision from. An Android-only user would make Improv BLE through Web Bluetooth workable; an iPhone rules it out.
- How the 2026 iOS captive-portal sheet and Android captive-portal handling behave with an ESP SoftAP that has no internet (for example, Android keeping mobile data as the default route). No primary vendor docs found; needs testing on devices.
- Whether ESP Web Tools or the Improv serial JS SDK work from Android Chrome 148+ over USB-OTG to the S3's USB-Serial-JTAG (Chrome added Web Serial on Android, but ESP Web Tools says Android is not implemented) (UNVERIFIED).
- How long on-device SRP salt/verifier generation (esp_srp_gen_salt_verifier, 3072-bit modular exponentiation) takes on the ESP32-S3 before the QR can be shown (expected around a second or less; UNVERIFIED).
- Whether Espressif will fix the station takeover, Wi-Fi mode change and empty-scan issues (#793, #745, #787). They do not matter in a provisioning-only boot, but would if provisioning ever ran alongside normal app Wi-Fi.
- Whether espressif/esp_wifi_service (ESP-ADF, 0.5.x since May 2026, license limited to Espressif chips, ADF dependencies) matures enough to replace a hand-written multi-network store and captive portal; worth re-checking at 1.0.

## Critical claims and verification

### C1: confirmed

**Claim:** espressif/network_provisioning latest is 1.3.1 (published 2026-10-01; 1.3.0 yanked). It declares idf >=5.1, and CI builds its wifi_prov example on IDF release-v5.3, v5.4, v5.5, v6.0, v6.1 and master. Testing is build-only; the issue asking for example tests (#428) is still open.

**Source:** https://components.espressif.com/components/espressif/network_provisioning ; https://github.com/espressif/idf-extra-components/blob/master/network_provisioning/CHANGELOG.md ; https://github.com/espressif/idf-extra-components/blob/master/.github/workflows/build_and_run_apps.yml

**Why it matters:** Shows the component is current and builds on both ESP-IDF 5.5 and 6.x, so the pending IDF version choice does not change the provisioning stack.

**Correction:** No correction needed. Note that 1.3.x replaced cJSON with espressif/json_generator ^2.0.0.

**Evidence:** Registry API (components.espressif.com/api/components/espressif/network_provisioning): 1.3.1 created 2026-10-01T16:47Z with deps idf ">=5.1" and espressif/json_generator ^2.0.0. 1.3.0 was yanked on 2026-10-01 with the message "Depends on yanked json_generator 1.3.0; use 1.3.1". The master idf_component.yml has version 1.3.1. The build_and_run_apps.yml matrix is release-v5.3, v5.4, v5.5, v6.0, v6.1 and latest (master). .build-test-rules.yml enables examples/wifi_prov for IDF >=5.1 on Wi-Fi targets. The network_provisioning tree has no pytest file and no test_app, so it is build-only. #428 ("Add example tests", IEC-222) has been open since 2024-10-16. The scheduled run 39346718823 on 2026-10-20 passed all 30 build jobs, and its logs show wifi_prov/build_esp32s3 built on IDF 5.5 and 6.1.

### C2: confirmed

**Claim:** ESP-IDF v6.0 removed the wifi_provisioning component; network_provisioning replaces it (wifi_prov_* renamed to network_prov_*). Protocomm Security 0 and 1 now default to disabled. On v5.x you add the new component and exclude wifi_provisioning.

**Source:** https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/migration-guides/release-6.x/6.0/provisioning.html

**Why it matters:** Settles which component to use on either IDF version and makes Security 2 the natural default.

**Correction:** No correction. On v5.5, SEC0/SEC1 still default to y, so turn them off by hand.

**Evidence:** The release/v6.0 source docs/en/migration-guides/release-6.x/6.0/provisioning.rst states three things: wifi_provisioning was removed and renamed network_provisioning (with Thread added); v5.x projects should add the manifest and set(EXCLUDE_COMPONENTS wifi_provisioning); and wifi_prov_xx becomes network_prov_xx (with a 5-row table of exceptions). In release/v6.0 the protocomm Kconfig sets SECURITY_VERSION_0 and _1 to default n and _2 to default y. The release/v6.0 components/ directory has no wifi_provisioning, while release/v5.5 still has it. The v6.1 docs (now the 'stable' docs) carry the same page, and the v6.1 migration guide covers only peripherals and tools.

### C3: partially

**Claim:** esp_bt_mem_release() cannot be undone within the current boot. Espressif staff closed a re-init panic report (IDF #18065, ESP32-S3, v5.5.2) by pointing to this, and recommended keeping BLE on or rebooting.

**Source:** https://github.com/espressif/esp-idf/blob/release/v6.0/components/bt/include/esp32c3/include/esp_bt.h ; https://github.com/espressif/esp-idf/issues/18065

**Why it matters:** Justifies running provisioning in its own boot that always ends in esp_restart() or deep sleep, which removes the one-way memory and teardown problems the user hit.

**Correction:** Espressif's only suggestion was CONFIG_ESP_PROTOCOMM_KEEP_BLE_ON_AFTER_BLE_STOP or DISCONNECT_AFTER_BLE_STOP to keep the BLE stack alive, and the reply noted this does not free the BT memory. Rebooting was the reporter's own workaround, not advice from Espressif. Rebooting is still the only way to get the memory back.

**Evidence:** The esp_bt.h note in components/bt/include/esp32c3/include, which ESP32-S3 uses (bt/CMakeLists sets TARGET_SRC_NAME "esp32c3" for S3), says: "Once Bluetooth Controller memory is released, the process cannot be reversed." IDF #18065 (ESP32-S3 rev v0.2, v5.5.2, legacy wifi_prov_mgr + BLE) was closed on 2026-02-02 by rahult-github, an Espressif COLLABORATOR, who pointed to this irreversibility. network_provisioning has the same FREE_BTDM/FREE_BLE handlers, which call esp_bt_mem_release at NETWORK_PROV_DEINIT (src/scheme_ble.c).

### C4: confirmed

**Claim:** network_provisioning has no idle or session timeout. Its only auto-stop timer starts after STA_GOT_IP (CONFIG_NETWORK_PROV_AUTOSTOP_TIMEOUT, default 30 s) and is followed by NETWORK_PROV_END. Connection retries are bounded by network_prov_wifi_conn_cfg_t.wifi_conn_attempts (0 = infinite).

**Source:** https://github.com/espressif/idf-extra-components/blob/master/network_provisioning/Kconfig ; https://github.com/espressif/idf-extra-components/blob/master/network_provisioning/src/manager.c ; https://github.com/espressif/idf-extra-components/blob/master/network_provisioning/include/network_provisioning/manager.h

**Why it matters:** The firmware must enforce its own session timeout for battery life and should exit on NETWORK_PROV_END so the phone app can read the result.

**Correction:** In the normal path, handlers.c calls network_prov_mgr_done() when the phone reads the 'connected' status. NETWORK_PROV_END then follows after the default cleanup delay of about 1 s, so the 30-s timer is only a fallback. When wifi_conn_attempts=0, the code does not retry forever on every error: auth errors and AP-not-found give an immediate CRED_FAIL, and only other disconnect reasons retry indefinitely.

**Evidence:** Kconfig defines NETWORK_PROV_AUTOSTOP_TIMEOUT with default 30, range 5-600, described as applying "after connecting to a network successfully". manager.c has only four timers: autostop (started on IP_EVENT_STA_GOT_IP), cleanup_delay, a 1-s wifi_connect timer, and a 20-s timer used only for Thread. There is no idle, advertising or session timeout. The stop path (prov_stop_and_notify) emits NETWORK_PROV_END. manager.h says of wifi_conn_attempts: "If value 0 same as legacy behavior of infinite connection attempts."

### C5: confirmed

**Claim:** While provisioning, the manager takes full control of the station: it forces STA mode, switches storage to RAM, clears the config and disconnects (open issues #793 and #745). When applying credentials it writes a single STA config to the Wi-Fi driver's NVS and gives the app the wifi_sta_config_t in NETWORK_PROV_WIFI_CRED_RECV.

**Source:** https://github.com/espressif/idf-extra-components/issues/793 ; https://github.com/espressif/idf-extra-components/issues/745 ; https://github.com/espressif/idf-extra-components/blob/master/network_provisioning/src/manager.c

**Why it matters:** Provisioning must not run alongside other Wi-Fi use, and storing several networks has to be built on top using the CRED_RECV event.

**Correction:** The switch to RAM storage is temporary: storage goes back to FLASH before start_provisioning returns. The stop path also forces WIFI_MODE_STA again.

**Evidence:** network_prov_mgr_start_provisioning() calls esp_wifi_set_mode(STA), esp_wifi_start(), esp_wifi_set_storage(WIFI_STORAGE_RAM), sets an empty STA config and calls esp_wifi_disconnect(). configure_wifi_sta() calls set_storage(FLASH) and then esp_wifi_set_config(), which writes one config to the driver's NVS. It emits NETWORK_PROV_SET_WIFI_STA_CONFIG with the full wifi_config_t before applying it, then NETWORK_PROV_WIFI_CRED_RECV with wifi_sta_config_t. #793 (IEC-574) and #745 (IEC-538) are both open. A comment on #793 dated 2026-10-11 reports the same behaviour on 1.3.1 with IDF 6.1 and NimBLE, and a 15-50 s rejoin under BLE coexistence.

### C6: confirmed

**Claim:** protocomm exposes esp_srp_gen_salt_verifier() in a public header. Its documentation says it suits devices that intentionally generate a different password each time and can show it, for example on a display.

**Source:** https://github.com/espressif/esp-idf/blob/release/v6.0/components/protocomm/include/crypto/srp6a/esp_srp.h

**Why it matters:** Allows a fresh Security 2 password per session, shown in the LCD QR code, instead of a password hard-coded in firmware.

**Evidence:** In release/v6.0, components/protocomm/include/crypto/srp6a/esp_srp.h has a note on esp_srp_gen_salt_verifier(): it can be used "for devices which intentionally want to generate different password each time and can send it to the client securely. e.g., a device has a display and it shows the pin". The protocomm CMakeLists lists include/crypto/srp6a as a public include dir. esp_srp.c is compiled only when SECURITY_VERSION_2 is enabled, which is the default.

### C7: confirmed

**Claim:** Phone app status: Android ESP BLE and SoftAP Provisioning updated 6-7 Oct 2026 (Android 16, Sec2 username setting hidden, default wifiprov). iOS ESP BLE Provisioning 3.0.4 released 2025-05-26. iOS ESP SoftAP Provisioning 2.1.0 released 2022-10-08. The component's top committer at Espressif says the SoftAP app 'has not been updated in recent 3 years' and to use the BLE app's transport switch.

**Source:** https://apps.apple.com/app/id1473590141 ; https://apps.apple.com/app/id1474040630 ; https://play.google.com/store/apps/details?id=com.espressif.provble ; https://github.com/espressif/idf-extra-components/issues/751

**Why it matters:** BLE is the only network_provisioning transport with current first-party apps on both platforms; the SoftAP app path is weak on iOS.

**Correction:** The "not updated in 3 years" remark was written in May 2026. It now applies only to iOS, because the Android SoftAP app was updated on 2026-10-07.

**Evidence:** The iTunes lookup API shows ESP BLE Provisioning at 3.0.4, currentVersionReleaseDate 2025-05-26, and ESP SoftAP Provisioning at 2.1.0, 2022-10-08. Both are in the VN storefront and are English-only. On Google Play, com.espressif.provble was updated Oct 6, 2026 and com.espressif.provsoftap Oct 7, 2026. Their 'What's new' reads: "Updated the app for Android 16... Removed the 'Sec2 Username' settings". Android commit e1fae063 (2026-10-05) sets isPreferenceVisible=false with defaultValue "wifiprov", and AppConstants has DEFAULT_USER_NAME_WIFI="wifiprov". The #751 comment by wqx6 was checked word for word. wqx6 (Wang Qixiang) lists Espressif as employer and has 11 commits to network_provisioning, more than anyone else. Separately, Espressif's adwait-esp on IDF #17726 recommends the iOS BLE app because it also supports SoftAP.

### C8: confirmed

**Claim:** Improv specs define no encryption; BLE only has optional physical authorization with a timeout. Neither Web Bluetooth nor Web Serial works in iOS Safari. ESP Web Tools says Android support 'has not been implemented yet'.

**Source:** https://www.improv-wifi.com/ble/ ; https://caniuse.com/web-bluetooth ; https://caniuse.com/web-serial ; https://esphome.github.io/esp-web-tools/

**Why it matters:** Rules Improv out as the main path for a battery device that may be set up from an iPhone in the field.

**Correction:** Not a correction, but for context: Chrome 148 release notes list Web Serial on Android, so ESP Web Tools' Android gap is in the tool, not in the browser.

**Evidence:** improv-wifi.com/ble describes only optional physical authorization ("like pressing a button"), revoked after a timeout (1 min suggested). It has no text on encryption or pairing. caniuse data (Fyrd JSON) marks web-bluetooth and web-serial as 'n' for ios_saf through 27.2 and for Safari through TP; on iOS, Web Bluetooth is available only through a third-party extension. The ESP Web Tools page says: "Android support should be possible but has not been implemented yet." The latest npm release is 10.4.0 (2026-07-15).

### C9: confirmed

**Claim:** When running AP and station together, the channel of the AP the ESP station joins takes priority over the ESP SoftAP channel.

**Source:** https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/wifi-driver/overview.html

**Why it matters:** This is the main UX failure in a SoftAP captive portal: the phone can be dropped while credentials are tested, so the result must be shown on the device LCD.

**Correction:** The same doc says the AP announces the move with a CSA, and stations that support it "transit without disconnecting". The phone being dropped is therefore possible but not certain. manager.c already delays esp_wifi_connect by 1 s so the response reaches the client "before a channel change happens".

**Evidence:** In the release/v6.0 Wi-Fi driver overview.rst: "the channel of the external AP, which the ESP station is connected to, has higher priority over the ESP AP channel". The home-channel section says the AP moves to the station's channel.

### C10: confirmed

**Claim:** ESP32-S3 datasheet v2.2: Wi-Fi receive 88 mA, transmit peaks 283-340 mA, BLE receive 93 mA, BLE transmit 176 mA at 0 dBm. ESP-IDF can switch off the RF of a disconnected station when idle; this is forced on when Wi-Fi and BT run together. No such sleep is documented for SoftAP mode.

**Source:** https://documentation.espressif.com/esp32-s3_datasheet_en.pdf ; https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/wifi-driver/wifi-performance-and-power-save.html

**Why it matters:** Sets the power budget: BLE provisioning draws less on average than SoftAP, but either costs only a few mAh per short session if a hard timeout is enforced.

**Correction:** No correction needed. BLE's peak RX current is not lower than Wi-Fi's; it saves power only through duty-cycling. The figure of a few mAh per session is an estimate and remains UNVERIFIED.

**Evidence:** ESP32-S3 datasheet v2.2, Table 5-7: TX peaks of 340 mA (11b @21 dBm), 291 mA (11g), 283 mA (HT20) and 286 mA (HT40); RX of 88 mA (HT20) and 91 mA (HT40). Table 5-8: BLE TX 176 mA @0 dBm (335 mA @21 dBm), BLE RX 93 mA. In wifi-performance-and-power-save.rst, 'Disconnected State Sleep' (station mode only) turns RF/PHY/BB off when idle through CONFIG_ESP_WIFI_STA_DISCONNECTED_PM_ENABLE, which defaults to y. The esp_coex Kconfig makes ESP_COEX_SW_COEXIST_ENABLE select it. The 'AP Sleep' section documents no AP sleep.

## Verifier's assessment of the recommendation

The recommendation holds. Every factual pillar checks out: the component is current, it builds on IDF 5.5, 6.1 and master as of today, Security 2 is the default on v6, and the BLE apps are the maintained path while the iOS SoftAP app is stale. The esp_bt_mem_release limit is real, and the manager has no idle timeout. It needs these amendments:
1. If IDF 6.x is chosen, require v6.0.3 or v6.1. On v6.0.0–6.0.2, protocomm Security 2 double-freed on any decrypt failure (IDF #18792, fixed by commit beed5fe, which is in v6.0.3 and v6.1 only). The v5.5 mbedtls code path does not free the buffer on error. On v5.5, disable SEC0/SEC1 by hand and exclude wifi_provisioning.
2. Generate the per-session password, and the salt (esp_fill_random), only once RF is on (an explicit esp_wifi_start() before start_provisioning) or between bootloader_random_enable() and bootloader_random_disable(). The ESP-IDF RNG docs only guarantee true random numbers under those conditions.
3. "Leave via deep sleep" still requires an orderly radio stop first. The sleep-modes docs say the app must call nimble_port_stop/deinit and esp_wifi_stop before deep sleep. esp_restart() resets Wi-Fi/BT and runs the esp_wifi_stop shutdown handler, so it is the simplest exit. The design removes re-initialisation, not teardown, which still runs once per boot.
4. Set CONFIG_BT_NIMBLE_ENABLED=y explicitly, because the default BT host in v6.1 Kconfig is still Bluedroid.
5. Exiting on NETWORK_PROV_END is right. It normally fires about 1 s after the phone reads success, so keep the firmware's own hard session timeout.
6. Test on an iPhone early, since the iOS app has not shipped since 2025-05.

## What the research missed (verifier)

1. A Security 2 crash bug in the IDF version choice (IDF #18792): on v6.0.0–6.0.2, a stale, replayed or malformed packet during a Sec2 session corrupts the heap. The fix is in v6.0.3 (2026-09-02) and v6.1 (GA 2026-08-27, now the 'stable' docs). Also unmentioned: v5.5.6 shipped on 2026-10-16, and the 6.1 migration guide has no provisioning changes, so 6.0 and 6.1 behave the same here.
2. Randomness for the per-session password: entropy is only guaranteed with RF or SAR-ADC entropy enabled. network_prov_mgr_start_provisioning() needs the salt and verifier as input, so start Wi-Fi first or use bootloader_random_enable().
3. Deep-sleep exit: per the ESP-IDF sleep_modes docs, Wi-Fi and BT must be disabled before esp_deep_sleep_start(). esp_restart() avoids this, because its doc says Wi-Fi and BT are reset.
4. Normal boots still carry BT's static RAM because BT is in the same image. Calling esp_bt_mem_release(ESP_BT_MODE_BTDM) early in normal boots is the documented way to return the controller and host BSS/data to the heap. The amount is UNVERIFIED; measure it.
5. Kconfig defaults:
   - BT_HOST defaults to Bluedroid.
   - BT_NIMBLE_MEM_ALLOC_MODE_EXTERNAL (PSRAM) requires SPIRAM_USE_CAPS_ALLOC or SPIRAM_USE_MALLOC.
   - CONFIG_ESP_COEX_SW_COEXIST_ENABLE forces STA disconnected power save on.
6. The blocking event NETWORK_PROV_SET_WIFI_STA_CONFIG lets the firmware adjust wifi_config_t (PMF, auth threshold) before it is applied. Also, the manager also saves a plaintext copy in the Wi-Fi driver's own NVS, next to the firmware's own list; consider NVS encryption.
7. 1.3.x depends on espressif/json_generator ^2.0.0. json_generator 1.3.0 was yanked as a breaking change released as a minor version. Any other dependency pinned to json_generator ^1 will conflict; if so, fall back to 1.2.5.
8. Alternatives the researcher did not assess:
   - Wi-Fi Easy Connect (DPP): ESP32-S3 is supported as an enrollee. The device shows a QR code, and some Android 10+ phones provision from Wi-Fi settings with no app and public-key crypto. It does not work on iOS. It is a natural no-app fallback for a device with a screen.
   - BluFi and ESP-Touch v2 were also not evaluated.
9. Both Espressif apps are English-only; only the device UI will be in Vietnamese.
10. GPIO0 is the BOOT strapping pin. Datasheet v2.2 §3 says it is latched only at Chip Reset, so a long press followed by esp_restart() or a deep-sleep wake is safe. A power-on or brownout reset with the button held, however, enters download mode, so GPIO14 is the safer long-press key.
11. CI status: today's daily run built everything successfully, but on-target esp32s3 jobs for 6.1 and latest failed. Those failures are unrelated, since network_provisioning has no on-target tests.
