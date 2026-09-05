[CmdletBinding()]
param(
    [ValidateRange(1, 5)]
    [int]$Samples = 2,
    [ValidateRange(3, 30)]
    [int]$SampleSeconds = 8,
    [ValidateRange(3, 120)]
    [int]$SettleSeconds = 6,
    [ValidateSet('Stock', 'ShellOnly', 'ShellMedia', 'ShellSystemInfo', 'FullSuite')]
    [string[]]$Scenario = @('Stock', 'ShellOnly', 'ShellMedia', 'ShellSystemInfo', 'FullSuite'),
    [ValidateRange(0, 10)]
    [int]$AbbaRounds = 0,
    [int]$RandomSeed = 0,
    [Parameter(Mandatory)]
    [string]$Label
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Get-OpalMeasuredProcessCost.ps1')
$sessionId = (Get-Process -Id $PID).SessionId
$build = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'build\opal-suite\build-receipt.json') -Raw | ConvertFrom-Json
$corePath = Join-Path $env:LOCALAPPDATA 'Maxwell\Shell\Core\Maxwell.Shell.Core.exe'
$builtCorePath = Join-Path $PSScriptRoot 'build\maxwell-shell-core\Maxwell.Shell.Core.exe'
if ((Get-FileHash -LiteralPath $corePath).Hash -ne (Get-FileHash -LiteralPath $builtCorePath).Hash) { throw 'Installed companion does not match build.' }
$buildIdentity = [pscustomobject]@{Version=$build.version;DllSha256=$build.sha256;CoreSha256=(Get-FileHash -LiteralPath $corePath).Hash}
foreach ($source in $build.sourceInputs) {
    if ((Get-FileHash -LiteralPath $source.path).Hash -ne $source.sha256) { throw 'Source changed since build.' }
}

$modRegistryRoot = 'HKLM:\SOFTWARE\Windhawk\Engine\Mods'
$modId = 'local@opal'
$modPath = Join-Path $modRegistryRoot $modId
$settingsPath = Join-Path $modPath 'Settings'
$scenarios = [ordered]@{
    Stock = [pscustomobject]@{ Enabled = $false; Media = 0; Performance = 0 }
    ShellOnly = [pscustomobject]@{ Enabled = $true; Media = 0; Performance = 0 }
    ShellMedia = [pscustomobject]@{ Enabled = $true; Media = 1; Performance = 0 }
    ShellSystemInfo = [pscustomobject]@{ Enabled = $true; Media = 0; Performance = 1 }
    FullSuite = [pscustomobject]@{ Enabled = $true; Media = 1; Performance = 1 }
}

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class OpalPackageCostGuiResources {
    [DllImport("user32.dll")]
    public static extern int GetGuiResources(IntPtr process, int flag);
}
'@ -ErrorAction SilentlyContinue

function Get-GuiResourceCount {
    param(
        [Parameter(Mandatory)][System.Diagnostics.Process]$Process,
        [Parameter(Mandatory)][ValidateSet(0, 1)][int]$Kind
    )

    try {
        if ($Process.HasExited -or $Process.Handle -eq [IntPtr]::Zero) { return $null }
        [OpalPackageCostGuiResources]::GetGuiResources($Process.Handle, $Kind)
    } catch {
        $null
    }
}

function Get-Median {
    param([double[]]$Values)
    $sorted = @($Values | Sort-Object)
    if ($sorted.Count -eq 0) { return 0.0 }
    $middle = [int][math]::Floor($sorted.Count / 2)
    if ($sorted.Count % 2) { return $sorted[$middle] }
    ($sorted[$middle - 1] + $sorted[$middle]) / 2.0
}

function Clear-IntentionalRestartMarkers {
    $healthRoot = Join-Path $env:LOCALAPPDATA 'Maxwell\Opal'
    foreach ($package in @('media', 'performance')) {
        $path = Join-Path $healthRoot "health-$package.ini"
        # Mark only this intentional exit as clean. Preserve crash counts and
        # quarantine decisions; benchmarking must not reset crash protection.
        if (Test-Path -LiteralPath $path) {
            $text = [IO.File]::ReadAllText($path)
            [IO.File]::WriteAllText($path, [regex]::Replace($text, '(?m)^Dirty=\d+\r?$', 'Dirty=0'))
        }
    }
}

