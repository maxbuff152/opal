[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$scratch = Join-Path $root 'build\paired-analysis-test'
New-Item -ItemType Directory -Path $scratch -Force | Out-Null
$path = Join-Path $scratch 'fixture.json'
$all = @(foreach ($round in 1..2) {
    $order = if ($round -eq 1) { 'ABBA' } else { 'BAAB' }
    $values = if ($round -eq 1) { @(300, 330, 330, 400) } else { @(318, 300, 320, 322) }
    foreach ($position in 1..4) {
        $name = if ($order[$position - 1] -eq 'A') { 'Stock' } else { 'FullSuite' }
        [pscustomobject]@{ Round = $round; Position = $position; Order = $order; Scenario = $name; CpuPercentOneCore = $values[$position - 1] / 1000.0; PrivateMB = $values[$position - 1]; WorkingSetMB = $values[$position - 1] + 100 }
    }
})
$valid = @{ TestDesign = 'Randomized paired ABBA/BAAB'; AbbaRounds = 2; Scenarios = @(
    @{ Scenario = 'Stock'; Samples = @($all | Where-Object Scenario -eq 'Stock') },
    @{ Scenario = 'FullSuite'; Samples = @($all | Where-Object Scenario -eq 'FullSuite') }
) }
function Run-Analysis($Receipt) {
    $Receipt | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $path
    & (Join-Path $root 'Get-OpalPairedAnalysis.ps1') -ReceiptPath $path
}
function Assert-Near($Actual, $Expected) {
    if ([math]::Abs($Actual - $Expected) -gt 0.0000001) { throw "Expected $Expected; received $Actual." }
}
$result = Run-Analysis $valid
Assert-Near $result.Rounds[0].Delta.PrivateMB -20
Assert-Near $result.Rounds[1].Delta.PrivateMB 10
Assert-Near $result.PairedSummary.PrivateMB.MedianRoundDelta -5
Assert-Near $result.PairedSummary.PrivateMB.MinimumRoundDelta -20
Assert-Near $result.PairedSummary.PrivateMB.MaximumRoundDelta 10
Assert-Near $result.PairedSummary.PrivateMB.Range 30
Assert-Near $result.PooledScenarioMedians.PrivateMB.Delta 16
Assert-Near $result.PairedSummary.CpuPercentOneCore.MedianRoundDelta -0.005
Assert-Near $result.PairedSummary.WorkingSetMB.MedianRoundDelta -5
if ($result.PSObject.Properties['passed']) { throw 'Descriptive analysis must not imply acceptance.' }

$cases = @(
    @{ Name = 'missing metric'; Edit = { param($r) $r.Scenarios[0].Samples[0].PSObject.Properties.Remove('PrivateMB') } },
    @{ Name = 'nonfinite metric'; Edit = { param($r) $r.Scenarios[0].Samples[0].PrivateMB = [double]::NaN } },
    @{ Name = 'numeric string'; Edit = { param($r) $r.Scenarios[0].Samples[0].PrivateMB = '300' } },
    @{ Name = 'boolean metric'; Edit = { param($r) $r.Scenarios[0].Samples[0].PrivateMB = $true } },
    @{ Name = 'negative metric'; Edit = { param($r) $r.Scenarios[0].Samples[0].PrivateMB = -1 } },
    @{ Name = 'duplicate slot'; Edit = { param($r) $r.Scenarios[0].Samples[1].Position = 1 } },
    @{ Name = 'missing sample'; Edit = { param($r) $r.Scenarios[0].Samples = @($r.Scenarios[0].Samples | Select-Object -Skip 1) } },
    @{ Name = 'fractional round'; Edit = { param($r) $r.Scenarios[0].Samples[0].Round = 1.5 } },
    @{ Name = 'wrong order'; Edit = { param($r) $r.Scenarios[0].Samples[0].Order = 'BAAB' } },
    @{ Name = 'wrong scenario'; Edit = { param($r) $r.Scenarios[0].Samples[0].Scenario = 'FullSuite' } },
    @{ Name = 'duplicate scenario'; Edit = { param($r) $r.Scenarios[1].Scenario = 'Stock' } },
    @{ Name = 'missing round'; Edit = { param($r) $r.AbbaRounds = 3 } },
    @{ Name = 'nonpaired design'; Edit = { param($r) $r.TestDesign = 'Scenario medians' } }
)
foreach ($case in $cases) {
    $copy = $valid | ConvertTo-Json -Depth 12 | ConvertFrom-Json
    & $case.Edit $copy
    $rejected = $false
    try { $null = Run-Analysis $copy } catch { $rejected = $true }
    if (-not $rejected) { throw "Invalid evidence accepted: $($case.Name)." }
}
[pscustomobject]@{ passed = $true; cases = 1 + $cases.Count; evidence = 'Known ABBA/BAAB means, round median/range and distinct pooled median; malformed metric and paired-slot rejection.' }
