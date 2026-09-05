// ==WindhawkMod==
// @id              opal-addon-system-info
// @name            Opal Add-on · System Info
// @name:uk-UA      Системний монітор панелі завдань
// @description     A readable CPU/RAM-first taskbar glance with a full hardware command center for Opal.
// @description:uk-UA Компактний монітор CPU, GPU, RAM і VRAM із 60-секундними графіками для панелі завдань Windows 11.
// @version         4.5.0
// @author          Maxbuff152
// @github          https://github.com/Maxbuff152
// @homepage        https://github.com/starychenko/windhawk-taskbar-system-info
// @license         GPL-3.0
// @include         explorer.exe
// @architecture    x86-64
// @compilerOptions -lole32 -loleaut32 -lruntimeobject -lpdh -ldxgi -lshell32 -lpsapi -luser32 -lwtsapi32 -lkernel32 -DWIN32_LEAN_AND_MEAN
// ==/WindhawkMod==

// Taskbar XAML discovery and window-thread marshaling are based on techniques
// from "Multirow taskbar for Windows 11" by Michael Maltsev (m417z):
// https://github.com/ramensoftware/windhawk-mods/blob/main/mods/taskbar-multirow.wh.cpp
// Native GPU temperature collection via D3DKMT follows Taskbar Clock
// Customization by Michael Maltsev (m417z):
// https://github.com/m417z/my-windhawk-mods/commit/861920df6380f4c13abec5d9226362c4725e8362
// Both projects are distributed under the GNU General Public License v3.0.

// ==WindhawkModReadme==
/*
# Opal Add-on · System Info

System Info is a Maxwell-owned Opal component: its product design, maintained
implementation, command center, adaptive layouts, telemetry contract, and
monochrome visual system belong to Maxwell. GPL attribution for upstream
taskbar-discovery and sensor techniques is retained in source and does not
transfer ownership of this component.

A compact, interactive system command center for the far-left free area of the
Windows 11 taskbar. At the coordinated 184-DIP width it gives CPU and RAM the
full lane instead of squeezing four metrics into undersized columns. GPU, VRAM,
history, thermals, and process detail remain one click away in Hardware Command
Center. Wider profiles can still show the full four-metric grid. Click a metric
to open the Hardware Command Center: a live panel with the four metrics as
tiles, CPU and GPU history, thermal headroom, sensor provider, uptime, the
largest memory holders, and one-tap shortcuts to the matching Windows tools.
Drag the taskbar surface to reposition the complete hardware capsule; its
location persists across Explorer and Windhawk reloads.

![Taskbar System Info preview](https://raw.githubusercontent.com/starychenko/windhawk-taskbar-system-info/main/assets/taskbar-system-info.png)

## Hardware Command Center

Clicking any taskbar metric opens a native Windows Flyout. Windows owns the
material, shadow, corner treatment, opening motion, scrolling, focus visuals,
and accessibility. Opal supplies only a neutral content hierarchy built from
WinUI controls, grouped rows, and tabular numerals:

- A state capsule in the header summarizes overall health, with the live scene,
  taskbar mode and refresh cadence underneath.
- Four tiles show CPU, GPU, RAM and VRAM as a large current value, the rolling
  average over the history window, temperature or capacity, and a capacity rail.
- CPU and GPU history traces are drawn with a soft gradient fill so a sustained
  climb reads differently from a momentary spike.
- Detail rows carry thermal headroom, the active temperature providers, the
  selected graphics adapter, uptime with the live process count, and the current
  notification posture.
- The largest memory holders are ranked, so the panel answers "what is using
  this?" without opening Task Manager first.
- Shortcuts open Task Manager, Resource Monitor, Power & battery, Graphics,
  System info, and Storage. A native selector chooses the exact taskbar
  experience mode, and the final row copies a plain-text hardware report.

The panel refreshes from the same render pass that updates the taskbar rows, so
values stay live while it is open and no extra collection work is started. Its
height is capped to the monitor work area and scrolls if a display is short.

The adaptive layout keeps the two most actionable taskbar metrics readable at
compact width and expands into a predictable two-column grid when space is
available. The display unit can be switched between Celsius and Fahrenheit
without changing the Celsius-based warning thresholds used by the collector:

```text
CPU›  10% load  162°F  [history]    RAM›   52% used  16.7/32G
GPU›   4% load  133°F  [history]    VRAM›   9% used   2.1/24G
```

CPU and GPU history uses a fixed 0-100% scale. RAM and VRAM use thin capacity
bars. Fixed-width fields prevent the layout from shifting as values change.
Live load and memory values are blue normally, amber at warning level, and red
at critical level. Temperature values are green when normal, amber at warning,
and red at their critical thresholds. Network and disk activity are
intentionally not collected.

Unlike the performance placeholders in
[Taskbar Clock Customization](https://github.com/ramensoftware/windhawk-mods/blob/main/mods/taskbar-clock-customization.wh.cpp),
this mod does not alter the clock. It uses the free far-left taskbar area for a
stable 2x2 dashboard with rolling graphs, capacity bars and temperature alerts.

## Metrics

- CPU utilization from Windows system time counters.
- RAM usage and capacity from Windows memory status.
- GPU utilization and dedicated or shared GPU-memory usage from Windows PDH
  counters.
- GPU-memory capacity and adapter identity from live D3DKMT enumeration, with
  DXGI as a compatibility fallback. Shared system memory is used when an
  integrated adapter reports no dedicated video memory.
- CPU and GPU temperatures from HWiNFO when available.
- GPU fallback from the Windows display-driver interface (D3DKMT).
- CPU fallback from Windows ACPI thermal zones exposed through PDH.

Metric collection prefers the native `Maxwell.Shell.Core` companion process
through a versioned local shared-memory snapshot. Explorer only renders the
completed snapshot and refreshes optional temperature fallbacks every 30
seconds. If the companion is missing or stale, the existing in-process worker
automatically resumes full collection. If a display-driver restart changes an adapter
LUID or invalidates the active performance counters, the mod refreshes the live
adapter list and rebuilds the counters automatically. A missing VRAM instance
alone does not trigger recovery unless fresh enumeration confirms a LUID change.

The adapter with the most dedicated VRAM is selected automatically. A partial
adapter-name filter is available for multi-GPU systems. GPU usage and VRAM are
matched to the selected live adapter by LUID. Duplicate stale adapters without
a driver name are ignored when a named adapter with the same capacity exists.

## Temperature providers

The **Temperature source** setting provides these modes:

- **Automatic** fills CPU and GPU independently: HWiNFO shared memory first,
  then HWiNFO Gadget Registry, then Windows D3DKMT for a still-missing GPU
  reading and Windows thermal zones for a still-missing CPU reading.
- **HWiNFO automatic** uses only the two HWiNFO interfaces.
- **HWiNFO Shared Memory** uses only `Global\\HWiNFO_SENS_SM2`.
- **HWiNFO Gadget Registry** uses only
  `HKCU\\Software\\HWiNFO64\\VSB`.
- **Windows native** reads GPU temperature from the selected display driver via
  D3DKMT and CPU temperature from the same
  `\\Thermal Zone Information(*)\\Temperature` PDH source as Taskbar Clock
  Customization. It needs no third-party monitor. ACPI platform zones don't
  necessarily represent the CPU package sensor; the optional zone filter and
  average/hottest setting make this fallback explicit and controllable.
- **Disabled** skips temperature collection while keeping every other metric.

HWiNFO is optional and is not bundled with this mod. Shared-memory integration
targets HWiNFO 7.0 or newer, which permits full disclosure of the interface.
Temperature units are classified from HWiNFO's raw unit bytes, independently of
the Windows ANSI code page.
The free HWiNFO64 edition disables shared memory after 12 hours of continuous
use; HWiNFO64 Pro has no such limit. Gadget Registry is a separate HWiNFO
interface. Configure it under **Sensor Settings > HWiNFO Gadget** by enabling
**Report to Gadget** for the desired CPU and GPU temperature readings. If the
selected source is unavailable, temperatures are shown as `--°C`; all other
metrics continue to work. The active provider is written to the Windhawk log
only when it changes, which makes fallback behavior diagnosable without adding
noise every second.

## Compatibility and placement

- Windows 11 64-bit, primary taskbar. x64 is hardware-tested; ARM64 is
  compilation-tested.
- Centered taskbar icons are recommended.
- Enable **Reserve space before the Start button** if the widget overlaps
  left-aligned taskbar buttons.
- The widget is native XAML inside the taskbar and can coexist with Taskbar
  Styler.

## Credits and license

Taskbar discovery and window-thread marshaling follow techniques from
[Multirow taskbar for Windows 11](https://github.com/ramensoftware/windhawk-mods/blob/main/mods/taskbar-multirow.wh.cpp)
by Michael Maltsev (`m417z`). Native GPU temperature collection follows his
[Taskbar Clock Customization implementation](https://github.com/m417z/my-windhawk-mods/commit/861920df6380f4c13abec5d9226362c4725e8362).
Released under GPL-3.0.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- width: 184
  $name: Widget width
  $name:uk-UA: Ширина блока
  $description: "Allowed range: 184-800 pixels. The 184-pixel compact profile fits CPU/RAM without reserving an unused graph lane."
  $description:uk-UA: "Діапазон: 184-800 пікселів. Компактний профіль 184 пікселі вміщує CPU/RAM без порожньої смуги графіка."

- leftOffset: 10
  $name: Left offset
  $name:uk-UA: Відступ зліва

- reserveSpace: true
  $name: Reserve space before the Start button
  $name:uk-UA: Резервувати місце перед кнопкою Пуск
  $description: "Usually not needed when Windows 11 taskbar icons are centered."
  $description:uk-UA: "Для центрованих значків Windows 11 зазвичай не потрібно."

- reserveGap: 8
  $name: Reserved space gap
  $name:uk-UA: Проміжок після блока

- updateInterval: 2
  $name: Update interval
  $name:uk-UA: Інтервал оновлення
  $description: "Metric refresh interval, from 1 to 10 seconds."
  $description:uk-UA: "Від 1 до 10 секунд."

- gamingUpdateInterval: 10
  $name: Fullscreen gaming refresh interval
  $description: "Slower metric collection while a fullscreen app is foreground, from 5 to 30 seconds."

- batterySaverUpdateInterval: 15
  $name: Battery-saver refresh interval
  $description: "Lower-overhead collection while Windows battery saver is active, from 10 to 60 seconds."

- commandCenterEnabled: true
  $name: Hardware command center
  $description: "Click any metric to open a native hardware summary and system-tool launcher."

- performanceAuraEnabled: false
  $name: Performance aura
  $description: "Retained only for settings compatibility. Opal's native-neutral profile permanently disables the old chromatic aura."

- activityRailEnabled: true
  $name: Live activity rail
  $description: "Expanded profiles use four low-profile segments for CPU, GPU, RAM, and VRAM pressure; the compact CPU/RAM profile stays visually quiet."

- focusSceneEngineEnabled: true
  $name: Automatic focus scenes
  $description: "Reuse the existing collector to detect gaming/fullscreen, presentation, and battery-saver states."

- adaptiveOverlapEnabled: true
  $name: Adaptive overlap governor
  $description: "Continuously fit the widget into the live free zone before the centered app dock."

- contentPriorityEnabled: true
  $name: Content priority ladder
  $description: "Prioritize CPU and RAM on the compact taskbar; keep GPU, VRAM, history, and process detail in the command center until more width is available."

- showInlineGraphs: false
  $name: Compact CPU/GPU graphs
  $description: "Keep taskbar rows clean; full CPU/GPU history remains available in the command center."

- experienceMode: auto
  $name: Taskbar experience mode
  $description: "Right-click the hardware widget to cycle modes without adding another process or watcher."
  $options:
  - auto: Automatic
  - balanced: Balanced
  - focus: Focus
  - gaming: Gaming
  - minimal: Minimal

- historySeconds: 60
  $name: Graph history
  $name:uk-UA: Історія графіків
  $description: "CPU and GPU history window, from 15 to 180 seconds."
  $description:uk-UA: "Від 15 до 180 секунд."

- fontSize: 13
  $name: Font size
  $name:uk-UA: Розмір тексту
  $description: "From 9 to 13 pixels."
  $description:uk-UA: "Від 9 до 13 пікселів."

- fontFamily: "Segoe UI Variable Text"
  $name: Font family
  $name:uk-UA: Шрифт

- textColor: "#FFF5F5F7"
  $name: Text color
  $name:uk-UA: Колір тексту
  $description: "#RRGGBB or #AARRGGBB. Leave empty to use the system color."
  $description:uk-UA: "#RRGGBB або #AARRGGBB. Порожнє значення використовує системний колір."

- graphColor: "#D6D6D8"
  $name: Graph and bar color
  $name:uk-UA: Колір графіків і смуг
  $description: "Accent color for CPU/GPU history and memory capacity bars."
  $description:uk-UA: "Стриманий акцент для історії CPU/GPU та смуг памяті."

- safeColor: "#FFA8A8AD"
  $name: Safe temperature color
  $description: "CPU/GPU temperatures below their warning thresholds use this quiet graphite tone."

- warningColor: "#FFC7C7CC"
  $name: Warning color
  $name:uk-UA: Колір попередження

- criticalColor: "#FFF5F5F7"
  $name: Critical color
  $name:uk-UA: Критичний колір

- textOpacity: 96
  $name: Text opacity
  $name:uk-UA: Прозорість тексту
  $description: "From 0 to 100 percent."
  $description:uk-UA: "Від 0 до 100."

- temperatureUnit: fahrenheit
  $name: Temperature display unit
  $description: "Changes only the displayed CPU/GPU temperatures. Safety thresholds remain configured in Celsius."
  $options:
  - celsius: Celsius (°C)
  - fahrenheit: Fahrenheit (°F)

- computeWarningPercent: 75
  $name: CPU/GPU load warning
  $description: "CPU and GPU utilization at or above this percentage turns amber."

- computeCriticalPercent: 90
  $name: CPU/GPU load critical
  $description: "CPU and GPU utilization at or above this percentage turns red."

- cpuWarningTemp: 75
  $name: CPU temperature warning
  $name:uk-UA: Попередження температури CPU

- cpuCriticalTemp: 85
  $name: CPU critical temperature
  $name:uk-UA: Критична температура CPU

- gpuWarningTemp: 80
  $name: GPU temperature warning
  $name:uk-UA: Попередження температури GPU

- gpuCriticalTemp: 90
  $name: GPU critical temperature
  $name:uk-UA: Критична температура GPU

- memoryWarningPercent: 80
  $name: Memory usage warning
  $name:uk-UA: Попередження заповнення памяті

- memoryCriticalPercent: 90
  $name: Critical memory usage
  $name:uk-UA: Критичне заповнення памяті

- gpuAdapter: ""
  $name: GPU adapter filter
  $name:uk-UA: Відеокарта
  $description: "Optional partial adapter name. Empty selects the adapter with the most dedicated VRAM."
  $description:uk-UA: "Необовязкова частина назви. Порожнє значення вибирає адаптер з найбільшим обсягом VRAM."

- temperatureSource: auto
  $name: Temperature source
  $name:uk-UA: Джерело температури
  $description: "Automatic tries both HWiNFO interfaces, then Windows D3DKMT for a missing GPU reading and Windows thermal zones for a missing CPU reading."
  $description:uk-UA: "Автоматичний режим перевіряє обидва інтерфейси HWiNFO, а потім Windows D3DKMT для відсутньої температури GPU та системні термозони Windows для відсутньої температури CPU."
  $options:
  - auto: Automatic
  - hwinfoAuto: HWiNFO automatic
  - sharedMemory: HWiNFO Shared Memory
  - gadgetRegistry: HWiNFO Gadget Registry
  - windowsNative: Windows native (ACPI CPU + D3DKMT GPU)
  - disabled: Disabled
  $options:uk-UA:
  - auto: Автоматично
  - hwinfoAuto: HWiNFO автоматично
  - sharedMemory: HWiNFO Shared Memory
  - gadgetRegistry: HWiNFO Gadget Registry
  - windowsNative: Системні датчики Windows (ACPI CPU + D3DKMT GPU)
  - disabled: Вимкнено

- windowsThermalZoneFilter: ""
  $name: Windows thermal zone filter
  $name:uk-UA: Фільтр системної термозони Windows
  $description: "Optional partial PDH instance name. Empty uses every valid ACPI thermal zone. Applies to the CPU part of Windows native temperature collection."
  $description:uk-UA: "Необов'язкова частина назви екземпляра PDH. Порожнє значення використовує всі коректні термозони ACPI. Застосовується до CPU у системному режимі Windows."

- windowsThermalZoneAggregation: average
  $name: Windows thermal zone aggregation
  $name:uk-UA: Об'єднання системних термозон Windows
  $description: "Average matches Taskbar Clock Customization. Hottest is safer for alert-oriented monitoring."
  $description:uk-UA: "Середня відповідає Taskbar Clock Customization. Найгарячіша краще підходить для моніторингу попереджень."
  $options:
  - average: Average
  - hottest: Hottest
  $options:uk-UA:
  - average: Середня
  - hottest: Найгарячіша

- cpuTempSensor: ""
  $name: CPU temperature sensor filter
  $name:uk-UA: Датчик температури CPU
  $description: "Optional partial HWiNFO sensor name. Empty automatically selects CPU (Tctl/Tdie), CPU Die, or CPU Package."
  $description:uk-UA: "Необов'язкова частина назви HWiNFO. Порожнє значення автоматично вибирає CPU (Tctl/Tdie), CPU Die або CPU Package."

- gpuTempSensor: ""
  $name: GPU temperature sensor filter
  $name:uk-UA: Датчик температури GPU
  $description: "Optional partial HWiNFO sensor name. Empty automatically selects GPU Temperature."
  $description:uk-UA: "Необов'язкова частина назви HWiNFO. Порожнє значення автоматично вибирає GPU Temperature."
*/
// ==/WindhawkModSettings==

#include <windhawk_utils.h>

#include <dxgi.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <psapi.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <windows.h>
#include <wtsapi32.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <deque>
#include <iterator>
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

// Windhawk compiles local source from stdin, so quoted sibling headers aren't
// available during `mod install --file`. Keep this small wire contract inline;
// the standalone header remains the source shared by the optional publisher.
namespace MaxwellShellTelemetry {
inline constexpr wchar_t kMappingName[] =
    L"Local\\Maxwell.Shell.Telemetry.v1";
inline constexpr wchar_t kChangedEventName[] =
    L"Local\\Maxwell.Shell.TelemetryChanged.v1";
inline constexpr wchar_t kStopEventName[] =
    L"Local\\Maxwell.Shell.Core.Stop.v1";
inline constexpr wchar_t kInstanceMutexName[] =
    L"Local\\Maxwell.Shell.Core.Instance.v1";
inline constexpr wchar_t kReaderMappingName[] =
    L"Local\\Maxwell.Shell.TelemetryReader.v1";
inline constexpr wchar_t kReaderWakeEventName[] =
    L"Local\\Maxwell.Shell.TelemetryReaderWake.v1";
inline constexpr std::uint32_t kMagic = 0x3154534d;
inline constexpr std::uint32_t kReaderMagic = 0x3152534d;
inline constexpr std::uint16_t kVersion = 1;
inline constexpr std::uint32_t kMinIntervalMs = 5000;
inline constexpr std::uint32_t kMaxIntervalMs = 30000;
enum MetricFlags : std::uint32_t {
    MetricCpu = 1u << 0,
    MetricRam = 1u << 1,
    MetricGpu = 1u << 2,
    MetricVram = 1u << 3,
};
struct alignas(8) SnapshotV1 {
    volatile LONG64 sequence;
    std::uint32_t magic;
    std::uint16_t version;
    std::uint16_t structSize;
    std::uint32_t flags;
    std::uint32_t publisherPid;
    std::uint32_t sampleIntervalMs;
    std::uint32_t reserved;
    std::uint64_t sampledTickMs;
    double cpuPercent;
    double ramPercent;
    double ramUsedGiB;
    double ramTotalGiB;
    double gpuPercent;
    double vramPercent;
    double vramUsedGiB;
    double vramTotalGiB;
};
static_assert(sizeof(SnapshotV1) == 104);

struct alignas(8) ReaderRequestV1 {
    volatile LONG64 sequence;
    std::uint32_t magic;
    std::uint16_t version;
    std::uint16_t structSize;
    std::uint32_t desiredIntervalMs;
    std::uint32_t readerPid;
    std::uint64_t heartbeatTickMs;
};
static_assert(sizeof(ReaderRequestV1) == 32);
}  // namespace MaxwellShellTelemetry

#undef GetCurrentTime

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.Input.h>
#include <winrt/Windows.UI.Text.h>
#include <winrt/Windows.UI.Xaml.Automation.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Controls.Primitives.h>
#include <winrt/Windows.UI.Xaml.Documents.h>
#include <winrt/Windows.UI.Xaml.Input.h>
#include <winrt/Windows.UI.Xaml.Interop.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Media.Animation.h>
#include <winrt/Windows.UI.Xaml.Shapes.h>
#include <winrt/Windows.UI.Xaml.h>

#include "opal-control.h"
#include "opal-performance-diagnostics.h"

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Controls;
using namespace winrt::Windows::UI::Xaml::Controls::Primitives;
using namespace winrt::Windows::UI::Xaml::Documents;
using namespace winrt::Windows::UI::Xaml::Input;
using namespace winrt::Windows::UI::Xaml::Media;
using namespace winrt::Windows::UI::Xaml::Media::Animation;
using XamlPolyline = winrt::Windows::UI::Xaml::Shapes::Polyline;
using XamlRectangle = winrt::Windows::UI::Xaml::Shapes::Rectangle;
using XamlPolygon = winrt::Windows::UI::Xaml::Shapes::Polygon;
using XamlEllipse = winrt::Windows::UI::Xaml::Shapes::Ellipse;

