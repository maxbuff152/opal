<#
.SYNOPSIS
    Cross-builds Opal's unified Windhawk DLL on Linux (clang-20 + LLVM-MinGW).

.DESCRIPTION
    Faithful Linux counterpart to Build-OpalSuite.ps1. It compiles the Shell,
    Media, and Performance components and links the single `local@opal` DLL with
    the same flags Windhawk uses, but drives a native clang-20/lld-20 toolchain
    against Windhawk's own headers (windhawk_api.h, windhawk_utils.h, the
    cppwinrt WinRT projections) and engine import library (windhawk.lib),
    provisioned by .cursor/install-buildchain.sh into a git-ignored cache.

    Output mirrors Build-OpalSuite.ps1 exactly (build\opal-suite\*.dll,
    build-receipt.json, sources\opal.wh.cpp) so Test-OpalSuite.ps1 -StaticOnly
    validates the result on Linux.

    This produces a real Windows PE DLL for compile/link verification. Runtime
    behavior (Explorer/Windhawk integration) still requires a Windows host.

.PARAMETER ToolchainDir
    Root of the staged Windhawk toolchain. Defaults to $env:OPAL_WINDHAWK_TOOLCHAIN
    or ~/.cache/opal-windhawk.
#>
[CmdletBinding()]
param(
    [string] $ToolchainDir = $(if ($env:OPAL_WINDHAWK_TOOLCHAIN) { $env:OPAL_WINDHAWK_TOOLCHAIN } else { Join-Path $HOME '.cache/opal-windhawk' })
)

$ErrorActionPreference = 'Stop'

$clang = 'clang++-20'
if (-not (Get-Command $clang -ErrorAction SilentlyContinue)) {
    throw "clang++-20 not found. Run .cursor/install-buildchain.sh first."
}
# Ensure the matching lld is discoverable for -fuse-ld=lld.
$llvmBin = '/usr/lib/llvm-20/bin'
if (Test-Path -LiteralPath $llvmBin) { $env:PATH = "${llvmBin}:$env:PATH" }

$compilerInclude = Join-Path $ToolchainDir 'Compiler/include'
$resourceDir = Join-Path $ToolchainDir 'Compiler/lib/clang/20'
$sysroot = Join-Path $ToolchainDir 'Compiler/x86_64-w64-mingw32'
$engineLibrary = Join-Path $ToolchainDir 'Engine/64/windhawk.lib'
foreach ($required in @($compilerInclude, $engineLibrary, (Join-Path $resourceDir 'include'))) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Toolchain component missing: $required. Run .cursor/install-buildchain.sh."
    }
}

$sourceRoot = Join-Path $PSScriptRoot 'mod/visual-clones'
$outputRoot = Join-Path $PSScriptRoot 'build/opal-suite'
$objectRoot = Join-Path $outputRoot 'objects'
$packageRoot = Join-Path $outputRoot 'sources'
foreach ($path in @($outputRoot, $objectRoot, $packageRoot)) {
    New-Item -ItemType Directory -Path $path -Force | Out-Null
}

$version = '4.4.0'
$metadataId = 'opal'
$localId = 'local@opal'
$include = 'explorer.exe|StartMenuExperienceHost.exe|SearchHost.exe|SearchApp.exe|ShellExperienceHost.exe|ShellHost.exe'
$shellSource = Join-Path $sourceRoot 'maxwell-shell.wh.cpp'
$mediaSource = Join-Path $sourceRoot 'maxwell-opal-media.wh.cpp'
$performanceSource = Join-Path $sourceRoot 'maxwell-taskbar-system-info.wh.cpp'
$output = Join-Path $outputRoot "local_at_opal_${version}_owned.dll"
$installedSource = Join-Path $packageRoot 'opal.wh.cpp'

