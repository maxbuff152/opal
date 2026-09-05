# Pure receipt validation and descriptive analysis; dot-sourcing performs no I/O.
function Test-OpalModuleAlias([string]$Name) {
    return $Name -match '(?i)(opal|maxwell.*(shell|taskbar|media|system.?info)).*\.dll$'
}

function Get-OpalMatchedSoakAnalysis {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$ReceiptPath)
    $ErrorActionPreference='Stop'
    $r=Get-Content -LiteralPath $ReceiptPath -Raw|ConvertFrom-Json
    function Number($Object,[string]$Name,[double]$Minimum=0) {
        $v=$Object.PSObject.Properties[$Name].Value
        if($null -eq $v -or $v -is [bool] -or $v -is [string] -or $v -isnot [ValueType] -or
            -not [double]::IsFinite([double]$v) -or [double]$v -lt $Minimum){throw "Invalid/missing $Name."}
        return [double]$v
    }
    function Mean($Values){return ($Values|Measure-Object -Average).Average}
    function Median($Values){$v=@($Values|Sort-Object);$m=[int][math]::Floor($v.Count/2);if($v.Count%2){return $v[$m]};return ($v[$m-1]+$v[$m])/2}
    function PrivateRegionMB($Capture){return (($Capture.Metadata.RegionClasses|Where-Object {$_.State -eq 'Committed' -and $_.Type -eq 'Private'}|Measure-Object -Property Bytes -Sum).Sum/1MB)}
    if($r.SchemaVersion -ne 1 -or $r.Complete -isnot [bool] -or -not $r.Complete -or
        $r.Valid -isnot [bool] -or -not $r.Valid -or $r.Error -or $r.Restoration.Verified -isnot [bool] -or -not $r.Restoration.Verified){throw 'Incomplete, failed or unrestored experiment cannot be analyzed as valid.'}
    $seconds=Number $r LegSeconds 480
    $interval=Number $r IntervalSeconds 5
    if($interval -gt 15){throw 'Sampling interval is too sparse.'}
    $threshold=Number $r StepThresholdMB 16
    $cpuCount=Number $r LogicalProcessors 1
    $null=Number $r SessionId
    if($r.Order -notin @('ABBA','BAAB') -or @($r.Legs).Count -ne 4){throw 'Four counterbalanced ABBA/BAAB legs required.'}
    if($r.Baseline.DllSha256 -notmatch '^[A-Fa-f0-9]{64}$' -or $r.Baseline.CoreSha256 -notmatch '^[A-Fa-f0-9]{64}$' -or
        -not $r.Baseline.Version -or -not $r.Baseline.SourceProvenance.sourceInputs -or @($r.Baseline.SourceProvenance.sourceInputs).Count -eq 0){throw 'Missing frozen build/source provenance.'}
    $rows=@();$positions=@{};$nameCounts=$null
    foreach($leg in $r.Legs){
        $position=Number $leg Position 1
        if($position -ne [math]::Floor($position) -or $position -gt 4 -or $positions.ContainsKey($position)){throw 'Duplicate/invalid leg position.'}
        $positions[$position]=$true
        $expected=if($r.Order[[int]$position-1] -eq 'A'){'Stock'}else{'FullSuite'}
        if($leg.Scenario -ne $expected -or $leg.Complete -isnot [bool] -or -not $leg.Complete){throw 'Incomplete/mislabelled leg.'}
        foreach($proof in @($leg.StartProof,$leg.EndProof)){
            $null=Number $proof Pid 1
            if($proof.Scenario -ne $expected -or $proof.SessionId -ne $r.SessionId -or -not $proof.StartedAtUtc -or
                $proof.HashReadback -isnot [bool] -or -not $proof.HashReadback){throw 'Missing runtime/hash/session proof.'}
            if($expected -eq 'Stock' -and @($proof.LoadedAliases).Count -ne 0){throw 'Stock contains an Opal alias.'}
            if($expected -eq 'FullSuite' -and (@($proof.LoadedAliases).Count -ne 1 -or $proof.LoadedAliases[0] -ne $r.Baseline.DllName)){throw 'FullSuite DLL proof mismatch.'}
        }
        if($leg.StartProof.Pid -ne $leg.EndProof.Pid -or $leg.StartProof.StartedAtUtc -ne $leg.EndProof.StartedAtUtc){throw 'Explorer changed across the leg.'}
        $samples=@($leg.Samples)
        if($samples.Count -lt [math]::Floor($seconds/($interval*2.5))+1){throw 'Insufficient same-process observations.'}
        $start=Number $samples[0] ElapsedSeconds
        $end=Number $samples[-1] ElapsedSeconds
        if($start -gt $interval -or $end-$start -lt $seconds -or $end-$start -gt $seconds+$interval*2.5){throw 'Invalid observed leg duration.'}
        $first=@($samples[0].Processes)
        if(@($first|Where-Object Name -eq 'explorer').Count -ne 1 -or @($first|Where-Object Name -eq 'Maxwell.Shell.Core').Count -ne 1){throw 'Required controlled process set is absent.'}
        $counts=(@($first|Group-Object Name|Sort-Object Name|ForEach-Object{"$($_.Name)=$($_.Count)"}) -join ';')
        if($null -eq $nameCounts){$nameCounts=$counts}elseif($counts -ne $nameCounts){throw 'Process-name membership differs between legs.'}
        $previous=$null;$previousTime=$null;$derivedSteps=@();$points=@()
        foreach($sample in $samples){
            $time=Number $sample ElapsedSeconds
            if($sample.RuntimeVerified -isnot [bool] -or -not $sample.RuntimeVerified){throw 'Unverified sample runtime.'}
            if($null -ne $previousTime -and ($time -le $previousTime -or $time-$previousTime -gt $interval*2.5)){throw 'Invalid/non-monotonic or sparse sample interval.'}
            $processes=@($sample.Processes)
            if($processes.Count -ne $first.Count){throw 'Process membership changed during a leg.'}
            foreach($p in $processes){
                $pidValue=Number $p Pid 1
                if($pidValue -ne [math]::Floor($pidValue) -or $p.Name -notin @('explorer','Maxwell.Shell.Core','windhawk') -or -not $p.StartedAtUtc){throw 'Invalid measured process identity.'}
                $match=@($first|Where-Object {$_.Pid -eq $p.Pid -and $_.Name -eq $p.Name -and $_.StartedAtUtc -eq $p.StartedAtUtc})
                if($match.Count -ne 1 -or @($processes|Where-Object Pid -eq $p.Pid).Count -ne 1){throw 'Process replacement/duplicate invalidates the leg.'}
                foreach($metric in @('CpuSeconds','PrivateMB','WorkingSetMB')){$null=Number $p $metric}
                if($null -ne $previous){
                    $prior=@($previous.Processes|Where-Object Pid -eq $p.Pid)[0]
                    if($p.CpuSeconds -lt $prior.CpuSeconds){throw 'CPU counter moved backwards.'}
                }
            }
            $explorer=@($processes|Where-Object Name -eq 'explorer')[0]
            if($explorer.Pid -ne $leg.StartProof.Pid -or $explorer.StartedAtUtc -ne $leg.StartProof.StartedAtUtc){throw 'Sample Explorer does not match runtime proof.'}
            if($null -ne $previous){
                $priorExplorer=@($previous.Processes|Where-Object Name -eq 'explorer')[0]
                $step=$explorer.PrivateMB-$priorExplorer.PrivateMB
                if($step -ge $threshold){$derivedSteps += [pscustomobject]@{ElapsedSeconds=$time;PrivateStepMB=$step}}
            }
            $points += [pscustomobject]@{ElapsedSeconds=$time;ExplorerPrivateMB=$explorer.PrivateMB;ExplorerWorkingSetMB=$explorer.WorkingSetMB;CombinedPrivateMB=($processes|Measure-Object PrivateMB -Sum).Sum}
            $previous=$sample;$previousTime=$time
        }
        $captures=@($leg.MemoryCaptures)
        $startup=@($captures|Where-Object Reason -eq 'Startup');$finish=@($captures|Where-Object Reason -eq 'End')
        $steps=@($captures|Where-Object Reason -eq 'PrivateCommitStep')
        if($startup.Count -ne 1 -or $finish.Count -ne 1 -or $steps.Count -ne $derivedSteps.Count){throw 'Missing/extra startup, end or step metadata captures.'}
        foreach($capture in $captures){
            if($capture.Reason -notin @('Startup','End','PrivateCommitStep') -or $capture.Metadata.Pid -ne $leg.StartProof.Pid -or
                $capture.Metadata.StartedAtUtc -ne $leg.StartProof.StartedAtUtc -or @($capture.Metadata.RegionClasses).Count -eq 0){throw 'Invalid memory metadata provenance.'}
            $null=Number $capture ElapsedSeconds
            foreach($region in $capture.Metadata.RegionClasses){$null=Number $region Bytes;$null=Number $region Regions 1}
        }
        foreach($step in $derivedSteps){
            $match=@($steps|Where-Object {[math]::Abs($_.ElapsedSeconds-$step.ElapsedSeconds) -lt 0.001 -and [math]::Abs($_.PrivateStepMB-$step.PrivateStepMB) -lt 0.001})
            if($match.Count -ne 1){throw 'Threshold step lacks corresponding region metadata.'}
        }
        $early=@($points|Where-Object {$_.ElapsedSeconds -le $start+60})
        $late=@($points|Where-Object {$_.ElapsedSeconds -ge $end-60})
        $memory=[ordered]@{}
        foreach($metric in @('ExplorerPrivateMB','ExplorerWorkingSetMB','CombinedPrivateMB')){
            $a=Mean $early.$metric;$b=Mean $late.$metric
            $memory[$metric]=[pscustomobject]@{EarlyMean=$a;LateMean=$b;LateMinusEarly=$b-$a}
        }
        $cpuRows=@(foreach($p in $first){$last=@($samples[-1].Processes|Where-Object Pid -eq $p.Pid)[0];[pscustomobject]@{Name=$p.Name;Pid=$p.Pid;CpuPercentOneCore=($last.CpuSeconds-$p.CpuSeconds)/($end-$start)*100;PrivateStartMB=$p.PrivateMB;PrivateEndMB=$last.PrivateMB;WorkingSetStartMB=$p.WorkingSetMB;WorkingSetEndMB=$last.WorkingSetMB}})
        $rows += [pscustomobject]@{Position=$position;Scenario=$expected;DurationSeconds=$end-$start;Samples=$samples.Count;Memory=[pscustomobject]$memory;StepOccurrences=$derivedSteps;PrivateCommittedRegionDeltaMB=(PrivateRegionMB $finish[0])-(PrivateRegionMB $startup[0]);AddedModules=@($finish[0].Metadata.Modules|Where-Object {$_.Path -notin @($startup[0].Metadata.Modules.Path)}|Select-Object Name,Path);RemovedModules=@($startup[0].Metadata.Modules|Where-Object {$_.Path -notin @($finish[0].Metadata.Modules.Path)}|Select-Object Name,Path);Processes=$cpuRows;CombinedCpuPercentOneCore=($cpuRows|Measure-Object CpuPercentOneCore -Sum).Sum}
    }
    $comparison=[ordered]@{}
    $stock=@($rows|Where-Object Scenario -eq 'Stock');$full=@($rows|Where-Object Scenario -eq 'FullSuite')
    foreach($metric in @('ExplorerPrivateMB','ExplorerWorkingSetMB','CombinedPrivateMB')){
        $a=@($stock|ForEach-Object{$_.Memory.$metric.LateMinusEarly});$b=@($full|ForEach-Object{$_.Memory.$metric.LateMinusEarly})
        $comparison[$metric]=[pscustomobject]@{StockLegGrowth=$a;FullSuiteLegGrowth=$b;MeanGrowthDifference=(Mean $b)-(Mean $a);FullSuiteLateMeanMinusStockLateMean=(Mean @($full|ForEach-Object{$_.Memory.$metric.LateMean}))-(Mean @($stock|ForEach-Object{$_.Memory.$metric.LateMean}))}
    }
    [pscustomobject]@{EvidenceValid=$true;Receipt=(Resolve-Path -LiteralPath $ReceiptPath).Path;Order=$r.Order;Baseline=$r.Baseline;Legs=@($rows|Sort-Object Position);Comparison=[pscustomobject]$comparison;StockSteps=@($stock|ForEach-Object{$_.StepOccurrences}|Where-Object{$null -ne $_}).Count;FullSuiteSteps=@($full|ForEach-Object{$_.StepOccurrences}|Where-Object{$null -ne $_}).Count;Limitation='Descriptive four-leg comparison of first/last 60-second windows. Workload differences, startup readiness and observer effects remain; region classes/module changes do not identify allocation stacks. No leak verdict, causal proof, total physical-memory claim or acceptance-budget change.'}
}
