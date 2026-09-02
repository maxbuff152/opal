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
