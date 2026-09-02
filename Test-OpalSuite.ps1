<# .SYNOPSIS Validates the single unified Opal Windhawk mod. #>
[CmdletBinding()]
param([switch] $StaticOnly)

$ErrorActionPreference = 'Stop'
$buildRoot = Join-Path $PSScriptRoot 'build\opal-suite'
$receiptPath = Join-Path $buildRoot 'build-receipt.json'
$modsRoot = 'HKLM:\SOFTWARE\Windhawk\Engine\Mods'
$writableRoot = 'HKLM:\SOFTWARE\Windhawk\Engine\ModsWritable'
$dll64Root = 'C:\ProgramData\Windhawk\Engine\Mods\64'
$dll32Root = 'C:\ProgramData\Windhawk\Engine\Mods\32'
$sourceRoot = 'C:\ProgramData\Windhawk\ModsSource'
$profilePath = 'C:\ProgramData\Windhawk\userprofile.json'
$expectedId = 'local@opal'
$expectedMetadataId = 'opal'
$expectedVersion = '4.4.0'
$failures = [Collections.Generic.List[string]]::new()

function Assert-Opal([bool] $Condition, [string] $Message) {
    if (-not $Condition) { $failures.Add($Message) }
}
function Metadata([string] $Path, [string] $Name) {
    $line = Get-Content -LiteralPath $Path -TotalCount 90 |
        Where-Object { $_ -match "^//\s+@$([regex]::Escape($Name))\s+(.*)$" } |
        Select-Object -First 1
    if ($line -match "^//\s+@$([regex]::Escape($Name))\s+(.*)$") { $Matches[1].Trim() }
}

Assert-Opal (Test-Path -LiteralPath $receiptPath) "Missing build receipt: $receiptPath"
$build = if (Test-Path -LiteralPath $receiptPath) {
    @(Get-Content -LiteralPath $receiptPath -Raw | ConvertFrom-Json)
} else { @() }
Assert-Opal ($build.Count -eq 1) "Expected one Opal build entry, found $($build.Count)."
if ($build.Count -eq 1) {
    $item = $build[0]
    Assert-Opal ($item.localId -eq $expectedId) "Build ID is $($item.localId), not $expectedId."
    Assert-Opal ($item.metadataId -eq $expectedMetadataId) 'Build metadata ID is not opal.'
    Assert-Opal ($item.version -eq $expectedVersion) 'Build version mismatch.'
    Assert-Opal (@($item.components).Count -eq 3) 'Unified DLL does not contain exactly Shell, Media, and Performance components.'
    Assert-Opal ((@($item.components.name) -join ',') -eq 'shell,media,performance') 'Unified component order or ownership is wrong.'
    Assert-Opal (Test-Path -LiteralPath $item.output) "Missing unified DLL: $($item.output)"
    if (Test-Path -LiteralPath $item.output) {
        Assert-Opal ((Get-FileHash -LiteralPath $item.output -Algorithm SHA256).Hash -eq $item.sha256) 'Unified DLL hash mismatch.'
    }
    Assert-Opal (Test-Path -LiteralPath $item.packageSource) 'Packaged Opal source is missing.'
    if (Test-Path -LiteralPath $item.packageSource) {
        Assert-Opal ((Metadata $item.packageSource 'id') -eq $expectedMetadataId) 'Packaged metadata ID is not opal.'
        Assert-Opal ((Metadata $item.packageSource 'version') -eq $expectedVersion) 'Packaged version mismatch.'
        Assert-Opal (-not [regex]::IsMatch([IO.File]::ReadAllText($item.packageSource), '(?m)^\s*#include\s+"[^"]+"')) 'Packaged source has a local include.'
    }
}

$builtDlls = @(Get-ChildItem -LiteralPath $buildRoot -File -Filter 'local_at_opal*_owned.dll' -ErrorAction SilentlyContinue)
$packagedSources = @(Get-ChildItem -LiteralPath (Join-Path $buildRoot 'sources') -File -Filter 'opal*.wh.cpp' -ErrorAction SilentlyContinue)
Assert-Opal ($builtDlls.Count -eq 1) "Build tree contains $($builtDlls.Count) Opal DLLs instead of one."
Assert-Opal ($packagedSources.Count -eq 1) "Build tree contains $($packagedSources.Count) Opal package sources instead of one."
Assert-Opal (-not (Test-Path -LiteralPath (Join-Path $buildRoot 'Opal-Control.exe'))) 'Standalone Opal controller returned.'
Assert-Opal (-not (Test-Path -LiteralPath (Join-Path $PSScriptRoot 'native\Opal.Control\OpalControl.cpp'))) 'Standalone controller source returned.'

