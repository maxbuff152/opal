[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$analyzer = Join-Path $root 'Get-OpalLatencyAnalysis.ps1'
$labels = @(
    'Shell successful attachment/recovery pass (excluding backoff)'
    'Media command dispatch'
    'Media command to next update (not acknowledgment)'
    'Media snapshot to UI'
    'Media UI work'
    'Hardware panel to Opened (not pixel-present)'
    'Hardware panel refresh'
)
$header = 'Opal latency diagnostics (milliseconds; latest 64 samples per metric)'
function Fixture([int]$Count=32) {
    $p95 = if ($Count -lt 20) { '8.000' } else { '5.000' }
    @('Maxwell hardware snapshot','CPU         5%', $header) +
        @($labels | ForEach-Object { $_ + ": n=$Count total=100 median=2.000 p95=$p95 max=8.000" }) -join "`r`n"
}
$valid = Fixture
$script:checks = 0
function Check([bool]$Pass,[string]$Label) {
    ++$script:checks
    if (-not $Pass) { throw "Latency evidence assertion failed: $Label" }
}
function Analyze([string]$Text,[hashtable]$Budgets=@{MediaCommandDispatch=10;HardwareOpen=10},[int]$Minimum=20) {
    & $analyzer -ExportText $Text -P95Budgets $Budgets -MinimumSamples $Minimum
}
function Invalid([string]$Text,[string]$Label) {
    $r = Analyze $Text
    Check (-not $r.EvidenceValid -and $r.Verdict -eq 'InvalidEvidence' -and $null -eq $r.Passed) $Label
}
# Read the current C++ producer's literals. A label-only producer change must
# fail this contract check even when the independently authored fixtures below
# still happen to match the parser. This is synthetic text, not a live UI export.
$producer = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\opal-performance-diagnostics.h'))
$nameBlock = [regex]::Match($producer,'(?s)constexpr\s+const\s+wchar_t\*\s+names\[\]\s*=\s*\{(?<names>.*?)\};')
$sectionLiteral = [regex]::Match($producer,'std::wstring\s+result\s*=\s*L"(?<value>(?:\\.|[^"\\])*)";')
$rowLiteral = [regex]::Match($producer,'swprintf_s\(row,\s*L"(?<value>(?:\\.|[^"\\])*)"')
$emptyLiteral = [regex]::Match($producer,'if\s*\(!s.count\)\s*\{\s*result\s*\+=\s*L"(?<value>(?:\\.|[^"\\])*)";')
Check ($nameBlock.Success -and $sectionLiteral.Success -and $rowLiteral.Success -and $emptyLiteral.Success) 'producer export contract boundaries found'
$producerLabels = @([regex]::Matches($nameBlock.Groups['names'].Value,'L"(?<value>(?:\\.|[^"\\])*)"') | ForEach-Object { [regex]::Unescape($_.Groups['value'].Value) })
$producerHeader = [regex]::Unescape($sectionLiteral.Groups['value'].Value)
$producerRow = [regex]::Unescape($rowLiteral.Groups['value'].Value).Replace('%zu','32').Replace('%llu','100').Replace('%.3f','5.000')
$producerFixture = $producerHeader + (($producerLabels | ForEach-Object { $_ + $producerRow }) -join '')
$r = Analyze $producerFixture
Check ($r.EvidenceValid -and $r.Verdict -eq 'Passed' -and $r.Metrics.Count -eq $producerLabels.Count -and @($r.Metrics | Where-Object { $_.Count -ne 32 -or $_.P95Ms -ne 5 }).Count -eq 0) 'actual producer labels/header/numeric row format accepted by parser'
$producerEmptyRow = [regex]::Unescape($emptyLiteral.Groups['value'].Value)
$r = Analyze ($producerHeader + (($producerLabels | ForEach-Object { $_ + $producerEmptyRow }) -join ''))
Check ($r.EvidenceValid -and $r.Verdict -eq 'InsufficientEvidence' -and $null -eq $r.Passed) 'actual producer empty-row format stays unavailable'
$r = Analyze $valid
Check ($r.EvidenceValid -and $r.Verdict -eq 'Passed' -and $r.Passed -eq $true -and $r.Metrics.Count -eq 7) 'complete high-count statistics meet explicit budgets'
Check ($r.Metrics[1].Count -eq 32 -and $r.Metrics[1].TotalCount -eq 100 -and $r.Metrics[1].P95Ms -eq 5) 'parsed measured statistics preserved'
Check (-not $r.RuntimeVerified -and $r.Limitations.Count -ge 6 -and $r.Metrics[2].Label.Contains('not acknowledgment') -and $r.Metrics[5].Label.Contains('not pixel-present')) 'measurement limits remain explicit'
$r = Analyze $valid @{}
Check ($r.Verdict -eq 'NotEvaluated' -and $null -eq $r.Passed -and $r.RequestedBudgetCount -eq 0) 'no invented budgets or acceptance'
$r = Analyze $valid @{HardwareOpen=4}
Check ($r.Verdict -eq 'Failed' -and $r.Passed -eq $false -and $r.Metrics[5].Status -eq 'OverBudget') 'explicit budget failure'
$r = Analyze (Fixture 4)
Check ($r.EvidenceValid -and $r.Verdict -eq 'InsufficientEvidence' -and $null -eq $r.Passed) 'low n cannot pass despite larger total count'
$r = Analyze (Fixture 4) @{HardwareOpen=10} 4
Check ($r.Verdict -eq 'Passed') 'caller can configure minimum samples'
$none = $header + "`n" + (($labels | ForEach-Object { $_ + ': no samples' }) -join "`n")
$r = Analyze $none
Check ($r.EvidenceValid -and $r.Verdict -eq 'InsufficientEvidence' -and $null -eq $r.Passed -and $null -eq $r.Metrics[0].P95Ms) 'no samples stay unavailable, never zero latency'
$partial = $valid.Replace($labels[5]+': n=32 total=100 median=2.000 p95=5.000 max=8.000',$labels[5]+': no samples')
$r = Analyze $partial
Check ($r.Verdict -eq 'InsufficientEvidence' -and $r.Metrics[1].Status -eq 'WithinBudget' -and $r.Metrics[5].Status -eq 'NoSamples') 'one measured interaction cannot stand in for a missing budgeted interaction'
$r = Analyze $partial @{MediaCommandDispatch=10}
Check ($r.Verdict -eq 'Passed' -and $r.RequestedBudgetCount -eq 1) 'caller-scoped verdict does not imply acceptance of unbudgeted metrics'
Invalid '' 'empty input'
Invalid 'Maxwell hardware snapshot' 'missing diagnostics section'
Invalid ($valid.Replace($labels[6]+': n=32 total=100 median=2.000 p95=5.000 max=8.000','')) 'missing row'
Invalid ($valid+"`n"+$labels[0]+': no samples') 'duplicate metric'
Invalid ($valid+"`n"+$header) 'duplicate section header'
Invalid ($valid.Replace('p95=5.000','p95=NaN')) 'NaN statistics'
Invalid ($valid.Replace('max=8.000','max=Infinity')) 'infinite statistics'
Invalid ($valid.Replace('max=8.000','max=1e9999')) 'overflow statistics'
Invalid ($valid.Replace('median=2.000','median=-1')) 'negative statistics'
Invalid ($valid.Replace('median=2.000','median=6')) 'unordered median and p95'
Invalid ($valid.Replace('max=8.000','max=4')) 'unordered p95 and maximum'
Invalid ($valid.Replace('n=32','n=65')) 'count exceeds ring capacity'
Invalid ($valid.Replace('n=32','n=0')) 'zero-count numerical row is invalid'
Invalid ($valid.Replace('n=32','n=2.5')) 'fractional count'
Invalid ($valid.Replace('n=32','n=4')) 'small-sample p95 must equal maximum under producer quantile formula'
Invalid ((Fixture 1)) 'single-sample median must equal maximum'
$r = Analyze ((Fixture 1).Replace('median=2.000','median=8.000')) @{HardwareOpen=10} 1
Check ($r.Verdict -eq 'Passed') 'consistent single sample only accepted with explicitly lowered minimum'
Invalid ($valid.Replace('total=100','total=31')) 'total below retained count'
Invalid ($valid.Replace('total=100','total=18446744073709551616')) 'overflow total count'
Invalid ($valid.Replace('p95=5.000 ','') ) 'malformed missing field'
Invalid ($valid.Replace('Hardware panel refresh:','Unrecognized metric:')) 'unknown row cannot conceal missing evidence'
$r = Analyze ($valid.Replace('total=100','total=18446744073709551615'))
Check ($r.EvidenceValid -and $r.Metrics[0].TotalCount -eq [uint64]::MaxValue) 'saturated lifetime counter remains valid'
$badBudgets = @(@{HardwareOpen=[double]::NaN},@{HardwareOpen=0},@{HardwareOpen=-1},@{HardwareOpen=$null},@{HardwareOpen=$true},@{TypoMetric=10})
foreach ($budget in $badBudgets) {
    $rejected = $false
    try { $null = Analyze $valid $budget } catch { $rejected = $true }
    Check $rejected 'invalid caller budget rejected'
}
$scratch = Join-Path $root 'build\latency-evidence-test'
New-Item -ItemType Directory -Path $scratch -Force | Out-Null
$path = Join-Path $scratch 'export.txt'
[IO.File]::WriteAllText($path,$valid)
$r = & $analyzer -ReportPath $path -P95Budgets @{HardwareOpen=10}
Check ($r.Verdict -eq 'Passed' -and $r.ReportPath -eq $path) 'literal file input'
[pscustomobject]@{passed=$true;cases=$script:checks;evidence='Actual C++ producer/parser format contract; complete/partial exports, missing/malformed/duplicate rows, nonfinite/unordered values, count bounds, explicit budgets, sample eligibility, scope limitations, and file input. Synthetic exports only.'}
