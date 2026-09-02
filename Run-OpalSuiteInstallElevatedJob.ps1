[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$resultPath = Join-Path $PSScriptRoot 'build\opal-suite\elevated-install-result.json'
try {
    $output = & (Join-Path $PSScriptRoot 'Install-OpalSuite.ps1') 6>&1 | Out-String
    [pscustomobject]@{
        succeeded = $true
        completedUtc = (Get-Date).ToUniversalTime().ToString('o')
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