$shell = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'mod\visual-clones\maxwell-shell.wh.cpp'))
$media = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'mod\visual-clones\maxwell-opal-media.wh.cpp'))
$performance = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'mod\visual-clones\maxwell-taskbar-system-info.wh.cpp'))
$builder = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'Build-OpalSuite.ps1'))
foreach ($setting in @('leanMode','layoutMode','mediaEnabled','mediaMonitor','mediaFullDisplay','mediaSize','performanceEnabled','performanceMonitor','performanceFullDisplay','performanceSize','mirrorStyle','widgetTextSize','widgetBackgroundStrength','showArtwork','showArtist','hideWithoutSession','smoothProgress','temperatureUnit','showInlineGraphs','commandCenterEnabled','clockSize','resetWidgetPositions','resetCrashQuarantine')) {
    Assert-Opal ($shell -match "(?m)^\s*- ${setting}:") "Windhawk Opal settings are missing $setting."
}
Assert-Opal ($shell -match 'OpalMedia_ModSettingsChanged' -and $shell -match 'OpalPerformance_ModSettingsChanged') 'The one Windhawk settings handler does not fan out to Media and Performance.'
Assert-Opal ($builder -match 'DWh_ModInit=\{0\}_ModInit' -and $builder -match 'local@opal') 'Builder does not link internal components into one mod.'
Assert-Opal ($media -match 'Wh_GetIntSetting\(L"media\.mediaEnabled"\)' -and $media -notmatch 'InstallControlListener') 'Media still uses a second control plane.'
Assert-Opal ($performance -match 'Wh_GetIntSetting\(L"performance\.performanceEnabled"\)' -and $performance -notmatch 'InstallControlListener') 'Performance still uses a second control plane.'
Assert-Opal ($media -match 'stream\.Close\(\)' -and $media -match 'targetPixels') 'Media artwork allocation improvements are missing.'
Assert-Opal ($performance -match 'PdhCollectQueryDataEx' -and $performance -match 'PerformanceSuspensionReason') 'Event-driven or suspension-aware performance sampling is missing.'
Assert-Opal ($performance -match 'ResetCommandCenterView\(\)' -and $performance -match 'g_commandCenterFlyout = nullptr') 'Command center is not destroyed on close.'

$live = $null
if (-not $StaticOnly) {
    $liveIds = if (Test-Path -LiteralPath $modsRoot) { @(Get-ChildItem -LiteralPath $modsRoot | ForEach-Object PSChildName) } else { @() }
    $writableIds = if (Test-Path -LiteralPath $writableRoot) { @(Get-ChildItem -LiteralPath $writableRoot | ForEach-Object PSChildName) } else { @() }
    Assert-Opal (@($liveIds).Count -eq 1 -and @($liveIds)[0] -eq $expectedId) 'Windhawk Mods registry is not exactly one Opal mod.'
    Assert-Opal (@($writableIds).Count -eq 1 -and @($writableIds)[0] -eq $expectedId) 'Windhawk ModsWritable registry is not exactly one Opal mod.'
    $key = Join-Path $modsRoot $expectedId
    $settingsKey = Join-Path $key 'Settings'
    if (Test-Path -LiteralPath $key) {
        $properties = Get-ItemProperty -LiteralPath $key
        Assert-Opal ([string]$properties.Version -eq $expectedVersion) 'Live Opal version mismatch.'
        Assert-Opal ([int]$properties.Disabled -eq 0) 'Live Opal is disabled.'
        $settings = Get-ItemProperty -LiteralPath $settingsKey
        foreach ($name in @('everyday.leanMode','media.mediaEnabled','screens.mediaMonitor','performance.performanceEnabled','screens.performanceMonitor')) {
            Assert-Opal ($null -ne $settings.PSObject.Properties[$name]) "Live Opal settings are missing $name."
        }
    }
    $profile = Get-Content -LiteralPath $profilePath -Raw | ConvertFrom-Json
    $profileModIds = @($profile.mods.PSObject.Properties | ForEach-Object Name)
    if ($profileModIds.Count -gt 0) {
        Assert-Opal ($profileModIds.Count -eq 1 -and $profileModIds -contains $expectedMetadataId) 'Windhawk profile contains mod entries that conflict with the unified Opal installation.'
    }
    $expectedDll = if ($build.Count) { $build[0].dllName } else { '' }
    $explorer = Get-Process explorer | Sort-Object StartTime -Descending | Select-Object -First 1
    $modules = @($explorer.Modules | ForEach-Object ModuleName)
    Assert-Opal ($expectedDll -in $modules) 'Explorer has not loaded the unified Opal DLL.'
    Assert-Opal (-not (Test-Path -LiteralPath (Join-Path $env:LOCALAPPDATA 'Maxwell\Opal\Opal-Control.exe'))) 'Standalone Opal app is still installed.'
    Assert-Opal (-not (Test-Path -LiteralPath (Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs\Opal.lnk'))) 'Standalone Opal shortcut is still installed.'
    $live = [pscustomobject]@{ registryIds=$liveIds; profileModIds=$profileModIds; explorerPid=$explorer.Id; module=$expectedDll }
}

$result = [pscustomobject]@{ passed=$failures.Count -eq 0; staticOnly=[bool]$StaticOnly; checkedUtc=(Get-Date).ToUniversalTime().ToString('o'); failures=@($failures); live=$live }
$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $buildRoot 'test-receipt.json') -Encoding UTF8
$result
if ($failures.Count) { throw "Opal validation failed: $($failures -join ' | ')" }
