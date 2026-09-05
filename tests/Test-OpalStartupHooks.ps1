[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$source=[IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\maxwell-taskbar-system-info.wh.cpp'))
$init=$source.Substring($source.IndexOf('BOOL Wh_ModInit()'))
$start=$init.IndexOf('    } else {')
$end=$init.IndexOf('    OpalControl::PublishRuntimeState(',$start)
if($start -lt 0 -or $end -lt 0){throw 'Missing production startup branch.'}
$branch=$init.Substring($start,$end-$start)
$out=Join-Path $root 'build\startup-hooks-test'
New-Item -ItemType Directory -Path $out -Force|Out-Null
$prefix=@'
#include <windows.h>
int calls=0;
using LoadLibraryExW_t=void*;
void* LoadLibraryExW_Original=nullptr;
void* LoadLibraryExW_Hook=nullptr;
BOOL failInit(const wchar_t*){return FALSE;}
HMODULE FakeModule(LPCWSTR){return reinterpret_cast<HMODULE>(1);}
void* FakeProc(HMODULE,LPCSTR){return reinterpret_cast<void*>(1);}
namespace WindhawkUtils {bool SetFunctionHook(void*,void*,void**){++calls;return false;}}
#define GetModuleHandleW FakeModule
#define GetProcAddress FakeProc
BOOL ColdStart(){if(false){
'@
$suffix=@'
 return TRUE;
}
int main(){
#ifdef OPAL_UNIFIED_BUILD
 return !ColdStart() || calls!=0;
#else
 return ColdStart() || calls!=1;
#endif
}
'@
$cpp=Join-Path $out 'startup.cpp'
[IO.File]::WriteAllText($cpp,$prefix+"`n"+$branch+"`n"+$suffix)
foreach($mode in @('unified','standalone')){
 $exe=Join-Path $out "$mode.exe"
 $arguments=@('-std=c++20','-target','x86_64-w64-mingw32','-static',$cpp,'-o',$exe)
 if($mode -eq 'unified'){$arguments+='-DOPAL_UNIFIED_BUILD'}
 & 'C:\Program Files\Windhawk\Compiler\bin\clang++.exe' @arguments
 if($LASTEXITCODE -ne 0){throw "Startup regression compilation failed: $mode"}
 & $exe
 if($LASTEXITCODE -ne 0){throw "Cold startup fallback failed: $mode"}
}
[pscustomobject]@{passed=$true;cases=2;evidence='Compiled production cold-start branch; unified startup survives unavailable/redundant loader hook, standalone retains its fallback failure.'}
