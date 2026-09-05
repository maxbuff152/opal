[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$tokens=$null;$parseErrors=$null
$ast=[Management.Automation.Language.Parser]::ParseFile((Join-Path $root 'Measure-OpalAllocationRebound.ps1'),[ref]$tokens,[ref]$parseErrors)
if($parseErrors.Count){throw ($parseErrors|Out-String)}
$functions=@($ast.FindAll({param($node)$node -is [Management.Automation.Language.FunctionDefinitionAst]},$true))
$source=($functions.Extent.Text -join "`n")
foreach($case in @('rebound','deadline','pid-change','dll-change','start-failure','start-status-failure','stop-failure','existing-instance','preflight-timeout','cleanup-status-timeout','memory-lifetime-change')){
    & {
        param($Source,$Case)
        . ([scriptblock]::Create($Source))
        $state=@{Elapsed=0.0;Private=300.0;Poll=0;Status=0;Stops=0;Starts=0;Writes=0;MemoryCalls=0;Commands=[Collections.Generic.List[object]]::new()}
        $build=[pscustomobject]@{version='fixture';dllName='fixture-opal.dll';sha256='A'*64}
        function Invoke-OpalReboundWpr {
            param([string[]]$Arguments,[int]$TimeoutSeconds=60)
            $state.Commands.Add(@($Arguments))
            if($Arguments[-2] -ne '-instancename' -or $Arguments[-1] -ne 'OpalAllocationRebound'){throw 'Instance name must be the final argument pair.'}
            if('-cancel' -in $Arguments){throw 'Unrelated recording cancellation is forbidden.'}
            $success=$true;$text='';$exitCode=0
            switch($Arguments[0]){
                '-status'{
                    $state.Status++
                    if($state.Status -eq 1){$text=if($Case -eq 'existing-instance'){'WPR is recording OpalAllocation'}else{'WPR is not recording'}}
                    elseif($state.Stops -gt 0){$text=if($Case -eq 'stop-failure'){'WPR is recording OpalAllocation'}else{'WPR is not recording'}}
                    else{$text='WPR is recording OpalAllocation';if($Case -eq 'start-status-failure'){$success=$false;$exitCode=1}}
                }
                '-start'{$state.Starts++;if($Case -eq 'start-failure'){$success=$false;$exitCode=1;$text='Start failed'}}
                '-stop'{$state.Stops++;if($Case -eq 'stop-failure'){$success=$false;$exitCode=1;$text='Stop failed'}}
                default{throw 'Unexpected WPR verb.'}
            }
            $timedOut=($Case -eq 'preflight-timeout' -and $state.Status -eq 1) -or ($Case -eq 'cleanup-status-timeout' -and $state.Stops -gt 0 -and $Arguments[0] -eq '-status')
            [pscustomobject]@{Arguments=$Arguments;Success=$success -and -not $timedOut;ExitCode=if($timedOut){$null}else{$exitCode};TimedOut=$timedOut;Output=$text;Error=''}
        }
        function Get-OpalReboundSnapshot {
            param([int]$ProcessId,[string]$Phase)
            if($Phase -eq 'Poll'){
                $state.Poll++
                $state.Private=if($Case -eq 'rebound'){@(210.0,205.0,272.0)[[math]::Min(2,$state.Poll-1)]}else{300.0-$state.Poll}
            }
            [pscustomobject]@{AtUtc='2026-09-05T01:00:00Z';Phase=$Phase;Pid=if($Case -eq 'pid-change' -and $Phase -eq 'Poll'){999}else{$ProcessId};Name='explorer';SessionId=1;StartedAtUtc='same-start';PrivateMB=$state.Private;WorkingSetMB=250.0;CpuSeconds=$state.Elapsed/100;Dlls=@([pscustomobject]@{Name='fixture-opal.dll';Path='fixture';Sha256=if($Case -eq 'dll-change' -and $Phase -eq 'Poll'){'B'*64}else{'A'*64}})}
        }
        function Get-OpalMemoryRegions {param([int]$ProcessId)$state.MemoryCalls++;[pscustomobject]@{Pid=$ProcessId;StartedAtUtc=if($Case -eq 'memory-lifetime-change'){'different-start'}else{'same-start'};RegionClasses=@();Modules=@();CapturedAtUtc='fixture'} }
        function Get-OpalReboundElapsed($Timer){return $state.Elapsed}
        function Wait-OpalReboundPoll([double]$Seconds){$state.Elapsed+=$Seconds}
        function Write-OpalReboundReceipt($Receipt,[string]$Path){$state.Writes++}
        function Get-OpalReboundTraceFile([string]$Path){[pscustomobject]@{Exists=$state.Stops -gt 0;Bytes=1024;LastWriteUtc='fixture'}}
        $result=Invoke-OpalAllocationReboundCapture -ProcessId 100 -TracePath 'fixture.etl' -MetadataPath 'fixture.json' -ProfilePath 'fixture.wprp' -Build $build -MaximumSeconds 6 -PollSeconds 2 -ThresholdMB 64
        $shouldOwn=$Case -notin @('start-failure','existing-instance','preflight-timeout')
        if($result.OwnershipAcquired -ne $shouldOwn -or $state.Stops -ne [int]$shouldOwn){throw "Incorrect owned cleanup: $Case"}
        if($state.Writes -lt 1){throw "Failed capture metadata was not saved: $Case"}
        if($result.LossStatus.State -ne 'UnknownPendingEtlAnalysis'){throw 'Command success must not imply zero event loss.'}
        if($Case -in @('rebound','deadline')){
            if(-not $result.SameProcessObservationValid -or -not $result.CompletedCapture -or -not $result.CleanupVerified){throw "Valid fixture lifecycle failed: $Case"}
        }elseif($result.SameProcessObservationValid){throw "Invalid fixture marked usable: $Case"}
        if($Case -eq 'rebound'){
            if(-not $result.SignalDetected -or $result.Nadir.PrivateMB -ne 205 -or $result.Rebound.ReboundMB -ne 67 -or $result.Rebound.ElapsedSeconds -ne 6){throw 'Actual decline/nadir/rebound calculation failed.'}
            if($result.Memory.Nadir.SampleElapsedSeconds -ne 4 -or $result.Memory.Rebound.SampleElapsedSeconds -ne 6){throw 'Memory metadata lost its triggering sample association.'}
        }
        if($Case -eq 'deadline' -and ($result.SignalDetected -or $result.Outcome -ne 'DeadlineWithoutRebound' -or $state.Elapsed -ne 6)){throw 'Deadline control flow failed.'}
        if($Case -eq 'existing-instance' -and $state.Starts -ne 0){throw 'Existing instance was modified.'}
    } $source $case
}
$plan=& (Join-Path $root 'Measure-OpalAllocationRebound.ps1') -TracePath 'unused.etl' -MetadataPath 'unused.json' -DescribeOnly
if($plan.MaximumObservationSeconds -ne 300 -or $plan.PollSeconds -ne 2 -or $plan.BufferMB -ne 64 -or $plan.RestartsExplorer -or $plan.ChangesSettings -or $plan.ReadsMemoryBytes){throw 'Unexpected live capture defaults.'}
[pscustomobject]@{passed=$true;cases=12;evidence='Real runner control flow with stubbed WPR/process/time: rebound, deadline, PID/DLL drift, region-scan lifetime drift, failed start/status/stop, occupied instance, ambiguous timed-out status and dry description. No live trace/process query and no fabricated live success.'}
