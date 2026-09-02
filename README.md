# Opal Windows Shell

Friendly local path: `D:\Opal` (a junction to this canonical project; no files
are duplicated or moved). See `OPAL-START-HERE.md` for the short project map.

## Supported architecture

Opal is the only supported Windhawk visual runtime in this tree. Windhawk shows
exactly one Maxwell-owned mod: `local@opal` 4.4.0. Its DLL contains internally
isolated Shell, Clock, Media, and Performance components.

Media and Performance initialize only in Explorer. Start, Search, notification,
and shell-flyout hosts use the shell component without starting the optional
workers or taskbar UI.
`Maxwell.Shell.Core.exe` is a telemetry-only companion; the retired Adaptive Dock
is not built, installed, or started.

Open **Opal → Settings** in Windhawk. The page is grouped into seven plain
sections—**Start here, Screens, Media, Computer stats, Windows look, Clock,**
and **Advanced**—instead of one long list of prefixed technical fields:

- **Resource Saver**: on by default; releases idle Media XAML, loaded-frame
  subscriptions, histories, closed flyouts, and fallback metric providers while
  retaining event-driven controls.
- **Media**: independently enables or fully dormants the Media capability.
- **Performance**: independently enables or fully dormants the CPU/RAM glance
  and on-demand Hardware Command Center.
- **Display ownership**: independently places Media and Performance on the
  primary display, secondary display, or both displays. Both uses one rich view
  plus one compact mirror backed by the same immutable snapshot—never duplicate
  media-session workers or sensor collectors.
- **Widget appearance**: sets all Media and Performance text to Small,
  Standard, or Large, and makes their backgrounds Subtle, Glass, or Strong.
- **Media details**: independently shows or hides album art and artist details,
  hides the widget when nothing is playing, and enables smooth progress motion.
- **Performance details**: chooses Fahrenheit or Celsius, turns the tiny history
  graphs on or off, and controls whether clicking opens the detailed hardware
  view.
- **Clock appearance**: sets clock text to Small, Standard, or Large separately
  from the other widgets.
- **Reset crash protection**: clears an automatic component quarantine after an
  unstable Explorer session.

The everyday choices are in **Start here**. Screen placement, feature-specific
choices, and Windows styling have their own sections; raw clock formatting,
taskbar dimensions, repair switches, and diagnostic logging live under
**Advanced**.
Automatic placement is recommended. Custom placement enables dragging for the
full Media and Performance widgets and saves separate positions for the main
and second screens. Size choices use Compact, Standard, and Expanded labels;
raw dimensions remain in the advanced section. Text size changes affect the
full widgets and compact mirrors without creating more collectors or processes.

Windhawk applies Media and Performance changes live through the single mod's
settings-change callback. There is no Opal executable, Start-menu shortcut,
tray resident, control mirror, message hook, polling watcher, or ordinary
Explorer restart. Per-Explorer runtime INI files are diagnostics only.

The old modular Windhawk stack, monolithic Maxwell shell, ChatGPT glass,
third-party pinned sources, migration scripts, superseded tests, and historical
measurements were moved to:

`D:\CODEX SPINE, PLUGINS AND AGENTS\90-ARCHIVE\Windhawk ChatGPT Guard\pre-opal-cleanup-20260831-2145`

## Performance contract

Opal targets stock-like idle behavior rather than claiming less custom overhead
than stock Windows' zero. The acceptance gates are:

- zero monotonic handle, GDI, or USER-object growth;
- near-zero idle CPU in Explorer, Windhawk, and the telemetry companion;
- no more than 20 MB private-commit and 16 MB working-set delta in the controlled paired package-cost receipt;
- exact host gating for all internal components;
- no hidden expanded-only XAML objects in the canonical 184-DIP CPU/RAM layout;
- stale XAML style generations released after settings changes;
- event-driven media and shared-memory hardware telemetry;
- no audio-analysis stack, selectable theme engine, wallpaper watcher, or
  repeating PowerShell overlay.

The pre-4.1 one-mod baseline receipt is
`measurements\package-cost-opal-4-unified-valid-settled-abba2.json`. It used two
paired rounds, randomized to ABBA then BAAB, with a 45-second settle and a
15-second sample for every leg:

| Explorer metric | Stock Windows | Full Opal Lean | Measured delta |
|---|---:|---:|---:|
| CPU, % of machine | 2.1268% | 1.8836% | -0.2432 pp (-11.4%) |
| Private commit | 334.60 MB | 348.92 MB | +14.32 MB |
| Working set | 390.54 MB | 401.66 MB | +11.12 MB |
| Handles | 5,711.5 | 5,759.5 | +48 |
| GDI objects | 324.0 | 324.5 | +0.5 |
| USER objects | 486.5 | 491.5 | +5 |
| Threads | 212.0 | 215.5 | +3.5 |