namespace {

constexpr wchar_t kWidgetName[] = L"OpalSystemInfo";
constexpr wchar_t kMirrorWidgetName[] = L"OpalSystemInfoMirror";
constexpr wchar_t kLegacyWidgetName[] = L"WindhawkTaskbarSystemInfo";
constexpr wchar_t kLegacyMirrorWidgetName[] = L"WindhawkTaskbarSystemInfoMirror";
// One geometry contract across Opal: Media, System Info, Clock/tray, and the
// tools dock all occupy the same 50-DIP visual envelope.
constexpr double kWidgetHeight = 50.0;
constexpr double kCompactHardwareWidth = 184.0;
constexpr double kDualMirrorLaneReserve = 204.0;
constexpr double kContentHorizontalInset = 8.0;
constexpr double kWidgetDragThreshold = 5.0;
constexpr wchar_t kPrimaryWidgetLeftValue[] = L"PerformancePrimaryLeft";
constexpr wchar_t kSecondaryWidgetLeftValue[] = L"PerformanceSecondaryLeft";
constexpr double kRowHeight = 23.0;
constexpr double kRowGap = 4.0;
constexpr double kColumnGap = 14.0;
constexpr double kMetricLabelWidth = 38.0;
constexpr double kMetricUsageWidth = 54.0;
constexpr double kMetricTempWidth = 48.0;
constexpr double kGraphLeftGap = 8.0;
// Deliberately the compute-row widths, not independent values. When these
// drifted apart the two row types shared no column boundary and the widget read
// as two unrelated strips rather than one block.
constexpr double kMemoryLabelWidth = kMetricLabelWidth;
constexpr double kMemoryPercentWidth = kMetricUsageWidth;
constexpr double kGraphHeight = 12.0;
constexpr uint32_t kHwInfoSignature = 0x53695748;  // "HWiS"
constexpr uint32_t kHwInfoTemperatureType = 1;

enum class TemperatureSource {
    Auto,
    HwInfoAuto,
    SharedMemory,
    GadgetRegistry,
    WindowsNative,
    Disabled,
};

enum class ThermalZoneAggregation {
    Average,
    Hottest,
};

enum class TemperatureUnit {
    Celsius,
    Fahrenheit,
};

enum class TemperatureProvider {
    None,
    HwInfoSharedMemory,
    HwInfoGadgetRegistry,
    WindowsD3dkmt,
    WindowsThermalZones,
};

enum class FocusScene {
    Normal,
    Gaming,
    Focus,
    Presentation,
    BatterySaver,
};

enum class ExperienceMode {
    Auto,
    Balanced,
    Focus,
    Gaming,
    Minimal,
};

enum class ContentPriority {
    Full,
    Balanced,
    Essential,
};

struct ModSettings {
    std::wstring fontFamily;
    std::wstring textColor;
    std::wstring graphColor;
    std::wstring safeColor;
    std::wstring warningColor;
    std::wstring criticalColor;
    std::wstring gpuAdapter;
    std::wstring cpuTempSensor;
    std::wstring gpuTempSensor;
    std::wstring windowsThermalZoneFilter;
    TemperatureSource temperatureSource = TemperatureSource::Auto;
    ThermalZoneAggregation windowsThermalZoneAggregation =
        ThermalZoneAggregation::Average;
    TemperatureUnit temperatureUnit = TemperatureUnit::Fahrenheit;
    int width = 184;
    int leftOffset = 10;
    bool reserveSpace = false;
    int reserveGap = 8;
    int updateInterval = 2;
    int gamingUpdateInterval = 10;
    int batterySaverUpdateInterval = 15;
    bool commandCenterEnabled = true;
    bool performanceAuraEnabled = false;
    bool activityRailEnabled = true;
    bool focusSceneEngineEnabled = true;
    bool adaptiveOverlapEnabled = true;
    bool contentPriorityEnabled = true;
    bool showInlineGraphs = false;
    ExperienceMode experienceMode = ExperienceMode::Auto;
    int historySeconds = 60;
    int fontSize = 12;
    int textOpacity = 96;
    int computeWarningPercent = 75;
    int computeCriticalPercent = 90;
    int cpuWarningTemp = 75;
    int cpuCriticalTemp = 85;
    int gpuWarningTemp = 80;
    int gpuCriticalTemp = 90;
    int memoryWarningPercent = 80;
    int memoryCriticalPercent = 90;
};

ModSettings g_settings;
std::mutex g_settingsMutex;
bool g_performanceEnabled = true;
bool g_leanMode = true;
bool g_highContrast = false;
bool g_reducedMotion = false;
bool g_manualLayout = false;
bool g_fullViewOnPrimary = true;
bool g_mirrorDetailed = true;
uint8_t g_widgetBackgroundAlpha = 0x9C;
OpalControl::MonitorTarget g_monitorTarget = OpalControl::MonitorTarget::Primary;
OpalControl::QuarantineState g_quarantine;
std::atomic<bool> g_unloading;
std::atomic<bool> g_uiTornDown;
std::atomic<bool> g_taskbarViewDllLoaded;
std::atomic<HWND> g_taskbarWindow{nullptr};
std::atomic<DWORD> g_taskbarThreadId{0};
std::atomic<ExperienceMode> g_experienceMode{ExperienceMode::Auto};

struct PerformanceMirrorSlot {
    HWND window = nullptr;
    Grid rootGrid{nullptr};
    Grid widget{nullptr};
    FrameworkElement repeater{nullptr};
    double reservedMargin = 0.0;
    TextBlock cpuText{nullptr};
    TextBlock ramText{nullptr};
    Border surfaceBorder{nullptr};
};

[[clang::no_destroy]] Grid g_widget{nullptr};
[[clang::no_destroy]] Grid g_rootGrid{nullptr};
[[clang::no_destroy]] std::vector<PerformanceMirrorSlot> g_performanceMirrors;
[[clang::no_destroy]] FrameworkElement g_taskItemsRepeater{nullptr};
[[clang::no_destroy]] Grid g_cpuRow{nullptr};
[[clang::no_destroy]] Grid g_gpuRow{nullptr};
[[clang::no_destroy]] Grid g_ramRow{nullptr};
[[clang::no_destroy]] Grid g_vramRow{nullptr};
[[clang::no_destroy]] Border g_surfaceBorder{nullptr};
[[clang::no_destroy]] Border g_auraBorder{nullptr};
[[clang::no_destroy]] Storyboard g_auraPulseStoryboard{nullptr};
[[clang::no_destroy]] Grid g_activityRail{nullptr};
[[clang::no_destroy]] XamlRectangle g_cpuActivity{nullptr};
[[clang::no_destroy]] XamlRectangle g_gpuActivity{nullptr};
[[clang::no_destroy]] XamlRectangle g_ramActivity{nullptr};
[[clang::no_destroy]] XamlRectangle g_vramActivity{nullptr};
[[clang::no_destroy]] Flyout g_commandCenterFlyout{nullptr};
double g_reservedMargin = 0.0;
double g_effectiveWidgetWidth = 390.0;
double g_graphWidth = 96.0;
double g_memoryBarWidth = 120.0;
bool g_userLeftLoaded = false;
int g_userLeft = -1;
bool g_widgetDragPending = false;
bool g_widgetDragging = false;
uint32_t g_widgetDragPointerId = 0;
double g_widgetDragStartX = 0.0;
double g_widgetDragStartLeft = 0.0;
[[clang::no_destroy]] DispatcherTimer g_timer{nullptr};
event_token g_timerToken{};
event_token g_rootSizeChangedToken{};
[[clang::no_destroy]]
std::optional<std::list<FrameworkElement::Loaded_revoker>> g_loadedRevokers{
    std::in_place};

[[clang::no_destroy]] TextBlock g_cpuLabel{nullptr};
[[clang::no_destroy]] TextBlock g_cpuUsageText{nullptr};
[[clang::no_destroy]] TextBlock g_cpuTempText{nullptr};
[[clang::no_destroy]] TextBlock g_gpuLabel{nullptr};
[[clang::no_destroy]] TextBlock g_gpuUsageText{nullptr};
[[clang::no_destroy]] TextBlock g_gpuTempText{nullptr};
[[clang::no_destroy]] TextBlock g_ramLabel{nullptr};
[[clang::no_destroy]] TextBlock g_ramPercentText{nullptr};
[[clang::no_destroy]] TextBlock g_ramCapacityText{nullptr};
[[clang::no_destroy]] TextBlock g_vramLabel{nullptr};
[[clang::no_destroy]] TextBlock g_vramPercentText{nullptr};
[[clang::no_destroy]] TextBlock g_vramCapacityText{nullptr};
[[clang::no_destroy]] XamlPolyline g_cpuGraph{nullptr};
[[clang::no_destroy]] XamlPolyline g_gpuGraph{nullptr};
[[clang::no_destroy]] XamlRectangle g_ramTrack{nullptr};
[[clang::no_destroy]] XamlRectangle g_ramFill{nullptr};
[[clang::no_destroy]] XamlRectangle g_vramTrack{nullptr};
[[clang::no_destroy]] XamlRectangle g_vramFill{nullptr};
[[clang::no_destroy]] ScaleTransform g_ramFillScale{nullptr};
[[clang::no_destroy]] ScaleTransform g_vramFillScale{nullptr};
[[clang::no_destroy]] ColumnDefinition g_leftColumn{nullptr};
[[clang::no_destroy]] ColumnDefinition g_gapColumn{nullptr};
[[clang::no_destroy]] ColumnDefinition g_rightColumn{nullptr};

std::deque<double> g_cpuHistory;
std::deque<double> g_gpuHistory;
int g_historyInterval = 0;
int g_historyWindow = 0;
FocusScene g_appliedScene = FocusScene::Normal;
bool g_contextDensityApplied = false;
ContentPriority g_contentPriority = ContentPriority::Full;
double g_lastAvailableWidth = -1.0;

PDH_HQUERY g_pdhQuery = nullptr;
HANDLE g_pdhCompletionEvent = nullptr;
PDH_HCOUNTER g_gpuCounter = nullptr;
PDH_HCOUNTER g_vramCounter = nullptr;
PDH_HCOUNTER g_sharedVramCounter = nullptr;
PDH_HCOUNTER g_thermalZoneCounter = nullptr;
std::chrono::steady_clock::time_point g_nextPdhCounterRetry{};
std::chrono::steady_clock::time_point g_nextPdhRecovery{};
std::chrono::steady_clock::time_point g_nextGpuIdentityCheck{};
uint32_t g_consecutivePdhReadFailures = 0;

struct MetricsSnapshot {
    double cpu = 0.0;
    double ram = 0.0;
    double ramUsedGb = 0.0;
    double ramTotalGb = 0.0;
    double gpu = 0.0;
    bool gpuAvailable = false;
    double vram = 0.0;
    double vramUsedGb = 0.0;
    double vramTotalGb = 0.0;
    bool vramAvailable = false;
    std::optional<double> cpuTemp;
    std::optional<double> gpuTemp;
    TemperatureProvider cpuTempProvider = TemperatureProvider::None;
    TemperatureProvider gpuTempProvider = TemperatureProvider::None;
    FocusScene scene = FocusScene::Normal;
};

std::mutex g_metricsMutex;
MetricsSnapshot g_latestMetrics;
uint64_t g_latestMetricsSequence = 0;
bool g_latestMetricsAvailable = false;
uint64_t g_lastRenderedMetricsSequence = 0;

std::mutex g_metricsWorkerMutex;
std::atomic<bool> g_stopMetricsWorker{false};
HANDLE g_metricsWorkerWakeEvent = nullptr;
[[clang::no_destroy]] std::optional<std::thread> g_metricsWorker;

HANDLE g_externalTelemetryMapping = nullptr;
const MaxwellShellTelemetry::SnapshotV1* g_externalTelemetryView = nullptr;
HANDLE g_externalTelemetryChangedEvent = nullptr;

HANDLE g_readerRequestMapping = nullptr;
MaxwellShellTelemetry::ReaderRequestV1* g_readerRequestView = nullptr;
HANDLE g_readerWakeEvent = nullptr;
uint32_t g_publishedReaderInterval = 0;

void CloseReaderRequest() {
    if (g_readerRequestView) {
        // Zero the heartbeat on the way out so the publisher drops to its idle
        // cadence immediately instead of waiting for the staleness window.
        InterlockedIncrement64(&g_readerRequestView->sequence);
        MemoryBarrier();
        g_readerRequestView->heartbeatTickMs = 0;
        g_readerRequestView->desiredIntervalMs = 0;
        MemoryBarrier();
        InterlockedIncrement64(&g_readerRequestView->sequence);
        UnmapViewOfFile(g_readerRequestView);
        g_readerRequestView = nullptr;
    }
    if (g_readerRequestMapping) {
        CloseHandle(g_readerRequestMapping);
        g_readerRequestMapping = nullptr;
    }
    if (g_readerWakeEvent) {
        SetEvent(g_readerWakeEvent);
        CloseHandle(g_readerWakeEvent);
        g_readerWakeEvent = nullptr;
    }
    g_publishedReaderInterval = 0;
}

// Tells the companion the cadence this widget actually renders at. The scene
// engine already decides that; without this the publisher sampled every 5 s
// regardless, so battery-saver and gaming scenes paid full desktop cost.
void PublishReaderRequest(int intervalSeconds) {
    if (!g_readerRequestView) {
        if (!g_readerRequestMapping) {
            g_readerRequestMapping = OpenFileMappingW(
                FILE_MAP_WRITE, FALSE,
                MaxwellShellTelemetry::kReaderMappingName);
            if (!g_readerRequestMapping) {
                return;
            }
        }
        g_readerRequestView =
            static_cast<MaxwellShellTelemetry::ReaderRequestV1*>(MapViewOfFile(
                g_readerRequestMapping, FILE_MAP_WRITE, 0, 0,
                sizeof(MaxwellShellTelemetry::ReaderRequestV1)));
        if (!g_readerRequestView) {
            CloseHandle(g_readerRequestMapping);
            g_readerRequestMapping = nullptr;
            return;
        }
    }

    uint32_t desired = std::clamp(
        static_cast<uint32_t>(std::max(1, intervalSeconds)) * 1000u,
        MaxwellShellTelemetry::kMinIntervalMs,
        MaxwellShellTelemetry::kMaxIntervalMs);

    InterlockedIncrement64(&g_readerRequestView->sequence);
    MemoryBarrier();
    g_readerRequestView->magic = MaxwellShellTelemetry::kReaderMagic;
    g_readerRequestView->version = MaxwellShellTelemetry::kVersion;
    g_readerRequestView->structSize = sizeof(*g_readerRequestView);
    g_readerRequestView->desiredIntervalMs = desired;
    g_readerRequestView->readerPid = GetCurrentProcessId();
    g_readerRequestView->heartbeatTickMs = GetTickCount64();
    MemoryBarrier();
    InterlockedIncrement64(&g_readerRequestView->sequence);

    // Only nudge the publisher when the request actually changes, so a steady
    // cadence costs one shared-memory write per tick and nothing else.
    if (desired != g_publishedReaderInterval) {
        if (!g_readerWakeEvent) {
            g_readerWakeEvent = OpenEventW(
                EVENT_MODIFY_STATE, FALSE,
                MaxwellShellTelemetry::kReaderWakeEventName);
        }
        if (g_readerWakeEvent) {
            SetEvent(g_readerWakeEvent);
        }
        g_publishedReaderInterval = desired;
    }
}

void EnsureShellCoreProcess() {
    static ULONGLONG lastAttemptMs = 0;
    const ULONGLONG now = GetTickCount64();
    if (lastAttemptMs && now - lastAttemptMs < 15000) {
        return;
    }
    lastAttemptMs = now;
    HANDLE existing = OpenMutexW(SYNCHRONIZE, FALSE,
                                 MaxwellShellTelemetry::kInstanceMutexName);
    if (existing) {
        CloseHandle(existing);
        return;
    }
    wchar_t localAppData[32768]{};
    const DWORD length = GetEnvironmentVariableW(
        L"LOCALAPPDATA", localAppData, ARRAYSIZE(localAppData));
    if (!length || length >= ARRAYSIZE(localAppData)) {
        return;
    }
    std::wstring exe = localAppData;
    exe.append(L"\\Maxwell\\Shell\\Core\\Maxwell.Shell.Core.exe");
    if (GetFileAttributesW(exe.c_str()) == INVALID_FILE_ATTRIBUTES) {
        return;
    }
    STARTUPINFOW startup{sizeof(startup)};
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION process{};
    std::wstring command = L"\"" + exe + L"\"";
    if (CreateProcessW(exe.c_str(), command.data(), nullptr, nullptr, FALSE,
                       CREATE_NO_WINDOW, nullptr, nullptr, &startup,
                       &process)) {
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        Wh_Log(L"Restarted Maxwell.Shell.Core");
    }
}

void CloseExternalTelemetry() {
    if (g_externalTelemetryView) {
        UnmapViewOfFile(g_externalTelemetryView);
        g_externalTelemetryView = nullptr;
    }
    if (g_externalTelemetryMapping) {
        CloseHandle(g_externalTelemetryMapping);
        g_externalTelemetryMapping = nullptr;
    }
    if (g_externalTelemetryChangedEvent) {
        CloseHandle(g_externalTelemetryChangedEvent);
        g_externalTelemetryChangedEvent = nullptr;
    }
}

bool EnsureExternalTelemetry() {
    if (g_externalTelemetryView) {
        return true;
    }
    CloseExternalTelemetry();
    g_externalTelemetryMapping = OpenFileMappingW(
        FILE_MAP_READ, FALSE, MaxwellShellTelemetry::kMappingName);
    if (!g_externalTelemetryMapping) {
        EnsureShellCoreProcess();
        return false;
    }
    g_externalTelemetryView =
        static_cast<const MaxwellShellTelemetry::SnapshotV1*>(MapViewOfFile(
            g_externalTelemetryMapping, FILE_MAP_READ, 0, 0,
            sizeof(MaxwellShellTelemetry::SnapshotV1)));
    if (!g_externalTelemetryView) {
        CloseExternalTelemetry();
        return false;
    }
    g_externalTelemetryChangedEvent = OpenEventW(
        SYNCHRONIZE, FALSE, MaxwellShellTelemetry::kChangedEventName);
    return true;
}

bool TryReadExternalTelemetry(MetricsSnapshot& snapshot) {
    if (!EnsureExternalTelemetry()) {
        return false;
    }

    MaxwellShellTelemetry::SnapshotV1 wire{};
    bool stable = false;
    for (int attempt = 0; attempt < 4; attempt++) {
        LONG64 before = g_externalTelemetryView->sequence;
        if (before & 1) {
            SwitchToThread();
            continue;
        }
        MemoryBarrier();
        std::memcpy(&wire,
                    const_cast<const MaxwellShellTelemetry::SnapshotV1*>(
                        g_externalTelemetryView),
                    sizeof(wire));
        MemoryBarrier();
        LONG64 after = g_externalTelemetryView->sequence;
        if (before == after && !(after & 1)) {
            stable = true;
            break;
        }
    }

    // A torn read means the publisher was mid-write. That is expected under
    // load and says nothing about the mapping, so keep it and retry next tick.
    if (!stable) {
        // A publisher can die after making sequence odd. Do not wait for a
        // stable snapshot to reach recovery. This helper is throttled and
        // checks the instance mutex, so an ordinary concurrent write never
        // starts a duplicate companion or tears down its valid mapping.
        EnsureShellCoreProcess();
        return false;
    }

    // A structurally wrong mapping will never become valid: the publisher was
    // replaced by an incompatible build. Tear it down so the next tick reopens.
    if (wire.magic != MaxwellShellTelemetry::kMagic ||
        wire.version != MaxwellShellTelemetry::kVersion ||
        wire.structSize != sizeof(wire)) {
        Wh_Log(L"External telemetry contract mismatch; closing mapping");
        CloseExternalTelemetry();
        return false;
    }

    // A publisher that exited leaves its last sample behind. Detect the dead
    // process directly instead of waiting for the freshness window to lapse.
    if (wire.publisherPid) {
        HANDLE publisher =
            OpenProcess(SYNCHRONIZE, FALSE, wire.publisherPid);
        if (!publisher) {
            if (GetLastError() == ERROR_INVALID_PARAMETER) {
                Wh_Log(L"External telemetry publisher %u is gone",
                       wire.publisherPid);
                CloseExternalTelemetry();
                EnsureShellCoreProcess();
                return false;
            }
        } else {
            DWORD alive = WaitForSingleObject(publisher, 0);
            CloseHandle(publisher);
            if (alive == WAIT_OBJECT_0) {
                Wh_Log(L"External telemetry publisher %u exited",
                       wire.publisherPid);
                CloseExternalTelemetry();
                EnsureShellCoreProcess();
                return false;
            }
        }
    }

    // Everything below is a live publisher that is merely behind. Fall back to
    // in-process collection for this tick but hold the mapping open, so a busy
    // or briefly descheduled companion does not cost an unmap/reopen cycle.
    constexpr uint32_t requiredFlags =
        MaxwellShellTelemetry::MetricCpu | MaxwellShellTelemetry::MetricRam;
    if ((wire.flags & requiredFlags) != requiredFlags) {
        return false;
    }

    uint64_t now = GetTickCount64();
    uint64_t freshnessLimit =
        std::max<uint64_t>(15000, wire.sampleIntervalMs * 3ull);
    if (wire.sampledTickMs > now || now - wire.sampledTickMs > freshnessLimit) {
        return false;
    }

    snapshot.cpu = std::clamp(wire.cpuPercent, 0.0, 100.0);
    snapshot.ram = std::clamp(wire.ramPercent, 0.0, 100.0);
    snapshot.ramUsedGb = std::max(0.0, wire.ramUsedGiB);
    snapshot.ramTotalGb = std::max(0.0, wire.ramTotalGiB);
    if (wire.flags & MaxwellShellTelemetry::MetricGpu) {
        snapshot.gpu = std::clamp(wire.gpuPercent, 0.0, 100.0);
        snapshot.gpuAvailable = true;
    }
    if (wire.flags & MaxwellShellTelemetry::MetricVram) {
        snapshot.vram = std::clamp(wire.vramPercent, 0.0, 100.0);
        snapshot.vramUsedGb = std::max(0.0, wire.vramUsedGiB);
        snapshot.vramTotalGb = std::max(0.0, wire.vramTotalGiB);
        snapshot.vramAvailable = wire.vramTotalGiB > 0.0;
    }
    return true;
}

std::wstring GetStringSetting(PCWSTR name) {
    return WindhawkUtils::StringSetting::make(name).get();
}

TemperatureSource ParseTemperatureSource(const std::wstring& value) {
    if (value == L"hwinfoAuto") {
        return TemperatureSource::HwInfoAuto;
    }
    if (value == L"sharedMemory") {
        return TemperatureSource::SharedMemory;
    }
    if (value == L"gadgetRegistry") {
        return TemperatureSource::GadgetRegistry;
    }
    if (value == L"windowsNative" || value == L"windowsThermalZones") {
        return TemperatureSource::WindowsNative;
    }
    if (value == L"disabled") {
        return TemperatureSource::Disabled;
    }
    return TemperatureSource::Auto;
}

ThermalZoneAggregation ParseThermalZoneAggregation(
    const std::wstring& value) {
    return value == L"hottest" ? ThermalZoneAggregation::Hottest
                                : ThermalZoneAggregation::Average;
}

TemperatureUnit ParseTemperatureUnit(const std::wstring& value) {
    return value == L"celsius" ? TemperatureUnit::Celsius
                                : TemperatureUnit::Fahrenheit;
}

ExperienceMode ParseExperienceMode(const std::wstring& value) {
    if (value == L"balanced") {
        return ExperienceMode::Balanced;
    }
    if (value == L"focus") {
        return ExperienceMode::Focus;
    }
    if (value == L"gaming") {
        return ExperienceMode::Gaming;
    }
    if (value == L"minimal") {
        return ExperienceMode::Minimal;
    }
    return ExperienceMode::Auto;
}

void LoadSettings() {
    OpalPerformanceDiagnostics::SetEnabled(
        Wh_GetIntSetting(L"everyday.performanceDiagnostics") != 0);
    g_performanceEnabled =
        Wh_GetIntSetting(L"performance.performanceEnabled") != 0;
    g_leanMode = Wh_GetIntSetting(L"everyday.leanMode") != 0;
    g_manualLayout = OpalControl::ReadStringSetting(
        L"everyday.layoutMode", L"automatic") == L"custom";
    g_fullViewOnPrimary = OpalControl::ReadStringSetting(
        L"screens.performanceFullDisplay", L"primary") != L"secondary";
    g_mirrorDetailed = OpalControl::ReadStringSetting(
        L"screens.mirrorStyle", L"detailed") != L"minimal";
    const std::wstring widgetTextSize = OpalControl::ReadStringSetting(
        L"everyday.widgetTextSize", L"standard");
    const std::wstring backgroundStrength = OpalControl::ReadStringSetting(
        L"everyday.widgetBackgroundStrength", L"glass");
    g_widgetBackgroundAlpha = backgroundStrength == L"subtle" ? 0x70
                                  : (backgroundStrength == L"strong"
                                         ? 0xD0 : 0x9C);
#ifdef OPAL_UNIFIED_BUILD
    // The owned Grid style supplies Opal's compositor glass. A second solid
    // fill on PerformanceSurface would cover it with a dark inner capsule.
    g_widgetBackgroundAlpha = 0;
#endif
    g_monitorTarget = OpalControl::ReadMonitorSetting(
        L"screens.performanceMonitor", OpalControl::MonitorTarget::Primary);
    g_highContrast = OpalControl::HighContrast();
    g_reducedMotion = OpalControl::ReducedMotion();
    ModSettings settings;
    settings.fontFamily = GetStringSetting(L"fontFamily");
    settings.textColor = GetStringSetting(L"textColor");
    settings.graphColor = GetStringSetting(L"graphColor");
    settings.safeColor = GetStringSetting(L"safeColor");
    settings.warningColor = GetStringSetting(L"warningColor");
    settings.criticalColor = GetStringSetting(L"criticalColor");
    settings.gpuAdapter = GetStringSetting(L"gpuAdapter");
    settings.temperatureSource =
        ParseTemperatureSource(GetStringSetting(L"temperatureSource"));
    settings.cpuTempSensor = GetStringSetting(L"cpuTempSensor");
    settings.gpuTempSensor = GetStringSetting(L"gpuTempSensor");
    settings.windowsThermalZoneFilter =
        GetStringSetting(L"windowsThermalZoneFilter");
    settings.windowsThermalZoneAggregation = ParseThermalZoneAggregation(
        GetStringSetting(L"windowsThermalZoneAggregation"));
    settings.temperatureUnit =
        ParseTemperatureUnit(GetStringSetting(L"performance.temperatureUnit"));
    settings.width = std::clamp(Wh_GetIntSetting(L"width"), 184, 800);
    settings.leftOffset = std::clamp(Wh_GetIntSetting(L"leftOffset"), 0, 1000);
    settings.reserveSpace = Wh_GetIntSetting(L"reserveSpace") != 0;
    settings.reserveGap = std::clamp(Wh_GetIntSetting(L"reserveGap"), 0, 100);
    settings.updateInterval =
        std::clamp(Wh_GetIntSetting(L"updateInterval"), 1, 10);
    settings.gamingUpdateInterval = std::clamp(
        Wh_GetIntSetting(L"gamingUpdateInterval"),
        std::max(5, settings.updateInterval), 30);
    settings.batterySaverUpdateInterval = std::clamp(
        Wh_GetIntSetting(L"batterySaverUpdateInterval"), 10, 60);
    settings.commandCenterEnabled =
        Wh_GetIntSetting(L"performance.commandCenterEnabled") != 0;
    // Opal v3 has one neutral visual system; the previous scene-colored aura is
    // intentionally unavailable even if an older registry value remains.
    settings.performanceAuraEnabled = false;
    settings.activityRailEnabled =
        Wh_GetIntSetting(L"activityRailEnabled") != 0;
    settings.focusSceneEngineEnabled =
        Wh_GetIntSetting(L"focusSceneEngineEnabled") != 0;
    settings.adaptiveOverlapEnabled =
        Wh_GetIntSetting(L"adaptiveOverlapEnabled") != 0;
    settings.contentPriorityEnabled =
        Wh_GetIntSetting(L"contentPriorityEnabled") != 0;
    settings.showInlineGraphs =
        Wh_GetIntSetting(L"performance.showInlineGraphs") != 0;
    settings.experienceMode =
        ParseExperienceMode(GetStringSetting(L"experienceMode"));
    settings.historySeconds =
        std::clamp(Wh_GetIntSetting(L"historySeconds"), 15, 180);
    settings.fontSize = std::clamp(Wh_GetIntSetting(L"fontSize"), 9, 13);
    settings.textOpacity =
        std::clamp(Wh_GetIntSetting(L"textOpacity"), 0, 100);
    settings.computeWarningPercent =
        std::clamp(Wh_GetIntSetting(L"computeWarningPercent"), 50, 98);
    settings.computeCriticalPercent = std::clamp(
        Wh_GetIntSetting(L"computeCriticalPercent"),
        settings.computeWarningPercent + 1, 100);
    settings.cpuWarningTemp =
        std::clamp(Wh_GetIntSetting(L"cpuWarningTemp"), 40, 95);
    settings.cpuCriticalTemp = std::clamp(
        Wh_GetIntSetting(L"cpuCriticalTemp"), settings.cpuWarningTemp + 1, 105);
    settings.gpuWarningTemp =
        std::clamp(Wh_GetIntSetting(L"gpuWarningTemp"), 40, 105);
    settings.gpuCriticalTemp = std::clamp(
        Wh_GetIntSetting(L"gpuCriticalTemp"), settings.gpuWarningTemp + 1, 115);
    settings.memoryWarningPercent =
        std::clamp(Wh_GetIntSetting(L"memoryWarningPercent"), 50, 98);
    settings.memoryCriticalPercent =
        std::clamp(Wh_GetIntSetting(L"memoryCriticalPercent"),
                   settings.memoryWarningPercent + 1, 100);

    const std::wstring size = OpalControl::ReadStringSetting(
        L"performance.performanceSize", L"standard");
    if (size == L"compact") {
        settings.width = 184;
    } else if (size == L"expanded") {
        settings.width = 390;
    } else {
        settings.width = 260;
    }
    settings.fontSize = widgetTextSize == L"small" ? 10
                            : (widgetTextSize == L"large" ? 13 : 12);
    if (Wh_GetIntSetting(L"advanced.repair.resetWidgetPositions") != 0) {
        Wh_SetIntValue(kPrimaryWidgetLeftValue, -1);
        Wh_SetIntValue(kSecondaryWidgetLeftValue, -1);
        Wh_SetIntValue(L"ForceCanonicalLayout", 1);
        g_manualLayout = false;
        settings.width = 260;
        settings.fontSize = widgetTextSize == L"small" ? 10
                                : (widgetTextSize == L"large" ? 13 : 12);
        OpalControl::ResetPackageQuarantine(L"performance");
    }
    g_userLeftLoaded = false;

    if (settings.fontFamily.empty()) {
        // Segoe UI Variable ships three optical masters. At the 9-13 px this
        // widget uses, Small is the correct one: more open apertures and looser
        // sidebearings than Text, which is cut for 12-28 pt.
        settings.fontFamily = L"Segoe UI Variable Small";
    }
    if (settings.graphColor.empty()) {
        settings.graphColor = L"#D6D6D8";
    }
    if (settings.safeColor.empty()) {
        settings.safeColor = L"#FFA8A8AD";
    }
    if (settings.warningColor.empty()) {
        settings.warningColor = L"#FFC7C7CC";
    }
    if (settings.criticalColor.empty()) {
        settings.criticalColor = L"#FFF5F5F7";
    }

    std::lock_guard lock(g_settingsMutex);
    g_settings = std::move(settings);
    g_experienceMode.store(g_settings.experienceMode);
}

ModSettings CurrentSettings() {
    std::lock_guard lock(g_settingsMutex);
    return g_settings;
}

const wchar_t* CurrentWidgetLeftValue() {
    HWND window = g_taskbarWindow.load();
    bool secondary = window ? OpalControl::IsSecondaryTaskbar(window)
                            : !g_fullViewOnPrimary;
    return secondary ? kSecondaryWidgetLeftValue : kPrimaryWidgetLeftValue;
}

void LoadUserPosition() {
    if (g_userLeftLoaded) {
        return;
    }
    g_userLeft = g_manualLayout
        ? Wh_GetIntValue(CurrentWidgetLeftValue(), -1) : -1;
    g_userLeftLoaded = true;
}

double EffectiveLeftOffset(const ModSettings& settings) {
    if (g_userLeft >= 0) return static_cast<double>(g_userLeft);
    return static_cast<double>(settings.leftOffset);
}

std::wstring ToLower(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch) {
        return static_cast<wchar_t>(std::towlower(ch));
    });
    return value;
}

bool Contains(const std::wstring& text, const std::wstring& needle) {
    return needle.empty() || text.find(needle) != std::wstring::npos;
}

std::wstring FixedAnsiToWide(const char* value, size_t capacity) {
    size_t length = 0;
    while (length < capacity && value[length]) {
        length++;
    }
    if (!length) {
        return {};
    }

    int wideLength = MultiByteToWideChar(CP_ACP, 0, value,
                                         static_cast<int>(length), nullptr, 0);
    if (wideLength <= 0) {
        return {};
    }

    std::wstring result(wideLength, L'\0');
    MultiByteToWideChar(CP_ACP, 0, value, static_cast<int>(length),
                        result.data(), wideLength);
    return result;
}

int CpuTemperatureScore(const std::wstring& sensorName,
                        const std::wstring& label,
                        const std::wstring& preferred) {
    std::wstring sensor = ToLower(sensorName);
    std::wstring reading = ToLower(label);
    std::wstring combined = sensor + L" " + reading;
    std::wstring preferredLower = ToLower(preferred);

    if (!preferredLower.empty()) {
        return Contains(combined, preferredLower) ? 10000 : -1;
    }

    bool gpuSensor = Contains(sensor, L"gpu") || Contains(sensor, L"nvidia") ||
                     Contains(sensor, L"radeon");
    bool cpuSensor =
        Contains(sensor, L"cpu") || Contains(sensor, L"processor") ||
        Contains(sensor, L"ryzen") || Contains(sensor, L"threadripper") ||
        Contains(sensor, L"epyc") || Contains(sensor, L"xeon") ||
        (Contains(sensor, L"intel") && !gpuSensor);
    if (!cpuSensor) {
        return -1;
    }

    if (Contains(reading, L"vrm") || Contains(reading, L"ccd") ||
        Contains(reading, L"iod") || Contains(reading, L"soc") ||
        Contains(reading, L"l3 cache")) {
        return -1;
    }

    if (Contains(reading, L"tctl/tdie")) {
        return 1000;
    }
    if (Contains(reading, L"cpu die (average)")) {
        return 950;
    }
    if (Contains(reading, L"cpu package")) {
        return 900;
    }
    if (Contains(reading, L"package temperature")) {
        return 850;
    }
    if (Contains(reading, L"cpu temperature")) {
        return 800;
    }
    if (Contains(reading, L"core temperatures")) {
        return 700;
    }
    if (Contains(reading, L"temperature")) {
        return 400;
    }

    // Gadget labels follow the selected HWiNFO UI language. A temperature
    // unit check is applied by the registry reader, so a reading that belongs
    // to a recognized CPU sensor remains a safe low-priority fallback even if
    // words such as "temperature" or "package" are localized.
    return 100;
}

int GpuTemperatureScore(const std::wstring& sensorName,
                        const std::wstring& label,
                        const std::wstring& preferred) {
    std::wstring sensor = ToLower(sensorName);
    std::wstring reading = ToLower(label);
    std::wstring combined = sensor + L" " + reading;
    std::wstring preferredLower = ToLower(preferred);

    if (!preferredLower.empty()) {
        return Contains(combined, preferredLower) ? 10000 : -1;
    }

    if (!Contains(sensor, L"gpu") && !Contains(sensor, L"nvidia") &&
        !Contains(sensor, L"radeon")) {
        return -1;
    }

    if (Contains(reading, L"hot spot") || Contains(reading, L"hotspot") ||
        Contains(reading, L"memory") || Contains(reading, L"vram")) {
        return -1;
    }

    if (reading == L"gpu temperature") {
        return 1000;
    }
    if (Contains(reading, L"gpu temperature")) {
        return 950;
    }
    if (Contains(reading, L"gpu core")) {
        return 900;
    }
    if (Contains(reading, L"temperature")) {
        return 500;
    }

    // Prefer the shortest localized label containing the stable GPU acronym.
    // This normally selects labels such as "GPU Temperature" over longer
    // hotspot, junction, or memory-temperature labels.
    if (Contains(reading, L"gpu")) {
        return 400 -
               static_cast<int>(std::min<size_t>(reading.size(), 200));
    }

    // As with CPU readings, the registry path validates the temperature unit
    // before this locale-independent fallback can be selected.
    return 100;
}

// HWiNFO's published shared-memory layout explicitly uses one-byte packing.
#pragma pack(push, 1)
struct HwInfoHeader {
    uint32_t signature;
    uint32_t version;
    uint32_t revision;
    int64_t pollTime;
    uint32_t sensorOffset;
    uint32_t sensorStride;
    uint32_t sensorCount;
    uint32_t readingOffset;
    uint32_t readingStride;
    uint32_t readingCount;
    uint32_t pollingPeriod;
};

struct HwInfoSensorPrefix {
    uint32_t sensorId;
    uint32_t sensorInstance;
    char originalName[128];
    char userName[128];
};

struct HwInfoReadingPrefix {
    uint32_t readingType;
    uint32_t sensorIndex;
    uint32_t readingId;
    char originalLabel[128];
    char userLabel[128];
    char unit[16];
    double value;
};
#pragma pack(pop)

static_assert(sizeof(HwInfoHeader) == 48);
static_assert(offsetof(HwInfoHeader, pollTime) == 12);
static_assert(offsetof(HwInfoHeader, sensorOffset) == 20);
static_assert(sizeof(HwInfoSensorPrefix) == 264);
static_assert(offsetof(HwInfoReadingPrefix, value) == 284);
static_assert(sizeof(HwInfoReadingPrefix) == 292);

bool IsRangeValid(size_t totalSize,
                  uint32_t offset,
                  uint32_t stride,
                  uint32_t count,
                  size_t minimumStride) {
    if (stride < minimumStride || offset > totalSize) {
        return false;
    }
    size_t remaining = totalSize - offset;
    return count <= remaining / stride;
}

std::optional<double> NormalizeTemperature(double value,
                                           std::wstring unitOrFormattedValue) {
    if (!std::isfinite(value)) {
        return std::nullopt;
    }

    std::wstring unit = ToLower(unitOrFormattedValue);
    unit.erase(std::remove_if(unit.begin(), unit.end(), [](wchar_t character) {
                   return std::iswspace(character) != 0;
               }),
               unit.end());

    bool celsius = Contains(unit, L"\u00B0c") || Contains(unit, L"\u2103") ||
                   unit == L"c" || unit == L"celsius";
    bool fahrenheit =
        Contains(unit, L"\u00B0f") || Contains(unit, L"\u2109") ||
        unit == L"f" || unit == L"fahrenheit";
    if (celsius == fahrenheit) {
        return std::nullopt;
    }

    double celsiusValue =
        fahrenheit ? (value - 32.0) * 5.0 / 9.0 : value;
    if (!std::isfinite(celsiusValue) || celsiusValue < -50.0 ||
        celsiusValue > 200.0) {
        return std::nullopt;
    }
    return celsiusValue;
}

constexpr char HwInfoTemperatureUnit(const char* unit, size_t capacity) {
    char result = 0;
    for (size_t i = 0; i < capacity && unit[i]; i++) {
        char candidate = 0;
        if (unit[i] == 'C' || unit[i] == 'c') {
            candidate = 'C';
        } else if (unit[i] == 'F' || unit[i] == 'f') {
            candidate = 'F';
        }
        if (candidate) {
            if (result && result != candidate) {
                return 0;
            }
            result = candidate;
        }
    }
    return result;
}

constexpr char kHwInfoRawCelsiusUnit[] = {
    static_cast<char>(0xB0), 'C', 0};
constexpr char kHwInfoRawFahrenheitUnit[] = {
    static_cast<char>(0xB0), 'F', 0};
static_assert(HwInfoTemperatureUnit(kHwInfoRawCelsiusUnit,
                                    std::size(kHwInfoRawCelsiusUnit)) == 'C');
static_assert(HwInfoTemperatureUnit(kHwInfoRawFahrenheitUnit,
                                    std::size(kHwInfoRawFahrenheitUnit)) ==
              'F');

std::optional<double> NormalizeHwInfoTemperature(double value,
                                                 const char* unit,
                                                 size_t capacity) {
    // HWiNFO stores a raw single-byte degree sign followed by an ASCII unit
    // letter. Decoding the buffer through CP_ACP corrupts that sequence on
    // DBCS locales, so classify the ASCII letter directly from the bytes.
    char unitLetter = HwInfoTemperatureUnit(unit, capacity);
    if (!unitLetter) {
        return std::nullopt;
    }
    return NormalizeTemperature(value, unitLetter == 'F' ? L"F" : L"C");
}

void ReadHwInfoSharedMemory(MetricsSnapshot& snapshot,
                            const ModSettings& settings) {
    HANDLE mapping = OpenFileMappingW(FILE_MAP_READ, FALSE,
                                      L"Global\\HWiNFO_SENS_SM2");
    if (!mapping) {
        return;
    }

    HANDLE mutex = OpenMutexW(SYNCHRONIZE | MUTEX_MODIFY_STATE, FALSE,
                              L"Global\\HWiNFO_SM2_MUTEX");
    bool mutexOwned = false;
    if (mutex) {
        DWORD waitResult = WaitForSingleObject(mutex, 50);
        if (waitResult == WAIT_OBJECT_0 || waitResult == WAIT_ABANDONED) {
            mutexOwned = true;
        } else {
            CloseHandle(mutex);
            CloseHandle(mapping);
            return;
        }
    }

    void* view = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    if (!view) {
        if (mutexOwned) {
            ReleaseMutex(mutex);
        }
        if (mutex) {
            CloseHandle(mutex);
        }
        CloseHandle(mapping);
        return;
    }

    MEMORY_BASIC_INFORMATION memoryInfo{};
    if (VirtualQuery(view, &memoryInfo, sizeof(memoryInfo))) {
        size_t viewOffset = static_cast<const uint8_t*>(view) -
                            static_cast<const uint8_t*>(memoryInfo.BaseAddress);
        size_t mappedSize = viewOffset <= memoryInfo.RegionSize
                                ? memoryInfo.RegionSize - viewOffset
                                : 0;
        if (mappedSize >= sizeof(HwInfoHeader)) {
            // HWiNFO updates this mapping in place. Snapshot the range metadata
            // so every address below uses the same values that passed
            // validation even when the shared mutex isn't accessible from
            // Explorer.
            HwInfoHeader header{};
            std::memcpy(&header, view, sizeof(header));

            if (header.signature == kHwInfoSignature &&
                IsRangeValid(mappedSize, header.sensorOffset,
                             header.sensorStride, header.sensorCount,
                             sizeof(HwInfoSensorPrefix)) &&
                IsRangeValid(mappedSize, header.readingOffset,
                             header.readingStride, header.readingCount,
                             sizeof(HwInfoReadingPrefix))) {
                int bestCpuScore = -1;
                int bestGpuScore = -1;
                const auto* bytes = static_cast<const uint8_t*>(view);

                for (uint32_t i = 0; i < header.readingCount; i++) {
                    HwInfoReadingPrefix reading{};
                    const uint8_t* readingAddress =
                        bytes + header.readingOffset +
                        static_cast<size_t>(i) * header.readingStride;
                    std::memcpy(&reading, readingAddress, sizeof(reading));

                    if (reading.readingType != kHwInfoTemperatureType ||
                        reading.sensorIndex >= header.sensorCount) {
                        continue;
                    }

                    auto value = NormalizeHwInfoTemperature(
                        reading.value, reading.unit, std::size(reading.unit));
                    if (!value) {
                        continue;
                    }

                    HwInfoSensorPrefix sensor{};
                    const uint8_t* sensorAddress =
                        bytes + header.sensorOffset +
                        static_cast<size_t>(reading.sensorIndex) *
                            header.sensorStride;
                    std::memcpy(&sensor, sensorAddress, sizeof(sensor));

                    std::wstring sensorName =
                        FixedAnsiToWide(sensor.originalName,
                                        std::size(sensor.originalName));
                    std::wstring label =
                        FixedAnsiToWide(reading.originalLabel,
                                        std::size(reading.originalLabel));

                    int cpuScore = CpuTemperatureScore(
                        sensorName, label, settings.cpuTempSensor);
                    if (cpuScore > bestCpuScore) {
                        bestCpuScore = cpuScore;
                        snapshot.cpuTemp = *value;
                        snapshot.cpuTempProvider =
                            TemperatureProvider::HwInfoSharedMemory;
                    }

                    int gpuScore = GpuTemperatureScore(
                        sensorName, label, settings.gpuTempSensor);
                    if (gpuScore > bestGpuScore) {
                        bestGpuScore = gpuScore;
                        snapshot.gpuTemp = *value;
                        snapshot.gpuTempProvider =
                            TemperatureProvider::HwInfoSharedMemory;
                    }
                }
            }
        }
    }

    UnmapViewOfFile(view);
    if (mutexOwned) {
        ReleaseMutex(mutex);
    }
    if (mutex) {
        CloseHandle(mutex);
    }
    CloseHandle(mapping);
}

std::optional<std::wstring> ReadRegistryString(HKEY key,
                                                const std::wstring& name) {
    DWORD type = 0;
    DWORD bytes = 0;
    LONG status = RegQueryValueExW(key, name.c_str(), nullptr, &type, nullptr,
                                   &bytes);
    if (status != ERROR_SUCCESS ||
        (type != REG_SZ && type != REG_EXPAND_SZ) || bytes < sizeof(wchar_t)) {
        return std::nullopt;
    }

    std::vector<wchar_t> buffer(bytes / sizeof(wchar_t) + 1, L'\0');
    status = RegQueryValueExW(key, name.c_str(), nullptr, &type,
                              reinterpret_cast<BYTE*>(buffer.data()), &bytes);
    if (status != ERROR_SUCCESS) {
        return std::nullopt;
    }
    return std::wstring(buffer.data());
}

std::optional<double> ParseLocalizedDouble(std::wstring value) {
    std::replace(value.begin(), value.end(), L',', L'.');
    wchar_t* end = nullptr;
    double result = std::wcstod(value.c_str(), &end);
    if (end == value.c_str() || !std::isfinite(result)) {
        return std::nullopt;
    }
    return result;
}

std::optional<double> NormalizeRegistryTemperature(
    const std::wstring& rawValue,
    const std::wstring& formattedValue) {
    auto value = ParseLocalizedDouble(rawValue);
    if (!value) {
        return std::nullopt;
    }

    return NormalizeTemperature(*value, formattedValue);
}

void ReadHwInfoGadgetRegistry(MetricsSnapshot& snapshot,
                              const ModSettings& settings) {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\HWiNFO64\\VSB", 0,
                      KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
        return;
    }

    int bestCpuScore = snapshot.cpuTemp ? 10000 : -1;
    int bestGpuScore = snapshot.gpuTemp ? 10000 : -1;
    int consecutiveMissing = 0;
    for (int i = 0; i < 1024; i++) {
        std::wstring suffix = std::to_wstring(i);
        auto sensor = ReadRegistryString(key, L"Sensor" + suffix);
        if (!sensor) {
            if (++consecutiveMissing >= 16) {
                break;
            }
            continue;
        }
        consecutiveMissing = 0;
        auto label = ReadRegistryString(key, L"Label" + suffix);
        auto rawValue = ReadRegistryString(key, L"ValueRaw" + suffix);
        auto formattedValue = ReadRegistryString(key, L"Value" + suffix);
        if (!label || !rawValue || !formattedValue) {
            continue;
        }
        auto value =
            NormalizeRegistryTemperature(*rawValue, *formattedValue);
        if (!value) {
            continue;
        }

        int cpuScore =
            CpuTemperatureScore(*sensor, *label, settings.cpuTempSensor);
        if (cpuScore > bestCpuScore) {
            bestCpuScore = cpuScore;
            snapshot.cpuTemp = *value;
            snapshot.cpuTempProvider =
                TemperatureProvider::HwInfoGadgetRegistry;
        }

        int gpuScore =
            GpuTemperatureScore(*sensor, *label, settings.gpuTempSensor);
        if (gpuScore > bestGpuScore) {
            bestGpuScore = gpuScore;
            snapshot.gpuTemp = *value;
            snapshot.gpuTempProvider =
                TemperatureProvider::HwInfoGadgetRegistry;
        }
    }

    RegCloseKey(key);
}

