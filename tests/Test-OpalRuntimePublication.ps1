[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$source=[IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\maxwell-taskbar-system-info.wh.cpp'))
$a=$source.IndexOf('    // Runtime publication cache:')
$b=$source.IndexOf('    // End runtime publication cache.',$a)
if($a -lt 0 -or $b -lt 0){throw 'Missing production publication cache.'}
$body=$source.Substring($a,$b-$a)
$controlStart=$source.IndexOf('void ApplyPerformanceControlChange(DWORD)')
$controlEnd=$source.IndexOf('}  // namespace',$controlStart)
$control=$source.Substring($controlStart,$controlEnd-$controlStart)
$prefix=@'
#include <windows.h>
#include <optional>
#include <string>
int writes=0;ULONGLONG ticks=0;
bool stopped=false,lastActive=false,lastSuspended=false,inactiveBeforeStop=false;
bool g_performanceEnabled=true;ULONGLONG g_lastRenderedMetricsSequence=0;
struct {bool quarantined=false;std::wstring reason;} g_quarantine;
int Wh_GetIntSetting(const wchar_t*){return 0;}
void LoadSettings(){} void StopMetricsWorker(){stopped=true;}
void TearDownTaskbarUi(){} void CloseMetricSources(){}
void StartMetricsWorker(){} void WakeMetricsWorker(){} void ApplyOnTaskbarThread(){}
namespace OpalControl {
const wchar_t* kPerformanceRuntimeActiveValue=L"Active";
const wchar_t* kPerformanceRuntimePidValue=L"Pid";
void ResetPackageQuarantine(const wchar_t*){}
auto BeginPackageSession(const wchar_t*){return g_quarantine;}
void PublishRuntimeState(const wchar_t*,const wchar_t*,bool active,bool suspended,const wchar_t*,bool){++writes;lastActive=active;lastSuspended=suspended;if(!active&&!stopped)inactiveBeforeStop=true;}
}
ULONGLONG FakeTick(){return ticks;}
#define GetTickCount64 FakeTick
'@
$suffix=@'
 publishWorkerRuntime(false,L"");if(writes!=1)return 1;
 for(int i=0;i<100;++i){ticks=100+i;publishWorkerRuntime(false,L"");}if(writes!=1)return 2;
 ticks=29999;publishWorkerRuntime(false,L"");if(writes!=1)return 3;
 ticks=30000;publishWorkerRuntime(false,L"");if(writes!=2)return 4;
 publishWorkerRuntime(true,L"fullscreen");if(writes!=3)return 5;
 publishWorkerRuntime(true,L"battery");if(writes!=4)return 6;
 g_quarantine.quarantined=true;publishWorkerRuntime(true,L"battery");if(writes!=5)return 7;
 publishWorkerRuntime(false,L"");if(writes!=6)return 8;
 g_quarantine.quarantined=false;publishWorkerRuntime(true,L"fullscreen");
 ApplyPerformanceControlChange(0);publishWorkerRuntime(true,L"fullscreen");
 if(writes!=7||!lastSuspended)return 9;
 g_performanceEnabled=false;ApplyPerformanceControlChange(0);
 if(writes!=8||lastActive||!stopped||inactiveBeforeStop)return 10;
 return 0;
}
'@
$out=Join-Path $root 'build\runtime-publication-test'
New-Item -ItemType Directory -Path $out -Force|Out-Null
$cpp=Join-Path $out 'publication.cpp';$exe=Join-Path $out 'publication.exe'
[IO.File]::WriteAllText($cpp,$prefix+"`n"+$control+"`nint main(){`n"+$body+"`n"+$suffix)
& 'C:\Program Files\Windhawk\Compiler\bin\clang++.exe' -std=c++20 -target x86_64-w64-mingw32 -static $cpp -o $exe
if($LASTEXITCODE -ne 0){throw 'Publication regression compilation failed.'}
& $exe
if($LASTEXITCODE -ne 0){throw "Publication regression failed: $LASTEXITCODE"}
[pscustomobject]@{passed=$true;cases=10;evidence='Compiled production cache and settings callback: unchanged suppression, refresh/transition writes, suspension survives settings changes, inactive status after worker drain.'}
