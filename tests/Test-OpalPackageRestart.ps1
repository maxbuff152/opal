[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$sourcePath = Join-Path (Split-Path $PSScriptRoot -Parent) 'Measure-OpalPackageCost.ps1'
$tokens = $null
$parseErrors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile($sourcePath, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count) { throw ($parseErrors | Out-String) }
foreach ($name in @('Restart-PackageExplorer', 'Get-VerifiedPackageExplorer', 'Set-Scenario', 'Measure-ExplorerSample')) {
    $definition = @($ast.FindAll({ param($node)
        $node -is [Management.Automation.Language.FunctionDefinitionAst]
    }, $true) | Where-Object Name -eq $name)
    if ($definition.Count -ne 1) { throw "Expected one production function: $name" }
    . ([scriptblock]::Create($definition[0].Extent.Text))
}
$outerTry = @($ast.EndBlock.Statements | Where-Object { $_ -is [Management.Automation.Language.TryStatementAst] })
if ($outerTry.Count -ne 1 -or -not $outerTry[0].Finally) { throw 'Expected production restoration block.' }
$restore = [scriptblock]::Create($outerTry[0].Finally.Extent.Text.Trim().Substring(1).TrimEnd().TrimEnd('}'))

# Every operating-system boundary below is mocked. Only extracted production
# functions and the actual finally body execute; this test never restarts Explorer.
$sessionId = 7
$SettleSeconds = 6
$SampleSeconds = 3
$modPath = 'test-mod'
$settingsPath = 'test-settings'
$build = [pscustomobject]@{dllName='test-opal.dll'}
$originalState = [pscustomobject]@{Disabled=0;Media=1;Performance=1}
$script:assertions = 0
function Assert($Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
    $script:assertions++
}
function Assert-Throws([scriptblock]$Action, [string]$Pattern) {
    $caught = $null
    try { & $Action | Out-Null } catch { $caught = $_.Exception.Message }
    Assert ($caught -and $caught -like $Pattern) "Expected failure '$Pattern', got '$caught'"
}
function New-TestProcess([int]$Identity, [int]$Session = 7, [int]$Birth = 0) {
    $process = [pscustomobject]@{Id=$Identity;SessionId=$Session;StartTime=([datetime]'2026-01-01').AddSeconds($Birth);Modules=@();HandleCount=10;Threads=@(1)}
    $process | Add-Member ScriptMethod Refresh {}
    return $process
}
function Reset-Case([scriptblock]$Timeline) {
    $script:milliseconds = 0
    $script:processReads = 0
    $script:launchTimes = @()
    $script:stoppedIds = @()
    $script:markerClears = 0
    $script:registryWrites = 0
    $script:verifiedExplorer = $null
    $script:timeline = $Timeline
}
function Get-Date { ([datetime]'2026-01-01').AddMilliseconds($script:milliseconds) }
function Start-Sleep([int]$Milliseconds, [int]$Seconds) { $script:milliseconds += $Milliseconds + 1000 * $Seconds }
function Get-Process([string]$Name, [int]$Id, [string]$ErrorAction) {
    if ($Name -ne 'explorer' -or $Id) { throw 'Unexpected process lookup in test.' }
    $script:processReads++
    if ($script:processReads -eq 1) { New-TestProcess 10; New-TestProcess 90 8; return }
    & $script:timeline
}
function Stop-Process {
    [CmdletBinding()]param([Parameter(ValueFromPipeline)]$InputObject, [switch]$Force)
    process { if ($null -ne $InputObject) { $script:stoppedIds += $InputObject.Id } }
}
function Start-Process([string]$FilePath, [string]$WindowStyle) {
    if ($FilePath -ne "$env:WINDIR\explorer.exe" -or $WindowStyle -ne 'Hidden') { throw 'Unexpected explicit launch.' }
    $script:launchTimes += $script:milliseconds
}
function Clear-IntentionalRestartMarkers { $script:markerClears++ }
function Set-ItemProperty { $script:registryWrites++ }
function Get-GuiResourceCount { 1 }
function Get-OpalMeasuredProcessSnapshot { [pscustomobject]@{Pid=20} }
function Compare-OpalMeasuredProcessSnapshot {
    [pscustomobject]@{Processes=@([pscustomobject]@{Pid=20;CpuSeconds=0;PrivateMB=1;WorkingSetMB=2})}
}

Reset-Case { New-TestProcess 20 }
$script:verifiedExplorer = Restart-PackageExplorer
Assert ($script:verifiedExplorer.Id -eq 20) 'Automatic replacement was not accepted.'
Assert ($script:launchTimes.Count -eq 0) 'Automatic recovery caused an explicit launch.'
Assert ($script:stoppedIds.Count -eq 1 -and $script:stoppedIds[0] -eq 10) 'Restart crossed session boundaries.'
Assert ($script:markerClears -eq 1) 'Intentional restart marker was not cleared once.'

Reset-Case { if ($script:milliseconds -ge 4500) { New-TestProcess 20 } }
$replacement = Restart-PackageExplorer
Assert ($replacement.Id -eq 20 -and $script:milliseconds -eq 4500) 'Delayed automatic recovery failed.'
Assert ($script:launchTimes.Count -eq 0) 'Launch preceded the automatic-recovery grace period.'

Reset-Case { if ($script:launchTimes.Count -and $script:milliseconds -ge 6000) { New-TestProcess 20 } }
$replacement = Restart-PackageExplorer
Assert ($replacement.Id -eq 20) 'Explicit fallback did not find replacement.'
Assert ($script:launchTimes.Count -eq 1 -and $script:launchTimes[0] -eq 5000) 'Fallback must launch once at five seconds.'

Reset-Case {}
Assert-Throws { Restart-PackageExplorer } '*within 20 seconds*'
Assert ($script:milliseconds -eq 20000 -and $script:launchTimes.Count -eq 1) 'Missing replacement must time out without repeated launch.'
Assert ($null -eq $script:verifiedExplorer) 'Failed restart retained a verified process.'

Reset-Case { New-TestProcess 10 }
Assert-Throws { Restart-PackageExplorer } '*within 20 seconds*'
Assert ($script:launchTimes.Count -eq 0) 'Still-running old process caused another launch.'

Reset-Case { New-TestProcess 20; New-TestProcess 30 }
Assert-Throws { Restart-PackageExplorer } '*within 20 seconds*'
Assert ($script:launchTimes.Count -eq 0) 'Ambiguous replacement caused another launch.'

Reset-Case { if ($script:milliseconds -lt 2000) { New-TestProcess 10 }; New-TestProcess 20 }
$replacement = Restart-PackageExplorer
Assert ($replacement.Id -eq 20 -and $script:milliseconds -eq 2000) 'Transient old/new overlap was accepted before resolving.'
Assert ($script:launchTimes.Count -eq 0) 'Transient overlap caused another launch.'

Reset-Case { New-TestProcess 20 }
Set-Scenario -Configuration ([pscustomobject]@{Enabled=$false;Media=0;Performance=0})
Assert ($script:verifiedExplorer.Id -eq 20 -and $script:milliseconds -eq 6500) 'Scenario lost verified PID or changed settling duration.'
Assert ($script:registryWrites -eq 3 -and $script:markerClears -eq 1) 'Scenario setup semantics changed.'
$sample = Measure-ExplorerSample
Assert ($sample.Pid -eq 20 -and $sample.RuntimeVerified -and $script:milliseconds -eq 9500) 'Sampling failed to preserve verified identity or duration.'

$script:timeline = { New-TestProcess 20; New-TestProcess 30 }
Assert-Throws { Measure-ExplorerSample } '*ambiguous*'
$script:timeline = { New-TestProcess 30 }
Assert-Throws { Measure-ExplorerSample } '*ambiguous*'
$script:timeline = { New-TestProcess 20 7 1 }
Assert-Throws { Measure-ExplorerSample } '*ambiguous*'

Reset-Case { if ($script:milliseconds -lt 3000) { New-TestProcess 20 } else { New-TestProcess 30 } }
$script:verifiedExplorer = Restart-PackageExplorer
Assert-Throws { Measure-ExplorerSample } '*ambiguous*'

Reset-Case { New-TestProcess 20 }
. $restore
Assert ($script:verifiedExplorer.Id -eq 20 -and $script:markerClears -eq 1) 'Restoration did not use verified restart.'
Assert ($script:registryWrites -eq 3 -and $script:launchTimes.Count -eq 0) 'Restoration changed settings semantics or forced a launch.'

# Model System.Diagnostics.Process's lazy StartTime lookup: the same candidate
# object would observe a reused PID's new birth if its first read were deferred.
$script:lazyBirth = [datetime]'2026-01-01'
$script:birthReads = 0
$script:lazyCandidate = New-TestProcess 20
$script:lazyCandidate.PSObject.Properties.Remove('StartTime')
$script:lazyCandidate | Add-Member ScriptProperty StartTime {
    $script:birthReads++
    return $script:lazyBirth
}
Reset-Case { $script:lazyCandidate }
$script:verifiedExplorer = Restart-PackageExplorer
Assert ($script:birthReads -eq 1) 'Restart deferred reading the accepted process birth.'
$script:lazyBirth = $script:lazyBirth.AddSeconds(1)
Assert ($script:verifiedExplorer.StartTime -eq [datetime]'2026-01-01') 'Accepted birth changed after simulated PID reuse.'
Assert-Throws { Get-VerifiedPackageExplorer } '*ambiguous*'

[pscustomobject]@{Passed=$true;Assertions=$script:assertions;ProductionSource=$sourcePath;LiveRestartPerformed=$false}