void ReadWindowsThermalZones(MetricsSnapshot& snapshot,
                             const ModSettings& settings);
void ReadWindowsGpuTemperature(MetricsSnapshot& snapshot,
                               const ModSettings& settings);

void ReadHwInfoTemperatures(MetricsSnapshot& snapshot,
                            const ModSettings& settings) {
    ReadHwInfoSharedMemory(snapshot, settings);
    if (!snapshot.cpuTemp || !snapshot.gpuTemp) {
        ReadHwInfoGadgetRegistry(snapshot, settings);
    }
}

void ReadTemperatures(MetricsSnapshot& snapshot,
                      const ModSettings& settings) {
    switch (settings.temperatureSource) {
        case TemperatureSource::SharedMemory:
            ReadHwInfoSharedMemory(snapshot, settings);
            break;

        case TemperatureSource::GadgetRegistry:
            ReadHwInfoGadgetRegistry(snapshot, settings);
            break;

        case TemperatureSource::WindowsNative:
            ReadWindowsGpuTemperature(snapshot, settings);
            ReadWindowsThermalZones(snapshot, settings);
            break;

        case TemperatureSource::Disabled:
            break;

        case TemperatureSource::HwInfoAuto:
            ReadHwInfoTemperatures(snapshot, settings);
            break;

        case TemperatureSource::Auto:
        default:
            ReadHwInfoTemperatures(snapshot, settings);
            if (!snapshot.gpuTemp) {
                ReadWindowsGpuTemperature(snapshot, settings);
            }
            if (!snapshot.cpuTemp) {
                ReadWindowsThermalZones(snapshot, settings);
            }
            break;
    }
}

uint64_t FileTimeValue(const FILETIME& value) {
    ULARGE_INTEGER result{};
    result.LowPart = value.dwLowDateTime;
    result.HighPart = value.dwHighDateTime;
    return result.QuadPart;
}

double ReadCpuUsage() {
    static bool initialized = false;
    static uint64_t previousIdle = 0;
    static uint64_t previousKernel = 0;
    static uint64_t previousUser = 0;

    FILETIME idleTime{};
    FILETIME kernelTime{};
    FILETIME userTime{};
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        return 0.0;
    }

    uint64_t idle = FileTimeValue(idleTime);
    uint64_t kernel = FileTimeValue(kernelTime);
    uint64_t user = FileTimeValue(userTime);
    if (!initialized) {
        initialized = true;
        previousIdle = idle;
        previousKernel = kernel;
        previousUser = user;
        return 0.0;
    }

    uint64_t idleDelta = idle - previousIdle;
    uint64_t kernelDelta = kernel - previousKernel;
    uint64_t userDelta = user - previousUser;
    previousIdle = idle;
    previousKernel = kernel;
    previousUser = user;

    uint64_t total = kernelDelta + userDelta;
    if (!total || idleDelta > total) {
        return 0.0;
    }
    return std::clamp(100.0 * static_cast<double>(total - idleDelta) /
                          static_cast<double>(total),
                      0.0, 100.0);
}

void ReadMemory(MetricsSnapshot& snapshot) {
    MEMORYSTATUSEX memory{};
    memory.dwLength = sizeof(memory);
    if (!GlobalMemoryStatusEx(&memory)) {
        return;
    }

    constexpr double gib = 1024.0 * 1024.0 * 1024.0;
    snapshot.ram = static_cast<double>(memory.dwMemoryLoad);
    snapshot.ramTotalGb = static_cast<double>(memory.ullTotalPhys) / gib;
    snapshot.ramUsedGb =
        static_cast<double>(memory.ullTotalPhys - memory.ullAvailPhys) / gib;
}

constexpr double kGiB = 1024.0 * 1024.0 * 1024.0;

// D3DKMT exposes display-adapter performance data, including temperature,
// without requiring a third-party monitoring application. The declarations are
// kept local so the mod can build in Windhawk environments without d3dkmthk.h.
using D3DKMT_HANDLE = UINT32;

struct D3DKMT_OPENADAPTERFROMLUID {
    LUID AdapterLuid;
    D3DKMT_HANDLE hAdapter;
};

struct D3DKMT_CLOSEADAPTER {
    D3DKMT_HANDLE hAdapter;
};

struct D3DKMT_QUERYADAPTERINFO {
    D3DKMT_HANDLE hAdapter;
    UINT Type;
    void* pPrivateDriverData;
    UINT PrivateDriverDataSize;
};

struct D3DKMT_ADAPTER_PERFDATA {
    UINT PhysicalAdapterIndex;
    ULONGLONG MemoryFrequency;
    ULONGLONG MaxMemoryFrequency;
    ULONGLONG MaxMemoryFrequencyOC;
    ULONGLONG MemoryBandwidth;
    ULONGLONG PCIEBandwidth;
    ULONG FanRPM;
    ULONG Power;
    ULONG Temperature;
    UCHAR PowerStateOverride;
};

struct D3DKMT_ADAPTERINFO {
    D3DKMT_HANDLE hAdapter;
    LUID AdapterLuid;
    ULONG NumOfSources;
    BOOL bPresentMoveRegionsPreferred;
};

struct D3DKMT_ENUMADAPTERS2 {
    ULONG NumAdapters;
    D3DKMT_ADAPTERINFO* pAdapters;
};

struct D3DKMT_ADAPTERREGISTRYINFO {
    WCHAR AdapterString[MAX_PATH];
    WCHAR BiosString[MAX_PATH];
    WCHAR DacType[MAX_PATH];
    WCHAR ChipType[MAX_PATH];
};

struct D3DKMT_SEGMENTSIZEINFO {
    ULONGLONG DedicatedVideoMemorySize;
    ULONGLONG DedicatedSystemMemorySize;
    ULONGLONG SharedSystemMemorySize;
};

constexpr UINT kAdapterRegistryInfoQueryType = 8;
constexpr UINT kAdapterSegmentSizeQueryType = 3;
constexpr UINT kAdapterPerfDataQueryType = 62;  // KMTQAITYPE_ADAPTERPERFDATA
constexpr ULONG kMaxD3dkmtAdapters = 16;

using D3DKMTEnumAdapters2_t = LONG(WINAPI*)(D3DKMT_ENUMADAPTERS2*);
using D3DKMTOpenAdapterFromLuid_t =
    LONG(WINAPI*)(D3DKMT_OPENADAPTERFROMLUID*);
using D3DKMTQueryAdapterInfo_t =
    LONG(WINAPI*)(D3DKMT_QUERYADAPTERINFO*);
using D3DKMTCloseAdapter_t =
    LONG(WINAPI*)(const D3DKMT_CLOSEADAPTER*);

D3DKMTEnumAdapters2_t g_d3dkmtEnumAdapters2 = nullptr;
D3DKMTOpenAdapterFromLuid_t g_d3dkmtOpenAdapterFromLuid = nullptr;
D3DKMTQueryAdapterInfo_t g_d3dkmtQueryAdapterInfo = nullptr;
D3DKMTCloseAdapter_t g_d3dkmtCloseAdapter = nullptr;

struct GpuAdapterInfo {
    std::wstring description;
    std::wstring luid;
    LUID luidValue{};
    uint64_t dedicatedVideoMemory = 0;
    uint64_t sharedSystemMemory = 0;
};

std::optional<std::wstring> g_cachedGpuAdapterFilter;
std::optional<GpuAdapterInfo> g_cachedGpuAdapterInfo;
bool g_cachedGpuAdapterResolved = false;

void InvalidateGpuAdapterCache() {
    g_cachedGpuAdapterFilter.reset();
    g_cachedGpuAdapterInfo.reset();
    g_cachedGpuAdapterResolved = false;
    g_nextGpuIdentityCheck = {};
}

std::wstring FormatAdapterLuid(const LUID& luid) {
    wchar_t buffer[32];
    swprintf(buffer, std::size(buffer), L"0x%08X_0x%08X",
             static_cast<DWORD>(luid.HighPart), luid.LowPart);
    return ToLower(buffer);
}

std::wstring FixedWideToString(const wchar_t* value, size_t capacity) {
    size_t length = 0;
    while (length < capacity && value[length]) {
        length++;
    }
    return std::wstring(value, length);
}

std::optional<GpuAdapterInfo> GetLiveD3dkmtAdapterInfo(
    const std::wstring& filterLower) {
    if (!g_d3dkmtEnumAdapters2 || !g_d3dkmtQueryAdapterInfo ||
        !g_d3dkmtCloseAdapter) {
        return std::nullopt;
    }

    D3DKMT_ADAPTERINFO adapters[kMaxD3dkmtAdapters]{};
    D3DKMT_ENUMADAPTERS2 enumeration{};
    enumeration.NumAdapters = std::size(adapters);
    enumeration.pAdapters = adapters;
    if (g_d3dkmtEnumAdapters2(&enumeration) != 0) {
        return std::nullopt;
    }

    std::optional<GpuAdapterInfo> selected;
    ULONG adapterCount = std::min<ULONG>(enumeration.NumAdapters,
                                         std::size(adapters));
    for (ULONG index = 0; index < adapterCount; index++) {
        const auto& adapter = adapters[index];

        D3DKMT_ADAPTERREGISTRYINFO registryInfo{};
        D3DKMT_QUERYADAPTERINFO registryQuery{};
        registryQuery.hAdapter = adapter.hAdapter;
        registryQuery.Type = kAdapterRegistryInfoQueryType;
        registryQuery.pPrivateDriverData = &registryInfo;
        registryQuery.PrivateDriverDataSize = sizeof(registryInfo);
        bool registryAvailable =
            g_d3dkmtQueryAdapterInfo(&registryQuery) == 0;

        D3DKMT_SEGMENTSIZEINFO segmentInfo{};
        D3DKMT_QUERYADAPTERINFO segmentQuery{};
        segmentQuery.hAdapter = adapter.hAdapter;
        segmentQuery.Type = kAdapterSegmentSizeQueryType;
        segmentQuery.pPrivateDriverData = &segmentInfo;
        segmentQuery.PrivateDriverDataSize = sizeof(segmentInfo);
        bool segmentsAvailable =
            g_d3dkmtQueryAdapterInfo(&segmentQuery) == 0;

        std::wstring description =
            registryAvailable
                ? FixedWideToString(registryInfo.AdapterString,
                                    std::size(registryInfo.AdapterString))
                : L"";
        GpuAdapterInfo candidate{
            description,
            FormatAdapterLuid(adapter.AdapterLuid),
            adapter.AdapterLuid,
            segmentsAvailable ? segmentInfo.DedicatedVideoMemorySize : 0,
            segmentsAvailable ? segmentInfo.SharedSystemMemorySize : 0,
        };

        bool matchesFilter =
            filterLower.empty() ||
            Contains(ToLower(candidate.description), filterLower);
        bool betterCandidate =
            !selected || candidate.dedicatedVideoMemory >
                             selected->dedicatedVideoMemory ||
            (candidate.dedicatedVideoMemory ==
                 selected->dedicatedVideoMemory &&
             !candidate.description.empty() &&
             selected->description.empty()) ||
            (candidate.dedicatedVideoMemory ==
                 selected->dedicatedVideoMemory &&
             candidate.description.empty() == selected->description.empty() &&
             candidate.sharedSystemMemory > selected->sharedSystemMemory);
        if (matchesFilter && betterCandidate) {
            selected = std::move(candidate);
        }
    }

    for (ULONG index = 0; index < adapterCount; index++) {
        if (adapters[index].hAdapter) {
            D3DKMT_CLOSEADAPTER closeAdapter{adapters[index].hAdapter};
            g_d3dkmtCloseAdapter(&closeAdapter);
        }
    }

    // Stale D3DKMT duplicates can retain the full memory sizes while losing
    // their registry identity. Prefer the DXGI compatibility path instead of
    // caching such an ambiguous adapter.
    return selected && !selected->description.empty() ? selected
                                                       : std::nullopt;
}

std::optional<GpuAdapterInfo> GetDxgiAdapterInfo(
    const std::wstring& filterLower) {
    com_ptr<IDXGIFactory> factory;
    if (FAILED(CreateDXGIFactory(IID_PPV_ARGS(factory.put())))) {
        return std::nullopt;
    }

    DXGI_ADAPTER_DESC selected{};
    bool found = false;
    for (UINT index = 0;; index++) {
        com_ptr<IDXGIAdapter> adapter;
        HRESULT result = factory->EnumAdapters(index, adapter.put());
        if (result == DXGI_ERROR_NOT_FOUND) {
            break;
        }
        if (FAILED(result)) {
            continue;
        }

        DXGI_ADAPTER_DESC description{};
        if (FAILED(adapter->GetDesc(&description))) {
            continue;
        }

        if (!filterLower.empty()) {
            if (Contains(ToLower(description.Description), filterLower)) {
                selected = description;
                found = true;
                break;
            }
        } else if (!found || description.DedicatedVideoMemory >
                                      selected.DedicatedVideoMemory) {
            selected = description;
            found = true;
        }
    }

    if (!found) {
        return std::nullopt;
    }

    return GpuAdapterInfo{
        selected.Description, FormatAdapterLuid(selected.AdapterLuid),
        selected.AdapterLuid,
        selected.DedicatedVideoMemory, selected.SharedSystemMemory};
}

std::optional<GpuAdapterInfo> ResolveCurrentGpuAdapterInfo(
    const std::wstring& filterLower,
    PCWSTR* provider = nullptr) {
    auto adapter = GetLiveD3dkmtAdapterInfo(filterLower);
    PCWSTR resolvedProvider = L"D3DKMT";
    if (!adapter) {
        adapter = GetDxgiAdapterInfo(filterLower);
        resolvedProvider = L"DXGI fallback";
    }
    if (provider) {
        *provider = resolvedProvider;
    }
    return adapter;
}

// The collector resolves the adapter on its own thread; the command center
// reads the name on the UI thread, so publish a guarded copy.
std::mutex g_adapterDisplayMutex;
std::wstring g_adapterDisplayName;

void PublishGpuAdapterName(const std::wstring& name) {
    std::lock_guard<std::mutex> lock(g_adapterDisplayMutex);
    g_adapterDisplayName = name;
}

std::wstring ActiveGpuAdapterName() {
    std::lock_guard<std::mutex> lock(g_adapterDisplayMutex);
    return g_adapterDisplayName.empty() ? std::wstring(L"Not detected")
                                        : g_adapterDisplayName;
}

std::optional<GpuAdapterInfo> GetGpuAdapterInfo(
    const std::wstring& adapterFilter) {
    std::wstring filterLower = ToLower(adapterFilter);
    if (g_cachedGpuAdapterResolved &&
        g_cachedGpuAdapterFilter == filterLower) {
        return g_cachedGpuAdapterInfo;
    }

    g_cachedGpuAdapterFilter = filterLower;
    PCWSTR provider = nullptr;
    g_cachedGpuAdapterInfo =
        ResolveCurrentGpuAdapterInfo(filterLower, &provider);
    g_cachedGpuAdapterResolved = true;

    if (!g_cachedGpuAdapterInfo) {
        PublishGpuAdapterName(std::wstring());
        if (filterLower.empty()) {
            Wh_Log(L"No GPU adapter found");
        } else {
            Wh_Log(L"No GPU adapter matched: %s", filterLower.c_str());
        }
        return std::nullopt;
    }

    PublishGpuAdapterName(g_cachedGpuAdapterInfo->description);
    Wh_Log(L"Selected GPU (%s): %s, LUID %s, dedicated %.1f GiB, shared %.1f "
           L"GiB",
           provider, g_cachedGpuAdapterInfo->description.c_str(),
           g_cachedGpuAdapterInfo->luid.c_str(),
           static_cast<double>(g_cachedGpuAdapterInfo->dedicatedVideoMemory) /
               kGiB,
           static_cast<double>(g_cachedGpuAdapterInfo->sharedSystemMemory) /
               kGiB);
    return g_cachedGpuAdapterInfo;
}

bool HasGpuAdapterIdentityChanged(const GpuAdapterInfo& cachedAdapter,
                                  const std::wstring& adapterFilter) {
    auto now = std::chrono::steady_clock::now();
    if (now < g_nextGpuIdentityCheck) {
        return false;
    }
    g_nextGpuIdentityCheck = now + std::chrono::seconds(5);

    auto currentAdapter =
        ResolveCurrentGpuAdapterInfo(ToLower(adapterFilter));
    return currentAdapter && currentAdapter->luid != cachedAdapter.luid;
}

void ReadWindowsGpuTemperature(MetricsSnapshot& snapshot,
                               const ModSettings& settings) {
    if (snapshot.gpuTemp || !g_d3dkmtOpenAdapterFromLuid ||
        !g_d3dkmtQueryAdapterInfo || !g_d3dkmtCloseAdapter) {
        return;
    }

    auto adapter = GetGpuAdapterInfo(settings.gpuAdapter);
    if (!adapter) {
        return;
    }

    D3DKMT_OPENADAPTERFROMLUID openAdapter{};
    openAdapter.AdapterLuid = adapter->luidValue;
    if (g_d3dkmtOpenAdapterFromLuid(&openAdapter) != 0) {
        return;
    }

    D3DKMT_ADAPTER_PERFDATA perfData{};
    D3DKMT_QUERYADAPTERINFO queryInfo{};
    queryInfo.hAdapter = openAdapter.hAdapter;
    queryInfo.Type = kAdapterPerfDataQueryType;
    queryInfo.pPrivateDriverData = &perfData;
    queryInfo.PrivateDriverDataSize = sizeof(perfData);

    LONG status = g_d3dkmtQueryAdapterInfo(&queryInfo);

    D3DKMT_CLOSEADAPTER closeAdapter{};
    closeAdapter.hAdapter = openAdapter.hAdapter;
    g_d3dkmtCloseAdapter(&closeAdapter);

    // The driver reports tenths of a degree Celsius. Zero means unavailable;
    // reject values above 200 C as invalid driver data.
    if (status != 0 || perfData.Temperature == 0 ||
        perfData.Temperature > 2000) {
        return;
    }

    snapshot.gpuTemp = perfData.Temperature / 10.0;
    snapshot.gpuTempProvider = TemperatureProvider::WindowsD3dkmt;
}

bool MatchesGpuAdapter(const std::wstring& instance,
                       const std::optional<GpuAdapterInfo>& adapter) {
    return !adapter || Contains(ToLower(instance), adapter->luid);
}

void CloseMetricSources();

constexpr auto kPdhCounterRetryInterval = std::chrono::seconds(30);
constexpr auto kPdhRecoveryRetryDelay = std::chrono::seconds(1);
constexpr auto kPdhRecoveryCooldown = std::chrono::seconds(30);
constexpr uint32_t kPdhReadFailureThreshold = 3;

void ClosePdhQuery() {
    if (g_pdhQuery) {
        PdhCloseQuery(g_pdhQuery);
        g_pdhQuery = nullptr;
    }
    if (g_pdhCompletionEvent) {
        CloseHandle(g_pdhCompletionEvent);
        g_pdhCompletionEvent = nullptr;
    }
    g_gpuCounter = nullptr;
    g_vramCounter = nullptr;
    g_sharedVramCounter = nullptr;
    g_thermalZoneCounter = nullptr;
}

void RecreatePdhSources(PCWSTR reason,
                        PDH_STATUS status,
                        std::chrono::steady_clock::time_point now) {
    if (status == ERROR_SUCCESS) {
        Wh_Log(L"Recreating GPU performance counters after %s", reason);
    } else {
        Wh_Log(L"Recreating GPU performance counters after %s: %08X",
               reason, status);
    }

    ClosePdhQuery();
    InvalidateGpuAdapterCache();
    g_consecutivePdhReadFailures = 0;
    g_nextPdhCounterRetry = now + kPdhRecoveryRetryDelay;
    g_nextPdhRecovery = now + kPdhRecoveryCooldown;
}

void RecordPdhReadFailure(PCWSTR reason,
                          PDH_STATUS status = ERROR_SUCCESS) {
    g_consecutivePdhReadFailures =
        std::min(g_consecutivePdhReadFailures + 1,
                 kPdhReadFailureThreshold);

    auto now = std::chrono::steady_clock::now();
    if (g_consecutivePdhReadFailures < kPdhReadFailureThreshold ||
        now < g_nextPdhRecovery) {
        return;
    }

    RecreatePdhSources(reason, status, now);
}

void RecoverFromGpuAdapterIdentityChange() {
    auto now = std::chrono::steady_clock::now();
    if (now < g_nextPdhRecovery) {
        return;
    }
    RecreatePdhSources(L"confirmed adapter LUID change", ERROR_SUCCESS, now);
}

void RecordPdhReadSuccess() {
    g_consecutivePdhReadFailures = 0;
}

bool AddPdhCounter(PDH_HCOUNTER& counter,
                   PCWSTR path,
                   PCWSTR description) {
    if (counter) {
        return false;
    }

    PDH_HCOUNTER newCounter = nullptr;
    PDH_STATUS status =
        PdhAddEnglishCounterW(g_pdhQuery, path, 0, &newCounter);
    if (status != ERROR_SUCCESS) {
        Wh_Log(L"Adding the %s counter failed: %08X", description, status);
        return false;
    }

    counter = newCounter;
    return true;
}

bool NeedsWindowsThermalZones(const ModSettings& settings) {
    return settings.temperatureSource == TemperatureSource::Auto ||
           settings.temperatureSource == TemperatureSource::WindowsNative;
}

enum class PdhQueryDemand { None, ThermalOnly, FullMetrics };
PdhQueryDemand g_pdhQueryDemand = PdhQueryDemand::FullMetrics;

void EnsurePdhQuery(
    const ModSettings& settings,
    PdhQueryDemand demand = PdhQueryDemand::FullMetrics) {
    auto now = std::chrono::steady_clock::now();
    bool thermalZonesRequired = NeedsWindowsThermalZones(settings);
    if (demand == PdhQueryDemand::ThermalOnly && !thermalZonesRequired) {
        demand = PdhQueryDemand::None;
    }
    if (demand != g_pdhQueryDemand) {
        // Provider demand changes invalidate both the counter set and its
        // retry deadline. A failed thermal probe must not delay GPU fallback.
        ClosePdhQuery();
        g_nextPdhCounterRetry = {};
        g_pdhQueryDemand = demand;
    }
    if (demand == PdhQueryDemand::None) return;
    const bool gpuCountersRequired = demand == PdhQueryDemand::FullMetrics;
    if (!thermalZonesRequired && g_thermalZoneCounter) {
        PdhRemoveCounter(g_thermalZoneCounter);
        g_thermalZoneCounter = nullptr;
    }

    bool queryCreated = false;
    if (!g_pdhQuery) {
        if (now < g_nextPdhCounterRetry) {
            return;
        }
        if (PdhOpenQueryW(nullptr, 0, &g_pdhQuery) != ERROR_SUCCESS) {
            g_pdhQuery = nullptr;
            g_nextPdhCounterRetry = now + kPdhCounterRetryInterval;
            return;
        }
        queryCreated = true;
    } else if ((!gpuCountersRequired ||
                (g_gpuCounter && g_vramCounter && g_sharedVramCounter)) &&
               (!thermalZonesRequired || g_thermalZoneCounter)) {
        return;
    }

    if (!queryCreated && now < g_nextPdhCounterRetry) {
        return;
    }

    bool counterAdded = false;
    if (gpuCountersRequired) {
        counterAdded |= AddPdhCounter(
            g_gpuCounter, L"\\GPU Engine(*)\\Utilization Percentage", L"GPU usage");
        counterAdded |= AddPdhCounter(
            g_vramCounter, L"\\GPU Adapter Memory(*)\\Dedicated Usage",
            L"VRAM usage");
        counterAdded |= AddPdhCounter(
            g_sharedVramCounter, L"\\GPU Adapter Memory(*)\\Shared Usage",
            L"shared GPU-memory usage");
    }
    if (thermalZonesRequired) {
        counterAdded |= AddPdhCounter(
            g_thermalZoneCounter,
            L"\\Thermal Zone Information(*)\\Temperature",
            L"Windows thermal-zone");
    }

    if (!g_gpuCounter && !g_vramCounter && !g_sharedVramCounter &&
        (!thermalZonesRequired || !g_thermalZoneCounter)) {
        PdhCloseQuery(g_pdhQuery);
        g_pdhQuery = nullptr;
        g_nextPdhCounterRetry = now + kPdhCounterRetryInterval;
        return;
    }

    if ((gpuCountersRequired &&
         (!g_gpuCounter || !g_vramCounter || !g_sharedVramCounter)) ||
        (thermalZonesRequired && !g_thermalZoneCounter)) {
        g_nextPdhCounterRetry = now + kPdhCounterRetryInterval;
    }

    if (queryCreated || counterAdded) {
        PDH_STATUS collectStatus = PdhCollectQueryData(g_pdhQuery);
        if (collectStatus != ERROR_SUCCESS) {
            Wh_Log(L"Initial metric counter collection failed: %08X",
                   collectStatus);
        }
    }
}

PDH_STATUS ReadPdhArray(PDH_HCOUNTER counter,
                        std::vector<uint8_t>& buffer,
                        DWORD& itemCount) {
    if (!counter) {
        return PDH_CSTATUS_NO_COUNTER;
    }
    DWORD bufferSize = 0;
    PDH_STATUS status = PdhGetFormattedCounterArrayW(
        counter, PDH_FMT_DOUBLE, &bufferSize, &itemCount, nullptr);
    if (status == ERROR_SUCCESS && !bufferSize) {
        buffer.clear();
        itemCount = 0;
        return ERROR_SUCCESS;
    }
    if (status != static_cast<PDH_STATUS>(PDH_MORE_DATA) || !bufferSize) {
        return status;
    }

    buffer.resize(bufferSize);
    auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
    return PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &bufferSize,
                                        &itemCount, items);
}

bool IsHardPdhArrayFailure(PDH_STATUS status) {
    return status != ERROR_SUCCESS &&
           status != static_cast<PDH_STATUS>(PDH_NO_DATA) &&
           status != static_cast<PDH_STATUS>(PDH_CSTATUS_NO_INSTANCE);
}

void ReadWindowsThermalZones(MetricsSnapshot& snapshot,
                             const ModSettings& settings) {
    if (snapshot.cpuTemp || !g_thermalZoneCounter) {
        return;
    }

    std::vector<uint8_t> buffer;
    DWORD itemCount = 0;
    if (ReadPdhArray(g_thermalZoneCounter, buffer, itemCount) !=
        ERROR_SUCCESS) {
        return;
    }

    std::wstring filter = ToLower(settings.windowsThermalZoneFilter);
    auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
    double aggregate = 0.0;
    size_t validCount = 0;

    for (DWORD i = 0; i < itemCount; i++) {
        const auto& item = items[i];
        const auto& value = item.FmtValue;
        if (value.CStatus != PDH_CSTATUS_VALID_DATA &&
            value.CStatus != PDH_CSTATUS_NEW_DATA) {
            continue;
        }

        std::wstring instance = item.szName ? ToLower(item.szName) : L"";
        if (!filter.empty() && !Contains(instance, filter)) {
            continue;
        }

        // This counter is reported in Kelvin. Match Taskbar Clock
        // Customization by rejecting dead zones below 200 K, and reject values
        // above the module's supported 200 °C ceiling as corrupt data.
        double kelvin = value.doubleValue;
        if (!std::isfinite(kelvin) || kelvin < 200.0 || kelvin > 473.15) {
            continue;
        }

        double celsius = kelvin - 273.15;
        if (settings.windowsThermalZoneAggregation ==
            ThermalZoneAggregation::Hottest) {
            aggregate = validCount ? std::max(aggregate, celsius) : celsius;
        } else {
            aggregate += celsius;
        }
        validCount++;
    }

    if (!validCount) {
        return;
    }

    if (settings.windowsThermalZoneAggregation ==
        ThermalZoneAggregation::Average) {
        aggregate /= validCount;
    }
    snapshot.cpuTemp = aggregate;
    snapshot.cpuTempProvider = TemperatureProvider::WindowsThermalZones;
}

std::optional<double> ReadGpuUsage(
    const std::optional<GpuAdapterInfo>& adapter,
    PDH_STATUS& readStatus) {
    std::vector<uint8_t> buffer;
    DWORD itemCount = 0;
    readStatus = ReadPdhArray(g_gpuCounter, buffer, itemCount);
    if (readStatus == static_cast<PDH_STATUS>(PDH_NO_DATA) ||
        readStatus == static_cast<PDH_STATUS>(PDH_CSTATUS_NO_INSTANCE)) {
        return 0.0;
    }
    if (readStatus != ERROR_SUCCESS) {
        return std::nullopt;
    }

    auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
    std::unordered_map<std::wstring, double> engineTotals;
    bool found = false;
    for (DWORD i = 0; i < itemCount; i++) {
        const auto& value = items[i].FmtValue;
        if ((value.CStatus != PDH_CSTATUS_VALID_DATA &&
             value.CStatus != PDH_CSTATUS_NEW_DATA) ||
            !std::isfinite(value.doubleValue) || value.doubleValue < 0.0) {
            continue;
        }
        std::wstring instance = items[i].szName ? items[i].szName : L"";
        if (!MatchesGpuAdapter(instance, adapter)) {
            continue;
        }
        size_t luidPosition = instance.find(L"luid_");
        std::wstring engineKey =
            luidPosition == std::wstring::npos ? instance
                                                : instance.substr(luidPosition);
        engineTotals[engineKey] += value.doubleValue;
        found = true;
    }

    double busiestEngine = 0.0;
    for (const auto& [engine, usage] : engineTotals) {
        busiestEngine = std::max(busiestEngine, usage);
    }
    return found ? std::clamp(busiestEngine, 0.0, 100.0) : 0.0;
}

std::optional<double> ReadVramUsedBytes(
    PDH_HCOUNTER counter,
    const std::optional<GpuAdapterInfo>& adapter,
    bool& anyValidSample,
    PDH_STATUS& readStatus) {
    std::vector<uint8_t> buffer;
    DWORD itemCount = 0;
    readStatus = ReadPdhArray(counter, buffer, itemCount);
    if (readStatus != ERROR_SUCCESS) {
        return std::nullopt;
    }

    auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
    double total = 0.0;
    bool found = false;
    for (DWORD i = 0; i < itemCount; i++) {
        const auto& value = items[i].FmtValue;
        if ((value.CStatus != PDH_CSTATUS_VALID_DATA &&
             value.CStatus != PDH_CSTATUS_NEW_DATA) ||
            !std::isfinite(value.doubleValue) || value.doubleValue < 0.0) {
            continue;
        }
        anyValidSample = true;

        std::wstring instance = items[i].szName ? items[i].szName : L"";
        if (!MatchesGpuAdapter(instance, adapter)) {
            continue;
        }
        total += value.doubleValue;
        found = true;
    }
    return found ? std::optional<double>(total) : std::nullopt;
}

void ReadPdhMetrics(MetricsSnapshot& snapshot, const ModSettings& settings,
                    bool completionReady = false) {
    EnsurePdhQuery(settings);
    if (!g_pdhQuery) {
        return;
    }

    if (!completionReady) {
        PDH_STATUS collectStatus = PdhCollectQueryData(g_pdhQuery);
        if (collectStatus != ERROR_SUCCESS) {
            RecordPdhReadFailure(L"collection", collectStatus);
            return;
        }
    }

    auto adapter = GetGpuAdapterInfo(settings.gpuAdapter);
    PDH_STATUS gpuReadStatus = ERROR_SUCCESS;
    auto gpuUsage = ReadGpuUsage(adapter, gpuReadStatus);
    if (gpuUsage) {
        snapshot.gpu = *gpuUsage;
        snapshot.gpuAvailable = true;
    }
    uint64_t vramTotalBytes = 0;
    PDH_HCOUNTER vramCounter = nullptr;
    if (adapter) {
        if (adapter->dedicatedVideoMemory > 0) {
            vramTotalBytes = adapter->dedicatedVideoMemory;
            vramCounter = g_vramCounter;
        } else if (adapter->sharedSystemMemory > 0) {
            vramTotalBytes = adapter->sharedSystemMemory;
            vramCounter = g_sharedVramCounter;
        }
    }
    bool vramCounterHasData = false;
    PDH_STATUS vramReadStatus = ERROR_SUCCESS;
    auto vramUsedBytes =
        ReadVramUsedBytes(vramCounter, adapter, vramCounterHasData,
                          vramReadStatus);
    bool vramAvailable = vramTotalBytes > 0 && vramUsedBytes.has_value();
    if (vramAvailable) {
        snapshot.vramUsedGb = *vramUsedBytes / kGiB;
        snapshot.vramTotalGb = static_cast<double>(vramTotalBytes) / kGiB;
        snapshot.vram = std::clamp(
            snapshot.vramUsedGb / snapshot.vramTotalGb * 100.0, 0.0, 100.0);
        snapshot.vramAvailable = true;
    }

    bool hardReadFailure =
        (g_gpuCounter && IsHardPdhArrayFailure(gpuReadStatus)) ||
        (vramCounter && IsHardPdhArrayFailure(vramReadStatus));
    bool adapterMismatch = adapter && vramReadStatus == ERROR_SUCCESS &&
                           vramCounterHasData && !vramAvailable;
    bool adapterIdentityChanged =
        adapterMismatch &&
        HasGpuAdapterIdentityChanged(*adapter, settings.gpuAdapter);
    if (hardReadFailure) {
        RecordPdhReadFailure(L"counter read");
    } else if (adapterIdentityChanged) {
        RecoverFromGpuAdapterIdentityChange();
    } else {
        RecordPdhReadSuccess();
    }
}

MetricsSnapshot CollectMetrics(const ModSettings& settings, FocusScene scene,
                               bool pdhCompletionReady = false) {
    MetricsSnapshot snapshot;
    snapshot.scene = scene;
    snapshot.cpu = ReadCpuUsage();
    ReadMemory(snapshot);
    ReadPdhMetrics(snapshot, settings, pdhCompletionReady);
    ReadTemperatures(snapshot, settings);
    return snapshot;
}

PCWSTR TemperatureProviderName(TemperatureProvider provider) {
    switch (provider) {
        case TemperatureProvider::HwInfoSharedMemory:
            return L"HWiNFO Shared Memory";
        case TemperatureProvider::HwInfoGadgetRegistry:
            return L"HWiNFO Gadget Registry";
        case TemperatureProvider::WindowsD3dkmt:
            return L"Windows D3DKMT";
        case TemperatureProvider::WindowsThermalZones:
            return L"Windows thermal zones";
        case TemperatureProvider::None:
        default:
            return L"unavailable";
    }
}

