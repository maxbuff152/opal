[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$source = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\maxwell-taskbar-system-info.wh.cpp'))
$start = $source.IndexOf('ContentPriority ResolveContentPriority(')
$end = $source.IndexOf('void ApplyWidgetGeometry(', $start)
if ($start -lt 0 -or $end -lt 0) { throw 'Production density function not found.' }
$function = $source.Substring($start, $end - $start)
$preamble = @'
#include <algorithm>
constexpr double kCompactHardwareWidth = 184.0;
enum class ContentPriority { Essential, Balanced, Full };
struct ModSettings { bool contentPriorityEnabled = true; };
'@
$checks = @'
int main() {
    for (bool adaptive : {false, true}) {
        ModSettings settings{adaptive};
        if (ResolveContentPriority(260, 260, settings) != ContentPriority::Essential) return 1;
        if (ResolveContentPriority(390, 390, settings) != ContentPriority::Full) return 2;
        if (ResolveContentPriority(310, 390, settings) != ContentPriority::Balanced) return 3;
        if (ResolveContentPriority(100, 390, settings) != ContentPriority::Essential) return 4;
    }
    return 0;
}
'@
$build = Join-Path $root 'build\density-test'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$cpp = Join-Path $build 'density.cpp'
$exe = Join-Path $build 'density.exe'
[IO.File]::WriteAllText($cpp, $preamble + "`n" + $function + "`n" + $checks)
& 'C:\Program Files\Windhawk\Compiler\bin\clang++.exe' -std=c++23 -static $cpp -o $exe
if ($LASTEXITCODE -ne 0) { throw 'Density regression test did not compile.' }
& $exe
if ($LASTEXITCODE -ne 0) { throw "Density regression failed: $LASTEXITCODE" }
[pscustomobject]@{passed=$true; cases=8; evidence='Compiled production density function with enabled and disabled adaptive profiles.'}
