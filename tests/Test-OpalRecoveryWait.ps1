[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$source = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\maxwell-shell.wh.cpp'))
function Extract([string]$Start, [string]$End) {
    $a = $source.IndexOf($Start); $b = $source.IndexOf($End, $a)
    if ($a -lt 0 -or $b -lt 0) { throw "Missing production boundary: $Start" }
    $source.Substring($a, $b - $a)
}
$lifecycle = Extract 'static HANDLE g_recoveryStopEvent' 'BOOL Wh_ModInit()'
$wait = Extract 'static bool SleepUnlessUnloading(DWORD ms)' '// The XAML diagnostics endpoint'
$teardown = Extract 'void Wh_ModBeforeUninit()' '// ---------------------------------------------------------------------'
$prefix = @'
#include <windows.h>
#include <atomic>
#include <vector>
#include <cstdio>
#include <stdexcept>
std::atomic<bool> g_unloading{false};
std::atomic<int> running{0};
HANDLE trackedEvent=nullptr;
bool failCreate=false,failDrain=false,queueMessage=false,earlyClose=false;
int eventCloses=0,pumps=0,teardowns=0,checks=0;
void Wh_Log(const wchar_t*,...){}
HANDLE FakeCreateEvent(LPSECURITY_ATTRIBUTES attrs,BOOL manual,BOOL state,LPCWSTR name){
 if(failCreate){SetLastError(ERROR_NOT_ENOUGH_MEMORY);return nullptr;}
 trackedEvent=CreateEventW(attrs,manual,state,name);return trackedEvent;
}
BOOL FakeCloseHandle(HANDLE h){
 if(h==trackedEvent){if(running.load()!=0)earlyClose=true;++eventCloses;trackedEvent=nullptr;}
 return CloseHandle(h);
}
DWORD FakeMsgWait(DWORD count,const HANDLE* handles,BOOL all,DWORD ms,DWORD mask){
 if(failDrain){failDrain=false;SetLastError(ERROR_INVALID_HANDLE);return WAIT_FAILED;}
 if(queueMessage){queueMessage=false;return WAIT_OBJECT_0+count;}
 return MsgWaitForMultipleObjects(count,handles,all,ms,mask);
}
BOOL FakePeek(LPMSG message,HWND window,UINT low,UINT high,UINT flags){++pumps;return PeekMessageW(message,window,low,high,flags);}
#define CreateEventW FakeCreateEvent
#define CloseHandle FakeCloseHandle
#define MsgWaitForMultipleObjects FakeMsgWait
#define PeekMessageW FakePeek
'@
$stubs = @'
void Check(bool pass,const char* label){++checks;if(!pass){std::fprintf(stderr,"Assertion %d: %s\n",checks,label);throw std::runtime_error(label);}}
void ComponentTeardown(){Check(g_unloading.load()&&running.load()==0&&!g_recoveryStopEvent,"components tear down only after both workers stop");++teardowns;}
bool g_mediaComponentInit=true,g_performanceComponentInit=true;
void OpalMedia_ModBeforeUninit(){ComponentTeardown();}
void OpalPerformance_ModBeforeUninit(){ComponentTeardown();}
void OpalMedia_ModUninit(){ComponentTeardown();}
void OpalPerformance_ModUninit(){ComponentTeardown();}
void TaskbarClockUninit(){ComponentTeardown();}
void TaskbarGeometryUninit(){ComponentTeardown();}
std::vector<int> g_animatedSurfaces;
int g_host=0,g_applied=0;
const wchar_t* HostName(int){return L"test";}
struct WorkerState {HANDLE ready;DWORD delay;};
DWORD WINAPI Worker(void* raw){
 auto* state=static_cast<WorkerState*>(raw);++running;SetEvent(state->ready);
 bool timedOut=SleepUnlessUnloading(state->delay);
 --running;return timedOut?1:0;
}
void StartWorkers(WorkerState& tap,WorkerState& attach){
 g_tapThread=CreateThread(nullptr,0,Worker,&tap,0,nullptr);
 g_lateAttachThread=CreateThread(nullptr,0,Worker,&attach,0,nullptr);
 HANDLE ready[]={tap.ready,attach.ready};
 Check(g_tapThread&&g_lateAttachThread&&WaitForMultipleObjects(2,ready,TRUE,2000)==WAIT_OBJECT_0,"both real worker threads started");
 ResetEvent(tap.ready);ResetEvent(attach.ready);
}
'@
$main = @'
int main(){try{
 WorkerState tap{CreateEventA(nullptr,TRUE,FALSE,nullptr),5000};
 WorkerState attach{CreateEventA(nullptr,TRUE,FALSE,nullptr),15000};
 failCreate=true;Check(!BeginRecoverySession()&&!g_recoveryStopEvent&&g_unloading.load(),"event creation failure leaves stopped session");
 Check(!SleepUnlessUnloading(5000),"missing event never falls back to polling or long sleep");
 failCreate=false;Check(BeginRecoverySession()&&!g_unloading.load(),"successful session opens unsignaled stop event");
 ULONGLONG before=GetTickCount64();Check(SleepUnlessUnloading(35)&&GetTickCount64()-before>=20,"normal wait preserves requested cadence");
 SetEvent(g_recoveryStopEvent);before=GetTickCount64();
 Check(!SleepUnlessUnloading(15000)&&GetTickCount64()-before<1000,"stop-before-wait returns promptly");
 Check(WaitForSingleObject(g_recoveryStopEvent,0)==WAIT_OBJECT_0,"manual-reset stop remains signaled for every waiter");
 Check(BeginRecoverySession()&&WaitForSingleObject(g_recoveryStopEvent,0)==WAIT_TIMEOUT,"retained DLL reload gets unsignaled event");
 StartWorkers(tap,attach);queueMessage=true;before=GetTickCount64();Wh_ModBeforeUninit();
 Check(!g_tapThread&&!g_lateAttachThread&&!g_recoveryStopEvent&&running.load()==0,"BeforeUninit drains both workers and closes shared event");
 Check(GetTickCount64()-before<1500&&pumps==1&&!earlyClose,"long waits cancel promptly while preserving UI message pumping");
 Wh_ModUninit();Check(teardowns==6,"normal BeforeUninit/Uninit performs all component teardown");
 Check(BeginRecoverySession(),"new session can follow completed unload");StartWorkers(tap,attach);
 before=GetTickCount64();Check(BeginRecoverySession(),"reload drains retained workers before reset");
 Check(running.load()==0&&!g_tapThread&&!g_lateAttachThread&&!g_unloading.load()&&WaitForSingleObject(g_recoveryStopEvent,0)==WAIT_TIMEOUT&&GetTickCount64()-before<1500,"reload leaves no old waiter and a fresh live session");
 g_mediaComponentInit=g_performanceComponentInit=true;StartWorkers(tap,attach);Wh_ModUninit();
 Check(!g_recoveryStopEvent&&!g_tapThread&&!g_lateAttachThread,"Uninit alone drains before resource teardown");
 Check(BeginRecoverySession(),"session starts for drain-failure case");StartWorkers(tap,attach);
 int closes=eventCloses;failDrain=true;Check(!StopRecoveryThreads()&&g_recoveryStopEvent&&eventCloses==closes&&g_tapThread,"failed wait cannot close event with undrained thread handle");
 Check(StopRecoveryThreads()&&!g_recoveryStopEvent&&!g_tapThread,"drain retry closes event only after exit confirmation");
 Check(!earlyClose,"event never closed while a worker was active");
 CloseHandle(tap.ready);CloseHandle(attach.ready);
 std::printf("%d recovery wait/lifecycle assertions passed.\n",checks);return 0;
 }catch(...){return 1;}}
'@
$out = Join-Path $root 'build\recovery-wait-test'
New-Item -ItemType Directory -Path $out -Force | Out-Null
$cpp = Join-Path $out 'recovery.cpp'; $exe = Join-Path $out 'recovery.exe'
[IO.File]::WriteAllText($cpp, $prefix + "`n" + $lifecycle + "`n" + $wait + "`n" + $stubs + "`n" + $teardown + "`n" + $main)
& 'C:\Program Files\Windhawk\Compiler\bin\clang++.exe' -std=c++20 -target x86_64-w64-mingw32 -static -DOPAL_UNIFIED_BUILD $cpp -o $exe -luser32
if ($LASTEXITCODE -ne 0) { throw 'Recovery wait regression compilation failed.' }
& $exe
if ($LASTEXITCODE -ne 0) { throw 'Recovery wait regression failed.' }
[pscustomobject]@{passed=$true;evidence='Compiled production event wait, lifecycle, and teardown with real Win32 threads/events; creation/wait failures, retained reload, stop-before-wait, UI pumping, and close ordering.'}
