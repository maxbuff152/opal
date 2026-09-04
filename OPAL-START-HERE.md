# Opal Start Here

Run Opal commands from the repository root. On Maxwell's development machine,
the stable local alias is:

`D:\Opal`

That alias is a Windows junction, not a copy. The product and repository are
**Opal 4.4**. Runtime state, diagnostic logs, and installer rollbacks belong
under `%LOCALAPPDATA%\Maxwell\Opal`. Older SafeDock snapshots may still exist
under `%LOCALAPPDATA%\Maxwell\WindhawkChatGPTGuard` for compatibility.

## What Opal is

- One Windhawk mod: `local@opal` 4.4.0. That mod is the taskbar.
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
- Install: `Install-OpalSuite.ps1`
- Validation: `Test-OpalSuite.ps1`
- Final performance evidence:
  `measurements\package-cost-opal-4-unified-valid-settled-abba2.json`

Edit the canonical component sources, then build through
`Build-OpalSuite.ps1`. After Opal source should go live, run
`Install-OpalSuite.ps1` elevated (UAC / Explorer restart) in the same pass.
Files under `build`, `dist`, and `measurements` are generated artifacts or
evidence, not alternate source owners.
