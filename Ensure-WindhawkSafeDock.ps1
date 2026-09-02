[CmdletBinding()]
param(
    [ValidateSet('Check','Repair','CircuitBreak','ClearCircuitBreaker')][string]$Mode = 'Check',
    [switch]$RequireLoaded,
    [switch]$AllowExplorerRestart,
    [switch]$DryRun,
    [switch]$Quiet,
    [string]$EventMessage,
    [string]$WerText
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'WindhawkSafeDock.Core.psm1') -Force
try {
    switch ($Mode) {
        'Check' { $result = Test-WindhawkSafeDockState -RequireLoaded:$RequireLoaded; $exitCode = if ($result.Healthy) { 0 } else { 1 } }
        'Repair' { $result = Invoke-WindhawkSafeDockRepair -DryRun:$DryRun -AllowExplorerRestart:$AllowExplorerRestart; $exitCode = if ($DryRun -or $result.After.Healthy) { 0 } else { 1 } }
        'CircuitBreak' { $result = Invoke-WindhawkSafeDockCircuitBreak -EventMessage $EventMessage -WerText $WerText -DryRun:$DryRun; $exitCode = 0 }
        'ClearCircuitBreaker' { $result = Clear-WindhawkSafeDockCircuitBreaker -DryRun:$DryRun; $exitCode = 0 }
    }
    if (-not $Quiet) { $result | ConvertTo-Json -Depth 10 }
    exit $exitCode
} catch {
    if (-not $Quiet) { [pscustomobject]@{mode=$Mode;error=$_.Exception.Message;type=$_.Exception.GetType().FullName} | ConvertTo-Json -Depth 4 }
    exit 2
}
