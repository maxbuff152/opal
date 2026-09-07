[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$scratch = Join-Path $root 'build\opal45-audit-probes'
New-Item -ItemType Directory -Path $scratch -Force | Out-Null
$core = [IO.File]::ReadAllText((Join-Path $root 'native\Maxwell.Shell.Core\MaxwellShellCore.cpp'))
$perf = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\maxwell-taskbar-system-info.wh.cpp'))
$fetchStart = $core.IndexOf('std::optional<std::wstring> FetchUrl(')
$fetchEnd = $core.IndexOf('std::wstring BuildWeatherUrl(', $fetchStart)
if ($fetchStart -lt 0 -or $fetchEnd -lt 0) { throw 'FetchUrl boundary missing' }
$fetch = $core.Substring($fetchStart, $fetchEnd - $fetchStart)
$waitStart = $perf.IndexOf('            HANDLE waits[3]{g_metricsWorkerWakeEvent,')
$waitEnd = $perf.IndexOf('            pdhCompletionReady =', $waitStart)
if ($waitStart -lt 0 -or $waitEnd -lt 0) { throw 'Metrics wait boundary missing' }
$wait = $perf.Substring($waitStart, $waitEnd - $waitStart)
$preamble = @'
#include <windows.h>
#include <wininet.h>
#include <optional>
#include <string>
#include <cstring>
#include <cstdio>
static int mode = 0, reads = 0;
HINTERNET FakeOpen(LPCWSTR,DWORD,LPCWSTR,LPCWSTR,DWORD) { return (HINTERNET)1; }
BOOL FakeOption(HINTERNET,DWORD,LPVOID,DWORD) { return TRUE; }
HINTERNET FakeUrl(HINTERNET,LPCWSTR,LPCWSTR,DWORD,DWORD,DWORD_PTR) { return (HINTERNET)2; }
BOOL FakeQuery(HINTERNET,DWORD,LPVOID out,LPDWORD,LPDWORD) { *(DWORD*)out=200; return TRUE; }
BOOL FakeClose(HINTERNET) { return TRUE; }
BOOL FakeRead(HINTERNET,LPVOID buffer,DWORD cap,LPDWORD size) {
  ++reads; *size=0;
  if (mode==0) {
    if (reads==1) { std::memcpy(buffer,"PARTIAL",7); *size=7; return TRUE; }
    SetLastError(ERROR_INTERNET_CONNECTION_ABORTED); return FALSE;
  }
  if (reads<=2048) { std::memset(buffer,'x',cap); *size=cap; }
  return TRUE;
}
#define InternetOpenW FakeOpen
#define InternetSetOptionW FakeOption
#define InternetOpenUrlW FakeUrl
#define HttpQueryInfoW FakeQuery
#define InternetCloseHandle FakeClose
#define InternetReadFile FakeRead
'@
$postamble = @'
HANDLE g_metricsWorkerWakeEvent=nullptr, g_externalTelemetryChangedEvent=nullptr, g_pdhCompletionEvent=nullptr;
bool ArmPdhCompletion(int,int) { return false; }
struct WaitResult { bool noPublisherWaitBlocked; DWORD rescuedResult; DWORD exitResult; };
DWORD WINAPI WaitWorker(void*) {
    int settings=0, activeInterval=1; DWORD waitResult=WAIT_TIMEOUT;
'@
$tail = @'
    return waitResult;
}
WaitResult CheckPublisherWait() {
  g_metricsWorkerWakeEvent=CreateEventW(nullptr,FALSE,FALSE,nullptr);
  g_externalTelemetryChangedEvent=CreateEventW(nullptr,FALSE,FALSE,nullptr);
  HANDLE worker=CreateThread(nullptr,0,WaitWorker,nullptr,0,nullptr);
  bool blocked=WaitForSingleObject(worker,1250)==WAIT_TIMEOUT;
  SetEvent(g_metricsWorkerWakeEvent);
  DWORD rescued=WaitForSingleObject(worker,2000), code=0;
  GetExitCodeThread(worker,&code);
  CloseHandle(worker); CloseHandle(g_metricsWorkerWakeEvent); CloseHandle(g_externalTelemetryChangedEvent);
  return {blocked,rescued,code};
}
int main() {
  mode=0; reads=0; auto partial=FetchUrl(L"https://unused.invalid");
  bool accepted=partial && *partial==L"PARTIAL";
  mode=1; reads=0; auto large=FetchUrl(L"https://unused.invalid");
  auto wait=CheckPublisherWait();
  std::printf("{\"partialReadAcceptedAsSuccess\":%s,\"acceptedResponseChars\":%llu,\"publisherSilentWaitExceededInterval\":%s,\"manualWakeRecovered\":%s}\n",
    accepted?"true":"false", (unsigned long long)(large?large->size():0),
    wait.noPublisherWaitBlocked?"true":"false",wait.rescuedResult==WAIT_OBJECT_0?"true":"false");
  return accepted && large && large->size()==2097152 && wait.noPublisherWaitBlocked && wait.rescuedResult==WAIT_OBJECT_0 ? 0 : 1;
}
'@
$cpp = Join-Path $scratch 'production-path-probes.cpp'
$exe = Join-Path $scratch 'production-path-probes.exe'
[IO.File]::WriteAllText($cpp, $preamble + "`n" + $fetch + "`n" + $postamble + "`n" + $wait + "`n" + $tail)
& 'C:\Program Files\Windhawk\Compiler\bin\clang++.exe' -std=c++20 -static $cpp -o $exe
if ($LASTEXITCODE -ne 0) { throw 'Probe compilation failed' }
$nativeResult = & $exe
if ($LASTEXITCODE -ne 0) { throw 'Expected fault behaviors were not all reproduced' }
$native = $nativeResult | ConvertFrom-Json
$invalidPath = Join-Path $scratch 'missing-metrics.json'
@{Scenarios=@(@{Scenario='Stock'},@{Scenario='FullSuite'})} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $invalidPath
$invalidResult = & (Join-Path $root 'Test-OpalPerformanceBudget.ps1') -ReceiptPath $invalidPath
$historical = & (Join-Path $root 'Test-OpalPerformanceBudget.ps1') -ReceiptPath (Join-Path $PSScriptRoot 'package-cost-opal-4-unified-valid-settled-abba2.json')
$result = [ordered]@{
  capturedAt = [DateTimeOffset]::Now.ToString('o')
  purpose = 'Adversarial audit; true fault flags mean defects reproduced, not a release pass.'
  sourceCommit = (& git -C $root rev-parse HEAD)
  sourceHashes = @('native\Maxwell.Shell.Core\MaxwellShellCore.cpp','mod\visual-clones\maxwell-taskbar-system-info.wh.cpp','Test-OpalPerformanceBudget.ps1') | ForEach-Object { Get-FileHash -LiteralPath (Join-Path $root $_) | Select-Object Path,Hash }
  nativeProbes = $native
  missingMetricsReceiptAccepted = [bool]$invalidResult.passed
  historical40ReceiptAcceptedForCurrent45 = [bool]$historical.passed
  boundaries = 'No network traffic; production FetchUrl with deterministic WinINet mocks. Actual Win32 wait block extracted from production, isolated private events, 1250ms silence then deliberate wake. No live process stops, settings changes, or Explorer restart.'
  limitations = 'Not an end-to-end live publisher crash or real network fault injection. Media cancellation and weather shutdown race are source-review findings.'
}
$result | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'opal45-audit-adversarial-20260904.json')
$result | ConvertTo-Json -Depth 7
