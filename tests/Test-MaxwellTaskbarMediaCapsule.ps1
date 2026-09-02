<#
.SYNOPSIS
    Regression checks for the Opal Media taskbar capsule.

.DESCRIPTION
    Proves the source-level interaction, timeline, accessibility, and ownership
    contracts for Opal Media. Without -StaticOnly it also verifies that the
    canonical package is enabled, source-synchronized, and loaded in Explorer.

    Read-only. Does not modify settings, registry, or Explorer.
#>
[CmdletBinding()]
param(
    [switch] $StaticOnly
)

$ErrorActionPreference = 'Stop'
$modId = 'local@opal'
$metadataId = 'opal-addon-media'
$expectedComponentVersion = '4.4.0'
$expectedUnifiedVersion = '4.4.0'
$root = Split-Path -Parent $PSScriptRoot
$master = Join-Path $root 'mod\visual-clones\maxwell-opal-media.wh.cpp'
$deployed = "C:\ProgramData\Windhawk\ModsSource\$modId.wh.cpp"
$modsRoot = 'HKLM:\SOFTWARE\Windhawk\Engine\Mods'

$failures = [Collections.Generic.List[string]]::new()
function Assert-True([bool] $Condition, [string] $Message) {
    if (-not $Condition) { $failures.Add($Message) }
}

