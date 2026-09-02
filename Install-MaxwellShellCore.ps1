[CmdletBinding()]
param(
    [ValidateSet('Install','Status','Stop')]
    [string]$Action = 'Install'
)

$ErrorActionPreference = 'Stop'
$appRoot = Join-Path $env:LOCALAPPDATA 'Maxwell\Shell\Core'
$stateRoot = Join-Path $env:LOCALAPPDATA 'Maxwell\Shell\State'
$installedExe = Join-Path $appRoot 'Maxwell.Shell.Core.exe'
$builtExe = Join-Path $PSScriptRoot 'build\maxwell-shell-core\Maxwell.Shell.Core.exe'
$runKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run'
$runName = 'MaxwellShellCore'
$receipt = Join-Path $stateRoot 'shell-core-install-last-run.json'
$retiredDockConfig = Join-Path $env:LOCALAPPDATA 'Maxwell\Shell\Config\adaptive-dock.ini'
$retiredDockProbe = Join-Path $stateRoot 'adaptive-dock-probe.json'

function Get-OwnedCoreProcess {
    Get-CimInstance Win32_Process -Filter "Name='Maxwell.Shell.Core.exe'" -ErrorAction SilentlyContinue |
        Where-Object { $_.ExecutablePath -and [IO.Path]::GetFullPath($_.ExecutablePath) -eq [IO.Path]::GetFullPath($installedExe) }
}

function Stop-OwnedCore {
    foreach ($process in @(Get-OwnedCoreProcess)) {
        Stop-Process -Id $process.ProcessId -ErrorAction SilentlyContinue
        Wait-Process -Id $process.ProcessId -Timeout 10 -ErrorAction SilentlyContinue
    }
    # Process teardown is asynchronous: the image section can stay mapped for a
    # moment after the process object is gone, which raced the Copy-Item below
    # and failed the install with "being used by another process". Wait until
    # the binary can actually be opened exclusively.
    if (-not (Test-Path -LiteralPath $installedExe -PathType Leaf)) { return }
    for ($attempt = 0; $attempt -lt 40; $attempt++) {
        try {
            $stream = [IO.File]::Open($installedExe, 'Open', 'ReadWrite', 'None')
            $stream.Close()
            return
        } catch {
            Start-Sleep -Milliseconds 250
        }
    }
    throw "Maxwell.Shell.Core still holds $installedExe after 10 seconds."
}

if ($Action -eq 'Stop') {
    Stop-OwnedCore
    [pscustomobject]@{succeeded=$true;action='Stop';running=$false;binary=$installedExe}
    return
}

if ($Action -eq 'Status') {
    $process = @(Get-OwnedCoreProcess | Select-Object -First 1)
    [pscustomobject]@{
        installed = Test-Path -LiteralPath $installedExe -PathType Leaf
        autostart = (Get-ItemPropertyValue -LiteralPath $runKey -Name $runName -ErrorAction SilentlyContinue)
        running = $process.Count -gt 0
        processId = if ($process) { $process[0].ProcessId } else { $null }
        binary = $installedExe
    }
    return
}

if (-not (Test-Path -LiteralPath $builtExe -PathType Leaf)) {
    & (Join-Path $PSScriptRoot 'Build-MaxwellShellCore.ps1') | Out-Null
}
New-Item -ItemType Directory -Path $appRoot,$stateRoot -Force | Out-Null
Stop-OwnedCore

$retiredDockBackups = [Collections.Generic.List[string]]::new()
foreach ($retiredPath in @($retiredDockConfig, $retiredDockProbe)) {
    if (-not (Test-Path -LiteralPath $retiredPath -PathType Leaf)) { continue }
    $backupName = '{0}.retired-{1}{2}' -f
        [IO.Path]::GetFileNameWithoutExtension($retiredPath),
        (Get-Date -Format 'yyyyMMdd-HHmmss'),
        [IO.Path]::GetExtension($retiredPath)
    $retiredBackup = Join-Path $stateRoot $backupName
    Copy-Item -LiteralPath $retiredPath -Destination $retiredBackup
    Remove-Item -LiteralPath $retiredPath -Force
    $retiredDockBackups.Add($retiredBackup)
}

$backup = $null
if (Test-Path -LiteralPath $installedExe -PathType Leaf) {
    $backup = Join-Path $stateRoot ('Maxwell.Shell.Core.before-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.exe')
    Copy-Item -LiteralPath $installedExe -Destination $backup
}
Copy-Item -LiteralPath $builtExe -Destination $installedExe -Force
$runValue = '"' + $installedExe + '"'
New-ItemProperty -LiteralPath $runKey -Name $runName -PropertyType String -Value $runValue -Force | Out-Null
Start-Process -FilePath $installedExe -WindowStyle Hidden
Start-Sleep -Milliseconds 900

$process = @(Get-OwnedCoreProcess | Select-Object -First 1)
if (-not $process) { throw 'Maxwell.Shell.Core did not remain running.' }
$probe = Join-Path $stateRoot 'shell-core-probe.json'
$probeProcess = Start-Process -FilePath $installedExe -ArgumentList @('--probe',('"' + $probe + '"')) -WindowStyle Hidden -Wait -PassThru
if ($probeProcess.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $probe -PathType Leaf)) {
    throw "Maxwell.Shell.Core shared-state probe failed with exit code $($probeProcess.ExitCode)."
}
$sample = Get-Content -LiteralPath $probe -Raw | ConvertFrom-Json
# The flags test needs the parentheses: PowerShell binds -ne tighter than
# -band, so the original `flags -band 3 -ne 3` evaluated as `flags -band ($false)`
# = 0, which is falsy for every input. The CPU/RAM presence check never fired.
if ([int]$sample.protocolVersion -ne 1 -or
    [int64]$sample.sampleAgeMs -gt 15000 -or
    (([int]$sample.flags -band 3) -ne 3)) {
    throw 'Maxwell.Shell.Core returned an invalid or stale CPU/RAM sample.'
}
$result = [ordered]@{
    succeeded = $true
    completedAt = [DateTimeOffset]::Now.ToString('o')
    binary = $installedExe
    sha256 = (Get-FileHash -LiteralPath $installedExe -Algorithm SHA256).Hash
    backup = $backup
    autostart = (Get-ItemPropertyValue -LiteralPath $runKey -Name $runName)
    processId = $process[0].ProcessId
    telemetry = $sample
    surface = 'telemetry-only'
    retiredDockBackups = @($retiredDockBackups)
}
$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $receipt -Encoding UTF8
[pscustomobject]$result
