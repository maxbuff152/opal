<#
.SYNOPSIS
    Validates an exported Opal latency summary and compares caller-supplied p95 budgets.
.DESCRIPTION
    Enable Collect performance timings, perform the interactions to measure, and
    use Hardware Command Center's Copy hardware report. Supply that text or a
    saved copy. This helper never operates the UI or reads the clipboard.

    No acceptance budgets are supplied by default. MinimumSamples defaults to 20
    and applies separately to each metric; at most 64 recent samples are retained.
    P95Budgets is a hashtable keyed by the metric keys in the returned Metrics.
    Only requested budgets participate in the aggregate budget verdict. Every
    diagnostic row must nevertheless be present and structurally valid.

    A Passed verdict means only that eligible, rounded statistics in the supplied
    export meet the caller's budgets. New exports include process/time provenance,
    but no build hash or raw samples, so this helper cannot verify a live build or recompute
    percentiles. It is not a release gate. Media's next update is not provider
    acknowledgment, panel Opened is not pixel presentation, and attachment pass
    timing excludes earlier retry/backoff time.
#>
[CmdletBinding(DefaultParameterSetName='Text')]
param(
    [Parameter(Mandatory,ParameterSetName='Text')]
    [AllowEmptyString()][string]$ExportText,
    [Parameter(Mandatory,ParameterSetName='File')]
    [ValidateNotNullOrEmpty()][string]$ReportPath,
    [hashtable]$P95Budgets = @{},
    [ValidateRange(1,64)][int]$MinimumSamples = 20
)
$ErrorActionPreference = 'Stop'

# These labels, the section header, and row fields are a stable cross-language
# export contract with opal-performance-diagnostics.h, not freely editable UI
# copy. Test-OpalLatencyEvidence.ps1 checks actual producer strings against this
# parser; producer format changes must update the parser and regression together.
$definitions = [ordered]@{
    ShellAttachment = 'Shell successful attachment/recovery pass (excluding backoff)'
    MediaCommandDispatch = 'Media command dispatch'
    MediaCommandNextUpdate = 'Media command to next update (not acknowledgment)'
    MediaSnapshotToUi = 'Media snapshot to UI'
    MediaUiWork = 'Media UI work'
    HardwareOpen = 'Hardware panel to Opened (not pixel-present)'
    HardwareRefresh = 'Hardware panel refresh'
}
$culture = [Globalization.CultureInfo]::InvariantCulture
function Convert-FiniteNonnegative([object]$Value) {
    if ($null -eq $Value -or $Value -is [bool]) { return $null }
    [double]$number = 0
    if (-not [double]::TryParse([Convert]::ToString($Value,$culture),
            [Globalization.NumberStyles]::Float,$culture,[ref]$number) -or
        [double]::IsNaN($number) -or [double]::IsInfinity($number) -or $number -lt 0) {
        return $null
    }
    return $number
}
$budgets = @{}
foreach ($key in $P95Budgets.Keys) {
    if (-not $definitions.Contains([string]$key)) { throw "Unknown latency budget metric: $key" }
    $number = Convert-FiniteNonnegative $P95Budgets[$key]
    if ($null -eq $number -or $number -le 0) { throw "P95 budget must be a finite positive millisecond value: $key" }
    $budgets[[string]$key] = $number
}

