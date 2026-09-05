<#
.SYNOPSIS
Observes one existing Explorer for a private-commit decline/rebound with a bounded ETW capture.
.DESCRIPTION
Run after the untraced comparison is complete. This does not restart Explorer,
change settings, disable guards, read memory contents, fetch symbols or upload data.
The named WPR instance is stopped only after this invocation's start succeeds.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$TracePath,
    [Parameter(Mandatory)][string]$MetadataPath,
    [ValidateRange(10,300)][int]$MaximumSeconds=300,
    [ValidateRange(1,5)][int]$PollSeconds=2,
    [ValidateRange(16,256)][double]$ThresholdMB=64,
    [string]$BuildReceiptPath=(Join-Path $PSScriptRoot 'build\opal-suite\build-receipt.json'),
    [switch]$DescribeOnly
)

function Invoke-OpalReboundWpr {
    param([string[]]$Arguments,[int]$TimeoutSeconds=60)
    $psi=[Diagnostics.ProcessStartInfo]::new()
    $psi.FileName=Join-Path $env:WINDIR 'System32\wpr.exe'
    $psi.UseShellExecute=$false;$psi.CreateNoWindow=$true
    $psi.RedirectStandardOutput=$true;$psi.RedirectStandardError=$true
    foreach($argument in $Arguments){$psi.ArgumentList.Add($argument)}
    $process=[Diagnostics.Process]::new();$process.StartInfo=$psi
    $timer=[Diagnostics.Stopwatch]::StartNew()
    try {
        if(-not $process.Start()){throw 'Could not launch the owned WPR command.'}
        $output=$process.StandardOutput.ReadToEndAsync();$errors=$process.StandardError.ReadToEndAsync()
        $finished=$process.WaitForExit($TimeoutSeconds*1000)
        if(-not $finished){
            # This terminates only this invocation's hung CLI process. It does not
            # cancel any recording; uncertain start/stop state is retained as an error.
            try{$process.Kill()}catch{}
            $null=$process.WaitForExit(3000)
        }
        $stdout=if($output.Wait(1000)){$output.Result}else{'WPR stdout incomplete.'}
        $stderr=if($errors.Wait(1000)){$errors.Result}else{'WPR stderr incomplete.'}
        [pscustomobject]@{Arguments=$Arguments;ExitCode=if($finished){$process.ExitCode}else{$null};TimedOut=-not $finished;Success=$finished -and $process.ExitCode -eq 0;Output=$stdout;Error=$stderr;ElapsedSeconds=$timer.Elapsed.TotalSeconds}
    } finally {$process.Dispose()}
}