function Restart-PackageExplorer {
    $script:verifiedExplorer = $null
    Clear-IntentionalRestartMarkers
    $previous = @(Get-Process -Name explorer -ErrorAction SilentlyContinue |
        Where-Object SessionId -eq $sessionId)
    $previousIds = @($previous | ForEach-Object Id)
    $previous | Stop-Process -Force -ErrorAction Stop
    $started = Get-Date
    $deadline = $started.AddSeconds(20)
    $explicitLaunchRequested = $false
    do {
        Start-Sleep -Milliseconds 500
        $found = @(Get-Process -Name explorer -ErrorAction SilentlyContinue |
            Where-Object SessionId -eq $sessionId)
        if ($found.Count -eq 1 -and $found[0].Id -notin $previousIds) {
            # Process.StartTime is lazy: freeze scalar identity now, before
            # settling, so later PID reuse cannot change the baseline birth.
            return [pscustomobject]@{
                Id = $found[0].Id
                StartTime = $found[0].StartTime
            }
        }
        # Give Windows shell recovery time to start its replacement. Never
        # explicitly launch while an old or competing Explorer is present.
        $now = Get-Date
        if ($found.Count -eq 0 -and ($now - $started).TotalSeconds -ge 5 -and
            -not $explicitLaunchRequested -and $now -lt $deadline) {
            $explicitLaunchRequested = $true
            Start-Process -FilePath "$env:WINDIR\explorer.exe" -WindowStyle Hidden
        }
    } until ($now -ge $deadline)
    throw 'Exactly one replacement Explorer did not become ready within 20 seconds.'
}

function Get-VerifiedPackageExplorer {
    $found = @(Get-Process -Name explorer -ErrorAction Stop |
        Where-Object SessionId -eq $sessionId)
    if (-not $script:verifiedExplorer -or $found.Count -ne 1 -or
        $found[0].Id -ne $script:verifiedExplorer.Id -or
        $found[0].StartTime -ne $script:verifiedExplorer.StartTime) {
        throw 'Verified Explorer changed or became ambiguous before measurement completed.'
    }
    return $found[0]
}

