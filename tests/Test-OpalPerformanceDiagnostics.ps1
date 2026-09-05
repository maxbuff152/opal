[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$out=Join-Path $root 'build\performance-diagnostics-test'
New-Item -ItemType Directory -Path $out -Force|Out-Null
$prefix=@'
#include <windows.h>
#include <atomic>
extern std::atomic<LONGLONG> ticks;
extern std::atomic<int> clockCalls;
inline BOOL TestCounter(LARGE_INTEGER* out){++clockCalls;out->QuadPart=ticks.load();return TRUE;}
inline BOOL TestFrequency(LARGE_INTEGER* out){out->QuadPart=1000000;return TRUE;}
#define QueryPerformanceCounter TestCounter
#define QueryPerformanceFrequency TestFrequency
#include "opal-performance-diagnostics.h"
'@
$main=@'
#include <thread>
#include <vector>
#include <cstdio>
#include <stdexcept>
std::atomic<LONGLONG> ticks{1000};std::atomic<int> clockCalls{0};
void RecordFromOtherTranslationUnit();
int checks=0;
void Check(bool value,const char* message){++checks;if(!value){std::fprintf(stderr,"%s\n",message);throw std::runtime_error(message);}}
int main(){try{
 using namespace OpalPerformanceDiagnostics;
 Check(!Begin().ticks&&clockCalls==0,"disabled has no clock work");
 Check(Summary().empty(),"disabled produces no export");
 SetEnabled(true);auto start=Begin();ticks=2000;Record(Metric::MediaUiWork,start);
 auto& sample=g_samples[static_cast<size_t>(Metric::MediaUiWork)];
 Check(sample.count==1&&sample.milliseconds[0]==1.0,"QPC elapsed milliseconds");
 SetEnabled(true);Check(sample.count==1,"idempotent component settings do not reset samples");
 Record(static_cast<Metric>(99),start);Check(sample.total==1,"invalid metric ignored");
 ticks=500;Record(Metric::MediaUiWork,start);Check(sample.total==1,"backward clocks ignored");
 ticks=3000;auto stale=Begin();SetEnabled(false);Check(sample.count==0,"disable clears samples");
 Record(Metric::MediaUiWork,stale);Check(sample.count==0,"disabled record ignored");
 SetEnabled(true);Record(Metric::MediaUiWork,stale);Check(sample.count==0,"previous collection epoch ignored");
 for(int i=0;i<100;++i){ticks=1000;auto s=Begin();ticks=1000+1000*i;Record(Metric::MediaUiWork,s);}
 Check(sample.count==64&&sample.total==100&&sample.next==36,"ring bounded to 64 samples");
 auto text=Summary();Check(text.find(L"median=67.500 p95=96.000 max=99.000")!=std::wstring::npos,"summary describes recent ring not evicted samples");
 Check(text.find(L"not acknowledgment")!=std::wstring::npos&&text.find(L"not pixel-present")!=std::wstring::npos,"observation boundaries exported explicitly");
 RecordFromOtherTranslationUnit();Check(g_samples[static_cast<size_t>(Metric::HardwareOpen)].total==1,"inline storage shared across translation units");
 SetEnabled(false);SetEnabled(true);ticks=1000;auto concurrent=Begin();ticks=2000;
 std::vector<std::thread> threads;for(int i=0;i<4;++i)threads.emplace_back([&]{for(int j=0;j<100;++j)Record(Metric::MediaUiWork,concurrent);});for(auto& t:threads)t.join();
 Check(sample.total==400&&sample.count==64,"concurrent publishers preserve counts and bounded storage");
 sample.total=UINT64_MAX;Record(Metric::MediaUiWork,concurrent);Check(sample.total==UINT64_MAX,"total saturates without rollover");
 {Scope scope(Metric::HardwareRefresh);ticks+=1000;}
 Check(g_samples[static_cast<size_t>(Metric::HardwareRefresh)].total==1,"RAII scope records completed work");
 SetEnabled(false);Check(Summary().empty(),"disabling suppresses previously collected data");
 std::printf("%d diagnostics assertions passed.\n",checks);return 0;
 }catch(...){return 1;}}
'@
$other=@'
void RecordFromOtherTranslationUnit(){auto s=OpalPerformanceDiagnostics::Begin();ticks+=1000;OpalPerformanceDiagnostics::Record(OpalPerformanceDiagnostics::Metric::HardwareOpen,s);}
'@
$cpp=Join-Path $out 'diagnostics.cpp';$otherCpp=Join-Path $out 'other.cpp';$exe=Join-Path $out 'diagnostics.exe'
[IO.File]::WriteAllText($cpp,$prefix+"`n"+$main)
[IO.File]::WriteAllText($otherCpp,$prefix+"`n"+$other)
& 'C:\Program Files\Windhawk\Compiler\bin\clang++.exe' -std=c++20 -target x86_64-w64-mingw32 -static -I (Join-Path $root 'mod\visual-clones') $cpp $otherCpp -o $exe
if($LASTEXITCODE -ne 0){throw 'Diagnostics regression compilation failed.'}
& $exe
if($LASTEXITCODE -ne 0){throw 'Diagnostics regression failed.'}
[pscustomobject]@{passed=$true;cases=17;evidence='Production diagnostics header with deterministic QPC, concurrent writers, two linked translation units, bounded retention, disabled behavior, and export semantics.'}
