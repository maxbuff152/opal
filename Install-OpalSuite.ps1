<#
.SYNOPSIS
    Installs the single unified Opal mod and retires every old live mod.

.DESCRIPTION
    Creates a full rollback bundle, verifies the unified build hash, stops the
    Windhawk injection service, replaces the live registry/files/profile with
    the single canonical mod, restarts the shell, and verifies its DLL is mapped
    into Explorer. Existing Opal choices are carried forward across updates.
    Any failure after mutation triggers automatic restore.
#>
[CmdletBinding(SupportsShouldProcess)]
param()

$ErrorActionPreference = 'Stop'

$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = [Security.Principal.WindowsPrincipal]::new($identity)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'Administrator access is required to install the Opal suite.'
}

$modsRoot = 'HKLM:\SOFTWARE\Windhawk\Engine\Mods'
$writableRoot = 'HKLM:\SOFTWARE\Windhawk\Engine\ModsWritable'
$dll64Root = 'C:\ProgramData\Windhawk\Engine\Mods\64'
$dll32Root = 'C:\ProgramData\Windhawk\Engine\Mods\32'
$sourceRoot = 'C:\ProgramData\Windhawk\ModsSource'
$profilePath = 'C:\ProgramData\Windhawk\userprofile.json'
$windhawkExe = 'C:\Program Files\Windhawk\windhawk.exe'
$buildRoot = Join-Path $PSScriptRoot 'build\opal-suite'
$receiptPath = Join-Path $buildRoot 'build-receipt.json'
$controlInstallRoot = Join-Path $env:LOCALAPPDATA 'Maxwell\Opal'
$controlKey = 'HKCU:\Software\Maxwell\Opal'
$controlShortcut = Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs\Opal.lnk'