Assert-True (Test-Path -LiteralPath $master) "Master source is missing: $master"
if (Test-Path -LiteralPath $master) {
    $text = [IO.File]::ReadAllText($master)
    Assert-True ($text -match "(?m)^//\s+@id\s+$([regex]::Escape($metadataId))\s*$") `
        "Master metadata ID is not $metadataId."
    Assert-True ($text -match "(?m)^//\s+@version\s+$([regex]::Escape($expectedComponentVersion))\s*$") `
        "Media component version is not $expectedComponentVersion."
    Assert-True ($text -match 'GlobalSystemMediaTransportControlsSessionManager') `
        'The native Windows media-session manager is missing.'
    Assert-True ($text -match 'CurrentSessionChanged' -and $text -match 'SessionsChanged' -and
        $text -match 'MediaPropertiesChanged' -and $text -match 'PlaybackInfoChanged' -and
        $text -match 'TimelinePropertiesChanged') `
        'Media refresh is no longer event-driven across manager and session changes.'
    Assert-True ($text -match 'EnsureSessionManager\(\)' -and
        $text -match 'ReleaseSessionManager\(\)' -and
        $text -match 'refreshed \? INFINITE : kWorkerRetryMs') `
        'Media manager startup or transient refresh failures are no longer self-healing.'
    Assert-True ($text -match 'start100ns\s*=\s*timeline\.StartTime\(\)\.count\(\)') `
        'Timeline start is not captured; non-zero media timelines will render the wrong progress.'
    Assert-True ($text -match 'position\s*-\s*g_uiSnapshot\.start100ns' -and
        $text -match 'end100ns\s*-\s*g_uiSnapshot\.start100ns') `
        'Progress is not normalized against the real timeline start and duration.'
    Assert-True ($text -match 'kPlayingUiIntervalMs\s*=\s*250' -and
        $text -match 'kLeanPlayingUiIntervalMs\s*=\s*1000' -and
        $text -match 'kPausedUiIntervalMs\s*=\s*1000' -and
        $text -match 'kIdleUiIntervalMs\s*=\s*2000' -and
        $text -match 'g_leanMode\s*\?\s*kLeanPlayingUiIntervalMs') `
        'The progress rail lost its full, Lean, paused, or idle adaptive cadence.'
    Assert-True ($text -match 'kLeanIdleReleaseMs' -and
        $text -match 'AttachMediaVisuals' -and $text -match 'g_mediaEnabled' -and
        $text -match 'g_leanMode') `
        'Media no longer lazily owns its XAML tree or honors unified Opal controls.'
    Assert-True ($text -match 'PointerWheelChanged' -and $text -match 'VK_VOLUME_(UP|DOWN)') `
        'Capsule scroll-to-volume is missing.'
    Assert-True ($text -match 'TryChangePlaybackPositionAsync' -and
        $text -match 'MediaAction::SeekBackward' -and $text -match 'MediaAction::SeekForward' -and
        $text -match 'GetKeyState\(VK_SHIFT\)') `
        'Bidirectional ten-second seeking or compact Shift-scroll seeking is missing.'
    Assert-True ($text -match 'g_progressHost\.PointerPressed' -and
        $text -match 'g_progressHost\.PointerMoved' -and
        $text -match 'g_progressHost\.PointerReleased' -and $text -match 'CapturePointer') `
        'The progress rail no longer supports direct pointer scrubbing.'
    Assert-True ($text -match 'bool transport = width >= 176' -and
        $text -match 'bool artwork = g_settings\.showArtwork && width >= kMinimumSafeWidth' -and
        $text -match 'g_artworkHost\.Width\(46\)' -and
        $text -match 'g_title\.FontSize\(14\.0 \* g_widgetTextScale\)' -and
        $text -match 'g_identityPanel\.Visibility\(showIdentity' -and
        $text -match 'g_seekBackward\.Visibility\(Visibility::Collapsed\)' -and
        $text -match 'g_seekForward\.Visibility\(Visibility::Collapsed\)' -and
        $text -match 'g_previous\.Visibility\(transport' -and
        $text -match 'g_next\.Visibility\(transport') `
        'Compact density no longer exposes artwork plus consistent previous/play/next controls.'
    Assert-True ($text -match 'g_progressHost\.Margin\(Thickness\{artwork \? 58\.0 : 10\.0, 0,\s*transport \? 100\.0 : 40\.0, 1\}\)') `
        'The scrub target can overlap artwork or transport instead of staying in the center metadata lane.'
    Assert-True ($text -match 'kMinimumSafeWidth\s*=\s*120' -and
        $text -match 'available < kMinimumSafeWidth' -and
        $text -match 'desired = std::min\(desired, available\)') `
        'The hard Start-button no-overlap guard is missing.'
    Assert-True ($text -match 'kSystemInfoWidgetName\[\]\s*=\s*L"OpalSystemInfo"' -and
        $text -match 'FindSystemInfoLane' -and
        $text -match 'kLegacySystemInfoWidgetName' -and
        $text -match 'contentInset \+ width \+ 8\.0' -and
        $text -match 'point\.X\) - zoneLeft - 8\.0') `
        'Media no longer reserves a live non-overlapping lane after System Info.'
    Assert-True ($text -match 'OpalControl::FullViewWindow\(g_monitorTarget,\s*!g_fullViewOnPrimary\)' -and
        $text -match 'OpalControl::OtherTaskbarWindows\(g_monitorTarget, fullWindow\)') `
        'Media no longer honors the configured full-view taskbar and other-display mirrors.'
    Assert-True ($text -match 'g_sessionCycleRequest' -and $text -match 'RequestSessionCycle') `
        'Artwork scroll session switching is missing.'
    Assert-True ($text -match 'g_shell\.Tapped' -and
        $text -match 'IsMediaControlSurface' -and
        $text -match 'LaunchSourceApp\(\)' -and
        $text -match 'shell:AppsFolder') `
        'One-click non-control owning-app launch is missing.'
    Assert-True ($text -match 'AutomationProperties::SetName') `
        'Glyph-only media controls do not expose accessible names.'
    Assert-True ($text -match 'canToggle' -and $text -match 'IsPlayPauseToggleEnabled') `
        'Play/pause does not honor the active session capability.'
    Assert-True ($text -notmatch '(?i)IAudioClient|IMMDevice|ksmedia|fftw|g_visualizer') `
        'A retired audio-capture or visualizer subsystem returned.'
    $rawGlyphs = [regex]::Matches($text, '[\uE000-\uF8FF]')
    Assert-True ($rawGlyphs.Count -eq 0) `
        "Master contains $($rawGlyphs.Count) raw private-use glyphs; use \uXXXX escapes."
}

if (-not $StaticOnly) {
    $key = Join-Path $modsRoot $modId
    Assert-True (Test-Path -LiteralPath $key) "$modId is not registered."
    if (Test-Path -LiteralPath $key) {
        $registration = Get-ItemProperty -LiteralPath $key
        Assert-True ([int] $registration.Disabled -eq 0) "$modId is disabled."
        Assert-True ([string] $registration.Version -eq $expectedUnifiedVersion) `
            "$modId version is $($registration.Version), expected unified release $expectedUnifiedVersion."

        $explorer = Get-Process explorer -ErrorAction SilentlyContinue | Select-Object -First 1
        Assert-True ($null -ne $explorer) 'explorer.exe is not running.'
        if ($explorer) {
            $loaded = @($explorer.Modules | Where-Object ModuleName -eq $registration.LibraryFileName)
            Assert-True ($loaded.Count -gt 0) `
                "$modId is enabled but $($registration.LibraryFileName) is not loaded in Explorer PID $($explorer.Id)."
        }
    }

    Assert-True (Test-Path -LiteralPath $deployed) "Deployed source is missing: $deployed"
    if ((Test-Path -LiteralPath $master) -and (Test-Path -LiteralPath $deployed)) {
        $buildReceipt = Join-Path $root 'build\opal-suite\build-receipt.json'
        Assert-True (Test-Path -LiteralPath $buildReceipt) "Build receipt is missing: $buildReceipt"
        $unifiedBuild = if (Test-Path -LiteralPath $buildReceipt) {
            Get-Content -LiteralPath $buildReceipt -Raw | ConvertFrom-Json
        } else { $null }
        Assert-True ($unifiedBuild -and [string]$unifiedBuild.version -eq $expectedUnifiedVersion) `
            "Unified build version is not $expectedUnifiedVersion."
        $mediaBuild = @($unifiedBuild.components | Where-Object name -eq 'media') |
            Select-Object -First 1
        $masterHash = (Get-FileHash -LiteralPath $master -Algorithm SHA256).Hash
        $deployedHash = (Get-FileHash -LiteralPath $deployed -Algorithm SHA256).Hash
        Assert-True ($mediaBuild -and $masterHash -eq [string]$mediaBuild.sha256) `
            'Master Media source does not match its build receipt.'
        Assert-True ($unifiedBuild -and $deployedHash -eq [string]$unifiedBuild.packageSourceSha256) `
            'Deployed unified Opal source does not match its package receipt.'
    }
}

$result = [pscustomobject]@{
    passed = $failures.Count -eq 0
    staticOnly = [bool] $StaticOnly
    checkedUtc = (Get-Date).ToUniversalTime().ToString('o')
    failures = @($failures)
}
$result
if ($failures.Count) {
    throw "Opal Media validation failed: $($failures -join ' | ')"
}