The paired result meets the stock-or-better steady-state CPU target on this run.
The honest cost is roughly 14 MB of private commit, 11 MB of working set, 48
handles, five USER objects, and three to four threads for the full visual and
telemetry experience. Absolute CPU was elevated by the active desktop workload,
so the same-run paired difference is more meaningful than either absolute value.
Opal 4.1 adds lazy loaded-frame revokers, releases dormant history capacity and
external-telemetry PDH fallbacks, and adds shared-data dual-monitor mirrors. A
new full ABBA release comparison should be run only when another performance
decision depends on it; the 4.0 receipt remains the honest numerical baseline.

## Legacy stack reduction

The archived pre-Opal baseline used 10 enabled mods, 20 process injections,
20,706 KB of mapped custom DLLs, and 97,130 source lines. Explorer alone mapped
8 custom mod DLLs totaling 7,276 KB. Current Opal uses one package and one
1.28-MB DLL; Explorer maps that single DLL instead of three Opal DLLs. Other
shell hosts map the same binary but do not initialize Media or Performance.
Cross-run Explorer memory snapshots are not used for the reduction claim because
open windows and shell cache state differ.

## Canonical experience

- 68-DIP taskbar, 38-DIP official app icons, and 50-DIP taskbar cells.
- Fixed graphite/neutral frost with no selectable color themes.
- 176-DIP two-line clock with date and optional Katy weather.
- 184-DIP CPU/RAM glance; GPU, VRAM, thermals, history, processes, and native
  Windows tools remain one click away in Hardware Command Center.
- Adaptive 232-336-DIP media capsule with artwork, previous/play/next, timeline
  scrubbing, Shift-scroll seeking, volume scrolling, and session switching.
- Windows continues to own taskbar previews, flyout material, accessibility,
  scrolling, and native transitions where practical.

## Build, verify, and install

Run from this directory:

```powershell
.\Build-OpalSuite.ps1
.\Test-OpalSuite.ps1 -StaticOnly
.\Test-OpalPerformanceBudget.ps1 -ReceiptPath .\measurements\package-cost-opal-4-unified-valid-settled-abba2.json
.\Install-OpalSuite.ps1 -WhatIf
.\Install-OpalSuite.ps1
.\Test-OpalSuite.ps1
.\tests\Test-WindhawkSafeDock.ps1
.\tests\Test-MaxwellShellCore.ps1
.\tests\Test-OpalUnifiedControl.ps1
```

`Build-OpalSuite.ps1` is the only release build. It produces deterministic DLLs
and hashes. `Install-OpalSuite.ps1` creates a complete live rollback bundle before
changing Windhawk and automatically restores it if Explorer or a package fails.
Recognized `local@opal` choices are carried forward across later installs, so a
DLL update no longer silently replaces the user's appearance or performance
profile. The two one-shot reset switches are intentionally cleared after an update.

## Measure

```powershell
.\Measure-ShellPerformance.ps1 -Seconds 15
.\Measure-MaxwellShellFootprint.ps1 -Label opal-current -Samples 5 -SampleSeconds 2
.\Measure-OpalPackageCost.ps1 -Label stock-ab -Samples 2 -SampleSeconds 10 -SettleSeconds 15
.\Measure-OpalPackageCost.ps1 -AbbaRounds 2 -RandomSeed 3 -Label final-settled -SampleSeconds 15 -SettleSeconds 45
```

The package-cost harness can measure stock, Shell only, Shell plus Media, Shell
plus Performance, and the full suite. Its randomized ABBA/BAAB mode controls for
warm-up and time drift. It marks only its planned Explorer restarts as
intentional, restores the original registry state in a `finally` block, and
leaves crash quarantine armed for ordinary sessions.

## Recovery

```powershell
.\Ensure-WindhawkSafeDock.ps1 -Mode Check -RequireLoaded
.\Ensure-WindhawkSafeDock.ps1 -Mode Repair -AllowExplorerRestart
.\Ensure-WindhawkSafeDock.ps1 -Mode CircuitBreak
.\Ensure-WindhawkSafeDock.ps1 -Mode ClearCircuitBreaker
.\New-WindhawkSafeDockBackup.ps1
.\Restore-WindhawkSafeDockBackup.ps1 -Slot current -WhatIf
.\New-MaxwellShellRollbackPoint.ps1 -Note 'manual checkpoint'
```

The circuit breaker provides stock fallback. Tooling backups retain only the
current supported surface; live installer rollback bundles remain under
`%LOCALAPPDATA%\Maxwell\WindhawkChatGPTGuard\rollback-*`.
