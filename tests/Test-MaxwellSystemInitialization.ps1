<#
.SYNOPSIS
    System Initialization must own one local@opal mod and must not start MaxwellShell.exe.
#>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$failures = [Collections.Generic.List[string]]::new()
function Check([bool]$condition, [string]$message) {
    if (-not $condition) { $failures.Add($message) }
}

$scriptPath = Join-Path $root 'Initialize-MaxwellSystem.ps1'
Check (Test-Path -LiteralPath $scriptPath) 'Initialize-MaxwellSystem.ps1 is missing.'
$text = if (Test-Path -LiteralPath $scriptPath) { [IO.File]::ReadAllText($scriptPath) } else { '' }
Check ($text -match "local@opal") 'System Initialization does not recognize the single local@opal owner.'
Check ($text -notmatch "local@opal-shell" -or $text -match 'retiredModIds') 'Retired five-mod ids must be listed only as retired, not as the live owner.'
Check ($text -match 'retiredModIds' -and $text -match 'local@opal-addon-clock') 'System Initialization still expects the old five-mod Windhawk configuration as current.'
Check ($text -notmatch 'Start-Process[^\r\n]*MaxwellShell\.exe') 'System Initialization must not start leftover MaxwellShell.exe.'
Check ($text -match 'Stop-LeftoverMaxwellShell' -and $text -match 'Disable-LeftoverShellTasks') 'System Initialization does not retire leftover Adaptive Dock processes or tasks.'
Check ($text -match "Install-MaxwellShellCore\.ps1") 'System Initialization does not keep Maxwell.Shell.Core as the telemetry owner.'
Check ($text -match "Wallpaper/F12" -and $text -match 'not-owned' -and $text -match "never") 'Wallpaper/F12 is still an Opal repair target that can invoke the retired guard.'
Check ($text -notmatch 'Ensure-WindhawkSafeDock' -or $text -match 'retiredGuardInvoked = \$false') 'System Initialization must not silently invoke the retired ChatGPT Guard path.'

$exports = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\opal-unified-exports.h'))
$shell = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\maxwell-shell.wh.cpp'))
$media = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\maxwell-opal-media.wh.cpp'))
Check ($exports -match 'bool OpalMedia_EnsureAttached\(\);') 'Media late-attach is missing from the unified export header.'
Check ($shell -match 'opal-unified-exports.h') 'Shell no longer includes the Media/Performance forward declarations.'
Check ($media -match 'TaskbarOccluded\(current\)' -and $media -match 'g_rootSizeToken' -and $media -match 'VisibleFullViewWindow') 'Media lost fullscreen remount or the SizeChanged layout watchers.'

$result = [pscustomobject]@{ passed = $failures.Count -eq 0; failures = @($failures) }
$result
if ($failures.Count) { throw "System Initialization owner validation failed: $($failures -join ' | ')" }
