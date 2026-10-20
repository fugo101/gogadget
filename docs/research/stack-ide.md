# Stack fact-check: IDE (CLion vs VS Code)

Date: 2026-10-20

Method: multi-agent research workflow: an adversarial fact-checker re-checked claims from the earlier single-agent reports against primary sources and tried to refute each one

Status: verified: 1 confirmed / 1 partially / 0 refuted

The claims below came from the early report ide-tooling.md. Verdicts: **confirmed** (primary evidence found), **partially** (with the precise correction), **refuted**, or **unverifiable**.

## Summary

| Claim | Topic | Verdict |
|---|---|---|
| C1 | CLion licence and ESP-IDF support | partially |
| C2 | VS Code ESP-IDF extension v2.3.0 | confirmed |

## C1. CLion licence and ESP-IDF support

**Claim:** CLion is free for non-commercial use (since May 2025); it has no dedicated ESP-IDF plugin: ESP-IDF projects are opened as CMake projects with the IDF environment loaded as a toolchain; JetBrains' ESP-IDF help page is outdated and does not mention EIM. CLion cannot open two independent CMake projects (firmware + simulator) in one window (CPP-1537).

**Verdict:** partially

**Correction:** CLion has been free for non-commercial use since v2025.1.1 (announced 2025-05-07), and that is still the case in 2026.2; anonymous usage statistics cannot be turned off.

Neither JetBrains nor Espressif maintains an ESP-IDF plugin. Espressif's idf-clion-plugin stopped at a 2023 beta.

A third-party plugin does exist: the unofficial, single-maintainer 'ESP-IDF' plugin on JetBrains Marketplace (org.btik.idf-for-clion, v0.9.1, 2026-10-07). Each build works with only one CLion major version, currently 2026.2.x. It adds a project wizard, a Menuconfig GUI, OpenOCD debugging and MCP tools.

CLion itself also has a built-in ESP-IDF debug server template (since 2025.2), which became an ESP-IDF debug profile in 2026.2.

Without that plugin, the documented workflow is still a plain CMake project with the IDF environment script set as the toolchain environment.

The JetBrains help page is outdated ('will be updated soon'; CPP-41862 is open) and does not mention EIM.

CPP-1537 (several projects in one window) is still open. JetBrains started pre-production work on it in July 2026, but it is not on the roadmap through late 2026.

**Evidence:** FREE FOR NON-COMMERCIAL USE: confirmed. The JetBrains blog post 'CLion Is Now Free for Non-Commercial Use' is dated 2025-05-07 (blog.jetbrains.com/clion/2025/05/clion-is-now-free-for-non-commercial-use/). The CLion 2025.2 release post says 'Starting from v2025.1.1, the IDE is free for non-commercial use'. The CLion 2026.2 Register help page still offers 'Free non-commercial use'. Terms: a one-year license that auto-renews only if the IDE was used in the last 6 months, and anonymous usage statistics cannot be turned off.

CMAKE PROJECT + IDF ENVIRONMENT AS TOOLCHAIN: confirmed. The help page and the tutorial it links both use Toolchain > Add environment > From file > export.sh.

'NO DEDICATED ESP-IDF PLUGIN': inaccurate as stated.

(a) An unofficial third-party plugin named 'ESP-IDF' is on JetBrains Marketplace. Marketplace API, plugin 23886: xmlId org.btik.idf-for-clion, unverified individual vendor, MIT license, source at github.com/yunyizhi/ESP-IDF-for-Clion, 19,470 downloads. Releases run from 0.1-beta (2024-03-02) to 0.9.1 (2026-10-07). 0.9.1 works only with CLion 2026.2 to 2026.2.3.1 (since/until 262.*). Its docs call it '非官方' (unofficial). Features: new-project wizard, Menuconfig GUI, idf task tree, OpenOCD debugging, and MCP tools (added in 0.9).

(b) Espressif's own github.com/espressif/idf-clion-plugin never got past one pre-release, v0.1.0-beta (2023-07-25), offering only OpenOCD debugging. Its last commit was 2023-07-17.

(c) CLion itself has ESP-IDF-specific support. 2025.2 added an 'ESP32 debug server designed for ESP-IDF projects' (an ESP-IDF template). In 2026.2 this became an 'ESP-IDF' debug profile in the new Debug Profiles page (debug-profiles.html).

HELP PAGE OUTDATED, NO EIM MENTION: confirmed. jetbrains.com/help/clion/esp-idf.html is labelled 'CLion 2026.2 Help' and its footer date is 09 Sep 2026. It opens with 'This article will be updated soon. In the meantime, follow the Working with ESP-IDF in CLion tutorial'. That tutorial is developer.espressif.com/blog/clion/, dated 2025-03-13; it uses export.sh and the pre-2026.2 Debug Servers UI. The page body still has: git clone into $HOME/esp/esp-idf, ./install.sh, export.sh, and Windows example paths using idf5.0_py3.8_env. EIM is never mentioned. JetBrains tracks the problem as YouTrack CPP-41862 'Actualize ESP-IDF tutorial', Open since 2024-10-31.

CPP-1537: confirmed. YouTrack API data: title 'Support multiple projects in the same frame', State Open, Type Feature, created 2014-11-06, updated 2026-10-08, 365 votes, no fix version. On 2026-07-20 a JetBrains employee wrote: 'we're starting pre-production work on this task: defining the scope and what the feature design might look like' (with a survey link). The roadmap published 2026-08-27, 'What's Coming Between Now and Late 2026', has no multi-project item. CLion's CMake docs describe 'Unload CMake Project' for when CLion loaded one CMakeLists.txt 'but you want to use another one'.

