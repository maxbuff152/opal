[CmdletBinding()]
param([Parameter(Mandatory)][string]$ReceiptPath)
$ErrorActionPreference='Stop'
$receipt=Get-Content -LiteralPath $ReceiptPath -Raw|ConvertFrom-Json
$build=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'build\opal-suite\build-receipt.json') -Raw|ConvertFrom-Json
$coreHash=(Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'build\maxwell-shell-core\Maxwell.Shell.Core.exe')).Hash
if($receipt.SchemaVersion -ne 1 -or $receipt.Build.DllSha256 -ne $build.sha256 -or $receipt.Build.CoreSha256 -ne $coreHash){throw 'Soak build/schema mismatch.'}
$age=([DateTimeOffset]::Now-[DateTimeOffset]::Parse($receipt.CapturedAt)).TotalHours
if($age -lt -0.02 -or $age -gt 24){throw 'Stale/future soak.'}
$samples=@($receipt.Samples)
if($samples.Count -lt 11 -or $receipt.DurationSeconds -lt 300 -or $samples[-1].ElapsedSeconds-$samples[0].ElapsedSeconds -lt 300){throw 'At least five minutes of same-process samples required.'}
if($null -eq $receipt.LogicalProcessors -or -not [double]::IsFinite([double]$receipt.LogicalProcessors) -or $receipt.LogicalProcessors -lt 1){throw 'Missing CPU normalization.'}
if(-not [double]::IsFinite([double]$receipt.DurationSeconds) -or [math]::Abs($receipt.DurationSeconds-$samples[-1].ElapsedSeconds) -gt 1){throw 'Inconsistent soak duration.'}
$previous=-1.0
foreach($sample in $samples){
 if($null -eq $sample.ElapsedSeconds -or -not [double]::IsFinite([double]$sample.ElapsedSeconds) -or $sample.ElapsedSeconds -le $previous -or $sample.ElapsedSeconds-$previous -gt 60){throw 'Invalid/non-monotonic or sparse soak timestamps.'}
 $previous=$sample.ElapsedSeconds
 if(@($sample.Processes).Count -ne @($samples[0].Processes).Count){throw 'Process set changed during soak.'}
 foreach($first in $samples[0].Processes){if(@($sample.Processes|Where-Object {$_.Pid -eq $first.Pid -and $_.Name -eq $first.Name}).Count -ne 1){throw 'Duplicate or missing process identity.'}}
}
$names=@($samples[0].Processes.Name)
if('explorer' -notin $names -or 'Maxwell.Shell.Core' -notin $names -or 'windhawk' -notin $names){throw 'Missing required processes.'}
$findings=[Collections.Generic.List[object]]::new();$rows=@()
function Get-WindowMedian($Values){$sorted=@($Values|Sort-Object);$m=[int][math]::Floor($sorted.Count/2);if($sorted.Count%2){return $sorted[$m]};return ($sorted[$m-1]+$sorted[$m])/2}
foreach($first in $samples[0].Processes){
 $series=@($samples|ForEach-Object {@($_.Processes|Where-Object { $_.Pid -eq $first.Pid -and $_.Name -eq $first.Name })})
 if($series.Count -ne $samples.Count){throw 'Process restarted/disappeared; soak invalid.'}
 foreach($metric in @('PrivateMB','WorkingSetMB','Handles','Gdi','User','CpuSeconds')){
  $values=@($series|ForEach-Object {$_.PSObject.Properties[$metric].Value})
  foreach($value in $values){if($null -eq $value -or $value -is [string] -or [double]::IsNaN([double]$value) -or [double]::IsInfinity([double]$value) -or $value -lt 0){throw "Invalid soak $metric"}}
  $positive=0;$negative=0
  for($i=1;$i -lt $values.Count;$i++){if($values[$i] -gt $values[$i-1]){$positive++};if($values[$i] -lt $values[$i-1]){$negative++}}
  $delta=$values[-1]-$values[0]
  # Compare early/middle/late windows, allowing occasional releases. A single
  # allocation followed by a plateau must not masquerade as a sustained trend.
  $third=[int][math]::Floor($values.Count/3)
  $early=Get-WindowMedian $values[0..($third-1)]
  $middle=Get-WindowMedian $values[$third..(2*$third-1)]
  $late=Get-WindowMedian $values[(2*$third)..($values.Count-1)]
  if($metric -in @('Handles','Gdi','User') -and $positive -ge 3 -and $delta -gt 0 -and $early -lt $middle -and $middle -lt $late){$findings.Add(@{process=$first.Name;pid=$first.Pid;metric=$metric;delta=$delta;windowMedians=@($early,$middle,$late)})}
 }
 $last=$series[-1]
 $rows += [pscustomobject]@{name=$first.Name;pid=$first.Pid;privateStartMB=[math]::Round($first.PrivateMB,2);privateEndMB=[math]::Round($last.PrivateMB,2);handleDelta=$last.Handles-$first.Handles;gdiDelta=$last.Gdi-$first.Gdi;userDelta=$last.User-$first.User;cpuPercentMachine=[math]::Round(($last.CpuSeconds-$first.CpuSeconds)/($samples[-1].ElapsedSeconds-$samples[0].ElapsedSeconds)/$receipt.LogicalProcessors*100,4)}
}
$result=[pscustomobject]@{passed=$findings.Count -eq 0;durationSeconds=$receipt.DurationSeconds;samples=$samples.Count;processes=$rows;sustainedObjectGrowth=@($findings);limitation='A five-minute soak can detect repeated object growth; it does not prove leak freedom. Private memory changes remain observations.'}
$result
if(-not $result.passed){throw 'Sustained object growth detected in soak.'}
