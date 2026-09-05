<#
.SYNOPSIS
    Enforces Opal's controlled stock-delta release budget.

.DESCRIPTION
    Reads a Measure-OpalPackageCost receipt and rejects a release that exceeds
    the bounded idle CPU, private-commit, or working-set gates.
    Object growth is checked separately by Test-OpalResourceSoak.ps1.
    Negative deltas are treated as measurement noise, never as negative cost.
#>
[CmdletBinding()]
param(
    [string] $ReceiptPath,
    [string] $ExpectedBuildReceiptPath = (Join-Path $PSScriptRoot 'build\opal-suite\build-receipt.json'),
    [string] $ExpectedCorePath = (Join-Path $PSScriptRoot 'build\maxwell-shell-core\Maxwell.Shell.Core.exe'),
    [double] $MaximumCpuDeltaOneCore = 0.25,
    [double] $MaximumPrivateDeltaMB = 20.0,
    [double] $MaximumWorkingSetDeltaMB = 16.0
)

$ErrorActionPreference = 'Stop'
if (-not $ReceiptPath) {
    $ReceiptPath = Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'measurements') `
        -File -Filter 'package-cost-*.json' | Sort-Object LastWriteTime -Descending |
        Select-Object -First 1 -ExpandProperty FullName
}
if (-not $ReceiptPath -or -not (Test-Path -LiteralPath $ReceiptPath -PathType Leaf)) {
    throw 'No Opal package-cost receipt was found.'
}

$receipt = Get-Content -LiteralPath $ReceiptPath -Raw | ConvertFrom-Json
function Require-Number($Object, [string]$Name, [double]$Minimum = 0) {
    $value = $Object.PSObject.Properties[$Name].Value
    if ($null -eq $value -or $value -is [string] -or $value -is [bool]) { throw "Missing/non-numeric $Name" }
    $number = [double]$value
    if ([double]::IsNaN($number) -or [double]::IsInfinity($number) -or $number -lt $Minimum) { throw "Invalid $Name" }
    return $number
}
function Get-Median([double[]]$Values) {
    $sorted = @($Values | Sort-Object)
    $middle = [int][math]::Floor($sorted.Count / 2)
    if ($sorted.Count % 2) { return $sorted[$middle] }
    return ($sorted[$middle-1] + $sorted[$middle]) / 2
}
if ($receipt.SchemaVersion -ne 2) { throw 'Current release acceptance requires schema 2; historical receipts are not release evidence.' }
$build = Get-Content -LiteralPath $ExpectedBuildReceiptPath -Raw | ConvertFrom-Json
$coreHash = (Get-FileHash -LiteralPath $ExpectedCorePath).Hash
if ($receipt.Build.Version -ne $build.version -or $receipt.Build.DllSha256 -ne $build.sha256 -or
    $receipt.Build.CoreSha256 -ne $coreHash -or $build.sha256 -notmatch '^[A-Fa-f0-9]{64}$') { throw 'Receipt belongs to a different build.' }
if ((Get-FileHash -LiteralPath $build.output).Hash -ne $build.sha256) { throw 'Expected build output changed.' }
foreach ($source in $build.sourceInputs) {
    if ((Get-FileHash -LiteralPath $source.path).Hash -ne $source.sha256) { throw 'Source changed since expected build.' }
}
$age = ([DateTimeOffset]::Now - [DateTimeOffset]::Parse($receipt.CapturedAt)).TotalHours
if ($age -lt -0.02 -or $age -gt 24) { throw 'Receipt is stale or future-dated.' }
$sampleSeconds = Require-Number $receipt SampleSeconds 15
$null = Require-Number $receipt SettleSeconds 45
$rounds = Require-Number $receipt AbbaRounds 2
if ($rounds -ne [math]::Floor($rounds) -or $receipt.TestDesign -ne 'Randomized paired ABBA/BAAB') { throw 'Paired ABBA/BAAB evidence required.' }
$cpuCount = Require-Number $receipt LogicalProcessors 1
if (@($receipt.Scenarios).Count -ne 2) { throw 'Exactly Stock and FullSuite scenarios required.' }
$slots = @{}
foreach ($name in @('Stock','FullSuite')) {
    $matches = @($receipt.Scenarios | Where-Object Scenario -eq $name)
    if ($matches.Count -ne 1) { throw "Duplicate/missing $name scenario." }
    $scenario = $matches[0]
    $samples = @($scenario.Samples)
    if ($samples.Count -ne 2*$rounds) { throw "Insufficient or extra $name samples." }
    foreach ($sample in $samples) {
        if ($sample.Scenario -ne $name -or $sample.RuntimeVerified -isnot [bool] -or -not $sample.RuntimeVerified) { throw 'Unverified sample runtime.' }
        foreach ($metric in @('CpuSeconds','CpuPercentOneCore','CpuPercentMachine','PrivateMB','WorkingSetMB','Handles','Gdi','User','Threads','Pid')) { $null = Require-Number $sample $metric }
        foreach ($metric in @('HandleDelta','GdiDelta','UserDelta')) { $null = Require-Number $sample $metric ([double]::MinValue) }
        $elapsed = Require-Number $sample ElapsedSeconds $sampleSeconds
        if ([math]::Abs($sample.CpuPercentOneCore-$sample.CpuSeconds/$elapsed*100) -gt 0.01 -or
            [math]::Abs($sample.CpuPercentMachine-$sample.CpuPercentOneCore/$cpuCount) -gt 0.01) { throw 'Inconsistent CPU sample.' }
        $round = Require-Number $sample Round 1
        $position = Require-Number $sample Position 1
        if ($round -gt $rounds -or $position -gt 4 -or $round -ne [math]::Floor($round) -or $position -ne [math]::Floor($position)) { throw 'Invalid paired position.' }
        if ($sample.Order -notin @('ABBA','BAAB')) { throw 'Invalid paired order.' }
        $expected = if ($sample.Order[[int]$position-1] -eq 'A') {'Stock'} else {'FullSuite'}
        $key = "$round/$position"
        if ($expected -ne $name -or $slots.ContainsKey($key)) { throw 'Duplicate/mislabelled paired position.' }
        $slots[$key] = $sample.Order
    }
    foreach ($metric in @('CpuPercentOneCore','PrivateMB','WorkingSetMB')) {
        $declared = Require-Number $scenario $metric
        $computed = Get-Median ([double[]]$samples.$metric)
        if ([math]::Abs($computed-$declared) -gt 0.011) { throw "Incorrect $name $metric summary." }
    }
}
for ($round=1; $round -le $rounds; $round++) {
    if (@(1..4 | ForEach-Object {$slots["$round/$_"]} | Select-Object -Unique).Count -ne 1) { throw 'Inconsistent order within paired round.' }
}
$stock = @($receipt.Scenarios | Where-Object Scenario -eq 'Stock') | Select-Object -First 1
$full = @($receipt.Scenarios | Where-Object Scenario -eq 'FullSuite') | Select-Object -First 1
if (-not $stock -or -not $full) { throw 'Receipt must contain Stock and FullSuite scenarios.' }

$cpuDelta = [math]::Round([double]$full.CpuPercentOneCore - [double]$stock.CpuPercentOneCore, 3)
$privateDelta = [math]::Round([double]$full.PrivateMB - [double]$stock.PrivateMB, 2)
$workingDelta = [math]::Round([double]$full.WorkingSetMB - [double]$stock.WorkingSetMB, 2)
$growth = @($full.Samples | Where-Object {
    [double]$_.HandleDelta -gt 0 -or [double]$_.GdiDelta -gt 0 -or [double]$_.UserDelta -gt 0
})

$checks = [ordered]@{
    cpu = $cpuDelta -le $MaximumCpuDeltaOneCore
    privateCommit = $privateDelta -le $MaximumPrivateDeltaMB
    workingSet = $workingDelta -le $MaximumWorkingSetDeltaMB
}
$result = [pscustomobject]@{
    passed = @($checks.Values | Where-Object { -not $_ }).Count -eq 0
    receipt = (Resolve-Path -LiteralPath $ReceiptPath).Path
    build = $receipt.Build
    limitation = 'Paired overhead gate only. Positive short-window object changes are diagnostic, not a leak verdict; require a same-process resource soak.'
    delta = [pscustomobject]@{
        cpuPercentOneCore = $cpuDelta
        privateMB = $privateDelta
        workingSetMB = $workingDelta
        positiveGrowthSamples = $growth.Count
    }
    budget = [pscustomobject]@{
        cpuPercentOneCore = $MaximumCpuDeltaOneCore
        privateMB = $MaximumPrivateDeltaMB
        workingSetMB = $MaximumWorkingSetDeltaMB
    }
    checks = [pscustomobject]$checks
}
$result
if (-not $result.passed) {
    throw "Opal performance budget failed: $($result.delta | ConvertTo-Json -Compress)"
}
