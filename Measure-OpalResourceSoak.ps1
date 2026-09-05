[CmdletBinding()]
param([ValidateRange(300,3600)][int]$Seconds=300,[ValidateRange(5,30)][int]$IntervalSeconds=15,[Parameter(Mandatory)][string]$OutputPath)
$ErrorActionPreference='Stop'
$build=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'build\opal-suite\build-receipt.json') -Raw|ConvertFrom-Json
$core=Join-Path $env:LOCALAPPDATA 'Maxwell\Shell\Core\Maxwell.Shell.Core.exe'
$identity=@{Version=$build.version;DllSha256=$build.sha256;CoreSha256=(Get-FileHash -LiteralPath $core).Hash}
Add-Type @'
using System;using System.Runtime.InteropServices;
public static class OpalSoakGui { [DllImport("user32.dll")] public static extern int GetGuiResources(IntPtr p,int kind); }
'@
$owned=@(Get-Process -Name explorer,Maxwell.Shell.Core,windhawk | Where-Object SessionId -eq (Get-Process -Id $PID).SessionId)
$explorer=@($owned|Where-Object ProcessName -eq explorer)
if($explorer.Count -ne 1){throw 'Exactly one interactive Explorer is required.'}
$mapped=@($explorer[0].Modules|Where-Object ModuleName -eq $build.dllName)
if($mapped.Count -ne 1 -or (Get-FileHash -LiteralPath $mapped[0].FileName).Hash -ne $build.sha256){throw 'Soak must measure the current installed build.'}
if((Get-FileHash -LiteralPath $core).Hash -ne (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'build\maxwell-shell-core\Maxwell.Shell.Core.exe')).Hash){throw 'Companion build mismatch.'}
$timer=[Diagnostics.Stopwatch]::StartNew();$samples=[Collections.Generic.List[object]]::new()
do {
 $rows=foreach($original in $owned){
  $p=Get-Process -Id $original.Id -ErrorAction Stop
  if($p.StartTime -ne $original.StartTime){throw 'Process identity changed during soak.'}
  $p.Refresh()
  [pscustomobject]@{Name=$p.ProcessName;Pid=$p.Id;CpuSeconds=$p.CPU;PrivateMB=$p.PrivateMemorySize64/1MB;WorkingSetMB=$p.WorkingSet64/1MB;Handles=$p.HandleCount;Gdi=[OpalSoakGui]::GetGuiResources($p.Handle,0);User=[OpalSoakGui]::GetGuiResources($p.Handle,1)}
 }
 $samples.Add([pscustomobject]@{ElapsedSeconds=$timer.Elapsed.TotalSeconds;Processes=@($rows)})
 if($timer.Elapsed.TotalSeconds -ge $Seconds){break}
 Start-Sleep -Seconds $IntervalSeconds
}while($true)
$result=@{SchemaVersion=1;CapturedAt=[DateTimeOffset]::Now.ToString('o');Build=$identity;LogicalProcessors=[Environment]::ProcessorCount;DurationSeconds=$timer.Elapsed.TotalSeconds;Samples=@($samples);Limitation='Observational same-process soak, not proof of indefinite leak freedom or stock-relative cost.'}
$result|ConvertTo-Json -Depth 8|Set-Content -LiteralPath $OutputPath
& (Join-Path $PSScriptRoot 'Test-OpalResourceSoak.ps1') -ReceiptPath $OutputPath
