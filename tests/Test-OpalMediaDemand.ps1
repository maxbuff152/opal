[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$source=[IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\maxwell-opal-media.wh.cpp'))
function Extract([string]$a,[string]$b){$start=$source.IndexOf($a);$end=$source.IndexOf($b,$start);if($start -lt 0 -or $end -lt 0){throw 'Missing production source boundary'};$source.Substring($start,$end-$start)}
$art=Extract 'template <typename Loader>' 'bool RefreshMediaSnapshot() {'
$mirror=Extract 'void UpdateMediaMirror() {' 'bool InjectWidget(FrameworkElement taskbarFrame) {'
$prefix=@'
#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
int writes=0,names=0,positions=0,removes=0,injects=0,checks=0;
struct Text {bool valid=true;explicit operator bool()const{return valid;}void TextValue(const std::wstring&){++writes;}};
struct Frame {int get(){return 1;}};
struct Slot {bool widget=true;Text title{},status{};Frame taskbarFrame;int window=1;bool metadataRendered=false;uint64_t renderedSequence=0;bool renderedDetailed=false;};
namespace OpalControl {enum class MonitorTarget {Primary,Both};}
namespace winrt::Windows::UI::Xaml::Automation {struct AutomationProperties {static void SetName(bool,const std::wstring&){++names;}};}
OpalControl::MonitorTarget g_monitorTarget=OpalControl::MonitorTarget::Both;
struct {bool hasSession=true,playing=true;std::wstring title=L"Title",description=L"Description";} g_uiSnapshot;
struct {bool hideWithoutSession=true;} g_settings;
std::vector<Slot> g_mediaMirrors(1);bool g_mirrorDetailed=true;uint64_t g_uiSequence=1;
void RemoveMediaMirrorVisuals(bool=false){++removes;}
void InjectMediaMirror(int,int){++injects;}
void PositionMediaMirror(Slot&){++positions;}
#define Text(...) TextValue(__VA_ARGS__)
void Check(bool condition,const char* label){++checks;if(!condition){std::fprintf(stderr,"%s\n",label);throw std::runtime_error(label);}}
'@
$main=@'
int main(){try{
 int reads=0;auto load=[&]{++reads;return std::vector<uint8_t>{1,2};};
 std::shared_ptr<const std::vector<uint8_t>> empty;
 auto art=ReadRequestedArtwork(false,false,empty,load);Check(!art&&reads==0,"disabled artwork never invokes provider");
 art=ReadRequestedArtwork(true,false,empty,load);Check(art&&reads==1,"enabled artwork fetched");
 auto same=ReadRequestedArtwork(true,true,art,load);Check(same==art&&reads==1,"same identity shares existing bytes");
 auto disabled=ReadRequestedArtwork(false,true,art,load);Check(!disabled&&reads==1,"disabled ignores retained previous artwork");
 auto changed=ReadRequestedArtwork(true,false,art,load);Check(changed!=art&&reads==2,"changed identity fetches new artwork");
 auto missing=ReadRequestedArtwork(true,false,empty,[]{return std::vector<uint8_t>{};});Check(!missing,"missing art does not allocate retained empty vector");
 auto retry=ReadRequestedArtwork(true,true,missing,load);Check(retry&&reads==3,"later artwork event retries previously unavailable artwork");
 UpdateMediaMirror();Check(writes==2&&names==1&&positions==1,"initial mirror metadata rendered");
 for(int i=0;i<100;++i)UpdateMediaMirror();Check(writes==2&&names==1&&positions==101,"unchanged snapshots skip metadata but preserve geometry recovery");
 ++g_uiSequence;UpdateMediaMirror();Check(writes==4&&names==2,"new snapshot updates mirror");
 g_mirrorDetailed=false;UpdateMediaMirror();Check(writes==6&&names==3,"display style change updates unchanged snapshot");
 g_mediaMirrors[0].metadataRendered=false;UpdateMediaMirror();Check(writes==8,"recreated mirror updates unchanged snapshot");
 g_uiSnapshot.hasSession=false;UpdateMediaMirror();Check(removes==1&&writes==8,"hidden no-session mirror avoids metadata work");
 g_monitorTarget=OpalControl::MonitorTarget::Primary;UpdateMediaMirror();Check(removes==2&&writes==8,"disabled mirror avoids metadata work");
 std::printf("%d media demand assertions passed.\n",checks);return 0;
 }catch(...){return 1;}}
'@
$out=Join-Path $root 'build\media-demand-test'
New-Item -ItemType Directory -Path $out -Force|Out-Null
$cpp=Join-Path $out 'demand.cpp';$exe=Join-Path $out 'demand.exe'
[IO.File]::WriteAllText($cpp,$prefix+"`n"+$art+"`n"+$mirror+"`n"+$main)
& 'C:\Program Files\Windhawk\Compiler\bin\clang++.exe' -std=c++20 -target x86_64-w64-mingw32 -static $cpp -o $exe
if($LASTEXITCODE -ne 0){throw 'Media demand regression compilation failed.'}
& $exe
if($LASTEXITCODE -ne 0){throw 'Media demand regression failed.'}
[pscustomobject]@{passed=$true;cases=14;evidence='Compiled production artwork demand helper and mirror update function: provider suppression/retry/reuse and metadata suppression with geometry recovery preserved.'}
