[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$source = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\opal-control.h'))
$start = $source.IndexOf('inline HWND VisibleFullViewWindow(')
$end = $source.IndexOf('inline std::vector<HWND> OtherTaskbarWindows', $start)
if ($start -lt 0 -or $end -lt 0) { throw 'Production monitor routing function not found.' }
$preamble = @'
#include <vector>
using HWND = void*;
enum class MonitorTarget { Primary, Secondary, Both };
struct TaskbarWindows { HWND primary; HWND secondary; std::vector<HWND> secondaries; };
TaskbarWindows windows;
HWND covered = nullptr;
TaskbarWindows CurrentProcessTaskbars() { return windows; }
bool TaskbarOccluded(HWND window) { return window == covered; }
'@
$checks = @'
int main() {
    int a, b;
    windows = {&a, &b, {&b}};
    for (HWND hidden : {static_cast<HWND>(&a), static_cast<HWND>(&b), static_cast<HWND>(nullptr)}) {
        covered = hidden;
        if (VisibleFullViewWindow(MonitorTarget::Primary, false) != &a) return 1;
        if (VisibleFullViewWindow(MonitorTarget::Secondary, true) != &b) return 2;
    }
    covered = &a;
    if (VisibleFullViewWindow(MonitorTarget::Both, false) != &b) return 3;
    covered = &b;
    if (VisibleFullViewWindow(MonitorTarget::Both, true) != &a) return 4;
    windows = {&a, nullptr, {}};
    if (VisibleFullViewWindow(MonitorTarget::Secondary, true) != &a) return 5;
    windows = {nullptr, &b, {&b}};
    if (VisibleFullViewWindow(MonitorTarget::Primary, false) != &b) return 6;
    return 0;
}
'@
$build = Join-Path $root 'build\monitor-routing-test'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$cpp = Join-Path $build 'routing.cpp'
$exe = Join-Path $build 'routing.exe'
[IO.File]::WriteAllText($cpp, $preamble + "`n" + $source.Substring($start, $end-$start) + "`n" + $checks)
& 'C:\Program Files\Windhawk\Compiler\bin\clang++.exe' -std=c++23 -static $cpp -o $exe
if ($LASTEXITCODE -ne 0) { throw 'Monitor routing regression test did not compile.' }
& $exe
if ($LASTEXITCODE -ne 0) { throw "Monitor routing regression failed: $LASTEXITCODE" }
[pscustomobject]@{passed=$true; cases=10; evidence='Production routing under fullscreen and disconnected-display scenarios.'}