$resolvedPath = $null
if ($PSCmdlet.ParameterSetName -eq 'File') {
    $resolvedPath = (Resolve-Path -LiteralPath $ReportPath).ProviderPath
    $ExportText = [IO.File]::ReadAllText($resolvedPath)
}
$header = 'Opal latency diagnostics (milliseconds; latest 64 samples per metric)'
$issues = [Collections.Generic.List[string]]::new()
$parsed = @{}
$provenance = $null
$provenanceCount = 0
$section = $false
$headerCount = 0
foreach ($rawLine in [regex]::Split($ExportText,'\r\n|\n|\r')) {
    $line = $rawLine.Trim()
    if ($line.StartsWith('Opal latency provenance:',[StringComparison]::Ordinal)) {
        ++$provenanceCount
        if ($section -or $provenanceCount -ne 1 -or $line -cnotmatch '^Opal latency provenance: version=1 pid=(?<pid>[0-9]+) processStartFileTime=(?<start>[0-9]+) capturedFileTime=(?<capture>[0-9]+) epoch=(?<epoch>[0-9]+)$') {
            $issues.Add('Malformed, misplaced or duplicate latency provenance.')
            continue
        }
        $tokens = $Matches.Clone()
        try {
            $processIdValue=[uint32]::Parse($tokens['pid'],$culture)
            $epochValue=[uint64]::Parse($tokens['epoch'],$culture)
            $startValue=[int64]::Parse($tokens['start'],$culture)
            $captureValue=[int64]::Parse($tokens['capture'],$culture)
            if (-not $processIdValue -or -not $epochValue -or $startValue -le 0 -or $captureValue -lt $startValue) { throw 'Invalid provenance values.' }
            $provenance=[pscustomobject]@{Version=1;Pid=$processIdValue;ProcessStartUtc=[datetime]::FromFileTimeUtc($startValue).ToString('o');CapturedAtUtc=[datetime]::FromFileTimeUtc($captureValue).ToString('o');Epoch=$epochValue;VerifiedAgainstRuntime=$false}
        } catch { $issues.Add('Invalid process identity or file time in latency provenance.') }
        continue
    }
    if ($line -ceq $header) {
        $section = $true
        ++$headerCount
        continue
    }
    if (-not $section -or -not $line) { continue }
    $key = $null
    foreach ($candidate in $definitions.Keys) {
        if ($line.StartsWith($definitions[$candidate]+':',[StringComparison]::Ordinal)) {
            $key = $candidate
            break
        }
    }
    if ($null -eq $key) { $issues.Add('Unrecognized text inside diagnostic section.'); continue }
    if ($parsed.ContainsKey($key)) { $issues.Add("Duplicate metric: $key"); continue }
    $parsed[$key] = [pscustomobject]@{ Valid=$false; NoSamples=$false; Count=$null; TotalCount=$null; MedianMs=$null; P95Ms=$null; MaxMs=$null }
    $row = $parsed[$key]
    $values = $line.Substring($definitions[$key].Length)
    if ($values -ceq ': no samples') {
        $row.Valid = $true
        $row.NoSamples = $true
        $row.Count = 0
        $row.TotalCount = [uint64]0
        continue
    }
    if ($values -cnotmatch '^: n=(?<count>\S+) total=(?<total>\S+) median=(?<median>\S+) p95=(?<p95>\S+) max=(?<maximum>\S+)$') {
        $issues.Add("Malformed statistics: $key")
        continue
    }
    $tokens = $Matches.Clone()
    [uint64]$count = 0
    [uint64]$total = 0
    if (-not [uint64]::TryParse($tokens['count'],[Globalization.NumberStyles]::None,$culture,[ref]$count) -or
        -not [uint64]::TryParse($tokens['total'],[Globalization.NumberStyles]::None,$culture,[ref]$total) -or
        $count -lt 1 -or $count -gt 64 -or $total -lt $count) {
        $issues.Add("Invalid count or total: $key")
        continue
    }
    $median = Convert-FiniteNonnegative $tokens['median']
    $p95 = Convert-FiniteNonnegative $tokens['p95']
    $maximum = Convert-FiniteNonnegative $tokens['maximum']
    if ($null -eq $median -or $null -eq $p95 -or $null -eq $maximum -or
        $median -gt $p95 -or $p95 -gt $maximum) {
        $issues.Add("Nonfinite, negative, or unordered statistics: $key")
        continue
    }
    # The producer uses nearest-rank p95. Below 20 samples that rank is the
    # maximum; one sample must also have an identical median.
    if (($count -lt 20 -and $p95 -ne $maximum) -or
        ($count -eq 1 -and $median -ne $maximum)) {
        $issues.Add("Statistics are inconsistent with the reported sample count: $key")
        continue
    }
    $row.Valid = $true
    $row.Count = [int]$count
    $row.TotalCount = $total
    $row.MedianMs = $median
    $row.P95Ms = $p95
    $row.MaxMs = $maximum
}
if ($headerCount -ne 1) { $issues.Add('Exactly one complete diagnostic section header is required.') }
foreach ($key in $definitions.Keys) {
    if (-not $parsed.ContainsKey($key)) { $issues.Add("Missing metric: $key") }
}

