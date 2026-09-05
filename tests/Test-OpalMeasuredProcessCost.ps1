[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
. (Join-Path (Split-Path -Parent $PSScriptRoot) 'Get-OpalMeasuredProcessCost.ps1')
$before = @(
    [pscustomobject]@{Name='explorer';Pid=1;StartedAtUtc='start-1';CpuSeconds=10.0;PrivateMB=300.0;WorkingSetMB=350.0},
    [pscustomobject]@{Name='Maxwell.Shell.Core';Pid=2;StartedAtUtc='start-1';CpuSeconds=1.0;PrivateMB=8.0;WorkingSetMB=12.0}
)
$after = $before | ConvertTo-Json | ConvertFrom-Json
$after[0].CpuSeconds = 10.25
$after[1].CpuSeconds = 1.125
$result = Compare-OpalMeasuredProcessSnapshot -Before $before -After $after -ElapsedSeconds 15
if ($result.CpuPercentOneCore -ne 2.5 -or $result.PrivateMB -ne 308 -or $result.PSObject.Properties.Name -contains 'WorkingSetMB') {
    throw 'Incorrect aggregate or shared working sets were summed.'
}
$cases = @(
    @{Edit={param($a)$a[1].Pid=3}},
    @{Edit={param($a)$a[1].StartedAtUtc='start-2'}},
    @{Edit={param($a)$a[1].Pid=1}},
    @{Edit={param($a)$a[1].CpuSeconds=0.5}},
    @{Edit={param($a)$a[1].PrivateMB=$null}},
    @{Edit={param($a)$a[1].PrivateMB=[double]::NaN}},
    @{Edit={param($a)$a[1].CpuSeconds='1.125'}}
)
foreach ($case in $cases) {
    $copy=$after | ConvertTo-Json | ConvertFrom-Json
    & $case.Edit $copy
    $rejected=$false
    try { $null=Compare-OpalMeasuredProcessSnapshot -Before $before -After $copy -ElapsedSeconds 15 } catch { $rejected=$true }
    if (-not $rejected) { throw 'Invalid process-set sample accepted.' }
}
foreach ($duration in @(0, [double]::NaN)) {
    $rejected=$false
    try { $null=Compare-OpalMeasuredProcessSnapshot -Before $before -After $after -ElapsedSeconds $duration } catch { $rejected=$true }
    if (-not $rejected) { throw 'Invalid duration accepted.' }
}
$rejected=$false
try { $null=Compare-OpalMeasuredProcessSnapshot -Before $before -After @($after[0]) -ElapsedSeconds 15 } catch { $rejected=$true }
if (-not $rejected) { throw 'Process exit accepted.' }
[pscustomobject]@{passed=$true;cases=11;evidence='Correct CPU/private aggregation; per-process working sets; rejects membership/PID reuse/duplicate/counter/duration errors.'}