bool SameMetrics(const MetricsSnapshot& first, const MetricsSnapshot& second) {
    return first.cpu == second.cpu && first.ram == second.ram &&
           first.ramUsedGb == second.ramUsedGb &&
           first.ramTotalGb == second.ramTotalGb &&
           first.gpu == second.gpu &&
           first.gpuAvailable == second.gpuAvailable &&
           first.vram == second.vram &&
           first.vramUsedGb == second.vramUsedGb &&
           first.vramTotalGb == second.vramTotalGb &&
           first.vramAvailable == second.vramAvailable &&
           first.cpuTemp == second.cpuTemp &&
           first.gpuTemp == second.gpuTemp &&
           first.cpuTempProvider == second.cpuTempProvider &&
           first.gpuTempProvider == second.gpuTempProvider &&
           first.scene == second.scene;
}

void PublishMetrics(MetricsSnapshot snapshot) {
    std::lock_guard lock(g_metricsMutex);
    // The companion publishes at its own power-aware cadence. The reader can
    // wake more frequently to detect scene changes, but an unchanged shared
    // snapshot must not become a new UI render sequence.
    if (g_latestMetricsAvailable && SameMetrics(g_latestMetrics, snapshot)) {
        return;
    }
    g_latestMetrics = std::move(snapshot);
    g_latestMetricsSequence++;
    g_latestMetricsAvailable = true;
}

bool IsFullscreenApplicationActive() {
    HWND window = GetForegroundWindow();
    if (!window || IsIconic(window)) {
        return false;
    }

    DWORD processId = 0;
    if (!GetWindowThreadProcessId(window, &processId) ||
        processId == GetCurrentProcessId()) {
        return false;
    }

    RECT windowRect{};
    MONITORINFO monitorInfo{sizeof(monitorInfo)};
    HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONULL);
    if (!monitor || !GetWindowRect(window, &windowRect) ||
        !GetMonitorInfoW(monitor, &monitorInfo)) {
        return false;
    }

    constexpr LONG tolerance = 2;
    return windowRect.left <= monitorInfo.rcMonitor.left + tolerance &&
           windowRect.top <= monitorInfo.rcMonitor.top + tolerance &&
           windowRect.right >= monitorInfo.rcMonitor.right - tolerance &&
           windowRect.bottom >= monitorInfo.rcMonitor.bottom - tolerance;
}

std::wstring ForegroundProcessName() {
    HWND window = GetForegroundWindow();
    DWORD processId = 0;
    if (!window || !GetWindowThreadProcessId(window, &processId) ||
        processId == GetCurrentProcessId()) {
        return {};
    }

    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
                                 processId);
    if (!process) {
        return {};
    }
    std::array<wchar_t, 1024> path{};
    DWORD length = static_cast<DWORD>(path.size());
    std::wstring result;
    if (QueryFullProcessImageNameW(process, 0, path.data(), &length) && length) {
        std::wstring fullPath(path.data(), length);
        size_t slash = fullPath.find_last_of(L"\\/");
        result = ToLower(slash == std::wstring::npos
                             ? fullPath
                             : fullPath.substr(slash + 1));
    }
    CloseHandle(process);
    return result;
}

FocusScene DetectFocusScene() {
    SYSTEM_POWER_STATUS power{};
    if (GetSystemPowerStatus(&power) && power.SystemStatusFlag == 1) {
        return FocusScene::BatterySaver;
    }

    std::wstring process = ForegroundProcessName();
    if (Contains(process, L"powerpnt") ||
        Contains(process, L"presentationfontcache")) {
        return FocusScene::Presentation;
    }

    if (!IsFullscreenApplicationActive()) {
        return FocusScene::Normal;
    }
    if (Contains(process, L"cod") || Contains(process, L"modernwarfare") ||
        Contains(process, L"game") || Contains(process, L"steam") ||
        Contains(process, L"battle.net") || Contains(process, L"epicgames")) {
        return FocusScene::Gaming;
    }
    return FocusScene::Focus;
}

const wchar_t* PerformanceSuspensionReason(FocusScene scene) {
    WTS_CONNECTSTATE_CLASS* connection = nullptr;
    DWORD bytes = 0;
    if (WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE,
            WTS_CURRENT_SESSION, WTSConnectState,
            reinterpret_cast<LPWSTR*>(&connection), &bytes)) {
        const bool active = connection && *connection == WTSActive;
        WTSFreeMemory(connection);
        if (!active) return L"Remote session disconnected";
    }

    HWND taskbar = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (!taskbar || !IsWindowVisible(taskbar)) return L"Taskbar is not visible";
    APPBARDATA appbar{sizeof(appbar)};
    appbar.hWnd = taskbar;
    if (SHAppBarMessage(ABM_GETSTATE, &appbar) & ABS_AUTOHIDE)
        return L"Taskbar auto-hide is active";
    if (scene == FocusScene::Gaming || scene == FocusScene::Focus)
        return L"Fullscreen application";
    if (scene == FocusScene::Presentation)
        return L"Presentation mode";
    return nullptr;
}

bool ArmPdhCompletion(const ModSettings& settings, int intervalSeconds) {
    EnsurePdhQuery(settings);
    if (!g_pdhQuery) return false;
    if (!g_pdhCompletionEvent)
        g_pdhCompletionEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!g_pdhCompletionEvent) return false;
    ResetEvent(g_pdhCompletionEvent);
    PDH_STATUS status = PdhCollectQueryDataEx(
        g_pdhQuery, std::max(1, intervalSeconds), g_pdhCompletionEvent);
    if (status != ERROR_SUCCESS) {
        Wh_Log(L"Arming PDH completion event failed: %08X", status);
        return false;
    }
    return true;
}

PCWSTR FocusSceneName(FocusScene scene) {
    switch (scene) {
        case FocusScene::Gaming:
            return L"Gaming";
        case FocusScene::Focus:
            return L"Fullscreen focus";
        case FocusScene::Presentation:
            return L"Presentation";
        case FocusScene::BatterySaver:
            return L"Battery saver";
        case FocusScene::Normal:
        default:
            return L"Normal";
    }
}

PCWSTR ExperienceModeName(ExperienceMode mode) {
    switch (mode) {
        case ExperienceMode::Balanced:
            return L"Balanced";
        case ExperienceMode::Focus:
            return L"Focus";
        case ExperienceMode::Gaming:
            return L"Gaming";
        case ExperienceMode::Minimal:
            return L"Minimal";
        case ExperienceMode::Auto:
        default:
            return L"Auto";
    }
}

FocusScene ApplyExperienceModeToScene(FocusScene detected) {
    switch (g_experienceMode.load()) {
        case ExperienceMode::Focus:
            return FocusScene::Focus;
        case ExperienceMode::Gaming:
            return FocusScene::Gaming;
        case ExperienceMode::Minimal:
            return FocusScene::Presentation;
        case ExperienceMode::Balanced:
        case ExperienceMode::Auto:
        default:
            return detected;
    }
}

int LeanUpdateInterval(int configured, int leanFloor) {
    return g_leanMode ? std::max(configured, leanFloor) : configured;
}

int EffectiveUpdateInterval(const ModSettings& settings, FocusScene scene) {
    switch (scene) {
        case FocusScene::BatterySaver:
            return LeanUpdateInterval(settings.batterySaverUpdateInterval, 30);
        case FocusScene::Gaming:
        case FocusScene::Focus:
        case FocusScene::Presentation:
            return LeanUpdateInterval(settings.gamingUpdateInterval, 15);
        case FocusScene::Normal:
        default:
            return LeanUpdateInterval(settings.updateInterval, 5);
    }
}

bool GetLatestMetrics(MetricsSnapshot& snapshot, uint64_t& sequence) {
    std::lock_guard lock(g_metricsMutex);
    if (!g_latestMetricsAvailable) {
        return false;
    }
    snapshot = g_latestMetrics;
    sequence = g_latestMetricsSequence;
    return true;
}

void MetricsWorkerProc() {
    ReadCpuUsage();

    // Runtime publication cache: keep status changes immediate, but avoid
    // rewriting five INI fields and two registry values on every sample.
    // A periodic refresh repairs a missed write or externally removed state.
    std::optional<bool> publishedSuspended;
    bool publishedQuarantined = false;
    std::wstring publishedReason;
    ULONGLONG lastRuntimePublish = 0;
    const auto publishWorkerRuntime = [&](bool suspended, const wchar_t* reason) {
        const ULONGLONG now = GetTickCount64();
        if (publishedSuspended && *publishedSuspended == suspended &&
            publishedQuarantined == g_quarantine.quarantined &&
            publishedReason == reason && now - lastRuntimePublish < 30000) return;
        OpalControl::PublishRuntimeState(
            OpalControl::kPerformanceRuntimeActiveValue,
            OpalControl::kPerformanceRuntimePidValue, true, suspended, reason,
            g_quarantine.quarantined);
        publishedSuspended = suspended;
        publishedQuarantined = g_quarantine.quarantined;
        publishedReason = reason;
        lastRuntimePublish = now;
    };
    // End runtime publication cache.

    bool firstSample = true;
    bool lastSampleWasExternal = false;
    bool telemetrySourceLogged = false;
    bool providersLogged = false;
    TemperatureProvider lastCpuProvider = TemperatureProvider::None;
    TemperatureProvider lastGpuProvider = TemperatureProvider::None;
    std::optional<double> cachedCpuTemp;
    std::optional<double> cachedGpuTemp;
    TemperatureProvider cachedCpuProvider = TemperatureProvider::None;
    TemperatureProvider cachedGpuProvider = TemperatureProvider::None;
    std::chrono::steady_clock::time_point nextExternalTemperatureRefresh{};
    while (!g_stopMetricsWorker) {
        ModSettings settings = CurrentSettings();
        FocusScene scene = ApplyExperienceModeToScene(
            settings.focusSceneEngineEnabled ? DetectFocusScene()
                                             : FocusScene::Normal);
        if (const wchar_t* reason = PerformanceSuspensionReason(scene)) {
            CloseReaderRequest();
            CloseMetricSources();
            publishWorkerRuntime(true, reason);
            WaitForSingleObject(g_metricsWorkerWakeEvent, 15000);
            firstSample = true;
            continue;
        }
        publishWorkerRuntime(false, L"");
        int activeInterval = EffectiveUpdateInterval(settings, scene);
        PublishReaderRequest(activeInterval);
        bool pdhCompletionReady = false;
        DWORD waitResult = WAIT_TIMEOUT;
        if (!firstSample) {
            EnsureExternalTelemetry();
            HANDLE waits[3]{g_metricsWorkerWakeEvent,
                            g_externalTelemetryChangedEvent, nullptr};
            DWORD waitCount = g_externalTelemetryChangedEvent ? 2 : 1;
            if (!g_externalTelemetryChangedEvent &&
                ArmPdhCompletion(settings, activeInterval)) {
                waits[waitCount++] = g_pdhCompletionEvent;
            }
            waitResult = WaitForMultipleObjects(
                waitCount, waits, FALSE,
                // A named event stays alive in this reader after its publisher
                // exits. A deadline is required to reach liveness/freshness
                // checks even if the companion never signals again.
                static_cast<DWORD>(activeInterval) * 1000);
            pdhCompletionReady = g_pdhCompletionEvent &&
                waitResult == WAIT_OBJECT_0 + waitCount - 1 &&
                waits[waitCount - 1] == g_pdhCompletionEvent;
        }
        firstSample = false;

        if (g_stopMetricsWorker) {
            break;
        }
        if (waitResult == WAIT_FAILED) {
            Wh_Log(L"Metrics worker wait failed: %u", GetLastError());
            break;
        }

        settings = CurrentSettings();
        scene = ApplyExperienceModeToScene(
            settings.focusSceneEngineEnabled ? DetectFocusScene()
                                             : FocusScene::Normal);
        MetricsSnapshot snapshot;
        snapshot.scene = scene;
        bool external = TryReadExternalTelemetry(snapshot);
        if (external) {
            if (!lastSampleWasExternal) {
                // The companion owns the frequent counters. Do not retain a
                // fallback PDH query after external telemetry recovers.
                ClosePdhQuery();
            }
            auto now = std::chrono::steady_clock::now();
            if (now >= nextExternalTemperatureRefresh) {
                // Only Windows thermal-zone sources need PDH here. Do not
                // recreate GPU wildcard counters already owned by the companion.
                EnsurePdhQuery(settings, PdhQueryDemand::ThermalOnly);
                if (g_pdhQuery) {
                    PdhCollectQueryData(g_pdhQuery);
                }
                ReadTemperatures(snapshot, settings);
                // Temperature fallback is sampled only every 30 seconds. Free
                // its PDH query between reads instead of carrying counters and
                // completion handles for the whole Explorer session.
                ClosePdhQuery();
                cachedCpuTemp = snapshot.cpuTemp;
                cachedGpuTemp = snapshot.gpuTemp;
                cachedCpuProvider = snapshot.cpuTempProvider;
                cachedGpuProvider = snapshot.gpuTempProvider;
                nextExternalTemperatureRefresh =
                    now + std::chrono::seconds(30);
            } else {
                snapshot.cpuTemp = cachedCpuTemp;
                snapshot.gpuTemp = cachedGpuTemp;
                snapshot.cpuTempProvider = cachedCpuProvider;
                snapshot.gpuTempProvider = cachedGpuProvider;
            }
        } else {
            snapshot = CollectMetrics(settings, scene, pdhCompletionReady);
            cachedCpuTemp = snapshot.cpuTemp;
            cachedGpuTemp = snapshot.gpuTemp;
            cachedCpuProvider = snapshot.cpuTempProvider;
            cachedGpuProvider = snapshot.gpuTempProvider;
            nextExternalTemperatureRefresh = {};
        }
        if (!telemetrySourceLogged || external != lastSampleWasExternal) {
            Wh_Log(L"Metric source: %s",
                   external ? L"Maxwell.Shell.Core shared state"
                            : L"in-process fallback");
            lastSampleWasExternal = external;
            telemetrySourceLogged = true;
        }
        if (!providersLogged ||
            snapshot.cpuTempProvider != lastCpuProvider ||
            snapshot.gpuTempProvider != lastGpuProvider) {
            Wh_Log(L"Temperature providers: CPU=%s, GPU=%s",
                   TemperatureProviderName(snapshot.cpuTempProvider),
                   TemperatureProviderName(snapshot.gpuTempProvider));
            lastCpuProvider = snapshot.cpuTempProvider;
            lastGpuProvider = snapshot.gpuTempProvider;
            providersLogged = true;
        }
        PublishMetrics(std::move(snapshot));
    }

    CloseMetricSources();
    CloseExternalTelemetry();
    CloseReaderRequest();
}

bool StartMetricsWorker() {
    std::lock_guard lock(g_metricsWorkerMutex);
    if (g_metricsWorker) {
        return true;
    }
    if (g_unloading) {
        return false;
    }

    g_metricsWorkerWakeEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!g_metricsWorkerWakeEvent) {
        Wh_Log(L"Creating metrics worker event failed: %u", GetLastError());
        return false;
    }

    {
        std::lock_guard metricsLock(g_metricsMutex);
        g_latestMetrics = {};
        g_latestMetricsSequence = 0;
        g_latestMetricsAvailable = false;
    }
    g_stopMetricsWorker = false;
    try {
        g_metricsWorker.emplace(MetricsWorkerProc);
    } catch (...) {
        CloseHandle(g_metricsWorkerWakeEvent);
        g_metricsWorkerWakeEvent = nullptr;
        Wh_Log(L"Starting metrics worker failed");
        return false;
    }
    return true;
}

void WakeMetricsWorker() {
    std::lock_guard lock(g_metricsWorkerMutex);
    if (g_metricsWorkerWakeEvent) {
        SetEvent(g_metricsWorkerWakeEvent);
    }
}

void StopMetricsWorker() {
    std::lock_guard lock(g_metricsWorkerMutex);
    g_stopMetricsWorker = true;
    if (g_metricsWorkerWakeEvent) {
        SetEvent(g_metricsWorkerWakeEvent);
    }
    if (g_metricsWorker) {
        if (g_metricsWorker->joinable()) {
            g_metricsWorker->join();
        }
        g_metricsWorker.reset();
    }
    if (g_metricsWorkerWakeEvent) {
        CloseHandle(g_metricsWorkerWakeEvent);
        g_metricsWorkerWakeEvent = nullptr;
    }

    std::lock_guard metricsLock(g_metricsMutex);
    g_latestMetrics = {};
    g_latestMetricsSequence = 0;
    g_latestMetricsAvailable = false;
}

std::wstring FormatFixed(double value, int decimals) {
    wchar_t buffer[64];
    swprintf(buffer, std::size(buffer), decimals == 0 ? L"%.0f" : L"%.1f",
             value);
    return buffer;
}

std::wstring FormatPercent(double value) {
    wchar_t buffer[64];
    swprintf(buffer, std::size(buffer), L"%.0f%%",
             std::clamp(value, 0.0, 100.0));
    return buffer;
}

ExperienceMode SetExperienceMode(ExperienceMode next) {
    g_experienceMode.store(next);
    g_contextDensityApplied = false;
    g_lastAvailableWidth = -1.0;
    g_lastRenderedMetricsSequence = 0;
    Wh_Log(L"Taskbar experience mode: %s", ExperienceModeName(next));
    WakeMetricsWorker();
    return next;
}

ExperienceMode CycleExperienceMode() {
    ExperienceMode next = ExperienceMode::Auto;
    switch (g_experienceMode.load()) {
        case ExperienceMode::Auto:
            next = ExperienceMode::Balanced;
            break;
        case ExperienceMode::Balanced:
            next = ExperienceMode::Focus;
            break;
        case ExperienceMode::Focus:
            next = ExperienceMode::Gaming;
            break;
        case ExperienceMode::Gaming:
            next = ExperienceMode::Minimal;
            break;
        case ExperienceMode::Minimal:
        default:
            next = ExperienceMode::Auto;
            break;
    }
    return SetExperienceMode(next);
}

std::wstring FormatLoadPercent(double value, bool available = true) {
    return available ? FormatPercent(value) + L" load" : L"--% load";
}

std::wstring FormatUsedPercent(double value, bool available = true) {
    return available ? FormatPercent(value) + L" used" : L"--% used";
}

std::wstring FormatTemperature(const std::optional<double>& value,
                               const ModSettings& settings) {
    bool fahrenheit = settings.temperatureUnit == TemperatureUnit::Fahrenheit;
    std::wstring suffix = fahrenheit ? L"°F" : L"°C";
    if (!value) {
        return L"--" + suffix;
    }
    double displayed = fahrenheit ? (*value * 9.0 / 5.0 + 32.0) : *value;
    return FormatFixed(displayed, 0) + suffix;
}

std::wstring FormatTemperatureThreshold(double celsius,
                                        const ModSettings& settings) {
    bool fahrenheit = settings.temperatureUnit == TemperatureUnit::Fahrenheit;
    double displayed = fahrenheit ? (celsius * 9.0 / 5.0 + 32.0) : celsius;
    return FormatFixed(displayed, 0) + (fahrenheit ? L"°F" : L"°C");
}

std::wstring FormatCapacity(double usedGb, double totalGb, bool available) {
    if (!available || !std::isfinite(usedGb) || !std::isfinite(totalGb) ||
        totalGb <= 0.0) {
        return L"--/--G";
    }
    int totalDecimals = totalGb < 1.0 ? 1 : 0;
    return FormatFixed(usedGb, 1) + L"/" +
           FormatFixed(totalGb, totalDecimals) + L"G";
}

enum class AlertLevel { Normal, Warning, Critical };

AlertLevel MaxAlert(AlertLevel first, AlertLevel second) {
    return static_cast<int>(first) >= static_cast<int>(second) ? first : second;
}

AlertLevel g_cpuTemperatureAlert = AlertLevel::Normal;
AlertLevel g_gpuTemperatureAlert = AlertLevel::Normal;
AlertLevel g_cpuUsageAlert = AlertLevel::Normal;
AlertLevel g_gpuUsageAlert = AlertLevel::Normal;
AlertLevel g_ramAlert = AlertLevel::Normal;
AlertLevel g_vramAlert = AlertLevel::Normal;
AlertLevel g_auraPulseAlert = AlertLevel::Normal;

// Cached last-written visual state. The renderer already avoids unchanged text,
// color, and bar writes; these let the ambient edge and activity rail follow the
// same rule instead of allocating fresh brushes and tooltips on every tick.
std::optional<Color> g_auraAppliedBackground;
std::optional<Color> g_auraAppliedBorder;
std::optional<FocusScene> g_auraAppliedTooltipScene;
std::optional<AlertLevel> g_cpuActivityAlert;
std::optional<AlertLevel> g_gpuActivityAlert;
std::optional<AlertLevel> g_ramActivityAlert;
std::optional<AlertLevel> g_vramActivityAlert;

struct PresentationState {
    bool showGraphs;
    bool showCapacity;
    bool showMemoryBars;
    bool showSecondary;
    bool showRail;
    double opacity;
};
std::optional<PresentationState> g_appliedPresentation;

bool SameColor(const Color& first, const Color& second) {
    return first.A == second.A && first.R == second.R && first.G == second.G &&
           first.B == second.B;
}

void ResetVisualWriteCache() {
    g_auraAppliedBackground.reset();
    g_auraAppliedBorder.reset();
    g_auraAppliedTooltipScene.reset();
    g_cpuActivityAlert.reset();
    g_gpuActivityAlert.reset();
    g_ramActivityAlert.reset();
    g_vramActivityAlert.reset();
    g_appliedPresentation.reset();
}

template <typename T>
void SetVisibilityIfChanged(T element, Visibility visibility) {
    if (element && element.Visibility() != visibility) {
        element.Visibility(visibility);
    }
}

AlertLevel EvaluateAlert(double value,
                         double warning,
                         double critical,
                         AlertLevel previous,
                         double releaseMargin) {
    if (!std::isfinite(value)) {
        return AlertLevel::Normal;
    }
    if (value >= critical ||
        (previous == AlertLevel::Critical &&
         value >= critical - releaseMargin)) {
        return AlertLevel::Critical;
    }
    if (value >= warning ||
        (previous != AlertLevel::Normal && value >= warning - releaseMargin)) {
        return AlertLevel::Warning;
    }
    return AlertLevel::Normal;
}

std::optional<Color> ParseColor(const std::wstring& value) {
    std::wstring hex = value;
    if (!hex.empty() && hex.front() == L'#') {
        hex.erase(hex.begin());
    }
    if (hex.size() != 6 && hex.size() != 8) {
        return std::nullopt;
    }
    if (!std::all_of(hex.begin(), hex.end(), [](wchar_t character) {
            return std::iswxdigit(character) != 0;
        })) {
        return std::nullopt;
    }

    wchar_t* end = nullptr;
    unsigned long parsed = std::wcstoul(hex.c_str(), &end, 16);
    if (!end || *end) {
        return std::nullopt;
    }

    Color color{};
    if (hex.size() == 8) {
        color.A = static_cast<uint8_t>((parsed >> 24) & 0xFF);
    } else {
        color.A = 0xFF;
    }
    color.R = static_cast<uint8_t>((parsed >> 16) & 0xFF);
    color.G = static_cast<uint8_t>((parsed >> 8) & 0xFF);
    color.B = static_cast<uint8_t>(parsed & 0xFF);
    return color;
}

Color MakeColor(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue) {
    Color color{};
    color.A = alpha;
    color.R = red;
    color.G = green;
    color.B = blue;
    return color;
}

SolidColorBrush BrushFromSetting(const std::wstring& value, Color fallback) {
    return SolidColorBrush(ParseColor(value).value_or(fallback));
}

// Folds the configured text opacity, and the quieter label role, into the brush
// alpha. Carrying the fade here rather than on UIElement.Opacity keeps the
// glyph antialiasing sharp: element opacity composites the rasterized text.
Color WithTextAlpha(Color color, const ModSettings& settings, bool label) {
    double scale =
        static_cast<double>(std::clamp(settings.textOpacity, 0, 100)) / 100.0;
    if (label) {
        scale *= 0.78;
    }
    double alpha = std::clamp(static_cast<double>(color.A) * scale, 0.0, 255.0);
    color.A = static_cast<uint8_t>(std::lround(alpha));
    return color;
}

void SetTextForeground(TextBlock text,
                       AlertLevel alert,
                       const ModSettings& settings,
                       bool label = false) {
    if (!text) {
        return;
    }

    std::optional<Color> color;
    if (alert == AlertLevel::Critical) {
        color = ParseColor(settings.criticalColor);
    } else if (alert == AlertLevel::Warning) {
        color = ParseColor(settings.warningColor);
    } else {
        color = ParseColor(settings.textColor);
    }

    text.Foreground(SolidColorBrush(WithTextAlpha(
        color.value_or(MakeColor(0xFF, 0xF5, 0xF5, 0xF7)), settings, label)));
}

// Segoe UI Variable carries weight cheaply, so alert state gets a second
// channel alongside hue. Weight survives colour blindness and reads faster in
// peripheral vision than a hue shift does.
void SetAlertWeight(TextBlock text, AlertLevel alert) {
    switch (alert) {
        case AlertLevel::Critical:
            text.FontWeight(Text::FontWeights::SemiBold());
            break;
        case AlertLevel::Warning:
            text.FontWeight(Text::FontWeights::Medium());
            break;
        case AlertLevel::Normal:
        default:
            text.FontWeight(Text::FontWeights::Normal());
            break;
    }
}

void SetMetricForeground(TextBlock text,
                         AlertLevel alert,
                         const ModSettings& settings) {
    if (!text) {
        return;
    }

    std::optional<Color> color;
    if (alert == AlertLevel::Critical) {
        color = ParseColor(settings.criticalColor);
    } else if (alert == AlertLevel::Warning) {
        color = ParseColor(settings.warningColor);
    } else {
        color = ParseColor(settings.textColor);
    }

    text.Foreground(SolidColorBrush(WithTextAlpha(
        color.value_or(MakeColor(0xFF, 0xF5, 0xF5, 0xF7)), settings, false)));
    SetAlertWeight(text, alert);
}

void SetTemperatureForeground(TextBlock text,
                              AlertLevel alert,
                              const ModSettings& settings) {
    if (!text) {
        return;
    }

    std::optional<Color> color;
    if (alert == AlertLevel::Critical) {
        color = ParseColor(settings.criticalColor);
    } else if (alert == AlertLevel::Warning) {
        color = ParseColor(settings.warningColor);
    } else {
        color = ParseColor(settings.safeColor);
    }

    text.Foreground(SolidColorBrush(WithTextAlpha(
        color.value_or(MakeColor(0xFF, 0x8B, 0xD4, 0x9C)), settings, false)));
    SetAlertWeight(text, alert);
}

Color AlertColor(AlertLevel alert, const ModSettings& settings) {
    if (alert == AlertLevel::Critical) {
        return ParseColor(settings.criticalColor)
            .value_or(MakeColor(0xFF, 0xFF, 0x6B, 0x6B));
    }
    if (alert == AlertLevel::Warning) {
        return ParseColor(settings.warningColor)
            .value_or(MakeColor(0xFF, 0xFF, 0xB9, 0x00));
    }
    return ParseColor(settings.graphColor)
        .value_or(MakeColor(0xFF, 0x78, 0xA8, 0xFF));
}

SolidColorBrush AlertBrush(AlertLevel alert, const ModSettings& settings) {
    return SolidColorBrush(AlertColor(alert, settings));
}

AlertLevel OverallAlert() {
    return MaxAlert(MaxAlert(g_cpuUsageAlert, g_cpuTemperatureAlert),
                    MaxAlert(MaxAlert(g_gpuUsageAlert, g_gpuTemperatureAlert),
                             MaxAlert(g_ramAlert, g_vramAlert)));
}

Color SceneAccent(FocusScene scene) {
    switch (scene) {
        case FocusScene::Gaming:
            return MakeColor(0xFF, 0xB9, 0x78, 0xFF);
        case FocusScene::Presentation:
            return MakeColor(0xFF, 0x6C, 0xE5, 0xE8);
        case FocusScene::BatterySaver:
            return MakeColor(0xFF, 0x8B, 0xD4, 0x9C);
        case FocusScene::Focus:
            return MakeColor(0xFF, 0x78, 0xA8, 0xFF);
        case FocusScene::Normal:
        default:
            return MakeColor(0xFF, 0x78, 0xA8, 0xFF);
    }
}

void StopAuraPulse() {
    if (g_auraPulseStoryboard) {
        try {
            g_auraPulseStoryboard.Stop();
        } catch (...) {
        }
        g_auraPulseStoryboard = nullptr;
    }
    g_auraPulseAlert = AlertLevel::Normal;
    if (g_auraBorder) {
        g_auraBorder.Opacity(1.0);
    }
}

void UpdateAuraPulse(AlertLevel alert) {
    if (g_reducedMotion || g_highContrast) {
        StopAuraPulse();
        return;
    }
    if (!g_auraBorder) {
        return;
    }
    if (alert == AlertLevel::Normal) {
        StopAuraPulse();
        return;
    }
    if (g_auraPulseStoryboard && g_auraPulseAlert == alert) {
        return;
    }

    StopAuraPulse();
    BOOL animationsEnabled = TRUE;
    if (SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0,
                              &animationsEnabled, 0) &&
        !animationsEnabled) {
        g_auraPulseAlert = alert;
        return;
    }
    try {
        Storyboard storyboard;
        DoubleAnimation pulse;
        pulse.From(alert == AlertLevel::Critical ? 0.52 : 0.68);
        pulse.To(1.0);
        pulse.Duration(DurationHelper::FromTimeSpan(std::chrono::milliseconds(
            alert == AlertLevel::Critical ? 700 : 1200)));
        pulse.AutoReverse(true);
        pulse.RepeatBehavior(RepeatBehaviorHelper::Forever());
        Storyboard::SetTarget(pulse, g_auraBorder);
        Storyboard::SetTargetProperty(pulse, L"Opacity");
        storyboard.Children().Append(pulse);
        g_auraPulseStoryboard = storyboard;
        g_auraPulseAlert = alert;
        storyboard.Begin();
    } catch (...) {
        g_auraPulseStoryboard = nullptr;
        g_auraPulseAlert = alert;
        g_auraBorder.Opacity(1.0);
    }
}

void UpdatePerformanceAura(const MetricsSnapshot& snapshot,
                           const ModSettings& settings) {
    if (!g_auraBorder) {
        return;
    }
    SetVisibilityIfChanged(g_auraBorder,
                           settings.performanceAuraEnabled
                               ? Visibility::Visible
                               : Visibility::Collapsed);
    if (!settings.performanceAuraEnabled) {
        StopAuraPulse();
        return;
    }

    AlertLevel overall = OverallAlert();
    Color accent = SceneAccent(snapshot.scene);
    if (overall == AlertLevel::Critical) {
        accent = ParseColor(settings.criticalColor)
                     .value_or(MakeColor(0xFF, 0xFF, 0x6B, 0x6B));
    } else if (overall == AlertLevel::Warning) {
        accent = ParseColor(settings.warningColor)
                     .value_or(MakeColor(0xFF, 0xFF, 0xB9, 0x00));
    }
    Color background = accent;
    background.A = overall == AlertLevel::Critical
                       ? 0x20
                       : (overall == AlertLevel::Warning ? 0x14 : 0x08);
    Color border = accent;
    // Staged rather than a cliff. The old 0x50 -> 0xD8 jump fired at the same
    // instant the pulse storyboard started, which read as alarming rather than
    // informative; warning now shifts colour, critical adds the full weight.
    border.A = overall == AlertLevel::Critical
                   ? 0xD8
                   : (overall == AlertLevel::Warning ? 0x94 : 0x50);
    if (!g_auraAppliedBackground ||
        !SameColor(*g_auraAppliedBackground, background)) {
        g_auraBorder.Background(SolidColorBrush(background));
        g_auraAppliedBackground = background;
    }
    if (!g_auraAppliedBorder || !SameColor(*g_auraAppliedBorder, border)) {
        g_auraBorder.BorderBrush(SolidColorBrush(border));
        g_auraAppliedBorder = border;
    }
    UpdateAuraPulse(overall);

    // The tooltip text only depends on the scene. Rewriting it every tick also
    // dismissed a tooltip the pointer was resting on.
    if (!g_auraAppliedTooltipScene ||
        *g_auraAppliedTooltipScene != snapshot.scene) {
        ToolTipService::SetToolTip(
            g_auraBorder,
            box_value(std::wstring(L"Scene: ") +
                      FocusSceneName(snapshot.scene) +
                      L". Click a metric for Hardware Command Center."));
        g_auraAppliedTooltipScene = snapshot.scene;
    }
}

void SetActivitySegment(XamlRectangle segment,
                        double value,
                        bool available,
                        AlertLevel alert,
                        const ModSettings& settings,
                        std::optional<AlertLevel>& appliedAlert) {
    if (!segment) {
        return;
    }
    // AlertBrush re-parses a color string and allocates a brush, so only run it
    // when the segment actually changes state.
    if (!appliedAlert || *appliedAlert != alert) {
        segment.Fill(AlertBrush(alert, settings));
        appliedAlert = alert;
    }
    // Perceived lightness follows roughly the 1/2.2 power of linear intensity,
    // so a linear ramp reads as bright far too early and wastes most of its
    // resolution below 50%. Pre-warping by 2.2 puts mid load at mid brightness
    // and keeps usable contrast across the 70-100% band that actually matters.
    double opacity = 0.08;
    if (available) {
        double load = std::clamp(value, 0.0, 100.0) / 100.0;
        opacity = 0.20 + std::pow(load, 2.2) * 0.80;
    }
    if (std::abs(segment.Opacity() - opacity) >= 0.004) {
        segment.Opacity(opacity);
    }
}

void UpdateActivityRail(const MetricsSnapshot& snapshot,
                        const ModSettings& settings) {
    // Focus transformation owns visibility. In compact mode the old renderer
    // made this rail visible, repainted four segments, then collapsed it again
    // on every sample. Avoid that dependency-property churn entirely.
    if (!g_activityRail ||
        g_activityRail.Visibility() != Visibility::Visible) {
        return;
    }
    SetActivitySegment(g_cpuActivity, snapshot.cpu, true, g_cpuUsageAlert,
                       settings, g_cpuActivityAlert);
    SetActivitySegment(g_gpuActivity, snapshot.gpu, snapshot.gpuAvailable,
                       g_gpuUsageAlert, settings, g_gpuActivityAlert);
    SetActivitySegment(g_ramActivity, snapshot.ram, true, g_ramAlert, settings,
                       g_ramActivityAlert);
    SetActivitySegment(g_vramActivity, snapshot.vram, snapshot.vramAvailable,
                       g_vramAlert, settings, g_vramActivityAlert);
}

size_t HistoryCapacity(const ModSettings& settings,
                       FocusScene scene = FocusScene::Normal) {
    int interval = EffectiveUpdateInterval(settings, scene);
    int intervals =
        (settings.historySeconds + interval - 1) / interval;
    return std::max<size_t>(2, static_cast<size_t>(intervals) + 1);
}

