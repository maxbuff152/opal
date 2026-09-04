<#
.SYNOPSIS
    Captures a complete, restorable snapshot of the live Windhawk shell state.

.DESCRIPTION
    New-WindhawkSafeDockBackup.ps1 snapshots the *tooling* (.ps1/.psm1/.md). It does
    not capture the things that actually decide what your shell looks like and
    whether it boots: the installed mod DLLs, the installed mod sources, and the
    Windhawk registry hives.

    The maxwell-shell consolidation replaces several live mods at once, so a
    tooling backup is not a rollback. This captures everything needed to put the
    shell back exactly as it was:

      - HKLM\SOFTWARE\Windhawk\Engine\Mods         (.reg)
      - HKLM\SOFTWARE\Windhawk\Engine\ModsWritable (.reg)
      - every DLL in Engine\Mods\64 and Engine\Mods\32
      - every installed source in ProgramData\Windhawk\ModsSource
      - ProgramData\Windhawk\userprofile.json (the Windhawk UI's mod list)
      - a manifest with SHA256 for each file and the enabled/disabled state

    Restore with Restore-MaxwellShellRollbackPoint.ps1 -Bundle <path>.

    Requires administrator (the DLL and registry locations are machine scope).

.PARAMETER Note
    Short label recorded in the manifest, e.g. "before maxwell-shell merge".
#>
[CmdletBinding()]
param(
    [string] $Note = 'manual rollback point'
)

$ErrorActionPreference = 'Stop'

$identity  = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = [Security.Principal.WindowsPrincipal]::new($identity)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'Administrator access is required to snapshot Windhawk engine state.'
}

$stamp  = Get-Date -Format 'yyyyMMdd-HHmmss'
$root   = Join-Path $env:LOCALAPPDATA "Maxwell\Opal\rollback-$stamp"
New-Item -ItemType Directory -Path $root -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $root 'dlls')    -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $root 'sources') -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $root 'profile') -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $root 'apps\opal') -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $root 'shortcuts') -Force | Out-Null

$files = [Collections.Generic.List[object]]::new()

function Copy-Tree([string]$from, [string]$to, [string]$kind) {
    if (-not (Test-Path -LiteralPath $from)) { return }
    foreach ($f in Get-ChildItem -LiteralPath $from -File -Recurse -ErrorAction SilentlyContinue) {
        $rel  = $f.FullName.Substring($from.Length).TrimStart('\')
        $dest = Join-Path $to $rel
        New-Item -ItemType Directory -Path (Split-Path $dest -Parent) -Force | Out-Null
        Copy-Item -LiteralPath $f.FullName -Destination $dest -Force
        $files.Add([pscustomobject]@{
            Kind = $kind; Relative = $rel; Bytes = $f.Length
            Sha256 = (Get-FileHash -LiteralPath $f.FullName -Algorithm SHA256).Hash
        })
    }
}

function Copy-One([string]$from, [string]$to, [string]$kind) {
    if (-not (Test-Path -LiteralPath $from -PathType Leaf)) { return }
    Copy-Item -LiteralPath $from -Destination $to -Force
    $item = Get-Item -LiteralPath $from
    $files.Add([pscustomobject]@{
        Kind = $kind; Relative = (Split-Path -Leaf $to); Bytes = $item.Length
        Sha256 = (Get-FileHash -LiteralPath $from -Algorithm SHA256).Hash
    })
}

Copy-Tree 'C:\ProgramData\Windhawk\Engine\Mods\64' (Join-Path $root 'dlls\64')  'dll64'
Copy-Tree 'C:\ProgramData\Windhawk\Engine\Mods\32' (Join-Path $root 'dlls\32')  'dll32'
Copy-Tree 'C:\ProgramData\Windhawk\ModsSource'     (Join-Path $root 'sources')  'source'

$opalApp = Join-Path $env:LOCALAPPDATA 'Maxwell\Opal'
$opalShortcut = Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs\Opal.lnk'
$opalAppPresent = Test-Path -LiteralPath $opalApp -PathType Container
$opalShortcutPresent = Test-Path -LiteralPath $opalShortcut -PathType Leaf
Copy-Tree $opalApp (Join-Path $root 'apps\opal') 'opalApp'
Copy-One $opalShortcut (Join-Path $root 'shortcuts\Opal.lnk') 'opalShortcut'

$userProfilePath = 'C:\ProgramData\Windhawk\userprofile.json'
if (Test-Path -LiteralPath $userProfilePath) {
    $profileCopy = Join-Path $root 'profile\userprofile.json'
    Copy-Item -LiteralPath $userProfilePath -Destination $profileCopy -Force
    $profileItem = Get-Item -LiteralPath $userProfilePath
    $files.Add([pscustomobject]@{
        Kind = 'profile'; Relative = 'userprofile.json'; Bytes = $profileItem.Length
        Sha256 = (Get-FileHash -LiteralPath $userProfilePath -Algorithm SHA256).Hash
    })
}

foreach ($store in @('Mods', 'ModsWritable')) {
    $key = "HKLM\SOFTWARE\Windhawk\Engine\$store"
    $out = Join-Path $root "$store.reg"
    & reg.exe export $key $out /y 2>$null | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "Registry export failed for $key" }
}

$opalControlKeyPresent = Test-Path -LiteralPath 'HKCU:\Software\Maxwell\Opal'
if ($opalControlKeyPresent) {
    & reg.exe export 'HKCU\Software\Maxwell\Opal' (Join-Path $root 'OpalControl.reg') /y 2>$null | Out-Null
    if ($LASTEXITCODE -ne 0) { throw 'Registry export failed for HKCU\Software\Maxwell\Opal' }
}

# enabled/disabled state, recorded separately so it is readable without importing
$state = @()
$modsKey = 'HKLM:\SOFTWARE\Windhawk\Engine\Mods'
if (Test-Path $modsKey) {
    $state = @(Get-ChildItem $modsKey | ForEach-Object {
        $p = Get-ItemProperty $_.PSPath -ErrorAction SilentlyContinue
        [pscustomobject]@{
            Mod = $_.PSChildName; Disabled = [int]$p.Disabled
            Version = [string]$p.Version; LibraryFileName = [string]$p.LibraryFileName
        }
    } | Sort-Object Mod)
}

$manifest = [pscustomobject]@{
    CreatedUtc    = (Get-Date).ToUniversalTime().ToString('o')
    Note          = $Note
    Bundle        = $root
    FileCount     = $files.Count
    TotalBytes    = ($files | Measure-Object Bytes -Sum).Sum
    OpalAppPresent = [bool]$opalAppPresent
    OpalShortcutPresent = [bool]$opalShortcutPresent
    OpalControlKeyPresent = [bool]$opalControlKeyPresent
    ModState      = $state
    Files         = $files
}
$manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $root 'manifest.json') -Encoding UTF8

Write-Host ""
Write-Host "  rollback point created" -ForegroundColor Green
Write-Host ("  bundle : {0}" -f $root)
Write-Host ("  note   : {0}" -f $Note)
Write-Host ("  files  : {0}  ({1} MB)" -f $files.Count, [math]::Round(($files | Measure-Object Bytes -Sum).Sum / 1MB, 1))
Write-Host ("  mods   : {0} recorded ({1} enabled)" -f $state.Count, @($state | Where-Object { $_.Disabled -eq 0 }).Count)
Write-Host ""
Write-Host "  restore: Restore-MaxwellShellRollbackPoint.ps1 -Bundle '$root'" -ForegroundColor Cyan
