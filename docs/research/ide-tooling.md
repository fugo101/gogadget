# IDE and AI-agent tooling

Date: 2026-10-20

Method: single research agent (web and primary sources; no files written, nothing built)

Status: unverified research notes; key claims were re-checked later in stack-ide.md, stack-claude-code.md and stack-esp-idf.md (A4 corrects the monitor mode's name and release date, A5 the MCP server)

Question: CLion or VS Code for ESP-IDF on macOS (Apple Silicon) when an AI agent (Claude Code CLI) writes most of the code, and how the agent can build, flash, capture logs and decode crashes without interaction.

**Recommendation: use VS Code.** Section 5 gives the reasons.

## 1. CLion (latest 2026.2.3.1, released 2026-10-01; 2026.3 is in EAP)

- **Licence:** free for non-commercial use since 2025.1.1 (announced May 2025), and hobby projects count. It lasts one year and renews automatically only if you used it in the last six months. You cannot turn off the anonymous telemetry [1].
- **ESP-IDF support:** there is no plugin. You open the project as a plain CMake project and load the ESP-IDF environment script as the toolchain (Toolchains → "Add environment → From file").
  - CLion creates `app`, `flash` and `monitor` targets automatically. You set the serial port through the `ESPPORT` variable.
  - menuconfig only runs as `idf.py menuconfig` in the terminal.
  - JetBrains' ESP-IDF help page (2026.2, dated 2026-09-09) still describes the manual setup, says it "will be updated soon", and doesn't mention EIM (Espressif's installer) [2].
- **Debugging:**
  - 2025.2 added an ESP-IDF debug-server template. In 2026.2 (2026-07-16) this became an "ESP-IDF" debug profile, and the docs don't say what it configures [3].
  - Espressif hosts a tutorial written by JetBrains (2025-03-13, M2 Mac, ESP32-S3). It uses Espressif's OpenOCD with `-f board/esp32s3-builtin.cfg` and `xtensa-esp32s3-elf-gdb`. It warns that "Persistent session" can be unstable and that the new terminal can break menuconfig [4].
  - Live watches only worked with J-Link or ST-LINK probes (as of 2025.2).
- **Serial monitor:** the JetBrains "Serial Port Monitor" plugin (updated October 2026) is a raw terminal. It does not decode ESP-IDF crash output.
- **Firmware and simulator in one repo:** the request for several projects in one window (CPP-1537) has been open since 2014 with 365 votes. You need two windows or a wrapper top-level CMakeLists. 2026.1 added code insight for `ExternalProject_Add` subprojects and names ESP-IDF; whether that covers this layout is UNVERIFIED [5].
- **Code insight:** CLion uses its own Nova engine (the old Classic engine was removed from the default install in 2026.2), so there is no clangd to set up. A bug where `sizeof(time_t)` comes out wrong for ESP-IDF on the old engine (CPP-23731) is still open.
- **AI:**
  - CLion has a built-in MCP server (since 2025.2) with a one-click "Auto-Configure" for Claude Code. Its debugger tools let an agent start a session, set breakpoints, step and evaluate expressions.
  - 2026.2 includes a `/clion-debugger` skill for Claude Code. The 2026.2.2 hard-fault skill is for ARM Cortex-M chips only, so it does not apply to the ESP32-S3 [6].
  - Whether Claude can drive these debugger tools through the ESP-IDF/OpenOCD profile is UNVERIFIED.

## 2. VS Code with Espressif's ESP-IDF extension (v2.3.0, 2026-09-23; needs VS Code 1.101 or later)

- **Installer:** EIM replaced the extension's own setup in v2.0.2 (2026-02-04). EIM itself is at v0.20.1 (2026-10-16).
- **Build, flash, monitor:** separate commands for each, flashing over UART, JTAG or DFU, plus a combined "Build App, Flash App and Start Monitor" command (new in v2.3.0).
- **menuconfig:** a graphical "SDK Configuration Editor (Menuconfig)". The classic text menuconfig also opens in an editor tab (since v1.11.0). Since v2.2.0, project configurations are stored as CMakePresets.
- **Debugging:** the extension starts OpenOCD itself and you pick the board config.
  - v2.3.0 loads the build's gdbinit files, which fixes "?? Unknown Source" frames for ROM code in the call stack.
  - There is an LVGL image viewer inside the debugger (since v2.0.2).
  - Crash analysis from core dumps or a GDB stub is built in.
- **Components and workspaces:** a "Show ESP Component Registry" UI, and "Pick a Workspace Folder" for workspaces with several project folders (a multi-folder bug was fixed in v2.3.0) [7][8].
- **AI:** a chat tool (`#espIdfCommands`), an "Ask AI to fix" button on failed tasks, and Espressif's documentation and component-registry MCP servers registered automatically. These all feed VS Code's own Chat (Copilot and similar), not Claude Code; that last point is my inference [7].
- **Simulator:** use CMake Tools (1.25.6) with clangd (0.6.0).
  - The clangd extension runs one server per window; its multi-folder support request (issue #38) has been open since 2020 [9].
  - Workaround: put a `.clangd` file in each subproject that points to its own `compile_commands.json`, and use one `--query-driver` pattern that matches both compilers. The firmware folder can use the extension's "Configure project for ESP-Clang" command.
  - If clangd is set up badly, Claude receives false errors through its diagnostics tool.

## 3. Claude Code integration

- **VS Code extension** (2.1.304, released 2026-10-19, not beta):
  - A native chat panel with inline diffs, accept or reject per change, plan review, checkpoints you can rewind to, and several sessions at once.
  - Your editor selection and @-mentions are shared with Claude.
  - Claude can read the Problems panel errors. The CLI run in VS Code's terminal gets the same diff and diagnostics connection [10].
- **JetBrains plugin "Claude Code [Beta]"** (0.1.14-beta, 2025-12-05, no release since):
  - It runs the CLI in the IDE's terminal. Diffs open in the IDE's diff viewer and your selection is shared.
  - Claude can read inspection results, but doesn't fetch them on its own after making edits [10].
  - CLion's own MCP server and its AI chat (Claude Agent available since 2025.3) come on top of this.
- **Net:** VS Code is better for reviewing changes. CLion is the only one where Claude can control a debugger.

## 4. Running ESP-IDF from the agent's command line

- **Environment:** each Bash call starts a fresh shell. Use `eim run "idf.py build"`, or source the ESP-IDF export script in every call.
- **Build and flash:** both run without interaction. With `idf.py -p PORT flash`, esptool resets the chip into download mode over the native USB port by itself.
- **Capturing logs for N seconds:** esp-idf-monitor 1.10.0 (2026-08-27) added a "Command-Stream Mode". It switches on automatically when stdin is not a terminal, and ESP-IDF v6.0 and v6.1 require this version or newer. Crash decoding still works in this mode [11]. Example:
  `printf 'reset\nsleep 10\nexit\n' | python -m esp_idf_monitor -p /dev/cu.usbmodemXXXX build/app.elf > boot.log`
  - The script can also use `expect --timeout 10 <regex>`, which exits with code 110 on timeout. `--save-log` writes a log file.
  - macOS has no `timeout` command (checked on macOS 27), so use the monitor's `sleep` instead.
  - Piping into `idf.py monitor` should also work because it passes stdin through, but this is UNVERIFIED. ESP-IDF master adds `idf.py monitor --command-file` (2026-09-22).
  - Other options are a small pyserial script, or `dut.expect()` from pytest-embedded-idf 2.9.3. esptool has no monitor function.
- **Decoding crashes:**
  - The monitor decodes ESP32-S3 backtraces automatically. By hand: `xtensa-esp32s3-elf-addr2line -pfiaC -e build/app.elf ADDR…` [12].
  - For core dumps, set `CONFIG_ESP_COREDUMP_TO_FLASH_OR_UART` to flash. After a crash, `idf.py -p PORT coredump-info` prints registers, the call stack and the task list without interaction. `coredump-debug` opens an interactive GDB session instead.
  - For live debugging, run `idf.py openocd` (default config `board/esp32s3-builtin.cfg`) and drive GDB with `gdb -batch`.
- **How the port appears on macOS:**
  - Espressif's docs only say `/dev/cu*`. In practice it is `/dev/cu.usbmodem<number>`; that exact name is UNVERIFIED in Espressif docs.
  - No driver is needed. Use the `cu.*` name: the monitor rewrites `tty.*` to `cu.*` because `tty.*` hangs GDB.
  - In ESP-IDF v6.1 the main console defaults to UART0, and the USB port is a secondary, output-only console that still shows crash output. The T-Display-S3's USB-C goes straight to the chip's own USB, so make USB the main console if you need to type input.
  - The port disappears in deep sleep, or if the app reconfigures GPIO19/20. Recover with BOOT+RST [12][15].
- **Espressif's AI tooling:**
  - **Local MCP server:** `idf.py mcp-server`, for ESP-IDF 6.0 or later, announced 2026-04-28. Install it with `--enable-mcp`.
    - In v6.1 it offers build, set target, flash, create project and clean, plus read-only project config, status and connected-device lists.
    - A serial monitor tool (`monitor_device`) is on master and the release/v6.0 branch (merged 2026-10-12) but not yet in a v6.1 release.
    - It is new, runs only locally, and simply wraps `idf.py`.
    - Add it to Claude Code with `claude mcp add --transport stdio esp-idf-eim -- eim run "idf.py mcp-server"` [13].
  - **Docs MCP server:** `https://mcp.espressif.com/docs` over HTTP, sign-in with GitHub or WeChat, limited to 40 requests an hour and 200 a day.
  - **Component Registry MCP server:** `https://components.espressif.com/mcp`.
  - Espressif's July 2026 AI-agent workshop targets VS Code and Cursor [14].

## 5. Verdict: VS Code

- Espressif's own VS Code extension ships often and covers this project: installer, graphical menuconfig, one-click flash and monitor, USB-JTAG debugging, the component registry and an LVGL image viewer. CLion's ESP-IDF docs are out of date, and its serial monitor and menuconfig are plain terminals.
- Firmware plus simulator fits one VS Code workspace with two folders; CLion needs two windows.
- The Claude Code VS Code extension is out of beta and better for reviewing changes; the JetBrains plugin is still beta.
- The agent works through the command line either way (idf.py, the scripted monitor, optionally MCP). The IDE mainly serves review, flashing and debugging, which is where VS Code with Espressif's extension is strongest.

Keep CLion (free for hobby use) as a second tool only if you want Claude to step through a live debugger with CLion's debugger tools. Test that with the ESP-IDF/OpenOCD profile first (UNVERIFIED).

**Sources**

[1] https://www.jetbrains.com/help/clion/register.html · https://blog.jetbrains.com/clion/2025/05/clion-is-now-free-for-non-commercial-use/

[2] https://www.jetbrains.com/help/clion/esp-idf.html

[3] https://www.jetbrains.com/clion/whatsnew/2025-2/ · https://www.jetbrains.com/help/clion/debug-profiles.html

[4] https://developer.espressif.com/blog/clion/

[5] https://youtrack.jetbrains.com/issue/CPP-1537 · https://youtrack.jetbrains.com/issue/CPP-23731 · https://blog.jetbrains.com/clion/2026/03/2026-1-release/

[6] https://blog.jetbrains.com/clion/2026/07/2026-2-release/ · https://blog.jetbrains.com/clion/2026/09/hard-fault-debugging/ · https://www.jetbrains.com/help/clion/mcp-server.html

[7] https://github.com/espressif/vscode-esp-idf-extension/releases · https://developer.espressif.com/blog/2026/10/esp-idf-vscode-extension-v230/ · https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/additionalfeatures/language-tools.html

[8] https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/debugproject.html · https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/configureproject.html

[9] https://github.com/clangd/vscode-clangd/issues/38

[10] https://code.claude.com/docs/en/vs-code · https://code.claude.com/docs/en/jetbrains · https://plugins.jetbrains.com/plugin/27310-claude-code-beta-

[11] https://github.com/espressif/esp-idf-monitor (README "Command-Stream Mode", CHANGELOG v1.10.0) · https://dl.espressif.com/dl/esp-idf/espidf.constraints.v6.1.txt

[12] https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/tools/idf-monitor.html · https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/core_dump.html · https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/usb-serial-jtag-console.html · https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/jtag-debugging/using-debugger.html

[13] https://developer.espressif.com/blog/2026/04/esp-idf-tools-mcp-server/ · https://github.com/espressif/esp-idf/blob/master/tools/idf_py_actions/mcp_ext.py · https://developer.espressif.com/blog/2026/04/doc-mcp-server/

[14] https://developer.espressif.com/workshops/ai-agent-coding-for-esp-idf/

[15] https://github.com/Xinyuan-LilyGO/T-Display-S3

Version numbers and dates come from the GitHub, PyPI, VS Code Marketplace, JetBrains Marketplace and JetBrains release APIs, checked 2026-10-20.
