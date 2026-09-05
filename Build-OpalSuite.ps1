<#
.SYNOPSIS
    Builds Opal as one Windhawk mod with internal Shell, Media, and Performance components.
#>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$compilerRoot = 'C:\Program Files\Windhawk\Compiler'
$clang = Join-Path $compilerRoot 'bin\clang++.exe'
$engineVersion = Get-ChildItem -LiteralPath 'C:\Program Files\Windhawk\Engine' -Directory |
    Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
if (-not (Test-Path -LiteralPath $clang)) { throw "Windhawk compiler not found: $clang" }
if (-not $engineVersion) { throw 'Windhawk engine files were not found.' }
$engineLibrary = Join-Path $engineVersion.FullName '64\windhawk.lib'

$sourceRoot = Join-Path $PSScriptRoot 'mod\visual-clones'
$outputRoot = Join-Path $PSScriptRoot 'build\opal-suite'
$objectRoot = Join-Path $outputRoot 'objects'
$packageRoot = Join-Path $outputRoot 'sources'
foreach ($path in @($outputRoot, $objectRoot, $packageRoot)) {
    New-Item -ItemType Directory -Path $path -Force | Out-Null
}

$version = '4.5.0'
$metadataId = 'opal'
$localId = 'local@opal'
$include = 'explorer.exe|StartMenuExperienceHost.exe|SearchHost.exe|SearchApp.exe|ShellExperienceHost.exe|ShellHost.exe'
$shellSource = Join-Path $sourceRoot 'maxwell-shell.wh.cpp'
$mediaSource = Join-Path $sourceRoot 'maxwell-opal-media.wh.cpp'
$performanceSource = Join-Path $sourceRoot 'maxwell-taskbar-system-info.wh.cpp'
$output = Join-Path $outputRoot "local_at_opal_${version}_owned.dll"
$installedSource = Join-Path $packageRoot 'opal.wh.cpp'

# One mod means one package artifact. Remove only known superseded build
# outputs; source-owned rollback bundles remain untouched.
foreach ($file in @(Get-ChildItem -LiteralPath $outputRoot -File -ErrorAction SilentlyContinue)) {
    $retired = ($file.Name -like 'local_at_opal*_owned.dll' -and $file.FullName -ne $output) -or
               $file.Name -like 'Opal-Control*.exe' -or
               $file.Name -like '*probe*.exe' -or
               $file.Name -eq 'control-build-receipt.json'
    if ($retired) { Remove-Item -LiteralPath $file.FullName -Force }
}
foreach ($file in @(Get-ChildItem -LiteralPath $packageRoot -File -Filter 'opal*.wh.cpp' -ErrorAction SilentlyContinue)) {
    if ($file.FullName -ne $installedSource) { Remove-Item -LiteralPath $file.FullName -Force }
}

function Get-MetadataValue([string[]] $Header, [string] $Name) {
    $escapedName = [regex]::Escape($Name)
    $line = $Header | Where-Object { $_ -match "^//\s+@$escapedName\s+(.*)$" } | Select-Object -First 1
    if ($line -and $line -match "^//\s+@$escapedName\s+(.*)$") { return $Matches[1].Trim() }
    $null
}

$header = Get-Content -LiteralPath $shellSource -TotalCount 80
if ((Get-MetadataValue $header 'id') -ne $metadataId) { throw 'Opal metadata ID mismatch.' }
if ((Get-MetadataValue $header 'version') -ne $version) { throw 'Opal metadata version mismatch.' }
foreach ($source in @($shellSource, $mediaSource, $performanceSource)) {
    if (-not (Test-Path -LiteralPath $source)) { throw "Component source is missing: $source" }
    $rawGlyphs = [regex]::Matches([IO.File]::ReadAllText($source), '[\uE000-\uF8FF]')
    if ($rawGlyphs.Count) { throw "Raw private-use glyph found in $source; use a \\uXXXX escape." }
}

