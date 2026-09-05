$ErrorActionPreference='Stop'
$out=$PSScriptRoot
$before=[IO.File]::ReadAllText('D:\Opal\build\pdh-demand-test\before-performance.cpp')
$after=[IO.File]::ReadAllText('D:\Opal\mod\visual-clones\maxwell-taskbar-system-info.wh.cpp')
function Extract($s,$a,$b){$start=$s.IndexOf($a);$end=$s.IndexOf($b,$start);if($start -lt 0 -or $end -lt 0){throw 'Missing production snippet'};$s.Substring($start,$end-$start)}
$prefix=@'
#include <windows.h>
#include <pdh.h>
#include <psapi.h>
#include <chrono>
#include <cstdio>
#include <cstdint>
int opens=0,adds=0,addFailures=0,collects=0,collectFailures=0;
unsigned long lastAddStatus=0,lastCollectStatus=0;
PDH_STATUS ProbeOpen(LPCWSTR s,DWORD_PTR n,PDH_HQUERY* q){++opens;return PdhOpenQueryW(s,n,q);}
PDH_STATUS ProbeAdd(PDH_HQUERY q,LPCWSTR p,DWORD_PTR n,PDH_HCOUNTER* c){++adds;auto s=PdhAddEnglishCounterW(q,p,n,c);if(s){++addFailures;lastAddStatus=s;}return s;}
PDH_STATUS ProbeCollect(PDH_HQUERY q){++collects;auto s=PdhCollectQueryData(q);if(s){++collectFailures;lastCollectStatus=s;}return s;}
#define PdhOpenQueryW ProbeOpen
#define PdhAddEnglishCounterW ProbeAdd
#define PdhCollectQueryData ProbeCollect
#define Wh_Log(...) ((void)0)
enum class TemperatureSource {Auto,WindowsNative,Disabled};
struct ModSettings{TemperatureSource temperatureSource=TemperatureSource::Auto;};
PDH_HQUERY g_pdhQuery=nullptr;
HANDLE g_pdhCompletionEvent=nullptr;
PDH_HCOUNTER g_gpuCounter=nullptr,g_vramCounter=nullptr,g_sharedVramCounter=nullptr,g_thermalZoneCounter=nullptr;
std::chrono::steady_clock::time_point g_nextPdhCounterRetry{};
constexpr auto kPdhCounterRetryInterval=std::chrono::seconds(30);
struct Sample {size_t privateBytes,workingSet;DWORD handles;uint64_t cpu;};
uint64_t Ft(FILETIME f){ULARGE_INTEGER x{};x.LowPart=f.dwLowDateTime;x.HighPart=f.dwHighDateTime;return x.QuadPart;}
Sample Take(){PROCESS_MEMORY_COUNTERS_EX m{};m.cb=sizeof(m);GetProcessMemoryInfo(GetCurrentProcess(),(PROCESS_MEMORY_COUNTERS*)&m,sizeof(m));DWORD h=0;GetProcessHandleCount(GetCurrentProcess(),&h);FILETIME a{},b{},k{},u{};GetProcessTimes(GetCurrentProcess(),&a,&b,&k,&u);return {m.PrivateUsage,m.WorkingSetSize,h,Ft(k)+Ft(u)};}
'@
$suffix=@'
int main(){
 auto baseline=Take();auto t0=std::chrono::steady_clock::now();
 ModSettings settings;
 CALL_SETUP
 if(g_pdhQuery)PdhCollectQueryData(g_pdhQuery);
 auto active=Take();int counters=(g_gpuCounter?1:0)+(g_vramCounter?1:0)+(g_sharedVramCounter?1:0)+(g_thermalZoneCounter?1:0);
 ClosePdhQuery();auto end=Take();double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count();
 printf("{\"wallMs\":%.4f,\"cpuMs\":%.4f,\"baselinePrivate\":%llu,\"activePrivateDelta\":%lld,\"endPrivateDelta\":%lld,\"activeWorkingSetDelta\":%lld,\"endWorkingSetDelta\":%lld,\"activeHandleDelta\":%ld,\"endHandleDelta\":%ld,\"counters\":%d,\"opens\":%d,\"adds\":%d,\"addFailures\":%d,\"lastAddStatus\":%lu,\"collects\":%d,\"collectFailures\":%d,\"lastCollectStatus\":%lu}\n",ms,(end.cpu-baseline.cpu)/10000.0,(unsigned long long)baseline.privateBytes,(long long)active.privateBytes-baseline.privateBytes,(long long)end.privateBytes-baseline.privateBytes,(long long)active.workingSet-baseline.workingSet,(long long)end.workingSet-baseline.workingSet,(long)active.handles-(long)baseline.handles,(long)end.handles-(long)baseline.handles,counters,opens,adds,addFailures,lastAddStatus,collects,collectFailures,lastCollectStatus);
}
'@
$variants=@(
 @{name='before-auto';src=$before;call='EnsurePdhQuery(settings);'},
 @{name='after-thermal';src=$after;call='EnsurePdhQuery(settings,PdhQueryDemand::ThermalOnly);'},
 @{name='before-disabled';src=$before;call='settings.temperatureSource=TemperatureSource::Disabled;EnsurePdhQuery(settings);'},
 @{name='after-disabled';src=$after;call='settings.temperatureSource=TemperatureSource::Disabled;EnsurePdhQuery(settings,PdhQueryDemand::ThermalOnly);'}
)
foreach($v in $variants){
 $close=Extract $v.src 'void ClosePdhQuery() {' 'void RecreatePdhSources('
 $query=Extract $v.src 'bool AddPdhCounter(' 'PDH_STATUS ReadPdhArray('
 $cpp=Join-Path $out ($v.name+'.cpp');$exe=Join-Path $out ($v.name+'.exe')
 [IO.File]::WriteAllText($cpp,$prefix+"`n"+$close+"`n"+$query+"`n"+$suffix.Replace('CALL_SETUP',$v.call))
 & 'C:\Program Files\Windhawk\Compiler\bin\clang++.exe' -std=c++20 -O2 -target x86_64-w64-mingw32 -static $cpp -lpdh -lpsapi -o $exe
 if($LASTEXITCODE -ne 0){throw "Compilation failed: $($v.name)"}
}
$results=@()
$order=@('before-auto','after-thermal','after-disabled','before-disabled','before-disabled','after-disabled','after-thermal','before-auto')
$start=Get-Date
foreach($name in $order){
 if(((Get-Date)-$start).TotalSeconds -gt 45){break}
 $stdout=Join-Path $out ($name+'.out.json');$stderr=Join-Path $out ($name+'.err.txt')
 $p=Start-Process -FilePath (Join-Path $out ($name+'.exe')) -PassThru -WindowStyle Hidden -RedirectStandardOutput $stdout -RedirectStandardError $stderr
 if(-not $p.WaitForExit(6000)){$p.Kill();$results+= [pscustomobject]@{variant=$name;timeout=$true};continue}
 $p.WaitForExit();$r=Get-Content -LiteralPath $stdout -Raw|ConvertFrom-Json;$r|Add-Member variant $name;$r|Add-Member exitCode $p.ExitCode;$results+=$r
}
$results|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $out 'results.json')
$results|Select-Object variant,wallMs,cpuMs,activePrivateDelta,endPrivateDelta,counters,adds,addFailures,collectFailures|Format-Table -AutoSize
