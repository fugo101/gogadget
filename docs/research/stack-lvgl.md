# Stack fact-check: LVGL

Date: 2026-10-20

Method: multi-agent research workflow: an adversarial fact-checker re-checked claims from the earlier single-agent reports against primary sources and tried to refute each one

Status: verified: 5 confirmed / 2 partially / 0 refuted

Notes: the settled decisions (docs/adr/) use no LVGL input devices or groups: navigation is core/ state fed by an own ISR + FSM button driver, so note 5's keypad or encoder indev on espressif/button events is not used.

The claims below came from the early report lvgl-ecosystem.md. Verdicts: **confirmed** (primary evidence found), **partially** (with the precise correction), **refuted**, or **unverifiable**.

## Summary

| Claim | Topic | Verdict |
|---|---|---|
| B1 | LVGL 9.6.0 is the last v9 release | confirmed |
| B2 | LVGL test module API | confirmed |
| B3 | lv_port_pc_vscode release/v9.6 on macOS | partially |
| B4 | LVGL Pro Editor Community tier and CLI | confirmed |
| B5 | SquareLine Studio, EEZ Studio, NXP GUI Guider | partially |
| B6 | esp_lvgl_port buttons, gap on rotation, avoid_tearing | confirmed |
| B7 | LVGL translation module | confirmed |

## B1. LVGL 9.6.0 is the last v9 release

**Claim:** LVGL v9.6.0 was released 2026-09-16 and its changelog calls it the last release of the v9 series (a stability release); lvgl/lvgl 9.6.0~1 is on the ESP Component Registry; v10 is in development with no release date.

**Verdict:** confirmed

**Evidence:** GitHub API: release v9.6.0 published 2026-09-16T14:42:43Z. v9.6.0 is still the newest tag. release/v9.6 carries backports up to 8df3de9c (2026-10-07), but there is no v9.6.1. The changelog at the tag (docs/src/changelog/CHANGELOG.mdx) says: "LVGL v9.6.0 is the last release of the v9 series, and it is what we're considering a _stability_ release." The ESP Component Registry API lists lvgl/lvgl 9.6.0 (2026-09-16T14:44Z) and 9.6.0~1 (2026-09-17T15:36Z); neither is yanked. The ~1 revision was uploaded right after commit cf729900 "fix(ppa): restore build on ESP-IDF older than 6.0 (#10729)", which touches P4-only PPA code. On v10: master include/lvgl/lv_version.h is 10.0.0 "dev"; docs/src/changelog/migration-v10.mdx has the callout "LVGL v10.0.0 is currently in development and not yet released."; the GitHub milestone "v10.0.0" is open with due_on=null. policies.mdx only says major releases come "approximately yearly". The master policy table lists v9.6 as supported until 16 Sep 2027.

## B2. LVGL test module API

**Claim:** LVGL's test module is public since v9.3.0 and usable in user projects: LV_USE_TEST, LV_USE_TEST_SCREENSHOT_COMPARE, lv_test_display_create(w,h) (headless), lv_test_indev_create_all(), lv_test_key_hit(), lv_test_fast_forward()/lv_test_wait(), lv_test_screenshot_compare("ref.png") which creates a missing reference and writes *_err.png on mismatch; REF_IMGS_PATH and REF_IMG_TOLERANCE macros. Verify exact names and signatures in the v9.6.0 source.

**Verdict:** confirmed

