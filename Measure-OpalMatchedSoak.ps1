<#
.SYNOPSIS
Runs four controlled Stock/FullSuite observation legs and restores the original settings.
.DESCRIPTION
This restarts interactive Explorer. Run only after authorization for that disruption.
Expected installed identity is frozen at start; source edits/builds elsewhere do not
invalidate the installed baseline. A new installation during the run does invalidate it.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$OutputPath,
    [string]$BaselineSnapshotBuildReceiptPath = (Join-Path $PSScriptRoot 'build\opal-suite\build-receipt.json'),
    [ValidateRange(480,3600)][int]$LegSeconds = 480,
    [ValidateRange(5,15)][int]$IntervalSeconds = 5,
    [ValidateSet('ABBA','BAAB')][string]$Order = 'ABBA',
    [ValidateRange(16,256)][double]$StepThresholdMB = 64,
    [ValidateRange(20,120)][int]$ReadinessTimeoutSeconds = 60,
    [switch]$DescribeOnly
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Get-OpalMeasuredProcessCost.ps1')
. (Join-Path $PSScriptRoot 'Get-OpalMemoryRegions.ps1')
. (Join-Path $PSScriptRoot 'Get-OpalMatchedSoakAnalysis.ps1')
if ($DescribeOnly) {
    [pscustomobject]@{Order=$Order;LegSeconds=$LegSeconds;IntervalSeconds=$IntervalSeconds;Scenarios=@($Order.ToCharArray()|ForEach-Object{if($_ -eq 'A'){'Stock'}else{'FullSuite'}});Writes='Output receipt plus temporary Windhawk toggles with restoration';RestartsExplorer=$true;ReadsMemoryBytes=$false}
    return
}
if (-not [IO.Path]::IsPathRooted($OutputPath)) { throw 'Use an absolute output path.' }
if (Test-Path -LiteralPath $OutputPath) { throw 'Output already exists; preserve the earlier experiment.' }
$outputDirectory = Split-Path -Parent $OutputPath
if (-not (Test-Path -LiteralPath $outputDirectory -PathType Container)) { throw 'Output directory must already exist.' }
Initialize-OpalMemoryRegionReader
$sessionId = (Get-Process -Id $PID).SessionId
$modPath = 'HKLM:\SOFTWARE\Windhawk\Engine\Mods\local@opal'
$settingsPath = Join-Path $modPath 'Settings'
$corePath = Join-Path $env:LOCALAPPDATA 'Maxwell\Shell\Core\Maxwell.Shell.Core.exe'
$buildBytes = [IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $BaselineSnapshotBuildReceiptPath).Path)
$buildReceiptHash = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($buildBytes))
$buildText = [Text.Encoding]::UTF8.GetString($buildBytes).TrimStart([char]0xFEFF)
$build = $buildText | ConvertFrom-Json
if ($build.sha256 -notmatch '^[A-Fa-f0-9]{64}$' -or -not $build.version -or -not $build.dllName -or -not $build.sourceInputs -or @($build.sourceInputs).Count -eq 0) { throw 'Baseline build receipt must identify the artifact and source inputs.' }
foreach ($source in $build.sourceInputs) {
    if (-not $source.path -or $source.sha256 -notmatch '^[A-Fa-f0-9]{64}$') { throw 'Malformed baseline source provenance.' }
}
$coreHash = (Get-FileHash -LiteralPath $corePath).Hash
$attachmentScript = [scriptblock]::Create((Get-Content -LiteralPath (Join-Path $PSScriptRoot 'tests\Test-OpalAttachment.ps1') -Raw))

