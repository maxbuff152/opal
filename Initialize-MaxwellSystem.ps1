<#
.SYNOPSIS
    System Initialization owner for Opal: one Windhawk mod, telemetry Core, no Adaptive Dock.

.DESCRIPTION
    Probe or repair the Maxwell pieces this repository owns. The live Windhawk
    owner is exactly local@opal. Repair never starts MaxwellShell.exe, never
    restores the retired five-mod stack, and never treats Wallpaper/F12 as an
    Opal repair target.
#>
[CmdletBinding()]
param(
    [ValidateSet('Probe', 'Repair')]
    [string]$Action = 'Probe'
)

$ErrorActionPreference = 'Stop'

$expectedModId = 'local@opal'
$retiredModIds = @(
    'local@opal-shell',
    'local@opal-addon-clock',
    'local@opal-addon-media',
    'local@opal-addon-system-info',
    'local@maxwell-shell',
    'local@maxwell-taskbar-styler',
    'local@maxwell-taskbar-clock',
    'local@maxwell-taskbar-experience'
)
$modsRoot = 'HKLM:\SOFTWARE\Windhawk\Engine\Mods'
$runKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run'
$coreExe = Join-Path $env:LOCALAPPDATA 'Maxwell\Shell\Core\Maxwell.Shell.Core.exe'

function Test-IsLeftoverMaxwellShellPath([string]$Value) {
    if ([string]::IsNullOrWhiteSpace($Value)) { return $false }
    if ($Value -match '(?i)Maxwell\.Shell\.Core\.exe') { return $false }
    return [bool]($Value -match '(?i)MaxwellShell\.exe')
}

function Stop-LeftoverMaxwellShell {
    foreach ($process in @(Get-CimInstance Win32_Process -Filter "Name='MaxwellShell.exe'" -ErrorAction SilentlyContinue)) {
        $path = [string]$process.ExecutablePath
        if ($path -and ([IO.Path]::GetFileName($path) -ieq 'Maxwell.Shell.Core.exe')) { continue }
        Stop-Process -Id $process.ProcessId -Force -ErrorAction SilentlyContinue
        Wait-Process -Id $process.ProcessId -Timeout 8 -ErrorAction SilentlyContinue
    }
    Get-Process -Name MaxwellShell -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue

    if (Test-Path -LiteralPath $runKey) {
        foreach ($name in @((Get-Item -LiteralPath $runKey).Property)) {
            if ($name -eq 'MaxwellShellCore') { continue }
            $value = [string](Get-ItemPropertyValue -LiteralPath $runKey -Name $name -ErrorAction SilentlyContinue)
            if (Test-IsLeftoverMaxwellShellPath $value) {
                Remove-ItemProperty -LiteralPath $runKey -Name $name -ErrorAction SilentlyContinue
            }
        }
    }
}

function Get-LeftoverShellTasks {
    foreach ($task in @(Get-ScheduledTask -ErrorAction SilentlyContinue)) {
        $executes = @(
            foreach ($action in @($task.Actions)) {
                '{0} {1}' -f [string]$action.Execute, [string]$action.Arguments
            }
        ) -join ' '
        if (Test-IsLeftoverMaxwellShellPath $executes) {
            $task
        }
    }
}

function Disable-LeftoverShellTasks {
    foreach ($task in @(Get-LeftoverShellTasks)) {
        Stop-ScheduledTask -TaskName $task.TaskName -TaskPath $task.TaskPath -ErrorAction SilentlyContinue
        Disable-ScheduledTask -TaskName $task.TaskName -TaskPath $task.TaskPath -ErrorAction SilentlyContinue
        Unregister-ScheduledTask -TaskName $task.TaskName -TaskPath $task.TaskPath -Confirm:$false -ErrorAction SilentlyContinue
    }
}

function Get-WindhawkOwner {
    $ids = if (Test-Path -LiteralPath $modsRoot) {
        @(Get-ChildItem -LiteralPath $modsRoot -ErrorAction SilentlyContinue | ForEach-Object PSChildName)
    } else { @() }
    $retiredPresent = @($ids | Where-Object { $_ -in $retiredModIds })
    $ready = ($ids.Count -eq 1 -and $ids[0] -eq $expectedModId)
    [pscustomobject]@{
        name = 'Windhawk/Opal'
        owner = $expectedModId
        liveIds = @($ids)
        retiredIdsPresent = @($retiredPresent)
        status = if ($ready) { 'ready' } elseif ($ids -contains $expectedModId) { 'repair-required' } else { 'repair-required' }
        ready = $ready
    }
}

function Get-CoreOwner {
    $running = @(Get-CimInstance Win32_Process -Filter "Name='Maxwell.Shell.Core.exe'" -ErrorAction SilentlyContinue)
    $autostart = [string](Get-ItemPropertyValue -LiteralPath $runKey -Name 'MaxwellShellCore' -ErrorAction SilentlyContinue)
    $installed = Test-Path -LiteralPath $coreExe -PathType Leaf
    $ready = $installed -and $running.Count -ge 1 -and ($autostart -match 'Maxwell\.Shell\.Core\.exe')
    [pscustomobject]@{
        name = 'Maxwell.Shell.Core'
        installed = $installed
        running = $running.Count
        autostart = $autostart
        status = if ($ready) { 'ready' } else { 'repair-required' }
        ready = $ready
    }
}

function Get-LeftoverShellOwner {
    $processes = @(Get-Process -Name MaxwellShell -ErrorAction SilentlyContinue)
    $tasks = @(Get-LeftoverShellTasks)
    $ready = $processes.Count -eq 0 -and $tasks.Count -eq 0
    [pscustomobject]@{
        name = 'Leftover MaxwellShell.exe'
        processes = $processes.Count
        leftoverTasks = @($tasks | ForEach-Object { $_.TaskName })
        status = if ($ready) { 'ready' } else { 'repair-required' }
        ready = $ready
    }
}

function Get-WallpaperOwner {
    # Wallpaper/F12 is not an Opal surface. Reporting it as repair-required
    # caused System Initialization to invoke the retired Adaptive Dock.
    [pscustomobject]@{
        name = 'Wallpaper/F12'
        ownedBy = 'not-opal'
        status = 'not-owned'
        ready = $true
        repair = 'never'
    }
}

function Invoke-OpalOwnedRepair {
    Stop-LeftoverMaxwellShell
    Disable-LeftoverShellTasks
    if (-not (Get-CoreOwner).ready) {
        $coreScript = Join-Path $PSScriptRoot 'Install-MaxwellShellCore.ps1'
        if (Test-Path -LiteralPath $coreScript) {
            & $coreScript -Action Install | Out-Null
        }
    }
}

$windhawk = Get-WindhawkOwner
$core = Get-CoreOwner
$leftover = Get-LeftoverShellOwner
$wallpaper = Get-WallpaperOwner

if ($Action -eq 'Repair') {
    Invoke-OpalOwnedRepair
    $windhawk = Get-WindhawkOwner
    $core = Get-CoreOwner
    $leftover = Get-LeftoverShellOwner
    $wallpaper = Get-WallpaperOwner
}

$ownedReady = [bool]$windhawk.ready -and [bool]$core.ready -and [bool]$leftover.ready
$result = [pscustomobject]@{
    succeeded = $ownedReady
    action = $Action
    owner = $expectedModId
    components = @($windhawk, $core, $leftover, $wallpaper)
    retiredGuardInvoked = $false
}
$result
if (-not $ownedReady) { exit 1 }
