[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Get-Content -LiteralPath (Join-Path $root 'build\opal-suite\build-receipt.json') -Raw|ConvertFrom-Json
$coreHash=(Get-FileHash -LiteralPath (Join-Path $root 'build\maxwell-shell-core\Maxwell.Shell.Core.exe')).Hash
$scratch=Join-Path $root 'build\soak-evidence-test'
New-Item -ItemType Directory -Path $scratch -Force|Out-Null
$valid=@{SchemaVersion=1;CapturedAt=[DateTimeOffset]::Now.ToString('o');Build=@{DllSha256=$build.sha256;CoreSha256=$coreHash};DurationSeconds=300;LogicalProcessors=24;Samples=@()}
foreach($i in 0..10){
 $rows=foreach($name in @('explorer','Maxwell.Shell.Core','windhawk')){[pscustomobject]@{Name=$name;Pid=@{explorer=1;'Maxwell.Shell.Core'=2;windhawk=3}[$name];CpuSeconds=$i;PrivateMB=100+10*[int]($i -gt 2);WorkingSetMB=100;Handles=100+($i%2);Gdi=5;User=5}}
 $valid.Samples+=@{ElapsedSeconds=$i*30;Processes=@($rows)}
}
function Run-Gate($value){$path=Join-Path $scratch 'fixture.json';$value|ConvertTo-Json -Depth 10|Set-Content -LiteralPath $path;& (Join-Path $root 'Test-OpalResourceSoak.ps1') -ReceiptPath $path}
if(-not (Run-Gate $valid).passed){throw 'Stable/jitter control failed.'}
$cases=@(
 {param($r)for($i=0;$i -lt $r.Samples.Count;$i++){$r.Samples[$i].Processes[0].Handles=100+$i}},
 {param($r)$v=@(100,110,120,130,140,139,160,170,180,190,200);for($i=0;$i -lt $v.Count;$i++){$r.Samples[$i].Processes[0].Handles=$v[$i]}},
 {param($r)$r.Samples[5].Processes[0].Pid=9},
 {param($r)$r.Samples[5].Processes[0].Gdi=$null},
 {param($r)$r.Samples[5].ElapsedSeconds=$r.Samples[4].ElapsedSeconds},
 {param($r)$r.DurationSeconds=1},
 {param($r)$r.Build.DllSha256='bad'}
)
foreach($change in $cases){$copy=$valid|ConvertTo-Json -Depth 10|ConvertFrom-Json;& $change $copy;$rejected=$false;try{$null=Run-Gate $copy}catch{$rejected=$true};if(-not $rejected){throw 'Invalid or growing soak accepted.'}}
[pscustomobject]@{passed=$true;cases=8;evidence='Stable/jitter control; growth with occasional releases, process replacement, unavailable counters, duplicate time, short duration and wrong build rejection.'}
