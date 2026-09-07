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
$rules = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\maxwell-shell-rules.h'))
$ownedRules = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\maxwell-shell-owned-overrides.h'))
$media = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\maxwell-opal-media.wh.cpp'))
$performance = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\maxwell-taskbar-system-info.wh.cpp'))
$apply = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\maxwell-shell-apply.h'))
$control = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\opal-control.h'))
$icons = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\opal-addon-icons.h'))
$installer = [IO.File]::ReadAllText((Join-Path $root 'Install-OpalSuite.ps1'))

Check ($control -match 'VisibleFullViewWindow' -and $control -match 'TaskbarOccluded') 'Visible full-view selection is missing.'
Check ($media -match 'VisibleFullViewWindow' -and $performance -match 'VisibleFullViewWindow') 'Media or Performance still attach only to the occluded primary bar.'
Check ($performance -match 'return ApplyOnTaskbarThread\(true\)' -and $performance -match 'attached = attached && mirror.attached') 'Performance must verify the full view and every expected mirror.'
Check ($performance -notmatch 'return ContentPriority::Hidden') 'Tight taskbars still hide Computer stats.'
Check ($icons -match 'IconSizeSmall: 28' -or $shell -match 'IconSizeSmall: 28') 'Tray icons are not one-scale with app icons.'
Check ($icons -match 'iconSizeSmall = 28' -and $icons -match 'iconSizeSmall < 24') 'Reset layout does not restore tray icon scale.'
Check ($shell -match 'Reset layout' -and $media -match 'ForceCanonicalLayout' -and $performance -match 'ForceCanonicalLayout') 'One-click reset does not restore canonical geometry.'
Check ($control -match 'neverLived' -and $control -match 'planned-explorer-restart') 'Crash protection still counts never-lived or planned Explorer restarts.'
Check ($performance -match 'EnsureShellCoreProcess') 'Missing Core relaunch.'
Check ([regex]::Matches($performance, 'CloseExternalTelemetry\(\);\s*EnsureShellCoreProcess\(\);').Count -ge 2) 'A dead Shell Core publisher still leaves Opal attached to a stale telemetry mapping.'
Check ($media -match 'wideInsideCapsule' -and $media -match 'Nothing playing') 'Wide media still shoves the bar or idle media still vanishes.'
Check ([regex]::Matches($media, 'playback\s*&&\s*playback\.PlaybackStatus\(\)').Count -ge 2) 'Session selection can still dereference a missing playback-info interface.'
Check ($media -match 'if \(!playback\)\s*\{\s*PublishEmptySnapshot\(\);\s*return false;' -and $media -match 'if \(controls\)') 'Media refresh must clear stale state before returning on missing playback info and guard missing controls.'
Check ($shell -match 'hideWithoutSession: false' -and $installer -match "'media.hideWithoutSession' = 0") 'Installer still hides the media lane when idle.'
Check ($installer -match "'screens.mediaMonitor' = 'primary'" -and $installer -match "'screens.performanceMonitor' = 'primary'") 'Installer should not create unwanted duplicate widgets.'
Check ($installer -match 'planned-explorer-restart') 'Installer still restarts Explorer without the planned-restart marker.'
Check ($installer -match 'Stop-LeftoverMaxwellShell' -and $installer -match 'MaxwellShell\.exe') 'Installer still leaves Adaptive Dock MaxwellShell.exe running.'
Check ($installer -match "Install-MaxwellShellCore\.ps1") 'Installer does not install Maxwell.Shell.Core autostart.'
Check ($installer -notmatch 'Start-Process[^\r\n]*MaxwellShell\.exe') 'Installer must not start leftover MaxwellShell.exe.'
Check ($shell -match 'healthy \? 5000' -and $shell -match 'attachment health:') 'Attachment recovery stops after startup instead of checking replaced taskbars.'
Check ($shell -match 'barsChanged' -and $shell -match 'CurrentProcessTaskbars' -and $shell -match 'healthy && !barsChanged' -and $control -match 'TouchAttachmentProof' -and $shell -match 'TouchAttachmentProof') 'Healthy recovery still walks XAML every five seconds on unchanged taskbar windows.'
Check ($shell.IndexOf('g_unloading.store(true', $shell.IndexOf('void Wh_ModBeforeUninit')) -lt $shell.IndexOf('OpalMedia_ModBeforeUninit();', $shell.IndexOf('void Wh_ModBeforeUninit() {'))) 'Component teardown can race recovery.'
Check ($media -notmatch 'g_fullViewOnPrimary = !performanceFullOnPrimary') 'Automatic placement silently splits full views across monitors.'
foreach ($component in @($media,$performance)) {
    Check ($component -match 'slot.repeater.Margin\(margin\)' -and $component -match 'margin.Left -= slot.reservedMargin') 'Mirrors must own and release only their own reserved space.'
    Check ($component -match 'PublishAttachmentProof' -and $component -match 'context->repairOnly') 'Screen attachment lacks current, idempotent proof.'
}
Check ($installer -match 'attachmentDeadline' -and $installer -match 'Source changed since the build') 'Installer accepts stale source or a loaded DLL without attached widgets.'
Check ($shell -notmatch '(?m)^\s*- enableTaskbar:' -and $shell -match 'The Explorer taskbar is Opal') 'Opal and the taskbar are still separate products.'
foreach ($propsName in @('kProps6', 'kProps7')) {
    $propsBlock = [regex]::Match(
        $rules,
        "(?s)inline constexpr Prop $propsName\[\] = \{(?<body>.*?)\r?\n\};"
    ).Groups['body'].Value
    Check ($propsBlock -match 'L"CornerRadius", nullptr, L"25"') "$propsName still leaves boxy corners on the system tray capsule."
    Check ($propsBlock -match 'L"Padding", nullptr, L"8,0,8,0"') "$propsName still has asymmetric system tray padding."
    Check ($propsBlock -match 'L"Background", nullptr, L"\$OpalTaskbarSurface"') "$propsName does not use the canonical taskbar glass."
}
Check ($rules -match 'inline constexpr Prop kProps1\[\][\s\S]*?L"Margin", nullptr, L"10,9,10,9"[\s\S]*?L"CornerRadius", nullptr, L"25"[\s\S]*?L"\$OpalTaskbarSurface"') 'The application lane is not a full-radius glass capsule.'
Check ($rules -match 'inline constexpr Prop kProps194\[\][\s\S]*?L"Padding", nullptr, L"8,0,8,0"[\s\S]*?L"Margin", nullptr, L"8,9,8,9"[\s\S]*?L"CornerRadius", nullptr, L"25"') 'The late tray rule can still restore square or asymmetric geometry.'
Check ($ownedRules -match 'inline constexpr Prop kFloatingTaskbarRoot\[\][\s\S]*?L"\$OpalTaskbarSurface"[\s\S]*?L"Margin", nullptr, L"10,9,10,9"[\s\S]*?L"CornerRadius", nullptr, L"25"') 'The final app-lane invariant is not a 50-DIP pill.'
Check ($ownedRules -match 'inline constexpr Prop kFrostedTraySurface\[\][\s\S]*?L"Padding", nullptr, L"8,0,8,0"[\s\S]*?L"CornerRadius", nullptr, L"25"') 'The final tray invariant is not a symmetric 50-DIP pill.'
Check ($apply -notmatch 'ApplyCompositionCornerRadius|ElementCompositionPreview|CreateRoundedRectangleGeometry') 'An unsupported compositor clip can still crash Explorer while styling the tray StackPanel.'

$result = [pscustomobject]@{ passed = $failures.Count -eq 0; failures = @($failures) }
$result
if ($failures.Count) { throw "Taskbar resilience validation failed: $($failures -join ' | ')" }
