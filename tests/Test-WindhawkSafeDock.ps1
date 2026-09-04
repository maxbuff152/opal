[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$module = Join-Path $root 'WindhawkSafeDock.Core.psm1'
$failures = [Collections.Generic.List[string]]::new()
function Assert-True([bool]$Condition, [string]$Message) { if (-not $Condition) { $script:failures.Add($Message) } }

$paths = @(
    $module,
    (Join-Path $root 'Ensure-WindhawkSafeDock.ps1'),
    (Join-Path $root 'Build-OpalSuite.ps1'),
    (Join-Path $root 'Install-OpalSuite.ps1'),
    (Join-Path $root 'Test-OpalSuite.ps1')
)
foreach ($path in $paths) {
    $tokens = $null; $errors = $null
    [void][Management.Automation.Language.Parser]::ParseFile($path, [ref]$tokens, [ref]$errors)
    Assert-True ($errors.Count -eq 0) "Parse errors in $path : $($errors -join '; ')"
}

Import-Module $module -Force
$safeDockModule = Get-Module | Where-Object Path -eq $module | Select-Object -First 1
$config = Get-WindhawkSafeDockConfig
$expectedIds = @('local@opal')
Assert-True (@($config.StableMods).Count -eq 1) 'Exactly one unified Opal mod is required.'
Assert-True (-not (Compare-Object $expectedIds @($config.StableMods.Id))) 'Safe-dock owner is not local@opal.'
Assert-True (($config.StableMods | Select-Object -First 1).Include -match 'StartMenuExperienceHost') 'Unified Opal must own the shared shell hosts.'
Assert-True ((@($config.EvidenceRetryDelaysSeconds) -join ',') -eq '2,3,5,8,13') 'WER retry backoff changed.'
Assert-True (-not (& $safeDockModule { Test-WindhawkValueEqual ([string][char]0x2009) ([string][char]0x2002) })) 'Unicode settings equality must stay ordinal.'

$nl = [Environment]::NewLine
$werEvent = 'Faulting application name: Explorer.EXE' + $nl + 'Faulting module name: Windows.UI.Xaml.dll' + $nl + 'Exception code: 0xc000027b'
$opalShellLibrary = [string]($config.StableMods |
    Where-Object Id -eq 'local@opal' |
    Select-Object -First 1 -ExpandProperty Library)
$goodWer = 'LoadedModule[50]=C:\ProgramData\Windhawk\Engine\Mods\64\' + $opalShellLibrary
$badWer = 'LoadedModule[50]=C:\Windows\System32\Taskbar.dll'
Assert-True (Test-WindhawkCrashEvidence $werEvent $goodWer) 'Qualified Explorer/XAML/Opal evidence must trip.'
Assert-True (-not (Test-WindhawkCrashEvidence $werEvent $badWer)) 'Explorer XAML without an Opal DLL must not trip.'

$installerText = Get-Content (Join-Path $root 'Install-WindhawkSafeDock.ps1') -Raw
Assert-True ($installerText -notmatch 'RepetitionInterval|PT1M') 'Safe installer must not register a repeating repair loop.'

$opalTest = & (Join-Path $root 'Test-OpalSuite.ps1')
Assert-True ([bool]$opalTest.passed) ('Opal suite failed: ' + ($opalTest.failures -join ' | '))
$state = Test-WindhawkSafeDockState -RequireLoaded
Assert-True $state.Healthy ('Live safe-dock state unhealthy: ' + ($state.Drift -join ' | '))
$dry = Invoke-WindhawkSafeDockRepair -DryRun -AllowExplorerRestart
Assert-True (-not $dry.Changed) ('Healthy repair dry-run must be idempotent: ' + ($dry.Actions -join ' | '))

if ($failures.Count) {
    [pscustomobject]@{ passed=$false; failures=@($failures); checkedAt=[DateTimeOffset]::Now } | ConvertTo-Json -Depth 6
    exit 1
}
[pscustomobject]@{
    passed=$true; assertions=12; liveMode=$state.Mode; explorerPid=$state.ExplorerPid
    loadedModules=$state.LoadedModules; checkedAt=[DateTimeOffset]::Now
} | ConvertTo-Json -Depth 6
