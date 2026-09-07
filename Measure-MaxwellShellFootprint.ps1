<#
.SYNOPSIS
    Captures the runtime + source footprint of the unified one-mod Opal suite.

.DESCRIPTION
    Run once before the consolidation (-Label baseline) and again after
    (-Label merged) to get a like-for-like comparison. Modelled on the existing
    20-theme-before-after.json harness in MaxwellWallpaperHost: several timed
    samples, median reported, so a single noisy sample cannot skew the result.

    Measures, per host process that Windhawk injects into:
      - how many Windhawk mod DLLs are loaded, and their total bytes
      - process private / working set / handle count / thread count
    Plus, statically:
      - source line counts and built DLL sizes per mod
      - how many enabled mods exist

    Read-only. Writes a JSON receipt next to the script under measurements\.

.PARAMETER Label
    Name for this capture, e.g. baseline or merged.

.PARAMETER Samples
    How many runtime samples to take (default 3).

.PARAMETER SampleSeconds
    Seconds to settle between samples (default 4).
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)] [string] $Label,
    [int] $Samples = 3,
    [int] $SampleSeconds = 4
)

$ErrorActionPreference = 'Stop'

$hostProcesses = @('explorer', 'StartMenuExperienceHost', 'SearchHost', 'SearchApp', 'ShellExperienceHost', 'ShellHost')
$engineModRoot = 'C:\ProgramData\Windhawk\Engine\Mods\64'
$srcRoot       = Join-Path $PSScriptRoot 'mod\visual-clones'
$buildRoot     = Join-Path $PSScriptRoot 'build\opal-suite'
$outRoot       = Join-Path $PSScriptRoot 'measurements'
New-Item -ItemType Directory -Path $outRoot -Force | Out-Null

# --- which mods are enabled ----------------------------------------------------
$modsKey = 'HKLM:\SOFTWARE\Windhawk\Engine\Mods'
$enabled = @()
if (Test-Path $modsKey) {
    $enabled = @(Get-ChildItem $modsKey | ForEach-Object {
        $p = Get-ItemProperty $_.PSPath -ErrorAction SilentlyContinue
        if ([int]$p.Disabled -eq 0) {
            [pscustomobject]@{
                Mod = $_.PSChildName
                Version = [string]$p.Version
                LibraryFileName = [string]$p.LibraryFileName
                Include = [string]$p.Include
            }
        }
    } | Where-Object { $_ })
}
$registeredDllNames = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($mod in $enabled) {
    if ($mod.LibraryFileName) { [void]$registeredDllNames.Add($mod.LibraryFileName) }
}

# --- runtime samples -----------------------------------------------------------
function Get-RuntimeSample {
    $rows = @()
    foreach ($name in $hostProcesses) {
        foreach ($proc in @(Get-Process -Name $name -ErrorAction SilentlyContinue)) {
            $modDlls = @()
            try {
                $modDlls = @($proc.Modules | Where-Object {
                    $_.FileName -like "$engineModRoot\*" -and
                    $registeredDllNames.Contains((Split-Path $_.FileName -Leaf))
                })
            } catch {
                # Access denied on some hosts is expected and non-fatal.
                Write-Verbose "Module enumeration denied for ${name}: $($_.Exception.Message)"
            }
            $rows += [pscustomobject]@{
                Process       = $name
                Pid           = $proc.Id
                ModDllCount   = $modDlls.Count
                ModDllBytes   = ($modDlls | ForEach-Object { try { (Get-Item $_.FileName).Length } catch { 0 } } | Measure-Object -Sum).Sum
                ModDlls       = @($modDlls | ForEach-Object { Split-Path $_.FileName -Leaf } | Sort-Object)
                PrivateMB     = [math]::Round($proc.PrivateMemorySize64 / 1MB, 1)
                WorkingSetMB  = [math]::Round($proc.WorkingSet64 / 1MB, 1)
                Handles       = $proc.HandleCount
                Threads       = $proc.Threads.Count
            }
        }
    }
    return $rows
}