function Get-OpalReboundSnapshot {
    param([int]$ProcessId,[string]$Phase)
    $process=Get-Process -Id $ProcessId -ErrorAction Stop
    $process.Refresh()
    $aliases=@($process.Modules|Where-Object{Test-OpalModuleAlias $_.ModuleName})
    $dlls=@(foreach($module in $aliases){[pscustomobject]@{Name=$module.ModuleName;Path=$module.FileName;Sha256=(Get-FileHash -LiteralPath $module.FileName).Hash}})
    [pscustomobject]@{AtUtc=[DateTimeOffset]::UtcNow.ToString('o');Phase=$Phase;Pid=$process.Id;Name=$process.ProcessName;SessionId=$process.SessionId;StartedAtUtc=$process.StartTime.ToUniversalTime().ToString('o');CpuSeconds=$process.CPU;PrivateMB=$process.PrivateMemorySize64/1MB;WorkingSetMB=$process.WorkingSet64/1MB;Dlls=$dlls}
}
function Get-OpalReboundElapsed($Timer){return $Timer.Elapsed.TotalSeconds}
function Get-OpalReboundMemory($Snapshot,[double]$ElapsedSeconds){
    $metadata=Get-OpalMemoryRegions -ProcessId $Snapshot.Pid
    if($metadata.Pid -ne $Snapshot.Pid -or $metadata.StartedAtUtc -ne $Snapshot.StartedAtUtc){throw 'Memory-region scan belongs to a different process lifetime.'}
    return [pscustomobject]@{SampleElapsedSeconds=$ElapsedSeconds;Snapshot=$Snapshot;Metadata=$metadata}
}
function Wait-OpalReboundPoll([double]$Seconds){if($Seconds -gt 0){Start-Sleep -Milliseconds ([int][math]::Ceiling($Seconds*1000))}}
function Write-OpalReboundReceipt($Receipt,[string]$Path){
    $temporary="$Path.partial"
    $Receipt|ConvertTo-Json -Depth 16|Set-Content -LiteralPath $temporary -Encoding utf8
    Move-Item -LiteralPath $temporary -Destination $Path -Force
}
function Get-OpalReboundTraceFile([string]$Path){
    if(Test-Path -LiteralPath $Path -PathType Leaf){$file=Get-Item -LiteralPath $Path;return [pscustomobject]@{Exists=$true;Bytes=$file.Length;LastWriteUtc=$file.LastWriteTimeUtc.ToString('o')}}
    return [pscustomobject]@{Exists=$false;Bytes=0;LastWriteUtc=$null}
}
function Assert-OpalReboundSnapshot($Snapshot,$Before,$Build){
    if($Snapshot.Pid -ne $Before.Pid -or $Snapshot.StartedAtUtc -ne $Before.StartedAtUtc -or
        $Snapshot.SessionId -ne $Before.SessionId -or $Snapshot.Name -ne 'explorer'){throw 'Target Explorer identity/session changed.'}
    foreach($name in @('PrivateMB','WorkingSetMB','CpuSeconds')){
        $value=$Snapshot.PSObject.Properties[$name].Value
        if($null -eq $value -or $value -is [bool] -or $value -is [string] -or -not [double]::IsFinite([double]$value) -or $value -lt 0){throw "Unavailable/invalid $name counter."}
    }
    if(@($Snapshot.Dlls).Count -ne 1 -or $Snapshot.Dlls[0].Name -ne $Build.dllName -or
        $Snapshot.Dlls[0].Sha256 -ne $Build.sha256){throw 'Current loaded DLL does not match frozen build provenance.'}
}
function Test-OpalReboundNotRecording($Result){
    # Fail closed if WPR's status is ambiguous. Never add to an existing named recording.
    return (-not $Result.TimedOut -and (($Result.Output+"`n"+$Result.Error) -match '(?i)(WPR is not recording|not currently recording|no recording (is )?(in progress|running)|not recording any profiles)'))
}

