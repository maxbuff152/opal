[CmdletBinding()]
param([string]$CandidateDll)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$script:cases=0
function Check([bool]$Condition,[string]$Label) { if(-not $Condition){throw $Label}; $script:cases++ }
function Reject([scriptblock]$Action,[string]$Label) { $rejected=$false; try { & $Action | Out-Null } catch {$rejected=$true}; Check $rejected $Label }
$tokens=$null; $errors=$null
$ast=[Management.Automation.Language.Parser]::ParseFile((Join-Path $root 'Build-OpalSuite.ps1'),[ref]$tokens,[ref]$errors)
Check (-not $errors.Count) 'Build script parses'
foreach($name in @('Get-OpalPeExports','Assert-OpalExportContract')) {
    $function=$ast.Find({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq $name},$true)
    if(-not $function){throw "Production function missing: $name"}
    . ([scriptblock]::Create($function.Extent.Text))
}
$def=Join-Path $root 'config\OpalExports.def'
$required=@(Get-Content -LiteralPath $def | ForEach-Object {($_ -split ';',2)[0].Trim()} | Where-Object {$_ -and $_ -ne 'EXPORTS'} | ForEach-Object {($_ -split '\s+')[0]})
Check ($required.Count -eq 7 -and ($required | Sort-Object -Unique).Count -eq 7) 'Seven unique Windhawk and XAML exports'
# Execute the production sourceInputs value independently of compilation. The
# existing acceptance gate consumes these ordinary path/sha256 entries.
$receiptAst=$ast.Find({param($node) $node -is [Management.Automation.Language.HashtableAst] -and
    @($node.KeyValuePairs | Where-Object {$_.Item1.Value -eq 'sourceInputs'}).Count -eq 1},$true)