void AppendHistory(std::deque<double>& history,
                   double value,
                   size_t capacity) {
    history.push_back(std::clamp(value, 0.0, 100.0));
    while (history.size() > capacity) {
        history.pop_front();
    }
}

void UpdateSparkline(XamlPolyline graph,
                     const std::deque<double>& history,
                     size_t capacity) {
    // PointCollection rebuilds allocate and run on Explorer's UI thread. Keep
    // history in the cheap deque, but don't materialize points for a collapsed
    // graph (the default compact presentation has both graphs hidden).
    if (!graph || graph.Visibility() != Visibility::Visible) {
        return;
    }

    auto points = graph.Points();
    points.Clear();
    if (history.size() < 2 || capacity < 2 || g_graphWidth <= 1.0) {
        graph.Visibility(Visibility::Collapsed);
        return;
    }

    constexpr double verticalPadding = 1.0;
    double usableHeight = kGraphHeight - verticalPadding * 2.0;
    double step = g_graphWidth / static_cast<double>(capacity - 1);
    double firstX =
        g_graphWidth - step * static_cast<double>(history.size() - 1);
    for (size_t i = 0; i < history.size(); i++) {
        double x = firstX + step * static_cast<double>(i);
        double y = verticalPadding +
                   (100.0 - std::clamp(history[i], 0.0, 100.0)) / 100.0 *
                       usableHeight;
        points.Append(Point{static_cast<float>(x), static_cast<float>(y)});
    }
    graph.Visibility(Visibility::Visible);
}

void UpdateMemoryBar(XamlRectangle fill,
                     ScaleTransform scale,
                     double percent,
                     bool available,
                     AlertLevel alert,
                     const ModSettings& settings,
                     bool alertChanged) {
    if (!fill || fill.Visibility() != Visibility::Visible) {
        return;
    }
    double scaleX = available
                        ? std::clamp(percent, 0.0, 100.0) / 100.0
                        : 0.0;
    // Width invalidates layout. ScaleX is a render transform, so the live bar
    // remains smooth without asking Explorer to measure and arrange the row.
    if (scale && std::abs(scale.ScaleX() - scaleX) >= 0.002) {
        scale.ScaleX(scaleX);
    }
    if (alertChanged) {
        fill.Fill(AlertBrush(alert, settings));
    }
}

void SetTextIfChanged(TextBlock text, const std::wstring& value) {
    if (text && wcscmp(text.Text().c_str(), value.c_str()) != 0) {
        text.Text(value);
    }
}

void SetTextColorIfChanged(TextBlock text, const Color& color) {
    if (!text) {
        return;
    }
    // Read the actual brush so theme changes and rebuilt elements do not need
    // a separate cache invalidation path. Non-solid/null brushes are replaced.
    auto current = text.Foreground().try_as<SolidColorBrush>();
    if (!current || !SameColor(current.Color(), color)) {
        text.Foreground(SolidColorBrush(color));
    }
}

void SetAutomationNameIfChanged(FrameworkElement element,
                                const std::wstring& value) {
    using winrt::Windows::UI::Xaml::Automation::AutomationProperties;
    if (element &&
        wcscmp(AutomationProperties::GetName(element).c_str(), value.c_str()) !=
            0) {
        AutomationProperties::SetName(element, value);
    }
}

template <typename F>
FrameworkElement FindChildRecursive(FrameworkElement element,
                                    F callback,
                                    int depth = 16) {
    if (!element || depth <= 0) {
        return nullptr;
    }
    int count = VisualTreeHelper::GetChildrenCount(element);
    for (int i = 0; i < count; i++) {
        auto child = VisualTreeHelper::GetChild(element, i)
                         .try_as<FrameworkElement>();
        if (!child) {
            continue;
        }
        if (callback(child)) {
            return child;
        }
        if (auto nested = FindChildRecursive(child, callback, depth - 1)) {
            return nested;
        }
    }
    return nullptr;
}

FrameworkElement FindDirectChildByName(FrameworkElement parent, PCWSTR name) {
    if (!parent) {
        return nullptr;
    }
    int count = VisualTreeHelper::GetChildrenCount(parent);
    for (int i = 0; i < count; i++) {
        auto child = VisualTreeHelper::GetChild(parent, i)
                         .try_as<FrameworkElement>();
        if (child && child.Name() == name) {
            return child;
        }
    }
    return nullptr;
}

void ApplyTextStyle(TextBlock text,
                    bool label,
                    const ModSettings& settings) {
    if (!text) {
        return;
    }
    text.FontFamily(Media::FontFamily(settings.fontFamily));
    text.FontSize(settings.fontSize);
    // Labels are quiet by alpha, not by weight. SemiBold-and-faded was a
    // contradictory signal; Regular at a slightly higher alpha reads cleaner
    // and leaves weight free to carry alert state on the values.
    text.FontWeight(Text::FontWeights::Normal());
    // Element opacity composites the rasterized glyphs, which dulls the
    // antialiased edges of small text. The same fade is carried in the brush
    // alpha instead (see SetTextForeground), so leave the element opaque.
    text.Opacity(1.0);
    if (!label) {
        Typography::SetNumeralAlignment(text, FontNumeralAlignment::Tabular);
        Typography::SetNumeralStyle(text, FontNumeralStyle::Lining);
    }
    text.TextWrapping(TextWrapping::NoWrap);
    text.TextTrimming(TextTrimming::CharacterEllipsis);
    SetTextForeground(text, AlertLevel::Normal, settings, label);
}

double ContextSceneWidth(const ModSettings& settings, FocusScene scene) {
    double reduction = 0.0;
    switch (scene) {
        case FocusScene::Gaming:
            reduction = 24.0;
            break;
        case FocusScene::Focus:
        case FocusScene::Presentation:
        case FocusScene::BatterySaver:
            reduction = 40.0;
            break;
        case FocusScene::Normal:
        default:
            break;
    }
    if (g_experienceMode.load() == ExperienceMode::Minimal) {
        reduction = std::max(reduction, 96.0);
    }
    return std::clamp(static_cast<double>(settings.width) - reduction,
                      kCompactHardwareWidth,
                      static_cast<double>(settings.width));
}

std::wstring TaskbarMonitorPersonality() {
    HWND window = g_taskbarWindow.load();
    HMONITOR monitor = window
                           ? MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST)
                           : nullptr;
    MONITORINFOEXW info{};
    info.cbSize = sizeof(info);
    if (!monitor || !GetMonitorInfoW(monitor, &info)) {
        return L"Performance display";
    }
    bool primary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;
    return std::wstring(primary ? L"Primary performance lane  ·  "
                                : L"Secondary context lane  ·  ") +
           info.szDevice;
}

// The RootGrid carries Opal's horizontal padding. TransformToVisual measures
// from the grid's outer edge while margins are laid out inside the padding, so
// every measurement here drops the padding before it is compared with a margin.
Thickness RootGridPadding() {
    try {
        return g_rootGrid ? g_rootGrid.Padding() : Thickness{};
    } catch (...) {
        return Thickness{};
    }
}

double AvailableLeftZoneWidth(const ModSettings& settings) {
    const Thickness padding = RootGridPadding();
    if (g_userLeft >= 0 && g_rootGrid) {
        try {
            double available = g_rootGrid.ActualWidth() - padding.Left -
                               padding.Right - EffectiveLeftOffset(settings);
            if (std::isfinite(available)) {
                return std::max(0.0, available);
            }
        } catch (...) {
        }
    }
    double available = -1.0;
    if (g_taskItemsRepeater && g_rootGrid) {
        try {
            auto transform = g_taskItemsRepeater.TransformToVisual(g_rootGrid);
            Point origin = transform.TransformPoint(Point{0.0F, 0.0F});
            // The repeater origin already reflects the margin that this mod
            // applies in ApplyReservedSpace. Subtracting g_reservedMargin a
            // second time creates a self-hiding feedback loop: once the lane
            // reserves its space, the next density pass decides that no space
            // remains and collapses the widget. Measure from the live origin
            // and remove only the physical inset/gap.
            available = static_cast<double>(origin.X) - padding.Left -
                        EffectiveLeftOffset(settings) - settings.reserveGap;
        } catch (...) {
        }
    }
    if (available > 0.0) {
        return available;
    }
    HWND taskbarWindow = g_taskbarWindow.load();
    RECT taskbarRect{};
    if (taskbarWindow && GetWindowRect(taskbarWindow, &taskbarRect)) {
        double taskbarWidth = taskbarRect.right - taskbarRect.left;
        double taskbarHeight = taskbarRect.bottom - taskbarRect.top;
        if (taskbarWidth > taskbarHeight && taskbarWidth > 0) {
            return taskbarWidth * 0.23;
        }
    }
    return static_cast<double>(settings.width);
}

ContentPriority ResolveContentPriority(double available,
                                       double requested,
                                       const ModSettings& settings) {
    // Geometry is a safety constraint even when an old profile disables
    // adaptive content. Two metric columns cannot fit inside a 260-DIP lane.
    // Never treat a tight left lane as "no widget". Compact CPU/RAM at
    // kCompactHardwareWidth is the floor; overlap is handled by reserved
    // space, not by collapsing Computer stats off the taskbar.
    double effective = std::min(available, requested);
    if (effective < kCompactHardwareWidth) {
        return ContentPriority::Essential;
    }
    if (effective >= 360.0) {
        return ContentPriority::Full;
    }
    if (effective >= 310.0) {
        return ContentPriority::Balanced;
    }
    return ContentPriority::Essential;
}

void ApplyWidgetGeometry(const ModSettings& settings, FocusScene scene) {
    if (!g_widget) {
        return;
    }

    double requestedWidth = ContextSceneWidth(settings, scene);
    double available = settings.adaptiveOverlapEnabled
                           ? AvailableLeftZoneWidth(settings)
                           : requestedWidth;
    g_contentPriority = ResolveContentPriority(available, requestedWidth, settings);
    g_lastAvailableWidth = available;
    g_widget.Visibility(Visibility::Visible);
    double boundedAvailable =
        available > 0.0 ? std::max(available, kCompactHardwareWidth)
                        : requestedWidth;
    g_effectiveWidgetWidth = std::clamp(std::min(requestedWidth, boundedAvailable),
                                        kCompactHardwareWidth,
                                        requestedWidth);
    double columnGap = g_contentPriority == ContentPriority::Full
                           ? kColumnGap
                           : (g_contentPriority == ContentPriority::Balanced
                                  ? 10.0
                                  : 0.0);
    // Compact is deliberately CPU/RAM-only. Give those primary rows the whole
    // 184-DIP lane; wider modes split evenly so neither metric family clips.
    double rightWidth = g_contentPriority == ContentPriority::Essential
                            ? 0.0
                            : (g_effectiveWidgetWidth - columnGap) * 0.5;
    double leftWidth = g_effectiveWidgetWidth - columnGap - rightWidth;
    g_graphWidth = std::max(
        0.0, leftWidth - (2.0 * kContentHorizontalInset) -
                 kMetricLabelWidth - kMetricUsageWidth - kMetricTempWidth -
                 kGraphLeftGap);
    g_memoryBarWidth = g_contentPriority == ContentPriority::Essential
                           ? leftWidth - (2.0 * kContentHorizontalInset)
                           : rightWidth - (2.0 * kContentHorizontalInset);
    g_memoryBarWidth = std::max(0.0, g_memoryBarWidth);

    g_widget.Width(g_effectiveWidgetWidth);
    g_widget.Height(kWidgetHeight);
    if (g_leftColumn) {
        g_leftColumn.Width(GridLength{leftWidth, GridUnitType::Pixel});
    }
    if (g_gapColumn) {
        g_gapColumn.Width(GridLength{columnGap, GridUnitType::Pixel});
    }
    if (g_rightColumn) {
        g_rightColumn.Width(GridLength{rightWidth, GridUnitType::Pixel});
    }
    for (XamlPolyline graph : {g_cpuGraph, g_gpuGraph}) {
        if (graph) {
            graph.Width(g_graphWidth);
            graph.Height(kGraphHeight);
        }
    }
    for (XamlRectangle bar :
         {g_ramTrack, g_ramFill, g_vramTrack, g_vramFill}) {
        if (bar) {
            bar.Width(g_memoryBarWidth);
        }
    }
}

void ApplyReservedSpace(const ModSettings& settings) {
    if (!g_taskItemsRepeater) {
        return;
    }

    Thickness margin = g_taskItemsRepeater.Margin();
    margin.Left -= g_reservedMargin;
    g_reservedMargin = g_userLeft < 0 && settings.reserveSpace
                            ? EffectiveLeftOffset(settings) +
                                  g_effectiveWidgetWidth +
                                  settings.reserveGap
                            : 0.0;
    margin.Left += g_reservedMargin;
    g_taskItemsRepeater.Margin(margin);
}

void ApplyAdaptiveOverlapGovernor(const ModSettings& settings,
                                  FocusScene scene) {
    ApplyWidgetGeometry(settings, scene);
}

void LaunchMonitoringTool(bool resourceMonitor) {
    PCWSTR executable = resourceMonitor ? L"resmon.exe" : L"taskmgr.exe";
    auto result = ShellExecuteW(nullptr, L"open", executable, nullptr, nullptr,
                                SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        Wh_Log(L"Failed to open %s (ShellExecute code %lld)", executable,
               static_cast<long long>(reinterpret_cast<INT_PTR>(result)));
    }
}

void ApplyReservedSpace(const ModSettings& settings);

void ApplyFocusTransformation(const MetricsSnapshot& snapshot,
                              const ModSettings& settings) {
    bool calmScene = snapshot.scene == FocusScene::Presentation ||
                     snapshot.scene == FocusScene::Focus;
    bool minimalMode = g_experienceMode.load() == ExperienceMode::Minimal;
    bool showGraphs = settings.showInlineGraphs && !calmScene && !minimalMode &&
                      (g_contentPriority == ContentPriority::Full ||
                       g_contentPriority == ContentPriority::Balanced);
    bool showCapacity = !calmScene && !minimalMode &&
                        (g_contentPriority == ContentPriority::Full ||
                         g_contentPriority == ContentPriority::Essential);
    bool showMemoryBars = !minimalMode;
    bool showSecondary = !minimalMode &&
                          g_contentPriority != ContentPriority::Essential;
    bool showRail = settings.activityRailEnabled && !calmScene &&
                    !minimalMode && showSecondary;

    double opacity = 1.0;
    if (minimalMode) {
        opacity = 0.74;
    } else if (calmScene) {
        opacity = 0.82;
    } else if (snapshot.scene == FocusScene::BatterySaver) {
        opacity = 0.90;
    }

    PresentationState desired{showGraphs, showCapacity, showMemoryBars,
                              showSecondary, showRail, opacity};
    if (g_appliedPresentation) {
        const auto& applied = *g_appliedPresentation;
        if (applied.showGraphs == desired.showGraphs &&
            applied.showCapacity == desired.showCapacity &&
            applied.showMemoryBars == desired.showMemoryBars &&
            applied.showSecondary == desired.showSecondary &&
            applied.showRail == desired.showRail &&
            std::abs(applied.opacity - desired.opacity) < 0.001) {
            return;
        }
    }

    SetVisibilityIfChanged(g_cpuRow, Visibility::Visible);
    SetVisibilityIfChanged(g_ramRow, Visibility::Visible);
    for (Grid secondary : {g_gpuRow, g_vramRow}) {
        SetVisibilityIfChanged(secondary, showSecondary ? Visibility::Visible
                                                        : Visibility::Collapsed);
    }

    for (XamlPolyline graph : {g_cpuGraph, g_gpuGraph}) {
        SetVisibilityIfChanged(graph, showGraphs ? Visibility::Visible
                                                 : Visibility::Collapsed);
    }
    for (TextBlock capacity : {g_ramCapacityText, g_vramCapacityText}) {
        SetVisibilityIfChanged(capacity, showCapacity ? Visibility::Visible
                                                      : Visibility::Collapsed);
    }
    for (XamlRectangle bar : {g_ramTrack, g_ramFill, g_vramTrack, g_vramFill}) {
        SetVisibilityIfChanged(bar, showMemoryBars ? Visibility::Visible
                                                   : Visibility::Collapsed);
    }
    SetVisibilityIfChanged(g_activityRail, showRail ? Visibility::Visible
                                                    : Visibility::Collapsed);
    if (std::abs(g_widget.Opacity() - opacity) >= 0.001) {
        g_widget.Opacity(opacity);
    }
    g_appliedPresentation = desired;
}

void ApplyContextAwareDensity(const MetricsSnapshot& snapshot,
                              const ModSettings& settings) {
    if (!g_widget) {
        return;
    }

    double available = settings.adaptiveOverlapEnabled
                           ? AvailableLeftZoneWidth(settings)
                           : ContextSceneWidth(settings, snapshot.scene);
    bool geometryChanged = !g_contextDensityApplied ||
                           snapshot.scene != g_appliedScene ||
                           std::abs(available - g_lastAvailableWidth) >= 4.0;
    if (geometryChanged) {
        ApplyAdaptiveOverlapGovernor(settings, snapshot.scene);
        ApplyReservedSpace(settings);
        if (g_timer) {
            g_timer.Interval(std::chrono::seconds(
                EffectiveUpdateInterval(settings, snapshot.scene)));
        }
        g_appliedScene = snapshot.scene;
        g_contextDensityApplied = true;
    }
    ApplyFocusTransformation(snapshot, settings);
}

void LaunchShellTarget(PCWSTR target) {
    auto result = ShellExecuteW(nullptr, L"open", target, nullptr, nullptr,
                                SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        Wh_Log(L"Failed to open %s (ShellExecute code %lld)", target,
               static_cast<long long>(reinterpret_cast<INT_PTR>(result)));
    }
}

struct ProcessMemoryEntry {
    std::wstring name;
    uint64_t bytes = 0;
};

struct ProcessMemoryReport {
    std::vector<ProcessMemoryEntry> top;
    uint32_t processCount = 0;
};

std::wstring FormatMemoryBytes(uint64_t bytes) {
    wchar_t value[64]{};
    if (bytes >= 1024ull * 1024ull * 1024ull) {
        swprintf(value, std::size(value), L"%.1f GB",
                 static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0));
    } else {
        swprintf(value, std::size(value), L"%.0f MB",
                 static_cast<double>(bytes) / (1024.0 * 1024.0));
    }
    return value;
}

std::wstring FormatUptime() {
    uint64_t minutes = GetTickCount64() / 60000ull;
    uint64_t days = minutes / 1440ull;
    minutes %= 1440ull;
    uint64_t hours = minutes / 60ull;
    minutes %= 60ull;
    wchar_t value[64]{};
    if (days > 0) {
        swprintf(value, std::size(value), L"%llud %lluh", days, hours);
    } else if (hours > 0) {
        swprintf(value, std::size(value), L"%lluh %llum", hours, minutes);
    } else {
        swprintf(value, std::size(value), L"%llum", minutes);
    }
    return value;
}

bool CopyTextToClipboard(const std::wstring& text) {
    if (!OpenClipboard(nullptr)) {
        return false;
    }
    bool copied = false;
    if (EmptyClipboard()) {
        size_t bytes = (text.size() + 1) * sizeof(wchar_t);
        HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (handle) {
            void* buffer = GlobalLock(handle);
            if (buffer) {
                memcpy(buffer, text.c_str(), bytes);
                GlobalUnlock(handle);
                copied = SetClipboardData(CF_UNICODETEXT, handle) != nullptr;
            }
            if (!copied) {
                GlobalFree(handle);
            }
        }
    }
    CloseClipboard();
    return copied;
}

// One process walk answers both questions the panel asks: how many processes
// are running, and which of them actually hold the memory.
ProcessMemoryReport CollectProcessMemory(size_t topCount) {
    ProcessMemoryReport report;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return report;
    }

    std::vector<ProcessMemoryEntry> entries;
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (!entry.th32ProcessID) {
                continue;
            }
            ++report.processCount;
            if (entry.th32ProcessID == GetCurrentProcessId()) {
                continue;
            }
            HANDLE process = OpenProcess(
                PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE,
                entry.th32ProcessID);
            if (!process) {
                continue;
            }
            PROCESS_MEMORY_COUNTERS_EX counters{};
            counters.cb = sizeof(counters);
            if (K32GetProcessMemoryInfo(
                    process,
                    reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
                    sizeof(counters))) {
                entries.push_back(ProcessMemoryEntry{
                    entry.szExeFile,
                    static_cast<uint64_t>(counters.WorkingSetSize)});
            }
            CloseHandle(process);
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);

    size_t wanted = std::min(topCount, entries.size());
    std::partial_sort(
        entries.begin(),
        entries.begin() + static_cast<std::ptrdiff_t>(wanted), entries.end(),
        [](const ProcessMemoryEntry& first, const ProcessMemoryEntry& second) {
            return first.bytes > second.bytes;
        });
    entries.resize(wanted);
    report.top = std::move(entries);
    return report;
}

PCWSTR SystemHealthName(AlertLevel level) {
    switch (level) {
        case AlertLevel::Critical:
            return L"Critical";
        case AlertLevel::Warning:
            return L"Needs attention";
        case AlertLevel::Normal:
        default:
            return L"Safe";
    }
}

std::wstring FormatThermalHeadroom(const std::optional<double>& temperature,
                                   double warningCelsius,
                                   const ModSettings& settings) {
    if (!temperature) {
        return L"Sensor unavailable";
    }
    double deltaCelsius = warningCelsius - *temperature;
    double displayedDelta = settings.temperatureUnit == TemperatureUnit::Fahrenheit
                                ? deltaCelsius * 9.0 / 5.0
                                : deltaCelsius;
    wchar_t value[64]{};
    swprintf(value, std::size(value), L"%.0f°%c %s warning",
             std::abs(displayedDelta),
             settings.temperatureUnit == TemperatureUnit::Fahrenheit ? L'F'
                                                                       : L'C',
             displayedDelta >= 0.0 ? L"below" : L"over");
    return value;
}

PCWSTR NotificationPostureName(FocusScene scene) {
    switch (scene) {
        case FocusScene::Focus:
        case FocusScene::Presentation:
            return L"Quiet visual posture";
        case FocusScene::Gaming:
            return L"Priority-only visual posture";
        case FocusScene::BatterySaver:
            return L"Reduced-motion posture";
        case FocusScene::Normal:
        default:
            return L"Normal visual posture";
    }
}

// ---------------------------------------------------------------------------
// Hardware Command Center
//
// One quiet material: a single dark glass card, grouped inset lists separated
// by hairlines, tabular numerals, and color reserved for state instead of
// decoration. Every surface reads the metrics snapshot the taskbar rows
// already render, so opening the panel adds no collection work. While the
// panel is open the same render pass keeps its values live.
// ---------------------------------------------------------------------------

constexpr double kCcCardWidth = 352.0;
constexpr double kCcContentWidth = 320.0;
constexpr double kCcTileWidth = 156.0;
constexpr double kCcTileBarWidth = 132.0;
constexpr double kCcActionWidth = 100.0;
constexpr double kCcGroupRadius = 8.0;
constexpr double kCcTileRadius = 8.0;
constexpr double kCcTraceWidth = 132.0;
constexpr double kCcTraceHeight = 18.0;
constexpr double kCcGroupSpacing = 8.0;

struct CommandCenterTile {
    TextBlock value{nullptr};
    TextBlock detail{nullptr};
    TextBlock badge{nullptr};
    XamlRectangle fill{nullptr};
};

struct CommandCenterTrace {
    XamlPolyline line{nullptr};
    XamlPolygon area{nullptr};
    TextBlock value{nullptr};
};

struct CommandCenterView {
    bool open = false;
    TextBlock subtitle{nullptr};
    TextBlock statusText{nullptr};
    XamlEllipse statusDot{nullptr};
    Border statusPill{nullptr};
    CommandCenterTile cpu{};
    CommandCenterTile gpu{};
    CommandCenterTile ram{};
    CommandCenterTile vram{};
    CommandCenterTrace cpuTrace{};
    CommandCenterTrace gpuTrace{};
    TextBlock thermalValue{nullptr};
    TextBlock gpuThermalValue{nullptr};
    TextBlock sensorValue{nullptr};
    TextBlock uptimeValue{nullptr};
    TextBlock postureValue{nullptr};
};

[[clang::no_destroy]] CommandCenterView g_commandCenter;
uint64_t g_commandCenterGeneration = 0;

void ResetCommandCenterView() {
    g_commandCenter = CommandCenterView{};
}

Color CcPrimaryColor(const ModSettings& settings) {
    return ParseColor(settings.textColor)
        .value_or(MakeColor(0xFF, 0xF5, 0xF5, 0xF7));
}

Color CcHealthColor(AlertLevel level, const ModSettings& settings) {
    if (level == AlertLevel::Critical) {
        return ParseColor(settings.criticalColor)
            .value_or(MakeColor(0xFF, 0xFF, 0x6B, 0x6B));
    }
    if (level == AlertLevel::Warning) {
        return ParseColor(settings.warningColor)
            .value_or(MakeColor(0xFF, 0xFF, 0xB9, 0x00));
    }
    return ParseColor(settings.safeColor)
        .value_or(MakeColor(0xFF, 0x8B, 0xD4, 0x9C));
}

Color CcValueColor(AlertLevel alert, const ModSettings& settings) {
    if (alert == AlertLevel::Normal) {
        return CcPrimaryColor(settings);
    }
    return AlertBrush(alert, settings).Color();
}

TextBlock CcText(const std::wstring& value,
                 double size,
                 Text::FontWeight weight,
                 double opacity,
                 const ModSettings& settings) {
    TextBlock text;
    text.Text(value);
    text.FontFamily(Media::FontFamily(settings.fontFamily));
    text.FontSize(size);
    text.FontWeight(weight);
    text.Opacity(opacity);
    text.Foreground(SolidColorBrush(CcPrimaryColor(settings)));
    text.TextWrapping(TextWrapping::NoWrap);
    text.TextTrimming(TextTrimming::CharacterEllipsis);
    text.VerticalAlignment(VerticalAlignment::Center);
    return text;
}

TextBlock CcNumeric(const std::wstring& value,
                    double size,
                    Text::FontWeight weight,
                    double opacity,
                    const ModSettings& settings) {
    TextBlock text = CcText(value, size, weight, opacity, settings);
    Typography::SetNumeralAlignment(text, FontNumeralAlignment::Tabular);
    return text;
}

TextBlock CcSectionLabel(const std::wstring& value,
                         const ModSettings& settings) {
    TextBlock text =
        CcText(value, 10.0, Text::FontWeights::SemiBold(), 0.55, settings);
    text.CharacterSpacing(45);
    text.Margin(Thickness{4, 4, 0, -2});
    return text;
}

XamlRectangle CcHairline(double leftInset) {
    XamlRectangle line;
    line.Height(1.0);
    line.Fill(SolidColorBrush(MakeColor(0x16, 0xFF, 0xFF, 0xFF)));
    line.Margin(Thickness{leftInset, 0, 0, 0});
    line.HorizontalAlignment(HorizontalAlignment::Stretch);
    return line;
}

Border CcGroupShell(UIElement child) {
    Border group;
    group.CornerRadius(CornerRadius{kCcGroupRadius, kCcGroupRadius,
                                    kCcGroupRadius, kCcGroupRadius});
    group.Background(SolidColorBrush(MakeColor(0x12, 0xFF, 0xFF, 0xFF)));
    group.Child(child);
    return group;
}

// A grouped inset list: hairline dividers between rows, rounded only at the
// outer edges so hover highlights never square off the group corners.
Border CcRowGroup(const std::vector<FrameworkElement>& rows,
                  double dividerInset) {
    StackPanel stack;
    for (size_t i = 0; i < rows.size(); ++i) {
        if (i > 0) {
            stack.Children().Append(CcHairline(dividerInset));
        }
        FrameworkElement row = rows[i];
        if (auto control = row.try_as<Control>()) {
            double top = (i == 0) ? kCcGroupRadius : 0.0;
            double bottom = (i + 1 == rows.size()) ? kCcGroupRadius : 0.0;
            control.CornerRadius(CornerRadius{top, top, bottom, bottom});
        }
        stack.Children().Append(row);
    }
    return CcGroupShell(stack);
}

// These remain real WinUI Buttons: keyboard focus, accessibility, touch hit
// targets, and native input routing are preserved. Only the heavy default
// fill is reduced so the rows sit naturally inside one native FlyoutPresenter.
void ApplyCcButtonSkin(Button button, const ModSettings& settings) {
    auto resources = button.Resources();
    Color transparent = MakeColor(0x00, 0xFF, 0xFF, 0xFF);
    Color primary = CcPrimaryColor(settings);
    auto assign = [&resources](PCWSTR key, Color color) {
        resources.Insert(box_value(winrt::hstring(key)),
                         SolidColorBrush(color));
    };
    assign(L"ButtonBackground", transparent);
    assign(L"ButtonBackgroundPointerOver", MakeColor(0x16, 0xFF, 0xFF, 0xFF));
    assign(L"ButtonBackgroundPressed", MakeColor(0x28, 0xFF, 0xFF, 0xFF));
    assign(L"ButtonBackgroundDisabled", transparent);
    assign(L"ButtonBorderBrush", transparent);
    assign(L"ButtonBorderBrushPointerOver", transparent);
    assign(L"ButtonBorderBrushPressed", transparent);
    assign(L"ButtonBorderBrushDisabled", transparent);
    assign(L"ButtonForeground", primary);
    assign(L"ButtonForegroundPointerOver", primary);
    assign(L"ButtonForegroundPressed", primary);
    button.Background(SolidColorBrush(transparent));
    button.BorderThickness(Thickness{0, 0, 0, 0});
    button.CornerRadius(CornerRadius{kCcGroupRadius, kCcGroupRadius,
                                     kCcGroupRadius, kCcGroupRadius});
    button.HorizontalAlignment(HorizontalAlignment::Stretch);
    button.HorizontalContentAlignment(HorizontalAlignment::Stretch);
    button.VerticalContentAlignment(VerticalAlignment::Center);
    button.UseSystemFocusVisuals(true);
}

Border CcGlyphChip(PCWSTR glyph,
                   double size,
                   const ModSettings& settings) {
    Border chip;
    chip.Width(size);
    chip.Height(size);
    chip.CornerRadius(CornerRadius{6, 6, 6, 6});
    chip.Background(SolidColorBrush(MakeColor(0x16, 0xFF, 0xFF, 0xFF)));
    chip.VerticalAlignment(VerticalAlignment::Center);
    TextBlock icon;
    icon.Text(glyph);
    icon.FontFamily(Media::FontFamily(L"Segoe Fluent Icons"));
    icon.FontSize(size * 0.55);
    icon.Foreground(SolidColorBrush(CcPrimaryColor(settings)));
    icon.HorizontalAlignment(HorizontalAlignment::Center);
    icon.VerticalAlignment(VerticalAlignment::Center);
    chip.Child(icon);
    return chip;
}

void CcAttachTooltip(FrameworkElement element, const std::wstring& text) {
    ToolTip tip;
    tip.Content(box_value(text));
    ToolTipService::SetToolTip(element, tip);
}

// Label left, value right. Two real columns, so a long value can never
// overrun the label; whichever side runs out of room ellipsizes instead.
Grid CcDetailRow(const std::wstring& label,
                 const std::wstring& value,
                 TextBlock& valueOut,
                 const ModSettings& settings) {
    Grid row;
    row.Height(31.0);
    row.Padding(Thickness{15, 0, 15, 0});
    ColumnDefinition labelColumn;
    labelColumn.Width(
        GridLengthHelper::FromValueAndType(0.0, GridUnitType::Auto));
    ColumnDefinition valueColumn;
    valueColumn.Width(
        GridLengthHelper::FromValueAndType(1.0, GridUnitType::Star));
    row.ColumnDefinitions().Append(labelColumn);
    row.ColumnDefinitions().Append(valueColumn);

    TextBlock caption =
        CcText(label, 12.0, Text::FontWeights::Normal(), 0.52, settings);
    caption.HorizontalAlignment(HorizontalAlignment::Left);
    caption.MaxWidth(148.0);
    Grid::SetColumn(caption, 0);
    row.Children().Append(caption);

    TextBlock detail =
        CcNumeric(value, 12.0, Text::FontWeights::Normal(), 0.92, settings);
    detail.HorizontalAlignment(HorizontalAlignment::Right);
    detail.TextAlignment(TextAlignment::Right);
    detail.Margin(Thickness{10, 0, 0, 0});
    Grid::SetColumn(detail, 1);
    row.Children().Append(detail);

    valueOut = detail;
    return row;
}

// CPU and GPU tiles carry their own sparkline; RAM and VRAM carry a capacity
// line and rail. Both variants are the same height so the pair reads as a row.
Border CcMetricTile(const std::wstring& label,
                    CommandCenterTile& tile,
                    CommandCenterTrace* trace,
                    const ModSettings& settings) {
    Border card;
    card.Width(kCcTileWidth);
    card.Height(82.0);
    card.CornerRadius(CornerRadius{kCcTileRadius, kCcTileRadius, kCcTileRadius,
                                   kCcTileRadius});
    card.Background(SolidColorBrush(MakeColor(0x12, 0xFF, 0xFF, 0xFF)));
    card.Padding(Thickness{12, 8, 12, 9});

    StackPanel content;

    Grid header;
    TextBlock name =
        CcText(label, 10.0, Text::FontWeights::SemiBold(), 0.45, settings);
    name.CharacterSpacing(90);
    name.HorizontalAlignment(HorizontalAlignment::Left);
    header.Children().Append(name);
    TextBlock badge =
        CcNumeric(L"", 11.0, Text::FontWeights::SemiBold(), 0.95, settings);
    badge.HorizontalAlignment(HorizontalAlignment::Right);
    header.Children().Append(badge);
    content.Children().Append(header);

    TextBlock value =
        CcNumeric(L"--", 20.0, Text::FontWeights::SemiBold(), 1.0, settings);
    value.Margin(Thickness{0, 1, 0, 0});
    content.Children().Append(value);

    TextBlock detail =
        CcNumeric(L"", 11.0, Text::FontWeights::Normal(), 0.5, settings);
    if (!trace) {
        content.Children().Append(detail);
    }

    if (trace) {
        Grid plot;
        plot.Width(kCcTraceWidth);
        plot.Height(kCcTraceHeight);
        plot.Margin(Thickness{0, 4, 0, 0});
        plot.HorizontalAlignment(HorizontalAlignment::Left);
        XamlPolygon area;
        area.Width(kCcTraceWidth);
        area.Height(kCcTraceHeight);
        area.Stretch(Stretch::None);
        plot.Children().Append(area);
        XamlPolyline line;
        line.Width(kCcTraceWidth);
        line.Height(kCcTraceHeight);
        line.Stretch(Stretch::None);
        line.StrokeThickness(1.5);
        line.StrokeStartLineCap(PenLineCap::Round);
        line.StrokeEndLineCap(PenLineCap::Round);
        line.StrokeLineJoin(PenLineJoin::Round);
        plot.Children().Append(line);
        content.Children().Append(plot);
        trace->line = line;
        trace->area = area;
        trace->value = nullptr;
    }

    Grid track;
    track.Width(kCcTileBarWidth);
    track.Height(3.0);
    track.Margin(Thickness{0, 7, 0, 0});
    track.HorizontalAlignment(HorizontalAlignment::Left);
    XamlRectangle groove;
    groove.Width(kCcTileBarWidth);
    groove.Height(3.0);
    groove.RadiusX(1.5);
    groove.RadiusY(1.5);
    groove.Fill(SolidColorBrush(MakeColor(0x1E, 0xFF, 0xFF, 0xFF)));
    track.Children().Append(groove);
    XamlRectangle fill;
    fill.Height(3.0);
    fill.Width(0.0);
    fill.RadiusX(1.5);
    fill.RadiusY(1.5);
    fill.HorizontalAlignment(HorizontalAlignment::Left);
    track.Children().Append(fill);
    if (!trace) {
        content.Children().Append(track);
    }

    card.Child(content);

    tile.value = value;
    tile.detail = detail;
    tile.badge = badge;
    tile.fill = fill;
    return card;
}

