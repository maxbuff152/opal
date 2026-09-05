<#
.SYNOPSIS
    Builds Opal as one Windhawk mod with internal Shell, Media, and Performance components.
#>
[CmdletBinding()]
param(
    # Candidate builds cannot replace the ordinary build artifact or receipt.
    [ValidatePattern('^[A-Za-z0-9][A-Za-z0-9_-]{0,63}$')]
    [string]$CandidateName,
    [ValidateSet('Required', 'LegacyAllSymbols')]
    [string]$ExportMode = 'Required'
)

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
if ($CandidateName) { $outputRoot = Join-Path $PSScriptRoot "build\opal-suite-candidates\$CandidateName" }
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

# Read the PE tables without loading the DLL or running static initializers.
# An ASCII substring search also accepts private strings and missing exports.
function Get-OpalPeExports([string]$Path) {
    $bytes = [IO.File]::ReadAllBytes($Path)
    function Read-U16([long]$Offset) {
        if ($Offset -lt 0 -or $Offset + 2 -gt $bytes.Length) { throw 'Truncated PE field.' }
        [BitConverter]::ToUInt16($bytes, [int]$Offset)
    }
    function Read-U32([long]$Offset) {
        if ($Offset -lt 0 -or $Offset + 4 -gt $bytes.Length) { throw 'Truncated PE field.' }
        [BitConverter]::ToUInt32($bytes, [int]$Offset)
    }
    if ((Read-U16 0) -ne 0x5A4D) { throw 'Missing DOS signature.' }
    $pe = [long](Read-U32 0x3C)
    if ((Read-U32 $pe) -ne 0x4550 -or (Read-U16 ($pe+4)) -ne 0x8664) { throw 'Expected x64 PE.' }
    $optional = $pe + 24
    $optionalSize = Read-U16 ($pe+20)
    if ($optionalSize -lt 120 -or (Read-U16 $optional) -ne 0x20B) { throw 'Expected PE32+ optional header.' }
    $sections = @(for ($i=0; $i -lt (Read-U16 ($pe+6)); $i++) {
        $offset = $optional + $optionalSize + 40*$i
        $characteristics = Read-U32 ($offset+36)
        [pscustomobject]@{
            Name = [Text.Encoding]::ASCII.GetString($bytes, [int]$offset, 8).TrimEnd([char]0)
            VirtualSize = Read-U32 ($offset+8); Rva = Read-U32 ($offset+12)
            RawSize = Read-U32 ($offset+16); RawOffset = Read-U32 ($offset+20)
            Characteristics = $characteristics
        }
    })
    function Rva-Offset([long]$Rva, [long]$Length=1) {
        foreach ($section in $sections) {
            $delta = $Rva - $section.Rva
            if ($delta -ge 0 -and $delta+$Length -le $section.RawSize) {
                $offset = [long]$section.RawOffset + $delta
                if ($offset+$Length -gt $bytes.Length) { throw 'Truncated PE section.' }
                return $offset
            }
        }
        throw 'PE RVA has no file-backed section.'
    }
    $exportRva = Read-U32 ($optional+112)
    $exportSize = Read-U32 ($optional+116)
    if (-not $exportRva -or $exportSize -lt 40) { throw 'Missing PE export directory.' }
    $directory = Rva-Offset $exportRva 40
    $functionCount = Read-U32 ($directory+20)
    $nameCount = Read-U32 ($directory+24)
    if (-not $nameCount -or $nameCount -gt 100000 -or $functionCount -gt 100000) { throw 'Invalid PE export count.' }
    $functions = Rva-Offset (Read-U32 ($directory+28)) (4L*$functionCount)
    $names = Rva-Offset (Read-U32 ($directory+32)) (4L*$nameCount)
    $ordinals = Rva-Offset (Read-U32 ($directory+36)) (2L*$nameCount)
    $exports = @(for ($i=0; $i -lt $nameCount; $i++) {
        $nameRva = Read-U32 ($names+4L*$i)
        $offset = Rva-Offset $nameRva
        $length = 0
        while ($length -lt 4096 -and $offset+$length -lt $bytes.Length -and $bytes[$offset+$length]) { $length++ }
        if ($length -eq 4096 -or $offset+$length -ge $bytes.Length) { throw 'Invalid PE export name.' }
        [void](Rva-Offset $nameRva ($length+1))
        $name = [Text.Encoding]::ASCII.GetString($bytes, [int]$offset, $length)
        $ordinal = Read-U16 ($ordinals+2L*$i)
        if ($ordinal -ge $functionCount) { throw 'Invalid PE export ordinal.' }
        $target = Read-U32 ($functions+4L*$ordinal)
        if (-not $target -or ($target -ge $exportRva -and $target -lt [long]$exportRva+$exportSize)) { throw 'Null or forwarded export is not supported.' }
        $targetSection = @($sections | Where-Object { $target -ge $_.Rva -and $target -lt [long]$_.Rva+[Math]::Max($_.VirtualSize,$_.RawSize) })
        if ($targetSection.Count -ne 1) { throw 'Export target is outside the image sections.' }
        [pscustomobject]@{Name=$name; Rva=$target; Section=$targetSection[0].Name; Characteristics=$targetSection[0].Characteristics}
    })
    if (@($exports.Name | Sort-Object -Unique).Count -ne $nameCount) { throw 'Duplicate PE export names.' }
    [pscustomobject]@{Exports=$exports; Sections=$sections; SizeOfImage=(Read-U32 ($optional+56)); EntryPointRva=(Read-U32 ($optional+16))}
}

