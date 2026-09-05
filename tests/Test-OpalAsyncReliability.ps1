[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$out = Join-Path $root 'build\async-reliability-test'
New-Item -ItemType Directory -Path $out -Force | Out-Null
$core = [IO.File]::ReadAllText((Join-Path $root 'native\Maxwell.Shell.Core\MaxwellShellCore.cpp'))
$media = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\maxwell-opal-media.wh.cpp'))
$perf = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\maxwell-taskbar-system-info.wh.cpp'))
function Extract([string]$Text,[string]$Start,[string]$End) {
    $a=$Text.IndexOf($Start); $b=$Text.IndexOf($End,$a)
    if($a -lt 0 -or $b -lt 0){throw "Missing production boundary $Start"}
    $Text.Substring($a,$b-$a)
}
$fetch=Extract $core 'std::optional<std::wstring> FetchUrl(' 'std::wstring BuildWeatherUrl('
$await=Extract $media 'template <typename Operation>' 'std::vector<uint8_t> ReadArtwork('
$wait=Extract $perf '            HANDLE waits[3]{g_metricsWorkerWakeEvent,' '            pdhCompletionReady ='
$prefix=@'
#include <windows.h>
#include <wininet.h>
#include <optional>
#include <string>
#include <cstring>
#include <cstdio>
#include <stdexcept>
int mode=0, reads=0; ULONGLONG tick=0; HANDLE stop=nullptr;
ULONGLONG FakeTick(){return tick;}
HINTERNET FakeOpen(LPCWSTR,DWORD,LPCWSTR,LPCWSTR,DWORD){return (HINTERNET)1;}
BOOL FakeOption(HINTERNET,DWORD,LPVOID,DWORD){return TRUE;}
HINTERNET FakeUrl(HINTERNET,LPCWSTR,LPCWSTR,DWORD,DWORD,DWORD_PTR){return (HINTERNET)2;}
BOOL FakeQuery(HINTERNET,DWORD,LPVOID out,LPDWORD,LPDWORD){*(DWORD*)out=200;return TRUE;}
BOOL FakeClose(HINTERNET){return TRUE;}
BOOL FakeRead(HINTERNET,LPVOID buf,DWORD cap,LPDWORD size){
 ++reads;*size=0;
 if(mode==0){if(reads==1){memcpy(buf,"PARTIAL",7);*size=7;return TRUE;}return FALSE;}
 if(mode==1){if(reads<2049){memset(buf,'x',cap);*size=cap;}return TRUE;}
 if(mode==2){if(reads==1){memcpy(buf,"OK",2);*size=2;}return TRUE;}
 if(mode==3){SetEvent(stop);memcpy(buf,"OK",2);*size=2;return TRUE;}
 if(mode==4){tick+=6000;memset(buf,'x',cap);*size=cap;return TRUE;}
 return TRUE;
}
#define GetTickCount64 FakeTick
#define InternetOpenW FakeOpen
#define InternetSetOptionW FakeOption
#define InternetOpenUrlW FakeUrl
#define HttpQueryInfoW FakeQuery
#define InternetCloseHandle FakeClose
#define InternetReadFile FakeRead
'@
$middle=@'
#undef GetTickCount64
namespace winrt {
struct hresult_canceled : std::runtime_error {hresult_canceled():std::runtime_error("cancelled"){} };
namespace Windows::Foundation {enum class AsyncStatus {Started,Completed,Error};}
}
struct FakeOperation {
 mutable bool cancelled=false; int kind=0;
 winrt::Windows::Foundation::AsyncStatus Status() const {
  using S=winrt::Windows::Foundation::AsyncStatus;
  if(kind==3)SetEvent(stop);
  return kind==1?S::Completed:kind==2?S::Error:S::Started;
 }
 void Cancel() const {cancelled=true;}
 int GetResults() const {if(kind==2)throw std::runtime_error("provider error");return 42;}
};
'@
$worker=@'
HANDLE g_metricsWorkerWakeEvent=nullptr,g_externalTelemetryChangedEvent=nullptr,g_pdhCompletionEvent=nullptr;
bool ArmPdhCompletion(int,int){return false;}
DWORD WINAPI WaitWorker(void*) {
 int settings=0,activeInterval=1;DWORD waitResult=WAIT_TIMEOUT;
'@
$main=@'
 return waitResult;
}
int main(){
 stop=CreateEventW(nullptr,TRUE,FALSE,nullptr);
 if(FetchUrl(L"unused",stop))return 1;
 mode=1;reads=0;if(FetchUrl(L"unused",stop))return 2;
 mode=2;reads=0;auto ok=FetchUrl(L"unused",stop);if(!ok||*ok!=L"OK")return 3;
 mode=3;reads=0;if(FetchUrl(L"unused",stop))return 4;
 ResetEvent(stop);mode=4;reads=0;tick=0;if(FetchUrl(L"unused",stop))return 5;
 mode=5;reads=0;tick=0;if(FetchUrl(L"unused",stop))return 6;
 mode=2;reads=0;SetEvent(stop);if(FetchUrl(L"unused",stop)||reads)return 7;
 FakeOperation op;
 try{AwaitMediaOperation(op,stop,100);return 8;}catch(winrt::hresult_canceled&){}
 if(!op.cancelled)return 9;
 ResetEvent(stop);op.cancelled=false;auto before=GetTickCount64();
 try{AwaitMediaOperation(op,stop,75);return 10;}catch(winrt::hresult_canceled&){}
 if(!op.cancelled||GetTickCount64()-before>500)return 11;
 op.kind=1;if(AwaitMediaOperation(op,stop,100)!=42)return 12;
 op.kind=2;try{AwaitMediaOperation(op,stop,100);return 13;}catch(std::runtime_error&){}
 op.kind=3;op.cancelled=false;try{AwaitMediaOperation(op,stop,100);return 14;}catch(winrt::hresult_canceled&){}
 if(!op.cancelled)return 15;
 g_metricsWorkerWakeEvent=CreateEventW(nullptr,FALSE,FALSE,nullptr);
 g_externalTelemetryChangedEvent=CreateEventW(nullptr,FALSE,FALSE,nullptr);
 HANDLE worker=CreateThread(nullptr,0,WaitWorker,nullptr,0,nullptr);
 DWORD waited=WaitForSingleObject(worker,1600),code=0;GetExitCodeThread(worker,&code);
 if(waited!=WAIT_OBJECT_0){SetEvent(g_metricsWorkerWakeEvent);WaitForSingleObject(worker,2000);return 16;}
 if(code!=WAIT_TIMEOUT)return 17;
 CloseHandle(worker);CloseHandle(g_metricsWorkerWakeEvent);CloseHandle(g_externalTelemetryChangedEvent);CloseHandle(stop);
 std::puts("17 regression assertions passed: complete/error/oversize/empty/cancelled/deadline weather; media completion/error/cancellation/timeout; silent publisher deadline.");return 0;
}
'@
$cpp=Join-Path $out 'reliability.cpp';$exe=Join-Path $out 'reliability.exe'
[IO.File]::WriteAllText($cpp,$prefix+"`n"+$fetch+"`n"+$middle+"`n"+$await+"`n"+$worker+"`n"+$wait+"`n"+$main)
& 'C:\Program Files\Windhawk\Compiler\bin\clang++.exe' -std=c++20 -target x86_64-w64-mingw32 -static $cpp -o $exe
if($LASTEXITCODE -ne 0){throw 'Reliability regression compilation failed.'}
& $exe
if($LASTEXITCODE -ne 0){throw "Reliability regression failed at $LASTEXITCODE"}
if($core -notmatch 'WaitForSingleObject\(weatherThread, INFINITE\) != WAIT_OBJECT_0' -or $core -match 'WaitForSingleObject\(weatherThread, 8000\)'){throw 'Weather shutdown must confirm exit before cleanup.'}
if($media -match '(RequestAsync|OpenReadAsync|TryGetMediaPropertiesAsync)\(\)\.get\(\)' -or $media -match 'InputStreamOptions::None\)\.get\(\)'){throw 'Unbounded media async wait returned.'}
$unstable=Extract $perf '    if (!stable) {' '    // A structurally wrong mapping'
if($unstable -notmatch 'EnsureShellCoreProcess\(\);\s*return false;'){throw 'An abandoned odd sequence must reach companion recovery.'}
[pscustomobject]@{passed=$true;assertions=20;evidence='Extracted production code, deterministic I/O and provider mocks, real Win32 wait. Shutdown lifetime, abandoned sequence recovery, and call-site integration assertions.'}
