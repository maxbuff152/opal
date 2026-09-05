[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$source = [IO.File]::ReadAllText((Join-Path $root 'native\Maxwell.Shell.Core\MaxwellShellCore.cpp'))
$clock = [IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\opal-addon-clock.h'))
$handoffStart = $clock.IndexOf('if (PublishWeatherRequest(g_settings.webContentWeatherLocation,')
$fallbackStart = $clock.IndexOf('Wh_Log(L"Fetching weather from URL:', $handoffStart)
if ($handoffStart -lt 0 -or $fallbackStart -lt 0) { throw 'Weather ownership boundary not found.' }
$handoff = $clock.Substring($handoffStart, $fallbackStart - $handoffStart)
if ($handoff -notmatch 'return false;\s*}\s*$' -or $handoff -match 'GetUrlContent') {
    throw 'Explorer must not duplicate a request accepted by the weather companion.'
}
if ($source -notmatch 'INTERNET_OPTION_CONNECT_TIMEOUT' -or $source -notmatch 'INTERNET_OPTION_RECEIVE_TIMEOUT') {
    throw 'Weather companion requires explicit network timeouts.'
}
$start = $source.IndexOf('bool WeatherFetchDue(')
$end = $source.IndexOf('DWORD WINAPI WeatherLoop(', $start)
if ($start -lt 0 -or $end -lt 0) { throw 'Production weather retry function not found.' }
$function = $source.Substring($start, $end - $start)
$checks = @'
int main() {
    if (!WeatherFetchDue(1, 0, false, false)) return 1;
    if (WeatherFetchDue(30099, 100, false, false)) return 2;
    if (!WeatherFetchDue(30100, 100, false, false)) return 3;
    if (WeatherFetchDue(600099, 100, true, false)) return 4;
    if (!WeatherFetchDue(600100, 100, true, false)) return 5;
    if (!WeatherFetchDue(101, 100, true, true)) return 6;
    if (!WeatherFetchDue(50, 100, true, false)) return 7;
    return 0;
}
'@
$build = Join-Path $root 'build\weather-retry-test'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$cpp = Join-Path $build 'weather-retry.cpp'
$exe = Join-Path $build 'weather-retry.exe'
[IO.File]::WriteAllText($cpp, "#include <cstdint>`n" + $function + "`n" + $checks)
& 'C:\Program Files\Windhawk\Compiler\bin\clang++.exe' -std=c++20 -static $cpp -o $exe
if ($LASTEXITCODE -ne 0) { throw 'Weather retry regression test did not compile.' }
& $exe
if ($LASTEXITCODE -ne 0) { throw "Weather retry regression failed: $LASTEXITCODE" }
[pscustomobject]@{passed=$true; cases=7; evidence='Production weather scheduling: cold start, failed and successful interval boundaries, changed location, clock reset.'}
