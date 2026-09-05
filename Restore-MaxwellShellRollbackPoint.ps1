<#
.SYNOPSIS
    Restores the live Windhawk shell state from a rollback bundle.

.DESCRIPTION
    Puts back exactly what New-MaxwellShellRollbackPoint.ps1 captured: the mod
    DLLs, the installed sources, and both Windhawk registry hives, then restarts
    the Windhawk service and Explorer so the restored state is actually live.

    Files are verified against the manifest's SHA256 before anything is written,
    so a truncated or tampered bundle fails before it can half-restore the shell.

    Restoring is destructive to current state by design - that is the point - so
    it takes a fresh safety snapshot of the *current* state first unless -NoSafety
    is passed. If the restore itself goes wrong you can still get back.

.PARAMETER Bundle
    Path to the rollback bundle directory.

.PARAMETER NoSafety
    Skip snapshotting current state before restoring.

.PARAMETER WhatIf
    Report what would be restored without writing.
#>
[CmdletBinding(SupportsShouldProcess)]
param(
    [Parameter(Mandatory)] [string] $Bundle,
    [switch] $NoSafety
)

$ErrorActionPreference = 'Stop'

$identity  = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = [Security.Principal.WindowsPrincipal]::new($identity)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'Administrator access is required to restore Windhawk engine state.'
}

