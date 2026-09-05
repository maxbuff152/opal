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

function Stop-LeftoverMaxwellShell {
    foreach ($process in @(Get-CimInstance Win32_Process -Filter "Name='MaxwellShell.exe'" -ErrorAction SilentlyContinue)) {
        $path = [string]$process.ExecutablePath
        if ($path -and ([IO.Path]::GetFileName($path) -ieq 'Maxwell.Shell.Core.exe')) { continue }
        Stop-Process -Id $process.ProcessId -Force -ErrorAction SilentlyContinue
        Wait-Process -Id $process.ProcessId -Timeout 8 -ErrorAction SilentlyContinue
    }
    Get-Process -Name MaxwellShell -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue

    if (Test-Path -LiteralPath $runKey) {
        foreach ($name in @((Get-Item -LiteralPath $runKey).Property)) {
            if ($name -eq $runName) { continue }
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
Stop-LeftoverMaxwellShell
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
if (Get-Process -Name MaxwellShell -ErrorAction SilentlyContinue) {
    throw 'Leftover MaxwellShell.exe is still running; Opal telemetry is Maxwell.Shell.Core only.'
}
$probe = Join-Path $stateRoot 'shell-core-probe.json'
# PDH initialization can exceed the old 900-ms sleep. A mapped buffer is not
# necessarily published yet (probe exit 3). Wait for an actual sample from the
# current publisher, and never accept a leftover probe file from the old core.
$readyDeadline = (Get-Date).AddSeconds(30)
$ready = $false
$sample = $null
do {
    $probeProcess = Start-Process -FilePath $installedExe -ArgumentList @('--probe',('"' + $probe + '"')) -WindowStyle Hidden -PassThru
    if (-not $probeProcess.WaitForExit(5000)) {
        Stop-Process -Id $probeProcess.Id -Force -ErrorAction SilentlyContinue
        throw 'Maxwell.Shell.Core readiness probe timed out.'
    }
    if ($probeProcess.ExitCode -eq 0 -and (Test-Path -LiteralPath $probe -PathType Leaf)) {
        $sample = Get-Content -LiteralPath $probe -Raw | ConvertFrom-Json
        $process = @(Get-OwnedCoreProcess | Select-Object -First 1)
        $maximumAgeMs = [math]::Max(15000, [int64]$sample.sampleIntervalMs + 5000)
        $ready = $process.Count -eq 1 -and
            [int]$sample.publisherPid -eq [int]$process[0].ProcessId -and
            [int]$sample.protocolVersion -eq 1 -and
            [int64]$sample.sampleAgeMs -le $maximumAgeMs -and
            (([int]$sample.flags -band 3) -eq 3)
    }
    if (-not $ready) { Start-Sleep -Milliseconds 500 }
} until ($ready -or (Get-Date) -ge $readyDeadline)
if (-not $ready) { throw 'Maxwell.Shell.Core did not publish valid current CPU/RAM data within 30 seconds.' }
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
    leftoverMaxwellShellRetired = -not [bool](Get-Process -Name MaxwellShell -ErrorAction SilentlyContinue)
    retiredDockBackups = @($retiredDockBackups)
}
$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $receipt -Encoding UTF8
[pscustomobject]$result
