[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$source=Get-Content -LiteralPath (Join-Path $root 'mod\visual-clones\maxwell-opal-media.wh.cpp') -Raw
function Extract($begin,$end){$a=$source.IndexOf($begin);$b=$source.IndexOf($end,$a);if($a -lt 0 -or $b -lt 0){throw 'Production function missing.'};$source.Substring($a,$b-$a)}
$snapshot=Extract 'struct MediaSnapshot {' 'struct ButtonEventTokens {'
$functions=Extract 'template <typename Loader>' 'DWORD WINAPI MediaWorker(void*) {'
$prefix=@'
#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
namespace OpalPerformanceDiagnostics {struct Stamp{};Stamp Begin(){return {};}}
namespace winrt {int to_hresult(){return -1;}}
int mode=0,invalidCalls=0,emptyPublishes=0,binds=0,artReads=0,checks=0;
void Check(bool ok,const char* what){++checks;if(!ok)throw std::runtime_error(what);}
void Wh_Log(const wchar_t*,int){}
uint64_t GetTickCount64(){return 42;}
struct Time {int64_t value;int64_t count(){return value;}};
struct Controls {explicit operator bool()const{return mode!=5;}bool IsPreviousEnabled(){return true;}bool IsPlayPauseToggleEnabled(){return true;}bool IsNextEnabled(){return false;}};
enum class GlobalSystemMediaTransportControlsSessionPlaybackStatus {Playing,Paused};
struct Properties {explicit operator bool()const{return mode!=1;}std::wstring Title(){if(mode==1){++invalidCalls;throw std::runtime_error("null properties");}return L"Song";}int Thumbnail(){++artReads;return 1;}};
struct Playback {explicit operator bool()const{return mode!=2;}GlobalSystemMediaTransportControlsSessionPlaybackStatus PlaybackStatus(){if(mode==2){++invalidCalls;throw std::runtime_error("null playback");}return GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing;}::Controls Controls(){return {};}};
struct Timeline {explicit operator bool()const{return mode!=3;}Time StartTime(){if(mode==3){++invalidCalls;throw std::runtime_error("null timeline");}return {10};}Time Position(){return {20};}Time EndTime(){return {mode==4?10:100};}};
struct Session {explicit operator bool()const{return mode!=6;}Properties TryGetMediaPropertiesAsync(){if(mode==7)throw std::runtime_error("provider failure");return {};}Playback GetPlaybackInfo(){return {};}Timeline GetTimelineProperties(){return {};}std::wstring SourceAppUserModelId(){return L"Provider";}};
Session PickSession(){return {};}void BindSession(Session){++binds;}
Properties AwaitMediaOperation(Properties value,int){return value;}
std::wstring BuildSmartDescription(Properties,const std::wstring&){return L"Artist";}
std::vector<uint8_t> ReadArtwork(int){return {1,2};}
int g_workerStop=0;std::mutex g_mediaMutex;std::atomic<bool> g_artworkRequested{true};
'@
$middle=@'
MediaSnapshot g_snapshot;
void PublishEmptySnapshot(){++emptyPublishes;auto seq=g_snapshot.sequence;g_snapshot={};g_snapshot.sequence=seq+1;}
'@
$main=@'
int main(){try{
 mode=0;Check(RefreshMediaSnapshot()&&g_snapshot.hasSession&&g_snapshot.canSeek,"normal provider snapshot");
 mode=1;Check(!RefreshMediaSnapshot()&&!g_snapshot.hasSession&&emptyPublishes==1,"missing properties clears stale controls");
 mode=0;RefreshMediaSnapshot();mode=2;Check(!RefreshMediaSnapshot()&&!g_snapshot.hasSession&&emptyPublishes==2,"missing playback clears stale controls");
 mode=3;Check(RefreshMediaSnapshot()&&g_snapshot.hasSession&&!g_snapshot.canSeek&&g_snapshot.title==L"Song"&&g_snapshot.canToggle,"no timeline preserves stream controls without seeking");
 Check(g_snapshot.start100ns==0&&g_snapshot.position100ns==0&&g_snapshot.end100ns==0,"no stale timeline survives");
 mode=4;Check(RefreshMediaSnapshot()&&!g_snapshot.canSeek,"empty timeline range cannot seek");
 mode=5;Check(RefreshMediaSnapshot()&&g_snapshot.hasSession&&!g_snapshot.canPrevious&&!g_snapshot.canToggle&&!g_snapshot.canNext,"missing optional controls does not advertise unsupported commands");
 mode=6;Check(RefreshMediaSnapshot()&&!g_snapshot.hasSession,"no session clears snapshot without failed retry");
 mode=7;Check(!RefreshMediaSnapshot()&&!g_snapshot.hasSession,"provider exception clears snapshot and requests retry");
 mode=0;Check(RefreshMediaSnapshot()&&g_snapshot.canSeek,"provider recovery restores current seek range");
 Check(invalidCalls==0,"no method called through null provider interface");
 g_artworkRequested=false;auto before=artReads;Check(RefreshMediaSnapshot()&&!g_snapshot.artwork&&artReads==before,"provider guards preserve artwork demand suppression");
 std::printf("%d actual RefreshMediaSnapshot provider assertions passed.\n",checks);return 0;
 }catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}}
'@
$out=Join-Path $root 'build\media-provider-nulls'
New-Item -ItemType Directory -Path $out -Force|Out-Null
$cpp=Join-Path $out 'provider.cpp';$exe=Join-Path $out 'provider.exe'
[IO.File]::WriteAllText($cpp,$prefix+"`n"+$snapshot+"`n"+$middle+"`n"+$functions+"`n"+$main)
& 'C:\Program Files\Windhawk\Compiler\bin\clang++.exe' -std=c++20 -target x86_64-w64-mingw32 -static $cpp -o $exe
if($LASTEXITCODE){throw 'Provider regression compilation failed.'}
& $exe
if($LASTEXITCODE){throw 'Provider regression failed.'}
[pscustomobject]@{passed=$true;cases=12;scope='Extracted production RefreshMediaSnapshot with disappearing/null provider interfaces, recovery and artwork demand.'}
