<#
.SYNOPSIS
    Cross-platform static validation gate for Opal (runs on Linux, macOS, Windows).

.DESCRIPTION
    Runs every Opal check that does NOT require Windows or Windhawk, in one place:

      1. Assemble the single-file Windhawk mod (Assemble-Opal.ps1).
      2. Source regression: tests\Test-OpalUnifiedControl.ps1 -StaticOnly.
      3. Source regression: tests\Test-MaxwellTaskbarMediaCapsule.ps1 -StaticOnly.
      4. Parse (AST) every .ps1 / .psm1 in the tree.
      5. PSScriptAnalyzer, when the module is available (optional).

    The Windows-only build, install, and live registry/Explorer tests
    (Build-OpalSuite.ps1, Install-OpalSuite.ps1, Test-OpalSuite.ps1, and the
    non-StaticOnly suites) are intentionally excluded. Run those on a Windows
    host with Windhawk installed.

.PARAMETER SkipAnalyzer
    Skip the PSScriptAnalyzer step even when the module is installed.

.PARAMETER AnalyzerSeverity
    Diagnostic severities that make the analyzer step fail. Defaults to
    Error + Warning: the tree is warning-clean under
    PSScriptAnalyzerSettings.psd1, so new warnings block and cannot creep back.
    Pass -AnalyzerSeverity Error to loosen it.