function Assert-OpalExportContract($PeInfo, [string[]]$RequiredExports, [string]$Mode) {
    foreach ($name in $RequiredExports) {
        $export = @($PeInfo.Exports | Where-Object Name -CEQ $name)
        if ($export.Count -ne 1) { throw "Required Opal export missing: $name" }
        $expectedFlag = if ($name -eq 'InternalWhModPtr') { 0x80000000L } else { 0x20000000L }
        if (-not ($export[0].Characteristics -band $expectedFlag)) { throw "Opal export has incorrect section permissions: $name" }
    }
    if (-not $PeInfo.EntryPointRva) { throw 'Missing DLL initialization entrypoint.' }
    if ($Mode -eq 'Required' -and $PeInfo.Exports.Count -ne $RequiredExports.Count) { throw 'Unexpected Opal exports.' }
}

$exportDefinition = Join-Path $PSScriptRoot 'config\OpalExports.def'
$buildScriptPath = $PSCommandPath
$requiredExports = @(Get-Content -LiteralPath $exportDefinition | ForEach-Object { ($_ -split ';',2)[0].Trim() } |
    Where-Object { $_ -and $_ -ne 'EXPORTS' } | ForEach-Object { ($_ -split '\s+')[0] })

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
$exportArguments = @($exportDefinition, '-Wl,--exclude-all-symbols')
if ($ExportMode -eq 'LegacyAllSymbols') { $exportArguments = @('-Wl,--export-all-symbols') }
& $clang '-std=c++23' '-Os' '-shared' '-target' 'x86_64-w64-mingw32' `
    @($objects.Output) $engineLibrary '-flto' @exportArguments `
    '-Wl,--no-insert-timestamp' '-Wl,--gc-sections' '-Wl,--icf=all' '-Wl,-s' `
    @libraries '-o' $output
if ($LASTEXITCODE -ne 0) { throw 'Unified Opal link failed.' }

$peInfo = Get-OpalPeExports $output
Assert-OpalExportContract $peInfo $requiredExports $ExportMode

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
    # The existing acceptance gate validates every sourceInputs hash. Include
    # linker policy and this build script so flag/export drift invalidates it.
    sourceInputs = @(@(Get-ChildItem -LiteralPath $sourceRoot -File | Where-Object Extension -in @('.h','.cpp')) +
        @(Get-Item -LiteralPath $exportDefinition, $buildScriptPath) | Sort-Object FullName | ForEach-Object {
        [pscustomobject]@{ path=$_.FullName; sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
    })
    components = @($objects | ForEach-Object {
        [pscustomobject]@{ name=$_.Name; source=$_.Source; sha256=(Get-FileHash -LiteralPath $_.Source -Algorithm SHA256).Hash }
    })
    packageSource = $installedSource
    packageSourceSha256 = (Get-FileHash -LiteralPath $installedSource -Algorithm SHA256).Hash
    compilerVersion = (& $clang --version | Select-Object -First 1)
    engineVersion = $engineVersion.Name
    exportMode = $ExportMode
    exportDefinitionSha256 = (Get-FileHash -LiteralPath $exportDefinition -Algorithm SHA256).Hash
    exports = @($peInfo.Exports.Name)
    imageSize = $peInfo.SizeOfImage
    imageSections = $peInfo.Sections
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
