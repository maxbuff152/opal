# AGENTS.md

Guidance for automated agents working in this repository.

## What Opal is

Opal is a **Windows-only** shell experience delivered as a single Windhawk mod
(`local@opal` 4.4.0) plus a small native telemetry companion. See `README.md`
and `OPAL-START-HERE.md` for the product map, and `CONTRIBUTING.md` for the
change workflow. Canonical component sources live under `mod/visual-clones`.

## What runs where

Only the live install and runtime validation are strictly Windows + Windhawk:

- `Install-OpalSuite.ps1` writes the Windows registry and restarts Explorer.
- `Test-OpalSuite.ps1` **without** `-StaticOnly` reads the registry and inspects
  `explorer.exe`.

Runtime behavior (does the taskbar actually render, does Explorer stay healthy)
can only be verified on Windows.

### Real C++ builds run on Linux

`Build-OpalSuite.ps1` is the Windows release build (Windhawk's bundled clang).
A faithful Linux cross-build is also available and produces the identical
artifacts, so C++ changes can be compiled and validated here:

```bash
bash ./.cursor/install-buildchain.sh    # one-time: clang-20 + Windhawk headers
```
```powershell
pwsh ./Build-OpalSuite.Linux.ps1        # compiles + links the real PE DLL
pwsh ./Test-OpalSuite.ps1 -StaticOnly   # validates the built DLL + receipt
```

`Build-OpalSuite.Linux.ps1` drives clang-20/lld-20/mingw-w64 against Windhawk's
own headers (`windhawk_api.h`, cppwinrt WinRT projections) and engine import
library (`windhawk.lib`), fetched by `.cursor/install-buildchain.sh` into a
git-ignored cache (`~/.cache/opal-windhawk`). It writes
`build/opal-suite/build-receipt.json` in the same format as the Windows build,
so `Test-OpalSuite.ps1 -StaticOnly` validates it on Linux.

The following also run cross-platform in PowerShell 7 and are safe on Linux:

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

The Cloud Agent environment is Linux. `.cursor/install.sh` (wired through
`.cursor/environment.json`) provisions both PowerShell 7 + PSScriptAnalyzer and
the clang-20/mingw cross-build toolchain.

- Fast source check: `pwsh ./Test-OpalStatic.ps1` (assemble, static suites,
  parse-lint, PSScriptAnalyzer, consistency). Treat a green gate as success for
  PowerShell tooling and source-level contracts.
- C++ changes: `pwsh ./Build-OpalSuite.Linux.ps1` then
  `pwsh ./Test-OpalSuite.ps1 -StaticOnly` to compile the real DLL and validate
  it. A clean build proves the C++ compiles and links; runtime behavior still
  needs Windows.
- Only the live install and `Test-OpalSuite.ps1` without `-StaticOnly` require a
  Windows host with Windhawk.
- Generated output (`build/`, `dist/`) and the toolchain cache
  (`~/.cache/opal-windhawk`) are not committed.
