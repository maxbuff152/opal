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
$names=@($samples[0].Processes.Name)
if('explorer' -notin $names -or 'Maxwell.Shell.Core' -notin $names -or 'windhawk' -notin $names){throw 'Missing required processes.'}
$findings=[Collections.Generic.List[object]]::new();$rows=@()
foreach($first in $samples[0].Processes){
 $series=@($samples|ForEach-Object {@($_.Processes|Where-Object { $_.Pid -eq $first.Pid -and $_.Name -eq $first.Name })})
 if($series.Count -ne $samples.Count){throw 'Process restarted/disappeared; soak invalid.'}
 foreach($metric in @('PrivateMB','WorkingSetMB','Handles','Gdi','User','CpuSeconds')){
  $values=@($series|ForEach-Object {$_.PSObject.Properties[$metric].Value})
  foreach($value in $values){if($null -eq $value -or $value -is [string] -or [double]::IsNaN([double]$value) -or [double]::IsInfinity([double]$value) -or $value -lt 0){throw "Invalid soak $metric"}}
  $positive=0;$negative=0
  for($i=1;$i -lt $values.Count;$i++){if($values[$i] -gt $values[$i-1]){$positive++};if($values[$i] -lt $values[$i-1]){$negative++}}
  $delta=$values[-1]-$values[0]
  # Repeated growth with no release across the same process is a conservative
  # warning. A single allocation/cache step is not sufficient to call a leak.
  if($metric -in @('Handles','Gdi','User') -and $positive -ge 3 -and $negative -eq 0 -and $delta -gt 0){$findings.Add(@{process=$first.Name;pid=$first.Pid;metric=$metric;delta=$delta})}
 }
 $last=$series[-1]
 $rows += [pscustomobject]@{name=$first.Name;pid=$first.Pid;privateStartMB=[math]::Round($first.PrivateMB,2);privateEndMB=[math]::Round($last.PrivateMB,2);handleDelta=$last.Handles-$first.Handles;gdiDelta=$last.Gdi-$first.Gdi;userDelta=$last.User-$first.User;cpuPercentMachine=[math]::Round(($last.CpuSeconds-$first.CpuSeconds)/($samples[-1].ElapsedSeconds-$samples[0].ElapsedSeconds)/$receipt.LogicalProcessors*100,4)}
}
$result=[pscustomobject]@{passed=$findings.Count -eq 0;durationSeconds=$receipt.DurationSeconds;samples=$samples.Count;processes=$rows;sustainedObjectGrowth=@($findings);limitation='A five-minute soak can detect repeated object growth; it does not prove leak freedom. Private memory changes remain observations.'}
$result
if(-not $result.passed){throw 'Sustained object growth detected in soak.'}
