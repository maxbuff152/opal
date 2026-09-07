<# .SYNOPSIS Verifies that Windhawk is Opal's only settings surface. #>
[CmdletBinding()]
param([switch] $StaticOnly)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$failures = [Collections.Generic.List[string]]::new()
function Check([bool]$condition,[string]$message){ if(-not $condition){$failures.Add($message)} }
$shell = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\maxwell-shell.wh.cpp'))
$media = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\maxwell-opal-media.wh.cpp'))
$performance = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\maxwell-taskbar-system-info.wh.cpp'))
$control = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\opal-control.h'))
$builder = [IO.File]::ReadAllText((Join-Path $root 'Build-OpalSuite.ps1'))
$benchmark = [IO.File]::ReadAllText((Join-Path $root 'Measure-OpalPackageCost.ps1'))

Check ($shell -match '(?m)^// @id\s+opal$') 'Windhawk owner is not the single Opal mod.'
foreach($setting in @('leanMode','layoutMode','mediaEnabled','mediaMonitor','mediaFullDisplay','mediaSize','performanceEnabled','performanceMonitor','performanceFullDisplay','performanceSize','mirrorStyle','widgetTextSize','widgetBackgroundStrength','showArtwork','showArtist','hideWithoutSession','smoothProgress','temperatureUnit','showInlineGraphs','commandCenterEnabled','clockSize','resetWidgetPositions')){ Check ($shell -match "(?m)^\s*- ${setting}:") "Missing Windhawk setting $setting." }
Check ($shell -match 'OpalMedia_ModSettingsChanged' -and $shell -match 'OpalPerformance_ModSettingsChanged') 'Settings do not fan out live to both internal components.'
Check ($shell -match '\$name: Screens' -and $shell -match 'mediaMonitor: both' -and $shell -match 'performanceMonitor: both') 'Windhawk does not expose shared-data dual-monitor defaults.'
Check ($shell -notmatch '(?m)^\s*- enableTaskbar:') 'The taskbar is still a separate look toggle instead of Opal itself.'
Check ($shell -match 'case Host::Explorer:\s+return true;' -and $shell -match 'The Explorer taskbar is Opal') 'Explorer can still disable the Opal bar.'
Check ($shell -match '\$name: Start, Search, and notifications' -and $shell -notmatch '\$name: Windows look') 'Windows look still treats the taskbar as an optional skin.'
Check ($shell -match 'hideWithoutSession: false' -and $shell -match 'Reset layout' -and $shell -match 'IconSizeSmall: 28') 'Stable idle media, one-click reset, or one-scale tray defaults are missing.'
Check ($media -match 'VisibleFullViewWindow' -and $performance -match 'VisibleFullViewWindow') 'Visible-display attach is missing.'
Check ($control -match 'planned-explorer-restart' -and $performance -match 'EnsureShellCoreProcess') 'Planned Explorer restart or Core relaunch recovery is missing.'
$installer = [IO.File]::ReadAllText((Join-Path $root 'Install-OpalSuite.ps1'))
Check ($installer -match 'planned-explorer-restart' -and $installer -match 'Install-MaxwellShellCore\.ps1' -and $installer -match 'Stop-LeftoverMaxwellShell') 'Installer still skips Core autostart, leftover MaxwellShell.exe retirement, or planned Explorer restart marks.'
Check ($installer -match 'Initialize-MaxwellSystem\.ps1') 'Installer does not run System Initialization.'
Check ($shell -match 'opal-unified-exports.h' -and $media -match 'TaskbarOccluded\(current\)') 'Media forward declarations or fullscreen remount with layout watchers are missing.'
Check ($media -match 'ApplyMediaControlChange' -and $media -match 'Wh_GetIntSetting\(L"media\.mediaEnabled"\)') 'Media live toggle is missing.'
Check ($media -match 'InjectMediaMirror' -and $media -match 'OtherTaskbarWindows') 'Media lacks its lightweight second-display view.'
Check ($performance -match 'ApplyPerformanceControlChange' -and $performance -match 'Wh_GetIntSetting\(L"performance\.performanceEnabled"\)') 'Performance live toggle is missing.'
Check ($performance -match 'InjectPerformanceMirror' -and $performance -match 'OtherTaskbarWindows') 'Performance lacks its lightweight second-display view.'
Check ($control -notmatch 'ControlChanged|InstallControlListener|control\.ini') 'Retired cross-mod control plane remains.'
Check ($builder -match "localId = 'local@opal'" -and $builder -match 'components =') 'Build is not one DLL with internal components.'
Check ($benchmark -match 'ABBA' -and $benchmark -match 'BAAB') 'Randomized paired benchmark is missing.'
foreach($retired in @('native\Opal.Control\OpalControl.cpp','dist\Opal\Opal-ControlPanel.ps1')){ Check (-not(Test-Path -LiteralPath (Join-Path $root $retired))) "Retired second app returned: $retired" }

if(-not $StaticOnly){
    $ids=@(Get-ChildItem 'HKLM:\SOFTWARE\Windhawk\Engine\Mods' | ForEach-Object PSChildName)
    Check (@($ids).Count -eq 1 -and @($ids)[0] -eq 'local@opal') 'Live Windhawk inventory is not exactly one Opal mod.'
    Check (-not(Test-Path -LiteralPath (Join-Path $env:LOCALAPPDATA 'Maxwell\Opal\Opal-Control.exe'))) 'Standalone Opal executable remains installed.'
}
$result=[pscustomobject]@{passed=$failures.Count-eq 0;staticOnly=[bool]$StaticOnly;failures=@($failures)}
$result
if($failures.Count){throw "Unified control validation failed: $($failures -join ' | ')"}