$rows = foreach ($key in $definitions.Keys) {
    $value = $parsed[$key]
    $requested = $budgets.ContainsKey($key)
    $valid = $null -ne $value -and $value.Valid
    $eligible = $valid -and -not $value.NoSamples -and $value.Count -ge $MinimumSamples
    $status = if (-not $valid) { 'InvalidEvidence' }
              elseif ($value.NoSamples) { 'NoSamples' }
              elseif (-not $eligible) { 'InsufficientSamples' }
              elseif (-not $requested) { 'NotBudgeted' }
              elseif ($value.P95Ms -le $budgets[$key]) { 'WithinBudget' }
              else { 'OverBudget' }
    [pscustomobject]@{
        Key = $key
        Label = $definitions[$key]
        EvidenceValid = $valid
        Count = if ($null -ne $value) { $value.Count } else { $null }
        TotalCount = if ($null -ne $value) { $value.TotalCount } else { $null }
        MedianMs = if ($null -ne $value) { $value.MedianMs } else { $null }
        P95Ms = if ($null -ne $value) { $value.P95Ms } else { $null }
        MaxMs = if ($null -ne $value) { $value.MaxMs } else { $null }
        MinimumSamples = $MinimumSamples
        SampleEligible = $eligible
        BudgetRequested = $requested
        P95BudgetMs = if ($requested) { $budgets[$key] } else { $null }
        Status = $status
    }
}
$evidenceValid = $issues.Count -eq 0
$requestedRows = @($rows | Where-Object BudgetRequested)
$ineligible = @($requestedRows | Where-Object { -not $_.SampleEligible })
$overBudget = @($requestedRows | Where-Object Status -eq 'OverBudget')
$verdict = if (-not $evidenceValid) { 'InvalidEvidence' }
           elseif (-not $requestedRows.Count) { 'NotEvaluated' }
           elseif ($ineligible.Count) { 'InsufficientEvidence' }
           elseif ($overBudget.Count) { 'Failed' }
           else { 'Passed' }
[pscustomobject]@{
    SchemaVersion = 1
    AnalyzedAt = [DateTimeOffset]::Now.ToString('o')
    ReportPath = $resolvedPath
    EvidenceValid = $evidenceValid
    Verdict = $verdict
    Passed = if ($verdict -eq 'Passed') { $true } elseif ($verdict -eq 'Failed') { $false } else { $null }
    MinimumSamples = $MinimumSamples
    RequestedBudgetCount = $requestedRows.Count
    RuntimeVerified = $false
    Provenance = $provenance
    AcceptanceScope = 'Supplied rounded statistics against caller-provided p95 budgets only; no default SLA or release acceptance.'
    Metrics = @($rows)
    Issues = @($issues)
    Limitations = @(
        'The export contains rounded summary values, not raw samples; percentiles cannot be independently recomputed.'
        $(if ($null -eq $provenance) {'Legacy export lacks process/time provenance; current runtime and evidence freshness are unverified.'} else {'Process/time provenance is self-reported, not independently verified against an installed build or interaction log.'})
        'No build hash or input-action provenance is included; this is not a current-build release gate.'
        'Media command-to-next-update is an observation, not provider acknowledgment or confirmed command success.'
        'Hardware panel Opened is not pixel presentation; actual visible-frame latency is unavailable.'
        'Shell successful attachment/recovery pass excludes preceding retry/backoff time.'
        'Only the latest 64 observations per metric contribute to the exported percentiles; total count does not increase recent-sample eligibility.'
    )
}
