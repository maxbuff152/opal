[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$sourceRoot = Join-Path $root 'native\Maxwell.Shell.Core'
$source = Join-Path $sourceRoot 'MaxwellShellCore.cpp'
$outputRoot = Join-Path $root 'build\maxwell-shell-core'
$output = Join-Path $outputRoot 'Maxwell.Shell.Core.exe'
$compilerRoot = 'C:\Program Files\Windhawk\Compiler'
$clang = Join-Path $compilerRoot 'bin\clang++.exe'

if (-not (Test-Path -LiteralPath $clang -PathType Leaf)) {
    throw "Windhawk native compiler is missing: $clang"
}
New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null

$arguments = @(
    '-std=c++20','-O2','-Wall','-Wextra','-Wpedantic','-target','x86_64-w64-mingw32',
    '-I',(Join-Path $compilerRoot 'include'),
    '-DUNICODE','-D_UNICODE','-DWIN32_LEAN_AND_MEAN',
    '-municode','-mwindows','-static','-static-libgcc','-static-libstdc++',
    $source,
    '-lpdh','-ldxgi','-lshell32',
    '-flto','-ffunction-sections','-fdata-sections',
    '-Wl,--gc-sections','-Wl,--icf=all','-Wl,-s',
    '-o',$output
)
& $clang @arguments
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $output -PathType Leaf)) {
    throw "Maxwell.Shell.Core compilation failed with exit code $LASTEXITCODE."
}

$item = Get-Item -LiteralPath $output
[pscustomobject]@{
    succeeded = $true
    output = $item.FullName
    bytes = $item.Length
    sha256 = (Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash
    sourceSha256 = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
    surface = 'telemetry-only'
    compiler = (& $clang --version | Select-Object -First 1)
}