void CcUpdateTile(CommandCenterTile& tile,
                  const std::wstring& value,
                  const std::wstring& detail,
                  const std::wstring& badge,
                  AlertLevel badgeAlert,
                  double fraction,
                  bool available,
                  AlertLevel alert,
                  const ModSettings& settings) {
    if (!tile.value) {
        return;
    }
    SetTextIfChanged(tile.value, value);
    tile.value.Foreground(SolidColorBrush(CcValueColor(alert, settings)));
    if (tile.detail) {
        SetTextIfChanged(tile.detail, detail);
    }
    if (tile.badge) {
        SetTextIfChanged(tile.badge, badge);
        SetVisibilityIfChanged(tile.badge, badge.empty() ? Visibility::Collapsed
                                                       : Visibility::Visible);
        tile.badge.Foreground(
            SolidColorBrush(badgeAlert == AlertLevel::Normal
                                ? CcHealthColor(AlertLevel::Normal, settings)
                                : AlertBrush(badgeAlert, settings).Color()));
    }
    if (tile.fill) {
        double clamped = available ? std::clamp(fraction, 0.0, 1.0) : 0.0;
        tile.fill.Width(kCcTileBarWidth * clamped);
        tile.fill.Fill(AlertBrush(alert, settings));
    }
}

void CcUpdateTrace(CommandCenterTrace& trace,
                   const std::deque<double>& history,
                   double current,
                   bool available,
                   AlertLevel alert,
                   const ModSettings& settings) {
    if (!trace.line || !trace.area) {
        return;
    }
    SolidColorBrush stroke = AlertBrush(alert, settings);
    Color accent = stroke.Color();
    trace.line.Stroke(stroke);

    LinearGradientBrush gradient;
    gradient.StartPoint(Point{0.0F, 0.0F});
    gradient.EndPoint(Point{0.0F, 1.0F});
    GradientStop upper;
    upper.Color(MakeColor(0x4A, accent.R, accent.G, accent.B));
    upper.Offset(0.0);
    GradientStop lower;
    lower.Color(MakeColor(0x00, accent.R, accent.G, accent.B));
    lower.Offset(1.0);
    gradient.GradientStops().Append(upper);
    gradient.GradientStops().Append(lower);
    trace.area.Fill(gradient);

    auto linePoints = trace.line.Points();
    auto areaPoints = trace.area.Points();
    linePoints.Clear();
    areaPoints.Clear();
    if (history.size() >= 2) {
        double step = kCcTraceWidth / static_cast<double>(history.size() - 1);
        for (size_t i = 0; i < history.size(); ++i) {
            double x = step * static_cast<double>(i);
            double y = 1.0 +
                       (100.0 - std::clamp(history[i], 0.0, 100.0)) / 100.0 *
                           (kCcTraceHeight - 2.0);
            Point point{static_cast<float>(x), static_cast<float>(y)};
            linePoints.Append(point);
            areaPoints.Append(point);
        }
        areaPoints.Append(Point{static_cast<float>(kCcTraceWidth),
                                static_cast<float>(kCcTraceHeight)});
        areaPoints.Append(Point{0.0F, static_cast<float>(kCcTraceHeight)});
    }

    if (trace.value) {
        SetTextIfChanged(trace.value,
                         available ? FormatPercent(current) : L"--%");
        trace.value.Foreground(
            SolidColorBrush(CcValueColor(alert, settings)));
    }
}

std::wstring CcAverageLabel(const std::deque<double>& history,
                            bool available) {
    if (!available || history.empty()) {
        return L"avg --%";
    }
    double total = 0.0;
    for (double sample : history) {
        total += sample;
    }
    return L"avg " + FormatPercent(total / static_cast<double>(history.size()));
}

std::wstring CommandCenterSubtitle(const MetricsSnapshot& snapshot,
                                   const ModSettings& settings) {
    return std::wstring(FocusSceneName(snapshot.scene)) + L"  ·  " +
           ExperienceModeName(g_experienceMode.load()) + L"  ·  " +
           std::to_wstring(EffectiveUpdateInterval(settings, snapshot.scene)) +
           L"s";
}

// Full provider names belong in the log; the panel needs a token that fits.
PCWSTR CcProviderShortName(TemperatureProvider provider) {
    switch (provider) {
        case TemperatureProvider::HwInfoSharedMemory:
            return L"HWiNFO SM";
        case TemperatureProvider::HwInfoGadgetRegistry:
            return L"HWiNFO GR";
        case TemperatureProvider::WindowsD3dkmt:
            return L"D3DKMT";
        case TemperatureProvider::WindowsThermalZones:
            return L"Thermal zones";
        case TemperatureProvider::None:
        default:
            return L"None";
    }
}

std::wstring CommandCenterThermalSummary(const MetricsSnapshot& snapshot,
                                         const ModSettings& settings) {
    return L"CPU " +
           FormatThermalHeadroom(snapshot.cpuTemp, settings.cpuWarningTemp,
                                 settings) +
           L"  ·  GPU " +
           FormatThermalHeadroom(snapshot.gpuTemp, settings.gpuWarningTemp,
                                 settings);
}

std::wstring CommandCenterSensorSummary(const MetricsSnapshot& snapshot) {
    return std::wstring(CcProviderShortName(snapshot.cpuTempProvider)) +
           L"  ·  " + CcProviderShortName(snapshot.gpuTempProvider);
}

std::wstring CommandCenterSensorDetail(const MetricsSnapshot& snapshot) {
    return std::wstring(L"CPU  ") +
           TemperatureProviderName(snapshot.cpuTempProvider) + L"\nGPU  " +
           TemperatureProviderName(snapshot.gpuTempProvider);
}

std::wstring CommandCenterUptimeSummary(const ProcessMemoryReport& report) {
    return FormatUptime() + L"  ·  " + std::to_wstring(report.processCount) +
           L" processes";
}

std::wstring CommandCenterDiagnostics(const MetricsSnapshot& snapshot,
                                      const ProcessMemoryReport& report,
                                      const ModSettings& settings) {
    std::wstring text = L"Maxwell hardware snapshot\r\n";
    text += L"Health      " + std::wstring(SystemHealthName(OverallAlert())) +
            L"\r\n";
    text += L"Scene       " + CommandCenterSubtitle(snapshot, settings) +
            L"\r\n";
    text += L"CPU         " + FormatLoadPercent(snapshot.cpu) + L"  " +
            FormatTemperature(snapshot.cpuTemp, settings) + L"\r\n";
    text += L"GPU         " +
            FormatLoadPercent(snapshot.gpu, snapshot.gpuAvailable) + L"  " +
            FormatTemperature(snapshot.gpuTemp, settings) + L"\r\n";
    text += L"RAM         " + FormatUsedPercent(snapshot.ram) + L"  " +
            FormatCapacity(snapshot.ramUsedGb, snapshot.ramTotalGb, true) +
            L"\r\n";
    text += L"VRAM        " +
            FormatUsedPercent(snapshot.vram, snapshot.vramAvailable) + L"  " +
            FormatCapacity(snapshot.vramUsedGb, snapshot.vramTotalGb,
                           snapshot.vramAvailable) +
            L"\r\n";
    text += L"Graphics    " + ActiveGpuAdapterName() + L"\r\n";
    text += L"Sensors     " + CommandCenterSensorSummary(snapshot) + L"\r\n";
    text += L"Thermal     " + CommandCenterThermalSummary(snapshot, settings) +
            L"\r\n";
    text += L"Uptime      " + CommandCenterUptimeSummary(report) + L"\r\n";
    for (const auto& entry : report.top) {
        text += L"Memory      " + entry.name + L"  " +
                FormatMemoryBytes(entry.bytes) + L"\r\n";
    }
    return text;
}

void CloseCommandCenter() {
    if (!g_commandCenterFlyout) {
        return;
    }
    try {
        g_commandCenterFlyout.Hide();
    } catch (...) {
    }
    g_commandCenterFlyout = nullptr;
    ResetCommandCenterView();
}

Button CcActionTile(PCWSTR glyph,
                    const std::wstring& label,
                    const ModSettings& settings) {
    Button button;
    ApplyCcButtonSkin(button, settings);
    button.Width(kCcActionWidth);
    button.Height(64.0);
    button.CornerRadius(CornerRadius{kCcTileRadius, kCcTileRadius,
                                     kCcTileRadius, kCcTileRadius});
    button.Padding(Thickness{6, 8, 6, 8});
    button.HorizontalAlignment(HorizontalAlignment::Left);
    button.HorizontalContentAlignment(HorizontalAlignment::Center);
    button.Background(SolidColorBrush(MakeColor(0x12, 0xFF, 0xFF, 0xFF)));
    button.Resources().Insert(
        box_value(winrt::hstring(L"ButtonBackground")),
        SolidColorBrush(MakeColor(0x12, 0xFF, 0xFF, 0xFF)));
    button.Resources().Insert(
        box_value(winrt::hstring(L"ButtonBackgroundPointerOver")),
        SolidColorBrush(MakeColor(0x24, 0xFF, 0xFF, 0xFF)));
    button.Resources().Insert(
        box_value(winrt::hstring(L"ButtonBackgroundPressed")),
        SolidColorBrush(MakeColor(0x34, 0xFF, 0xFF, 0xFF)));

    StackPanel content;
    content.Spacing(5);
    content.HorizontalAlignment(HorizontalAlignment::Center);
    Border chip = CcGlyphChip(glyph, 24.0, settings);
    chip.HorizontalAlignment(HorizontalAlignment::Center);
    content.Children().Append(chip);
    TextBlock caption =
        CcText(label, 10.5, Text::FontWeights::Normal(), 0.78, settings);
    caption.TextWrapping(TextWrapping::Wrap);
    caption.TextTrimming(TextTrimming::None);
    caption.TextAlignment(TextAlignment::Center);
    caption.HorizontalAlignment(HorizontalAlignment::Center);
    caption.LineHeight(12.0);
    content.Children().Append(caption);
    button.Content(content);
    winrt::Windows::UI::Xaml::Automation::AutomationProperties::SetName(
        button, label);

    ToolTip tip;
    tip.Content(box_value(label));
    ToolTipService::SetToolTip(button, tip);
    return button;
}

Button CcLaunchTile(PCWSTR glyph,
                    const std::wstring& label,
                    PCWSTR target,
                    const ModSettings& settings) {
    Button button = CcActionTile(glyph, label, settings);
    std::wstring targetCopy(target);
    std::wstring helpText =
        L"Open the native Windows " + label + L" surface";
    winrt::Windows::UI::Xaml::Automation::AutomationProperties::SetHelpText(
        button, winrt::hstring(helpText));
    button.Click([targetCopy](IInspectable const&, RoutedEventArgs const&) {
        LaunchShellTarget(targetCopy.c_str());
        CloseCommandCenter();
    });
    return button;
}

StackPanel CcActionShelf(const std::vector<Button>& tiles) {
    StackPanel shelf;
    shelf.Orientation(Orientation::Horizontal);
    shelf.Spacing(kCcGroupSpacing);
    for (const auto& tile : tiles) {
        shelf.Children().Append(tile);
    }
    return shelf;
}

int ExperienceModeIndex(ExperienceMode mode) {
    switch (mode) {
        case ExperienceMode::Balanced: return 1;
        case ExperienceMode::Focus: return 2;
        case ExperienceMode::Gaming: return 3;
        case ExperienceMode::Minimal: return 4;
        case ExperienceMode::Auto:
        default: return 0;
    }
}

ExperienceMode ExperienceModeFromIndex(int index) {
    switch (index) {
        case 1: return ExperienceMode::Balanced;
        case 2: return ExperienceMode::Focus;
        case 3: return ExperienceMode::Gaming;
        case 4: return ExperienceMode::Minimal;
        case 0:
        default: return ExperienceMode::Auto;
    }
}

Grid CcModeSelector(const ModSettings& settings) {
    Grid row;
    row.Height(46.0);
    row.Padding(Thickness{14, 0, 12, 0});
    ColumnDefinition labelColumn;
    labelColumn.Width(GridLength{1, GridUnitType::Star});
    ColumnDefinition selectorColumn;
    selectorColumn.Width(
        GridLengthHelper::FromValueAndType(0.0, GridUnitType::Auto));
    row.ColumnDefinitions().Append(labelColumn);
    row.ColumnDefinitions().Append(selectorColumn);

    StackPanel left;
    left.Orientation(Orientation::Horizontal);
    left.Spacing(11);
    left.HorizontalAlignment(HorizontalAlignment::Left);
    left.VerticalAlignment(VerticalAlignment::Center);
    left.Children().Append(CcGlyphChip(L"\uE713", 22.0, settings));
    left.Children().Append(CcText(L"Taskbar mode", 12.5,
                                  Text::FontWeights::Normal(), 0.92,
                                  settings));
    Grid::SetColumn(left, 0);
    row.Children().Append(left);

    ComboBox selector;
    selector.Width(116.0);
    selector.Height(32.0);
    selector.Items().Append(box_value(L"Automatic"));
    selector.Items().Append(box_value(L"Balanced"));
    selector.Items().Append(box_value(L"Focus"));
    selector.Items().Append(box_value(L"Gaming"));
    selector.Items().Append(box_value(L"Minimal"));
    selector.SelectedIndex(ExperienceModeIndex(g_experienceMode.load()));
    selector.HorizontalAlignment(HorizontalAlignment::Right);
    winrt::Windows::UI::Xaml::Automation::AutomationProperties::SetName(
        selector, L"Taskbar performance mode");
    winrt::Windows::UI::Xaml::Automation::AutomationProperties::SetHelpText(
        selector, L"Choose how often hardware updates and how much detail the taskbar shows");
    selector.SelectionChanged(
        [](IInspectable const& sender, SelectionChangedEventArgs const&) {
            auto combo = sender.try_as<ComboBox>();
            if (combo && combo.SelectedIndex() >= 0) {
                SetExperienceMode(
                    ExperienceModeFromIndex(combo.SelectedIndex()));
            }
        });
    Grid::SetColumn(selector, 1);
    row.Children().Append(selector);
    return row;
}

Button CcCopyRow(const MetricsSnapshot& snapshot,
                 const ProcessMemoryReport& report,
                 const ModSettings& settings) {
    Button button;
    ApplyCcButtonSkin(button, settings);
    button.Height(42.0);
    button.Padding(Thickness{14, 0, 15, 0});

    Grid content;
    StackPanel left;
    left.Orientation(Orientation::Horizontal);
    left.Spacing(11);
    left.HorizontalAlignment(HorizontalAlignment::Left);
    left.VerticalAlignment(VerticalAlignment::Center);
    left.Children().Append(CcGlyphChip(L"\uE8C8", 22.0, settings));
    left.Children().Append(CcText(L"Copy hardware report", 12.5,
                                  Text::FontWeights::Normal(), 0.92,
                                  settings));
    content.Children().Append(left);

    TextBlock value =
        CcText(L"", 12.0, Text::FontWeights::Normal(), 0.5, settings);
    value.HorizontalAlignment(HorizontalAlignment::Right);
    content.Children().Append(value);
    button.Content(content);
    winrt::Windows::UI::Xaml::Automation::AutomationProperties::SetName(
        button, L"Copy hardware report");

    std::wstring payload =
        CommandCenterDiagnostics(snapshot, report, settings);
    button.Click([payload, value](IInspectable const&,
                                  RoutedEventArgs const&) {
        bool copied = CopyTextToClipboard(payload + OpalPerformanceDiagnostics::Summary());
        try {
            value.Text(copied ? L"Copied" : L"Unavailable");
        } catch (...) {
        }
    });
    return button;
}

double CommandCenterMaxHeight(FrameworkElement anchor) {
    double scale = 1.0;
    try {
        double rasterization = anchor.XamlRoot().RasterizationScale();
        if (rasterization > 0.1) {
            scale = rasterization;
        }
    } catch (...) {
    }
    HWND window = g_taskbarWindow.load();
    HMONITOR monitor =
        window ? MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST) : nullptr;
    MONITORINFO info{};
    info.cbSize = sizeof(info);
    if (monitor && GetMonitorInfoW(monitor, &info)) {
        double workHeight =
            static_cast<double>(info.rcWork.bottom - info.rcWork.top) / scale;
        return std::clamp(workHeight - 40.0, 360.0, 1000.0);
    }
    return 720.0;
}

void RefreshCommandCenter(const MetricsSnapshot& snapshot,
                          const ModSettings& settings) {
    if (!g_commandCenter.open) {
        return;
    }
    OpalPerformanceDiagnostics::Scope refresh(
        OpalPerformanceDiagnostics::Metric::HardwareRefresh);
    try {
        if (g_commandCenter.subtitle) {
            SetTextIfChanged(g_commandCenter.subtitle,
                CommandCenterSubtitle(snapshot, settings));
        }
        AlertLevel health = OverallAlert();
        Color healthColor = CcHealthColor(health, settings);
        if (g_commandCenter.statusText) {
            SetTextIfChanged(g_commandCenter.statusText,
                             SystemHealthName(health));
            g_commandCenter.statusText.Foreground(
                SolidColorBrush(healthColor));
        }
        if (g_commandCenter.statusDot) {
            g_commandCenter.statusDot.Fill(SolidColorBrush(healthColor));
        }
        if (g_commandCenter.statusPill) {
            g_commandCenter.statusPill.Background(SolidColorBrush(MakeColor(
                0x22, healthColor.R, healthColor.G, healthColor.B)));
        }

        CcUpdateTile(g_commandCenter.cpu, FormatPercent(snapshot.cpu),
                     L"",
                     FormatTemperature(snapshot.cpuTemp, settings),
                     g_cpuTemperatureAlert, snapshot.cpu / 100.0, true,
                     g_cpuUsageAlert, settings);
        CcUpdateTile(g_commandCenter.gpu,
                     snapshot.gpuAvailable ? FormatPercent(snapshot.gpu)
                                           : L"--%",
                     L"",
                     FormatTemperature(snapshot.gpuTemp, settings),
                     g_gpuTemperatureAlert, snapshot.gpu / 100.0,
                     snapshot.gpuAvailable, g_gpuUsageAlert, settings);
        CcUpdateTile(g_commandCenter.ram, FormatPercent(snapshot.ram),
                     FormatCapacity(snapshot.ramUsedGb, snapshot.ramTotalGb,
                                    true),
                     L"", AlertLevel::Normal, snapshot.ram / 100.0, true,
                     g_ramAlert, settings);
        CcUpdateTile(g_commandCenter.vram,
                     snapshot.vramAvailable ? FormatPercent(snapshot.vram)
                                            : L"--%",
                     FormatCapacity(snapshot.vramUsedGb, snapshot.vramTotalGb,
                                    snapshot.vramAvailable),
                     L"", AlertLevel::Normal, snapshot.vram / 100.0,
                     snapshot.vramAvailable, g_vramAlert, settings);

        CcUpdateTrace(g_commandCenter.cpuTrace, g_cpuHistory, snapshot.cpu,
                      true, g_cpuUsageAlert, settings);
        CcUpdateTrace(g_commandCenter.gpuTrace, g_gpuHistory, snapshot.gpu,
                      snapshot.gpuAvailable, g_gpuUsageAlert, settings);

        if (g_commandCenter.thermalValue) {
            SetTextIfChanged(g_commandCenter.thermalValue, FormatThermalHeadroom(
                snapshot.cpuTemp, settings.cpuWarningTemp, settings));
            g_commandCenter.thermalValue.Foreground(SolidColorBrush(
                CcValueColor(g_cpuTemperatureAlert, settings)));
        }
        if (g_commandCenter.gpuThermalValue) {
            SetTextIfChanged(g_commandCenter.gpuThermalValue, FormatThermalHeadroom(
                snapshot.gpuTemp, settings.gpuWarningTemp, settings));
            g_commandCenter.gpuThermalValue.Foreground(SolidColorBrush(
                CcValueColor(g_gpuTemperatureAlert, settings)));
        }
        if (g_commandCenter.sensorValue) {
            SetTextIfChanged(g_commandCenter.sensorValue,
                CommandCenterSensorSummary(snapshot));
        }
        if (g_commandCenter.postureValue) {
            SetTextIfChanged(g_commandCenter.postureValue,
                NotificationPostureName(snapshot.scene));
        }
    } catch (...) {
        Wh_Log(L"Hardware Command Center refresh failed");
    }
}

void ShowHardwareCommandCenter(FrameworkElement anchor) {
    if (!anchor) {
        return;
    }
    ModSettings settings = CurrentSettings();
    if (!settings.commandCenterEnabled) {
        LaunchMonitoringTool(false);
        return;
    }
    MetricsSnapshot snapshot;
    uint64_t sequence = 0;
    if (!GetLatestMetrics(snapshot, sequence)) {
        return;
    }

    if (g_commandCenterFlyout) {
        try {
            g_commandCenterFlyout.Hide();
        } catch (...) {
        }
    }
    ResetCommandCenterView();
    uint64_t generation = ++g_commandCenterGeneration;
    const auto diagnosticOpen = OpalPerformanceDiagnostics::Begin();

    ProcessMemoryReport memoryReport = CollectProcessMemory(5);
    AlertLevel health = OverallAlert();
    Color healthColor = CcHealthColor(health, settings);

    StackPanel panel;
    panel.Width(kCcContentWidth);
    panel.Spacing(kCcGroupSpacing);

    // Header: native Windows hierarchy, live scene line, and one state capsule.
    Grid header;
    header.Margin(Thickness{2, 0, 2, 2});
    StackPanel identity;
    identity.HorizontalAlignment(HorizontalAlignment::Left);
    identity.VerticalAlignment(VerticalAlignment::Center);
    TextBlock title =
        CcText(L"Hardware", 20.0, Text::FontWeights::SemiBold(), 1.0, settings);
    title.CharacterSpacing(-10);
    identity.Children().Append(title);
    TextBlock subtitle = CcText(CommandCenterSubtitle(snapshot, settings),
                                11.0, Text::FontWeights::Normal(), 0.5,
                                settings);
    subtitle.Margin(Thickness{0, 1, 0, 0});
    identity.Children().Append(subtitle);
    header.Children().Append(identity);

    Border pill;
    pill.CornerRadius(CornerRadius{4, 4, 4, 4});
    pill.Height(21.0);
    pill.Padding(Thickness{9, 0, 10, 0});
    pill.HorizontalAlignment(HorizontalAlignment::Right);
    pill.VerticalAlignment(VerticalAlignment::Center);
    pill.Background(SolidColorBrush(
        MakeColor(0x22, healthColor.R, healthColor.G, healthColor.B)));
    StackPanel pillContent;
    pillContent.Orientation(Orientation::Horizontal);
    pillContent.Spacing(6);
    pillContent.VerticalAlignment(VerticalAlignment::Center);
    XamlEllipse dot;
    dot.Width(6.0);
    dot.Height(6.0);
    dot.VerticalAlignment(VerticalAlignment::Center);
    dot.Fill(SolidColorBrush(healthColor));
    pillContent.Children().Append(dot);
    TextBlock healthText = CcText(SystemHealthName(health), 11.0,
                                  Text::FontWeights::SemiBold(), 1.0,
                                  settings);
    healthText.Foreground(SolidColorBrush(healthColor));
    pillContent.Children().Append(healthText);
    pill.Child(pillContent);
    header.Children().Append(pill);
    panel.Children().Append(header);

    panel.Children().Append(CcSectionLabel(L"LIVE PERFORMANCE", settings));

    // Four metric tiles: the value you need at a glance, then the supporting
    // detail. They are native XAML elements and reuse the taskbar snapshot.
    StackPanel topTiles;
    topTiles.Orientation(Orientation::Horizontal);
    topTiles.Spacing(kCcGroupSpacing);
    topTiles.Children().Append(CcMetricTile(
        L"CPU", g_commandCenter.cpu, &g_commandCenter.cpuTrace, settings));
    topTiles.Children().Append(CcMetricTile(
        L"GPU", g_commandCenter.gpu, &g_commandCenter.gpuTrace, settings));
    panel.Children().Append(topTiles);

    StackPanel bottomTiles;
    bottomTiles.Orientation(Orientation::Horizontal);
    bottomTiles.Spacing(kCcGroupSpacing);
    bottomTiles.Children().Append(
        CcMetricTile(L"RAM", g_commandCenter.ram, nullptr, settings));
    bottomTiles.Children().Append(
        CcMetricTile(L"VRAM", g_commandCenter.vram, nullptr, settings));
    panel.Children().Append(bottomTiles);

    // Context that does not fit on the taskbar itself.
    panel.Children().Append(CcSectionLabel(L"SYSTEM DETAILS", settings));
    std::vector<FrameworkElement> detailRows;
    TextBlock thermalValue{nullptr};
    TextBlock gpuThermalValue{nullptr};
    TextBlock sensorValue{nullptr};
    TextBlock adapterValue{nullptr};
    TextBlock uptimeValue{nullptr};
    TextBlock postureValue{nullptr};
    detailRows.push_back(CcDetailRow(
        L"CPU headroom",
        FormatThermalHeadroom(snapshot.cpuTemp, settings.cpuWarningTemp,
                              settings),
        thermalValue, settings));
    detailRows.push_back(CcDetailRow(
        L"GPU headroom",
        FormatThermalHeadroom(snapshot.gpuTemp, settings.gpuWarningTemp,
                              settings),
        gpuThermalValue, settings));
    std::wstring adapterName = ActiveGpuAdapterName();
    Grid adapterRow =
        CcDetailRow(L"Graphics", adapterName, adapterValue, settings);
    CcAttachTooltip(adapterRow, adapterName);
    detailRows.push_back(adapterRow);
    Grid sensorRow = CcDetailRow(
        L"Sensors", CommandCenterSensorSummary(snapshot), sensorValue,
        settings);
    CcAttachTooltip(sensorRow, CommandCenterSensorDetail(snapshot));
    detailRows.push_back(sensorRow);
    detailRows.push_back(CcDetailRow(
        L"Visual posture", NotificationPostureName(snapshot.scene),
        postureValue, settings));
    detailRows.push_back(CcDetailRow(L"Uptime",
                                     CommandCenterUptimeSummary(memoryReport),
                                     uptimeValue, settings));
    panel.Children().Append(CcRowGroup(detailRows, 15.0));
    g_commandCenter.thermalValue = thermalValue;
    g_commandCenter.gpuThermalValue = gpuThermalValue;
    g_commandCenter.sensorValue = sensorValue;
    g_commandCenter.uptimeValue = uptimeValue;
    g_commandCenter.postureValue = postureValue;

    // What is actually holding the memory, ranked.
    if (!memoryReport.top.empty()) {
        panel.Children().Append(CcSectionLabel(L"TOP MEMORY", settings));
        std::vector<FrameworkElement> memoryRows;
        for (const auto& entry : memoryReport.top) {
            TextBlock unusedValue{nullptr};
            Grid memoryRow = CcDetailRow(
                entry.name, FormatMemoryBytes(entry.bytes), unusedValue,
                settings);
            CcAttachTooltip(memoryRow, entry.name);
            memoryRows.push_back(memoryRow);
        }
        panel.Children().Append(CcRowGroup(memoryRows, 15.0));
    }

    // Native Windows destinations available without installing another tool.
    panel.Children().Append(CcSectionLabel(L"SYSTEM TOOLS", settings));
    StackPanel actions;
    actions.Spacing(kCcGroupSpacing);
    actions.Children().Append(CcActionShelf(
        {CcLaunchTile(L"\uE9D9", L"Task Manager", L"taskmgr.exe", settings),
         CcLaunchTile(L"\uE9D2", L"Resource Monitor", L"resmon.exe", settings),
         CcLaunchTile(L"\uE850", L"Power & battery",
                      L"ms-settings:powersleep", settings)}));
    actions.Children().Append(CcActionShelf(
        {CcLaunchTile(L"\uE7F4", L"Graphics",
                      L"ms-settings:display-advancedgraphics", settings),
         CcLaunchTile(L"\uE770", L"System info", L"ms-settings:about",
                      settings),
         CcLaunchTile(L"\uE8B7", L"Storage", L"ms-settings:storagesense",
                      settings)}));
    panel.Children().Append(actions);

    panel.Children().Append(CcSectionLabel(L"OPTIONS", settings));
    std::vector<FrameworkElement> controlRows;
    controlRows.push_back(CcModeSelector(settings));
    controlRows.push_back(CcCopyRow(snapshot, memoryReport, settings));
    panel.Children().Append(CcRowGroup(controlRows, 47.0));

    ScrollViewer scroller;
    scroller.Content(panel);
    scroller.HorizontalScrollMode(ScrollMode::Disabled);
    scroller.HorizontalScrollBarVisibility(ScrollBarVisibility::Disabled);
    scroller.VerticalScrollBarVisibility(ScrollBarVisibility::Auto);
    scroller.MaxHeight(CommandCenterMaxHeight(anchor) - 34.0);

    // The FlyoutPresenter owns the material, shadow, corners, and motion.
    // This transparent content card contributes only layout, keeping the
    // command center visually and operationally native to Windows.
    Border card;
    card.Width(kCcCardWidth);
    card.Padding(Thickness{12, 10, 12, 12});
    card.Background(SolidColorBrush(Colors::Transparent()));
    card.BorderThickness(Thickness{0, 0, 0, 0});
    card.Child(scroller);

    g_commandCenter.subtitle = subtitle;
    g_commandCenter.statusText = healthText;
    g_commandCenter.statusDot = dot;
    g_commandCenter.statusPill = pill;

    Flyout flyout;
    flyout.Content(card);
    flyout.ShouldConstrainToRootBounds(false);
    flyout.Placement(FlyoutPlacementMode::Top);

    // Use Windows' own flyout transition and reduced-motion behavior.
    flyout.Opened([generation, diagnosticOpen](IInspectable const&, IInspectable const&) {
        if (generation != g_commandCenterGeneration) {
            return;
        }
        g_commandCenter.open = true;
        OpalPerformanceDiagnostics::Record(
            OpalPerformanceDiagnostics::Metric::HardwareOpen, diagnosticOpen);
    });
    flyout.Closed([generation](IInspectable const&, IInspectable const&) {
        if (generation == g_commandCenterGeneration) {
            ResetCommandCenterView();
            g_commandCenterFlyout = nullptr;
        }
    });

    FrameworkElement placementTarget = anchor;
    Point placementPoint{};
    bool hasRootPlacement = false;
    try {
        auto xamlRoot = anchor.XamlRoot();
        auto rootContent = xamlRoot.Content().try_as<FrameworkElement>();
        if (rootContent) {
            auto transform = anchor.TransformToVisual(rootContent);
            Point topLeft = transform.TransformPoint(Point{0.0F, 0.0F});
            placementPoint = Point{
                topLeft.X + static_cast<float>(anchor.ActualWidth() * 0.5),
                topLeft.Y};
            placementTarget = rootContent;
            hasRootPlacement = true;
        }
    } catch (...) {
        Wh_Log(L"Hardware Command Center root placement unavailable");
    }

    g_commandCenterFlyout = flyout;
    g_commandCenter.open = true;
    RefreshCommandCenter(snapshot, settings);
    if (hasRootPlacement) {
        FlyoutShowOptions options;
        options.Placement(FlyoutPlacementMode::Top);
        options.Position(placementPoint);
        flyout.ShowAt(placementTarget, options);
    } else {
        flyout.ShowAt(anchor);
    }
}

void ApplyUnifiedHoverLanguage(Grid row) {
    if (!row) {
        return;
    }
    auto resting = SolidColorBrush(MakeColor(0x01, 0x00, 0x00, 0x00));
    auto hover = SolidColorBrush(MakeColor(0x12, 0xFF, 0xFF, 0xFF));
    auto pressed = SolidColorBrush(MakeColor(0x20, 0xD7, 0xDB, 0xE1));
    row.Background(resting);
    row.PointerEntered([hover](IInspectable const& sender,
                               PointerRoutedEventArgs const&) {
        if (auto grid = sender.try_as<Grid>()) {
            grid.Background(hover);
        }
    });
    row.PointerExited([resting](IInspectable const& sender,
                                PointerRoutedEventArgs const&) {
        if (auto grid = sender.try_as<Grid>()) {
            grid.Background(resting);
        }
    });
    row.PointerPressed([pressed](IInspectable const& sender,
                                 PointerRoutedEventArgs const&) {
        if (auto grid = sender.try_as<Grid>()) {
            grid.Background(pressed);
        }
    });
    row.PointerReleased([hover](IInspectable const& sender,
                                PointerRoutedEventArgs const&) {
        if (auto grid = sender.try_as<Grid>()) {
            grid.Background(hover);
        }
    });
}

void MakeMetricRowInteractive(Grid row, bool resourceMonitor) {
    if (!row) {
        return;
    }
    row.IsHitTestVisible(true);
    ApplyUnifiedHoverLanguage(row);
    row.Tapped([resourceMonitor](IInspectable const& sender,
                                 TappedRoutedEventArgs const& args) {
        ModSettings settings = CurrentSettings();
        if (settings.commandCenterEnabled) {
            ShowHardwareCommandCenter(sender.try_as<FrameworkElement>());
        } else {
            LaunchMonitoringTool(resourceMonitor);
        }
        args.Handled(true);
    });
}

void SetMetricTooltip(Grid row, const std::wstring& text) {
    if (!row) {
        return;
    }
    ToolTip tooltip;
    tooltip.Content(box_value(text));
    ToolTipService::SetToolTip(row, tooltip);
}