#>
[CmdletBinding()]
param(
    [switch] $SkipAnalyzer,
    [ValidateSet('Error', 'Warning', 'Information')]
    [string[]] $AnalyzerSeverity = @('Error', 'Warning')
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$steps = [System.Collections.Generic.List[object]]::new()

function Add-Result([string] $Name, [bool] $Passed, [string] $Detail) {
    $steps.Add([pscustomobject]@{ Step = $Name; Passed = $Passed; Detail = $Detail })
    $status = if ($Passed) { 'PASS' } else { 'FAIL' }
    Write-Host ("[{0}] {1} - {2}" -f $status, $Name, $Detail)
}

Write-Host '=== Opal cross-platform static gate ===' -ForegroundColor Cyan
Write-Host ("PowerShell {0} on {1}" -f $PSVersionTable.PSVersion, [System.Environment]::OSVersion.Platform)
Write-Host ''

# 1. Assemble the single-file Windhawk mod.
try {
    & (Join-Path $root 'Assemble-Opal.ps1') | Out-Null
    $assembled = Join-Path $root 'dist/Opal/opal.wh.cpp'
    if (Test-Path -LiteralPath $assembled) {
        $kb = [math]::Round((Get-Item -LiteralPath $assembled).Length / 1KB)
        Add-Result 'Assemble-Opal' $true ("produced dist/Opal/opal.wh.cpp ({0} KB)" -f $kb)
    } else {
        Add-Result 'Assemble-Opal' $false 'dist/Opal/opal.wh.cpp was not produced'
    }
} catch {
    Add-Result 'Assemble-Opal' $false $_.Exception.Message
}

# 2 & 3. Source-level static regression suites.
foreach ($suite in @(
        @{ Name = 'Test-OpalUnifiedControl'; Path = 'tests/Test-OpalUnifiedControl.ps1' },
        @{ Name = 'Test-MaxwellTaskbarMediaCapsule'; Path = 'tests/Test-MaxwellTaskbarMediaCapsule.ps1' }
    )) {
    try {
        $res = & (Join-Path $root $suite.Path) -StaticOnly
        $passed = [bool](@($res)[-1].passed)
        Add-Result $suite.Name $passed ($(if ($passed) { 'all source assertions passed' } else { 'assertions failed' }))
    } catch {
        Add-Result $suite.Name $false $_.Exception.Message
    }
}

# 4. AST parse-lint over every PowerShell script.
try {
    $files = Get-ChildItem -LiteralPath $root -Recurse -Include *.ps1, *.psm1 -File
    $bad = 0
    foreach ($f in $files) {
        $tokens = $null; $errors = $null
        [void][System.Management.Automation.Language.Parser]::ParseFile($f.FullName, [ref] $tokens, [ref] $errors)
        if ($errors.Count) {
            $bad++
            Write-Host ("    parse error in {0}: {1}" -f $f.Name, $errors[0].Message)
        }
    }
    Add-Result 'Parse-lint' ($bad -eq 0) ("parsed {0} scripts, {1} with errors" -f $files.Count, $bad)
} catch {
    Add-Result 'Parse-lint' $false $_.Exception.Message
}

# 5. PSScriptAnalyzer (optional; runs only when the module is present).
if ($SkipAnalyzer) {
    Write-Host '[SKIP] PSScriptAnalyzer - skipped by -SkipAnalyzer'
} elseif (-not (Get-Module -ListAvailable -Name PSScriptAnalyzer)) {
    Write-Host '[SKIP] PSScriptAnalyzer - module not installed (Install-Module PSScriptAnalyzer)'
} else {
    try {
        Import-Module PSScriptAnalyzer -ErrorAction Stop
        $settings = Join-Path $root 'PSScriptAnalyzerSettings.psd1'
        $params = @{ Path = $root; Recurse = $true }
        if (Test-Path -LiteralPath $settings) { $params.Settings = $settings }
        $diagnostics = @(Invoke-ScriptAnalyzer @params)
        foreach ($group in $diagnostics | Group-Object Severity | Sort-Object Name) {
            Write-Host ("    {0}: {1}" -f $group.Name, $group.Count)
        }
        $blocking = @($diagnostics | Where-Object { $_.Severity -in $AnalyzerSeverity })
        foreach ($d in $blocking) {
            Write-Host ("    {0} {1}:{2} {3}" -f $d.Severity, (Split-Path $d.ScriptPath -Leaf), $d.Line, $d.RuleName)
        }
        Add-Result 'PSScriptAnalyzer' ($blocking.Count -eq 0) `
        ("{0} total diagnostics, {1} at gating severity ({2})" -f $diagnostics.Count, $blocking.Count, ($AnalyzerSeverity -join ','))
    } catch {
        Add-Result 'PSScriptAnalyzer' $false $_.Exception.Message
    }
}

# 6. Source/doc consistency: version + mod id must agree across the tree.
try {
    $shellText = [IO.File]::ReadAllText((Join-Path $root 'mod/visual-clones/maxwell-shell.wh.cpp'))
    $metaId = if ($shellText -match '(?m)^//\s+@id\s+(\S+)\s*$') { $Matches[1] } else { $null }
    $metaVersion = if ($shellText -match '(?m)^//\s+@version\s+(\S+)\s*$') { $Matches[1] } else { $null }

    $problems = [System.Collections.Generic.List[string]]::new()
    if ($metaId -ne 'opal') { $problems.Add("shell @id is '$metaId', expected 'opal'") }
    if (-not $metaVersion) { $problems.Add('shell @version not found') }

    foreach ($build in @('Build-OpalSuite.ps1', 'Build-OpalSuite.Linux.ps1')) {
        $buildPath = Join-Path $root $build
        if (-not (Test-Path -LiteralPath $buildPath)) { continue }
        $buildText = [IO.File]::ReadAllText($buildPath)
        if ($metaVersion -and $buildText -notmatch [regex]::Escape("`$version = '$metaVersion'")) {
            $problems.Add("$build does not pin version $metaVersion")
        }
        if ($buildText -notmatch "localId = 'local@opal'") {
            $problems.Add("$build does not declare localId local@opal")
        }
    }

    $readme = [IO.File]::ReadAllText((Join-Path $root 'README.md'))
    if ($metaVersion -and $readme -notmatch [regex]::Escape($metaVersion)) {
        $problems.Add("README.md does not mention version $metaVersion")
    }
    if ($readme -notmatch 'local@opal') { $problems.Add('README.md does not mention local@opal') }

    Add-Result 'Consistency' ($problems.Count -eq 0) `
    ($(if ($problems.Count) { $problems -join '; ' } else { "id=$metaId version=$metaVersion agree across sources, build scripts, README" }))
} catch {
    Add-Result 'Consistency' $false $_.Exception.Message
}

Write-Host ''
Write-Host '=== Summary ===' -ForegroundColor Cyan
$steps | Format-Table -AutoSize | Out-String | Write-Host
$failed = @($steps | Where-Object { -not $_.Passed })
if ($failed.Count) {
    Write-Host ("Static gate FAILED: {0}" -f (($failed | ForEach-Object Step) -join ', ')) -ForegroundColor Red
    exit 1
}
Write-Host 'Static gate PASSED.' -ForegroundColor Green
