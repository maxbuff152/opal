<#
.SYNOPSIS
    Enforces Opal's controlled stock-delta release budget.

.DESCRIPTION
    Reads a Measure-OpalPackageCost receipt and rejects a release that exceeds
    the bounded idle CPU, private-commit, working-set, or object-growth gates.
    Negative deltas are treated as measurement noise, never as negative cost.
#>
[CmdletBinding()]
param(
    [string] $ReceiptPath,
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
    noObjectGrowth = $growth.Count -eq 0
}
$result = [pscustomobject]@{
    passed = @($checks.Values | Where-Object { -not $_ }).Count -eq 0
    receipt = (Resolve-Path -LiteralPath $ReceiptPath).Path
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
        positiveGrowthSamples = 0
    }
    checks = [pscustomobject]$checks
}
$result
if (-not $result.passed) {
    throw "Opal performance budget failed: $($result.delta | ConvertTo-Json -Compress)"
}