function Get-InteractiveExplorer {
    $found = @(Get-Process -Name explorer -ErrorAction SilentlyContinue | Where-Object SessionId -eq $sessionId)
    if ($found.Count -ne 1) { throw 'Exactly one interactive Explorer is required.' }
    return $found[0]
}
function Throw-RuntimeMismatch([string]$Message, $Observation) {
    $exception=[InvalidOperationException]::new("$Message Observation: $($Observation|ConvertTo-Json -Depth 5 -Compress)")
    $exception.Data['OpalRuntimeObservation']=$Observation
    throw $exception
}
function Assert-ScenarioRuntime([string]$Scenario, [switch]$HashReadback, [switch]$CheckAttachment) {
    $explorer = Get-InteractiveExplorer
    $aliases = @($explorer.Modules | Where-Object { Test-OpalModuleAlias $_.ModuleName })
    $disabled = Get-ItemPropertyValue -LiteralPath $modPath -Name Disabled
    $settings = Get-ItemProperty -LiteralPath $settingsPath
    $observation=[pscustomobject]@{
        Kind='RuntimeObservation';Scenario=$Scenario;Pid=$explorer.Id;StartedAtUtc=$explorer.StartTime.ToUniversalTime().ToString('o')
        SessionId=$sessionId;ObservedAtUtc=[DateTimeOffset]::UtcNow.ToString('o');Disabled=$disabled
        MediaEnabled=$settings.'media.mediaEnabled';PerformanceEnabled=$settings.'performance.performanceEnabled'
        ActualAliases=@(foreach($alias in $aliases){[pscustomobject]@{Name=$alias.ModuleName;Path=$alias.FileName}})
        ExpectedDisabled=[int]($Scenario -eq 'Stock');ExpectedAliasCount=[int]($Scenario -eq 'FullSuite')
        ExpectedDllName=$build.dllName;ExpectedDllSha256=$build.sha256;HashReadbackRequested=[bool]$HashReadback
    }
    if ($Scenario -eq 'Stock') {
        if ($aliases.Count -ne 0 -or $disabled -ne 1) { Throw-RuntimeMismatch 'Stock must have no Opal or legacy component DLL loaded.' $observation }
    } else {
        if ($aliases.Count -ne 1 -or $aliases[0].ModuleName -ne $build.dllName -or $disabled -ne 0) { Throw-RuntimeMismatch 'FullSuite installed runtime mismatch.' $observation }
        if ($settings.'media.mediaEnabled' -ne 1 -or $settings.'performance.performanceEnabled' -ne 1) { Throw-RuntimeMismatch 'FullSuite component settings changed.' $observation }
        if ($HashReadback -and (Get-FileHash -LiteralPath $aliases[0].FileName).Hash -ne $build.sha256) { Throw-RuntimeMismatch 'Installed DLL changed from the frozen baseline.' $observation }
        if ($CheckAttachment) { $null = & $attachmentScript }
    }
    if ($HashReadback -and (Get-FileHash -LiteralPath $corePath).Hash -ne $coreHash) { Throw-RuntimeMismatch 'Installed companion changed from frozen baseline.' $observation }
    [pscustomobject]@{Scenario=$Scenario;Pid=$explorer.Id;StartedAtUtc=$explorer.StartTime.ToUniversalTime().ToString('o');SessionId=$sessionId;VerifiedAtUtc=[DateTimeOffset]::UtcNow.ToString('o');HashReadback=[bool]$HashReadback;LoadedAliases=@(foreach($alias in $aliases){$alias.ModuleName});ProcessAgeSeconds=([DateTime]::Now-$explorer.StartTime).TotalSeconds}
}
function Clear-IntentionalExitMarkers {
    foreach ($name in @('media','performance')) {
        $path = Join-Path $env:LOCALAPPDATA "Maxwell\Opal\health-$name.ini"
        if (Test-Path -LiteralPath $path) {
            $text = [IO.File]::ReadAllText($path)
            [IO.File]::WriteAllText($path, [regex]::Replace($text,'(?m)^Dirty=\d+\r?$','Dirty=0'))
        }
    }
}
function Restart-InteractiveExplorer {
    param([string]$Reason='ScenarioTransition',[ValidateRange(0,10)][double]$AutomaticRestartGraceSeconds=5)
    Clear-IntentionalExitMarkers
    $old = @(Get-Process -Name explorer -ErrorAction SilentlyContinue | Where-Object SessionId -eq $sessionId)
    $oldIds = @($old.Id)
    $intent=[pscustomobject]@{Reason=$Reason;AtUtc=[DateTimeOffset]::UtcNow.ToString('o');OldPids=$oldIds;ExplicitLaunchRequested=$false;ReadyPid=$null;ReadyAtUtc=$null;MultiplicityObservations=[Collections.Generic.List[object]]::new()}
    $receipt.RestartEvents.Add($intent)
    Save-Receipt
    $old | Stop-Process -Force -ErrorAction Stop
    $timer = [Diagnostics.Stopwatch]::StartNew()
    do {
        Start-Sleep -Milliseconds 250
        $found = @(Get-Process -Name explorer -ErrorAction SilentlyContinue | Where-Object SessionId -eq $sessionId)
        if ($found.Count -eq 1 -and $found[0].Id -notin $oldIds) {
            $intent.ReadyPid=$found[0].Id;$intent.ReadyAtUtc=[DateTimeOffset]::UtcNow.ToString('o')
            Save-Receipt
            return
        }
        if ($found.Count -gt 1 -and $intent.MultiplicityObservations.Count -lt 32) {
            $intent.MultiplicityObservations.Add([pscustomobject]@{AtUtc=[DateTimeOffset]::UtcNow.ToString('o');Pids=@($found.Id)})
            Save-Receipt
        }
        # Let Windows' normal shell recovery act first; avoid launching a second
        # Explorer while an automatic replacement is already starting.
        if ($found.Count -eq 0 -and $timer.Elapsed.TotalSeconds -ge $AutomaticRestartGraceSeconds -and -not $intent.ExplicitLaunchRequested) {
            $intent.ExplicitLaunchRequested=$true
            Save-Receipt
            Start-Process -FilePath "$env:WINDIR\explorer.exe" -WindowStyle Hidden
        }
    } while ($timer.Elapsed.TotalSeconds -lt $ReadinessTimeoutSeconds)
    throw 'Explorer failed bounded restart verification.'
}
function Wait-ScenarioReady([string]$Scenario) {
    $timer = [Diagnostics.Stopwatch]::StartNew()
    $lastError = 'No runtime observation.'
    do {
        try { return Assert-ScenarioRuntime $Scenario -HashReadback -CheckAttachment }
        catch { $lastError = $_.Exception.Message }
        Start-Sleep -Milliseconds 500
    } while ($timer.Elapsed.TotalSeconds -lt $ReadinessTimeoutSeconds)
    throw "Runtime readiness timed out: $lastError"
}
function Save-Receipt {
    # Atomic checkpoints preserve partial evidence if a later operation fails.
    $temporary = "$OutputPath.partial"
    $receipt | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath $temporary -Encoding utf8
    Move-Item -LiteralPath $temporary -Destination $OutputPath -Force
}
function Wait-RestorationReady {
    $restoreTimer=[Diagnostics.Stopwatch]::StartNew()
    $lastObservationError=$null;$attempts=0
    do {
        $attempts++
        try {
            # Shell turnover can temporarily yield zero/multiple processes or stale
            # module handles. Retry only inside this bounded restoration window.
            $restoredExplorer=Get-InteractiveExplorer
            $candidatePid=[int]$restoredExplorer.Id
            $candidateStartedAtUtc=$restoredExplorer.StartTime.ToUniversalTime().ToString('o')
            $aliases=@($restoredExplorer.Modules|Where-Object{Test-OpalModuleAlias $_.ModuleName})
            $stateMatches = (Get-ItemPropertyValue -LiteralPath $modPath -Name Disabled) -eq $original.Disabled -and
                (Get-ItemPropertyValue -LiteralPath $settingsPath -Name 'media.mediaEnabled') -eq $original.Media -and
                (Get-ItemPropertyValue -LiteralPath $settingsPath -Name 'performance.performanceEnabled') -eq $original.Performance
            $dllMatches = if ($original.Disabled) {$aliases.Count -eq 0} else {$aliases.Count -eq 1 -and $aliases[0].ModuleName -eq $build.dllName -and (Get-FileHash -LiteralPath $aliases[0].FileName).Hash -eq $build.sha256}
            if(-not ($stateMatches -and $dllMatches)){throw "Restoration observation mismatch: SettingsMatch=$stateMatches; DllMatch=$dllMatches; Pid=$($restoredExplorer.Id)."}
            if(-not $original.Disabled){
                $attachment=@(& $attachmentScript)
                if($attachment.Count -ne 1 -or $attachment[0].explorerPid -ne $candidatePid){throw "Restoration attachment belongs to a different Explorer: ExpectedPid=$candidatePid; ObservedPids=$($attachment.explorerPid -join ',')."}
            }
            $finalExplorer=Get-InteractiveExplorer
            if($finalExplorer.Id -ne $candidatePid -or $finalExplorer.StartTime.ToUniversalTime().ToString('o') -ne $candidateStartedAtUtc){throw "Explorer lifetime changed during restoration verification: CandidatePid=$candidatePid; CandidateStartedAtUtc=$candidateStartedAtUtc; FinalPid=$($finalExplorer.Id)."}
            return [pscustomobject]@{Verified=$true;AtUtc=[DateTimeOffset]::UtcNow.ToString('o');Pid=$candidatePid;StartedAtUtc=$candidateStartedAtUtc;Settings=$original;ObservationAttempts=$attempts;LastObservationError=$lastObservationError}
        } catch {$lastObservationError=$_.Exception.Message}
        Start-Sleep -Milliseconds 500
    } while ($restoreTimer.Elapsed.TotalSeconds -lt $ReadinessTimeoutSeconds)
    $failure=[InvalidOperationException]::new("Restored settings/runtime did not pass bounded readback. Last observation: $lastObservationError")
    $failure.Data['LastRestorationObservationError']=$lastObservationError
    $failure.Data['RestorationObservationAttempts']=$attempts
    throw $failure
}