$expectedRoots = @(
    'C:\ProgramData\Windhawk\Engine\Mods\64',
    'C:\ProgramData\Windhawk\Engine\Mods\32',
    'C:\ProgramData\Windhawk\ModsSource'
)
foreach ($path in $expectedRoots) {
    if ([IO.Path]::GetFullPath($path).TrimEnd('\') -ne $path.TrimEnd('\')) {
        throw "Refusing an unresolved live target: $path"
    }
}
if (-not (Test-Path -LiteralPath $windhawkExe -PathType Leaf)) {
    throw "Windhawk runtime not found: $windhawkExe"
}

if (-not (Test-Path -LiteralPath $receiptPath)) { throw "Build receipt not found: $receiptPath" }
$build = @(Get-Content -LiteralPath $receiptPath -Raw | ConvertFrom-Json)
$expectedIds = @('local@opal')
if ($build.Count -ne 1 -or @($build | Where-Object localId -notin $expectedIds).Count) {
    throw 'Build receipt is not the single unified Opal mod.'
}
foreach ($item in $build) {
    if (-not (Test-Path -LiteralPath $item.output)) { throw "Built DLL is missing: $($item.output)" }
    if ((Get-FileHash -LiteralPath $item.output -Algorithm SHA256).Hash -ne $item.sha256) {
        throw "Built DLL hash mismatch: $($item.output)"
    }
    if (-not (Test-Path -LiteralPath $item.packageSource)) { throw "Packaged source is missing: $($item.packageSource)" }
    if ((Get-FileHash -LiteralPath $item.packageSource -Algorithm SHA256).Hash -ne $item.packageSourceSha256) {
        throw "Packaged source hash mismatch: $($item.packageSource)"
    }
}

$oldSettings = $null
$oldSettingsPath = Join-Path $modsRoot 'local@opal\Settings'
$oldShellSettingsPath = Join-Path $modsRoot 'local@opal-shell\Settings'
$legacySettingsPath = Join-Path $modsRoot 'local@maxwell-shell\Settings'
if (Test-Path -LiteralPath $oldSettingsPath) {
    $oldSettings = Get-ItemProperty -LiteralPath $oldSettingsPath
} elseif (Test-Path -LiteralPath $oldShellSettingsPath) {
    $oldSettings = Get-ItemProperty -LiteralPath $oldShellSettingsPath
} elseif (Test-Path -LiteralPath $legacySettingsPath) {
    $oldSettings = Get-ItemProperty -LiteralPath $legacySettingsPath
}
$oldClockSettings = $null
$oldClockSettingsPath = Join-Path $modsRoot 'local@opal-addon-clock\Settings'
if (Test-Path -LiteralPath $oldClockSettingsPath) {
    $oldClockSettings = Get-ItemProperty -LiteralPath $oldClockSettingsPath
}
$oldSystemInfoSettings = $null
$oldSystemInfoSettingsPath = Join-Path $modsRoot 'local@opal-addon-system-info\Settings'
if (Test-Path -LiteralPath $oldSystemInfoSettingsPath) {
    $oldSystemInfoSettings = Get-ItemProperty -LiteralPath $oldSystemInfoSettingsPath
}
function Get-OldValue([string] $Name, $Fallback) {
    if ($oldSettings -and $oldSettings.PSObject.Properties[$Name]) {
        return $oldSettings.PSObject.Properties[$Name].Value
    }
    return $Fallback
}
function Get-OldClockValue([string] $Name, $Fallback) {
    if ($oldClockSettings -and $oldClockSettings.PSObject.Properties[$Name]) {
        return $oldClockSettings.PSObject.Properties[$Name].Value
    }
    return Get-OldValue $Name $Fallback
}
function Get-OldSystemInfoValue([string] $Name, $Fallback) {
    if ($oldSystemInfoSettings -and $oldSystemInfoSettings.PSObject.Properties[$Name]) {
        return $oldSystemInfoSettings.PSObject.Properties[$Name].Value
    }
    return $Fallback
}

function Get-OldOrLegacyValue([string] $Name, [string] $LegacyName, $Fallback) {
    if ($oldSettings -and $oldSettings.PSObject.Properties[$Name]) {
        return $oldSettings.PSObject.Properties[$Name].Value
    }
    if ($oldSettings -and $oldSettings.PSObject.Properties[$LegacyName]) {
        return $oldSettings.PSObject.Properties[$LegacyName].Value
    }
    return $Fallback
}

$opalSettings = [ordered]@{
    'everyday.leanMode' = (Get-OldOrLegacyValue 'everyday.leanMode' 'leanMode' 1)
    'everyday.layoutMode' = (Get-OldOrLegacyValue 'everyday.layoutMode' 'layoutMode' 'automatic')
    'everyday.widgetTextSize' = (Get-OldOrLegacyValue 'everyday.widgetTextSize' 'widgetTextSize' 'standard')
    'everyday.widgetBackgroundStrength' = (Get-OldOrLegacyValue 'everyday.widgetBackgroundStrength' 'widgetBackgroundStrength' 'glass')
    'screens.mediaMonitor' = 'both'
    'screens.mediaFullDisplay' = (Get-OldOrLegacyValue 'screens.mediaFullDisplay' 'mediaFullDisplay' 'secondary')
    'screens.performanceMonitor' = 'both'
    'screens.performanceFullDisplay' = (Get-OldOrLegacyValue 'screens.performanceFullDisplay' 'performanceFullDisplay' 'primary')
    'screens.mirrorStyle' = (Get-OldOrLegacyValue 'screens.mirrorStyle' 'mirrorStyle' 'detailed')
    'media.mediaEnabled' = (Get-OldOrLegacyValue 'media.mediaEnabled' 'mediaEnabled' 1)
    'media.mediaSize' = (Get-OldOrLegacyValue 'media.mediaSize' 'mediaSize' 'standard')
    'media.showArtwork' = (Get-OldOrLegacyValue 'media.showArtwork' 'showArtwork' 1)
    'media.showArtist' = (Get-OldOrLegacyValue 'media.showArtist' 'showArtist' 1)
    'media.hideWithoutSession' = 0
    'media.smoothProgress' = (Get-OldOrLegacyValue 'media.smoothProgress' 'smoothProgress' 1)
    'performance.performanceEnabled' = (Get-OldOrLegacyValue 'performance.performanceEnabled' 'performanceEnabled' 1)
    'performance.performanceSize' = (Get-OldOrLegacyValue 'performance.performanceSize' 'performanceSize' 'standard')
    'performance.temperatureUnit' = (Get-OldOrLegacyValue 'performance.temperatureUnit' 'temperatureUnit' 'fahrenheit')
    'performance.showInlineGraphs' = (Get-OldOrLegacyValue 'performance.showInlineGraphs' 'showInlineGraphs' 0)
    'performance.commandCenterEnabled' = (Get-OldOrLegacyValue 'performance.commandCenterEnabled' 'commandCenterEnabled' 1)
    'windowsLook.enableTaskbar' = (Get-OldOrLegacyValue 'windowsLook.enableTaskbar' 'enableTaskbar' 1)
    'windowsLook.enableStart' = (Get-OldOrLegacyValue 'windowsLook.enableStart' 'enableStart' 1)
    'windowsLook.enableSearch' = (Get-OldOrLegacyValue 'windowsLook.enableSearch' 'enableSearch' 1)
    'windowsLook.enableNotifications' = (Get-OldOrLegacyValue 'windowsLook.enableNotifications' 'enableNotifications' 1)
    'windowsLook.enableMotion' = (Get-OldOrLegacyValue 'windowsLook.enableMotion' 'enableMotion' 1)
    'clock.ShowSeconds' = (Get-OldOrLegacyValue 'clock.ShowSeconds' 'ShowSeconds' 0)
    'clock.clockSize' = (Get-OldOrLegacyValue 'clock.clockSize' 'clockSize' 'standard')
    'clock.WebContentWeatherLocation' = (Get-OldOrLegacyValue 'clock.WebContentWeatherLocation' 'WebContentWeatherLocation' 'Katy, Texas')
    'advanced.clockFormatting.TimeFormat' = (Get-OldOrLegacyValue 'advanced.clockFormatting.TimeFormat' 'TimeFormat' "h':'mm")
    'advanced.clockFormatting.DateFormat' = (Get-OldOrLegacyValue 'advanced.clockFormatting.DateFormat' 'DateFormat' 'ddd, MMM d')
    'advanced.clockFormatting.TopLine' = (Get-OldOrLegacyValue 'advanced.clockFormatting.TopLine' 'TopLine' '%time%')
    'advanced.clockFormatting.BottomLine' = (Get-OldOrLegacyValue 'advanced.clockFormatting.BottomLine' 'BottomLine' '%date%  %weather%')
    'advanced.clockFormatting.TooltipLine' = (Get-OldOrLegacyValue 'advanced.clockFormatting.TooltipLine' 'TooltipLine' '%date% | %time% | BAT %battery% %battery_time% | DOWN %download_speed% UP %upload_speed%')
    'advanced.taskbarSizing.TaskbarHeight' = (Get-OldOrLegacyValue 'advanced.taskbarSizing.TaskbarHeight' 'TaskbarHeight' 68)
    'advanced.taskbarSizing.IconSize' = (Get-OldOrLegacyValue 'advanced.taskbarSizing.IconSize' 'IconSize' 38)
    'advanced.taskbarSizing.TaskbarButtonWidth' = (Get-OldOrLegacyValue 'advanced.taskbarSizing.TaskbarButtonWidth' 'TaskbarButtonWidth' 50)
    'advanced.taskbarSizing.IconSizeSmall' = 28
    'advanced.taskbarSizing.TaskbarButtonWidthSmall' = 42
    'advanced.repair.resetWidgetPositions' = 0
    'advanced.repair.resetCrashQuarantine' = 0
    'advanced.troubleshooting.diagnose' = (Get-OldOrLegacyValue 'advanced.troubleshooting.diagnose' 'diagnose' 0)
    'advanced.troubleshooting.logUnmatched' = (Get-OldOrLegacyValue 'advanced.troubleshooting.logUnmatched' 'logUnmatched' 0)
    WeekdayFormat = 'custom'; WeekdayFormatCustom = 'Sun, Mon, Tue, Wed, Thu, Fri, Sat'
    MiddleLine = ''
    TooltipLineMode = 'replace'; Width = 176; Height = 50; MaxWidth = 176
    TextSpacing = [uint32]4294967294
    'TimeStyle.TextColor' = '#FFF5F5F7'; 'TimeStyle.TextAlignment' = 'Center'
    'TimeStyle.FontSize' = 21; 'TimeStyle.FontFamily' = 'Segoe UI Variable Display'
    'TimeStyle.FontWeight' = 'SemiBold'; 'TimeStyle.FontStretch' = 'Normal'
    'TimeStyle.CharacterSpacing' = [uint32]4294967294; 'TimeStyle.LineHeight' = 23
    'DateStyle.Hidden' = 0; 'DateStyle.TextColor' = '#B8D1D1D6'
    'DateStyle.TextAlignment' = 'Center'; 'DateStyle.FontSize' = 11
    'DateStyle.FontFamily' = 'Segoe UI Variable Text'; 'DateStyle.FontWeight' = 'Medium'
    'DateStyle.FontStretch' = 'Normal'; 'DateStyle.CharacterSpacing' = 0
    'DateStyle.LineHeight' = 13
    WebContentWeatherFormat = '%c %t'; WebContentWeatherUnits = 'uscs'
    WebContentsUpdateInterval = 10; 'WebContentsItems[0].Url' = ''
    'DataCollection.UpdateInterval' = 30
    'DataCollection.NetworkMetricsFormat' = 'mbsDynamic'
    'DataCollection.NetworkMetricsFixedDecimals' = 1
    'DataCollection.NetworkIdleThresholdKBps' = 2
}
$mediaSettings = [ordered]@{
    minimumWidth = 232
    preferredWidth = 304
    maximumWidth = 336
    height = 50
}
$systemInfoSettings = [ordered]@{
    width = 184; leftOffset = 10; reserveSpace = 1; reserveGap = 8
    updateInterval = 2; gamingUpdateInterval = 10; batterySaverUpdateInterval = 15
    performanceAuraEnabled = 0; activityRailEnabled = 1
    adaptiveOverlapEnabled = 1; contentPriorityEnabled = 1
    experienceMode = 'auto'; historySeconds = 60
    fontSize = 13; fontFamily = 'Segoe UI Variable Text'; textColor = '#FFF5F5F7'
    graphColor = '#D6D6D8'; safeColor = '#FFA8A8AD'; warningColor = '#FFC7C7CC'
    criticalColor = '#FFF5F5F7'; textOpacity = 96
    computeWarningPercent = 75; computeCriticalPercent = 90
    cpuWarningTemp = 75; cpuCriticalTemp = 85; gpuWarningTemp = 80; gpuCriticalTemp = 90
    memoryWarningPercent = 80; memoryCriticalPercent = 90
    gpuAdapter = [string](Get-OldSystemInfoValue 'gpuAdapter' '')
    temperatureSource = [string](Get-OldSystemInfoValue 'temperatureSource' 'auto')
    windowsThermalZoneFilter = [string](Get-OldSystemInfoValue 'windowsThermalZoneFilter' '')
    windowsThermalZoneAggregation = [string](Get-OldSystemInfoValue 'windowsThermalZoneAggregation' 'average')
    cpuTempSensor = [string](Get-OldSystemInfoValue 'cpuTempSensor' '')
    gpuTempSensor = [string](Get-OldSystemInfoValue 'gpuTempSensor' '')
}

foreach ($entry in $mediaSettings.GetEnumerator()) { $opalSettings[$entry.Key] = $entry.Value }
foreach ($entry in $systemInfoSettings.GetEnumerator()) { $opalSettings[$entry.Key] = $entry.Value }

# An update must not silently turn a user's Windhawk choices back into installer
# defaults. Preserve every setting understood by the current package, while
# forcing the two one-shot recovery switches back off so they cannot replay on
# the next install.
$oneShotSettings = @(
    'advanced.repair.resetWidgetPositions',
    'advanced.repair.resetCrashQuarantine'
)
$preservedSettingNames = [Collections.Generic.List[string]]::new()
if ($oldSettings) {
    foreach ($name in @($opalSettings.Keys)) {
        if ($name -in $oneShotSettings) { continue }
        $property = $oldSettings.PSObject.Properties[[string]$name]
        if ($property) {
            $opalSettings[$name] = $property.Value
            $preservedSettingNames.Add([string]$name)
        }
    }
}
foreach ($name in $oneShotSettings) { $opalSettings[$name] = 0 }

$settingsById = @{ 'local@opal' = $opalSettings }

function Set-RegistryValue([string] $Path, [string] $Name, $Value) {
    $type = if ($Value -is [byte] -or $Value -is [int16] -or $Value -is [int32] -or
                   $Value -is [uint16] -or $Value -is [uint32] -or $Value -is [bool]) {
        'DWord'
    } else {
        'String'
    }
    New-ItemProperty -LiteralPath $Path -Name $Name -Value $Value -PropertyType $type -Force | Out-Null
}

function Remove-RetiredOpalControlSurface {
    foreach ($path in @(
        (Join-Path $controlInstallRoot 'Opal-Control.exe'),
        (Join-Path $controlInstallRoot 'Opal-ControlPanel.ps1'),
        (Join-Path $controlInstallRoot 'Opal-Companion.ps1'),
        (Join-Path $controlInstallRoot 'Set-OpalAccent.ps1'),
        (Join-Path $controlInstallRoot 'control.ini'),
        $controlShortcut
    )) {
        if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path -Force }
    }
    if (Test-Path -LiteralPath $controlKey) {
        foreach ($name in @('LeanMode','MediaEnabled','PerformanceEnabled','MediaMonitor','PerformanceMonitor','Generation')) {
            Remove-ItemProperty -LiteralPath $controlKey -Name $name -ErrorAction SilentlyContinue
        }
    }
}

