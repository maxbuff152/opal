[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$modulePath = Join-Path (Split-Path -Parent $PSScriptRoot) 'WindhawkSafeDock.Core.psm1'
Import-Module $modulePath -Force
$module = Get-Module | Where-Object Path -eq $modulePath | Select-Object -First 1
$expected = Join-Path ([Environment]::GetFolderPath('CommonApplicationData')) 'Windhawk\Opal\safedock'
if ((Get-WindhawkSafeDockConfig).StateRoot -ne $expected) { throw 'Safety state is not machine-wide.' }

# Run the real repair decision logic against isolated, in-memory owning-system
# doubles. No live registry values, services, processes, or latch files change.
& $module {
    function Get-WindhawkSafeDockConfig {
        [pscustomobject]@{
            StableMods = @([pscustomobject]@{ Id='local@opal'; Library='opal.dll'; CanonicalDll='dll'; CanonicalSource='source'; DllSha256='hash'; SourceSha256='hash'; Version='4.4.0'; Include='explorer.exe'; Settings=@{} })
            RetiredTaskNames=@(); UnsafeIds=@(); EngineModRoot='C:\test'; SourceRoot='C:\test'
        }
    }
    function Test-WindhawkSafeDockState {
        param([switch]$RequireLoaded)
        if (-not $RequireLoaded) { throw 'Repair accepted configuration without checking loaded DLLs.' }
        [pscustomobject]@{Healthy=$script:scenario -ne 'missing'; CircuitBreakerLatched=$script:scenario -eq 'latched'; LoadedModules=@(); Drift=@()}
    }
    function Get-WindhawkFileHash { param($Path) 'hash' }
    function Test-Path { param($LiteralPath) $true }
    function Set-WindhawkRegistryValueIfDifferent { param($Path,$Name,$Value,$Actions,[switch]$DryRun) $false }
    function Get-ChildItem { param($LiteralPath,$ErrorAction) @() }
    function Get-Service { param($Name,$ErrorAction) [pscustomobject]@{StartType='Automatic';Status='Running'} }
    function Restart-WindhawkExplorerOnce { param([switch]$DryRun) if (-not $DryRun) { throw 'Test must never restart Explorer.' }; $script:restartCalls++ }
    foreach ($scenario in @('healthy','missing','latched')) {
        $script:scenario = $scenario
        $script:restartCalls = 0
        $result = Invoke-WindhawkSafeDockRepair -DryRun -AllowExplorerRestart
        $expectedCalls = if ($scenario -eq 'missing') { 1 } else { 0 }
        if ($script:restartCalls -ne $expectedCalls) { throw "Wrong restart decision for $scenario" }
        if ($result.Changed -ne ($scenario -eq 'missing')) { throw "Wrong change decision for $scenario" }
    }
}
Remove-Module $module
Import-Module $modulePath -Force
[pscustomobject]@{passed=$true; scenarios=@('shared machine state','healthy idempotence','enabled but unloaded recovery','latched stock fallback')}