void UpdateMetricTooltips(const ModSettings& settings) {
    std::wstring taskManagerClickHelp = settings.commandCenterEnabled
                                            ? L" Click: Hardware Command Center. Right-click: switch taskbar mode."
                                            : L" Click: Task Manager. Right-click: switch taskbar mode.";
    std::wstring resourceMonitorClickHelp = settings.commandCenterEnabled
                                                ? L" Click: Hardware Command Center. Right-click: switch taskbar mode."
                                                : L" Click: Resource Monitor. Right-click: switch taskbar mode.";
    SetMetricTooltip(
        g_cpuRow,
        L"CPU load = processor work right now. Neutral below " +
            std::to_wstring(settings.computeWarningPercent) +
            L"%; amber at " +
            std::to_wstring(settings.computeWarningPercent) + L"%; red at " +
            std::to_wstring(settings.computeCriticalPercent) +
            L"%. CPU temperature: green below " +
            FormatTemperatureThreshold(settings.cpuWarningTemp, settings) +
            L"; amber at that point; red at " +
            FormatTemperatureThreshold(settings.cpuCriticalTemp, settings) +
            L"." + taskManagerClickHelp);
    SetMetricTooltip(
        g_gpuRow,
        L"GPU load = graphics-processor work right now. Neutral below " +
            std::to_wstring(settings.computeWarningPercent) +
            L"%; amber at " +
            std::to_wstring(settings.computeWarningPercent) + L"%; red at " +
            std::to_wstring(settings.computeCriticalPercent) +
            L"%. GPU temperature: green below " +
            FormatTemperatureThreshold(settings.gpuWarningTemp, settings) +
            L"; amber at that point; red at " +
            FormatTemperatureThreshold(settings.gpuCriticalTemp, settings) +
            L"." + taskManagerClickHelp);
    SetMetricTooltip(
        g_ramRow,
        L"RAM used = physical memory currently occupied. Neutral below " +
            std::to_wstring(settings.memoryWarningPercent) +
            L"%; amber at " +
            std::to_wstring(settings.memoryWarningPercent) + L"%; red at " +
            std::to_wstring(settings.memoryCriticalPercent) + L"%." +
            resourceMonitorClickHelp);
    SetMetricTooltip(
        g_vramRow,
        L"VRAM used = graphics memory currently occupied. Neutral below " +
            std::to_wstring(settings.memoryWarningPercent) +
            L"%; amber at " +
            std::to_wstring(settings.memoryWarningPercent) + L"%; red at " +
            std::to_wstring(settings.memoryCriticalPercent) + L"%." +
            resourceMonitorClickHelp);
}

void ApplyWidgetSettings() {
    if (!g_widget) {
        return;
    }
    // Color settings feed the cached brushes, so a settings pass must repaint
    // even when the alert levels and scene are unchanged.
    ResetVisualWriteCache();
    ModSettings settings = CurrentSettings();
    if (g_historyInterval != settings.updateInterval ||
        g_historyWindow != settings.historySeconds) {
        g_cpuHistory.clear();
        g_gpuHistory.clear();
        g_historyInterval = settings.updateInterval;
        g_historyWindow = settings.historySeconds;
    }
    MetricsSnapshot latest;
    uint64_t latestSequence = 0;
    bool hasLatest = GetLatestMetrics(latest, latestSequence);
    FocusScene activeScene = hasLatest ? latest.scene : FocusScene::Normal;
    if (g_userLeft >= 0 && g_rootGrid) {
        try {
            double rootWidth = g_rootGrid.ActualWidth();
            if (std::isfinite(rootWidth) && rootWidth > 0.0) {
                g_userLeft = static_cast<int>(std::lround(std::clamp(
                    static_cast<double>(g_userLeft), 0.0,
                    std::max(0.0, rootWidth - kCompactHardwareWidth))));
            }
        } catch (...) {
        }
    }
    ApplyAdaptiveOverlapGovernor(settings, activeScene);
    g_widget.Margin(Thickness{EffectiveLeftOffset(settings), 0, 0, 0});
    g_widget.HorizontalAlignment(HorizontalAlignment::Left);
    g_widget.VerticalAlignment(VerticalAlignment::Center);
    g_widget.IsHitTestVisible(true);
    UpdateMetricTooltips(settings);

    for (TextBlock label :
         {g_cpuLabel, g_gpuLabel, g_ramLabel, g_vramLabel}) {
        ApplyTextStyle(label, true, settings);
    }
    for (TextBlock value : {g_cpuUsageText, g_cpuTempText, g_gpuUsageText,
                            g_gpuTempText, g_ramPercentText,
                            g_ramCapacityText, g_vramPercentText,
                            g_vramCapacityText}) {
        ApplyTextStyle(value, false, settings);
    }
    SetMetricForeground(g_cpuUsageText, g_cpuUsageAlert, settings);
    SetMetricForeground(g_gpuUsageText, g_gpuUsageAlert, settings);
    SetMetricForeground(g_ramPercentText, g_ramAlert, settings);
    SetMetricForeground(g_vramPercentText, g_vramAlert, settings);
    for (TextBlock label :
         {g_cpuLabel, g_gpuLabel, g_ramLabel, g_vramLabel}) {
        SetTextForeground(label, AlertLevel::Normal, settings, true);
    }
    SetTemperatureForeground(g_cpuTempText, g_cpuTemperatureAlert, settings);
    SetTemperatureForeground(g_gpuTempText, g_gpuTemperatureAlert, settings);

    SolidColorBrush graphBrush = AlertBrush(AlertLevel::Normal, settings);
    for (XamlPolyline graph : {g_cpuGraph, g_gpuGraph}) {
        if (graph) {
            graph.Stroke(graphBrush);
            graph.StrokeThickness(1.6);
            graph.StrokeStartLineCap(PenLineCap::Round);
            graph.StrokeEndLineCap(PenLineCap::Round);
            graph.StrokeLineJoin(PenLineJoin::Round);
            graph.Opacity(0.90);
        }
    }
    for (XamlRectangle track : {g_ramTrack, g_vramTrack}) {
        if (track) {
            track.Fill(graphBrush);
            track.Opacity(0.25);
        }
    }
    for (XamlRectangle fill : {g_ramFill, g_vramFill}) {
        if (fill) {
            fill.Fill(graphBrush);
            fill.Opacity(0.90);
        }
    }

    size_t capacity = HistoryCapacity(settings, activeScene);
    while (g_cpuHistory.size() > capacity) {
        g_cpuHistory.pop_front();
    }
    while (g_gpuHistory.size() > capacity) {
        g_gpuHistory.pop_front();
    }
    if (hasLatest) {
        g_contextDensityApplied = false;
        ApplyContextAwareDensity(latest, settings);
    }
    UpdateSparkline(g_cpuGraph, g_cpuHistory, capacity);
    UpdateSparkline(g_gpuGraph, g_gpuHistory, capacity);
    ApplyReservedSpace(settings);

    if (hasLatest) {
        UpdatePerformanceAura(latest, settings);
        UpdateActivityRail(latest, settings);
    }

    if (g_timer) {
        g_timer.Interval(std::chrono::seconds(
            EffectiveUpdateInterval(settings, activeScene)));
    }
}

RowDefinition PixelRow(double height);
TextBlock CreateCellText(PCWSTR name, TextAlignment alignment);

void RemovePerformanceMirrorSlot(PerformanceMirrorSlot& slot) {
    if (slot.repeater && slot.reservedMargin != 0.0) {
        try {
            auto margin = slot.repeater.Margin();
            margin.Left -= slot.reservedMargin;
            slot.repeater.Margin(margin);
        } catch (...) {}
    }
    slot.repeater = nullptr;
    slot.reservedMargin = 0.0;
    if (slot.rootGrid && slot.widget) {
        try {
            uint32_t index = 0;
            if (slot.rootGrid.Children().IndexOf(slot.widget, index))
                slot.rootGrid.Children().RemoveAt(index);
        } catch (...) {}
    }
    slot.rootGrid = nullptr;
    slot.widget = nullptr;
    slot.surfaceBorder = nullptr;
    slot.cpuText = nullptr;
    slot.ramText = nullptr;
    slot.window = nullptr;
}

void RemovePerformanceMirror() {
    for (auto& slot : g_performanceMirrors) RemovePerformanceMirrorSlot(slot);
    g_performanceMirrors.clear();
}

void RemovePerformanceMirrorForWindow(void* value) {
    HWND window = static_cast<HWND>(value);
    for (auto it = g_performanceMirrors.begin();
         it != g_performanceMirrors.end(); ++it) {
        if (it->window == window) {
            RemovePerformanceMirrorSlot(*it);
            g_performanceMirrors.erase(it);
            return;
        }
    }
}

PerformanceMirrorSlot& PerformanceMirrorForWindow(HWND window) {
    for (auto& slot : g_performanceMirrors) {
        if (slot.window == window) return slot;
    }
    g_performanceMirrors.push_back({});
    g_performanceMirrors.back().window = window;
    return g_performanceMirrors.back();
}

bool InjectPerformanceMirror(FrameworkElement taskbarFrame, HWND window) {
    if (!taskbarFrame || g_monitorTarget != OpalControl::MonitorTarget::Both)
        return false;
    auto root = FindDirectChildByName(taskbarFrame, L"RootGrid").try_as<Grid>();
    if (!root) return false;
    auto& slot = PerformanceMirrorForWindow(window);
    RemovePerformanceMirrorSlot(slot);
    slot.window = window;
    for (uint32_t i = 0; i < root.Children().Size();) {
        auto child = root.Children().GetAt(i).try_as<FrameworkElement>();
        if (child && (child.Name() == kMirrorWidgetName ||
                      child.Name() == kLegacyMirrorWidgetName))
            root.Children().RemoveAt(i);
        else
            i++;
    }

    ModSettings settings = CurrentSettings();
    Grid widget;
    widget.Name(kMirrorWidgetName);
    slot.repeater = FindDirectChildByName(root, L"TaskbarFrameRepeater");
    if (slot.repeater) {
        slot.reservedMargin = (g_mirrorDetailed ? kCompactHardwareWidth : 140.0) + 16.0;
        auto margin = slot.repeater.Margin();
        margin.Left += slot.reservedMargin;
        slot.repeater.Margin(margin);
    }
    widget.Height(kWidgetHeight);
    widget.Width(g_mirrorDetailed ? kCompactHardwareWidth : 140.0);
    widget.Margin(Thickness{static_cast<double>(settings.leftOffset), 0, 0, 0});
    widget.HorizontalAlignment(HorizontalAlignment::Left);
    widget.VerticalAlignment(VerticalAlignment::Center);
    widget.UseLayoutRounding(true);
    widget.IsHitTestVisible(true);
    widget.Background(SolidColorBrush(Colors::Transparent()));
    Grid::SetColumn(widget, 0);
    Grid::SetColumnSpan(widget,
                        std::max(1, static_cast<int>(root.ColumnDefinitions().Size())));
    Canvas::SetZIndex(widget, 10000);

    widget.RowDefinitions().Append(PixelRow(kRowHeight));
    widget.RowDefinitions().Append(PixelRow(kRowGap));
    widget.RowDefinitions().Append(PixelRow(kRowHeight));
    slot.surfaceBorder = Border();
    slot.surfaceBorder.Name(L"PerformanceMirrorSurface");
    slot.surfaceBorder.CornerRadius(CornerRadius{25, 25, 25, 25});
    slot.surfaceBorder.Background(SolidColorBrush(g_highContrast
        ? MakeColor(0xFF, 0x00, 0x00, 0x00)
        : MakeColor(g_widgetBackgroundAlpha, 0x16, 0x18, 0x1D)));
    slot.surfaceBorder.IsHitTestVisible(false);
    Grid::SetRowSpan(slot.surfaceBorder, 3);
    Canvas::SetZIndex(slot.surfaceBorder, 0);
    widget.Children().Append(slot.surfaceBorder);
    slot.cpuText = CreateCellText(L"MirrorCpu", TextAlignment::Left);
    slot.ramText = CreateCellText(L"MirrorRam", TextAlignment::Left);
    for (TextBlock text : {slot.cpuText, slot.ramText}) {
        text.Margin(Thickness{12, 0, 12, 0});
        text.FontFamily(FontFamily(settings.fontFamily));
        text.FontSize(settings.fontSize);
        text.FontWeight(
            winrt::Windows::UI::Text::FontWeights::SemiBold());
        text.Foreground(SolidColorBrush(MakeColor(
            static_cast<uint8_t>(std::clamp(settings.textOpacity, 0, 100) *
                                 255 / 100),
            0xF5, 0xF5, 0xF7)));
    }
    Grid::SetRow(slot.cpuText, 0);
    Grid::SetRow(slot.ramText, 2);
    widget.Children().Append(slot.cpuText);
    widget.Children().Append(slot.ramText);
    slot.ramText.Visibility(g_mirrorDetailed ? Visibility::Visible
                                             : Visibility::Collapsed);
    widget.Tapped([](IInspectable const& sender,
                     TappedRoutedEventArgs const& args) {
        ShowHardwareCommandCenter(sender.try_as<FrameworkElement>());
        args.Handled(true);
    });
    widget.RightTapped([](IInspectable const&,
                          RightTappedRoutedEventArgs const& args) {
        CycleExperienceMode();
        args.Handled(true);
    });
    winrt::Windows::UI::Xaml::Automation::AutomationProperties::SetHelpText(
        widget, L"Shared CPU and RAM view. Uses the same collector as the primary Opal capsule.");
    root.Children().Append(widget);
    slot.rootGrid = root;
    slot.widget = widget;
    return true;
}

void UpdatePerformanceMirror(const MetricsSnapshot& snapshot,
                             const ModSettings& settings) {
    for (auto& slot : g_performanceMirrors) {
        if (!slot.widget) continue;
        SetTextIfChanged(slot.cpuText, g_mirrorDetailed
            ? L"CPU  " + FormatLoadPercent(snapshot.cpu)
            : L"CPU " + FormatPercent(snapshot.cpu) + L"  ·  RAM " +
                  FormatPercent(snapshot.ram));
        SetTextIfChanged(slot.ramText,
                         L"RAM  " + FormatUsedPercent(snapshot.ram));
        SetTextColorIfChanged(slot.cpuText, AlertColor(g_cpuUsageAlert, settings));
        SetTextColorIfChanged(slot.ramText, AlertColor(g_ramAlert, settings));
        SetAutomationNameIfChanged(
            slot.widget,
            L"CPU " + FormatLoadPercent(snapshot.cpu) + L", RAM " +
                FormatUsedPercent(snapshot.ram));
    }
}

void UpdateWidgetText() {
    if (!g_widget || g_unloading) {
        return;
    }
    MetricsSnapshot snapshot;
    uint64_t metricsSequence = 0;
    if (!GetLatestMetrics(snapshot, metricsSequence)) {
        return;
    }

    // The collector and UI renderer have independent clocks. Skip the normal
    // race where the UI wakes before a fresh snapshot is available.
    if (metricsSequence == g_lastRenderedMetricsSequence) {
        return;
    }
    ModSettings settings = CurrentSettings();

    AlertLevel previousCpuTemperatureAlert = g_cpuTemperatureAlert;
    AlertLevel previousGpuTemperatureAlert = g_gpuTemperatureAlert;
    AlertLevel previousCpuUsageAlert = g_cpuUsageAlert;
    AlertLevel previousGpuUsageAlert = g_gpuUsageAlert;
    AlertLevel previousRamAlert = g_ramAlert;
    AlertLevel previousVramAlert = g_vramAlert;
    g_cpuTemperatureAlert = snapshot.cpuTemp
                                ? EvaluateAlert(*snapshot.cpuTemp,
                                                settings.cpuWarningTemp,
                                                settings.cpuCriticalTemp,
                                                g_cpuTemperatureAlert, 3.0)
                                : AlertLevel::Normal;
    g_gpuTemperatureAlert = snapshot.gpuTemp
                                ? EvaluateAlert(*snapshot.gpuTemp,
                                                settings.gpuWarningTemp,
                                                settings.gpuCriticalTemp,
                                                g_gpuTemperatureAlert, 3.0)
                                : AlertLevel::Normal;
    g_cpuUsageAlert = EvaluateAlert(snapshot.cpu,
                                    settings.computeWarningPercent,
                                    settings.computeCriticalPercent,
                                    g_cpuUsageAlert, 3.0);
    g_gpuUsageAlert =
        snapshot.gpuAvailable
            ? EvaluateAlert(snapshot.gpu, settings.computeWarningPercent,
                            settings.computeCriticalPercent, g_gpuUsageAlert,
                            3.0)
            : AlertLevel::Normal;
    g_ramAlert = EvaluateAlert(snapshot.ram, settings.memoryWarningPercent,
                               settings.memoryCriticalPercent, g_ramAlert, 3.0);
    g_vramAlert =
        snapshot.vramAvailable
            ? EvaluateAlert(snapshot.vram, settings.memoryWarningPercent,
                            settings.memoryCriticalPercent, g_vramAlert, 3.0)
            : AlertLevel::Normal;

    // Resolve the presentation before touching optional visuals. This makes
    // collapsed graphs, the secondary GPU column, and the activity rail true
    // zero-work paths for the common 184-DIP layout.
    ApplyContextAwareDensity(snapshot, settings);
    bool secondaryVisible =
        g_gpuRow && g_gpuRow.Visibility() == Visibility::Visible;

    // Labels remain quiet and stable. Values, temperatures, the activity rail,
    // and the ambient edge carry state so warnings stay clear without noise.

    if (g_cpuUsageText) {
        SetTextIfChanged(g_cpuUsageText, FormatPercent(snapshot.cpu));
        if (g_cpuUsageAlert != previousCpuUsageAlert) {
            SetMetricForeground(g_cpuUsageText, g_cpuUsageAlert, settings);
        }
    }
    if (g_cpuTempText) {
        SetTextIfChanged(g_cpuTempText,
                         FormatTemperature(snapshot.cpuTemp, settings));
        if (g_cpuTemperatureAlert != previousCpuTemperatureAlert) {
            SetTemperatureForeground(g_cpuTempText, g_cpuTemperatureAlert,
                                     settings);
        }
    }
    if (secondaryVisible && g_gpuUsageText) {
        SetTextIfChanged(
            g_gpuUsageText,
            FormatLoadPercent(snapshot.gpu, snapshot.gpuAvailable));
        if (g_gpuUsageAlert != previousGpuUsageAlert) {
            SetMetricForeground(g_gpuUsageText, g_gpuUsageAlert, settings);
        }
    }
    if (secondaryVisible && g_gpuTempText) {
        SetTextIfChanged(g_gpuTempText,
                         FormatTemperature(snapshot.gpuTemp, settings));
        if (g_gpuTemperatureAlert != previousGpuTemperatureAlert) {
            SetTemperatureForeground(g_gpuTempText, g_gpuTemperatureAlert,
                                     settings);
        }
    }
    if (g_ramPercentText) {
        SetTextIfChanged(g_ramPercentText, FormatPercent(snapshot.ram));
        if (g_ramAlert != previousRamAlert) {
            SetMetricForeground(g_ramPercentText, g_ramAlert, settings);
        }
    }
    if (g_ramCapacityText) {
        SetTextIfChanged(g_ramCapacityText,
                         FormatCapacity(snapshot.ramUsedGb,
                                        snapshot.ramTotalGb, true));
    }
    if (secondaryVisible && g_vramPercentText) {
        SetTextIfChanged(
            g_vramPercentText,
            FormatUsedPercent(snapshot.vram, snapshot.vramAvailable));
        if (g_vramAlert != previousVramAlert) {
            SetMetricForeground(g_vramPercentText, g_vramAlert, settings);
        }
    }
    if (secondaryVisible && g_vramCapacityText) {
        SetTextIfChanged(g_vramCapacityText,
                         FormatCapacity(snapshot.vramUsedGb,
                                        snapshot.vramTotalGb,
                                        snapshot.vramAvailable));
    }

    size_t historyCapacity = HistoryCapacity(settings, snapshot.scene);
    AppendHistory(g_cpuHistory, snapshot.cpu, historyCapacity);
    if (snapshot.gpuAvailable) {
        AppendHistory(g_gpuHistory, snapshot.gpu, historyCapacity);
    }
    UpdateSparkline(g_cpuGraph, g_cpuHistory, historyCapacity);
    UpdateSparkline(g_gpuGraph, g_gpuHistory, historyCapacity);
    if (g_cpuGraph && g_cpuUsageAlert != previousCpuUsageAlert) {
        g_cpuGraph.Stroke(AlertBrush(g_cpuUsageAlert, settings));
    }
    if (g_gpuGraph && g_gpuUsageAlert != previousGpuUsageAlert) {
        g_gpuGraph.Stroke(AlertBrush(g_gpuUsageAlert, settings));
    }
    UpdateMemoryBar(g_ramFill, g_ramFillScale, snapshot.ram, true, g_ramAlert,
                    settings,
                    g_ramAlert != previousRamAlert);
    UpdateMemoryBar(g_vramFill, g_vramFillScale, snapshot.vram,
                    snapshot.vramAvailable, g_vramAlert, settings,
                    g_vramAlert != previousVramAlert);
    UpdatePerformanceAura(snapshot, settings);
    UpdateActivityRail(snapshot, settings);
    RefreshCommandCenter(snapshot, settings);
    UpdatePerformanceMirror(snapshot, settings);
    g_lastRenderedMetricsSequence = metricsSequence;
}

void EnsureTimer() {
    if (g_timer) {
        return;
    }
    ModSettings settings = CurrentSettings();
    g_timer = DispatcherTimer();
    MetricsSnapshot latest;
    uint64_t latestSequence = 0;
    FocusScene activeScene = GetLatestMetrics(latest, latestSequence)
                                 ? latest.scene
                                 : FocusScene::Normal;
    g_timer.Interval(std::chrono::seconds(
        EffectiveUpdateInterval(settings, activeScene)));
    g_timerToken = g_timer.Tick([](IInspectable const&, IInspectable const&) {
        try {
            UpdateWidgetText();
        } catch (...) {
            HRESULT error = winrt::to_hresult();
            Wh_Log(L"Metrics update failed: %08X",
                   static_cast<unsigned>(error));
        }
    });
    g_timer.Start();
}

ColumnDefinition PixelColumn(double width) {
    ColumnDefinition column;
    column.Width(GridLength{width, GridUnitType::Pixel});
    return column;
}

RowDefinition PixelRow(double height) {
    RowDefinition row;
    row.Height(GridLength{height, GridUnitType::Pixel});
    return row;
}

TextBlock CreateCellText(PCWSTR name, TextAlignment alignment) {
    TextBlock text;
    text.Name(name);
    text.HorizontalAlignment(HorizontalAlignment::Stretch);
    text.VerticalAlignment(VerticalAlignment::Center);
    text.TextAlignment(alignment);
    text.TextWrapping(TextWrapping::NoWrap);
    text.TextTrimming(TextTrimming::CharacterEllipsis);
    text.IsHitTestVisible(false);
    return text;
}

Grid CreateComputeRow(PCWSTR label,
                      PCWSTR prefix,
                      TextBlock& labelText,
                      TextBlock& usageText,
                      TextBlock& temperatureText,
                      XamlPolyline& graph,
                      bool createGraph) {
    Grid row;
    row.Height(kRowHeight);
    MakeMetricRowInteractive(row, false);

    row.ColumnDefinitions().Append(PixelColumn(kMetricLabelWidth));
    row.ColumnDefinitions().Append(PixelColumn(kMetricUsageWidth));
    row.ColumnDefinitions().Append(PixelColumn(kMetricTempWidth));
    ColumnDefinition graphColumn;
    graphColumn.Width(GridLength{1, GridUnitType::Star});
    row.ColumnDefinitions().Append(graphColumn);

    std::wstring labelName = std::wstring(prefix) + L"Label";
    labelText = CreateCellText(labelName.c_str(), TextAlignment::Left);
    labelText.Text(label);

    std::wstring usageName = std::wstring(prefix) + L"Usage";
    usageText = CreateCellText(usageName.c_str(), TextAlignment::Right);
    usageText.Text(L"--% load");

    std::wstring temperatureName = std::wstring(prefix) + L"Temperature";
    temperatureText =
        CreateCellText(temperatureName.c_str(), TextAlignment::Right);
    temperatureText.Text(FormatTemperature(std::nullopt, CurrentSettings()));

    graph = nullptr;
    if (createGraph) {
        graph = XamlPolyline();
        graph.Name((std::wstring(prefix) + L"History").c_str());
        graph.HorizontalAlignment(HorizontalAlignment::Left);
        graph.VerticalAlignment(VerticalAlignment::Center);
        graph.Margin(Thickness{kGraphLeftGap, 0, 0, 0});
        graph.Stretch(Stretch::None);
        graph.IsHitTestVisible(false);
    }

    Grid::SetColumn(labelText, 0);
    Grid::SetColumn(usageText, 1);
    Grid::SetColumn(temperatureText, 2);
    row.Children().Append(labelText);
    row.Children().Append(usageText);
    row.Children().Append(temperatureText);
    if (graph) {
        Grid::SetColumn(graph, 3);
        row.Children().Append(graph);
    }
    return row;
}

Grid CreateMemoryRow(PCWSTR label,
                     PCWSTR prefix,
                     TextBlock& labelText,
                     TextBlock& percentText,
                     TextBlock& capacityText,
                     XamlRectangle& track,
                     XamlRectangle& fill) {
    Grid row;
    row.Height(kRowHeight);
    MakeMetricRowInteractive(row, true);

    row.ColumnDefinitions().Append(PixelColumn(kMemoryLabelWidth));
    row.ColumnDefinitions().Append(PixelColumn(kMemoryPercentWidth));
    ColumnDefinition capacityColumn;
    capacityColumn.Width(GridLength{1, GridUnitType::Star});
    row.ColumnDefinitions().Append(capacityColumn);

    track = XamlRectangle();
    track.Name((std::wstring(prefix) + L"Track").c_str());
    track.Height(2.0);
    track.HorizontalAlignment(HorizontalAlignment::Left);
    track.VerticalAlignment(VerticalAlignment::Bottom);
    track.RadiusX(1.0);
    track.RadiusY(1.0);
    track.IsHitTestVisible(false);

    fill = XamlRectangle();
    fill.Name((std::wstring(prefix) + L"Fill").c_str());
    fill.Height(2.0);
    fill.HorizontalAlignment(HorizontalAlignment::Left);
    fill.VerticalAlignment(VerticalAlignment::Bottom);
    fill.RadiusX(1.0);
    fill.RadiusY(1.0);
    fill.IsHitTestVisible(false);
    ScaleTransform fillScale;
    fillScale.ScaleX(0.0);
    fillScale.ScaleY(1.0);
    fill.RenderTransformOrigin(Point{0.0F, 0.5F});
    fill.RenderTransform(fillScale);
    if (wcscmp(prefix, L"Ram") == 0) {
        g_ramFillScale = fillScale;
    } else {
        g_vramFillScale = fillScale;
    }

    std::wstring labelName = std::wstring(prefix) + L"Label";
    labelText = CreateCellText(labelName.c_str(), TextAlignment::Left);
    labelText.Text(label);

    std::wstring percentName = std::wstring(prefix) + L"Percent";
    percentText = CreateCellText(percentName.c_str(), TextAlignment::Right);
    percentText.Text(L"--% used");

    std::wstring capacityName = std::wstring(prefix) + L"Capacity";
    capacityText = CreateCellText(capacityName.c_str(), TextAlignment::Right);
    capacityText.Text(L"--/--G");

    Grid::SetColumnSpan(track, 3);
    Grid::SetColumnSpan(fill, 3);
    Grid::SetColumn(labelText, 0);
    Grid::SetColumn(percentText, 1);
    Grid::SetColumn(capacityText, 2);
    row.Children().Append(track);
    row.Children().Append(fill);
    row.Children().Append(labelText);
    row.Children().Append(percentText);
    row.Children().Append(capacityText);
    return row;
}

double ClampWidgetLeft(double left) {
    double rootWidth = g_rootGrid ? g_rootGrid.ActualWidth() : 0.0;
    double widgetWidth = g_widget ? g_widget.ActualWidth() : 0.0;
    if (!std::isfinite(widgetWidth) || widgetWidth <= 0.0) {
        widgetWidth = std::max(kCompactHardwareWidth, g_effectiveWidgetWidth);
    }
    if (!std::isfinite(rootWidth) || rootWidth <= widgetWidth) {
        return std::max(0.0, left);
    }
    return std::clamp(left, 0.0, rootWidth - widgetWidth);
}

void ApplyUserPosition(double left) {
    if (!g_widget) {
        return;
    }
    left = ClampWidgetLeft(left);
    g_userLeft = static_cast<int>(std::lround(left));
    auto margin = g_widget.Margin();
    g_widget.Margin(Thickness{left, margin.Top, margin.Right, margin.Bottom});
}

void AttachWidgetDragHandlers(Grid widget) {
    widget.PointerPressed(
        [](IInspectable const&, PointerRoutedEventArgs const& args) {
            if (!g_manualLayout) return;
            if (!g_rootGrid || !g_widget) {
                return;
            }
            auto point = args.GetCurrentPoint(g_rootGrid);
            if (!point.Properties().IsLeftButtonPressed()) {
                return;
            }
            g_widgetDragPending = true;
            g_widgetDragging = false;
            g_widgetDragPointerId = args.Pointer().PointerId();
            g_widgetDragStartX = point.Position().X;
            g_widgetDragStartLeft = g_widget.Margin().Left;
        });
    widget.PointerMoved(
        [](IInspectable const&, PointerRoutedEventArgs const& args) {
            if (!g_manualLayout) return;
            if (!g_widgetDragPending || !g_rootGrid || !g_widget ||
                args.Pointer().PointerId() != g_widgetDragPointerId) {
                return;
            }
            auto point = args.GetCurrentPoint(g_rootGrid);
            if (!point.Properties().IsLeftButtonPressed()) {
                g_widgetDragPending = false;
                g_widgetDragging = false;
                return;
            }
            double delta = static_cast<double>(point.Position().X) -
                           g_widgetDragStartX;
            if (!g_widgetDragging && std::abs(delta) < kWidgetDragThreshold) {
                return;
            }
            if (!g_widgetDragging) {
                g_widgetDragging = true;
                g_userLeft = static_cast<int>(std::lround(g_widgetDragStartLeft));
                ApplyReservedSpace(CurrentSettings());
                g_widget.CapturePointer(args.Pointer());
            }
            ApplyUserPosition(g_widgetDragStartLeft + delta);
            args.Handled(true);
        });
    widget.PointerReleased(
        [](IInspectable const&, PointerRoutedEventArgs const& args) {
            if (!g_widgetDragPending ||
                args.Pointer().PointerId() != g_widgetDragPointerId) {
                return;
            }
            bool dragged = g_widgetDragging;
            if (dragged && g_widget) {
                g_widget.ReleasePointerCapture(args.Pointer());
                Wh_SetIntValue(CurrentWidgetLeftValue(), g_userLeft);
            }
            g_widgetDragPending = false;
            g_widgetDragging = false;
            g_widgetDragPointerId = 0;
            if (dragged) {
                args.Handled(true);
            }
        });
    widget.PointerCaptureLost(
        [](IInspectable const&, PointerRoutedEventArgs const&) {
            if (!g_manualLayout) return;
            if (g_widgetDragging && g_userLeft >= 0) {
                Wh_SetIntValue(CurrentWidgetLeftValue(), g_userLeft);
            }
            g_widgetDragPending = false;
            g_widgetDragging = false;
            g_widgetDragPointerId = 0;
        });
}

void DetachRootLayoutWatchers() {
    if (g_rootGrid && g_rootSizeChangedToken) {
        try {
            g_rootGrid.SizeChanged(g_rootSizeChangedToken);
        } catch (...) {
        }
        g_rootSizeChangedToken = {};
    }
}

void AttachRootLayoutWatchers(Grid root) {
    DetachRootLayoutWatchers();
    if (!root) {
        return;
    }
    g_rootSizeChangedToken = root.SizeChanged(
        [](IInspectable const&, SizeChangedEventArgs const&) {
            if (g_unloading || !g_widget) {
                return;
            }
            try {
                ModSettings settings = CurrentSettings();
                MetricsSnapshot latest;
                uint64_t sequence = 0;
                FocusScene scene = GetLatestMetrics(latest, sequence)
                                       ? latest.scene
                                       : FocusScene::Normal;
                ApplyAdaptiveOverlapGovernor(settings, scene);
                ApplyReservedSpace(settings);
            } catch (...) {
            }
        });
}

bool WidgetIsMounted() {
    if (!g_widget || !g_rootGrid) {
        return false;
    }
    try {
        uint32_t index = 0;
        if (!g_rootGrid.Children().IndexOf(g_widget, index)) {
            return false;
        }
        return g_widget.Visibility() != Visibility::Collapsed;
    } catch (...) {
        return false;
    }
}

void RemoveWidget() {
    if (g_timer) {
        g_timer.Stop();
        g_timer.Tick(g_timerToken);
        g_timer = nullptr;
        g_timerToken = {};
    }
    DetachRootLayoutWatchers();

    // A rebuilt widget starts with unpainted elements; drop the write cache so
    // the first tick repaints instead of skipping as unchanged.
    ResetVisualWriteCache();

    if (g_taskItemsRepeater && g_reservedMargin != 0.0) {
        Thickness margin = g_taskItemsRepeater.Margin();
        margin.Left -= g_reservedMargin;
        g_taskItemsRepeater.Margin(margin);
    }
    g_reservedMargin = 0.0;
    g_contextDensityApplied = false;
    g_appliedScene = FocusScene::Normal;

    if (g_rootGrid && g_widget) {
        uint32_t index = 0;
        if (g_rootGrid.Children().IndexOf(g_widget, index)) {
            g_rootGrid.Children().RemoveAt(index);
        }
    }

    StopAuraPulse();
    g_widget = nullptr;
    g_rootGrid = nullptr;
    g_taskItemsRepeater = nullptr;
    g_cpuRow = nullptr;
    g_gpuRow = nullptr;
    g_ramRow = nullptr;
    g_vramRow = nullptr;
    if (g_commandCenterFlyout) {
        try {
            g_commandCenterFlyout.Hide();
        } catch (...) {
        }
    }
    g_commandCenterFlyout = nullptr;
    ResetCommandCenterView();
    g_surfaceBorder = nullptr;
    g_auraBorder = nullptr;
    g_activityRail = nullptr;
    g_cpuActivity = nullptr;
    g_gpuActivity = nullptr;
    g_ramActivity = nullptr;
    g_vramActivity = nullptr;
    g_cpuLabel = nullptr;
    g_cpuUsageText = nullptr;
    g_cpuTempText = nullptr;
    g_gpuLabel = nullptr;
    g_gpuUsageText = nullptr;
    g_gpuTempText = nullptr;
    g_ramLabel = nullptr;
    g_ramPercentText = nullptr;
    g_ramCapacityText = nullptr;
    g_vramLabel = nullptr;
    g_vramPercentText = nullptr;
    g_vramCapacityText = nullptr;
    g_cpuGraph = nullptr;
    g_gpuGraph = nullptr;
    g_ramTrack = nullptr;
    g_ramFill = nullptr;
    g_vramTrack = nullptr;
    g_vramFill = nullptr;
    g_ramFillScale = nullptr;
    g_vramFillScale = nullptr;
    g_leftColumn = nullptr;
    g_gapColumn = nullptr;
    g_rightColumn = nullptr;
    g_cpuHistory.clear();
    g_gpuHistory.clear();
    std::deque<double>().swap(g_cpuHistory);
    std::deque<double>().swap(g_gpuHistory);
    g_lastRenderedMetricsSequence = 0;
    g_cpuTemperatureAlert = AlertLevel::Normal;
    g_gpuTemperatureAlert = AlertLevel::Normal;
    g_cpuUsageAlert = AlertLevel::Normal;
    g_gpuUsageAlert = AlertLevel::Normal;
    g_ramAlert = AlertLevel::Normal;
    g_vramAlert = AlertLevel::Normal;
    g_widgetDragPending = false;
    g_widgetDragging = false;
    g_widgetDragPointerId = 0;
    RemovePerformanceMirror();
}