function Set-Scenario {
    param([Parameter(Mandatory)]$Configuration)
    Set-ItemProperty -LiteralPath $modPath -Name Disabled -Type DWord `
        -Value $(if ($Configuration.Enabled) { 0 } else { 1 })
    Set-ItemProperty -LiteralPath $settingsPath -Name 'media.mediaEnabled' -Type DWord `
        -Value ([int]$Configuration.Media)
    Set-ItemProperty -LiteralPath $settingsPath -Name 'performance.performanceEnabled' -Type DWord `
        -Value ([int]$Configuration.Performance)

    $script:verifiedExplorer = Restart-PackageExplorer
    Start-Sleep -Seconds $SettleSeconds
    $explorer = Get-VerifiedPackageExplorer
    $explorer.Refresh()
    $mapped = @($explorer.Modules | Where-Object ModuleName -eq $build.dllName)
    if ($Configuration.Enabled) {
        if ($mapped.Count -ne 1 -or (Get-FileHash -LiteralPath $mapped[0].FileName).Hash -ne $build.sha256) { throw 'Expected Opal DLL not loaded.' }
        & (Join-Path $PSScriptRoot 'tests\Test-OpalAttachment.ps1') | Out-Null
    } elseif ($mapped.Count) { throw 'Stock scenario still has Opal loaded.' }
}

function Measure-ExplorerSample {
    $process = Get-VerifiedPackageExplorer
    $process.Refresh()
    $startHandles = $process.HandleCount
    $startGdi = Get-GuiResourceCount -Process $process -Kind 0
    $startUser = Get-GuiResourceCount -Process $process -Kind 1
    $measuredBefore = @(Get-OpalMeasuredProcessSnapshot -SessionId $sessionId)
    $timer = [Diagnostics.Stopwatch]::StartNew()
    Start-Sleep -Seconds $SampleSeconds
    $measuredAfter = @(Get-OpalMeasuredProcessSnapshot -SessionId $sessionId)
    $elapsed = $timer.Elapsed.TotalSeconds
    $measuredCost = Compare-OpalMeasuredProcessSnapshot -Before $measuredBefore -After $measuredAfter -ElapsedSeconds $elapsed
    $explorerCost = @($measuredCost.Processes | Where-Object Pid -eq $process.Id)
    if ($explorerCost.Count -ne 1) { throw 'Sampled Explorer missing from measured process set.' }
    $process = Get-VerifiedPackageExplorer
    $process.Refresh()
    $cpuSeconds = $explorerCost[0].CpuSeconds
    $gdi = Get-GuiResourceCount -Process $process -Kind 0
    $user = Get-GuiResourceCount -Process $process -Kind 1
    [pscustomobject]@{
        Pid = $process.Id
        ElapsedSeconds = $elapsed
        RuntimeVerified = $true
        CpuSeconds = [math]::Round($cpuSeconds, 4)
        CpuPercentOneCore = [math]::Round($cpuSeconds / $elapsed * 100.0, 3)
        CpuPercentMachine = [math]::Round(
            $cpuSeconds / $elapsed * 100.0 / [Environment]::ProcessorCount, 4)
        PrivateMB = [math]::Round($explorerCost[0].PrivateMB, 2)
        WorkingSetMB = [math]::Round($explorerCost[0].WorkingSetMB, 2)
        Handles = $process.HandleCount
        HandleDelta = $process.HandleCount - $startHandles
        Gdi = $gdi
        GdiDelta = if ($null -ne $gdi -and $null -ne $startGdi) {
            $gdi - $startGdi
        } else { $null }
        User = $user
        UserDelta = if ($null -ne $user -and $null -ne $startUser) {
            $user - $startUser
        } else { $null }
        Threads = $process.Threads.Count
        MeasuredProcessCost = $measuredCost
    }
}

$originalState = [pscustomobject]@{
    Disabled = [int](Get-ItemPropertyValue -LiteralPath $modPath -Name Disabled)
    Media = [int](Get-ItemPropertyValue -LiteralPath $settingsPath -Name 'media.mediaEnabled')
    Performance = [int](Get-ItemPropertyValue -LiteralPath $settingsPath -Name 'performance.performanceEnabled')
}

$results = @()
$pairedSamples = @()
$seedUsed = if ($RandomSeed) { $RandomSeed } else { [Environment]::TickCount -band 0x7fffffff }
try {
    if ($AbbaRounds -gt 0) {
        $random = [Random]::new($seedUsed)
        for ($round = 1; $round -le $AbbaRounds; $round++) {
            # Randomizing between ABBA and BAAB prevents warm-up and time drift
            # from consistently favoring Stock or FullSuite.
            $orderName = if ($random.Next(2) -eq 0) { 'ABBA' } else { 'BAAB' }
            $order = if ($orderName -eq 'ABBA') {
                @('Stock', 'FullSuite', 'FullSuite', 'Stock')
            } else {
                @('FullSuite', 'Stock', 'Stock', 'FullSuite')
            }
            Write-Host "Paired round $round/$AbbaRounds · $orderName" -ForegroundColor Cyan
            for ($position = 0; $position -lt $order.Count; $position++) {
                $scenarioName = $order[$position]
                Write-Host "  $($position + 1)/4 $scenarioName"
                Set-Scenario -Configuration $scenarios[$scenarioName]
                $sample = Measure-ExplorerSample
                $pairedSamples += [pscustomobject]@{
                    Round = $round
                    Order = $orderName
                    Position = $position + 1
                    Scenario = $scenarioName
                    Pid = $sample.Pid
                    ElapsedSeconds = $sample.ElapsedSeconds
                    RuntimeVerified = $sample.RuntimeVerified
                    CpuSeconds = $sample.CpuSeconds
                    CpuPercentOneCore = $sample.CpuPercentOneCore
                    CpuPercentMachine = $sample.CpuPercentMachine
                    PrivateMB = $sample.PrivateMB
                    WorkingSetMB = $sample.WorkingSetMB
                    Handles = $sample.Handles
                    HandleDelta = $sample.HandleDelta
                    Gdi = $sample.Gdi
                    GdiDelta = $sample.GdiDelta
                    User = $sample.User
                    UserDelta = $sample.UserDelta
                    Threads = $sample.Threads
                    MeasuredProcessCost = $sample.MeasuredProcessCost
                }
            }
        }
        foreach ($scenarioName in @('Stock', 'FullSuite')) {
            $samplesForScenario = @($pairedSamples | Where-Object Scenario -eq $scenarioName)
            $results += [pscustomobject]@{
                Scenario = $scenarioName
                Configuration = $scenarios[$scenarioName]
                CpuPercentOneCore = [math]::Round((Get-Median ([double[]]$samplesForScenario.CpuPercentOneCore)), 3)
                CpuPercentMachine = [math]::Round((Get-Median ([double[]]$samplesForScenario.CpuPercentMachine)), 4)
                PrivateMB = [math]::Round((Get-Median ([double[]]$samplesForScenario.PrivateMB)), 2)
                WorkingSetMB = [math]::Round((Get-Median ([double[]]$samplesForScenario.WorkingSetMB)), 2)
                Handles = [math]::Round((Get-Median ([double[]]$samplesForScenario.Handles)), 1)
                Gdi = [math]::Round((Get-Median ([double[]]$samplesForScenario.Gdi)), 1)
                User = [math]::Round((Get-Median ([double[]]$samplesForScenario.User)), 1)
                Threads = [math]::Round((Get-Median ([double[]]$samplesForScenario.Threads)), 1)
                Samples = $samplesForScenario
            }
        }
    } else {
        foreach ($scenarioItem in $scenarios.GetEnumerator()) {
            if ($scenarioItem.Key -notin $Scenario) { continue }
            Write-Host "Scenario: $($scenarioItem.Key)" -ForegroundColor Cyan
            Set-Scenario -Configuration $scenarioItem.Value
            $samplesForScenario = @()
            for ($index = 1; $index -le $Samples; $index++) {
                Write-Host "  sample $index/$Samples"
                $samplesForScenario += Measure-ExplorerSample
            }
            $results += [pscustomobject]@{
                Scenario = $scenarioItem.Key
                Configuration = $scenarioItem.Value
                CpuPercentOneCore = [math]::Round(
                    (Get-Median ([double[]]$samplesForScenario.CpuPercentOneCore)), 3)
                CpuPercentMachine = [math]::Round(
                    (Get-Median ([double[]]$samplesForScenario.CpuPercentMachine)), 4)
                PrivateMB = [math]::Round(
                    (Get-Median ([double[]]$samplesForScenario.PrivateMB)), 2)
                WorkingSetMB = [math]::Round(
                    (Get-Median ([double[]]$samplesForScenario.WorkingSetMB)), 2)
                Handles = [math]::Round(
                    (Get-Median ([double[]]$samplesForScenario.Handles)), 1)
                Gdi = [math]::Round(
                    (Get-Median ([double[]]$samplesForScenario.Gdi)), 1)
                User = [math]::Round(
                    (Get-Median ([double[]]$samplesForScenario.User)), 1)
                Threads = [math]::Round(
                    (Get-Median ([double[]]$samplesForScenario.Threads)), 1)
                Samples = $samplesForScenario
            }
        }
    }
} finally {
    Set-ItemProperty -LiteralPath $modPath -Name Disabled -Type DWord -Value $originalState.Disabled
    Set-ItemProperty -LiteralPath $settingsPath -Name 'media.mediaEnabled' -Type DWord -Value $originalState.Media
    Set-ItemProperty -LiteralPath $settingsPath -Name 'performance.performanceEnabled' -Type DWord -Value $originalState.Performance
    $script:verifiedExplorer = Restart-PackageExplorer
}

$receipt = [ordered]@{
    SchemaVersion = 2
    Build = $buildIdentity
    Label = $Label
    CapturedAt = [DateTimeOffset]::Now.ToString('o')
    LogicalProcessors = [Environment]::ProcessorCount
    SampleSeconds = $SampleSeconds
    SettleSeconds = $SettleSeconds
    TestDesign = if ($AbbaRounds) { 'Randomized paired ABBA/BAAB' } else { 'Scenario medians' }
    AbbaRounds = $AbbaRounds
    RandomSeed = if ($AbbaRounds) { $seedUsed } else { $null }
    Scenarios = $results
}
$outputDirectory = Join-Path $PSScriptRoot 'measurements'
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
$safeLabel = $Label -replace '[^a-zA-Z0-9._-]', '-'
$outputPath = Join-Path $outputDirectory "package-cost-$safeLabel.json"
$json = ($receipt | ConvertTo-Json -Depth 8) -replace "`r`n", "`n"
[IO.File]::WriteAllText($outputPath, ($json + "`n"), [Text.UTF8Encoding]::new($false))

$results |
    Format-Table Scenario, CpuPercentOneCore, CpuPercentMachine, PrivateMB,
        WorkingSetMB, Handles, Gdi, User, Threads -AutoSize
Write-Host "Receipt: $outputPath" -ForegroundColor Green