if(-not $receiptAst){throw 'Build receipt sourceInputs expression missing'}
$sourceInputValue=($receiptAst.KeyValuePairs | Where-Object {$_.Item1.Value -eq 'sourceInputs'}).Item2.Extent.Text
$sourceRoot=Join-Path $root 'mod\visual-clones'
$exportDefinition=$def
$buildScriptPath=Join-Path $root 'Build-OpalSuite.ps1'
$sourceInputs=. ([scriptblock]::Create($sourceInputValue))
foreach($path in @($def,$buildScriptPath)) {
    $inputEntry=@($sourceInputs | Where-Object path -eq $path)
    Check ($inputEntry.Count -eq 1 -and $inputEntry[0].sha256 -eq (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash) "Acceptance sourceInputs hashes $path"
}
$scratch=Join-Path $root 'build\export-contract-test'
New-Item -ItemType Directory -Path $scratch -Force | Out-Null
foreach($runtime in @('libc++.whl','libunwind.whl')) {
    Copy-Item -LiteralPath (Join-Path 'C:\Program Files\Windhawk\ModsRuntime\64' $runtime) -Destination (Join-Path $scratch $runtime) -Force
}
$source=Join-Path $scratch 'fixture.cpp'
@'
#include <windows.h>
void* InternalWhModPtr;
struct Initializer { Initializer(){InternalWhModPtr=(void*)0x1234;} } initialize;
int Wh_ModInit(){return 1;}
void Wh_ModAfterInit(){}
void Wh_ModBeforeUninit(){}
void Wh_ModUninit(){}
void Wh_ModSettingsChanged(){}
extern "C" HRESULT DllGetClassObject(REFCLSID,REFIID,void**){return CLASS_E_CLASSNOTAVAILABLE;}
int UnusedInternalFunction(){return 42;}
'@ | Set-Content -LiteralPath $source -Encoding utf8
$clang='C:\Program Files\Windhawk\Compiler\bin\clang++.exe'
$requiredDll=Join-Path $scratch 'required.dll'
$legacyDll=Join-Path $scratch 'legacy.dll'
$common=@('-std=c++23','-Os','-shared','-target','x86_64-w64-mingw32','-flto','-Wl,--gc-sections','-Wl,--no-insert-timestamp',$source)
& $clang @common $def '-Wl,--exclude-all-symbols' '-o' $requiredDll
Check ($LASTEXITCODE -eq 0) 'Required fixture links'
& $clang @common '-Wl,--export-all-symbols' '-o' $legacyDll
Check ($LASTEXITCODE -eq 0) 'Legacy fixture links'
$pe=Get-OpalPeExports $requiredDll
Assert-OpalExportContract $pe $required Required
Check ($pe.Exports.Count -eq 7) 'Actual PE exports satisfy exact required contract'
$legacy=Get-OpalPeExports $legacyDll
Assert-OpalExportContract $legacy $required LegacyAllSymbols
Check ($legacy.Exports.Count -gt 7) 'Export-all retains extra internal fixture exports'
Reject {Assert-OpalExportContract $legacy $required Required} 'Unexpected exports rejected'
$missing=$pe | ConvertTo-Json -Depth 6 | ConvertFrom-Json
$missing.Exports=@($missing.Exports | Where-Object Name -ne '_Z10Wh_ModInitv')
Reject {Assert-OpalExportContract $missing $required Required} 'Missing exact lifecycle export rejected'
$wrong=$pe | ConvertTo-Json -Depth 6 | ConvertFrom-Json
($wrong.Exports | Where-Object Name -eq 'InternalWhModPtr').Characteristics=0x40000000
Reject {Assert-OpalExportContract $wrong $required Required} 'Read-only engine state export rejected'
$noEntry=$pe | ConvertTo-Json -Depth 6 | ConvertFrom-Json
$noEntry.EntryPointRva=0
Reject {Assert-OpalExportContract $noEntry $required Required} 'Missing CRT initialization entrypoint rejected'
$fake=Join-Path $scratch 'strings-only.dll'
[IO.File]::WriteAllText($fake,($required -join ' '))
Reject {Get-OpalPeExports $fake} 'Names embedded as text are not exports'
$truncated=Join-Path $scratch 'truncated.dll'
[IO.File]::WriteAllBytes($truncated,[IO.File]::ReadAllBytes($requiredDll)[0..79])
Reject {Get-OpalPeExports $truncated} 'Truncated PE rejected'
# Execute only our tiny fixture in an owned test process, never the Opal DLL.
$loaderSource=Join-Path $scratch 'loader.cpp'
@'
#include <windows.h>
int wmain(int argc,wchar_t**argv){
 if(argc!=2)return 1;
 HMODULE m=LoadLibraryW(argv[1]); if(!m)return 2;
 const char* names[]={"_Z10Wh_ModInitv","_Z15Wh_ModAfterInitv","_Z18Wh_ModBeforeUninitv","_Z12Wh_ModUninitv","_Z21Wh_ModSettingsChangedv","DllGetClassObject"};
 for(auto n:names)if(!GetProcAddress(m,n))return 3;
 auto p=(void**)GetProcAddress(m,"InternalWhModPtr");
 if(!p||*p!=(void*)0x1234)return 4;
 *p=(void*)0x5678; if(*p!=(void*)0x5678)return 5;
 FreeLibrary(m);return 0;
}
'@ | Set-Content -LiteralPath $loaderSource -Encoding utf8
$loader=Join-Path $scratch 'loader.exe'
& $clang '-Os' '-target' 'x86_64-w64-mingw32' '-municode' $loaderSource '-o' $loader
Check ($LASTEXITCODE -eq 0) 'Fixture loader compiles'
& $loader $requiredDll
Check ($LASTEXITCODE -eq 0) 'Explicit exports preserve static initialization, dynamic lookup and writable engine pointer'
if($CandidateDll){
    $candidate=Get-OpalPeExports $CandidateDll
    Assert-OpalExportContract $candidate $required Required
    Check ($candidate.Exports.Count -eq 7) 'Production candidate PE contract verified without loading it'
}
[pscustomobject]@{Passed=$true;Cases=$script:cases;Evidence='Actual production PE parser and export validator; paired fixture links; malformed/missing/unexpected exports; isolated fixture initializer/lookup/data-write test. No Opal DLL load or Explorer interaction.'}