**Correction:** The names are right; these details refine the claim:
- Since v9.5.0 (it was `bool` in 9.3/9.4) compare returns the enum FAILED=0, PASSED=1, NO_REFERENCE_IMAGE=2. NO_REFERENCE_IMAGE is truthy, so assert `== LV_TEST_SCREENSHOT_RESULT_PASSED`.
- Creating missing references depends on LV_TEST_SCREENSHOT_CREATE_REFERENCE_IMAGE. Set it to 0 in CI so a missing reference cannot pass silently.
- A width/height mismatch returns FAILED without writing `_err.png`.
- Only R, G and B are compared, with a per-channel tolerance, and the compare stops at the first differing pixel.
- REF_IMGS_PATH and REF_IMG_TOLERANCE are undocumented macros inside the .c file. They must reach LVGL's own compile, through lv_conf.h or a -D flag.
- LV_USE_LODEPNG=1 is required (the build stops with #error otherwise).

**Evidence:** The v9.3.0 changelog has "feat(test): make LVGL's test utilities public"; src/others/test first appears in v9.3.0. docs/src/debugging/test.mdx in v9.6.0 says the module can be used in any test framework. Exact v9.6.0 declarations (include/lvgl/debugging/test/; the old src/debugging/test/*.h paths now only emit a deprecation #warning):
- `lv_display_t * lv_test_display_create(int32_t hor_res, int32_t ver_res);` gives an XRGB8888 buffer allocated with malloc, LV_DISPLAY_RENDER_MODE_DIRECT and a dummy flush_cb, so it is headless.
- `void lv_test_indev_create_all(void);` creates a pointer, a keypad and an encoder.
- `void lv_test_key_hit(uint32_t k);`
- `void lv_test_wait(uint32_t ms);`
- `void lv_test_fast_forward(uint32_t ms);`
- `lv_test_screenshot_result_t lv_test_screenshot_compare(const char * fn_ref);` plus `lv_test_screenshot_compare_core()`.

Kconfig: LV_USE_TEST, LV_USE_TEST_SCREENSHOT_COMPARE and LV_TEST_SCREENSHOT_CREATE_REFERENCE_IMAGE (default 1). lv_test_screenshot_compare.c has `#ifndef REF_IMGS_PATH` → "" and `#ifndef REF_IMG_TOLERANCE` → 0. A missing reference is written from the screen and the call returns PASSED. A pixel mismatch writes `<name>_err.png` in REF_IMGS_PATH.

## B3. lv_port_pc_vscode release/v9.6 on macOS

**Claim:** lvgl/lv_port_pc_vscode has a release/v9.6 branch; on macOS it builds with Homebrew cmake/ninja/pkg-config/sdl2 (Homebrew sdl2 is now sdl2-compat over SDL3); a CMake 4.x configure issue (#113) was fixed only on master.

**Verdict:** partially

**Correction:** The #113 part is wrong:
- **Not a CMake 4.x issue.** The error comes from LVGL's new install(EXPORT) rules (added in #10128) combined with the port adding ${PROJECT_SOURCE_DIR} to lvgl's PUBLIC includes. It occurs with any CMake version.
- **The port-side fix never reached the upstream repo.** Commits 722eac3 and d713527e appear in no upstream branch, and the activity log shows only lvgl-bot pushes. Both master (cf17de5d) and release/v9.6 still have `target_include_directories(lvgl PUBLIC ${PROJECT_SOURCE_DIR} ${SDL2_INCLUDE_DIRS})` on line 70. The fix exists only on the maintainer's fork (AndreCostaaa/lv_port_pc_vscode master).
- **Both branches should configure anyway.** LVGL #10080 (merged 2026-09-04, included in v9.6.0) sets `option(LV_BUILD_INSTALL ... ${LVGL_IS_TOP_LEVEL})`, so install/export rules are off under add_subdirectory.

Also relevant: open issue #107 — USE_FREERTOS=ON crashes on macOS (main menu set from a non-main thread).

**Evidence:** release/v9.6 exists: lvgl-bot created it on 2026-09-16; head is 4c9e7e32. It pins the LVGL submodule to release/v9.6 HEAD 8df3de9c (2026-10-07), not the v9.6.0 tag.

Homebrew formula API: `sdl2` is an alias of sdl2-compat 2.32.74, which depends on sdl3 3.4.18; cmake is 4.4.4; pkg-config is an alias of pkgconf. sdl2-compat's SDL2Config.cmake defines SDL2_INCLUDE_DIRS, SDL2_LIBRARIES and SDL2::SDL2, which the port's find_package(SDL2) uses.

The README's macOS line is `brew install sdl2 cmake make`; ninja and pkg-config are optional, and LVGL 9.6 only warns when pkg-config is missing. The repo has no CI, and I did not build it because of the no-files rule, so macOS buildability is plausible but not verified.

Issue #113 was reported with CMake 4.4.2 and closed 2026-08-17 as "fixed in 722eac3".

## B4. LVGL Pro Editor Community tier and CLI

**Claim:** LVGL Pro Editor's Community tier is free with no time limit for personal/open-source use since Editor v2.0.0 (July 2026), but the LVGL Pro CLI (screenshots/tests/codegen) is not available on Community; the XML runtime was removed from open-source LVGL in v9.5.0.

**Verdict:** confirmed

**Evidence:** Editor v2.0.0 (GitHub release, 2026-07-06): "brings the editor to you with no time limit. No feature restrictions… If you're building something personal, open-source, or just evaluating, the full editor is yours. Every feature, no expiry." Pricing page (lvgl.io/pro/pricing) for Community: "For makers, personal and open source projects", Free, License validity "No expiry", Commercial use "Personal use and evaluation only", Online share "For public repos", CLI "Not available" (Growth, Product and Platform do have the CLI). CLI docs (lvgl.io/docs/pro/cli): "The CLI is a Professional feature and is not available with the Community and Evaluation licenses". It needs a Product or Platform token (LVGLPRO_CLI_TOKEN), is installed with `npm i -g @lvgl/lvglpro`, and has the commands generate, compile, validate, run-test, run-all-tests, screenshot and compare. The v9.5.0 changelog under Breaking Changes: "XML Engine Removed. XML UI engine development continues outside the main repository." plus PR #9565 "remove the XML parser and loader". The Pro docs say "the engine that can load XML files at runtime is not publicly available." Current Editor is v2.0.5 (2026-10-14), with v2.1.0-beta0 out; the site's download widget still links v1.2.1. The editor needs sign-in and a license choice.

## B5. SquareLine Studio, EEZ Studio, NXP GUI Guider

**Claim:** SquareLine Studio 1.6.2 supports LVGL only up to 9.5; EEZ Studio 0.29.0 (GPL-3.0) supports LVGL 8.4 and 9.2.2-9.5 and has no CLI code generation; NXP GUI Guider's licence restricts use to NXP products.

**Verdict:** partially

**Correction:** EEZ Studio does have command-line code generation. `eezstudio --build-project <file.eez-project>` (handled in packages/main/main.ts at v0.29.0) opens a hidden window, runs projectStore.build(), prints the build messages and quits. It has been available since 0.13.0, per the maintainer in issue #340. It is an Electron app, so it needs a display session (xvfb on Linux CI). SquareLine also has a command-line export (-projectfile/-exportfolder), which works only after a licence has been activated in the GUI.

**Evidence:** SquareLine changelog: "Version 1.6.2, Release date: 10 August 2026 … Added support for LVGL 9.5" (also full Apple-silicon support). 1.6.2 is still the newest version on the docs and forum; nothing supports 9.6.

EEZ Studio v0.29.0 was released 2026-08-02; the repo licence is GPL-3.0. At v0.29.0 and on master it bundles lvgl_runtime_v8.4.0, v9.2.2, v9.3.0, v9.4.0 and v9.5.0 (no 9.6).

GUI Guider v2.0.0's LICENSE page points to LA_OPT_NXP_Software_License v63. §2.2 grants the licence "solely for the development of an Authorized System" and use "solely in combination with a NXP Product". §1.2 Authorized System covers software "used exclusively in connection with an NXP Product". The current v64.2 (May 2026) has the same wording.

## B6. esp_lvgl_port buttons, gap on rotation, avoid_tearing

**Claim:** esp_lvgl_port's navigation-button input requires exactly three buttons (prev/next/enter); esp_lvgl_port does not re-apply the panel gap (e.g. 35 px offset) when rotating at runtime; avoid_tearing only applies to RGB/DSI panels.

**Verdict:** confirmed

**Evidence:** esp_lvgl_port is at 2.9.0 (2026-08-07; it is the only 2.9.x).
- **Buttons:** in src/lvgl9/esp_lvgl_port_button.c with espressif/button ≥4 (latest is 4.2.2), the port runs `ESP_GOTO_ON_FALSE(buttons_cfg->button_prev && buttons_cfg->button_next && buttons_cfg->button_enter, ESP_ERR_INVALID_ARG, …"Invalid some button handler!")`, so all three are required. The resulting indev is LV_INDEV_TYPE_ENCODER with LEFT, RIGHT and ENTER. Worse, the context comes from malloc, and the error path calls iot_button_delete() on uninitialized btn[]. Passing two buttons can therefore crash instead of returning NULL. On the <4 path, the register loop calls iot_button_register_cb on uninitialized handles under ESP_ERROR_CHECK.
- **Gap:** lvgl_port_disp_rotation_update() only calls esp_lcd_panel_swap_xy() and esp_lcd_panel_mirror(). There is no set_gap call anywhere in the component. In ESP-IDF v6.1, esp_lcd_panel_st7789.c adds x_gap/y_gap in draw_bitmap, and swap_xy only toggles the MADCTL MV bit. The app must call esp_lcd_panel_set_gap itself (for example on LV_EVENT_RESOLUTION_CHANGED), or use sw_rotate, which leaves panel orientation and gap untouched.
- **avoid_tearing:** the flag exists only in lvgl_port_display_rgb_cfg_t and lvgl_port_display_dsi_cfg_t. lvgl_port_add_disp() calls lvgl_port_add_disp_priv(cfg, NULL), giving type OTHER, and the avoid_tearing branch only fetches RGB or DPI frame buffers.

## B7. LVGL translation module

**Claim:** LVGL has a built-in translation module (LV_USE_TRANSLATION, lv_tr(), lv_translation_set_language) since v9.4.0.

**Verdict:** confirmed

**Evidence:** src/others/translation/ is absent at v9.3.0 and present at v9.4.0. The v9.4.0 changelog has "feat(translation): add translation support (#8255)" and "add language changed event (#8882)". lv_conf_template.h has `#define LV_USE_TRANSLATION 0`. The v9.6.0 header include/lvgl/core/lv_translation.h declares `const char * lv_tr(const char * tag)`, `void lv_translation_set_language(const char * lang)`, lv_translation_add_static/add_dynamic, lv_translation_get_language and others, and the event LV_EVENT_TRANSLATION_LANGUAGE_CHANGED. In 9.4, lv_tr was static inline. Version 9.6 added translation support to dropdown and roller.

## Fact-checker's notes

Related facts the decision needs that the claims leave out:

1. **LVGL 9.6 deprecations hit new UI code.** All of these still compile with a #warning or deprecation warning and will be removed in v10 (see docs/src/changelog/migration-v9-6.mdx):
   - the generic flag API: lv_obj_add_flag, remove_flag, set_flag, has_flag. Replacements are lv_obj_set_hidden(), lv_obj_is_hidden() and similar.
   - the widgets lv_list, lv_win, lv_menu and file_explorer.
   - LV_COLOR_DEPTH, replaced by LV_COLOR_FORMAT_DEFAULT.
   - including headers from src/. Public headers now live in include/lvgl/.
   - Most LVGL examples, AI-written code, and output from SquareLine, EEZ and LVGL Pro (all of which target ≤9.5) use these. Write v10-ready C from the start.

2. **esp_lvgl_port 2.9.0 predates LVGL 9.6.**
   - It was released 2026-08-07, and its last commit is from 2026-08-03.
   - Its manifest `lvgl/lvgl >=8,<10` makes the Component Manager pick 9.6.0~1 unless you pin a version.
   - esp-bsp PR #832 (merged 2026-10-05) fixed LVGL 9.6 deprecation warnings in the BSP examples. That suggests esp-bsp CI builds against 9.6, but there is no formal 9.6 statement.
   - The S3 SIMD blend hooks (LV_DRAW_SW_COLOR_BLEND_TO_RGB565 etc.) still exist in 9.6.

3. **Headless-test gotcha:** lv_sdl_window_create() calls lv_tick_set_cb(SDL_GetTicks), and lv_tick_get() prefers that callback. lv_test_wait() and lv_test_fast_forward() advance time through lv_tick_inc, so they stop controlling time once an SDL window exists. Test binaries should use only lv_test_display_create. For RGB565 parity with the device, call lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565); the compare supports that format.

4. **Simulator and device LVGL differ.** The port's release/v9.6 tracks LVGL release/v9.6 HEAD (8df3de9c, bot-updated, post-9.6.0 fixes), while the device would get registry 9.6.0~1. Pin both to the same tag. The bot also regenerates the port's lv_conf.h: on release/v9.6 it has LV_COLOR_FORMAT_DEFAULT RGB565, even though lv_conf.defaults still says LV_COLOR_DEPTH 32 (the deprecated key is ignored).

5. **The board has two buttons, so the esp_lvgl_port nav-button API is unusable.** Write a custom keypad or encoder indev fed by espressif/button events.

6. **LVGL Pro does not cover an unattended agent on the free tier.**
   - Its MCP server (lvgl.mcp.kapa.ai) is a read-only docs Q&A service and cannot take screenshots or validate anything.
   - The lvgl/lvgl_pro XML widget schemas exist only for v9.4.0 and v9.5.0, so Pro does not yet target 9.6.
   - Editor 2.0.2 fixed exported code that never created screens outside the Editor (a stale LV_USE_XML guard).
   - On the Community tier, Pro is therefore only a GUI aid.

7. **ESP-IDF v6.1 has no patch release yet.** It shipped 2026-08-27 with no 6.1.x bugfix so far. The newest 6.0 patch is v6.0.3 (2026-09-02). This matters given the earlier lesson of adopting 6.0 the week it shipped.
