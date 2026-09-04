# AGENTS.md

Guidance for automated agents working in this repository.

## What Opal is

Opal is a **Windows-only** shell experience delivered as a single Windhawk mod
(`local@opal` 4.4.0) plus a small native telemetry companion. See `README.md`
and `OPAL-START-HERE.md` for the product map, and `CONTRIBUTING.md` for the
change workflow. Canonical component sources live under `mod/visual-clones`.

## What runs where

The release build, install, and live validation are Windows + Windhawk only:

- `Build-OpalSuite.ps1` needs Windhawk's clang toolchain
  (`C:\Program Files\Windhawk\Compiler`).
- `Install-OpalSuite.ps1` writes the Windows registry and restarts Explorer.
- `Test-OpalSuite.ps1` (and the non-`-StaticOnly` suites) read the registry,
  inspect `explorer.exe`, and require a build receipt.

Do not attempt those on Linux/macOS; run them on a Windows host with Windhawk.

The following DO run cross-platform in PowerShell 7 and are safe on Linux:

- `Assemble-Opal.ps1` - inlines the multi-header source into one `.wh.cpp`.
- `tests/Test-OpalUnifiedControl.ps1 -StaticOnly`
- `tests/Test-MaxwellTaskbarMediaCapsule.ps1 -StaticOnly`
- AST parse-lint of every `.ps1`/`.psm1`.
- PSScriptAnalyzer (config in `PSScriptAnalyzerSettings.psd1`).

`Test-OpalStatic.ps1` runs all of the above as one gate. This is the canonical
cross-platform check and is what CI (`.github/workflows/ci.yml`) enforces:

```powershell
pwsh ./Test-OpalStatic.ps1
```

## Cursor Cloud specific instructions

The Cloud Agent environment is Linux, so only the cross-platform static gate is
runnable here; the Windows build/install/live tests cannot run in Cloud.

- PowerShell 7 and PSScriptAnalyzer are provisioned by
  `.cursor/install-pwsh.sh` (wired through `.cursor/environment.json`).
- To validate changes in Cloud, run `pwsh ./Test-OpalStatic.ps1` and treat a
  green gate as success for the PowerShell tooling and source-level contracts.
- Generated output (`build/`, `dist/`) is git-ignored; do not commit it.
