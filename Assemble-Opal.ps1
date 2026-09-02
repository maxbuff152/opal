<#
.SYNOPSIS
    Assembles the multi-header Opal source into ONE self-contained .wh.cpp.

.DESCRIPTION
    Windhawk marketplace mods are a single file - they cannot #include local
    headers. Opal is developed as a main source plus its shell and taskbar-
    geometry headers for sanity; this inlines all of them into one publishable
    file and strips the optional Maxhawk standalone scaffolding.

    Output: dist\Opal\opal.wh.cpp
#>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$dir  = Join-Path $PSScriptRoot 'mod\visual-clones'
$main = Join-Path $dir 'maxwell-shell.wh.cpp'
$outDir = Join-Path $PSScriptRoot 'dist\Opal'
New-Item -ItemType Directory -Path $outDir -Force | Out-Null
$out = Join-Path $outDir 'opal.wh.cpp'

# Headers inlined in dependency order (the order the main file includes them).
$headers = @(
    'maxwell-shell-rules.h',
    'maxwell-shell-owned-overrides.h',
    'maxwell-shell-selector.h',
    'maxwell-shell-apply.h',
    'maxwell-shell-style.h',
    'opal-control.h',
    'maxwell-xaml-tap.h',
    'opal-addon-icons.h',
    'opal-addon-clock.h'
)

function Strip-Header([string]$path) {
    $lines = [System.IO.File]::ReadAllLines($path)
    $keep = [System.Collections.Generic.List[string]]::new()
    foreach ($l in $lines) {
        if ($l -match '^\s*#pragma\s+once\s*$') { continue }
        $keep.Add($l)
    }
    return ($keep -join "`r`n")
}

$text = [System.IO.File]::ReadAllText($main)

# 1. Unwrap "#ifdef WH_MOD ... #endif" - keep its contents (the windhawk_api includes).
$text = [regex]::Replace($text,
    '(?ms)^#ifdef WH_MOD\r?\n(.*?)^#endif\r?\n',
    '$1')

# 2. Drop every "#ifndef WH_MOD ... #endif" block (the Maxhawk standalone path).
$text = [regex]::Replace($text,
    '(?ms)^#ifndef WH_MOD\r?\n.*?^#endif[^\r\n]*\r?\n',
    '')

# 3. Inline each local header in place of its #include line.
foreach ($h in $headers) {
    $body = Strip-Header (Join-Path $dir $h)
    $banner = "// ===== inlined: $h =====`r`n"
    $pattern = '^\s*#include\s+"' + [regex]::Escape($h) + '"\s*$'
    $replacement = $banner + $body
    $text = [regex]::Replace(
        $text,
        $pattern,
        [Text.RegularExpressions.MatchEvaluator]{ param($match) $replacement },
        [Text.RegularExpressions.RegexOptions]::Multiline)
}

# 4. Any remaining local includes would break a single-file build - fail loudly.
$stray = [regex]::Matches($text, '(?m)^\s*#include\s+"[^"]+"')
if ($stray.Count -gt 0) {
    throw "Un-inlined local include remains: $($stray[0].Value.Trim())"
}
if ($text -match 'maxhawk-runtime') { throw 'Maxhawk reference survived the strip.' }

[System.IO.File]::WriteAllText($out, $text)

$lines = ([regex]::Matches($text, "`n")).Count + 1
Write-Host ''
Write-Host "  assembled single-file Opal" -ForegroundColor Green
Write-Host ("  output : {0}" -f $out)
Write-Host ("  lines  : {0}" -f $lines)
Write-Host ("  size   : {0} KB" -f [math]::Round((Get-Item $out).Length / 1KB))
