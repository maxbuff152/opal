[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
. (Join-Path $root 'Get-OpalMatchedSoakAnalysis.ps1')
. (Join-Path $root 'Get-OpalMemoryRegions.ps1')
Initialize-OpalMemoryRegionReader # Compile interop only; no live process query.
$scratch=Join-Path $root 'build\matched-soak-evidence-test'
New-Item -ItemType Directory -Path $scratch -Force|Out-Null
$path=Join-Path $scratch 'fixture.json'
$valid=@{SchemaVersion=1;Complete=$true;Valid=$true;Error=$null;Restoration=@{Verified=$true};Order='ABBA';LegSeconds=480;IntervalSeconds=5;StepThresholdMB=64;SessionId=1;LogicalProcessors=24;Baseline=@{Version='fixture';DllSha256='A'*64;CoreSha256='B'*64;DllName='local_at_opal_fixture.dll';SourceProvenance=@{sourceInputs=@(@{path='fixture.cpp';sha256='C'*64})}};Legs=@()}
foreach($position in 1..4){
    $scenario=if($position -in @(1,4)){'Stock'}else{'FullSuite'}
    $owner=100+$position;$started='2026-09-05T01:00:00Z'
    $proof=@{Scenario=$scenario;Pid=$owner;StartedAtUtc=$started;SessionId=1;HashReadback=$true;LoadedAliases=@()}
    if($scenario -eq 'FullSuite'){$proof.LoadedAliases=@('local_at_opal_fixture.dll')}
    $metadata=@{Pid=$owner;StartedAtUtc=$started;RegionClasses=@(@{State='Committed';Type='Private';Bytes=100MB;Regions=1});Modules=@(@{Name='explorer.exe';Path='C:\Windows\explorer.exe';MappedImageBytes=1MB})}
    $leg=@{Position=$position;Scenario=$scenario;Complete=$true;StartProof=$proof;EndProof=$proof;Samples=@();MemoryCaptures=@(@{Reason='Startup';ElapsedSeconds=0;PrivateStepMB=0;Metadata=$metadata},@{Reason='End';ElapsedSeconds=480;PrivateStepMB=0;Metadata=$metadata})}
    foreach($i in 0..96){
        $step=if($scenario -eq 'FullSuite' -and $i -ge 60){100}else{0}
        $leg.Samples+=@{ElapsedSeconds=$i*5;RuntimeVerified=$true;Processes=@(
            @{Name='explorer';Pid=$owner;StartedAtUtc=$started;CpuSeconds=10+$i*0.01;PrivateMB=300+$step;WorkingSetMB=350},
            @{Name='Maxwell.Shell.Core';Pid=200;StartedAtUtc=$started;CpuSeconds=1+$i*0.001;PrivateMB=8;WorkingSetMB=10},
            @{Name='windhawk';Pid=300;StartedAtUtc=$started;CpuSeconds=1;PrivateMB=2;WorkingSetMB=3}
        )}
    }
    if($scenario -eq 'FullSuite'){$leg.MemoryCaptures+=@{Reason='PrivateCommitStep';ElapsedSeconds=300;PrivateStepMB=100;Metadata=$metadata}}
    $valid.Legs+=$leg
}
function Run-Analysis($Value){$Value|ConvertTo-Json -Depth 16|Set-Content -LiteralPath $path;Get-OpalMatchedSoakAnalysis -ReceiptPath $path}
$result=Run-Analysis $valid
if(-not $result.EvidenceValid -or $result.StockSteps -ne 0 -or $result.FullSuiteSteps -ne 2 -or $result.Comparison.ExplorerPrivateMB.MeanGrowthDifference -ne 100){throw 'Known window/step comparison failed.'}
$cases=@(
    @{Name='incomplete run';Edit={param($r)$r.Complete=$false}},
    @{Name='failed restoration';Edit={param($r)$r.Restoration.Verified=$false}},
    @{Name='duplicate leg';Edit={param($r)$r.Legs[3].Position=1}},
    @{Name='missing leg';Edit={param($r)$r.Legs=@($r.Legs|Select-Object -First 3)}},
    @{Name='PID replacement';Edit={param($r)$r.Legs[0].Samples[20].Processes[0].Pid=999}},
    @{Name='PID reuse';Edit={param($r)$r.Legs[0].Samples[20].Processes[0].StartedAtUtc='new-start'}},
    @{Name='duplicate time';Edit={param($r)$r.Legs[0].Samples[20].ElapsedSeconds=95}},
    @{Name='sparse time';Edit={param($r)$r.Legs[0].Samples=@($r.Legs[0].Samples|Where-Object {$_.ElapsedSeconds -le 90 -or $_.ElapsedSeconds -ge 120})}},
    @{Name='missing counter';Edit={param($r)$r.Legs[0].Samples[20].Processes[0].PrivateMB=$null}},
    @{Name='nonfinite counter';Edit={param($r)$r.Legs[0].Samples[20].Processes[0].CpuSeconds=[double]::NaN}},
    @{Name='missing step capture';Edit={param($r)$r.Legs[1].MemoryCaptures=@($r.Legs[1].MemoryCaptures|Where-Object Reason -ne 'PrivateCommitStep')}},
    @{Name='stock alias';Edit={param($r)$r.Legs[0].StartProof.LoadedAliases=@('local_at_opal_old.dll')}},
    @{Name='runtime proof missing';Edit={param($r)$r.Legs[0].Samples[20].RuntimeVerified=$false}},
    @{Name='short leg';Edit={param($r)$r.Legs[0].Samples=@($r.Legs[0].Samples|Select-Object -First 70)}},
    @{Name='wrong metadata owner';Edit={param($r)$r.Legs[1].MemoryCaptures[0].Metadata.Pid=999}}
)
foreach($case in $cases){
    $copy=$valid|ConvertTo-Json -Depth 16|ConvertFrom-Json
    & $case.Edit $copy
    $rejected=$false
    try{$null=Run-Analysis $copy}catch{$rejected=$true}
    if(-not $rejected){throw "Invalid fixture accepted: $($case.Name)"}
}
foreach($alias in @('local_at_opal_4.5.0_owned.dll','local_at_maxwell-shell.dll','maxwell-taskbar-system-info.dll','MAXWELL-OPAL-MEDIA.DLL')){if(-not (Test-OpalModuleAlias $alias)){throw "Missed legacy alias: $alias"}}
if(Test-OpalModuleAlias 'kernel32.dll'){throw 'Unrelated module classified as Opal.'}
$samplerTokens=$null;$samplerErrors=$null
$samplerAst=[Management.Automation.Language.Parser]::ParseFile((Join-Path $root 'Measure-OpalMatchedSoak.ps1'),[ref]$samplerTokens,[ref]$samplerErrors)
$captureNodes=@($samplerAst.FindAll({param($node)$node -is [Management.Automation.Language.HashtableAst] -and $node.Extent.Text -match "Reason='PrivateCommitStep'"},$true))
if($samplerErrors.Count -or $captureNodes.Count -ne 1){throw 'Cannot isolate the production step-capture expression.'}
# Execute the actual production record expression with a delayed metadata provider.
# Its completion clock differs from the triggering sample, without querying a process.
$delayedCapture=& {
    param($Expression)
    $elapsed=300.0;$step=100.0;$proof=@{Pid=123}
    $timer=@{Elapsed=@{TotalSeconds=301.25}}
    function Get-OpalMemoryRegions([int]$ProcessId){[pscustomobject]@{Pid=$ProcessId;CapturedAtUtc='2026-09-05T01:05:01.250Z';CaptureSeconds=1.25}}
    & ([scriptblock]::Create($Expression))
} ('[pscustomobject]'+$captureNodes[0].Extent.Text)
if($delayedCapture.ElapsedSeconds -ne 300 -or $delayedCapture.Metadata.CaptureSeconds -ne 1.25){throw 'Metadata capture delay changed the triggering sample timestamp.'}
$proofNodes=@($samplerAst.FindAll({param($node)$node -is [Management.Automation.Language.HashtableAst] -and $node.Extent.Text -match 'LoadedAliases=' -and $node.Extent.Text -match 'ProcessAgeSeconds='},$true))
if($proofNodes.Count -ne 1){throw 'Cannot isolate actual runtime-proof producer.'}
$emptyProof=& {
    param($Expression)
    $Scenario='Stock';$aliases=@();$sessionId=1;$HashReadback=$true
    $explorer=[pscustomobject]@{Id=100;StartTime=[DateTime]::Now.AddMinutes(-1)}
    & ([scriptblock]::Create($Expression))
} ('[pscustomobject]'+$proofNodes[0].Extent.Text)
$serializedProof=$emptyProof|ConvertTo-Json -Depth 4
$readbackProof=$serializedProof|ConvertFrom-Json
if($serializedProof -notmatch '"LoadedAliases":\s*\[\s*\]' -or @($readbackProof.LoadedAliases).Count -ne 0){throw 'Actual Stock proof must serialize an empty array, not a null element.'}
$restartNodes=@($samplerAst.FindAll({param($node)$node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Restart-InteractiveExplorer'},$true))
if($restartNodes.Count -ne 1){throw 'Cannot isolate actual restart control flow.'}
foreach($case in @('auto','delayed-auto','explicit','duplicate','timeout')){
    & {
        param($FunctionText,$Case)
        $sessionId=1;$ReadinessTimeoutSeconds=if($Case -eq 'timeout'){0}else{1}
        $receipt=@{RestartEvents=[Collections.Generic.List[object]]::new()}
        $state=@{Queue=[Collections.Queue]::new();Launches=0;Stops=0;Saves=0}
        $old=[pscustomobject]@{Id=100;SessionId=1};$new=[pscustomobject]@{Id=200;SessionId=1};$extra=[pscustomobject]@{Id=300;SessionId=1}
        $state.Queue.Enqueue(@($old))
        switch($Case){
            'auto'{$state.Queue.Enqueue(@($new))}
            'delayed-auto'{$state.Queue.Enqueue(@());$state.Queue.Enqueue(@($new))}
            'explicit'{$state.Queue.Enqueue(@());$state.Queue.Enqueue(@($new))}
            'duplicate'{$state.Queue.Enqueue(@($new,$extra));$state.Queue.Enqueue(@($new))}
            'timeout'{$state.Queue.Enqueue(@())}
        }
        function Get-Process {param($Name,$ErrorAction) if($state.Queue.Count){$state.Queue.Dequeue()}}
        function Stop-Process {[CmdletBinding()]param([Parameter(ValueFromPipeline)]$InputObject,[switch]$Force)process{$state.Stops++}}
        function Start-Process {param($FilePath,$WindowStyle)$state.Launches++}
        function Start-Sleep {param($Milliseconds)}
        function Clear-IntentionalExitMarkers {}
        function Save-Receipt {$state.Saves++}
        & ([scriptblock]::Create($FunctionText))
        # Define into this test scope; the called implementation is the exact source body.
        . ([scriptblock]::Create($FunctionText))
        $failed=$false
        try{Restart-InteractiveExplorer -Reason "fixture/$Case" -AutomaticRestartGraceSeconds $(if($Case -eq 'explicit'){0}else{5})}catch{$failed=$true}
        if($failed -ne ($Case -eq 'timeout')){throw "Unexpected restart outcome: $Case"}
        if($state.Stops -ne 1 -or $state.Launches -ne [int]($Case -eq 'explicit')){throw "Wrong stop/launch control flow: $Case"}
        $event=$receipt.RestartEvents[0]
        if($event.OldPids[0] -ne 100 -or $event.ExplicitLaunchRequested -ne ($Case -eq 'explicit') -or $state.Saves -lt 1){throw "Incomplete restart intent: $Case"}
        if($Case -ne 'timeout' -and $event.ReadyPid -ne 200){throw "Missing observed replacement PID: $Case"}
        if($Case -eq 'duplicate' -and $event.MultiplicityObservations.Count -ne 1){throw 'Transient duplicate Explorer was not recorded.'}
    } $restartNodes[0].Extent.Text $case
}
$throwNode=@($samplerAst.FindAll({param($node)$node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Throw-RuntimeMismatch'},$true))[0]
& {
    param($FunctionText)
    . ([scriptblock]::Create($FunctionText))
    $observation=[pscustomobject]@{Pid=6688;Disabled=0;ActualAliases=@();ExpectedDllName='fixture.dll'}
    try{Throw-RuntimeMismatch 'fixture failure' $observation;throw 'Expected mismatch exception.'}catch{
        if($_.Exception.Data['OpalRuntimeObservation'].Pid -ne 6688 -or $_.Exception.Message -notmatch 'ActualAliases'){throw 'Mismatch exception lost discriminating evidence.'}
    }
} $throwNode.Extent.Text
$restorationNode=@($samplerAst.FindAll({param($node)$node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Wait-RestorationReady'},$true))[0]
if(-not $restorationNode){throw 'Cannot isolate restoration readiness implementation.'}
foreach($case in @('immediate','absence','multiplicity','module-error','attachment-error','settings-mismatch','timeout','attachment-pid-race','final-pid-race','final-birth-race')){
    & {
        param($FunctionText,$Case)
        . ([scriptblock]::Create($FunctionText))
        $ReadinessTimeoutSeconds=if($Case -eq 'timeout'){0}else{2}
        $modPath='fixture-mod';$settingsPath='fixture-settings'
        $original=[pscustomobject]@{Disabled=0;Media=1;Performance=1}
        $build=[pscustomobject]@{dllName='fixture-opal.dll';sha256='A'*64}
        $state=@{Reads=0;Sleeps=0;Attachments=0}
        function Get-InteractiveExplorer {
            $state.Reads++
            if($Case -eq 'timeout' -or ($state.Reads -eq 1 -and $Case -in @('absence','multiplicity'))){throw "fixture $Case process observation"}
            $id=if($Case -in @('attachment-pid-race','final-pid-race') -and $state.Reads -ge 2){300}else{200}
            $birth=if($Case -eq 'final-birth-race' -and $state.Reads -ge 2){[DateTime]'2026-09-05T01:00:01Z'}else{[DateTime]'2026-09-05T01:00:00Z'}
            [pscustomobject]@{Id=$id;StartTime=$birth;Modules=@([pscustomobject]@{ModuleName='fixture-opal.dll';FileName='fixture.dll'})}
        }
        function Get-ItemPropertyValue {param($LiteralPath,$Name)if($Name -eq 'Disabled'){if($Case -eq 'settings-mismatch' -and $state.Reads -eq 1){return 1};return 0};return 1}
        function Get-FileHash {param($LiteralPath)if($Case -eq 'module-error' -and $state.Reads -eq 1){throw 'fixture stale module observation'};[pscustomobject]@{Hash='A'*64}}
        function Start-Sleep {param($Milliseconds)$state.Sleeps++}
        $attachmentScript={
            $state.Attachments++;if($Case -eq 'attachment-error' -and $state.Attachments -eq 1){throw 'fixture attachment observation'}
            [pscustomobject]@{passed=$true;explorerPid=if($Case -eq 'attachment-pid-race' -or ($Case -eq 'final-pid-race' -and $state.Attachments -ge 2)){300}else{200}}
        }
        $result=$null;$failure=$null
        try{$result=Wait-RestorationReady}catch{$failure=$_.Exception}
        if($Case -eq 'timeout'){
            if($null -eq $failure -or $failure.Data['LastRestorationObservationError'] -ne 'fixture timeout process observation' -or $failure.Data['RestorationObservationAttempts'] -ne 1){throw 'Bounded restoration timeout lost final observation evidence.'}
        }else{
            $expectedPid=if($Case -in @('attachment-pid-race','final-pid-race')){300}else{200}
            if($failure -or -not $result.Verified -or $result.Pid -ne $expectedPid){throw "Restoration fixture failed: $Case $failure"}
            $expected=if($Case -eq 'immediate'){1}else{2}
            $expectedReads=if($Case -in @('final-pid-race','final-birth-race')){4}else{$expected+1}
            if($result.ObservationAttempts -ne $expected -or $state.Reads -ne $expectedReads){throw "Unexpected readiness retries: $Case"}
            if($Case -eq 'final-birth-race' -and [DateTime]$result.StartedAtUtc -ne [DateTime]'2026-09-05T01:00:01Z'){throw 'Restoration recorded stale process birth after retry.'}
            if($Case -ne 'immediate' -and -not $result.LastObservationError){throw "Transient observation error was discarded: $Case"}
        }
    } $restorationNode.Extent.Text $case
}
$plan=& (Join-Path $root 'Measure-OpalMatchedSoak.ps1') -OutputPath (Join-Path $scratch 'not-created.json') -DescribeOnly
if($plan.LegSeconds -ne 480 -or $plan.IntervalSeconds -ne 5 -or $plan.Scenarios.Count -ne 4 -or $plan.ReadsMemoryBytes){throw 'Unexpected sampler plan.'}
[pscustomobject]@{passed=$true;cases=40;evidence='Matched windows/steps and invalid-evidence rejection; actual serialized empty alias proof; actual restart control flow; bounded restoration retries including attachment PID and final PID/birth races, immediate success and final timeout diagnostics; detailed mismatch exception; delayed-capture timestamp. All process/restart calls stubbed; no live query or scenario ran.'}
