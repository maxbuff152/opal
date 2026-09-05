[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$out=Join-Path $root 'build\latency-provenance-test'
New-Item -ItemType Directory -Path $out -Force|Out-Null
$cpp=Join-Path $out 'export.cpp';$exe=Join-Path $out 'export.exe'
$code=@'
#include "opal-performance-diagnostics.h"
#include <cstdio>
int main(){
 OpalPerformanceDiagnostics::SetEnabled(true);
 auto start=OpalPerformanceDiagnostics::Begin();
 Sleep(200);
 OpalPerformanceDiagnostics::Record(OpalPerformanceDiagnostics::Metric::HardwareOpen,start);
 auto report=OpalPerformanceDiagnostics::Summary();
 std::wprintf(L"%ls",report.c_str());
 return 0;
}
'@
[IO.File]::WriteAllText($cpp,$code)
& 'C:\Program Files\Windhawk\Compiler\bin\clang++.exe' -std=c++20 -target x86_64-w64-mingw32 -static '-I' (Join-Path $root 'mod\visual-clones') $cpp -o $exe
if($LASTEXITCODE){throw 'Latency provenance producer compilation failed.'}
$process=[Diagnostics.Process]::new()
$process.StartInfo=[Diagnostics.ProcessStartInfo]::new($exe)
$process.StartInfo.UseShellExecute=$false;$process.StartInfo.CreateNoWindow=$true;$process.StartInfo.RedirectStandardOutput=$true
$before=[datetime]::UtcNow
$null=$process.Start();$actualPid=$process.Id;$actualStart=$process.StartTime.ToUniversalTime()
$text=$process.StandardOutput.ReadToEnd();$process.WaitForExit();$after=[datetime]::UtcNow
if($process.ExitCode){throw 'Actual provenance producer failed.'}
$process.Dispose()
$analyzer=Join-Path $root 'Get-OpalLatencyAnalysis.ps1'
$checks=0
function Check($condition,$label){$script:checks++;if(-not $condition){throw "Provenance assertion: $label"}}
function Analyze($value){& $analyzer -ExportText $value -MinimumSamples 1}
$r=Analyze $text
Check ($r.EvidenceValid -and $null -ne $r.Provenance) 'actual production header exports parsed provenance'
Check ($r.Provenance.Pid -eq $actualPid) 'PID matches launched producer'
Check ([datetime]::Parse($r.Provenance.ProcessStartUtc).ToUniversalTime() -eq $actualStart) 'process lifetime matches OS process readback'
$captured=[datetime]::Parse($r.Provenance.CapturedAtUtc).ToUniversalTime()
Check ($captured -ge $before -and $captured -le $after) 'capture time inside actual execution'
Check ($r.Provenance.Epoch -eq 2) 'enabled collection epoch'
Check (-not $r.RuntimeVerified -and -not $r.Provenance.VerifiedAgainstRuntime -and $r.Verdict -eq 'NotEvaluated') 'metadata alone cannot certify installed runtime or invented budget'
Check ($r.Metrics[5].Count -eq 1 -and $r.Metrics[5].MedianMs -gt 0) 'actual collector numeric sample retained'
$line=($text -split "`r?`n")[0]
$legacy=$text.Substring($line.Length).TrimStart("`r","`n")
$old=Analyze $legacy
Check ($old.EvidenceValid -and $null -eq $old.Provenance) 'legacy summaries remain explicitly unverified'
foreach($bad in @(
 $text.Replace($line,"$line`r`n$line"),
 ($legacy+"`r`n"+$line),
 $text.Replace('version=1','version=9'),
 [regex]::Replace($text,'pid=[0-9]+','pid=0'),
 [regex]::Replace($text,'epoch=[0-9]+','epoch=0'),
 [regex]::Replace($text,'capturedFileTime=[0-9]+','capturedFileTime=1'),
 [regex]::Replace($text,'processStartFileTime=[0-9]+','processStartFileTime=99999999999999999999')
)) {Check (-not (Analyze $bad).EvidenceValid) 'malformed, duplicate, misplaced or impossible provenance rejected'}
[pscustomobject]@{passed=$true;cases=$checks;scope='Actual production Summary and parser with independently observed process PID/birth/capture window; malformed/legacy evidence. Numerical sample is an explicit test fixture, not live taskbar latency.'}
