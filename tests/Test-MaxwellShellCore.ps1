[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$failures = [Collections.Generic.List[string]]::new()
$assertions = 0

function Assert-True([bool] $Condition, [string] $Message) {
    $script:assertions++
    if (-not $Condition) { $script:failures.Add($Message) }
}

$source = Join-Path $root 'native\Maxwell.Shell.Core\MaxwellShellCore.cpp'
$buildScript = Join-Path $root 'Build-MaxwellShellCore.ps1'
$builtExe = Join-Path $root 'build\maxwell-shell-core\Maxwell.Shell.Core.exe'
$installedExe = Join-Path $env:LOCALAPPDATA 'Maxwell\Shell\Core\Maxwell.Shell.Core.exe'
$runKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run'
$probePath = Join-Path $env:LOCALAPPDATA 'Maxwell\Shell\State\shell-core-test-probe.json'

$sourceText = [IO.File]::ReadAllText($source)
$buildText = [IO.File]::ReadAllText($buildScript)
$installText = [IO.File]::ReadAllText((Join-Path $root 'Install-MaxwellShellCore.ps1'))
Assert-True ($sourceText -notmatch 'RunAdaptiveDock|TryHandleAdaptiveDockCommand') 'Telemetry core still invokes the retired Adaptive Dock.'
Assert-True ($buildText -notmatch 'AdaptiveDock(?:Services)?\.cpp') 'Telemetry build still compiles the retired Adaptive Dock.'
Assert-True ($buildText -notmatch '-l(?:dwmapi|d2d1|dwrite|gdiplus|oleacc|winmm|windowscodecs|winhttp)') 'Telemetry build still links a retired UI/media dependency.'
Assert-True ($sourceText -match 'wttr\.in' -and $sourceText -match 'Maxwell.Shell.Weather') 'Telemetry core does not fetch wttr.in weather for the clock.'
$performance = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\maxwell-taskbar-system-info.wh.cpp'))
Assert-True ($performance -match 'EnsureShellCoreProcess' -and $performance -match 'kInstanceMutexName') 'Explorer no longer relaunches Maxwell.Shell.Core when the mapping is missing.'
Assert-True ($installText -match 'Stop-LeftoverMaxwellShell' -and $installText -match 'MaxwellShell\.exe') 'Core install still leaves leftover MaxwellShell.exe running.'
Assert-True ($installText -notmatch 'Start-Process[^\r\n]*MaxwellShell\.exe') 'Core install must not start leftover MaxwellShell.exe.'
Assert-True ($buildText -match '-lwininet') 'Weather fetch is not linked with WinINet in Maxwell.Shell.Core.'

foreach ($retired in @(
    'native\Maxwell.Shell.Core\AdaptiveDock.cpp',
    'native\Maxwell.Shell.Core\AdaptiveDock.h',
    'native\Maxwell.Shell.Core\AdaptiveDockServices.cpp',
    'native\Maxwell.Shell.Core\AdaptiveDockServices.h',
    'config\adaptive-dock.ini',
    'mod\visual-clones\maxwell-taskbar-media-primary.wh.cpp',
    'mod\visual-clones\opal-addon-media.h'
)) {
    Assert-True (-not (Test-Path -LiteralPath (Join-Path $root $retired))) "Retired active source remains: $retired"
}

Assert-True (Test-Path -LiteralPath $builtExe -PathType Leaf) 'Telemetry-only build output is missing.'
if (Test-Path -LiteralPath $builtExe -PathType Leaf) {
    Assert-True ((Get-Item -LiteralPath $builtExe).Length -lt 300000) 'Telemetry-only core exceeds its 300 KB footprint ceiling.'
}
Assert-True (Test-Path -LiteralPath $installedExe -PathType Leaf) 'Installed telemetry core is missing.'
if ((Test-Path -LiteralPath $builtExe) -and (Test-Path -LiteralPath $installedExe)) {
    Assert-True ((Get-FileHash $builtExe -Algorithm SHA256).Hash -eq
                 (Get-FileHash $installedExe -Algorithm SHA256).Hash) 'Installed telemetry core does not match the verified build.'
}

$runValue = Get-ItemPropertyValue -LiteralPath $runKey -Name 'MaxwellShellCore' -ErrorAction SilentlyContinue
Assert-True ([string]$runValue -eq ('"' + $installedExe + '"')) 'Telemetry core autostart is missing or points elsewhere.'
$ownedProcess = @(Get-CimInstance Win32_Process -Filter "Name='Maxwell.Shell.Core.exe'" -ErrorAction SilentlyContinue |
    Where-Object { $_.ExecutablePath -and [IO.Path]::GetFullPath($_.ExecutablePath) -eq [IO.Path]::GetFullPath($installedExe) })
Assert-True ($ownedProcess.Count -eq 1) 'Exactly one telemetry core process is not running.'
Assert-True (-not (Get-Process -Name MaxwellShell -ErrorAction SilentlyContinue)) 'Leftover MaxwellShell.exe is still running beside Maxwell.Shell.Core.'

if (-not ('MaxwellTelemetryOnly.NativeWindow' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
namespace MaxwellTelemetryOnly {
    public static class NativeWindow {
        [DllImport("user32.dll", CharSet=CharSet.Unicode)]
        public static extern IntPtr FindWindow(string className, string windowName);
    }
}
'@
}
$retiredWindow = [MaxwellTelemetryOnly.NativeWindow]::FindWindow('MaxwellAdaptiveDockWindow', 'Maxwell Adaptive Dock')
Assert-True ($retiredWindow -eq [IntPtr]::Zero) 'The retired Maxwell Command/Media/Focus window still exists.'
Assert-True (-not (Test-Path -LiteralPath (Join-Path $env:LOCALAPPDATA 'Maxwell\Shell\Config\adaptive-dock.ini'))) 'The active Adaptive Dock configuration still exists.'

$probe = Start-Process -FilePath $installedExe -ArgumentList @('--probe',('"' + $probePath + '"')) -WindowStyle Hidden -Wait -PassThru
$sample = if ($probe.ExitCode -eq 0 -and (Test-Path -LiteralPath $probePath)) {
    Get-Content -LiteralPath $probePath -Raw | ConvertFrom-Json
} else { $null }
Assert-True ($probe.ExitCode -eq 0 -and $sample) 'Telemetry probe failed.'
Assert-True ($sample -and [int]$sample.protocolVersion -eq 1) 'Telemetry protocol version is not 1.'
$maximumSampleAgeMs = if ($sample) { [math]::Max(15000, [int64]$sample.sampleIntervalMs + 5000) } else { 15000 }
Assert-True ($sample -and [int64]$sample.sampleAgeMs -le $maximumSampleAgeMs) 'Telemetry sample is stale.'
Assert-True ($sample -and (([int]$sample.flags -band 3) -eq 3)) 'Telemetry sample lacks CPU or RAM data.'

$result = [ordered]@{
    passed = $failures.Count -eq 0
    assertions = $assertions
    failures = @($failures)
    surface = 'telemetry-and-weather'
    processId = if ($ownedProcess.Count) { $ownedProcess[0].ProcessId } else { $null }
    checkedAt = [DateTimeOffset]::Now.ToString('o')
}
$result | ConvertTo-Json -Depth 5
if ($failures.Count) { exit 1 }