bool InjectWidget(FrameworkElement taskbarFrame) {
    if (!taskbarFrame || g_unloading) {
        return false;
    }

    LoadUserPosition();
    auto root = FindDirectChildByName(taskbarFrame, L"RootGrid").try_as<Grid>();
    if (!root) {
        Wh_Log(L"Taskbar RootGrid not found");
        return false;
    }

    auto children = root.Children();
    for (uint32_t index = 0; index < children.Size();) {
        auto element = children.GetAt(index).try_as<FrameworkElement>();
        if (!element || (element.Name() != kWidgetName &&
                         element.Name() != kLegacyWidgetName)) {
            index++;
            continue;
        }

        uint32_t currentWidgetIndex = 0;
        if (g_widget && children.IndexOf(g_widget, currentWidgetIndex) &&
            currentWidgetIndex == index) {
            AttachRootLayoutWatchers(root);
            OpalControl::MarkPackageSessionLive(L"performance");
            ApplyWidgetSettings();
            if (!StartMetricsWorker()) {
                Wh_Log(L"Metrics worker unavailable");
            }
            EnsureTimer();
            UpdateWidgetText();
            return true;
        }

        Wh_Log(L"Removing stale Taskbar System Info widget");
        children.RemoveAt(index);
    }

    RemoveWidget();

    ModSettings initialSettings = CurrentSettings();
    const bool buildExpandedTaskbar =
        initialSettings.width > static_cast<int>(kCompactHardwareWidth);
    const bool buildInlineGraphs = initialSettings.showInlineGraphs;

    Grid widget;
    widget.Name(kWidgetName);
    // Snap the subtree to whole device pixels. The widget origin depends on the
    // taskbar layout pass and the content-priority ladder resizes columns at
    // runtime, so without this the text, the 1 px aura border, and the 2 px
    // activity rail can all land on half pixels and blur.
    widget.UseLayoutRounding(true);
    widget.IsHitTestVisible(true);
    AttachWidgetDragHandlers(widget);
    winrt::Windows::UI::Xaml::Automation::AutomationProperties::SetHelpText(
        widget, L"Drag to move this hardware capsule. Click a row for Hardware Command Center. Right-click to switch taskbar mode.");
    // The Grid receives Opal's owned glass style in the unified build. Keep
    // its child surface transparent there; standalone and high-contrast modes
    // retain their rounded fallback without square CPU/RAM corners.
    widget.Background(SolidColorBrush(Colors::Transparent()));
    widget.RightTapped([](IInspectable const&, RightTappedRoutedEventArgs const& args) {
        CycleExperienceMode();
        args.Handled(true);
    });
    Canvas::SetZIndex(widget, 10000);
    Grid::SetColumn(widget, 0);
    Grid::SetColumnSpan(widget,
                        std::max(1, static_cast<int>(root.ColumnDefinitions().Size())));

    g_leftColumn = ColumnDefinition();
    g_gapColumn = ColumnDefinition();
    g_rightColumn = ColumnDefinition();
    widget.ColumnDefinitions().Append(g_leftColumn);
    widget.ColumnDefinitions().Append(g_gapColumn);
    widget.ColumnDefinitions().Append(g_rightColumn);

    g_surfaceBorder = Border();
    g_surfaceBorder.Name(L"PerformanceSurface");
    g_surfaceBorder.CornerRadius(CornerRadius{13, 13, 13, 13});
    g_surfaceBorder.Background(SolidColorBrush(g_highContrast
        ? MakeColor(0xFF, 0x00, 0x00, 0x00)
        : MakeColor(g_widgetBackgroundAlpha, 0x16, 0x18, 0x1D)));
    g_surfaceBorder.IsHitTestVisible(false);
    Grid::SetColumnSpan(g_surfaceBorder, 3);
    Canvas::SetZIndex(g_surfaceBorder, 0);
    widget.Children().Append(g_surfaceBorder);

    if (initialSettings.performanceAuraEnabled) {
        g_auraBorder = Border();
        g_auraBorder.Name(L"PerformanceAura");
        g_auraBorder.CornerRadius(CornerRadius{13, 13, 13, 13});
        g_auraBorder.BorderThickness(Thickness{1, 1, 1, 1});
        g_auraBorder.Margin(Thickness{0, 0, 0, 0});
        g_auraBorder.IsHitTestVisible(false);
        Grid::SetColumnSpan(g_auraBorder, 3);
        Canvas::SetZIndex(g_auraBorder, 0);
        widget.Children().Append(g_auraBorder);
    }

    // The canonical 184-DIP surface is intentionally CPU/RAM-only. Avoid
    // allocating its expanded-only rail until an expanded width is requested;
    // GPU/VRAM remain available in Hardware Command Center either way.
    if (buildExpandedTaskbar && initialSettings.activityRailEnabled) {
        g_activityRail = Grid();
        g_activityRail.Name(L"LiveActivityRail");
        g_activityRail.Height(2.0);
        g_activityRail.Margin(Thickness{8, 0, 8, 1});
        g_activityRail.VerticalAlignment(VerticalAlignment::Bottom);
        g_activityRail.IsHitTestVisible(false);
        for (int i = 0; i < 4; i++) {
            ColumnDefinition column;
            column.Width(GridLength{1, GridUnitType::Star});
            g_activityRail.ColumnDefinitions().Append(column);
        }
        std::array<XamlRectangle*, 4> activityTargets{
            &g_cpuActivity, &g_gpuActivity, &g_ramActivity, &g_vramActivity};
        for (int i = 0; i < 4; i++) {
            XamlRectangle segment;
            segment.Height(2.0);
            segment.Margin(Thickness{1, 0, 1, 0});
            segment.RadiusX(1.0);
            segment.RadiusY(1.0);
            segment.HorizontalAlignment(HorizontalAlignment::Stretch);
            Grid::SetColumn(segment, i);
            g_activityRail.Children().Append(segment);
            *activityTargets[i] = segment;
        }
        Grid::SetColumnSpan(g_activityRail, 3);
        Canvas::SetZIndex(g_activityRail, 3);
        widget.Children().Append(g_activityRail);
    }

    Grid leftPanel;
    leftPanel.IsHitTestVisible(true);
    leftPanel.Margin(Thickness{kContentHorizontalInset, 0,
                               kContentHorizontalInset, 0});
    leftPanel.RowDefinitions().Append(PixelRow(kRowHeight));
    leftPanel.RowDefinitions().Append(PixelRow(kRowGap));
    leftPanel.RowDefinitions().Append(PixelRow(kRowHeight));

    g_cpuRow = CreateComputeRow(L"CPU", L"Cpu", g_cpuLabel,
                                g_cpuUsageText, g_cpuTempText, g_cpuGraph,
                                buildInlineGraphs);
    g_ramRow = CreateMemoryRow(L"RAM", L"Ram", g_ramLabel,
                               g_ramPercentText, g_ramCapacityText,
                               g_ramTrack, g_ramFill);
    Grid::SetRow(g_cpuRow, 0);
    Grid::SetRow(g_ramRow, 2);
    leftPanel.Children().Append(g_cpuRow);
    leftPanel.Children().Append(g_ramRow);

    Grid::SetColumn(leftPanel, 0);
    Canvas::SetZIndex(leftPanel, 1);
    widget.Children().Append(leftPanel);
    if (buildExpandedTaskbar) {
        Grid rightPanel;
        rightPanel.IsHitTestVisible(true);
        rightPanel.Margin(Thickness{kContentHorizontalInset, 0,
                                    kContentHorizontalInset, 0});
        rightPanel.RowDefinitions().Append(PixelRow(kRowHeight));
        rightPanel.RowDefinitions().Append(PixelRow(kRowGap));
        rightPanel.RowDefinitions().Append(PixelRow(kRowHeight));

        g_gpuRow = CreateComputeRow(L"GPU", L"Gpu", g_gpuLabel,
                                    g_gpuUsageText, g_gpuTempText, g_gpuGraph,
                                    buildInlineGraphs);
        g_vramRow = CreateMemoryRow(L"VRAM", L"Vram", g_vramLabel,
                                    g_vramPercentText, g_vramCapacityText,
                                    g_vramTrack, g_vramFill);
        Grid::SetRow(g_gpuRow, 0);
        Grid::SetRow(g_vramRow, 2);
        rightPanel.Children().Append(g_gpuRow);
        rightPanel.Children().Append(g_vramRow);
        Grid::SetColumn(rightPanel, 2);
        Canvas::SetZIndex(rightPanel, 1);
        widget.Children().Append(rightPanel);
    }
    root.Children().Append(widget);

    g_rootGrid = root;
    g_widget = widget;
    AttachRootLayoutWatchers(root);
    g_taskItemsRepeater =
        FindDirectChildByName(root, L"TaskbarFrameRepeater");
    g_reservedMargin = 0.0;
    // From here an unclean Explorer exit is attributable to Performance.
    OpalControl::MarkPackageSessionLive(L"performance");

    ApplyWidgetSettings();
    if (!StartMetricsWorker()) {
        Wh_Log(L"Metrics worker unavailable");
    }
    EnsureTimer();
    UpdateWidgetText();
    Wh_Log(L"Taskbar System Info injected");
    return true;
}

using RunFromWindowThreadProc = void (*)(void*);

bool RunFromWindowThread(HWND window,
                         RunFromWindowThreadProc callback,
                         void* context) {
    static const UINT message =
        RegisterWindowMessageW(L"Windhawk_RunFromWindowThread_Performance_" WH_MOD_ID);
    struct CallbackContext {
        RunFromWindowThreadProc callback;
        void* context;
        bool invoked;
    };

    DWORD threadId = GetWindowThreadProcessId(window, nullptr);
    if (!threadId) {
        return false;
    }
    if (threadId == GetCurrentThreadId()) {
        callback(context);
        return true;
    }

    HHOOK hook = SetWindowsHookExW(
        WH_CALLWNDPROC,
        [](int code, WPARAM wParam, LPARAM lParam) -> LRESULT {
            if (code == HC_ACTION) {
                const auto* messageData =
                    reinterpret_cast<const CWPSTRUCT*>(lParam);
                if (messageData->message == message) {
                    auto* callbackContext =
                        reinterpret_cast<CallbackContext*>(messageData->lParam);
                    callbackContext->callback(callbackContext->context);
                    callbackContext->invoked = true;
                }
            }
            return CallNextHookEx(nullptr, code, wParam, lParam);
        },
        nullptr, threadId);
    if (!hook) {
        Wh_Log(L"SetWindowsHookEx failed for taskbar thread %u: %u", threadId,
               GetLastError());
        return false;
    }

    CallbackContext callbackContext{callback, context, false};
    SendMessageW(window, message, 0,
                 reinterpret_cast<LPARAM>(&callbackContext));
    UnhookWindowsHookEx(hook);
    if (!callbackContext.invoked) {
        Wh_Log(L"Taskbar thread dispatch failed for thread %u", threadId);
    }
    return callbackContext.invoked;
}

HWND FindCurrentProcessTaskbarWindow() {
    return OpalControl::VisibleFullViewWindow(g_monitorTarget,
                                              !g_fullViewOnPrimary);
}

bool IsCurrentProcessWindow(HWND window) {
    DWORD processId = 0;
    WCHAR className[64];
    return window && IsWindow(window) &&
           GetWindowThreadProcessId(window, &processId) != 0 &&
           processId == GetCurrentProcessId() &&
           GetClassNameW(window, className, std::size(className)) != 0 &&
           (_wcsicmp(className, L"Shell_TrayWnd") == 0 ||
            _wcsicmp(className, L"Shell_SecondaryTrayWnd") == 0) &&
           OpalControl::MatchesMonitorTarget(window, g_monitorTarget);
}

void RememberTaskbarWindow(HWND window) {
    if (!IsCurrentProcessWindow(window)) {
        return;
    }
    DWORD threadId = GetWindowThreadProcessId(window, nullptr);
    if (threadId) {
        g_taskbarWindow = window;
        g_taskbarThreadId = threadId;
    }
}

HWND FindRememberedTaskbarWindow() {
    HWND rememberedWindow = g_taskbarWindow.load();
    if (IsCurrentProcessWindow(rememberedWindow)) {
        return rememberedWindow;
    }

    DWORD rememberedThreadId = g_taskbarThreadId.load();
    if (rememberedThreadId) {
        HWND threadWindow = nullptr;
        EnumThreadWindows(
            rememberedThreadId,
            [](HWND window, LPARAM context) -> BOOL {
                WCHAR className[64];
                if (GetClassNameW(window, className, std::size(className)) &&
                    (_wcsicmp(className, L"Shell_TrayWnd") == 0 ||
                     _wcsicmp(className, L"Shell_SecondaryTrayWnd") == 0)) {
                    *reinterpret_cast<HWND*>(context) = window;
                    return FALSE;
                }
                return TRUE;
            },
            reinterpret_cast<LPARAM>(&threadWindow));
        if (IsCurrentProcessWindow(threadWindow)) {
            RememberTaskbarWindow(threadWindow);
            return threadWindow;
        }
    }

    HWND currentWindow = FindCurrentProcessTaskbarWindow();
    if (currentWindow) {
        RememberTaskbarWindow(currentWindow);
    }
    return currentWindow;
}

HWND FindAnyWindowOnTaskbarThread(HWND excludedWindow) {
    DWORD threadId = g_taskbarThreadId.load();
    if (!threadId) {
        return nullptr;
    }

    struct SearchContext {
        HWND excludedWindow;
        HWND result;
    } context{excludedWindow, nullptr};
    EnumThreadWindows(
        threadId,
        [](HWND window, LPARAM contextValue) -> BOOL {
            auto* context = reinterpret_cast<SearchContext*>(contextValue);
            DWORD processId = 0;
            if (window != context->excludedWindow &&
                GetWindowThreadProcessId(window, &processId) != 0 &&
                processId == GetCurrentProcessId()) {
                context->result = window;
                return FALSE;
            }
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&context));
    return context.result;
}

using CTaskBand_GetTaskbarHost_t =
    void*(WINAPI*)(void* pThis, void* taskbarHostSharedPtr);
CTaskBand_GetTaskbarHost_t CTaskBand_GetTaskbarHost_Original = nullptr;
CTaskBand_GetTaskbarHost_t CSecondaryTaskBand_GetTaskbarHost_Original = nullptr;

using TaskbarHost_FrameHeight_t = int(WINAPI*)(void* pThis);
TaskbarHost_FrameHeight_t TaskbarHost_FrameHeight_Original = nullptr;

using RefCountBase_Decref_t = void(WINAPI*)(void* pThis);
RefCountBase_Decref_t RefCountBase_Decref_Original = nullptr;

void* CTaskBand_ITaskListWndSite_vftable = nullptr;
void* CSecondaryTaskBand_ITaskListWndSite_vftable = nullptr;

XamlRoot GetTaskbarXamlRoot(HWND taskbarWindow) {
    wchar_t className[64]{};
    GetClassNameW(taskbarWindow, className, std::size(className));
    const bool secondary = _wcsicmp(className, L"Shell_SecondaryTrayWnd") == 0;
    auto getHost = secondary ? CSecondaryTaskBand_GetTaskbarHost_Original
                             : CTaskBand_GetTaskbarHost_Original;
    void* expectedVtable = secondary ? CSecondaryTaskBand_ITaskListWndSite_vftable
                                     : CTaskBand_ITaskListWndSite_vftable;
    if (!getHost ||
        !TaskbarHost_FrameHeight_Original || !RefCountBase_Decref_Original ||
        !expectedVtable) {
        return nullptr;
    }

    HWND taskBandWindow = secondary
        ? FindWindowExW(taskbarWindow, nullptr, L"WorkerW", nullptr)
        : reinterpret_cast<HWND>(GetPropW(taskbarWindow, L"TaskbandHWND"));
    if (!taskBandWindow) {
        return nullptr;
    }

    void* taskBand = reinterpret_cast<void*>(
        GetWindowLongPtrW(taskBandWindow, 0));
    if (!taskBand) {
        return nullptr;
    }

    void* taskBandForSite = taskBand;
    for (int i = 0;
         *reinterpret_cast<void**>(taskBandForSite) !=
         expectedVtable;
         i++) {
        if (i == 20) {
            return nullptr;
        }
        taskBandForSite = reinterpret_cast<void**>(taskBandForSite) + 1;
    }

    void* taskbarHostSharedPtr[2]{};
    getHost(taskBandForSite, taskbarHostSharedPtr);
    if (!taskbarHostSharedPtr[0] || !taskbarHostSharedPtr[1]) {
        if (taskbarHostSharedPtr[1]) {
            RefCountBase_Decref_Original(taskbarHostSharedPtr[1]);
        }
        return nullptr;
    }

    size_t elementOffset = 0;
#if defined(_M_X64)
    const BYTE* code =
        reinterpret_cast<const BYTE*>(TaskbarHost_FrameHeight_Original);
    if (code[0] == 0x48 && code[1] == 0x83 && code[2] == 0xEC &&
        code[4] == 0x48 && code[5] == 0x83 && code[6] == 0xC1 &&
        code[7] <= 0x7F) {
        elementOffset = code[7];
    } else {
        Wh_Log(L"Unsupported TaskbarHost::FrameHeight pattern");
        RefCountBase_Decref_Original(taskbarHostSharedPtr[1]);
        return nullptr;
    }
#elif defined(_M_ARM64)
    const DWORD* code =
        reinterpret_cast<const DWORD*>(TaskbarHost_FrameHeight_Original);
    if (code[0] == 0xD503237F &&
        (code[1] & 0xFFC07FFF) == 0xA9807BFD &&
        code[2] == 0x910003FD &&
        (code[3] & 0xFFF00FE0) == 0xF8400C00) {
        elementOffset = (code[3] >> 12) & 0xFF;
    } else {
        Wh_Log(L"Unsupported TaskbarHost::FrameHeight pattern");
        RefCountBase_Decref_Original(taskbarHostSharedPtr[1]);
        return nullptr;
    }
#else
#error "Unsupported architecture"
#endif

    auto* elementUnknown = *reinterpret_cast<::IUnknown**>(
        static_cast<BYTE*>(taskbarHostSharedPtr[0]) + elementOffset);
    if (!elementUnknown) {
        RefCountBase_Decref_Original(taskbarHostSharedPtr[1]);
        return nullptr;
    }

    FrameworkElement taskbarElement = nullptr;
    elementUnknown->QueryInterface(winrt::guid_of<FrameworkElement>(),
                                   winrt::put_abi(taskbarElement));
    XamlRoot result = taskbarElement ? taskbarElement.XamlRoot() : nullptr;
    RefCountBase_Decref_Original(taskbarHostSharedPtr[1]);
    return result;
}

struct PerformanceApplyContext {
    HWND window;
    bool mirror;
    bool repairOnly = false;
    bool attached = false;
};

void ApplyToCurrentTaskbar(void* value) {
    auto* context = reinterpret_cast<PerformanceApplyContext*>(value);
    HWND taskbarWindow = context ? context->window
                                 : FindCurrentProcessTaskbarWindow();
    if (!taskbarWindow) {
        return;
    }

    try {
        XamlRoot xamlRoot = GetTaskbarXamlRoot(taskbarWindow);
        if (!xamlRoot) {
            Wh_Log(L"GetTaskbarXamlRoot failed");
            return;
        }
        auto content = xamlRoot.Content().try_as<FrameworkElement>();
        auto taskbarFrame = FindChildRecursive(content, [](FrameworkElement child) {
            return winrt::get_class_name(child) == L"Taskbar.TaskbarFrame";
        });
        if (taskbarFrame) {
            auto root = FindDirectChildByName(taskbarFrame, L"RootGrid").try_as<Grid>();
            if (context && context->repairOnly && root) {
                uint32_t index = 0;
                if (context->mirror) {
                    for (auto const& slot : g_performanceMirrors) {
                        if (slot.window == taskbarWindow && slot.rootGrid == root &&
                            slot.widget && root.Children().IndexOf(slot.widget, index)) {
                            context->attached = true;
                            return;
                        }
                    }
                } else if (g_taskbarWindow.load() == taskbarWindow &&
                           g_rootGrid == root && WidgetIsMounted()) {
                    context->attached = true;
                    return;
                }
            }
            if (context && context->mirror) {
                context->attached = InjectPerformanceMirror(taskbarFrame, taskbarWindow);
            } else {
                RememberTaskbarWindow(taskbarWindow);
                g_userLeftLoaded = false;
                bool attached = InjectWidget(taskbarFrame);
                if (context) context->attached = attached;
                if (!attached) {
                    g_taskbarWindow = nullptr;
                    g_taskbarThreadId = 0;
                }
            }
        }
    } catch (...) {
        HRESULT error = winrt::to_hresult();
        Wh_Log(L"Applying widget failed: %08X",
               static_cast<unsigned>(error));
    }
}

void RemoveFromCurrentTaskbar(void*) {
    try {
        RemoveWidget();
    } catch (...) {
        HRESULT error = winrt::to_hresult();
        Wh_Log(L"Removing widget failed: %08X",
               static_cast<unsigned>(error));
    }
    try {
        g_loadedRevokers.reset();
    } catch (...) {
        HRESULT error = winrt::to_hresult();
        Wh_Log(L"Removing taskbar Loaded handlers failed: %08X",
               static_cast<unsigned>(error));
    }
    g_taskbarWindow = nullptr;
    g_taskbarThreadId = 0;
}

bool ApplyOnTaskbarThread(bool repairOnly = false) {
    HWND fullWindow = FindCurrentProcessTaskbarWindow();
    if (!fullWindow) {
        Wh_Log(L"Taskbar window not found");
        OpalControl::PublishAttachmentProof(L"Performance", nullptr, 1, 0);
        return false;
    }
    PerformanceApplyContext full{fullWindow, false, repairOnly};
    if (!RunFromWindowThread(fullWindow, ApplyToCurrentTaskbar, &full)) {
        Wh_Log(L"Applying widget on taskbar thread failed");
    }
    bool attached = full.attached;
    unsigned attachedViews = full.attached ? 1 : 0;
    auto mirrors = OpalControl::OtherTaskbarWindows(g_monitorTarget, fullWindow);
    std::vector<HWND> leftover;
    for (auto const& slot : g_performanceMirrors) {
        bool wanted = false;
        for (HWND mirrorWindow : mirrors) {
            if (slot.window == mirrorWindow) { wanted = true; break; }
        }
        if (!wanted && slot.window) leftover.push_back(slot.window);
    }
    for (HWND window : leftover) {
        RunFromWindowThread(IsWindow(window) ? window : fullWindow, RemovePerformanceMirrorForWindow,
                            reinterpret_cast<void*>(window));
    }
    for (HWND mirrorWindow : mirrors) {
        PerformanceApplyContext mirror{mirrorWindow, true, repairOnly};
        RunFromWindowThread(mirrorWindow, ApplyToCurrentTaskbar, &mirror);
        attached = attached && mirror.attached;
        attachedViews += mirror.attached ? 1 : 0;
    }
    OpalControl::PublishAttachmentProof(L"Performance", fullWindow,
        static_cast<unsigned>(1 + mirrors.size()), attachedViews);
    return attached;
}

using TaskbarFrame_Constructor_t = void*(WINAPI*)(void* pThis);
TaskbarFrame_Constructor_t TaskbarFrame_Constructor_Original = nullptr;

void* WINAPI TaskbarFrame_Constructor_Hook(void* pThis) {
    void* result = TaskbarFrame_Constructor_Original(pThis);
    if (g_unloading || !g_performanceEnabled || g_quarantine.quarantined ||
        !g_loadedRevokers) {
        return result;
    }

    FrameworkElement taskbarFrame = nullptr;
    reinterpret_cast<::IUnknown**>(pThis)[1]->QueryInterface(
        winrt::guid_of<FrameworkElement>(), winrt::put_abi(taskbarFrame));
    if (!taskbarFrame) {
        return result;
    }

    g_loadedRevokers->emplace_back();
    auto revoker = std::prev(g_loadedRevokers->end());
    *revoker = taskbarFrame.Loaded(
        winrt::auto_revoke_t{},
        [revoker](IInspectable const&, RoutedEventArgs const&) {
            if (!g_loadedRevokers) {
                return;
            }
            g_loadedRevokers->erase(revoker);
            if (g_unloading) {
                return;
            }
            try {
                ApplyOnTaskbarThread();
            } catch (...) {
                HRESULT error = winrt::to_hresult();
                Wh_Log(L"Loaded injection failed: %08X",
                       static_cast<unsigned>(error));
            }
        });
    return result;
}

bool HookTaskbarDllSymbols() {
    HMODULE module =
        LoadLibraryExW(L"taskbar.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!module) {
        return false;
    }

    WindhawkUtils::SYMBOL_HOOK taskbarDllHooks[] = {
        {{LR"(const CTaskBand::`vftable'{for `ITaskListWndSite'})"},
         &CTaskBand_ITaskListWndSite_vftable},
        {{LR"(const CSecondaryTaskBand::`vftable'{for `ITaskListWndSite'})"},
         &CSecondaryTaskBand_ITaskListWndSite_vftable},
        {{LR"(public: virtual class std::shared_ptr<class TaskbarHost> __cdecl CTaskBand::GetTaskbarHost(void)const )"},
         &CTaskBand_GetTaskbarHost_Original},
        {{LR"(public: virtual class std::shared_ptr<class TaskbarHost> __cdecl CSecondaryTaskBand::GetTaskbarHost(void)const )"},
         &CSecondaryTaskBand_GetTaskbarHost_Original},
        {{LR"(public: int __cdecl TaskbarHost::FrameHeight(void)const )"},
         &TaskbarHost_FrameHeight_Original},
        {{LR"(public: void __cdecl std::_Ref_count_base::_Decref(void))"},
         &RefCountBase_Decref_Original},
    };
    return WindhawkUtils::HookSymbols(module, taskbarDllHooks,
                                      std::size(taskbarDllHooks));
}

bool HookTaskbarViewSymbols(HMODULE module) {
    // Taskbar.View.dll, ExplorerExtensions.dll
    WindhawkUtils::SYMBOL_HOOK hooks[] = {{
        {LR"(public: __cdecl winrt::Taskbar::implementation::TaskbarFrame::TaskbarFrame(void))"},
        &TaskbarFrame_Constructor_Original,
        TaskbarFrame_Constructor_Hook,
    }};
    return WindhawkUtils::HookSymbols(module, hooks, std::size(hooks));
}

HMODULE GetTaskbarViewModule() {
    HMODULE module = GetModuleHandleW(L"Taskbar.View.dll");
    if (!module) {
        module = GetModuleHandleW(L"ExplorerExtensions.dll");
    }
    return module;
}

using LoadLibraryExW_t = decltype(&LoadLibraryExW);
LoadLibraryExW_t LoadLibraryExW_Original = nullptr;

HMODULE WINAPI LoadLibraryExW_Hook(LPCWSTR fileName,
                                   HANDLE file,
                                   DWORD flags) {
    HMODULE module = LoadLibraryExW_Original(fileName, file, flags);
    if (module && !g_taskbarViewDllLoaded && GetTaskbarViewModule() == module &&
        !g_taskbarViewDllLoaded.exchange(true)) {
        if (HookTaskbarViewSymbols(module)) {
            Wh_ApplyHookOperations();
        }
    }
    return module;
}

void CloseMetricSources() {
    ClosePdhQuery();
    InvalidateGpuAdapterCache();
    g_nextPdhCounterRetry = {};
    g_nextPdhRecovery = {};
    g_consecutivePdhReadFailures = 0;
}

bool TearDownTaskbarUi() {
    HWND taskbarWindow = FindRememberedTaskbarWindow();
    if (taskbarWindow &&
        RunFromWindowThread(taskbarWindow, RemoveFromCurrentTaskbar, nullptr)) {
        return true;
    }

    // The Shell_TrayWnd can disappear during Explorer teardown. Any surviving
    // window on its UI thread is sufficient: the WH_CALLWNDPROC hook runs the
    // callback, while SendMessage only wakes that thread.
    HWND fallbackWindow = FindAnyWindowOnTaskbarThread(taskbarWindow);
    return fallbackWindow &&
           RunFromWindowThread(fallbackWindow, RemoveFromCurrentTaskbar,
                               nullptr);
}

void ApplyPerformanceControlChange(DWORD) {
    const bool wasEnabled = g_performanceEnabled && !g_quarantine.quarantined;
    if (Wh_GetIntSetting(
            L"advanced.repair.resetCrashQuarantine") != 0) {
        OpalControl::ResetPackageQuarantine(L"performance");
        g_quarantine = OpalControl::BeginPackageSession(L"performance");
    }
    LoadSettings();
    if (g_quarantine.quarantined) g_performanceEnabled = false;
    if (!g_performanceEnabled) {
        StopMetricsWorker();
        TearDownTaskbarUi();
        CloseMetricSources();
        // Publish only after the worker has drained so its last sample cannot
        // overwrite the inactive state. While enabled, the worker owns status
        // (including suspension); settings changes must not overwrite its cache.
        OpalControl::PublishRuntimeState(
            OpalControl::kPerformanceRuntimeActiveValue,
            OpalControl::kPerformanceRuntimePidValue, false, false,
            g_quarantine.reason.c_str(), g_quarantine.quarantined);
        return;
    }
    g_lastRenderedMetricsSequence = 0;
    if (!wasEnabled) StartMetricsWorker();
    WakeMetricsWorker();
    ApplyOnTaskbarThread();
}

}  // namespace

BOOL Wh_ModInit() {
    g_unloading = false;
    g_uiTornDown = false;
    LoadSettings();
    g_quarantine = OpalControl::BeginPackageSession(L"performance");
    if (g_quarantine.quarantined) g_performanceEnabled = false;
    OpalControl::PublishRuntimeState(
        OpalControl::kPerformanceRuntimeActiveValue,
        OpalControl::kPerformanceRuntimePidValue, false, false,
        L"Initializing", g_quarantine.quarantined);
    const auto failInit = [](const wchar_t* reason) -> BOOL {
        Wh_Log(L"Performance initialization failed: %s", reason);
        OpalControl::PublishRuntimeState(
            OpalControl::kPerformanceRuntimeActiveValue,
            OpalControl::kPerformanceRuntimePidValue, false, false,
            reason, g_quarantine.quarantined);
        return FALSE;
    };
    if (HMODULE gdi32 = GetModuleHandleW(L"gdi32.dll")) {
        g_d3dkmtEnumAdapters2 = reinterpret_cast<D3DKMTEnumAdapters2_t>(
            GetProcAddress(gdi32, "D3DKMTEnumAdapters2"));
        g_d3dkmtOpenAdapterFromLuid =
            reinterpret_cast<D3DKMTOpenAdapterFromLuid_t>(
                GetProcAddress(gdi32, "D3DKMTOpenAdapterFromLuid"));
        g_d3dkmtQueryAdapterInfo =
            reinterpret_cast<D3DKMTQueryAdapterInfo_t>(
                GetProcAddress(gdi32, "D3DKMTQueryAdapterInfo"));
        g_d3dkmtCloseAdapter = reinterpret_cast<D3DKMTCloseAdapter_t>(
            GetProcAddress(gdi32, "D3DKMTCloseAdapter"));
    }
    if (!HookTaskbarDllSymbols()) {
        return failInit(L"taskbar.dll symbols unavailable");
    }

    if (HMODULE module = GetTaskbarViewModule()) {
        g_taskbarViewDllLoaded = true;
        if (!HookTaskbarViewSymbols(module)) {
            return failInit(L"Taskbar.View symbols unavailable");
        }
    } else {
#ifndef OPAL_UNIFIED_BUILD
        // The unified shell owns late attachment. Media may already hook
        // LoadLibraryExW in the same mod, so a second hook can fail cold init.
        // Standalone builds still need their own loader fallback.
        HMODULE kernelBase = GetModuleHandleW(L"kernelbase.dll");
        if (!kernelBase) {
            kernelBase = GetModuleHandleW(L"kernel32.dll");
        }
        auto loadLibraryEx = kernelBase
                                 ? reinterpret_cast<LoadLibraryExW_t>(
                                       GetProcAddress(kernelBase,
                                                      "LoadLibraryExW"))
                                 : nullptr;
        if (!loadLibraryEx ||
            !WindhawkUtils::SetFunctionHook(loadLibraryEx, LoadLibraryExW_Hook,
                                            &LoadLibraryExW_Original)) {
            return failInit(L"Loader fallback hook unavailable");
        }
#endif
    }
    OpalControl::PublishRuntimeState(
        OpalControl::kPerformanceRuntimeActiveValue,
        OpalControl::kPerformanceRuntimePidValue, g_performanceEnabled, false,
        g_quarantine.reason.c_str(), g_quarantine.quarantined);
    return TRUE;
}

void Wh_ModAfterInit() {
    if (!g_performanceEnabled) {
        return;
    }
    if (!g_taskbarViewDllLoaded) {
        if (HMODULE module = GetTaskbarViewModule()) {
            if (!g_taskbarViewDllLoaded.exchange(true)) {
                if (HookTaskbarViewSymbols(module)) {
                    Wh_ApplyHookOperations();
                }
            }
        }
    }
    ApplyOnTaskbarThread();
}

void Wh_ModSettingsChanged() {
    ApplyPerformanceControlChange(0);
}

void Wh_ModBeforeUninit() {
    g_unloading = true;
    StopMetricsWorker();

    g_uiTornDown = TearDownTaskbarUi();
    if (!g_uiTornDown) {
        Wh_Log(L"Initial taskbar UI teardown failed; will retry");
    }
}

void Wh_ModUninit() {
    if (!g_uiTornDown) {
        g_uiTornDown = TearDownTaskbarUi();
        if (!g_uiTornDown) {
            Wh_Log(L"Taskbar UI teardown retry failed");
        }
    }
    StopMetricsWorker();
    CloseMetricSources();
    OpalControl::EndPackageSession(L"performance");
}

#ifdef OPAL_UNIFIED_BUILD
// Late attach, driven by the shell's poll (LateAttachProc in the shell source).
// See OpalMedia_EnsureAttached for the failure it recovers from: on 26200 the
// Taskbar.View module can arrive after Wh_ModInit through a loader path the
// LoadLibraryExW hook does not observe, and the one Wh_ModAfterInit attempt ran
// before Shell_TrayWnd existed, so the widget never appeared for the session.
bool OpalPerformance_EnsureAttached() {
    if (g_unloading || !g_performanceEnabled || g_quarantine.quarantined) return true;
    if (!g_taskbarViewDllLoaded) {
        if (HMODULE module = GetTaskbarViewModule()) {
            if (!g_taskbarViewDllLoaded.exchange(true) &&
                HookTaskbarViewSymbols(module)) {
                Wh_ApplyHookOperations();
            }
        }
    }
    return ApplyOnTaskbarThread(true);
}
#endif
