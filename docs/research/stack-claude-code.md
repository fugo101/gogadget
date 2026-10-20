# Stack fact-check: Claude Code IDE integrations

Date: 2026-10-20

Method: multi-agent research workflow: an adversarial fact-checker re-checked claims from the earlier single-agent reports against primary sources and tried to refute each one

Status: verified: 7 confirmed / 2 partially / 0 refuted / 1 unverifiable

The claims below came from the early report ide-tooling.md. Verdicts: **confirmed** (primary evidence found), **partially** (with the precise correction), **refuted**, or **unverifiable**.

The checker split claim C3 into sub-claims C3.1 to C3.9; each has its own verdict below.

## Summary

| Claim | Topic | Verdict |
|---|---|---|
| C3 | Claude Code: JetBrains plugin vs VS Code extension | partially |
| C3.1 | JetBrains plugin is named 'Claude Code [Beta]' | confirmed |
| C3.2 | JetBrains plugin's last release is 0.1.14-beta (2025-12-05) | confirmed |
| C3.3 | VS Code extension is not a pre-release (GA) | confirmed |
| C3.4 | VS Code extension 2.1.304 was published 2026-10-19 | confirmed |
| C3.5 | VS Code extension shows inline diffs | confirmed |
| C3.6 | VS Code extension has per-change accept/reject | confirmed |
| C3.7 | VS Code extension has checkpoints | confirmed |
| C3.8 | Diagnostics sharing is a VS Code advantage | partially |
| C3.9 | JetBrains plugin lacks per-change review and checkpoints | unverifiable |

## C3. Claude Code: JetBrains plugin vs VS Code extension

**Claim:** The Claude Code JetBrains plugin is still marked Beta (last release 0.1.14-beta, 2025-12-05), while the Claude Code VS Code extension is GA (v2.1.304, 2026-10-19) with inline diffs, accept/reject per change, checkpoints and diagnostics sharing. Compare what each IDE integration offers today for a CLI-first workflow.

**Verdict:** partially

**Correction:** Restate as: JetBrains is a Beta, terminal-only wrapper around the standalone CLI with diff viewing, selection sharing and diagnostics sharing. VS Code adds a graphical panel with per-change Accept/Reject buttons (Manual mode), plan comments and a rewind menu. Checkpoints are documented for the CLI and VS Code. Checkpoints and per-change review are undocumented for JetBrains, which is not the same as shown to be missing.

**Evidence:** Core facts hold. The JetBrains plugin (registry id 27310) is named 'Claude Code [Beta]' and its newest version is 0.1.14-beta, dated 2025-12-05. The VS Code extension 2.1.304 was published 2026-10-19 with no pre-release flag. The VS Code docs document inline diff review, per-change accept/reject, checkpoints and diagnostics sharing. The implied contrast is off: the JetBrains plugin also documents an IDE diff viewer and diagnostics sharing, and per-change accept/reject appears only in the Manual-mode review flow.

## C3.1. JetBrains plugin is named 'Claude Code [Beta]'

**Claim:** sub-claim of C3 split out by the fact-checker: JetBrains plugin is named 'Claude Code [Beta]'.

**Verdict:** confirmed

**Evidence:** https://plugins.jetbrains.com/api/plugins/27310 returns name 'Claude Code [Beta]', id 27310, xmlId com.anthropic.code.plugin. The update feed https://plugins.jetbrains.com/plugins/list?pluginId=com.anthropic.code.plugin shows the same name with vendor 'Anthropic, PBC'. https://code.claude.com/docs/en/jetbrains.md tells users to open Settings > Tools > Claude Code [Beta] and links plugins.jetbrains.com/plugin/27310-claude-code-beta-. The Beta label is part of the plugin name and the settings path, not a status heading.

## C3.2. JetBrains plugin's last release is 0.1.14-beta (2025-12-05)

**Claim:** sub-claim of C3 split out by the fact-checker: JetBrains plugin's last release is 0.1.14-beta (2025-12-05).

**Verdict:** confirmed

**Evidence:** The update feed lists 0.1.14-beta as the newest entry, with updatedDate 1764956658000, which is 2025-12-05T17:44:18Z. The updates API https://plugins.jetbrains.com/api/plugins/27310/updates agrees. Earlier releases: 0.1.12-beta on 2025-10-23, 0.1.11-beta on 2025-06-23, 0.1.9-beta on 2025-06-13. The beta, alpha and nightly feeds list no versions. Nothing newer exists as of 2026-10-20, about 10.5 months later. The 0.1.14-beta change note reads 'Fix crash on Windows when loading clickable links'.

## C3.3. VS Code extension is not a pre-release (GA)