function Invoke-RegExe {
    param([Parameter(Mandatory)][string[]] $Arguments)

    $regExe = Join-Path $env:SystemRoot 'System32\reg.exe'
    $process = Start-Process -FilePath $regExe -ArgumentList $Arguments `
        -WindowStyle Hidden -Wait -PassThru
    if ($process.ExitCode -ne 0) {
        throw "reg.exe failed with exit code $($process.ExitCode): $($Arguments -join ' ')"
    }
}

$manifestPath = Join-Path $Bundle 'manifest.json'
if (-not (Test-Path -LiteralPath $manifestPath)) { throw "Not a rollback bundle (no manifest.json): $Bundle" }
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json

Write-Host ""
Write-Host ("  bundle : {0}" -f $Bundle) -ForegroundColor Cyan
Write-Host ("  created: {0}" -f $manifest.CreatedUtc)
Write-Host ("  note   : {0}" -f $manifest.Note)
Write-Host ("  files  : {0}" -f $manifest.FileCount)

$map = @{ dll64 = 'C:\ProgramData\Windhawk\Engine\Mods\64'
          dll32 = 'C:\ProgramData\Windhawk\Engine\Mods\32'
          source = 'C:\ProgramData\Windhawk\ModsSource'
          profile = 'C:\ProgramData\Windhawk'
          opalApp = (Join-Path $env:LOCALAPPDATA 'Maxwell\Opal')
          opalShortcut = (Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs') }
$src = @{ dll64 = (Join-Path $Bundle 'dlls\64')
          dll32 = (Join-Path $Bundle 'dlls\32')
          source = (Join-Path $Bundle 'sources')
          profile = (Join-Path $Bundle 'profile')
          opalApp = (Join-Path $Bundle 'apps\opal')
          opalShortcut = (Join-Path $Bundle 'shortcuts') }

# --- verify the whole bundle BEFORE writing anything ---------------------------
$bad = [Collections.Generic.List[string]]::new()
foreach ($f in $manifest.Files) {
    $p = Join-Path $src[$f.Kind] $f.Relative
    if (-not (Test-Path -LiteralPath $p)) { $bad.Add("missing: $($f.Kind)/$($f.Relative)"); continue }
    if ((Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash -ne $f.Sha256) { $bad.Add("hash mismatch: $($f.Kind)/$($f.Relative)") }
}
if ($bad.Count -gt 0) {
    $bad | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
    throw "Bundle failed verification ($($bad.Count) problems). Refusing to restore."
}
Write-Host "  verified: all files match the manifest." -ForegroundColor Green

if ($WhatIfPreference) {
    Write-Host "  -WhatIf: nothing written." -ForegroundColor Yellow
    $manifest.ModState | Format-Table Mod, Disabled, Version -AutoSize
    return
}

if (-not $NoSafety) {
    Write-Host "  taking a safety snapshot of CURRENT state first..." -ForegroundColor Cyan
    & (Join-Path $PSScriptRoot 'New-MaxwellShellRollbackPoint.ps1') -Note "auto safety snapshot before restoring $Bundle" | Out-Null
}

if ($PSCmdlet.ShouldProcess('Windhawk shell state', 'restore from bundle')) {
    Stop-Service -Name 'Windhawk' -Force -ErrorAction SilentlyContinue
    foreach ($name in @('explorer', 'StartMenuExperienceHost', 'SearchHost', 'SearchApp', 'ShellExperienceHost', 'ShellHost')) {
        Stop-Process -Name $name -Force -ErrorAction SilentlyContinue
    }
    Start-Sleep -Seconds 2

    if ($manifest.PSObject.Properties['OpalAppPresent']) {
        $opalAppTarget = Join-Path $env:LOCALAPPDATA 'Maxwell\Opal'
        $expectedOpalAppTarget = [IO.Path]::GetFullPath((Join-Path $env:LOCALAPPDATA 'Maxwell\Opal')).TrimEnd('\')
        if ([IO.Path]::GetFullPath($opalAppTarget).TrimEnd('\') -ne $expectedOpalAppTarget) {
            throw "Refusing unresolved Opal app target: $opalAppTarget"
        }
        if (Test-Path -LiteralPath $opalAppTarget) {
            # Rollback bundles live inside this owner. Preserve them while
            # replacing runtime state so a restore never deletes its own
            # source halfway through the copy.
            $resolvedOpalAppTarget = [IO.Path]::GetFullPath($opalAppTarget).TrimEnd('\')
            foreach ($child in @(Get-ChildItem -LiteralPath $opalAppTarget -Force)) {
                if ($child.Name -like 'rollback-*') { continue }
                $resolvedChild = [IO.Path]::GetFullPath($child.FullName)
                if (-not $resolvedChild.StartsWith(
                        $resolvedOpalAppTarget + '\',
                        [StringComparison]::OrdinalIgnoreCase)) {
                    throw "Refusing to remove path outside Opal runtime: $resolvedChild"
                }
                Remove-Item -LiteralPath $child.FullName -Recurse -Force
            }
        }
        $opalShortcutTarget = Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs\Opal.lnk'
        if (Test-Path -LiteralPath $opalShortcutTarget) {
            Remove-Item -LiteralPath $opalShortcutTarget -Force
        }
    }

    foreach ($kind in @('dll64', 'dll32', 'source', 'profile', 'opalApp', 'opalShortcut')) {
        $from = $src[$kind]; $to = $map[$kind]
        if (-not (Test-Path -LiteralPath $from)) { continue }
        New-Item -ItemType Directory -Path $to -Force | Out-Null
        foreach ($f in Get-ChildItem -LiteralPath $from -File -Recurse) {
            $rel  = $f.FullName.Substring($from.Length).TrimStart('\')
            $dest = Join-Path $to $rel
            New-Item -ItemType Directory -Path (Split-Path $dest -Parent) -Force | Out-Null
            Copy-Item -LiteralPath $f.FullName -Destination $dest -Force
        }
    }

    if ($manifest.PSObject.Properties['OpalControlKeyPresent']) {
        if (Test-Path -LiteralPath 'HKCU:\Software\Maxwell\Opal') {
            Invoke-RegExe -Arguments @('delete', 'HKCU\Software\Maxwell\Opal', '/f')
        }
        if ([bool]$manifest.OpalControlKeyPresent) {
            $opalControlReg = Join-Path $Bundle 'OpalControl.reg'
            if (-not (Test-Path -LiteralPath $opalControlReg)) {
                throw 'Rollback manifest expects OpalControl.reg, but it is missing.'
            }
            Invoke-RegExe -Arguments @('import', $opalControlReg)
        }
    }

    foreach ($store in @('Mods', 'ModsWritable')) {
        $reg = Join-Path $Bundle "$store.reg"
        if (-not (Test-Path -LiteralPath $reg)) { continue }
        $key = "HKLM\SOFTWARE\Windhawk\Engine\$store"
        $providerKey = "HKLM:\SOFTWARE\Windhawk\Engine\$store"
        if (Test-Path -LiteralPath $providerKey) {
            Invoke-RegExe -Arguments @('delete', $key, '/f')
        }
        Invoke-RegExe -Arguments @('import', $reg)
    }

    Start-Service -Name 'Windhawk' -ErrorAction SilentlyContinue
    $windhawkExe = 'C:\Program Files\Windhawk\windhawk.exe'
    if (-not (Test-Path -LiteralPath $windhawkExe -PathType Leaf)) {
        throw "Windhawk runtime not found: $windhawkExe"
    }
    Start-Process -FilePath $windhawkExe -ArgumentList @('-restart', '-tray-only') `
        -WindowStyle Hidden -Wait

    Write-Host "  restarting Explorer so the restored state goes live..." -ForegroundColor Cyan
    $planned = Join-Path $env:LOCALAPPDATA 'Maxwell\Opal\planned-explorer-restart'
    New-Item -ItemType File -Force -Path $planned | Out-Null
    Stop-Process -Name explorer -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 3
    if (-not (Get-Process -Name explorer -ErrorAction SilentlyContinue)) { Start-Process explorer.exe }

    Write-Host ""
    Write-Host "  RESTORED." -ForegroundColor Green
    $manifest.ModState | Format-Table Mod, Disabled, Version -AutoSize
}
