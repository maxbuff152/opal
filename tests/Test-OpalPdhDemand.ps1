[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$source = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\maxwell-taskbar-system-info.wh.cpp'))
function Extract([string]$Start, [string]$End) {
    $a = $source.IndexOf($Start)
    $b = $source.IndexOf($End, $a)
    if ($a -lt 0 -or $b -lt 0) { throw "Missing production boundary: $Start" }
    $source.Substring($a, $b - $a)
}
$close = Extract 'void ClosePdhQuery()' 'void RecreatePdhSources('
$ensure = Extract 'bool AddPdhCounter(' 'PDH_STATUS ReadPdhArray('
$prefix = @'
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <chrono>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <stdexcept>
enum class TemperatureSource {Auto,WindowsNative,Disabled,SharedMemory,GadgetRegistry,HwInfoAuto};
struct ModSettings {TemperatureSource temperatureSource=TemperatureSource::Auto;};
PDH_HQUERY g_pdhQuery=nullptr;
PDH_HCOUNTER g_gpuCounter=nullptr,g_vramCounter=nullptr,g_sharedVramCounter=nullptr,g_thermalZoneCounter=nullptr;
HANDLE g_pdhCompletionEvent=nullptr;
std::chrono::steady_clock::time_point g_nextPdhCounterRetry{};
constexpr auto kPdhCounterRetryInterval=std::chrono::seconds(30);
int opens=0,closes=0,adds=0,collects=0,eventsClosed=0,checks=0;
bool failOpen=false,failCollect=false;
std::wstring failPath;
std::vector<std::pair<PDH_HCOUNTER,std::wstring>> counters;
void Wh_Log(const wchar_t*,...){}
PDH_STATUS FakeOpen(LPCWSTR,DWORD_PTR,PDH_HQUERY* out){++opens;if(failOpen)return ERROR_ACCESS_DENIED;*out=reinterpret_cast<PDH_HQUERY>(1);return ERROR_SUCCESS;}
PDH_STATUS FakeClose(PDH_HQUERY){++closes;counters.clear();return ERROR_SUCCESS;}
BOOL FakeCloseHandle(HANDLE){++eventsClosed;return TRUE;}
PDH_STATUS FakeAdd(PDH_HQUERY,LPCWSTR path,DWORD_PTR,PDH_HCOUNTER* out){
 ++adds;if(!failPath.empty()&&std::wstring(path).find(failPath)!=std::wstring::npos)return PDH_CSTATUS_NO_COUNTER;
 *out=reinterpret_cast<PDH_HCOUNTER>(static_cast<intptr_t>(adds+10));counters.emplace_back(*out,path);return ERROR_SUCCESS;
}
PDH_STATUS FakeRemove(PDH_HCOUNTER h){counters.erase(std::remove_if(counters.begin(),counters.end(),[&](auto& c){return c.first==h;}),counters.end());return ERROR_SUCCESS;}
PDH_STATUS FakeCollect(PDH_HQUERY){++collects;return failCollect?PDH_NO_DATA:ERROR_SUCCESS;}
#define PdhOpenQueryW FakeOpen
#define PdhCloseQuery FakeClose
#define CloseHandle FakeCloseHandle
#define PdhAddEnglishCounterW FakeAdd
#define PdhRemoveCounter FakeRemove
#define PdhCollectQueryData FakeCollect
void Check(bool pass,const char* label){++checks;if(!pass){std::fprintf(stderr,"Assertion %d: %s\n",checks,label);throw std::runtime_error(label);}}
bool Has(const wchar_t* value){return std::any_of(counters.begin(),counters.end(),[&](auto& c){return c.second.find(value)!=std::wstring::npos;});}
'@
$main = @'
int main(){try{
 ModSettings settings;
 const auto thermal=PdhQueryDemand::ThermalOnly;
 EnsurePdhQuery(settings,thermal);
 Check(opens==1&&counters.size()==1&&Has(L"Thermal Zone")&&!Has(L"GPU"),"Auto thermal-only registers exactly thermal counter");
 settings.temperatureSource=TemperatureSource::WindowsNative;
 EnsurePdhQuery(settings,thermal);
 Check(opens==1&&adds==1&&collects==1,"native source reuses complete thermal query");
 for(auto source:{TemperatureSource::Disabled,TemperatureSource::SharedMemory,TemperatureSource::GadgetRegistry,TemperatureSource::HwInfoAuto}){
  settings.temperatureSource=source;EnsurePdhQuery(settings,thermal);
  Check(!g_pdhQuery&&counters.empty()&&opens==1,"non-Windows thermal source never opens PDH");
 }
 settings.temperatureSource=TemperatureSource::Auto;EnsurePdhQuery(settings);
 Check(counters.size()==4&&Has(L"GPU Engine")&&Has(L"Dedicated Usage")&&Has(L"Shared Usage")&&Has(L"Thermal Zone"),"full fallback preserves all four counters");
 settings.temperatureSource=TemperatureSource::Disabled;EnsurePdhQuery(settings);
 Check(counters.size()==3&&!g_thermalZoneCounter,"full fallback removes unneeded thermal counter");
 settings.temperatureSource=TemperatureSource::Auto;
 g_pdhCompletionEvent=reinterpret_cast<HANDLE>(2);int beforeClose=closes;
 EnsurePdhQuery(settings,thermal);
 Check(closes==beforeClose+1&&eventsClosed==1&&counters.size()==1&&!g_gpuCounter&&!g_vramCounter&&!g_sharedVramCounter,"full-to-thermal closes full query and completion event");
 ClosePdhQuery();failPath=L"Thermal Zone";EnsurePdhQuery(settings,thermal);
 Check(!g_pdhQuery&&g_nextPdhCounterRetry>std::chrono::steady_clock::now(),"failed thermal registration closes query and sets retry");
 int beforeOpen=opens;failPath.clear();EnsurePdhQuery(settings,thermal);
 Check(opens==beforeOpen,"same-demand failed registration respects retry deadline");
 g_nextPdhCounterRetry=std::chrono::steady_clock::now()-std::chrono::milliseconds(1);
 EnsurePdhQuery(settings,thermal);
 Check(g_pdhQuery&&counters.size()==1,"expired thermal retry recovers");
 ClosePdhQuery();g_nextPdhCounterRetry={};failOpen=true;EnsurePdhQuery(settings,thermal);
 Check(!g_pdhQuery&&g_nextPdhCounterRetry>std::chrono::steady_clock::now(),"open failure schedules retry");
 beforeOpen=opens;failOpen=false;EnsurePdhQuery(settings,thermal);
 Check(opens==beforeOpen,"same-demand open failure respects retry deadline");
 EnsurePdhQuery(settings);
 Check(opens==beforeOpen+1&&counters.size()==4,"thermal failure never delays full fallback transition");
 ClosePdhQuery();g_nextPdhCounterRetry={};failPath=L"GPU Engine";EnsurePdhQuery(settings);
 Check(g_pdhQuery&&counters.size()==3&&!g_gpuCounter,"partial full query retained during failed counter retry");
 int beforeAdd=adds;failPath.clear();EnsurePdhQuery(settings);
 Check(adds==beforeAdd,"partial full query respects retry deadline");
 g_nextPdhCounterRetry=std::chrono::steady_clock::now()-std::chrono::milliseconds(1);EnsurePdhQuery(settings);
 Check(counters.size()==4&&g_gpuCounter,"partial full query recovers missing counter after deadline");
 g_nextPdhCounterRetry=std::chrono::steady_clock::now()+std::chrono::seconds(30);
 EnsurePdhQuery(settings,thermal);
 Check(counters.size()==1&&g_nextPdhCounterRetry==std::chrono::steady_clock::time_point{},"full-to-thermal clears unrelated retry state");
 EnsurePdhQuery(settings);
 Check(counters.size()==4,"thermal-to-full restores all fallback counters");
 settings.temperatureSource=TemperatureSource::Disabled;EnsurePdhQuery(settings,thermal);EnsurePdhQuery(settings);
 Check(counters.size()==3&&!g_thermalZoneCounter,"no-query-to-full restores GPU fallback without temperature");
 ClosePdhQuery();g_nextPdhCounterRetry={};failCollect=true;EnsurePdhQuery(settings);
 Check(g_pdhQuery&&counters.size()==3,"collection error retains registered fallback query as before");
 std::printf("%d production PDH-demand assertions passed.\n",checks);return 0;
 }catch(...){return 1;}}
'@
$out = Join-Path $root 'build\pdh-demand-test'
New-Item -ItemType Directory -Path $out -Force | Out-Null
$cpp = Join-Path $out 'demand.cpp'
$exe = Join-Path $out 'demand.exe'
[IO.File]::WriteAllText($cpp, $prefix + "`n" + $close + "`n" + $ensure + "`n" + $main)
& 'C:\Program Files\Windhawk\Compiler\bin\clang++.exe' -std=c++20 -target x86_64-w64-mingw32 -static $cpp -o $exe
if ($LASTEXITCODE -ne 0) { throw 'PDH demand regression compilation failed.' }
& $exe
if ($LASTEXITCODE -ne 0) { throw 'PDH demand regression failed.' }
[pscustomobject]@{passed=$true;cases=22;evidence='Compiled production query setup/cleanup with deterministic PDH failures, exact counter sets, source transitions, and retry deadlines.'}