$initialExplorer = Get-InteractiveExplorer
$initialAliases = @($initialExplorer.Modules | Where-Object { Test-OpalModuleAlias $_.ModuleName })
if ($initialAliases.Count -ne 1 -or $initialAliases[0].ModuleName -ne $build.dllName -or
    (Get-FileHash -LiteralPath $initialAliases[0].FileName).Hash -ne $build.sha256) { throw 'Start with the expected installed Opal baseline loaded; build output alone is insufficient.' }
$initialProcesses=@(Get-OpalMeasuredProcessSnapshot -SessionId $sessionId)
$initialNames=(@($initialProcesses|Group-Object Name|Sort-Object Name|ForEach-Object{"$($_.Name)=$($_.Count)"}) -join ';')
$initialCore=@($initialProcesses|Where-Object Name -eq 'Maxwell.Shell.Core')
if($initialCore.Count -ne 1 -or (Get-Process -Id $initialCore[0].Pid -ErrorAction Stop).Path -ne $corePath){throw 'One running companion at the frozen installed path is required.'}
$original = [pscustomobject]@{
    Disabled=Get-ItemPropertyValue -LiteralPath $modPath -Name Disabled
    Media=Get-ItemPropertyValue -LiteralPath $settingsPath -Name 'media.mediaEnabled'
    Performance=Get-ItemPropertyValue -LiteralPath $settingsPath -Name 'performance.performanceEnabled'
}
$receipt = [ordered]@{
    SchemaVersion=1;Experiment='Matched Stock/FullSuite long observation';StartedAtUtc=[DateTimeOffset]::UtcNow.ToString('o');CapturedAtUtc=$null
    Complete=$false;Valid=$false;Error=$null;FailureObservation=$null;RestartEvents=[Collections.Generic.List[object]]::new();Order=$Order;LegSeconds=$LegSeconds;IntervalSeconds=$IntervalSeconds;StepThresholdMB=$StepThresholdMB
    SessionId=$sessionId;LogicalProcessors=[Environment]::ProcessorCount;OriginalSettings=$original
    Baseline=[pscustomobject]@{Version=$build.version;DllSha256=$build.sha256;DllName=$build.dllName;InstalledDllPath=$initialAliases[0].FileName;CoreSha256=$coreHash;SourceProvenance=$build;BuildReceiptPath=(Resolve-Path -LiteralPath $BaselineSnapshotBuildReceiptPath).Path;BuildReceiptSha256=$buildReceiptHash;Policy='Frozen installed artifact and build-receipt source provenance; mutable checkout/build outputs are deliberately not rehashed during measurement.'}
    Legs=[Collections.Generic.List[object]]::new();Restoration=$null
    Limitation='Four observational legs do not prove causality or leak freedom. Workload is not automated; CPU/private totals cover only interactive Explorer, companion and Windhawk. Region scans add observer work and contain no memory bytes.'
}
$runError = $null
try {
    Save-Receipt
    for ($position=1; $position -le 4; $position++) {
        $scenario = if ($Order[$position-1] -eq 'A') { 'Stock' } else { 'FullSuite' }
        Write-Host "Matched leg $position/4: $scenario, $LegSeconds seconds"
        Set-ItemProperty -LiteralPath $modPath -Name Disabled -Type DWord -Value ([int]($scenario -eq 'Stock'))
        Set-ItemProperty -LiteralPath $settingsPath -Name 'media.mediaEnabled' -Type DWord -Value ([int]($scenario -eq 'FullSuite'))
        Set-ItemProperty -LiteralPath $settingsPath -Name 'performance.performanceEnabled' -Type DWord -Value ([int]($scenario -eq 'FullSuite'))
        Restart-InteractiveExplorer -Reason "Leg$position/$scenario"
        $proof = Wait-ScenarioReady $scenario
        $leg = [ordered]@{Position=$position;Scenario=$scenario;StartProof=$proof;EndProof=$null;Complete=$false;Samples=[Collections.Generic.List[object]]::new();MemoryCaptures=[Collections.Generic.List[object]]::new()}
        $receipt.Legs.Add($leg)
        $timer = [Diagnostics.Stopwatch]::StartNew()
        $first = @(Get-OpalMeasuredProcessSnapshot -SessionId $sessionId)
        if (@($first | Where-Object Name -eq 'Maxwell.Shell.Core').Count -ne 1) { throw 'Exactly one companion is required in both scenarios for a controlled process set.' }
        $firstNames=(@($first|Group-Object Name|Sort-Object Name|ForEach-Object{"$($_.Name)=$($_.Count)"}) -join ';')
        if($firstNames -ne $initialNames){throw 'Process-name membership differs from the frozen initial session.'}
        $core=@($first|Where-Object Name -eq 'Maxwell.Shell.Core')[0]
        if((Get-Process -Id $core.Pid -ErrorAction Stop).Path -ne $corePath){throw 'Running companion path changed.'}
        $previous = $first
        $previousTime = $timer.Elapsed.TotalSeconds
        $leg.Samples.Add([pscustomobject]@{ElapsedSeconds=$previousTime;RuntimeVerified=$true;Processes=$first;IntervalCost=$null})
        $leg.MemoryCaptures.Add([pscustomobject]@{Reason='Startup';ElapsedSeconds=$timer.Elapsed.TotalSeconds;PrivateStepMB=0;Metadata=Get-OpalMemoryRegions -ProcessId $proof.Pid})
        $nextDue = $IntervalSeconds
        do {
            $sleep = [math]::Max(0, $nextDue-$timer.Elapsed.TotalSeconds)
            if ($sleep -gt 0) { Start-Sleep -Milliseconds ([int][math]::Ceiling($sleep*1000)) }
            $runtime = Assert-ScenarioRuntime $scenario
            if ($runtime.Pid -ne $proof.Pid -or $runtime.StartedAtUtc -ne $proof.StartedAtUtc) { throw 'Explorer changed during a leg.' }
            $now = @(Get-OpalMeasuredProcessSnapshot -SessionId $sessionId)
            $elapsed = $timer.Elapsed.TotalSeconds
            $interval = $elapsed-$previousTime
            if ($interval -gt $IntervalSeconds*2.5) { throw 'Sampling interval exceeded allowed timing gap; leg invalid.' }
            $cost = Compare-OpalMeasuredProcessSnapshot -Before $previous -After $now -ElapsedSeconds $interval
            $leg.Samples.Add([pscustomobject]@{ElapsedSeconds=$elapsed;RuntimeVerified=$true;Processes=$now;IntervalCost=$cost})
            $previousExplorer = @($previous | Where-Object Pid -eq $proof.Pid)[0]
            $currentExplorer = @($now | Where-Object Pid -eq $proof.Pid)[0]
            $step = $currentExplorer.PrivateMB-$previousExplorer.PrivateMB
            if ($step -ge $StepThresholdMB) {
                # Keep the triggering sample's time; metadata has its own capture time.
                $leg.MemoryCaptures.Add([pscustomobject]@{Reason='PrivateCommitStep';ElapsedSeconds=$elapsed;PrivateStepMB=$step;Metadata=Get-OpalMemoryRegions -ProcessId $proof.Pid})
            }
            $previous=$now; $previousTime=$elapsed; $nextDue += $IntervalSeconds
            if ($leg.Samples.Count % 10 -eq 0) { Save-Receipt }
        } while ($elapsed-$leg.Samples[0].ElapsedSeconds -lt $LegSeconds)
        $leg.EndProof = Assert-ScenarioRuntime $scenario -HashReadback -CheckAttachment
        $leg.MemoryCaptures.Add([pscustomobject]@{Reason='End';ElapsedSeconds=$timer.Elapsed.TotalSeconds;PrivateStepMB=0;Metadata=Get-OpalMemoryRegions -ProcessId $proof.Pid})
        $leg.Complete=$true
        Save-Receipt
    }
    $receipt.Complete=$true
} catch {
    $runError=$_.Exception.Message
    $receipt.Error=$runError
    $receipt.FailureObservation=$_.Exception.Data['OpalRuntimeObservation']
} finally {
    try {
        Set-ItemProperty -LiteralPath $modPath -Name Disabled -Type DWord -Value $original.Disabled
        Set-ItemProperty -LiteralPath $settingsPath -Name 'media.mediaEnabled' -Type DWord -Value $original.Media
        Set-ItemProperty -LiteralPath $settingsPath -Name 'performance.performanceEnabled' -Type DWord -Value $original.Performance
        Restart-InteractiveExplorer -Reason 'Restoration'
        $receipt.Restoration=Wait-RestorationReady
    } catch {
        $receipt.Restoration=[pscustomobject]@{Verified=$false;Error=$_.Exception.Message;LastObservationError=$_.Exception.Data['LastRestorationObservationError'];ObservationAttempts=$_.Exception.Data['RestorationObservationAttempts']}
        $receipt.Error="Run: $runError; restoration: $($_.Exception.Message)"
    }
    $receipt.CapturedAtUtc=[DateTimeOffset]::UtcNow.ToString('o')
    $receipt.Valid=$receipt.Complete -and $receipt.Restoration.Verified -and -not $receipt.Error
    Save-Receipt
}
if (-not $receipt.Valid) { throw "Matched soak incomplete/invalid: $($receipt.Error). Evidence: $OutputPath" }
try { Get-OpalMatchedSoakAnalysis -ReceiptPath $OutputPath }
catch {
    $receipt.Valid=$false
    $receipt.Error="Final evidence validation: $($_.Exception.Message)"
    Save-Receipt
    throw
}
