<#
.SYNOPSIS
Describes paired Explorer benchmark differences without making an acceptance decision.
.DESCRIPTION
Recomputes each round's mean FullSuite minus mean Stock from measured samples.
Historical receipts can be analyzed: this does not certify freshness, runtime identity,
or release budgets. Pooled medians are reported separately for comparison only.
#>
[CmdletBinding()]
param([Parameter(Mandatory)][string]$ReceiptPath)

$ErrorActionPreference = 'Stop'
$receipt = Get-Content -LiteralPath $ReceiptPath -Raw | ConvertFrom-Json
$metrics = @('CpuPercentOneCore', 'PrivateMB', 'WorkingSetMB')

function Read-Number($Object, [string]$Name, [double]$Minimum = 0) {
    $value = $Object.PSObject.Properties[$Name].Value
    if ($null -eq $value -or $value -is [string] -or $value -is [bool] -or
        $value -isnot [ValueType]) { throw "Missing/non-numeric $Name." }
    $number = [double]$value
    if ([double]::IsNaN($number) -or [double]::IsInfinity($number) -or $number -lt $Minimum) {
        throw "Invalid $Name."
    }
    return $number
}

function Get-Median([double[]]$Values) {
    $sorted = @($Values | Sort-Object)
    $middle = [int][math]::Floor($sorted.Count / 2)
    if ($sorted.Count % 2) { return $sorted[$middle] }
    return ($sorted[$middle - 1] + $sorted[$middle]) / 2
}

if ($receipt.TestDesign -ne 'Randomized paired ABBA/BAAB') { throw 'Paired ABBA/BAAB design required.' }
$roundCount = Read-Number $receipt AbbaRounds 1
if ($roundCount -ne [math]::Floor($roundCount)) { throw 'Round count must be an integer.' }
if (@($receipt.Scenarios).Count -ne 2) { throw 'Exactly Stock and FullSuite scenarios required.' }
$samplesByScenario = @{}
$slots = @{}
foreach ($name in @('Stock', 'FullSuite')) {
    $scenario = @($receipt.Scenarios | Where-Object Scenario -eq $name)
    if ($scenario.Count -ne 1) { throw "Missing/duplicate $name scenario." }
    $samples = @($scenario[0].Samples)
    if ($samples.Count -ne 2 * $roundCount) { throw "Incomplete/extra $name samples." }
    $samplesByScenario[$name] = $samples
    foreach ($sample in $samples) {
        if ($sample.Scenario -ne $name) { throw 'Sample scenario disagrees with its container.' }
        $round = Read-Number $sample Round 1
        $position = Read-Number $sample Position 1
        if ($round -ne [math]::Floor($round) -or $round -gt $roundCount -or
            $position -ne [math]::Floor($position) -or $position -gt 4) { throw 'Invalid paired slot.' }
        if ($sample.Order -notin @('ABBA', 'BAAB')) { throw 'Invalid paired order.' }
        $expected = if ($sample.Order[[int]$position - 1] -eq 'A') { 'Stock' } else { 'FullSuite' }
        $key = "$round/$position"
        if ($name -ne $expected -or $slots.ContainsKey($key)) { throw 'Duplicate/mislabelled paired slot.' }
        foreach ($metric in $metrics) { $null = Read-Number $sample $metric }
        $slots[$key] = $sample
    }
}

$rounds = @(for ($round = 1; $round -le $roundCount; $round++) {
    $samples = @(1..4 | ForEach-Object { $slots["$round/$_"] })
    if (@($samples | Where-Object { $null -eq $_ }).Count -or
        @($samples.Order | Select-Object -Unique).Count -ne 1) { throw 'Incomplete/inconsistent paired round.' }
    $stock = @($samples | Where-Object Scenario -eq 'Stock')
    $full = @($samples | Where-Object Scenario -eq 'FullSuite')
    $stockMean = [ordered]@{}
    $fullMean = [ordered]@{}
    $delta = [ordered]@{}
    foreach ($metric in $metrics) {
        $a = ($stock[0].$metric + $stock[1].$metric) / 2.0
        $b = ($full[0].$metric + $full[1].$metric) / 2.0
        $stockMean[$metric] = $a
        $fullMean[$metric] = $b
        $delta[$metric] = $b - $a
    }
    [pscustomobject]@{ Round = $round; Order = $samples[0].Order; StockMean = [pscustomobject]$stockMean; FullSuiteMean = [pscustomobject]$fullMean; Delta = [pscustomobject]$delta }
})

$pairedSummary = [ordered]@{}
$pooledMedians = [ordered]@{}
foreach ($metric in $metrics) {
    $deltas = [double[]]@($rounds | ForEach-Object { $_.Delta.$metric })
    $bounds = $deltas | Measure-Object -Minimum -Maximum
    $pairedSummary[$metric] = [pscustomobject]@{
        MedianRoundDelta = Get-Median $deltas
        MinimumRoundDelta = $bounds.Minimum
        MaximumRoundDelta = $bounds.Maximum
        Range = $bounds.Maximum - $bounds.Minimum
    }
    $stock = Get-Median ([double[]]$samplesByScenario.Stock.$metric)
    $full = Get-Median ([double[]]$samplesByScenario.FullSuite.$metric)
    $pooledMedians[$metric] = [pscustomobject]@{ Stock = $stock; FullSuite = $full; Delta = $full - $stock }
}

[pscustomobject]@{
    Receipt = (Resolve-Path -LiteralPath $ReceiptPath).Path
    Build = $receipt.Build
    CapturedAt = $receipt.CapturedAt
    Scope = 'Explorer only; descriptive historical analysis, not release acceptance.'
    DeltaConvention = 'FullSuite minus Stock'
    RoundCount = $roundCount
    Rounds = $rounds
    PairedSummary = [pscustomobject]$pairedSummary
    PooledScenarioMedians = [pscustomobject]$pooledMedians
    Limitation = 'Round min/max describe observed spread, not confidence intervals. Runtime identity, freshness and budgets remain the responsibility of the acceptance gate.'
}
