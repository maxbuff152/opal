# Opal Start Here

Run Opal commands from the repository root. On Maxwell's development machine,
the stable local alias is:

`D:\Opal`

That alias is a Windows junction, not a copy. The product and repository are
**Opal 4.5**. Runtime state, diagnostic logs, and installer rollbacks belong
under `%LOCALAPPDATA%\Maxwell\Opal`. Older SafeDock snapshots may still exist
under `%LOCALAPPDATA%\Maxwell\WindhawkChatGPTGuard` for compatibility.
The recovery safety latch is machine-wide at
`%ProgramData%\Windhawk\Opal\safedock`; interactive checks and scheduled tasks
must use that same path. Shell recovery runs in the signed-in user session.

Explicit Primary/Secondary widget assignments stay on their chosen display
while a fullscreen app is open. Both mode keeps its full view on the chosen display and uses compact mirrors elsewhere; it falls back only when a taskbar disappears.
Run `tests\Test-OpalAttachment.ps1` after reload: a mapped DLL and active runtime
flags alone do not prove that the widgets attached. Follow with visual QA.

## What Opal is

- One Windhawk mod: `local@opal` 4.5.0. That mod is the taskbar.
- One settings page in Windhawk.
- Internal components: Shell, Clock, Media, and Performance.
- No separate Opal app or background controller.
- Resource Saver releases dormant UI and fallback providers.
- Both-display mode shares one worker per capability across a full view and a
  compact mirror.
- Windhawk settings are grouped into Start here, Screens, Media, Computer
  stats, Start/Search/notifications, Clock, and Advanced. The bar is not a
  separate look toggle.
- Widget customization stays inside Windhawk: shared Media/Performance text
  size and background strength, Media artwork/details/idle/progress choices,
  Performance temperature/graphs/click behavior, and a separate clock size.
- Full widgets can be dragged in Custom placement mode and remember independent
  positions for the main and second displays; compact mirrors stay coordinated
  automatically.

## Canonical files

- Main entry: `mod\visual-clones\maxwell-shell.wh.cpp`
- Media: `mod\visual-clones\maxwell-opal-media.wh.cpp`
- Performance: `mod\visual-clones\maxwell-taskbar-system-info.wh.cpp`
- Build: `Build-OpalSuite.ps1`
- Install: `Install-OpalSuite.ps1` (also installs `Maxwell.Shell.Core` and stops leftover `MaxwellShell.exe`)
- Validation: `Test-OpalSuite.ps1`
- Historical controlled performance evidence (before 4.5):
  `measurements\package-cost-opal-4-unified-valid-settled-abba2.json`
- 4.5 live resource sample: `measurements\opal45-final-live-performance.json`.
  This short sample is not a controlled overhead or long-run leak benchmark.

Edit the canonical component sources, then build through
`Build-OpalSuite.ps1`. After Opal source should go live, run
`Install-OpalSuite.ps1` elevated (UAC / Explorer restart) in the same pass.
Files under `build`, `dist`, and `measurements` are generated artifacts or
evidence, not alternate source owners.

## 4.5 reliability and two-screen release

The main-screen preset places the full Media and Computer stats widgets on Primary only. Secondary keeps its taskbar and clock without duplicate compact widgets. Automatic layout
no longer overrides the chosen full-view display. Fullscreen coverage does not
shuffle widgets. Each widget owns and releases its own reserved app-lane space.

The existing shell recovery thread now checks current XAML roots and all expected
views every five seconds after startup, repairing missing views without rebuilding
healthy ones. Recovery drains before component teardown. Attachment verification
requires fresh proof for every enabled view and the configured display.

`Install-OpalSuite.ps1 -MainScreenWidgets` applies this preset; ordinary updates
preserve recognized choices. The installer rejects builds whose source inputs
changed and rolls back if enabled widgets fail to attach after installation.
Rollback backup traversal prunes excluded folders before descending, so previous
rollback bundles cannot make a new backup recursively scan its own history.
The telemetry companion retries failed weather fetches after 30 seconds while
keeping the normal ten-minute interval after a successful fetch.
Explorer does not duplicate a request accepted by the companion. The companion
sets network timeouts, and a first failed clock attempt displays Weather
unavailable while retrying instead of leaving Loading indefinitely.

`Assemble-Opal.ps1` emits a shell source excerpt for inspection and the suite
builder. It is not an installable standalone Opal package. Its compile-time guard
prevents a Windhawk source recompile from silently removing Media and Performance.
Build and install through the suite scripts. The separate `D:\Repos\maxwell-shell`
command-palette project is not an Opal component or an alternate release path.

Maxwell clarified on 2026-09-04: no repeated simplified widgets on the left screen. This supersedes the earlier proposed mirrored layout. Both-screen mode remains an explicit optional choice, never the default or an automatic migration.