function Clear-OpalIntentionalRestartMarkers {
    New-Item -ItemType Directory -Path $controlInstallRoot -Force | Out-Null
    foreach ($package in @('media', 'performance')) {
        @('[Health]', 'Dirty=0', 'CrashCount=0', 'Quarantined=0', 'Reason=') |
            Set-Content -LiteralPath (Join-Path $controlInstallRoot "health-$package.ini") -Encoding ASCII
    }
}

function Stop-LeftoverMaxwellShell {
    # Adaptive Dock / old MaxwellShell.exe is not Opal. Never stop Maxwell.Shell.Core.exe.
    foreach ($process in @(Get-CimInstance Win32_Process -Filter "Name='MaxwellShell.exe'" -ErrorAction SilentlyContinue)) {
        $path = [string]$process.ExecutablePath
        if ($path -and ([IO.Path]::GetFileName($path) -ieq 'Maxwell.Shell.Core.exe')) { continue }
        Stop-Process -Id $process.ProcessId -Force -ErrorAction SilentlyContinue
        Wait-Process -Id $process.ProcessId -Timeout 8 -ErrorAction SilentlyContinue
    }
    Get-Process -Name MaxwellShell -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue

    $runKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run'
    if (Test-Path -LiteralPath $runKey) {
        foreach ($name in @((Get-Item -LiteralPath $runKey).Property)) {
            if ($name -eq 'MaxwellShellCore') { continue }
            $value = [string](Get-ItemPropertyValue -LiteralPath $runKey -Name $name -ErrorAction SilentlyContinue)
            if ($value -match '(?i)MaxwellShell\.exe' -and $value -notmatch '(?i)Maxwell\.Shell\.Core\.exe') {
                Remove-ItemProperty -LiteralPath $runKey -Name $name -ErrorAction SilentlyContinue
            }
        }
    }

    foreach ($dir in @(
        (Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs\Startup'),
        (Join-Path $env:ProgramData 'Microsoft\Windows\Start Menu\Programs\StartUp')
    )) {
        if (-not (Test-Path -LiteralPath $dir)) { continue }
        foreach ($item in @(Get-ChildItem -LiteralPath $dir -File -ErrorAction SilentlyContinue)) {
            $leftover = $item.Name -match '(?i)MaxwellShell'
            if (-not $leftover -and $item.Extension -eq '.lnk') {
                try {
                    $shortcut = (New-Object -ComObject WScript.Shell).CreateShortcut($item.FullName)
                    $leftover = [string]$shortcut.TargetPath -match '(?i)(?:^|[\\/])MaxwellShell\.exe$'
                } catch { }
            }
            if ($leftover) { Remove-Item -LiteralPath $item.FullName -Force -ErrorAction SilentlyContinue }
        }
    }
}

function Restart-OpalShell {
    New-Item -ItemType Directory -Path $controlInstallRoot -Force | Out-Null
    New-Item -ItemType File -Force -Path (Join-Path $controlInstallRoot 'planned-explorer-restart') | Out-Null
    Stop-Service -Name Windhawk -Force -ErrorAction SilentlyContinue
    foreach ($name in @('explorer', 'StartMenuExperienceHost', 'SearchHost', 'SearchApp', 'ShellExperienceHost', 'ShellHost')) {
        Stop-Process -Name $name -Force -ErrorAction SilentlyContinue
    }
    Start-Sleep -Seconds 2
}

$oldInventory = [pscustomobject]@{
    registry = @(
        if (Test-Path -LiteralPath $modsRoot) { Get-ChildItem -LiteralPath $modsRoot | ForEach-Object PSChildName }
    )
    dll64 = @(Get-ChildItem -LiteralPath $dll64Root -File -ErrorAction SilentlyContinue | ForEach-Object Name)
    dll32 = @(Get-ChildItem -LiteralPath $dll32Root -File -ErrorAction SilentlyContinue | ForEach-Object Name)
    sources = @(Get-ChildItem -LiteralPath $sourceRoot -File -ErrorAction SilentlyContinue | ForEach-Object Name)
}

if (-not $PSCmdlet.ShouldProcess('Windhawk live state', 'install the single unified Opal mod')) { return }

$rollbackStarted = Get-Date
& (Join-Path $PSScriptRoot 'New-MaxwellShellRollbackPoint.ps1') -Note 'before unified one-mod Opal install'
$rollbackRoots = @(
    (Join-Path $env:LOCALAPPDATA 'Maxwell\Opal')
    (Join-Path $env:LOCALAPPDATA 'Maxwell\WindhawkChatGPTGuard')
)
$rollbackBundle = $null
foreach ($rollbackRoot in $rollbackRoots) {
    if (-not (Test-Path -LiteralPath $rollbackRoot)) { continue }
    $rollbackBundle = Get-ChildItem -LiteralPath $rollbackRoot -Directory -Filter 'rollback-*' |
        Where-Object LastWriteTime -ge $rollbackStarted.AddSeconds(-2) |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    if ($rollbackBundle) { break }
}
if (-not $rollbackBundle -or -not (Test-Path -LiteralPath (Join-Path $rollbackBundle.FullName 'manifest.json'))) {
    throw 'Fresh rollback bundle was not verified; refusing to mutate Windhawk.'
}

$mutationStarted = $false
$installedDlls = [Collections.Generic.List[string]]::new()
$installedSources = [Collections.Generic.List[string]]::new()
try {
    $mutationStarted = $true
    Remove-RetiredOpalControlSurface
    Clear-OpalIntentionalRestartMarkers
    Stop-LeftoverMaxwellShell
    Restart-OpalShell

    foreach ($root in @($modsRoot, $writableRoot)) {
        if (-not (Test-Path -LiteralPath $root)) { New-Item -Path $root -Force | Out-Null }
        foreach ($child in @(Get-ChildItem -LiteralPath $root -ErrorAction SilentlyContinue)) {
            if ($child.PSParentPath -notlike '*\Windhawk\Engine\Mods*') { throw "Unexpected registry parent: $($child.Name)" }
            Remove-Item -LiteralPath $child.PSPath -Recurse -Force
        }
    }

    $protectedRuntimeFiles = @('libc++.whl', 'libunwind.whl', 'windhawk-mod-shim.dll')
    foreach ($root in @($dll64Root, $dll32Root)) {
        foreach ($file in @(Get-ChildItem -LiteralPath $root -File -ErrorAction SilentlyContinue)) {
            if ($file.Name -notin $protectedRuntimeFiles) { Remove-Item -LiteralPath $file.FullName -Force }
        }
    }
    foreach ($file in @(Get-ChildItem -LiteralPath $sourceRoot -File -ErrorAction SilentlyContinue)) {
        Remove-Item -LiteralPath $file.FullName -Force
    }

    $changeTime = [int][DateTimeOffset]::UtcNow.ToUnixTimeSeconds()
    foreach ($item in $build) {
        $dllDest = Join-Path $dll64Root $item.dllName
        Copy-Item -LiteralPath $item.output -Destination $dllDest -Force
        $installedDlls.Add($dllDest)
        $sourceName = '{0}.wh.cpp' -f $item.localId
        $sourceDest = Join-Path $sourceRoot $sourceName
        Copy-Item -LiteralPath $item.packageSource -Destination $sourceDest -Force
        $installedSources.Add($sourceDest)

        foreach ($root in @($modsRoot, $writableRoot)) {
            $key = Join-Path $root $item.localId
            $settingsKey = Join-Path $key 'Settings'
            New-Item -Path $settingsKey -Force | Out-Null
            Set-RegistryValue $key 'LibraryFileName' $item.dllName
            Set-RegistryValue $key 'Include' $item.include
            Set-RegistryValue $key 'Exclude' ''
            Set-RegistryValue $key 'Architecture' 'x86-64'
            Set-RegistryValue $key 'Version' $item.version
            Set-RegistryValue $key 'Disabled' 0
            Set-RegistryValue $key 'SettingsChangeTime' $changeTime
            foreach ($entry in $settingsById[$item.localId].GetEnumerator()) {
                Set-RegistryValue $settingsKey ([string]$entry.Key) $entry.Value
            }
        }
    }

    $profile = Get-Content -LiteralPath $profilePath -Raw | ConvertFrom-Json
    $canonicalMods = [ordered]@{}
    foreach ($item in $build) {
        $canonicalMods[$item.metadataId] = [ordered]@{ disabled = $false; version = $item.version }
    }
    $profile.mods = [pscustomobject]$canonicalMods
    $profile | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $profilePath -Encoding UTF8

    Start-Service -Name Windhawk
    # The service owns privileged injection, but the per-user Windhawk runtime
    # owns the active session. A service-only restart leaves Explorer stock and
    # makes a valid Opal DLL look as though it failed to load.
    Start-Process -FilePath $windhawkExe -ArgumentList @('-restart', '-tray-only') `
        -WindowStyle Hidden -Wait
    $runtimeDeadline = (Get-Date).AddSeconds(15)
    do {
        Start-Sleep -Milliseconds 500
        $windhawkRuntime = Get-Process -Name windhawk -ErrorAction SilentlyContinue |
            Select-Object -First 1
    } until ($windhawkRuntime -or (Get-Date) -ge $runtimeDeadline)
    if (-not $windhawkRuntime) {
        throw 'Windhawk user runtime did not restart.'
    }
    $interactiveSession = (Get-Process -Id $PID).SessionId
    if (-not (Get-Process -Name explorer -ErrorAction SilentlyContinue |
            Where-Object SessionId -eq $interactiveSession)) {
        Start-Process -FilePath "$env:WINDIR\explorer.exe"
    }

    $deadline = (Get-Date).AddSeconds(25)
    $loadedNames = @()
    do {
        Start-Sleep -Seconds 2
        $explorer = Get-Process -Name explorer -ErrorAction SilentlyContinue |
            Where-Object SessionId -eq $interactiveSession | Select-Object -First 1
        if ($explorer) {
            try { $loadedNames = @($explorer.Modules | ForEach-Object ModuleName) } catch { $loadedNames = @() }
        }
    } until ($explorer -and @($build | Where-Object { $_.dllName -notin $loadedNames }).Count -eq 0 -or (Get-Date) -ge $deadline)

    if (-not $explorer) { throw 'Explorer did not restart.' }
    $missing = @($build | Where-Object { $_.dllName -notin $loadedNames } | ForEach-Object dllName)
    if ($missing.Count) { throw "Opal DLLs did not load in Explorer: $($missing -join ', ')" }

    $coreInstall = & (Join-Path $PSScriptRoot 'Install-MaxwellShellCore.ps1') -Action Install
    if (-not $coreInstall -or -not $coreInstall.succeeded) {
        throw 'Maxwell.Shell.Core did not install or stay running.'
    }
    if (Get-Process -Name MaxwellShell -ErrorAction SilentlyContinue) {
        throw 'Leftover MaxwellShell.exe is still running after Opal install.'
    }

    $liveIds = @(Get-ChildItem -LiteralPath $modsRoot | ForEach-Object PSChildName | Sort-Object)
    if (Compare-Object ($expectedIds | Sort-Object) $liveIds) { throw 'Live registry is not exactly one Opal mod.' }

    $installReceipt = [pscustomobject]@{
        installedUtc = (Get-Date).ToUniversalTime().ToString('o')
        rollbackBundle = $rollbackBundle.FullName
        oldInventory = $oldInventory
        installed = @($build | Select-Object localId, metadataId, version, dllName, sha256)
        explorerPid = $explorer.Id
        verifiedLoaded = @($build | ForEach-Object dllName)
        leftoverMaxwellShellRetired = $true
        coreInstall = $coreInstall
        settingsOwner = 'Windhawk local@opal'
        preservedSettingCount = $preservedSettingNames.Count
        preservedSettingNames = @($preservedSettingNames)
    }
    $installReceipt | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $buildRoot 'install-receipt.json') -Encoding UTF8
    $installReceipt
}
catch {
    $failure = $_
    if ($mutationStarted) {
        try {
            # Explorer maps the Windhawk runtime files. Stop every owning shell
            # host before the rollback script copies the verified bundle back.
            Clear-OpalIntentionalRestartMarkers
            Restart-OpalShell
            foreach ($path in @($installedDlls) + @($installedSources)) {
                if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path -Force -ErrorAction SilentlyContinue }
            }
            & (Join-Path $PSScriptRoot 'Restore-MaxwellShellRollbackPoint.ps1') -Bundle $rollbackBundle.FullName -NoSafety
        } catch {
            throw "Opal install failed: $($failure.Exception.Message). Automatic rollback also failed: $($_.Exception.Message)"
        }
    }
    throw "Opal install failed and was rolled back: $($failure.Exception.Message)"
}
