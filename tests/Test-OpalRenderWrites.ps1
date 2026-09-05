[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$source=[IO.File]::ReadAllText((Join-Path $root 'mod\visual-clones\maxwell-taskbar-system-info.wh.cpp'))
function Extract([string]$Start,[string]$End){
    $a=$source.IndexOf($Start);$b=$source.IndexOf($End,$a)
    if($a -lt 0 -or $b -lt 0){throw "Production boundary missing: $Start"}
    return $source.Substring($a,$b-$a)
}
$functions=(Extract 'bool SameColor(' 'void ResetVisualWriteCache(')+
    (Extract 'template <typename T>' 'AlertLevel EvaluateAlert(')+
    (Extract 'std::optional<Color> ParseColor(' '// Folds the configured text opacity')+
    (Extract 'Color AlertColor(' 'AlertLevel OverallAlert(')+
    (Extract 'void SetTextIfChanged(' 'template <typename F>')
$tile=Extract 'void CcUpdateTile(' 'void CcUpdateTrace('
$mirror=Extract 'void UpdatePerformanceMirror(' 'void UpdateWidgetText()'
# Execute the exact production entry statements through the snapshot gate.
# Later XAML rendering is covered by the complete mirror/tile functions below.
$entry=(Extract 'void UpdateWidgetText()' '    AlertLevel previousCpuTemperatureAlert')+'}'
$prefix=@'
#include <algorithm>
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <cwchar>
#include <cwctype>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
struct Color {uint8_t A=0,R=0,G=0,B=0;};
enum class AlertLevel {Normal,Warning,Critical};
enum class Visibility {Visible,Collapsed};
int brushAllocations=0;
struct SolidColorBrush {
    bool valid=false;::Color color{};
    SolidColorBrush()=default;
    SolidColorBrush(::Color c):valid(true),color(c){++brushAllocations;}
    explicit operator bool() const{return valid;}
    ::Color Color() const{return color;}
};
struct Brush {SolidColorBrush solid;template<class T>T try_as() const{return solid;}};
struct UiState {
    std::wstring text,name;Brush brush;::Visibility visible=::Visibility::Visible;
    int textWrites=0,colorWrites=0,nameWrites=0,visibilityWrites=0;
};
struct Element {
    std::shared_ptr<UiState> state;
    Element()=default;Element(std::nullptr_t){};
    explicit Element(bool present){if(present)state=std::make_shared<UiState>();}
    explicit operator bool()const{return bool(state);}
    std::wstring Text() const{return state->text;}
    void Text(const std::wstring& v){state->text=v;++state->textWrites;}
    Brush Foreground() const{return state->brush;}
    void Foreground(SolidColorBrush b){state->brush.solid=b;++state->colorWrites;}
    ::Visibility Visibility() const{return state->visible;}
    void Visibility(::Visibility v){state->visible=v;++state->visibilityWrites;}
};
using TextBlock=Element;using FrameworkElement=Element;
namespace winrt::Windows::UI::Xaml::Automation {
struct AutomationProperties {
    static std::wstring GetName(Element e){return e.state->name;}
    static void SetName(Element e,const std::wstring& v){e.state->name=v;++e.state->nameWrites;}
};}
struct ModSettings {std::wstring criticalColor=L"#FF6B6B",warningColor=L"#FFB900",graphColor=L"#78A8FF";};
struct MetricsSnapshot {double cpu=2,ram=40;};
struct PerformanceMirrorSlot {Element widget{true},cpuText{true},ramText{true};};
std::vector<PerformanceMirrorSlot> g_performanceMirrors;
bool g_mirrorDetailed=false;
AlertLevel g_cpuUsageAlert=AlertLevel::Normal,g_ramAlert=AlertLevel::Normal;
std::wstring FormatPercent(double v){return std::to_wstring(static_cast<int>(v))+L"%";}
std::wstring FormatLoadPercent(double v){return FormatPercent(v)+L" load";}
std::wstring FormatUsedPercent(double v){return FormatPercent(v)+L" used";}
struct Rectangle {bool valid=false;double width=0;explicit operator bool()const{return valid;}void Width(double v){width=v;}void Fill(SolidColorBrush){} };
struct CommandCenterTile {Element value{true},detail{true},badge{true};Rectangle fill;};
constexpr double kCcTileBarWidth=100;
Color CcValueColor(AlertLevel,const ModSettings&){return Color{};}
Color CcHealthColor(AlertLevel,const ModSettings&){return Color{};}
Element g_widget{true};bool g_unloading=false,available=true;uint64_t sequence=1,g_lastRenderedMetricsSequence=1;
int settingsCopies=0;
ModSettings CurrentSettings(){++settingsCopies;return {};}
bool GetLatestMetrics(MetricsSnapshot&,uint64_t& out){out=sequence;return available;}
int checks=0;
void Check(bool value,const char* message){++checks;if(!value){std::fprintf(stderr,"%s\n",message);throw std::runtime_error(message);}}
'@
$main=@'
int main(){try{
 ModSettings settings;MetricsSnapshot snapshot;g_performanceMirrors.emplace_back();auto& slot=g_performanceMirrors[0];
 UpdatePerformanceMirror(snapshot,settings);
 Check(brushAllocations==2&&slot.cpuText.state->colorWrites==1&&slot.widget.state->nameWrites==1,"first mirror populates colors/accessibility");
 auto cpuText=slot.cpuText.state->text;auto ramText=slot.ramText.state->text;auto name=slot.widget.state->name;
 for(int i=0;i<20;++i)UpdatePerformanceMirror(snapshot,settings);
 Check(brushAllocations==2&&slot.cpuText.state->colorWrites==1&&slot.ramText.state->colorWrites==1,"unchanged mirrors allocate no brushes and write no foregrounds");
 Check(slot.cpuText.state->textWrites==1&&slot.ramText.state->textWrites==1&&slot.widget.state->nameWrites==1,"unchanged mirrors write no text or accessibility names");
 snapshot.cpu=7;UpdatePerformanceMirror(snapshot,settings);
 Check(slot.cpuText.state->text!=cpuText&&slot.ramText.state->text==ramText&&slot.widget.state->name!=name,"changed CPU updates visible/accessibility values at next refresh");
 g_cpuUsageAlert=AlertLevel::Warning;UpdatePerformanceMirror(snapshot,settings);
 Check(brushAllocations==3&&SameColor(slot.cpuText.Foreground().solid.Color(),AlertColor(AlertLevel::Warning,settings)),"alert transition preserves warning color");
 settings.warningColor=L"#123456";UpdatePerformanceMirror(snapshot,settings);
 Check(brushAllocations==4&&slot.cpuText.Foreground().solid.Color().R==0x12,"theme/color setting change updates same alert state");
 settings.warningColor=L"invalid";UpdatePerformanceMirror(snapshot,settings);
 Check(SameColor(slot.cpuText.Foreground().solid.Color(),MakeColor(255,255,185,0)),"invalid color preserves configured fallback");
 slot.cpuText.state->brush.solid.valid=false;int before=brushAllocations;UpdatePerformanceMirror(snapshot,settings);
 Check(brushAllocations==before+1&&slot.cpuText.Foreground().solid.valid,"null or non-solid foreground is safely replaced");
 g_cpuUsageAlert=AlertLevel::Critical;settings.criticalColor=L"#80112233";UpdatePerformanceMirror(snapshot,settings);
 Check(SameColor(slot.cpuText.Foreground().solid.Color(),MakeColor(0x80,0x11,0x22,0x33)),"critical color retains configured alpha");
 g_cpuUsageAlert=AlertLevel::Normal;settings.graphColor=L"invalid";UpdatePerformanceMirror(snapshot,settings);
 Check(SameColor(slot.cpuText.Foreground().solid.Color(),MakeColor(255,0x78,0xA8,255)),"alert release preserves normal fallback color");
 g_mirrorDetailed=true;UpdatePerformanceMirror(snapshot,settings);
 Check(slot.cpuText.state->text.find(L"load")!=std::wstring::npos,"detailed mode remains current without changing cadence");
 Element absent;before=brushAllocations;SetTextColorIfChanged(absent,Color{});SetAutomationNameIfChanged(absent,L"name");
 Check(brushAllocations==before,"absent elements do no brush work");
 CommandCenterTile tile;
 auto refresh=[&](const std::wstring& badge){CcUpdateTile(tile,L"7%",L"detail",badge,AlertLevel::Normal,0.3,true,AlertLevel::Normal,settings);};
 refresh(L"");int values=tile.value.state->textWrites,details=tile.detail.state->textWrites,visibility=tile.badge.state->visibilityWrites;
 refresh(L"");
 Check(tile.value.state->textWrites==values&&tile.detail.state->textWrites==details&&tile.badge.state->visibilityWrites==visibility,"unchanged open-panel tile text/visibility is not assigned again");
 refresh(L"42C");
 Check(tile.badge.state->text==L"42C"&&tile.badge.Visibility()==Visibility::Visible,"new badge becomes visible immediately");
 refresh(L"");Check(tile.badge.Visibility()==Visibility::Collapsed&&tile.badge.state->text.empty(),"removed badge collapses immediately");
 UpdateWidgetText();Check(settingsCopies==0,"duplicate snapshot does not lock/copy settings");
 available=false;UpdateWidgetText();Check(settingsCopies==0,"missing snapshot does not copy settings");
 available=true;sequence=2;UpdateWidgetText();Check(settingsCopies==1,"fresh snapshot still obtains current settings");
 std::printf("%d production renderer assertions passed.\n",checks);return 0;
 }catch(...){return 1;}}
'@
$out=Join-Path $root 'build\render-writes-test'
New-Item -ItemType Directory -Path $out -Force|Out-Null
$cpp=Join-Path $out 'render.cpp';$exe=Join-Path $out 'render.exe'
[IO.File]::WriteAllText($cpp,$prefix+"`n"+$functions+"`n"+$tile+"`n"+$mirror+"`n"+$entry+"`n"+$main)
& 'C:\Program Files\Windhawk\Compiler\bin\clang++.exe' -std=c++20 -static $cpp -o $exe
if($LASTEXITCODE -ne 0){throw 'Production renderer regression compilation failed.'}
& $exe
if($LASTEXITCODE -ne 0){throw 'Production renderer regression failed.'}
[pscustomobject]@{passed=$true;cases=18;evidence='Complete production mirror/tile update and unchanged-value helpers with counted XAML stubs, plus exact renderer entry statements through the snapshot gate. Preserves values, alert/theme changes, missing/non-solid brushes and visibility. No live UI or performance claim.'}