function Invoke-OpalAllocationReboundCapture {
    param([int]$ProcessId,[string]$TracePath,[string]$MetadataPath,[string]$ProfilePath,$Build,[int]$MaximumSeconds=300,[int]$PollSeconds=2,[double]$ThresholdMB=64)
    $instance='OpalAllocationRebound'
    $owned=$false;$startSucceeded=$false;$stopSucceeded=$false;$observationOkay=$false
    $receipt=[ordered]@{
        SchemaVersion=1;Experiment='One-shot private-commit decline/rebound allocation trace';StartedAtUtc=[DateTimeOffset]::UtcNow.ToString('o');FinishedAtUtc=$null
        Outcome='Starting';SignalDetected=$false;CompletedCapture=$false;SameProcessObservationValid=$false
        MaximumObservationSeconds=$MaximumSeconds;PollSeconds=$PollSeconds;ThresholdMB=$ThresholdMB;TargetPid=$ProcessId
        TracePath=$TracePath;InstanceName=$instance;Profile=$ProfilePath;BuildProvenance=$Build
        Before=$null;After=$null;LastObserved=$null;Decline=$null;Nadir=$null;Rebound=$null
        Memory=[ordered]@{Start=$null;Nadir=$null;Rebound=$null;End=$null}
        Samples=[Collections.Generic.List[object]]::new();Commands=[Collections.Generic.List[object]]::new();Errors=[Collections.Generic.List[string]]::new()
        OwnershipAcquired=$false;CleanupAttempted=$false;CleanupVerified=$false;TraceFile=$null
        LossStatus=[pscustomobject]@{State='UnknownPendingEtlAnalysis';ReportedLossLines=@();CircularHistoryMayBeOverwritten=$true;Reason='WPR command success does not prove zero lost events or complete circular history; ETL integrity/event-loss analysis remains required.'}
        Limitation='Allocation/free stacks and metadata only, with no raw process-memory dump. The existing profile records kernel allocation events beyond Explorer; analysis must filter exact PID/lifetime. Gross allocations, a dip/rebound and region classes do not establish a leak or causal ownership. WPR commands/cleanup can extend wall time beyond the observation deadline.'
    }
    try {
        $before=Get-OpalReboundSnapshot -ProcessId $ProcessId -Phase 'Before'
        $receipt.Before=$before
        Assert-OpalReboundSnapshot $before $before $Build
        $preflight=Invoke-OpalReboundWpr -Arguments @('-status','-instancename',$instance)
        $receipt.Commands.Add($preflight)
        if(-not (Test-OpalReboundNotRecording $preflight)){throw 'Named WPR instance is active or its state is unknown; no start/stop attempted.'}
        $start=Invoke-OpalReboundWpr -Arguments @('-start',"$ProfilePath!OpalAllocation",'-instancename',$instance)
        $receipt.Commands.Add($start)
        if(-not $start.Success){throw 'WPR start did not succeed; ownership was not acquired and no stop will be issued.'}
        $owned=$true;$startSucceeded=$true;$receipt.OwnershipAcquired=$true
        Write-OpalReboundReceipt $receipt $MetadataPath
        $status=Invoke-OpalReboundWpr -Arguments @('-status','-instancename',$instance)
        $receipt.Commands.Add($status)
        if(-not $status.Success -or (Test-OpalReboundNotRecording $status)){throw 'Owned WPR recording did not pass status readback.'}
        $timer=[Diagnostics.Stopwatch]::StartNew()
        $first=Get-OpalReboundSnapshot -ProcessId $ProcessId -Phase 'Start'
        Assert-OpalReboundSnapshot $first $before $Build
        $point=[pscustomobject]@{ElapsedSeconds=Get-OpalReboundElapsed $timer;Snapshot=$first}
        $receipt.Samples.Add($point);$receipt.LastObserved=$first
        $receipt.Memory.Start=Get-OpalReboundMemory $first $point.ElapsedSeconds
        $peak=$point;$nadir=$null
        $receipt.Outcome='Observing'
        Write-OpalReboundReceipt $receipt $MetadataPath
        while((Get-OpalReboundElapsed $timer) -lt $MaximumSeconds){
            $remaining=$MaximumSeconds-(Get-OpalReboundElapsed $timer)
            Wait-OpalReboundPoll ([math]::Min($PollSeconds,[math]::Max(0,$remaining)))
            $snapshot=Get-OpalReboundSnapshot -ProcessId $ProcessId -Phase 'Poll'
            $receipt.LastObserved=$snapshot
            Assert-OpalReboundSnapshot $snapshot $before $Build
            $time=Get-OpalReboundElapsed $timer
            $point=[pscustomobject]@{ElapsedSeconds=$time;Snapshot=$snapshot}
            $previous=$receipt.Samples[$receipt.Samples.Count-1]
            if($time -le $previous.ElapsedSeconds -or $time-$previous.ElapsedSeconds -gt $PollSeconds*3){throw 'Invalid or excessively delayed observation interval.'}
            if($snapshot.CpuSeconds -lt $previous.Snapshot.CpuSeconds){throw 'Target CPU counter moved backwards.'}
            $receipt.Samples.Add($point)
            if($null -eq $nadir){
                if($snapshot.PrivateMB -gt $peak.Snapshot.PrivateMB){$peak=$point}
                if($peak.Snapshot.PrivateMB-$snapshot.PrivateMB -ge $ThresholdMB){
                    $receipt.Decline=[pscustomobject]@{PeakElapsedSeconds=$peak.ElapsedSeconds;PeakPrivateMB=$peak.Snapshot.PrivateMB;DetectedElapsedSeconds=$time;DetectedPrivateMB=$snapshot.PrivateMB;DeclineMB=$peak.Snapshot.PrivateMB-$snapshot.PrivateMB}
                    $nadir=$point
                    $receipt.Memory.Nadir=Get-OpalReboundMemory $snapshot $time
                }
            } elseif($snapshot.PrivateMB -lt $nadir.Snapshot.PrivateMB){
                $nadir=$point
                # Keep the exact latest low-point metadata, not every superseded map.
                $receipt.Memory.Nadir=Get-OpalReboundMemory $snapshot $time
            }
            if($null -ne $nadir){
                $receipt.Nadir=[pscustomobject]@{ElapsedSeconds=$nadir.ElapsedSeconds;PrivateMB=$nadir.Snapshot.PrivateMB;AtUtc=$nadir.Snapshot.AtUtc}
                if($snapshot.PrivateMB-$nadir.Snapshot.PrivateMB -ge $ThresholdMB){
                    $receipt.Rebound=[pscustomobject]@{ElapsedSeconds=$time;PrivateMB=$snapshot.PrivateMB;ReboundMB=$snapshot.PrivateMB-$nadir.Snapshot.PrivateMB;AtUtc=$snapshot.AtUtc}
                    $receipt.Memory.Rebound=Get-OpalReboundMemory $snapshot $time
                    $receipt.SignalDetected=$true;$receipt.Outcome='ReboundObserved'
                    break
                }
            }
            if($receipt.Samples.Count%5 -eq 0){Write-OpalReboundReceipt $receipt $MetadataPath}
        }
        if(-not $receipt.SignalDetected){$receipt.Outcome='DeadlineWithoutRebound'}
        $after=Get-OpalReboundSnapshot -ProcessId $ProcessId -Phase 'After'
        $receipt.After=$after
        Assert-OpalReboundSnapshot $after $before $Build
        $receipt.Memory.End=Get-OpalReboundMemory $after (Get-OpalReboundElapsed $timer)
        $observationOkay=$true
    } catch {
        $receipt.Outcome='Error';$receipt.Errors.Add($_.Exception.Message)
    } finally {
        if($owned){
            $receipt.CleanupAttempted=$true
            try {
                $stop=Invoke-OpalReboundWpr -Arguments @('-stop',$TracePath,'-instancename',$instance)
                $receipt.Commands.Add($stop)
                $stopSucceeded=[bool]$stop.Success
                $receipt.LossStatus.ReportedLossLines=@(($stop.Output+"`n"+$stop.Error) -split "`r?`n"|Where-Object{$_ -match '(?i)(events? lost|lost events?|buffers? lost|loss)'})
                if(-not $stopSucceeded){$receipt.Errors.Add('Owned WPR stop failed or timed out; recording state requires inspection. No unrelated instance was cancelled.')}
                $post=Invoke-OpalReboundWpr -Arguments @('-status','-instancename',$instance)
                $receipt.Commands.Add($post)
                $receipt.CleanupVerified=$stopSucceeded -and (Test-OpalReboundNotRecording $post)
                if(-not $receipt.CleanupVerified){$receipt.Errors.Add('Owned recording stop did not pass status readback.')}
            } catch {$receipt.Errors.Add("Owned WPR cleanup exception: $($_.Exception.Message)")}
        }
        $receipt.TraceFile=Get-OpalReboundTraceFile $TracePath
        $receipt.CompletedCapture=$startSucceeded -and $stopSucceeded -and $receipt.CleanupVerified -and $receipt.TraceFile.Exists -and $receipt.TraceFile.Bytes -gt 0
        $receipt.SameProcessObservationValid=$observationOkay -and $receipt.CompletedCapture -and $receipt.Errors.Count -eq 0
        $receipt.FinishedAtUtc=[DateTimeOffset]::UtcNow.ToString('o')
        Write-OpalReboundReceipt $receipt $MetadataPath
    }
    [pscustomobject]$receipt
}

