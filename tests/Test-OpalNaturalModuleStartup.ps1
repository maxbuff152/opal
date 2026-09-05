[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$shell = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\maxwell-shell.wh.cpp'))
$icons = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\opal-addon-icons.h'))
function Slice([string]$Text, [string]$Start, [string]$End) {
    $begin = $Text.IndexOf($Start, [StringComparison]::Ordinal)
    $finish = $Text.IndexOf($End, $begin, [StringComparison]::Ordinal)
    if ($begin -lt 0 -or $finish -le $begin) { throw "Missing production boundary: $Start" }
    $Text.Substring($begin, $finish - $begin)
}
$geometry = Slice $shell 'static void TaskbarGeometryInit()' 'static void TaskbarGeometrySettingsChanged()'
$loader = Slice $icons 'using LoadLibraryExW_t = decltype(&LoadLibraryExW);' 'BOOL Init()'
$init = Slice $icons 'BOOL Init()' '// Late-hook safety net.'
$late = Slice $icons 'static DWORD WINAPI LateHookThreadProc(LPVOID)' 'void BeforeUninit()'
$prefix = @'
#include <windows.h>
#include <atomic>
#include <cstdio>
#include <cwchar>
#include <stdexcept>
int checks=0;
void Check(bool value,const char* message){++checks;if(!value){std::fprintf(stderr,"%s\n",message);throw std::runtime_error(message);}}
enum class Host{Explorer,Other};
Host g_host=Host::Explorer;
bool g_geometryInit=false;
namespace OpalAddonIcons {
std::atomic<bool> g_systemTrayModuleHooked{},g_taskbarViewDllLoaded{},g_searchUxUiDllLoaded{},g_unloading{};
bool g_iconGeometryFailSoft=false,g_hasDynamicIconScaling=false;
struct {int taskbarHeight=48,iconSize=24,taskbarButtonWidth=40;} g_settings;
HMODULE view=nullptr,tray=nullptr,search=nullptr;
int milliseconds=0,arrival=-1,viewHooks=0,trayHooks=0,searchHooks=0,applies=0,refreshes=0,settingsApplies=0;
int loaderCalls=0,threadStarts=0,hookRegistrations=0;
bool hookRegistrationSucceeds=true,viewHookSucceeds=true,taskbarHookSucceeds=true,searchHookSucceeds=true,throwInit=false;
LPTHREAD_START_ROUTINE pendingThread=nullptr;
HANDLE g_lateHookThread=nullptr;
HMODULE GetTaskbarViewModuleHandle(){return view;}
HMODULE GetSystemTrayModuleHandle(){return tray;}
HMODULE GetSearchUxUiModuleHandle(){return search;}
void Wh_Log(const wchar_t*,...){}
void IconsDiag(const wchar_t*,...){}
void LoadSettings(){if(throwInit)throw std::runtime_error("fixture init failure");}
bool HookTaskbarDllSymbols(){return taskbarHookSucceeds;}
bool HookSystemTraySymbols(HMODULE){++trayHooks;return true;}
bool HookTaskbarViewDllSymbols(HMODULE,bool inlineTray){++viewHooks;if(inlineTray)++trayHooks;return viewHookSucceeds;}
bool HookSearchUxUiDllSymbols(HMODULE){++searchHooks;return searchHookSucceeds;}
void Wh_ApplyHookOperations(){++applies;}
void QueueLateGeometryRefresh(){++refreshes;}
void ApplySettings(int){++settingsApplies;}
void* SHAppBarMessage_Original=nullptr;
void* SendMessageTimeoutW_Original=nullptr;
void SHAppBarMessage_Hook(){}
void SendMessageTimeoutW_Hook(){}
namespace WindhawkUtils {template<class A,class B,class C>bool SetFunctionHook(A,B,C){++hookRegistrations;return hookRegistrationSucceeds;}}
HMODULE FakeGetModuleHandle(LPCWSTR){return reinterpret_cast<HMODULE>(1);}
FARPROC FakeGetProcAddress(HMODULE,LPCSTR){return reinterpret_cast<FARPROC>(1);}
HANDLE WINAPI FakeCreateThread(LPSECURITY_ATTRIBUTES,SIZE_T,LPTHREAD_START_ROUTINE proc,LPVOID,DWORD,LPDWORD){++threadStarts;pendingThread=proc;return reinterpret_cast<HANDLE>(1);}
void FakeSleep(DWORD ms){milliseconds+=ms;if(arrival>=0&&milliseconds>=arrival){view=reinterpret_cast<HMODULE>(2);tray=reinterpret_cast<HMODULE>(3);}}
HMODULE WINAPI NaturalLoad(LPCWSTR name,HANDLE,DWORD){++loaderCalls;if(!wcscmp(name,L"Taskbar.View.dll"))view=reinterpret_cast<HMODULE>(2);else if(!wcscmp(name,L"SystemTray.dll"))tray=reinterpret_cast<HMODULE>(3);else if(!wcscmp(name,L"SearchUx.UI.dll"))search=reinterpret_cast<HMODULE>(4);return !wcscmp(name,L"Taskbar.View.dll")?view:!wcscmp(name,L"SystemTray.dll")?tray:search;}
#undef GetModuleHandle
#define GetModuleHandle FakeGetModuleHandle
#define GetProcAddress FakeGetProcAddress
#define CreateThread FakeCreateThread
#define Sleep FakeSleep
'@
$suffix = @'
void Reset(){
g_systemTrayModuleHooked=false;g_taskbarViewDllLoaded=false;g_searchUxUiDllLoaded=false;g_unloading=false;
g_iconGeometryFailSoft=false;view=nullptr;tray=nullptr;search=nullptr;
milliseconds=0;arrival=-1;viewHooks=trayHooks=searchHooks=applies=refreshes=settingsApplies=loaderCalls=threadStarts=hookRegistrations=0;
hookRegistrationSucceeds=viewHookSucceeds=taskbarHookSucceeds=searchHookSucceeds=true;throwInit=false;
pendingThread=nullptr;g_lateHookThread=nullptr;LoadLibraryExW_Original=NaturalLoad;
::g_geometryInit=false;::g_host=Host::Explorer;
}
} // namespace OpalAddonIcons
'@
$main = @'
int main(){try{
using namespace OpalAddonIcons;
Reset();TaskbarGeometryInit();
Check(g_geometryInit,"Cold init must succeed without a packaged module");
Check(!view&&!tray&&loaderCalls==0&&milliseconds==0,"Cold shell init must not preload or wait for packaged modules");
Check(viewHooks==0&&trayHooks==0,"Absent modules must not be hooked");
TaskbarGeometryAfterInit();
Check(threadStarts==1&&pendingThread==LateHookThreadProc,"Absent modules must retain late polling fallback");
Check(loaderCalls==0,"AfterInit must not load packaged modules");
arrival=1000;pendingThread(nullptr);
Check(viewHooks==1&&trayHooks==1,"Natural arrival missed geometry or tray hooks");
Check(applies==2&&refreshes==2,"Late natural arrival must apply hooks and queue geometry refresh");
Check(milliseconds==1000&&loaderCalls==0,"Polling must stop on readiness without loading modules");

Reset();hookRegistrationSucceeds=false;TaskbarGeometryInit();TaskbarGeometryAfterInit();
Check(g_geometryInit&&threadStarts==1,"Unavailable loader hook must retain startup and polling");
arrival=500;pendingThread(nullptr);
Check(viewHooks==1&&trayHooks==1&&refreshes==2,"Packaged activation invisible to loader hook must recover by polling");

Reset();TaskbarGeometryInit();TaskbarGeometryAfterInit();pendingThread(nullptr);
Check(milliseconds==30000&&viewHooks==0&&trayHooks==0&&loaderCalls==0,"Absent module must preserve bounded existing poll without speculative loading");
Check(refreshes==0,"Absent module falsely triggered geometry refresh");

Reset();TaskbarGeometryInit();TaskbarGeometryAfterInit();g_unloading=true;pendingThread(nullptr);
Check(milliseconds==0&&viewHooks==0&&refreshes==0,"Unload must stop pending discovery before work");

Reset();view=reinterpret_cast<HMODULE>(2);tray=reinterpret_cast<HMODULE>(3);search=reinterpret_cast<HMODULE>(4);
TaskbarGeometryInit();TaskbarGeometryAfterInit();
Check(viewHooks==1&&trayHooks==1&&searchHooks==1,"Warm startup lost direct symbol hooks");
Check(threadStarts==0&&loaderCalls==0,"Warm startup started redundant discovery or module loading");

Reset();TaskbarGeometryInit();LoadLibraryExW_Hook(L"Taskbar.View.dll",nullptr,0);LoadLibraryExW_Hook(L"SystemTray.dll",nullptr,0);
Check(viewHooks==1&&trayHooks==1&&applies==2&&refreshes==2,"Natural loader callback lost delayed hooks/refresh");
LoadLibraryExW_Hook(L"Taskbar.View.dll",nullptr,0);
Check(viewHooks==1&&refreshes==2,"Duplicate loader callback repeated hooks");

Reset();view=reinterpret_cast<HMODULE>(2);tray=view;search=reinterpret_cast<HMODULE>(4);
TaskbarGeometryInit();TaskbarGeometryAfterInit();
Check(viewHooks==1&&trayHooks==1&&threadStarts==0,"Older inline-tray module lost batched direct hooks");

Reset();taskbarHookSucceeds=false;TaskbarGeometryInit();TaskbarGeometryAfterInit();
Check(g_geometryInit&&g_iconGeometryFailSoft&&threadStarts==1,"Native taskbar symbol failure lost fail-soft discovery");

Reset();view=reinterpret_cast<HMODULE>(2);tray=reinterpret_cast<HMODULE>(3);viewHookSucceeds=false;
TaskbarGeometryInit();
Check(g_geometryInit&&g_iconGeometryFailSoft,"Initial view hook failure lost existing fail-soft behavior");

Reset();search=reinterpret_cast<HMODULE>(4);searchHookSucceeds=false;TaskbarGeometryInit();TaskbarGeometryAfterInit();
Check(!g_geometryInit&&threadStarts==0&&settingsApplies==0,"Failed init must not run geometry AfterInit");

Reset();throwInit=true;TaskbarGeometryInit();TaskbarGeometryAfterInit();
Check(!g_geometryInit&&threadStarts==0,"Thrown geometry init must remain isolated");

Reset();g_host=Host::Other;TaskbarGeometryInit();TaskbarGeometryAfterInit();
Check(!g_geometryInit&&hookRegistrations==0&&loaderCalls==0,"Non-Explorer host performed geometry startup");
std::printf("%d assertions passed\n",checks);
return 0;
}catch(...){return 1;}}
'@
$scratch = Join-Path $root 'build\natural-module-startup-test'
New-Item -ItemType Directory -Path $scratch -Force | Out-Null
$cpp = Join-Path $scratch 'startup.cpp'
$exe = Join-Path $scratch 'startup.exe'
[IO.File]::WriteAllText($cpp, ($prefix + "`n" + $loader + "`n" + $init + "`n" + $late + "`n" + $suffix + "`n" + $geometry + "`n" + $main))
& 'C:\Program Files\Windhawk\Compiler\bin\clang++.exe' '-std=c++20' '-target' 'x86_64-w64-mingw32' '-static' $cpp '-o' $exe
if ($LASTEXITCODE -ne 0) { throw 'Production delayed-module regression compilation failed.' }
& $exe
if ($LASTEXITCODE -ne 0) { throw 'Production delayed-module regression failed.' }
[pscustomobject]@{Passed=$true;Assertions=23;Evidence='Compiled actual shell geometry wrappers and icon Init/AfterInit, loader callback and late polling; mocked OS/module/symbol operations only.';LiveExplorerRestart=$false;Limitation='Preserves existing 30-second poll and existing hook-failure behavior; does not prove crash causality or live geometry.'}
