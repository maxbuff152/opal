[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$scratch=Join-Path $root 'build\performance-evidence-test'
New-Item -ItemType Directory -Path $scratch -Force | Out-Null
$binary=Join-Path $scratch 'fixture.bin';[IO.File]::WriteAllText($binary,'synthetic unit-test artifact')
$hash=(Get-FileHash -LiteralPath $binary).Hash
$buildPath=Join-Path $scratch 'build.json'
@{version='test';sha256=$hash;output=$binary;sourceInputs=@(@{path=$binary;sha256=$hash})} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $buildPath
$all=foreach($round in 1..2){
 $order=if($round -eq 1){'ABBA'}else{'BAAB'}
 foreach($pos in 1..4){
  $name=if($order[$pos-1] -eq 'A'){'Stock'}else{'FullSuite'}
  $full=$name -eq 'FullSuite';$cpu=if($full){1.1}else{1.0}
  [pscustomobject]@{Round=$round;Order=$order;Position=$pos;Scenario=$name;RuntimeVerified=$true;Pid=100+$round*4+$pos;ElapsedSeconds=15.0;CpuSeconds=$cpu*0.15;CpuPercentOneCore=$cpu;CpuPercentMachine=$cpu/24;PrivateMB=100+10*[int]$full;WorkingSetMB=100+8*[int]$full;Handles=100;Gdi=10;User=10;Threads=5;HandleDelta=0;GdiDelta=0;UserDelta=0}
 }
}
$valid=@{SchemaVersion=2;Build=@{Version='test';DllSha256=$hash;CoreSha256=$hash};CapturedAt=[DateTimeOffset]::Now.ToString('o');SampleSeconds=15;SettleSeconds=45;LogicalProcessors=24;AbbaRounds=2;TestDesign='Randomized paired ABBA/BAAB';Scenarios=@()}
foreach($name in @('Stock','FullSuite')){
 $samples=@($all|Where-Object Scenario -eq $name)
 $valid.Scenarios+=@{Scenario=$name;Samples=$samples;CpuPercentOneCore=$samples[0].CpuPercentOneCore;PrivateMB=$samples[0].PrivateMB;WorkingSetMB=$samples[0].WorkingSetMB}
}
$path=Join-Path $scratch 'receipt.json'
function Run-Gate($Receipt){
 $Receipt|ConvertTo-Json -Depth 12|Set-Content -LiteralPath $path
 & (Join-Path $root 'Test-OpalPerformanceBudget.ps1') -ReceiptPath $path -ExpectedBuildReceiptPath $buildPath -ExpectedCorePath $binary
}
if(-not (Run-Gate $valid).passed){throw 'Valid synthetic control rejected.'}
$cases=@(
 @{name='missing schema';edit={param($r)$r.SchemaVersion=1}},
 @{name='missing metric';edit={param($r)$r.Scenarios[0].Samples[0].PrivateMB=$null}},
 @{name='nonfinite metric';edit={param($r)$r.Scenarios[0].Samples[0].PrivateMB='NaN'}},
 @{name='wrong hash';edit={param($r)$r.Build.DllSha256='0'*64}},
 @{name='wrong version';edit={param($r)$r.Build.Version='old'}},
 @{name='stale receipt';edit={param($r)$r.CapturedAt=[DateTimeOffset]::Now.AddDays(-2).ToString('o')}},
 @{name='empty samples';edit={param($r)$r.Scenarios[0].Samples=@()}},
 @{name='duplicate scenario';edit={param($r)$r.Scenarios[1].Scenario='Stock'}},
 @{name='negative memory';edit={param($r)$r.Scenarios[0].Samples[0].PrivateMB=-1}},
 @{name='false summary';edit={param($r)$r.Scenarios[1].PrivateMB=0}},
 @{name='wrong CPU arithmetic';edit={param($r)$r.Scenarios[0].Samples[0].CpuSeconds=100}},
 @{name='unverified runtime';edit={param($r)$r.Scenarios[0].Samples[0].RuntimeVerified=$false}},
 @{name='string runtime flag';edit={param($r)$r.Scenarios[0].Samples[0].RuntimeVerified='true'}},
 @{name='short settle';edit={param($r)$r.SettleSeconds=1}},
 @{name='duplicate paired position';edit={param($r)$r.Scenarios[0].Samples[1].Position=$r.Scenarios[0].Samples[0].Position}}
)
foreach($case in $cases){
 $copy=$valid|ConvertTo-Json -Depth 12|ConvertFrom-Json
 & $case.edit $copy
 $rejected=$false
 try{$null=Run-Gate $copy}catch{$rejected=$true}
 if(-not $rejected){throw "Invalid evidence accepted: $($case.name)"}
}
[pscustomobject]@{passed=$true;cases=1+$cases.Count;evidence='Synthetic valid control plus malformed, stale, unrelated, duplicate, short and numerically inconsistent receipts.'}
