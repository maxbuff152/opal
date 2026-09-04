<#
.SYNOPSIS
    Source contracts for visible-display attach, compact-not-hide, stable
    media lanes, in-capsule wide, crash-free recovery, and one-click reset.
#>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$failures = [Collections.Generic.List[string]]::new()
function Check([bool]$condition, [string]$message) {
    if (-not $condition) { $failures.Add($message) }
}

$shell = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\maxwell-shell.wh.cpp'))
$media = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\maxwell-opal-media.wh.cpp'))
$performance = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\maxwell-taskbar-system-info.wh.cpp'))
$control = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\opal-control.h'))
$icons = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\opal-addon-icons.h'))
$installer = [IO.File]::ReadAllText((Join-Path $root 'Install-OpalSuite.ps1'))

Check ($control -match 'VisibleFullViewWindow' -and $control -match 'TaskbarOccluded') 'Visible full-view selection is missing.'
Check ($media -match 'VisibleFullViewWindow' -and $performance -match 'VisibleFullViewWindow') 'Media or Performance still attach only to the occluded primary bar.'
Check ($performance -match 'g_widget != nullptr \|\| !g_performanceMirrors.empty\(\)') 'Performance attach still requires the primary widget instead of a visible mirror.'
Check ($performance -notmatch 'return ContentPriority::Hidden') 'Tight taskbars still hide Computer stats.'
Check ($icons -match 'IconSizeSmall: 28' -or $shell -match 'IconSizeSmall: 28') 'Tray icons are not one-scale with app icons.'
Check ($icons -match 'iconSizeSmall = 28' -and $icons -match 'iconSizeSmall < 24') 'Reset layout does not restore tray icon scale.'
Check ($shell -match 'Reset layout' -and $media -match 'ForceCanonicalLayout' -and $performance -match 'ForceCanonicalLayout') 'One-click reset does not restore canonical geometry.'
Check ($control -match 'neverLived' -and $control -match 'planned-explorer-restart') 'Crash protection still counts never-lived or planned Explorer restarts.'
Check ($performance -match 'EnsureShellCoreProcess') 'Missing Core relaunch.'
Check ($media -match 'wideInsideCapsule' -and $media -match 'Nothing playing') 'Wide media still shoves the bar or idle media still vanishes.'
Check ($shell -match 'hideWithoutSession: false' -and $installer -match "'media.hideWithoutSession' = 0") 'Installer still hides the media lane when idle.'
Check ($installer -match "'screens.mediaMonitor' = 'both'" -and $installer -match "'screens.performanceMonitor' = 'both'") 'Installer does not default both screens.'
Check ($installer -match 'planned-explorer-restart') 'Installer still restarts Explorer without the planned-restart marker.'
Check ($installer -match 'Stop-LeftoverMaxwellShell' -and $installer -match 'MaxwellShell\.exe') 'Installer still leaves Adaptive Dock MaxwellShell.exe running.'
Check ($installer -match "Install-MaxwellShellCore\.ps1") 'Installer does not install Maxwell.Shell.Core autostart.'
Check ($installer -notmatch 'Start-Process[^\r\n]*MaxwellShell\.exe') 'Installer must not start leftover MaxwellShell.exe.'
Check ($shell -match '15000' -and $shell -match 'late attach still waiting') 'Late attach still gives up after 60 seconds.'
Check ($shell -notmatch '(?m)^\s*- enableTaskbar:' -and $shell -match 'The Explorer taskbar is Opal') 'Opal and the taskbar are still separate products.'

$result = [pscustomobject]@{ passed = $failures.Count -eq 0; failures = @($failures) }
$result
if ($failures.Count) { throw "Taskbar resilience validation failed: $($failures -join ' | ')" }