**Claim:** sub-claim of C3 split out by the fact-checker: VS Code extension is not a pre-release (GA).

**Verdict:** confirmed

**Evidence:** The VS Code Marketplace gallery API for anthropic.claude-code (https://marketplace.visualstudio.com/items?itemName=anthropic.claude-code) shows displayName 'Claude Code for VS Code', publisher anthropic, flags 'validated, public', and no Microsoft.VisualStudio.Code.PreRelease property on 2.1.304. https://code.claude.com/docs/en/vs-code.md calls the extension 'the recommended way to use Claude Code in VS Code' and never labels it beta or preview. Anthropic publishes no formal GA label, so GA here means not pre-release.

## C3.4. VS Code extension 2.1.304 was published 2026-10-19

**Claim:** sub-claim of C3 split out by the fact-checker: VS Code extension 2.1.304 was published 2026-10-19.

**Verdict:** confirmed

**Evidence:** Marketplace builds of 2.1.304 were published 2026-10-19 between 21:47 and 21:48 UTC, and the extension's lastUpdated is 2026-10-19T21:56:29Z. The CLI release v2.1.304 (https://github.com/anthropics/claude-code/releases/tag/v2.1.304) was published 2026-10-19T21:34:39Z. npm @anthropic-ai/claude-code 2.1.304 was modified 2026-10-19T21:36:43Z. The extension and the CLI share one version number; the extension's 2.1.89 build is dated 2026-04-01.

## C3.5. VS Code extension shows inline diffs

**Claim:** sub-claim of C3 split out by the fact-checker: VS Code extension shows inline diffs.

**Verdict:** confirmed

**Evidence:** The vs-code.md meta description says 'Get AI coding assistance with inline diffs, @-mentions, plan review, and keyboard shortcuts'. The body, under Manual mode, says Claude 'shows a side-by-side comparison of the original and proposed changes, then asks for permission'. So 'inline diffs' is the docs' term for a diff view inside the editor, not inline text annotations.

## C3.6. VS Code extension has per-change accept/reject

**Claim:** sub-claim of C3 split out by the fact-checker: VS Code extension has per-change accept/reject.

**Verdict:** confirmed

**Evidence:** vs-code.md, 'Review changes' step: 'Accept this change' and 'Reject this change' buttons appear under each change in the diff. Per-change review requires Claude Code v2.1.275 or later. Diffs over 100 changes open without per-change buttons. Commands 'Claude Code: Accept Change at Cursor' and 'Claude Code: Reject Change at Cursor' exist. The CLI changelog entry for 2.1.275 confirms '[VSCode] Added accept and reject buttons under each change in the proposed-change diff tab'. Scope: this sits in the Manual-mode flow, because Auto and Edit automatically modes edit most files without asking. The docs do not say whether a terminal session gets these buttons.

## C3.7. VS Code extension has checkpoints

**Claim:** sub-claim of C3 split out by the fact-checker: VS Code extension has checkpoints.

**Verdict:** confirmed

**Evidence:** vs-code.md, 'Rewind with checkpoints': hover a message, then choose Fork conversation from here, Rewind code to here, or Fork conversation and rewind code. The feature table lists Checkpoints as Yes for both CLI and VS Code. https://code.claude.com/docs/en/checkpointing.md: only edits made by Claude's file-editing tools are tracked. Bash-command file changes and external edits are not. Subagent edits are usually not restored. Symlinked and hard-linked files are skipped. The 100 most recent checkpoints are kept, and snapshots are swept about 30 days after the session last saved one unless cleanupPeriodDays is set. feature-availability.md lists Checkpoints among features available on every provider.

## C3.8. Diagnostics sharing is a VS Code advantage

**Claim:** sub-claim of C3 split out by the fact-checker: Diagnostics sharing is a VS Code advantage.

**Verdict:** partially

**Correction:** VS Code diagnostics sharing is real, but JetBrains also has it, so it is not a VS Code-only advantage. The Marketplace text says diagnostics are shared 'in real-time', while jetbrains.md says Claude calls getDiagnostics on demand and does not request diagnostics after edits. Use the docs wording.

**Evidence:** VS Code: the docs say the CLI calls mcp__ide__getDiagnostics, which returns the errors and warnings in the Problems panel. The chat panel reads them through a separate claude-vscode server from v2.1.285. JetBrains: https://code.claude.com/docs/en/jetbrains.md has a 'Diagnostic sharing' item that uses the same getDiagnostics tool, and the plugin description lists 'Diagnostics sharing'.

## C3.9. JetBrains plugin lacks per-change review and checkpoints

**Claim:** sub-claim of C3 split out by the fact-checker: JetBrains plugin lacks per-change review and checkpoints.

**Verdict:** unverifiable

**Evidence:** No JetBrains doc page and no plugin description mentions per-change accept/reject, a rewind UI or checkpoints. The plugin description lists only diff viewing ('reviewing and modifying proposed changes'), selection context, file reference shortcuts and diagnostics sharing. Absence from the docs is not proof, so the implied gap stays unverified. Checkpoints are a CLI feature (/rewind, or Esc twice), so they probably work in a JetBrains terminal, but no doc says so. The JetBrains docs also warn that the IDE terminal can capture Esc.

## Fact-checker's notes

Comparison for a CLI-first workflow, as of 2026-10-20. Sources: https://code.claude.com/docs/en/vs-code.md, https://code.claude.com/docs/en/jetbrains.md, https://code.claude.com/docs/en/checkpointing.md, https://code.claude.com/docs/en/platforms.md, https://code.claude.com/docs/en/feature-availability.md.

VS Code extension (anthropic.claude-code 2.1.304; needs VS Code 1.94.0 or later per the docs and the manifest engine field ^1.94.0): a graphical chat panel by default, or a CLI-style terminal mode via the Use Terminal setting (claudeCode.useTerminal). The panel bundles a private CLI copy, so 'claude' on PATH needs the standalone install. Running 'claude' in the integrated terminal connects it to VS Code for diffs, selection and diagnostics. Documented for the extension only: per-change Accept/Reject buttons (Manual mode, 2.1.275+), /plan opening the plan as Markdown with inline comments (2.1.280+), and a rewind menu with fork, rewind-code and fork-and-rewind options. The CLI /rewind menu adds restore-conversation and summarize options. CLI-only features: the '!' bash shortcut and tab completion; the extension also has a subset of commands. History is shared with the CLI (claude --resume).

JetBrains plugin (27310 'Claude Code [Beta]', vendor Anthropic, PBC; newest 0.1.14-beta): the docs describe only the terminal path. The plugin runs the standalone 'claude' in the IDE's integrated terminal and does not bundle a CLI. Documented: Cmd+Esc quick launch; diffs open in the IDE diff viewer or stay in the terminal (/config Diff tool = auto or terminal); selection sharing; Cmd+Option+K file references; getDiagnostics; /ide from external terminals; Option+Enter multi-line on macOS. Not documented: per-change accept/reject, a rewind UI, plan review, or a graphical panel.

Other facts the claim gets wrong or omits:
1. Checkpoints track only edits made by Claude's file tools. Bash-made changes (rm, mv, cp) and external edits are not rewound, so keep git as the real safety net for firmware work.
2. Snapshots are swept about 30 days after the session last saved one unless cleanupPeriodDays is set. The 100 most recent checkpoints are kept.
3. The JetBrains Marketplace text says diagnostics are shared 'in real-time'. jetbrains.md says Claude calls getDiagnostics on demand. The docs win.
4. CLI changelog entries 2.1.281 and 2.1.301 mention 'the VS Code and JetBrains panels' for a Continue/Stop prompt and a usage banner. No JetBrains doc or plugin description mentions a panel. This is unresolved; the official docs describe a terminal-only integration.
5. Cadence: the VS Code extension number matches the CLI number (2.1.304 for both, 2026-10-19), so it moves with the CLI. The JetBrains plugin is versioned separately (0.1.x-beta). Its last release is about 10.5 months old, while the CLI has shipped many versions since. The 0.1.12-beta notes say some fixes need a newer claude.
6. The two CLI copies (panel-bundled and standalone) should stay on matching versions. After you uninstall the extension, running 'claude' in VS Code's integrated terminal reinstalls it unless autoInstallIdeExtension is false or CLAUDE_CODE_IDE_SKIP_AUTO_INSTALL=1.
7. The IDE link (the built-in ide MCP server) exists only while the extension or plugin is active. Headless build and screenshot checks should not depend on it.
8. The JetBrains docs warn that acceptEdits mode can let Claude modify IDE config files that the IDE may run automatically, and they recommend Manual mode there.
9. Of 20 JetBrains registry results for 'claude code', only 27310 is vendored by Anthropic, PBC. Several third-party GUI plugins also match, so check the vendor before installing.
10. A summarized fetch of the VS Code Marketplace page said the minimum is VS Code 1.98.0. The docs and the manifest say 1.94.0.

Practical read for a VS Code-based CLI-first setup: run 'claude' in the integrated terminal with the standalone install, and use the extension panel when you want per-change review or plan comments. Switch to Manual mode to see per-change buttons, because from v2.1.283 Auto is the extension's built-in starting mode.

Local side effects: no project files were created or modified. One npm view query may have touched npm's cache directory.
