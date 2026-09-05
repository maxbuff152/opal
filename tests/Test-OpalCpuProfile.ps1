[CmdletBinding()]
param([string]$TracePath)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'tools\OpalAllocationTrace\OpalAllocationTrace.csproj'
$profile = Join-Path $root 'config\OpalCpu.wprp'
[xml]$xml = Get-Content -LiteralPath $profile -Raw
$collector = $xml.WindowsPerformanceRecorder.Profiles.SystemCollector
if ([int]$collector.BufferSize.Value -ne 1024 -or [int]$collector.Buffers.Value -ne 64) { throw 'CPU trace must use 64 x 1024 KB buffers.' }
$keywords = @($xml.WindowsPerformanceRecorder.Profiles.SystemProvider.Keywords.Keyword.Value | Sort-Object)
if (($keywords -join ',') -ne 'Loader,ProcessThread,SampledProfile') { throw 'CPU trace keywords changed.' }
if ($xml.WindowsPerformanceRecorder.Profiles.SystemProvider.Stacks.Stack.Value -ne 'SampledProfile') { throw 'CPU stacks missing.' }
if ($xml.WindowsPerformanceRecorder.Profiles.Profile.LoggingMode -ne 'Memory') { throw 'CPU profile must be bounded memory mode.' }
# Metadata inspection only: this command never starts a recorder session.
$details = & wpr -profiledetails "$profile!OpalCpu"
if ($LASTEXITCODE -ne 0) { throw 'Installed WPR rejected CPU profile metadata.' }
& dotnet build $project -c Release --nologo -p:RestoreLockedMode=true
if ($LASTEXITCODE -ne 0) { throw 'CPU analyzer build failed.' }
$out = Join-Path $root ('build\cpu-profile-test\' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $out -Force | Out-Null
$details | Set-Content -LiteralPath (Join-Path $out 'wpr-profile-details.txt') -Encoding utf8
$testProject = @"
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup><OutputType>Exe</OutputType><TargetFramework>net10.0</TargetFramework><ImplicitUsings>enable</ImplicitUsings></PropertyGroup>
  <ItemGroup><ProjectReference Include="$([Security.SecurityElement]::Escape($project))" /></ItemGroup>
</Project>
"@
[IO.File]::WriteAllText((Join-Path $out 'Counts.csproj'), $testProject)
$fixture = @'
using System.Text.Json;
int checks=0;
void Check(bool condition,string message){checks++;if(!condition)throw new Exception(message);}
var a=new CpuSampleAggregator();
a.Add(1,10,"A",new[]{"A","A","B","A"},false,false,false,1);
a.Add(1,10,"a",new[]{"a","B","a"},false,false,false,1);
a.Add(1,11,"B",Array.Empty<string>(),false,false,false,2);
a.Add(1,12,null,new[]{"","C"},true,true,false,1);
a.Add(2,20,"Kernel",new[]{"Kernel","C"},false,false,true,1);
a.Add(2,21,"Kernel",Array.Empty<string>(),false,true,true,0);
a.Add(1,10,"A",new[]{"A","B","A"},true,false,false,1);
Check(a.MatchedEvents==7,"all samples counted");
Check(a.EventsWithStacks==5&&a.EventsWithoutStacks==2,"stack coverage exact");
Check(a.TruncatedStacks==2,"truncated stacks counted");
Check(a.UnknownLeafEvents==1,"unknown leaf stays unknown rather than taking caller");
Check(a.NonProcessEvents==3,"DPC/ISR contexts separated");
Check(a.ReportedCountTotal==7&&a.NonPositiveReportedCountEvents==1,"reported Count preserved separately from events");
Check(a.Exclusive.Values.Sum()==7,"exclusive counts partition samples");
Check(a.Exclusive[("Process","a")]==3&&a.Exclusive[("Process","b")]==1,"exclusive leaf attribution");
Check(a.Inclusive[("Process","a")]==3&&a.Inclusive[("Process","b")]==4,"inclusive deduplicates recursive/repeated modules");
Check(a.Inclusive.Values.Sum()>7,"inclusive rows explicitly overlap");
Check(a.Exclusive[("DPC","<unknown>")]==1&&!a.Exclusive.ContainsKey(("DPC","c")),"unresolved IP not attributed to known stack caller");
Check(a.Exclusive[("ISR","kernel")]==1&&a.Exclusive[("DPC+ISR","kernel")]==1,"distinct nonprocess flags retained");
Check(a.Threads.Values.Sum()==7&&a.Threads[(1,10,"Process")]==3,"thread counts reconcile");
Check(a.Groups.Values.Sum(x=>x.Count)==7,"stack groups partition samples");
Check(a.Groups.Values.Single(x=>x.ProcessId==1&&x.LeafModule=="a"&&!x.StackTruncated).Count==2,"equivalent module paths merge case-insensitively");
Check(a.Groups.Values.Any(x=>x.Modules.SequenceEqual(new[]{"a","b","a"})),"nonconsecutive recursion preserved");
Check(a.Groups.Values.Count(x=>x.Modules.SequenceEqual(new[]{"<no-stack>"}))==2,"missing stack stays explicit despite known leaf");
using var report=JsonDocument.Parse(JsonSerializer.Serialize(a.Summary()));
Check(report.RootElement.GetProperty("ExclusiveLeafModuleCounts").GetArrayLength()==a.Exclusive.Count,"actual output exposes exclusive counts");
Check(report.RootElement.GetProperty("ModuleStackGroups").GetArrayLength()==a.Groups.Count,"actual output exposes all grouped samples");
Console.WriteLine($"{checks} deterministic CPU aggregation assertions passed (synthetic counts; no live capture claim).");
'@
[IO.File]::WriteAllText((Join-Path $out 'Program.cs'), $fixture)
& dotnet run --project (Join-Path $out 'Counts.csproj') -c Release --nologo
if ($LASTEXITCODE -ne 0) { throw 'CPU aggregation fixture failed.' }
$liveReport = $null
if ($TracePath) {
    $trace = (Resolve-Path -LiteralPath $TracePath).Path
    $hash = (Get-FileHash -LiteralPath $trace -Algorithm SHA256).Hash
    $liveReport = Join-Path $out 'cpu.summary.json'
    & dotnet (Join-Path $root 'tools\OpalAllocationTrace\bin\Release\net10.0\OpalAllocationTrace.dll') --cpu $trace $liveReport explorer
    if ($LASTEXITCODE -ne 0) { throw 'Existing CPU ETL analysis failed.' }
    $r = Get-Content -LiteralPath $liveReport -Raw | ConvertFrom-Json
    if ($r.AnalysisMode -ne 'SampledCpu' -or $r.Summary.MatchedEvents -le 0) { throw 'Trace contains no matched Explorer CPU samples.' }
    if (($r.Summary.ExclusiveLeafModuleCounts | Measure-Object Count -Sum).Sum -ne $r.Summary.MatchedEvents) { throw 'CPU leaf totals do not reconcile.' }
    if (($r.Summary.ModuleStackGroups | Measure-Object Count -Sum).Sum -ne $r.Summary.MatchedEvents) { throw 'CPU stack totals do not reconcile.' }
    if ($r.Summary.EventsWithStacks + $r.Summary.EventsWithoutStacks -ne $r.Summary.MatchedEvents) { throw 'CPU stack coverage does not reconcile.' }
    if ($r.InputSha256 -ne $hash -or (Get-FileHash -LiteralPath $trace -Algorithm SHA256).Hash -ne $hash) { throw 'CPU trace fingerprint changed.' }
}
[pscustomobject]@{passed=$true;syntheticAssertions=19;profile='Installed WPR metadata validation only; 64 MiB collector buffers';existingCpuTraceReport=$liveReport;captureStarted=$false}