foreach ($file in @(Get-ChildItem -LiteralPath $outputRoot -File -ErrorAction SilentlyContinue)) {
    if ($file.Name -like 'local_at_opal*_owned.dll' -and $file.FullName -ne $output) {
        Remove-Item -LiteralPath $file.FullName -Force
    }
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
    if ($rawGlyphs.Count) { throw "Raw private-use glyph found in $source; use a \uXXXX escape." }
}

& (Join-Path $PSScriptRoot 'Assemble-Opal.ps1') | Out-Null
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'dist/Opal/opal.wh.cpp') -Destination $installedSource -Force

# Windhawk-equivalent flags, adapted to drive a foreign clang against the
# bundle's headers (libc++ before the clang builtins before the SDK headers).
$toolchainFlags = @(
    '-target', 'x86_64-w64-mingw32', '-nostdinc',
    '-isystem', (Join-Path $compilerInclude 'c++/v1'),
    '-isystem', (Join-Path $resourceDir 'include'),
    '-isystem', $compilerInclude,
    '-resource-dir', $resourceDir
)
$common = @(
    '-std=c++23', '-Os', '-c', '-DWH_MOD', '-DOPAL_UNIFIED_BUILD',
    ('-DWH_MOD_ID=L"{0}"' -f $localId), ('-DWH_MOD_VERSION=L"{0}"' -f $version),
    '-DUNICODE', '-D_UNICODE', '-DWINVER=0x0A00', '-D_WIN32_WINNT=0x0A00',
    '-D_WIN32_IE=0x0A00', '-DNTDDI_VERSION=0x0A000008', '-D__USE_MINGW_ANSI_STDIO=0'
) + $toolchainFlags + @(
    '-include', 'windhawk_api.h', '-flto', '-ffunction-sections', '-fdata-sections', '-fno-ident'
)
$entryRenames = @(
    '-DWh_ModInit={0}_ModInit', '-DWh_ModAfterInit={0}_ModAfterInit',
    '-DWh_ModSettingsChanged={0}_ModSettingsChanged',
    '-DWh_ModBeforeUninit={0}_ModBeforeUninit', '-DWh_ModUninit={0}_ModUninit'
)
$objects = @(
    [pscustomobject]@{ Name = 'shell'; Source = $shellSource; Output = (Join-Path $objectRoot 'shell.o'); Defines = @() }
    [pscustomobject]@{ Name = 'media'; Source = $mediaSource; Output = (Join-Path $objectRoot 'media.o'); Defines = @($entryRenames | ForEach-Object { $_ -f 'OpalMedia' }) }
    [pscustomobject]@{ Name = 'performance'; Source = $performanceSource; Output = (Join-Path $objectRoot 'performance.o'); Defines = @($entryRenames | ForEach-Object { $_ -f 'OpalPerformance' }) + '-DWIN32_LEAN_AND_MEAN' }
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
    '--sysroot' $sysroot '-resource-dir' $resourceDir `
    '-stdlib=libc++' '-rtlib=compiler-rt' '-unwindlib=libunwind' '-fuse-ld=lld' `
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
    components = @($objects | ForEach-Object {
            [pscustomobject]@{ name = $_.Name; source = $_.Source; sha256 = (Get-FileHash -LiteralPath $_.Source -Algorithm SHA256).Hash }
        })
    packageSource = $installedSource
    packageSourceSha256 = (Get-FileHash -LiteralPath $installedSource -Algorithm SHA256).Hash
    compilerVersion = (& $clang --version | Select-Object -First 1)
    engineVersion = "windhawk-1.7.3 (linux-cross)"
}
$receipt | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $outputRoot 'build-receipt.json') -Encoding UTF8
foreach ($component in $objects) {
    if (Test-Path -LiteralPath $component.Output) { Remove-Item -LiteralPath $component.Output -Force }
}
if ((Test-Path -LiteralPath $objectRoot) -and -not @(Get-ChildItem -LiteralPath $objectRoot -Force).Count) {
    Remove-Item -LiteralPath $objectRoot
}
Write-Host ("Built {0} ({1:N0} bytes) on Linux" -f $item.Name, $item.Length) -ForegroundColor Green
$receipt