& (Join-Path $PSScriptRoot 'Assemble-Opal.ps1')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'dist\Opal\opal.wh.cpp') `
    -Destination $installedSource -Force

$common = @(
    '-std=c++23', '-Os', '-c', '-DWH_MOD', '-DOPAL_UNIFIED_BUILD',
    ('-DWH_MOD_ID=L"{0}"' -f $localId), ('-DWH_MOD_VERSION=L"{0}"' -f $version),
    '-DUNICODE', '-D_UNICODE', '-DWINVER=0x0A00', '-D_WIN32_WINNT=0x0A00',
    '-D_WIN32_IE=0x0A00', '-DNTDDI_VERSION=0x0A000008', '-D__USE_MINGW_ANSI_STDIO=0',
    '-target', 'x86_64-w64-mingw32', '-I', (Join-Path $compilerRoot 'include'),
    '-include', 'windhawk_api.h', '-flto', '-ffunction-sections', '-fdata-sections', '-fno-ident'
)
$entryRenames = @(
    '-DWh_ModInit={0}_ModInit', '-DWh_ModAfterInit={0}_ModAfterInit',
    '-DWh_ModSettingsChanged={0}_ModSettingsChanged',
    '-DWh_ModBeforeUninit={0}_ModBeforeUninit', '-DWh_ModUninit={0}_ModUninit'
)
$objects = @(
    [pscustomobject]@{ Name='shell'; Source=$shellSource; Output=(Join-Path $objectRoot 'shell.o'); Defines=@() }
    [pscustomobject]@{ Name='media'; Source=$mediaSource; Output=(Join-Path $objectRoot 'media.o'); Defines=@($entryRenames | ForEach-Object { $_ -f 'OpalMedia' }) }
    [pscustomobject]@{ Name='performance'; Source=$performanceSource; Output=(Join-Path $objectRoot 'performance.o'); Defines=@($entryRenames | ForEach-Object { $_ -f 'OpalPerformance' }) + '-DWIN32_LEAN_AND_MEAN' }
)
foreach ($component in $objects) {
    & $clang @common @($component.Defines) $component.Source `
        ("-frandom-seed=opal-{0}-{1}" -f $component.Name, $version) '-o' $component.Output
    if ($LASTEXITCODE -ne 0) { throw "Compilation failed for $($component.Name)." }
}

$libraries = @(
    '-lcomctl32', '-ldxgi', '-lgdi32', '-lole32', '-loleaut32', '-lpdh', '-lpowrprof',
    '-lruntimeobject', '-lwindowsapp', '-lshell32', '-lwindowscodecs', '-lshlwapi',
    '-lpsapi', '-ldwmapi', '-lshcore', '-lversion', '-lwininet', '-lwtsapi32',
    '-luser32', '-lkernel32'
)
& $clang '-std=c++23' '-Os' '-shared' '-target' 'x86_64-w64-mingw32' `
    @($objects.Output) $engineLibrary '-flto' '-Wl,--export-all-symbols' `
    '-Wl,--no-insert-timestamp' '-Wl,--gc-sections' '-Wl,--icf=all' '-Wl,-s' `
    @libraries '-o' $output
if ($LASTEXITCODE -ne 0) { throw 'Unified Opal link failed.' }

$ascii = [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($output))
if ($ascii -notmatch 'Wh_ModInit' -or $ascii -notmatch 'DllGetClassObject') {
    throw 'Unified Opal exports are incomplete.'
}

$item = Get-Item -LiteralPath $output
$receipt = [pscustomobject]@{
    key = 'opal'
    metadataId = $metadataId
    localId = $localId
    version = $version
    include = $include
    dllName = $item.Name
    output = $item.FullName
    bytes = $item.Length
    sha256 = (Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash
    source = $shellSource
    sourceSha256 = (Get-FileHash -LiteralPath $shellSource -Algorithm SHA256).Hash
    sourceInputs = @(Get-ChildItem -LiteralPath $sourceRoot -File | Where-Object Extension -in @('.h','.cpp') | Sort-Object Name | ForEach-Object {
        [pscustomobject]@{ path=$_.FullName; sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
    })
    components = @($objects | ForEach-Object {
        [pscustomobject]@{ name=$_.Name; source=$_.Source; sha256=(Get-FileHash -LiteralPath $_.Source -Algorithm SHA256).Hash }
    })
    packageSource = $installedSource
    packageSourceSha256 = (Get-FileHash -LiteralPath $installedSource -Algorithm SHA256).Hash
    compilerVersion = (& $clang --version | Select-Object -First 1)
    engineVersion = $engineVersion.Name
}
$receipt | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath `
    (Join-Path $outputRoot 'build-receipt.json') -Encoding UTF8
foreach ($component in $objects) {
    if (Test-Path -LiteralPath $component.Output) {
        Remove-Item -LiteralPath $component.Output -Force
    }
}
if ((Test-Path -LiteralPath $objectRoot) -and
    -not @(Get-ChildItem -LiteralPath $objectRoot -Force).Count) {
    Remove-Item -LiteralPath $objectRoot
}
$receipt
