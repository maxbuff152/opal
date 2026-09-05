[CmdletBinding()]
param([Parameter(Mandatory)][string]$TracePath)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'tools\OpalAllocationTrace\OpalAllocationTrace.csproj'
$trace = (Resolve-Path -LiteralPath $TracePath).Path
$before = (Get-FileHash -LiteralPath $trace -Algorithm SHA256).Hash
& dotnet build $project -c Release --nologo -p:RestoreLockedMode=true
if ($LASTEXITCODE -ne 0) { throw 'Allocation analyzer build failed.' }
$dll = Join-Path $root 'tools\OpalAllocationTrace\bin\Release\net10.0\OpalAllocationTrace.dll'
$out = Join-Path $root ('build\allocation-profile-test\' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $out -Force | Out-Null
$allPath = Join-Path $out 'explorer.summary.json'
& dotnet $dll $trace $allPath explorer.exe
if ($LASTEXITCODE -ne 0) { throw 'Existing ETL analysis failed.' }
$report = Get-Content -LiteralPath $allPath -Raw | ConvertFrom-Json
$checks = 0
function Check([bool]$Value, [string]$Label) {
    $script:checks++
    if (!$Value) { throw "Allocation assertion $script:checks`: $Label" }
}
function Sum($Rows, [string]$Property) { ($Rows | Measure-Object -Property $Property -Sum).Sum }
Check ($report.InputSha256 -eq $before) 'source fingerprint'
Check ($report.MatchedEvents -gt 0 -and $report.EventsWithStacks -gt 0) 'fixture has allocation events and stacks'
Check (($report.EventsWithStacks + $report.EventsWithoutStacks) -eq $report.MatchedEvents) 'stack coverage partitions events'
Check ((Sum $report.Totals Count) -eq $report.MatchedEvents) 'event totals reconcile'
Check ((Sum $report.ModuleStackGroups.Totals Count) -eq $report.MatchedEvents) 'all module stack groups reconcile'
Check ((Sum $report.ModuleStackGroups.Totals GrossEventBytes) -eq (Sum $report.Totals GrossEventBytes)) 'group bytes reconcile to event bytes'
$allocations = @($report.Totals | Where-Object Kind -eq Allocation)
Check ((Sum $allocations Count) -eq $report.AllocationEvents) 'allocation count excludes deallocation'
Check ((Sum $allocations GrossEventBytes) -eq $report.GrossAllocationEventBytes) 'gross allocation bytes exclude deallocation'
Check ($report.AllocationEventsBelowLargeThreshold -gt 0) 'sub-8MiB fixture activity included'
Check (@($report.LargeEvents | Where-Object { $_.Bytes -lt $report.LargeEventThresholdBytes }).Count -eq 0) 'large events honor threshold'
Check (@($report.ModuleStackGroups | Where-Object { $_.Modules.Count -gt 0 }).Count -eq $report.ModuleStackGroups.Count) 'missing stacks explicit'
Check ($null -ne $report.TraceEventsLost -and $null -ne $report.ConversionTruncated) 'loss/coverage fields present'
Check (!$report.ConversionTruncated) 'fixture conversion complete'
$selected = $report.Totals[0].ProcessId
$pidPath = Join-Path $out 'pid.summary.json'
& dotnet $dll $trace $pidPath "pid:$selected"
if ($LASTEXITCODE -ne 0) { throw 'PID-filtered analysis failed.' }
$pidReport = Get-Content -LiteralPath $pidPath -Raw | ConvertFrom-Json
Check ($pidReport.MatchedEvents -eq (Sum @($report.Totals | Where-Object ProcessId -eq $selected) Count)) 'independent PID run reconciles named-process totals'
Check (@($pidReport.Totals | Where-Object ProcessId -ne $selected).Count -eq 0) 'PID selection excludes other processes'
$outputHash = (Get-FileHash -LiteralPath $allPath -Algorithm SHA256).Hash
& dotnet $dll $trace $allPath explorer 2>&1 | Out-Null
Check ($LASTEXITCODE -ne 0 -and (Get-FileHash -LiteralPath $allPath -Algorithm SHA256).Hash -eq $outputHash) 'existing evidence cannot be overwritten'
& dotnet $dll '\\invalid-server\share\private.etl' (Join-Path $out 'network.json') 2>&1 | Out-Null
Check ($LASTEXITCODE -ne 0 -and !(Test-Path -LiteralPath (Join-Path $out 'network.json'))) 'network input rejected before analysis'
Check ((Get-FileHash -LiteralPath $trace -Algorithm SHA256).Hash -eq $before) 'source ETL unchanged'
[pscustomobject]@{passed=$true;assertions=$checks;report=$allPath;events=$report.MatchedEvents;stacked=$report.EventsWithStacks;lost=$report.TraceEventsLost;evidence='Read-only existing ETL, all-size aggregation reconciliation, independent PID filter, evidence preservation, local-only input.'}