$all = @()
for ($i = 1; $i -le $Samples; $i++) {
    Write-Host ("  sample {0}/{1}..." -f $i, $Samples)
    $all += ,(Get-RuntimeSample)
    if ($i -lt $Samples) { Start-Sleep -Seconds $SampleSeconds }
}

# median per process name, across samples
function Get-Median([double[]]$v) {
    if (-not $v -or $v.Count -eq 0) { return 0 }
    $s = @($v | Sort-Object); $n = $s.Count
    if ($n % 2) { return $s[[int](($n - 1) / 2)] }
    return [math]::Round((($s[$n/2 - 1] + $s[$n/2]) / 2), 1)
}

$byProcess = @()
foreach ($name in ($all | ForEach-Object { $_ } | ForEach-Object { $_.Process } | Sort-Object -Unique)) {
    $rows = @($all | ForEach-Object { $_ } | Where-Object { $_.Process -eq $name })
    $byProcess += [pscustomobject]@{
        Process      = $name
        ModDllCount  = Get-Median ([double[]]@($rows.ModDllCount))
        ModDllKB     = [math]::Round((Get-Median ([double[]]@($rows.ModDllBytes))) / 1KB)
        PrivateMB    = Get-Median ([double[]]@($rows.PrivateMB))
        WorkingSetMB = Get-Median ([double[]]@($rows.WorkingSetMB))
        Handles      = Get-Median ([double[]]@($rows.Handles))
        Threads      = Get-Median ([double[]]@($rows.Threads))
        ModDlls      = @($rows[0].ModDlls)
    }
}

# --- static footprint ----------------------------------------------------------
$officialSources = @(
    'maxwell-shell.wh.cpp',
    'maxwell-opal-media.wh.cpp',
    'maxwell-taskbar-system-info.wh.cpp'
)
$sources = @()
if (Test-Path $srcRoot) {
    $sources = @(Get-ChildItem $srcRoot -File | Where-Object { $_.Name -in $officialSources } | ForEach-Object {
        [pscustomobject]@{ Source = $_.BaseName; Lines = ([System.IO.File]::ReadAllLines($_.FullName)).Count; KB = [math]::Round($_.Length/1KB) }
    } | Sort-Object Source)
}
$builtDlls = @()
if (Test-Path $buildRoot) {
    $builtDlls = @(Get-ChildItem $buildRoot -Filter '*.dll' | ForEach-Object {
        [pscustomobject]@{ Dll = $_.Name; KB = [math]::Round($_.Length/1KB) }
    } | Sort-Object Dll)
}

$totalInjectedKB = ($byProcess | Measure-Object ModDllKB -Sum).Sum
$totalInjected   = ($byProcess | Measure-Object ModDllCount -Sum).Sum

$receipt = [pscustomobject]@{
    Label              = $Label
    CapturedUtc        = (Get-Date).ToUniversalTime().ToString('o')
    Samples            = $Samples
    EnabledModCount    = $enabled.Count
    EnabledMods        = @($enabled | Sort-Object Mod)
    TotalInjectedDlls  = $totalInjected
    TotalInjectedKB    = $totalInjectedKB
    PerProcess         = $byProcess
    SourceLinesTotal   = ($sources | Measure-Object Lines -Sum).Sum
    Sources            = $sources
    BuiltDlls          = $builtDlls
}

$outFile = Join-Path $outRoot "footprint-$Label.json"
$json = ($receipt | ConvertTo-Json -Depth 6) -replace "`r`n", "`n"
[IO.File]::WriteAllText($outFile, ($json + "`n"), [Text.UTF8Encoding]::new($false))

Write-Host ""
Write-Host "  === $Label ===" -ForegroundColor Cyan
Write-Host ("  enabled mods        {0}" -f $enabled.Count)
Write-Host ("  injected mod DLLs   {0}  ({1} KB total)" -f $totalInjected, $totalInjectedKB)
Write-Host ("  source lines        {0}" -f $receipt.SourceLinesTotal)
Write-Host ""
$byProcess | Format-Table Process, ModDllCount, ModDllKB, PrivateMB, WorkingSetMB, Handles, Threads -AutoSize
Write-Host ("  receipt: {0}" -f $outFile) -ForegroundColor Green