## C2. VS Code ESP-IDF extension v2.3.0

**Claim:** Espressif's VS Code ESP-IDF extension v2.3.0 was released 2026-09-23; it installs via EIM and offers a GUI menuconfig, OpenOCD/USB-JTAG debugging, a component registry UI and core-dump analysis.

**Verdict:** confirmed

**Evidence:** RELEASE DATE: The GitHub release v2.3.0 of espressif/vscode-esp-idf-extension was published 2026-09-23T04:43:29Z (not a pre-release). Open VSX shows 2.3.0 at 2026-09-23T04:39:25Z, and the VS Code Marketplace lastUpdated is 2026-09-23T04:49:35Z. As of 2026-10-20 there is no newer tag or release. One small mismatch: CHANGELOG.md dates 2.3.0 as 2026-09-18 (likely when the release was prepared), not 2026-09-23.

EIM: The v2.0.2 release notes (2026-02-04) say the 'ESP-IDF: Configure ESP-IDF extension' command and the idf.espIdfPath and idf.toolsPath settings 'are removed in favor of using our new Espressif Installation Manager (EIM)'. The install docs (installation.html) say the 'ESP-IDF: Open ESP-IDF Installation Manager' command will 'download and run the ESP-IDF Installation Manager to install the ESP-IDF framework'. On SSH, WSL, containers and other headless setups it runs 'eim wizard' instead. Installed versions are detected automatically from $HOME/.espressif/tools/eim_idf.json. Manual setups through idf.customExtraVars are still supported.

FEATURES: package.json at tag v2.3.0 (engines vscode ^1.101.0) contains:
- espIdf.menuconfig.start, 'SDK Configuration Editor (Menuconfig)', described in the docs as a UI 'equivalent to idf.py menuconfig'
- esp.component-manager.ui.show, 'Show ESP Component Registry' (browse, Install button, create project from example)
- espIdf.launchWSServerAndMonitor, 'Launch IDF Monitor for Core Dump Mode/GDB Stub Mode'. debugproject.html calls this post-mortem debugging: it 'can trigger the start of a debug session for ... GDBStub or ESP-IDF Core Dump'.
- espIdf.openOCDCommand ('OpenOCD Manager'), espIdf.jtag_flash, and the gdbtarget debugger. The docs describe debugging as OpenOCD plus the Eclipse CDT GDB Adapter.

The extension's board picker (src/espIdf/openOcd/boardConfiguration.ts) reads openocd-esp32's tcl/esp-config.json. That file lists 'ESP32-S3 chip (via builtin USB-JTAG)' mapped to board/esp32s3-builtin.cfg.

## Fact-checker's notes

Related facts the decision needs that the claims leave out:

1) Firmware and simulator in one window. The VS Code extension officially supports multi-root workspaces (docs page additionalfeatures/multiple-projects.html), so both projects can share one window in VS Code. Version 2.3.0 has a bug here: issue #1962, where the selected project is lost when VS Code reloads. The reporter confirmed a fix on 2026-10-13, but no release containing it exists yet.

2) Possible CLion workaround, not tested. CLion 2026.1 added code insight for CMake ExternalProject_Add() subprojects, loading them 'as part of the primary CMake project'. A superbuild might therefore get around CPP-1537. Nobody has confirmed this works with an ESP-IDF project plus a host simulator. A third-party plugin, 'CMake Mux', only switches between top-level CMakeLists files.

3) The VS Code extension has changed a lot in 2026, which matters for the stability goal:
- v2.0.2: the setup wizard, the idf.espIdfPath/idf.toolsPath settings and the old debug adapter were removed.
- v2.2.0 (2026-08-14): the Project Configuration Editor was removed; CMakePresets.json replaces it.
- v2.3.0: a major refactor, with tasks now run through node-pty.

Open issues filed after 2.3.0:
- #1954: on macOS with IDF 6.1, IntelliSense and clangd report #include errors. The maintainer advises relying on compile_commands.json and clangd.
- #1965: the activate_idf_v6.1.sh script that EIM writes points to 'python' while the extension uses 'python3'. Reported on Linux.

No 2.3.x patch release yet.

4) EIM's own FAQ says its automatic IDE integration through eim_idf.json targets only VS Code and Espressif-IDE. In CLion you would have to set EIM's $HOME/.espressif/tools/activate_idf_v6.1.sh as the toolchain environment yourself; JetBrains does not document this.

5) CLion 2026.2 replaced the Debug Servers settings with Debug Profiles, so the debugging steps in the March 2025 tutorial are stale. 2026.2 also removed the CLion Classic engine.

6) ESP-IDF v6.1 was released 2026-08-27 (rc1 on 2026-08-18). As of 2026-10-20 there is no v6.1.x bugfix release; the newest 6.0 patch is v6.0.3 (2026-09-02). This matters given the earlier failure from adopting 6.0 right after release.

7) For AI-agent work: v2.3.0 added MCP servers for the Component Registry and the documentation, plus an 'Ask AI to fix' action when a task fails. v2.0.2 added a debug image viewer that can show LVGL image arrays.

8) Process note: the browser tool automatically wrote 4 log files into a folder it created in the working directory. I deleted those files and the folder. No other files were created or changed.