$ErrorActionPreference='Stop'
if($DescribeOnly){
    [pscustomobject]@{Instance='OpalAllocationRebound';MaximumObservationSeconds=$MaximumSeconds;PollSeconds=$PollSeconds;ThresholdMB=$ThresholdMB;Profile='config/OpalAllocation.wprp!OpalAllocation';BufferMB=64;RestartsExplorer=$false;ChangesSettings=$false;ReadsMemoryBytes=$false;LossStatus='Unknown until ETL analysis';Cleanup='Stop only after successful owned start; never cancel another recording.'}
    return
}
foreach($entry in @(@{Path=$TracePath;Root=(Join-Path $PSScriptRoot 'build');Extension='.etl'},@{Path=$MetadataPath;Root=(Join-Path $PSScriptRoot 'measurements');Extension='.json'})){
    if(-not [IO.Path]::IsPathRooted($entry.Path)){throw 'Capture paths must be absolute.'}
    $resolved=[IO.Path]::GetFullPath($entry.Path);$prefix=[IO.Path]::GetFullPath($entry.Root).TrimEnd('\')+'\'
    if(-not $resolved.StartsWith($prefix,[StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::GetExtension($resolved) -ne $entry.Extension){throw 'Trace must be a new .etl beneath this repository build folder; metadata a new .json beneath measurements.'}
    if((Test-Path -LiteralPath $resolved) -or (Test-Path -LiteralPath "$resolved.partial")){throw 'Preserve existing capture evidence; choose new paths.'}
    if(-not (Test-Path -LiteralPath (Split-Path -Parent $resolved) -PathType Container)){throw 'Capture parent directories must already exist.'}
}
$profile=Join-Path $PSScriptRoot 'config\OpalAllocation.wprp'
$xml=[xml](Get-Content -LiteralPath $profile -Raw)
$collector=$xml.WindowsPerformanceRecorder.Profiles.SystemCollector
if(@($collector).Count -ne 1 -or [int]$collector.BufferSize.Value*[int]$collector.Buffers.Value -ne 65536){throw 'Expected the existing bounded 64MB allocation profile.'}
$build=Get-Content -LiteralPath $BuildReceiptPath -Raw|ConvertFrom-Json
if($build.sha256 -notmatch '^[A-Fa-f0-9]{64}$' -or -not $build.version -or -not $build.dllName){throw 'Build receipt lacks frozen DLL provenance.'}
$build|Add-Member -NotePropertyName CaptureProfileSha256 -NotePropertyValue (Get-FileHash -LiteralPath $profile).Hash
. (Join-Path $PSScriptRoot 'Get-OpalMemoryRegions.ps1')
. (Join-Path $PSScriptRoot 'Get-OpalMatchedSoakAnalysis.ps1')
Initialize-OpalMemoryRegionReader
$session=(Get-Process -Id $PID).SessionId
$explorer=@(Get-Process -Name explorer -ErrorAction Stop|Where-Object SessionId -eq $session)
if($explorer.Count -ne 1){throw 'Exactly one interactive Explorer is required.'}
$result=Invoke-OpalAllocationReboundCapture -ProcessId $explorer[0].Id -TracePath $TracePath -MetadataPath $MetadataPath -ProfilePath $profile -Build $build -MaximumSeconds $MaximumSeconds -PollSeconds $PollSeconds -ThresholdMB $ThresholdMB
$result
if(-not $result.SameProcessObservationValid){throw "Allocation capture requires attention; preserve $MetadataPath and $TracePath. $($result.Errors -join ' ')"}
