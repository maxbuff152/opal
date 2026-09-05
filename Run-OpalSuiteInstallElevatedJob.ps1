[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$resultPath = Join-Path $PSScriptRoot 'build\opal-suite\elevated-install-result.json'
try {
    $recoveredFrom = $null
    $liveModKey = 'HKLM:\SOFTWARE\Windhawk\Engine\Mods\local@opal'
    $liveLibrary = if (Test-Path -LiteralPath $liveModKey) {
        (Get-ItemProperty -LiteralPath $liveModKey).LibraryFileName
    } else { '' }
    $liveDll = Join-Path 'C:\ProgramData\Windhawk\Engine\Mods\64' ([IO.Path]::GetFileName([string]$liveLibrary))
    if (-not (Test-Path -LiteralPath $liveModKey) -or
        -not (Test-Path -LiteralPath $liveDll -PathType Leaf)) {
        $rollbackRoot = Join-Path $env:LOCALAPPDATA 'Maxwell\Opal'
        $candidate = Get-ChildItem -LiteralPath $rollbackRoot -Directory `
                -Filter 'rollback-*' -ErrorAction SilentlyContinue |
            Sort-Object LastWriteTime -Descending |
            Where-Object {
                $manifestPath = Join-Path $_.FullName 'manifest.json'
                if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
                    return $false
                }
                $manifest = Get-Content -Raw -LiteralPath $manifestPath |
                    ConvertFrom-Json
                return @($manifest.ModState |
                    Where-Object Mod -eq 'local@opal').Count -gt 0
            } |
            Select-Object -First 1
        if (-not $candidate) {
            throw 'Live Opal is incomplete and no valid rollback bundle was found.'
        }
        & (Join-Path $PSScriptRoot 'Restore-MaxwellShellRollbackPoint.ps1') `
            -Bundle $candidate.FullName -NoSafety
        $recoveredFrom = $candidate.FullName
    }
    $output = & (Join-Path $PSScriptRoot 'Install-OpalSuite.ps1') 6>&1 | Out-String
    [pscustomobject]@{
        succeeded = $true
        completedUtc = (Get-Date).ToUniversalTime().ToString('o')
        recoveredFrom = $recoveredFrom
        output = $output
    } | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $resultPath -Encoding UTF8
}
catch {
    [pscustomobject]@{
        succeeded = $false
        completedUtc = (Get-Date).ToUniversalTime().ToString('o')
        error = $_.Exception.Message
        detail = ($_ | Out-String)
    } | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $resultPath -Encoding UTF8
    exit 1
}
