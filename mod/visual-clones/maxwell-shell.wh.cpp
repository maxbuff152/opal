// Historical filenames (maxwell-shell*.h/.cpp) are the Opal 4.5 sources.
// Product name is Opal; Windhawk id is local@opal.

// ==WindhawkMod==
// @id              opal
// @name            Opal
// @description     The Windows 11 taskbar as one Windhawk mod: bar, clock, media, and computer stats
// @version         4.5.0
// @author          Maxbuff152
// @github          https://github.com/Maxbuff152
// @include         explorer.exe
// @include         StartMenuExperienceHost.exe
// @include         SearchHost.exe
// @include         SearchApp.exe
// @include         ShellExperienceHost.exe
// @include         ShellHost.exe
// @architecture    x86-64
// @compilerOptions -Wno-dll-attribute-on-redeclaration -lcomctl32 -ldxgi -lgdi32 -lole32 -loleaut32 -lpdh -lpowrprof -lruntimeobject -lshlwapi -lshell32 -ldwmapi -lshcore -lversion -lwininet -lkernel32
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Opal

One black shell, rendered cleanly at every Windows scale.

Opal is the Windows 11 taskbar. One Windhawk mod owns the bar, clock, media,
and computer stats. Start, Search, and notifications can use the same material.

Geometry is authored in device-independent pixels. Windows performs the final
per-monitor DPI transform, so the same source stays sharp at 100%, 150%, 200%,
and 4K-class scaling instead of stretching bitmap artwork.

On the taskbar, drag the clock/tray capsule or the central tools dock to place
either pill where it works best. Opal remembers both positions across reloads.

## One mod, internal components

Windhawk shows one Opal mod and one settings page. Shell, Clock, Media, and
Performance remain internally isolated components, so optional work can stop
cleanly while the user only has one thing to install and configure.

## Credits & licence

Opal and its integrated Clock are Maxwell-owned components. Their product
design, maintained implementation, XAML tap, selector matcher, style applier,
material, and geometry belong to Maxwell. Portions of the Clock implementation
derived from m417z's Taskbar Clock Customization retain their upstream GPL-3.0
attribution in source. Built on the **Windhawk** platform. GPL-3.0.
*/
// ==/WindhawkModReadme==

// Opal - a Windhawk mod. Copyright (C) 2026 Maxbuff152.
// Licensed under the GNU General Public License v3.0.
// Original work built on the Windhawk platform.

// ==WindhawkModSettings==
/*
- everyday:
  - leanMode: true
    $name: Use less memory and CPU
    $description: Leave this on. Opal slows or releases work that is not currently needed.
  - layoutMode: automatic
    $name: Place media and stats
    $description: Automatic keeps widgets apart. Choose drag them myself only when you want exact positions.
    $options:
    - automatic: Automatic (recommended)
    - custom: Let me drag them myself
  - widgetTextSize: standard
    $name: Widget text
    $description: Changes the Media and Computer stats text, including the small view on another screen.
    $options:
    - small: Small
    - standard: Normal (recommended)
    - large: Large and bold
  - widgetBackgroundStrength: glass
    $name: Widget background
    $description: Strong is easiest to read. Subtle shows more of the wallpaper.
    $options:
    - subtle: Light glass
    - glass: Clear glass (recommended)
    - strong: Strong and easy to read
  $name: Start here
  $description: Opal is the taskbar. One Windhawk mod. These are the everyday choices.
- screens:
  - mediaMonitor: primary
    $name: Show Media on
    $description: Both screens reuse the same media connection, so the extra view is lightweight.
    $options:
    - primary: Main screen only
    - secondary: Second screen only
    - both: Both screens
  - mediaFullDisplay: primary
    $name: Put the big Media widget on
    $description: The other screen gets a compact view. Fullscreen apps do not move your widgets.
    $options:
    - primary: Main screen
    - secondary: Second screen
  - performanceMonitor: primary
    $name: Show Computer stats on
    $description: Both screens reuse one collector; Opal does not measure your computer twice.
    $options:
    - primary: Main screen only
    - secondary: Second screen only
    - both: Both screens
  - performanceFullDisplay: primary
    $name: Put detailed Computer stats on
    $description: Only matters when stats are on both screens. The other screen gets a smaller CPU and RAM view.
    $options:
    - primary: Main screen
    - secondary: Second screen
  - mirrorStyle: detailed
    $name: Small widget on the other screen
    $options:
    - minimal: One short line
    - detailed: Two readable lines (recommended)
  $name: Screens
  $description: Choose where the full widgets and their smaller second-screen views appear.
- media:
  - mediaEnabled: true
    $name: Show the Media widget
    $description: Displays the current song or video and playback controls.
  - mediaSize: standard
    $name: Width
    $description: Wide adds more detail inside the same capsule. The rest of the bar does not move.
    $options:
    - compact: Small
    - standard: Comfortable (recommended)
    - wide: Wide
  - showArtwork: true
    $name: Show cover art
  - showArtist: true
    $name: Show artist and details
  - hideWithoutSession: false
    $name: Hide when nothing is playing
    $description: Off keeps a quiet idle pill so the bar does not jump. On frees the space.
  - smoothProgress: true
    $name: Smooth playback bar
  $name: Media
  $description: Music and video on the Opal bar.
- performance:
  - performanceEnabled: true
    $name: Show Computer stats
    $description: Displays CPU and memory use without opening Task Manager.
  - performanceSize: standard
    $name: Width
    $options:
    - compact: Small · CPU and RAM
    - standard: Comfortable (recommended)
    - expanded: Wide · room for more detail
  - temperatureUnit: fahrenheit
    $name: Temperature
    $options:
    - fahrenheit: Fahrenheit (°F)
    - celsius: Celsius (°C)
  - showInlineGraphs: false
    $name: Show tiny history graphs
    $description: Adds CPU and GPU history only when the wide widget has enough room.
  - commandCenterEnabled: true
    $name: Click for more details
    $description: Opens Opal's hardware panel. Turn off to use Windows Task Manager instead.
  $name: Computer stats
  $description: CPU, memory, and temperatures on the Opal bar, plus the hardware panel.
- windowsLook:
  - enableStart: true
    $name: Style the Start menu
  - enableSearch: true
    $name: Style Windows Search
  - enableNotifications: true
    $name: Style notifications and Quick Settings
  - enableMotion: true
    $name: Use animations
    $description: Brief lightweight motion. Windows reduced-motion preferences always win.
  $name: Start, Search, and notifications
  $description: Optional. The taskbar is Opal and cannot be turned off here. These switches only cover the other Windows surfaces.
- clock:
  - ShowSeconds: false
    $name: Show seconds
    $description: Leave off for a calmer clock and fewer updates.
  - clockSize: standard
    $name: Text size
    $options:
    - small: Small
    - standard: Normal (recommended)
    - large: Large
  - WebContentWeatherLocation: ""
    $name: Weather place
    $description: Enter a city or area. Leave blank to hide weather.
  $name: Clock
  $description: Time and weather on the Opal bar.
- advanced:
  - clockFormatting:
    - TimeFormat: >-
        h':'mm
      $name: Time format
      $description: Windows date/time pattern. The default shows hours and minutes.
    - DateFormat: >-
        ddd, MMM d
      $name: Date format
    - TopLine: '%time%'
      $name: Top line
    - BottomLine: '%date%  %weather%'
      $name: Bottom line
    - TooltipLine: '%date% | %time% | BAT %battery% %battery_time% | ↓%download_speed% ↑%upload_speed%'
      $name: Hover text
    $name: Clock formatting
    $description: Change these only if you want a custom clock layout.
  - taskbarSizing:
    - TaskbarHeight: 68
      $name: Taskbar height
    - IconSize: 38
      $name: App icon size
    - TaskbarButtonWidth: 50
      $name: App button width
    - IconSizeSmall: 28
      $name: Small icon size
    - TaskbarButtonWidthSmall: 42
      $name: Small button width
    $name: Bar size
    $description: Opal is the taskbar. These sizes are the bar itself.
  - repair:
    - resetWidgetPositions: false
      $name: Reset layout
      $description: Turn on once to restore the recommended bar, forget dragged positions, and bring widgets back. Then turn it off.
    - resetCrashQuarantine: false
      $name: Re-enable a protected widget
      $description: Use only if Opal disabled Media or Computer stats after repeated Explorer crashes.
    $name: Repair
  - troubleshooting:
    - diagnose: false
      $name: Write a detailed diagnostic log
      $description: Writes %LOCALAPPDATA%\Maxwell\Opal\diag.log. For debugging only.
    - logUnmatched: false
      $name: Log Windows compatibility misses
      $description: Useful only when a Windows update changes the taskbar or Start menu.
    $name: Troubleshooting
  $name: Advanced
  $description: Fine tuning and repair tools. Most people can leave this section alone.
*/
// ==/WindhawkModSettings==

// Dual-target. Under Windhawk the engine supplies the Wh_* API and calls the
// Wh_Mod* entry points. Standalone (Maxhawk) the shim supplies the handful of
// Wh_* functions this mod uses, and DllMain drives the same lifecycle.
#ifdef WH_MOD
#include <windhawk_api.h>
#include <windhawk_utils.h>
#endif

#include <windows.h>
#include <shlwapi.h>

#ifndef WH_MOD
#include "maxhawk-runtime.h"
#endif

// winbase.h defines GetCurrentTime as a macro and C++/WinRT declares a XAML
// animation method of the same name.
#undef GetCurrentTime

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// Collections is needed to range-for the VisualStateGroups vector; Automation
// for the one AutomationId predicate in the profile.
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.Numerics.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.Input.h>
#include <winrt/Windows.UI.Xaml.Automation.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/Windows.UI.Xaml.Input.h>
#include <winrt/Windows.UI.Xaml.Media.h>

#include "maxwell-shell-rules.h"
#include "maxwell-shell-owned-overrides.h"
#include "maxwell-shell-selector.h"
#include "maxwell-shell-apply.h"
#include "maxwell-shell-style.h"
#include "maxwell-xaml-tap.h"
#include "opal-control.h"

// Taskbar geometry and Clock are part of Opal itself. Media and System Info
// remain separate Explorer-only packages.
#include "opal-addon-icons.h"
#include "opal-addon-clock.h"

using MaxwellRules::Host;
namespace wux  = winrt::Windows::UI::Xaml;
namespace wuxc = winrt::Windows::UI::Xaml::Controls;
namespace wuxi = winrt::Windows::UI::Xaml::Input;
namespace wuxm = winrt::Windows::UI::Xaml::Media;
namespace wuxh = winrt::Windows::UI::Xaml::Hosting;
namespace wfn  = winrt::Windows::Foundation::Numerics;

#ifdef OPAL_UNIFIED_BUILD
BOOL OpalMedia_ModInit();
void OpalMedia_ModAfterInit();
void OpalMedia_ModSettingsChanged();
void OpalMedia_ModBeforeUninit();
void OpalMedia_ModUninit();
BOOL OpalPerformance_ModInit();
void OpalPerformance_ModAfterInit();
void OpalPerformance_ModSettingsChanged();
void OpalPerformance_ModBeforeUninit();
void OpalPerformance_ModUninit();
// Late attach (see LateAttachProc). Each returns true once the component is
// attached to the taskbar or has nothing to attach (disabled, quarantined).
bool OpalMedia_EnsureAttached();
bool OpalPerformance_EnsureAttached();
static bool g_mediaComponentInit = false;
static bool g_performanceComponentInit = false;
#endif

// ---------------------------------------------------------------------
//  Host identity. One binary, six processes; every rule is gated on this.
// ---------------------------------------------------------------------
static Host g_host = Host::Unknown;
static bool g_logUnmatched = false;

// Feature toggles.
//
// Each shell surface maps to exactly one host process, so a surface toggle is a
// host toggle: when off, this host compiles zero rules and Opal is inert there.
// The living accent is separate - it can be switched off while the frosted
// surfaces stay, leaving the authored template colour in place.
static bool g_enableTaskbar       = true;   // explorer.exe
static bool g_enableStart         = true;   // StartMenuExperienceHost.exe
static bool g_enableSearch        = true;   // SearchHost.exe / SearchApp.exe
static bool g_enableNotifications = true;   // ShellExperienceHost.exe / ShellHost.exe
static bool g_enableMotion        = true;
static bool g_leanMode            = true;
static bool g_highContrast        = false;
static std::vector<winrt::weak_ref<wux::FrameworkElement>> g_animatedSurfaces;
// Bumped when settings change so lazily cached XAML styles are rebuilt for
// elements arriving after the change.
static std::atomic<uint32_t> g_styleGeneration{1};

// Set true the instant unload begins. The watch thread and every dispatcher
// callback check it before touching shared state, so nothing runs against
// half-torn-down globals - a crash in explorer.exe is unacceptable.
static std::atomic<bool> g_unloading{false};

// The accent the rules were generated with. Any of these literals found in a
static constexpr const wchar_t* kTemplateAccents[] = { L"#886CD0", L"#A85C2A" };
static constexpr std::wstring_view kFixedAccent = L"#F5F5F7";

// ---------------------------------------------------------------------
//  Diagnostic file log.
//
//  Wh_Log writes to OutputDebugString, which needs a debugger attached to read.
//  When diagnosing why a rule is not matching, that is useless. This writes to
//  a file instead, and is off unless the diagnose setting is on.
// ---------------------------------------------------------------------
static bool g_diagnose = false;

static void DiagWrite(const std::wstring& line);

static void DiagLog(const wchar_t* fmt, ...) {
    if (!g_diagnose) { return; }

    wchar_t line[1024] = {};
    va_list args;
    va_start(args, fmt);
    _vsnwprintf_s(line, _TRUNCATE, fmt, args);
    va_end(args);
    DiagWrite(line);
}

// Always-on, low-volume attach/tap diagnostics, prefixed [attach]. These are
// the handful of lines that decide whether a session is healthy - tap bound or
// not, which late modules arrived, whether each component reached the taskbar -
// and they were invisible without the Windhawk log viewer attached.
static void AttachLog(const wchar_t* fmt, ...) {
    wchar_t line[1024] = {};
    va_list args;
    va_start(args, fmt);
    _vsnwprintf_s(line, _TRUNCATE, fmt, args);
    va_end(args);
    DiagWrite(std::wstring(L"[attach] ") + line);
}

static void DiagWrite(const std::wstring& line) {
    wchar_t localAppData[MAX_PATH] = {};
    DWORD chars = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
    if (!chars || chars >= MAX_PATH) { return; }
    const std::wstring dir = std::wstring(localAppData) + L"\\Maxwell\\Opal";
    CreateDirectoryW((std::wstring(localAppData) + L"\\Maxwell").c_str(), nullptr);
    CreateDirectoryW(dir.c_str(), nullptr);
    const std::wstring path = dir + L"\\diag.log";

    HANDLE h = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) { return; }

    std::wstring text(line);
    text += L"\r\n";
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (bytes > 1) {
        std::string utf8(bytes, '\0');
        WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, utf8.data(), bytes, nullptr, nullptr);
        utf8.resize(bytes - 1);
        DWORD written = 0;
        WriteFile(h, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
    }
    CloseHandle(h);
}

static Host DetectHost() {
    wchar_t path[MAX_PATH] = {};
    if (!GetModuleFileNameW(nullptr, path, MAX_PATH)) { return Host::Unknown; }
    const wchar_t* exe = PathFindFileNameW(path);
    if (_wcsicmp(exe, L"explorer.exe") == 0)                { return Host::Explorer; }
    if (_wcsicmp(exe, L"StartMenuExperienceHost.exe") == 0) { return Host::StartMenu; }
    if (_wcsicmp(exe, L"SearchHost.exe") == 0 ||
        _wcsicmp(exe, L"SearchApp.exe") == 0)               { return Host::Search; }
    if (_wcsicmp(exe, L"ShellExperienceHost.exe") == 0 ||
        _wcsicmp(exe, L"ShellHost.exe") == 0)               { return Host::ShellFlyout; }
    return Host::Unknown;
}

static const wchar_t* HostName(Host h) {
    switch (h) {
        case Host::Explorer:    return L"explorer";
        case Host::StartMenu:   return L"start";
        case Host::Search:      return L"search";
        case Host::ShellFlyout: return L"shell-flyout";
        default:                return L"unknown";
    }
}

// ---------------------------------------------------------------------
//  $Constant substitution.
//
//  Values may reference a style constant, e.g. "Background:=$Background".
//  Constants are per-host, so the taskbar's Background and Start's Background
//  resolve independently even though the sync script keeps them identical.
// ---------------------------------------------------------------------
// Returns the LAST match, not the first.
//
// A constant can be defined twice for the same host: once by the theme compiled
// into the styler, and again by the registry settings that override it. The
// generator emits theme constants first, so the later definition is the
// override - taking the first match silently applied the theme's default
// (Glass = BlurAmount 5, a ThemeResource tint) instead of the tuned profile
// (BlurAmount 32 over #08090D).
static std::wstring LookupConstant(std::wstring_view name) {
    const wchar_t* found = nullptr;
    for (int i = 0; i < MaxwellRules::kConstantCount; ++i) {
        const auto& c = MaxwellRules::kConstants[i];
        if (c.host == g_host && name == c.name) { found = c.value; }
    }
    return found ? std::wstring(found) : std::wstring(name);
}

// The profile contains exactly ONE {{expression}}, and it is the one that makes
// the dock float:
//
//   MaxWidth={{containerGridWidth>0?min($TaskbarFrameMaxWidth,containerGridWidth)
//                                    :$TaskbarFrameMaxWidth}}
//
// It clamps the taskbar frame to TaskbarFrameMaxWidth (1895) unless the
// container is narrower. Combined with Width=Auto that is what stops the frame
// stretching edge to edge.
//
// The upstream styler ships a 425-line general expression evaluator. Since the
// live profile uses a single expression, this recognises that one shape rather
// than reimplementing a language - and returns empty for anything else, which
// the caller logs, so a new expression surfaces instead of silently evaluating
// to nonsense.
static std::wstring EvaluateExpression(const wux::DependencyObject& element,
                                       std::wstring_view expr) {
    if (expr.find(L"containerGridWidth") == std::wstring_view::npos) { return L""; }

    // The clamp constant, resolved from the theme.
    double maxWidth = 0;
    {
        const size_t d = expr.find(L'$');
        if (d == std::wstring_view::npos) { return L""; }
        size_t e = d + 1;
        while (e < expr.size() && (iswalnum(expr[e]) || expr[e] == L'_')) { ++e; }
        maxWidth = _wtof(LookupConstant(expr.substr(d + 1, e - d - 1)).c_str());
    }
    if (maxWidth <= 0) { return L""; }

    // containerGridWidth: the measured width of the frame's container.
    double containerWidth = 0;
    try {
        auto parent = wuxm::VisualTreeHelper::GetParent(element);
        if (auto pfe = parent ? parent.try_as<wux::FrameworkElement>() : nullptr) {
            containerWidth = pfe.ActualWidth();
        }
    } catch (...) {
    }

    const double result = (containerWidth > 0 && containerWidth < maxWidth) ? containerWidth : maxWidth;
    wchar_t buf[32] = {};
    swprintf_s(buf, L"%.0f", result);
    return buf;
}

static std::wstring ResolveValue(const wux::DependencyObject& element,
                                 std::wstring_view value) {
    if (value.size() >= 2 && value[0] == L'{' && value[1] == L'{') {
        std::wstring v = EvaluateExpression(element, value);
        if (v.empty() && g_logUnmatched) {
            Wh_Log(L"[%s] unevaluated expression: %.80s", HostName(g_host), std::wstring(value).c_str());
        }
        return v;
    }
    // Constants substitute ANYWHERE in the value, not only when the whole value
    // is one. The profile contains "CornerRadius=$CornerRadius,0,0,$CornerRadius"
    // and "Margin=$TrayPadding" alike; treating only the whole-value case meant
    // the first form silently produced the literal text "CornerRadius,0,0,$..."
    // which then failed to parse - every rounded corner on the taskbar.
    if (value.find(L'$') == std::wstring_view::npos) { return std::wstring(value); }

    // Constants may reference other constants: the theme defines
    // "Background = $Glass", and a rule then says "Background:=$Background".
    // A single pass leaves "$Glass" sitting there as literal text, which then
    // fails to parse as a brush. Substitute repeatedly until it settles, with a
    // hard cap so a self-referential constant cannot spin forever.
    std::wstring current(value);
    for (int pass = 0; pass < 8; ++pass) {
        if (current.find(L'$') == std::wstring::npos) { break; }

        std::wstring out;
        out.reserve(current.size());
        bool substituted = false;

        for (size_t i = 0; i < current.size();) {
            if (current[i] != L'$') { out.push_back(current[i]); ++i; continue; }

            size_t j = i + 1;
            while (j < current.size() && (iswalnum(current[j]) || current[j] == L'_')) { ++j; }
            if (j == i + 1) { out.push_back(current[i]); ++i; continue; }   // lone '$'

            const std::wstring name = current.substr(i + 1, j - i - 1);
            const std::wstring resolved = LookupConstant(name);
            if (resolved != name) { substituted = true; }
            out += resolved;
            i = j;
        }

        current = std::move(out);
        if (!substituted) { break; }   // nothing left that we can resolve
    }

    // Opal has one fixed neutral accent. Substitute the legacy template colours
    // directly, avoiding accent-source state and a watcher in every shell host.
    for (const wchar_t* tmpl : kTemplateAccents) {
        const std::wstring from(tmpl);
        size_t pos = 0;
        while (pos + from.size() <= current.size()) {
            if (_wcsnicmp(current.c_str() + pos, from.c_str(), from.size()) == 0) {
                current.replace(pos, from.size(), kFixedAccent);
                pos += kFixedAccent.size();
            } else {
                ++pos;
            }
        }
    }
    return current;
}

// ---------------------------------------------------------------------
//  Selector matching, right to left.
//
//  Parsed selectors are cached: the tap fires for every element entering the
//  tree, and re-parsing 49 selectors per element would be pure waste.
// ---------------------------------------------------------------------
struct CompiledRule {
    std::vector<MaxwellShell::Segment> segments;
    const MaxwellRules::Rule*          rule;
};

static std::vector<CompiledRule> g_compiled;

// A surface maps 1:1 to a host process. Explorer is always Opal. Start, Search,
// and notification hosts honor their settings toggles.
static bool SurfaceEnabled() {
    // Native Windows high-contrast colors remain the source of truth. Opal's
    // fixed neutral palette stands down instead of overriding accessibility.
    if (g_highContrast) return false;
    switch (g_host) {
        case Host::Explorer:    return true;
        case Host::StartMenu:   return g_enableStart;
        case Host::Search:      return g_enableSearch;
        case Host::ShellFlyout: return g_enableNotifications;
        default:                return false;
    }
}

static void CompileRules() {
    g_compiled.clear();
    if (!SurfaceEnabled()) {
        Wh_Log(L"[%s] surface disabled by settings, no rules applied", HostName(g_host));
        return;
    }
    for (int i = 0; i < MaxwellRules::kRuleCount; ++i) {
        const auto& r = MaxwellRules::kRules[i];
        if (r.host != g_host) { continue; }
        CompiledRule cr;
        cr.segments = MaxwellShell::ParseSelector(r.selector);
        cr.rule = &r;
        if (!cr.segments.empty()) { g_compiled.push_back(std::move(cr)); }
    }
    // Owned invariants compile last, making icon-only, outline-free taskbar
    // presentation deterministic on both primary and secondary monitors.
    for (int i = 0; i < MaxwellOwnedOverrides::kRuleCount; ++i) {
        const auto& r = MaxwellOwnedOverrides::kRules[i];
        if (r.host != g_host) { continue; }
        CompiledRule cr;
        cr.segments = MaxwellShell::ParseSelector(r.selector);
        cr.rule = &r;
        if (!cr.segments.empty()) { g_compiled.push_back(std::move(cr)); }
    }
    Wh_Log(L"[%s] %zu rules apply to this host", HostName(g_host), g_compiled.size());
}

static std::wstring ElementTypeName(const wux::DependencyObject& obj) {
    try {
        if (auto insp = obj.try_as<winrt::Windows::Foundation::IInspectable>()) {
            return std::wstring(winrt::get_class_name(insp));
        }
    } catch (...) {
    }
    return L"";
}

static std::wstring ElementName(const wux::DependencyObject& obj) {
    if (auto fe = obj.try_as<wux::FrameworkElement>()) { return std::wstring(fe.Name()); }
    return L"";
}

static bool SystemMotionEnabled() {
    return !OpalControl::ReducedMotion() && !OpalControl::HighContrast();
}

static bool IsOpalEntranceSurface(std::wstring_view name) {
    switch (g_host) {
        case Host::StartMenu:
            return name == L"RootContent" || name == L"AllAppsRoot";
        case Host::Search:
            return name == L"OuterBorderGrid";
        case Host::ShellFlyout:
            return name == L"ControlCenterRegion" ||
                   name == L"NotificationCenterGrid" ||
                   name == L"CalendarCenterGrid";
        case Host::Explorer:
            return name == L"ModalRootGrid" || name == L"SnapPickerBorder" ||
                   name == L"SnapBarBorder";
        default:
            return false;
    }
}

static bool MarkSurfaceAnimated(const wux::FrameworkElement& element) {
    for (auto it = g_animatedSurfaces.begin(); it != g_animatedSurfaces.end();) {
        auto existing = it->get();
        if (!existing) {
            it = g_animatedSurfaces.erase(it);
        } else {
            if (existing == element) { return false; }
            ++it;
        }
    }
    g_animatedSurfaces.push_back(winrt::make_weak(element));
    return true;
}

static void StartOpalEntrance(const wux::FrameworkElement& element) {
    if (!g_enableMotion || !SystemMotionEnabled() ||
        !IsOpalEntranceSurface(ElementName(element)) ||
        !MarkSurfaceAnimated(element)) {
        return;
    }

    try {
        auto visual = wuxh::ElementCompositionPreview::GetElementVisual(element);
        auto compositor = visual.Compositor();
        auto outEase = compositor.CreateCubicBezierEasingFunction(
            wfn::float2{0.16F, 1.0F}, wfn::float2{0.30F, 1.0F});
        auto settleEase = compositor.CreateCubicBezierEasingFunction(
            wfn::float2{0.20F, 0.80F}, wfn::float2{0.20F, 1.0F});

        // Fast fade, gentle lift, and a tiny overshoot approximate the calm
        // spring response of premium mobile surfaces. These properties remain
        // entirely on the compositor thread and create no persistent timer.
        auto opacity = compositor.CreateScalarKeyFrameAnimation();
        opacity.InsertKeyFrame(0.0F, 0.0F);
        opacity.InsertKeyFrame(1.0F, 1.0F, outEase);
        opacity.Duration(winrt::Windows::Foundation::TimeSpan{
            std::chrono::milliseconds(190)});
        visual.StartAnimation(L"Opacity", opacity);

        wuxh::ElementCompositionPreview::SetIsTranslationEnabled(element, true);
        visual.Properties().InsertVector3(L"Translation",
                                           wfn::float3{0.0F, 14.0F, 0.0F});
        auto translation = compositor.CreateVector3KeyFrameAnimation();
        translation.InsertKeyFrame(0.0F, wfn::float3{0.0F, 14.0F, 0.0F});
        translation.InsertKeyFrame(0.78F, wfn::float3{0.0F, -1.0F, 0.0F},
                                   outEase);
        translation.InsertKeyFrame(1.0F, wfn::float3{0.0F, 0.0F, 0.0F},
                                   settleEase);
        translation.Duration(winrt::Windows::Foundation::TimeSpan{
            std::chrono::milliseconds(340)});
        visual.StartAnimation(L"Translation", translation);

        double width = element.ActualWidth();
        double height = element.ActualHeight();
        if (std::isfinite(width) && std::isfinite(height) &&
            width > 1.0 && height > 1.0) {
            visual.CenterPoint(wfn::float3{static_cast<float>(width / 2.0),
                                           static_cast<float>(height * 0.86),
                                           0.0F});
            auto scale = compositor.CreateVector3KeyFrameAnimation();
            scale.InsertKeyFrame(0.0F, wfn::float3{0.972F, 0.972F, 1.0F});
            scale.InsertKeyFrame(0.78F, wfn::float3{1.006F, 1.006F, 1.0F},
                                 outEase);
            scale.InsertKeyFrame(1.0F, wfn::float3{1.0F, 1.0F, 1.0F},
                                 settleEase);
            scale.Duration(winrt::Windows::Foundation::TimeSpan{
                std::chrono::milliseconds(360)});
            visual.StartAnimation(L"Scale", scale);
        }
    } catch (...) {
        DiagLog(L"entrance animation unavailable for %s",
                ElementName(element).c_str());
    }
}

// A selector type matches an element whose type DERIVES from it, not only one
// whose name is spelled the same.
//
// The profile says "Grid#IconPanel", but on this build the panel is a
// Taskbar.TaskListButtonPanel - a Grid subclass. Comparing type names alone
// rejected it, which is why the taskbar icons lost their 30x30 sizing and the
// pinned buttons collapsed. There is no runtime base-type query in C++/WinRT,
// so the framework types the profile actually names are tested with try_as.
static bool DerivesFromXamlType(std::wstring_view t, const wux::DependencyObject& o) {
    if (t == L"Grid")                  { return static_cast<bool>(o.try_as<wuxc::Grid>()); }
    if (t == L"Border")                { return static_cast<bool>(o.try_as<wuxc::Border>()); }
    if (t == L"Panel")                 { return static_cast<bool>(o.try_as<wuxc::Panel>()); }
    if (t == L"StackPanel")            { return static_cast<bool>(o.try_as<wuxc::StackPanel>()); }
    if (t == L"Control")               { return static_cast<bool>(o.try_as<wuxc::Control>()); }
    if (t == L"ContentControl")        { return static_cast<bool>(o.try_as<wuxc::ContentControl>()); }
    if (t == L"ContentPresenter")      { return static_cast<bool>(o.try_as<wuxc::ContentPresenter>()); }
    if (t == L"ScrollViewer")          { return static_cast<bool>(o.try_as<wuxc::ScrollViewer>()); }
    if (t == L"ScrollContentPresenter"){ return static_cast<bool>(o.try_as<wuxc::ScrollContentPresenter>()); }
    if (t == L"ItemsPresenter")        { return static_cast<bool>(o.try_as<wuxc::ItemsPresenter>()); }
    if (t == L"TextBlock")             { return static_cast<bool>(o.try_as<wuxc::TextBlock>()); }
    if (t == L"Image")                 { return static_cast<bool>(o.try_as<wuxc::Image>()); }
    if (t == L"Rectangle")             { return static_cast<bool>(o.try_as<winrt::Windows::UI::Xaml::Shapes::Rectangle>()); }
    if (t == L"MenuFlyoutPresenter")   { return static_cast<bool>(o.try_as<wuxc::MenuFlyoutPresenter>()); }
    return false;
}

static bool TypeMatchesElement(std::wstring_view selectorType, const wux::DependencyObject& obj) {
    if (selectorType.empty()) { return true; }
    if (MaxwellShell::TypeMatches(selectorType, ElementTypeName(obj))) { return true; }

    // Try the bare leaf of a qualified selector type too, so
    // "Windows.UI.Xaml.Controls.Grid" also reaches the derives-from test.
    const size_t dot = selectorType.find_last_of(L'.');
    const std::wstring_view leaf =
        (dot == std::wstring_view::npos) ? selectorType : selectorType.substr(dot + 1);
    return DerivesFromXamlType(leaf, obj);
}

static bool SegmentMatches(const MaxwellShell::Segment& seg, const wux::DependencyObject& obj) {
    if (seg.isRoot) { return true; }   // anchor, matches whatever ancestor is there
    if (!TypeMatchesElement(seg.type, obj)) { return false; }
    if (!seg.name.empty() && seg.name != ElementName(obj)) { return false; }

    if (!seg.attrName.empty()) {
        // Only AutomationProperties.AutomationId appears in the profile. Anything
        // else is rejected rather than silently treated as a match.
        if (seg.attrName != L"AutomationProperties.AutomationId") { return false; }
        auto ui = obj.try_as<wux::UIElement>();
        if (!ui) { return false; }
        try {
            if (std::wstring(wux::Automation::AutomationProperties::GetAutomationId(ui)) != seg.attrValue) {
                return false;
            }
        } catch (...) {
            return false;
        }
    }
    return true;
}

// Matches right to left. The RIGHTMOST segment must be the element itself; every
// segment to its left is matched against an ANCESTOR, not necessarily the direct
// parent.
//
// Direct-parent matching was wrong. XAML templates insert layers the profile's
// selectors do not mention - ContentPresenter, ItemsPresenter, panels - so
// "Taskbar.TaskListButton > Grid#IconPanel" has intermediate elements between
// the two in the real visual tree. Requiring adjacency rejected 29 of 62 rules,
// including the icon sizing, which is what made the pinned buttons collapse.
//
// Descendant matching is looser by design and can in principle match more
// broadly than intended; the segments in this profile are specific enough
// (type + x:Name) that this is the correct trade.
static bool RuleMatches(const CompiledRule& cr,
                        const wux::DependencyObject& element,
                        wux::DependencyObject& stateHostOut) {
    const int last = static_cast<int>(cr.segments.size()) - 1;

    // The element itself must satisfy the rightmost segment exactly.
    if (cr.segments[last].isRoot) { return true; }
    if (!SegmentMatches(cr.segments[last], element)) { return false; }
    if (!cr.segments[last].stateGroup.empty()) { stateHostOut = element; }

    wux::DependencyObject current = element;
    for (int i = last - 1; i >= 0; --i) {
        // ":root" is a ZERO-WIDTH ASSERTION, not a segment that consumes an
        // element. ":root > ScrollViewer" means the ScrollViewer *is* the visual
        // root, so nothing is matched against :root itself - instead the element
        // matched to its right must have no parent.
        //
        // Treating it as a wildcard that simply returned true was the bug: any
        // Grid with a Border/ScrollContentPresenter/ScrollViewer ancestry matched,
        // including inner ones deep in the tray. Re-columning one of those is what
        // collapsed the pinned buttons - 33 items became 16 - while the real
        // container was never touched.
        if (cr.segments[i].isRoot) {
            wux::DependencyObject above{nullptr};
            try { above = wuxm::VisualTreeHelper::GetParent(current); }
            catch (...) { return false; }
            return above == nullptr;
        }

        try { current = wuxm::VisualTreeHelper::GetParent(current); }
        catch (...) { return false; }
        if (!current) { return false; }
        if (!SegmentMatches(cr.segments[i], current)) { return false; }
        if (!cr.segments[i].stateGroup.empty()) { stateHostOut = current; }
    }
    return true;
}

// ---------------------------------------------------------------------
//  Visual states.
//
//  State-scoped values ("Background@PointerOver") cannot simply be set: they
//  have to be re-applied whenever the control changes state, or the control's
//  own template overwrites them on the next transition.
// ---------------------------------------------------------------------
static void ApplyPropsForStateManually(const wux::DependencyObject& target,
                                       const MaxwellRules::Rule* rule,
                                       std::wstring_view state);

// ---------------------------------------------------------------------
//  Style cache.
//
//  Parsing a ResourceDictionary per element would be far too slow - the tap
//  fires for every element entering the tree. A Style depends only on the rule,
//  the state, and the resolved TargetType, so it is built once per combination.
//
//  Cached per thread: XAML objects have thread affinity, and each shell surface
//  runs its own UI thread.
// ---------------------------------------------------------------------
struct CachedStyle {
    const MaxwellRules::Rule* rule;
    std::wstring              state;
    std::wstring              targetType;
    uint32_t                  generation;
    wux::Style                style{nullptr};
};
static thread_local std::vector<CachedStyle> tls_styles;
static thread_local uint32_t tls_styleGeneration = 0;

// Best XAML-resolvable TargetType for an element. The exact runtime class is
// tried first; types like Taskbar.TaskbarFrame are not resolvable as a XAML
// TargetType, so the nearest framework base is used instead - which still
// declares most of what the profile sets.
static std::wstring FallbackTypeFor(const wux::DependencyObject& obj) {
    if (obj.try_as<wuxc::Grid>())        { return L"Grid"; }
    if (obj.try_as<wuxc::Border>())      { return L"Border"; }
    if (obj.try_as<wuxc::TextBlock>())   { return L"TextBlock"; }
    if (obj.try_as<wuxc::Image>())       { return L"Image"; }
    if (obj.try_as<wuxc::StackPanel>())  { return L"StackPanel"; }
    if (obj.try_as<wuxc::Panel>())       { return L"Panel"; }
    if (obj.try_as<wuxc::Control>())     { return L"Control"; }
    if (obj.try_as<wux::FrameworkElement>()) { return L"FrameworkElement"; }
    return L"";
}

static wux::Style StyleFor(const wux::DependencyObject& target,
                           const MaxwellRules::Rule* rule,
                           std::wstring_view state) {
    const std::wstring type     = ElementTypeName(target);
    const std::wstring fallback = FallbackTypeFor(target);
    const uint32_t gen = g_styleGeneration.load(std::memory_order_relaxed);

    // Settings changes invalidate every XAML Style in this thread. Clearing the
    // whole generation at first use releases the old WinRT objects instead of
    // retaining one stale entry for every rule/type/state combination across
    // repeated live reloads.
    if (tls_styleGeneration != gen) {
        tls_styles.clear();
        tls_styleGeneration = gen;
    }

    for (auto& c : tls_styles) {
        if (c.rule == rule && c.state == state && c.targetType == type) {
            if (c.generation == gen) { return c.style; }
            // Stale after settings changed. Rebuild this entry below.
            c = tls_styles.back();
            tls_styles.pop_back();
            break;
        }
    }

    std::vector<MaxwellShell::StyleSetter> setters;
    for (int i = 0; i < rule->propCount; ++i) {
        const auto& p = rule->props[i];
        const bool stateless = (p.state == nullptr);
        if (!stateless && state != p.state) { continue; }
        if (stateless && !state.empty())    { continue; }

        setters.push_back({p.name, ResolveValue(target, p.value), p.isXaml});
    }

    wux::Style style{nullptr};
    if (!setters.empty()) {
        style = MaxwellShell::BuildStyleWithFallback(type, fallback, setters);
        if (!style) {
            DiagLog(L"  style build FAILED for %s (fallback %s)", type.c_str(), fallback.c_str());
        }
    }

    tls_styles.push_back({rule, std::wstring(state), type, gen, style});
    return style;
}

static void ApplyPropsForState(const wux::DependencyObject& target,
                               const MaxwellRules::Rule* rule,
                               std::wstring_view state) {
    const auto style = StyleFor(target, rule, state);
    if (style) {
        const int n = MaxwellShell::ApplyStyle(target, style);
        DiagLog(L"    applied %d setter(s) via Style", n);
        return;
    }
    ApplyPropsForStateManually(target, rule, state);
}

// Retained as a safety net for the case where XAML cannot build a Style at all
// (an unresolvable type with no usable base). Hand setters cover the common
// properties; anything else is logged rather than silently skipped.
static void ApplyPropsForStateManually(const wux::DependencyObject& target,
                                       const MaxwellRules::Rule* rule,
                                       std::wstring_view state) {
    for (int i = 0; i < rule->propCount; ++i) {
        const auto& p = rule->props[i];
        const bool stateless = (p.state == nullptr);
        if (!stateless && state != p.state) { continue; }
        if (stateless && !state.empty())    { continue; }   // already applied at attach time

        const std::wstring value = ResolveValue(target, p.value);
        const bool ok = MaxwellShell::ApplyProperty(target, p.name, value);
        if (std::wstring_view(p.name) == L"ColumnDefinitions") {
            DiagLog(L"  GRID COLUMNS BEFORE: %s   applied=%d",
                    MaxwellShell::g_lastGridColumns.c_str(), ok ? 1 : 0);
        }
        DiagLog(L"    %s %s = %.60s", ok ? L"set " : L"FAIL", p.name, value.c_str());
        if (!ok && g_logUnmatched) {
            Wh_Log(L"[%s] could not set %s on %s", HostName(g_host), p.name,
                   ElementTypeName(target).c_str());
        }
    }
}

static void HookVisualStates(const wux::DependencyObject& stateHost,
                             const wux::FrameworkElement& target,
                             const MaxwellRules::Rule* rule) {
    try {
        auto hostPanel = stateHost.try_as<wuxc::Panel>();
        if (!hostPanel) {
            DiagLog(L"  states: host %s is not a Panel, cannot subscribe (%s)",
                    ElementTypeName(stateHost).c_str(), rule->selector);
            return;
        }
        auto groups = wux::VisualStateManager::GetVisualStateGroups(hostPanel);
        if (!groups) {
            DiagLog(L"  states: no groups on %s (%s)",
                    ElementTypeName(stateHost).c_str(), rule->selector);
            return;
        }
        DiagLog(L"  states: %u group(s) on %s (%s)", groups.Size(),
                ElementTypeName(stateHost).c_str(), rule->selector);

        winrt::weak_ref<wux::FrameworkElement> weakTarget = winrt::make_weak(target);
        for (auto&& group : groups) {
            group.CurrentStateChanged(
                [weakTarget, rule](auto&&, wux::VisualStateChangedEventArgs const& args) {
                    auto t = weakTarget.get();
                    if (!t || !args.NewState()) { return; }
                    DiagLog(L"  state -> %s (%s)", args.NewState().Name().c_str(),
                            rule->selector);
                    ApplyPropsForState(t, rule, args.NewState().Name());
                });

            // Apply whatever state the control is already in, so the first paint
            // is correct rather than waiting for the first transition.
            if (auto cur = group.CurrentState()) {
                ApplyPropsForState(target, rule, cur.Name());
            }
        }
    } catch (...) {
        DiagLog(L"  states: exception subscribing (%s)", rule->selector);
    }
}

// ---------------------------------------------------------------------
//  Pointer-driven state emulation.
//
//  The taskbar's controls change visual state through a path that never raises
//  VisualStateGroup.CurrentStateChanged (verified live: groups found and
//  subscribed, zero events on hover), and replacing a Border's Background also
//  disconnects the stock hover storyboard from its brush. Both together left
//  the shell with no hover feedback at all. Pointer events do fire reliably,
//  so the *PointerOver / *Pressed props are driven straight from them.
// ---------------------------------------------------------------------
static void ApplyPropsWithStateSubstring(const wux::DependencyObject& target,
                                         const MaxwellRules::Rule* rule,
                                         std::wstring_view needle) {
    for (int i = 0; i < rule->propCount; ++i) {
        const auto& p = rule->props[i];
        if (!p.state) { continue; }
        if (std::wstring_view(p.state).find(needle) ==
            std::wstring_view::npos) { continue; }
        MaxwellShell::ApplyProperty(target, p.name, ResolveValue(target, p.value));
    }
}

// Restores the calmest definition the rule has for every property a hover or
// press could have touched: a stateless prop first, then a base-state one, and
// an identity transform when the rule defines no rest value at all.
static void ApplyPointerExitReset(const wux::DependencyObject& target,
                                  const MaxwellRules::Rule* rule) {
    for (int i = 0; i < rule->propCount; ++i) {
        const auto& p = rule->props[i];
        if (!p.state) { continue; }
        std::wstring_view st(p.state);
        if (st.find(L"PointerOver") == std::wstring_view::npos &&
            st.find(L"Pressed") == std::wstring_view::npos) { continue; }

        const MaxwellRules::Prop* reset = nullptr;
        for (int j = 0; j < rule->propCount; ++j) {
            const auto& q = rule->props[j];
            if (wcscmp(q.name, p.name) != 0) { continue; }
            if (!q.state) { reset = &q; break; }
            std::wstring_view qs(q.state);
            if (qs.find(L"PointerOver") == std::wstring_view::npos &&
                qs.find(L"Pressed") == std::wstring_view::npos && !reset) {
                reset = &q;
            }
        }
        if (reset) {
            MaxwellShell::ApplyProperty(target, reset->name,
                                        ResolveValue(target, reset->value));
        } else if (wcscmp(p.name, L"RenderTransform") == 0) {
            MaxwellShell::ApplyProperty(
                target, L"RenderTransform",
                L"<ScaleTransform ScaleX=\"1\" ScaleY=\"1\" />");
        }
    }
}

static void AttachPointerStateEmulation(const wux::FrameworkElement& host,
                                        const wux::FrameworkElement& target,
                                        const MaxwellRules::Rule* rule) {
    bool hasPointerProps = false;
    for (int i = 0; i < rule->propCount; ++i) {
        if (!rule->props[i].state) { continue; }
        std::wstring_view st(rule->props[i].state);
        if (st.find(L"PointerOver") != std::wstring_view::npos ||
            st.find(L"Pressed") != std::wstring_view::npos) {
            hasPointerProps = true;
            break;
        }
    }
    if (!hasPointerProps) { return; }

    winrt::weak_ref<wux::FrameworkElement> weakTarget = winrt::make_weak(target);

    host.AddHandler(
        wux::UIElement::PointerEnteredEvent(),
        winrt::box_value(wuxi::PointerEventHandler(
            [weakTarget, rule](auto&&, auto&&) {
                if (auto t = weakTarget.get()) {
                    DiagLog(L"  pointer enter (%s)", rule->selector);
                    ApplyPropsWithStateSubstring(t, rule, L"PointerOver");
                }
            })),
        true);
    host.AddHandler(
        wux::UIElement::PointerExitedEvent(),
        winrt::box_value(wuxi::PointerEventHandler(
            [weakTarget, rule](auto&&, auto&&) {
                if (auto t = weakTarget.get()) {
                    DiagLog(L"  pointer exit (%s)", rule->selector);
                    ApplyPointerExitReset(t, rule);
                }
            })),
        true);
    host.AddHandler(
        wux::UIElement::PointerPressedEvent(),
        winrt::box_value(wuxi::PointerEventHandler(
            [weakTarget, rule](auto&&, auto&&) {
                if (auto t = weakTarget.get()) {
                    ApplyPropsWithStateSubstring(t, rule, L"Pressed");
                }
            })),
        true);
    host.AddHandler(
        wux::UIElement::PointerReleasedEvent(),
        winrt::box_value(wuxi::PointerEventHandler(
            [weakTarget, rule](auto&&, auto&&) {
                if (auto t = weakTarget.get()) {
                    // Still hovering after the click.
                    ApplyPropsWithStateSubstring(t, rule, L"PointerOver");
                }
            })),
        true);
    host.PointerCaptureLost(
        [weakTarget, rule](auto&&, auto&&) {
            if (auto t = weakTarget.get()) {
                ApplyPointerExitReset(t, rule);
            }
        });
}

// ---------------------------------------------------------------------
//  Persistent taskbar pill placement.
//
//  These two surfaces are native Windows layout elements, so Opal moves them
//  with a render transform rather than rewriting the taskbar's column model.
//  A five-DIP threshold preserves ordinary clicks; only a deliberate horizontal
//  movement captures the pointer and becomes a drag.
// ---------------------------------------------------------------------
static constexpr double kPillDragThreshold = 5.0;

struct TaskbarPillDragState {
    winrt::weak_ref<wux::FrameworkElement> element;
    winrt::weak_ref<wux::FrameworkElement> root;
    wuxm::TranslateTransform transform{nullptr};
    std::wstring valueKey;
    bool pending = false;
    bool dragging = false;
    uint32_t pointerId = 0;
    double startPointerX = 0.0;
    double startOffset = 0.0;
};

static wux::FrameworkElement FindTaskbarRoot(
    wux::DependencyObject current) {
    while (current) {
        if (ElementName(current) == L"RootGrid") {
            return current.try_as<wux::FrameworkElement>();
        }
        try {
            current = wuxm::VisualTreeHelper::GetParent(current);
        } catch (...) {
            return nullptr;
        }
    }
    return nullptr;
}

static double ClampPillOffset(
    const std::shared_ptr<TaskbarPillDragState>& state,
    double offset) {
    auto element = state->element.get();
    auto root = state->root.get();
    if (!element || !root) {
        return offset;
    }
    try {
        double rootWidth = root.ActualWidth();
        double elementWidth = element.ActualWidth();
        auto visualOrigin = element.TransformToVisual(root).TransformPoint({0, 0});
        double layoutOrigin = static_cast<double>(visualOrigin.X) -
                              state->transform.X();
        if (!std::isfinite(rootWidth) || !std::isfinite(elementWidth) ||
            !std::isfinite(layoutOrigin) || rootWidth <= 0.0 ||
            elementWidth <= 0.0) {
            return offset;
        }
        return std::clamp(offset, -layoutOrigin,
                          rootWidth - layoutOrigin - elementWidth);
    } catch (...) {
        return offset;
    }
}

static void AttachTaskbarPillDrag(
    const wux::FrameworkElement& element,
    PCWSTR valueKey) {
    if (!element) {
        return;
    }
    auto root = FindTaskbarRoot(element);
    if (!root) {
        return;
    }

    auto state = std::make_shared<TaskbarPillDragState>();
    state->element = winrt::make_weak(element);
    state->root = winrt::make_weak(root);
    state->valueKey = valueKey;
    state->transform = wuxm::TranslateTransform();
    state->transform.X(static_cast<double>(Wh_GetIntValue(valueKey, 0)));
    element.RenderTransform(state->transform);
    element.RenderTransformOrigin({0.0F, 0.0F});

    element.AddHandler(
        wux::UIElement::PointerPressedEvent(),
        winrt::box_value(wuxi::PointerEventHandler(
            [state](auto const&, wuxi::PointerRoutedEventArgs const& args) {
                auto element = state->element.get();
                auto root = state->root.get();
                if (!element || !root) {
                    return;
                }
                auto point = args.GetCurrentPoint(root);
                if (!point.Properties().IsLeftButtonPressed()) {
                    return;
                }
                state->pending = true;
                state->dragging = false;
                state->pointerId = args.Pointer().PointerId();
                state->startPointerX = point.Position().X;
                state->startOffset = state->transform.X();
            })),
        true);
    element.AddHandler(
        wux::UIElement::PointerMovedEvent(),
        winrt::box_value(wuxi::PointerEventHandler(
            [state](auto const&, wuxi::PointerRoutedEventArgs const& args) {
                auto element = state->element.get();
                auto root = state->root.get();
                if (!state->pending || !element || !root ||
                    args.Pointer().PointerId() != state->pointerId) {
                    return;
                }
                auto point = args.GetCurrentPoint(root);
                if (!point.Properties().IsLeftButtonPressed()) {
                    state->pending = false;
                    state->dragging = false;
                    return;
                }
                double delta = static_cast<double>(point.Position().X) -
                               state->startPointerX;
                if (!state->dragging && std::abs(delta) < kPillDragThreshold) {
                    return;
                }
                if (!state->dragging) {
                    state->dragging = true;
                    element.CapturePointer(args.Pointer());
                }
                state->transform.X(
                    ClampPillOffset(state, state->startOffset + delta));
                args.Handled(true);
            })),
        true);
    element.AddHandler(
        wux::UIElement::PointerReleasedEvent(),
        winrt::box_value(wuxi::PointerEventHandler(
            [state](auto const&, wuxi::PointerRoutedEventArgs const& args) {
                if (!state->pending ||
                    args.Pointer().PointerId() != state->pointerId) {
                    return;
                }
                bool dragged = state->dragging;
                if (dragged) {
                    if (auto element = state->element.get()) {
                        element.ReleasePointerCapture(args.Pointer());
                    }
                    Wh_SetIntValue(
                        state->valueKey.c_str(),
                        static_cast<int>(std::lround(state->transform.X())));
                }
                state->pending = false;
                state->dragging = false;
                state->pointerId = 0;
                if (dragged) {
                    args.Handled(true);
                }
            })),
        true);
    element.PointerCaptureLost(
        [state](auto const&, wuxi::PointerRoutedEventArgs const&) {
            if (state->dragging) {
                Wh_SetIntValue(
                    state->valueKey.c_str(),
                    static_cast<int>(std::lround(state->transform.X())));
            }
            state->pending = false;
            state->dragging = false;
            state->pointerId = 0;
        });
}

// ---------------------------------------------------------------------
//  Element arrival
// ---------------------------------------------------------------------
static int g_applied = 0;

static void OnElementAdded(IXamlDiagnostics* diagnostics,
                           InstanceHandle handle,
                           const wchar_t* type,
                           const wchar_t* name) {
    if (g_unloading.load(std::memory_order_acquire)) { return; }
    (void)type;
    (void)name;

    wux::DependencyObject element{nullptr};
    {
        IInspectable* raw = nullptr;
        if (FAILED(diagnostics->GetIInspectableFromHandle(handle, &raw)) || !raw) { return; }
        winrt::Windows::Foundation::IInspectable insp{nullptr};
        winrt::attach_abi(insp, raw);
        element = insp.try_as<wux::DependencyObject>();
    }
    if (!element) { return; }

    // What the tap reports vs what we resolve the object to - if these disagree
    // the selector will never match no matter how correct the rule table is.
    if (g_diagnose) {
        DiagLog(L"elem tap=[%s#%s] resolved=[%s#%s]",
                type ? type : L"", name ? name : L"",
                ElementTypeName(element).c_str(), ElementName(element).c_str());

    }

    int matched = 0;
    for (const auto& cr : g_compiled) {
        wux::DependencyObject stateHost{nullptr};
        if (!RuleMatches(cr, element, stateHost)) { continue; }
        DiagLog(L"  MATCH %s  (%d props)", cr.rule->selector, cr.rule->propCount);
        ++matched;

        // Stateless properties apply immediately.
        ApplyPropsForState(element, cr.rule, L"");
        ++g_applied;

        // State-scoped ones need a subscription on the state host.
        bool hasStateProps = false;
        for (int i = 0; i < cr.rule->propCount; ++i) {
            if (cr.rule->props[i].state) { hasStateProps = true; break; }
        }
        if (hasStateProps) {
            auto fe = element.try_as<wux::FrameworkElement>();
            if (fe) {
                HookVisualStates(stateHost ? stateHost : element, fe, cr.rule);
                auto hostFe = (stateHost ? stateHost : element)
                                  .try_as<wux::FrameworkElement>();
                if (hostFe) {
                    AttachPointerStateEmulation(hostFe, fe, cr.rule);
                }
            }
        }
    }

    if (auto fe = element.try_as<wux::FrameworkElement>()) {
        StartOpalEntrance(fe);
    }

    // One always-on line per taskbar frame root with the box it ended up with
    // after every rule ran. Three rules target this element and the profile's
    // slab/border/padding was still visible on 26200; this is the evidence that
    // settles which values actually landed, at two lines per taskbar.
    if (g_host == Host::Explorer && ElementName(element) == L"RootGrid") {
        try {
            auto parent = wuxm::VisualTreeHelper::GetParent(element);
            auto grid = element.try_as<wuxc::Grid>();
            if (grid && parent && ElementTypeName(parent) == L"Taskbar.TaskbarFrame") {
                const auto pad = grid.Padding();
                const auto bt  = grid.BorderThickness();
                const auto bg  = grid.Background();
                AttachLog(L"frame RootGrid: %d rule(s) matched, padding=%.0f,%.0f,%.0f,%.0f "
                          L"border=%.1f,%.1f,%.1f,%.1f background=%ls",
                          matched, pad.Left, pad.Top, pad.Right, pad.Bottom,
                          bt.Left, bt.Top, bt.Right, bt.Bottom,
                          bg ? ElementTypeName(bg).c_str() : L"(none)");
            }
        } catch (...) {
        }
    }

    if (g_host == Host::Explorer) {
        if (auto fe = element.try_as<wux::FrameworkElement>()) {
            std::wstring elementName = ElementName(element);
            if (elementName == L"SystemTrayFrameGrid") {
                AttachTaskbarPillDrag(fe, L"ClockTrayX");
            } else if (elementName == L"TaskbarFrameRepeater") {
                AttachTaskbarPillDrag(fe, L"ToolsDockX");
            }
        }
    }
}

// ---------------------------------------------------------------------
//  Tap installation
// ---------------------------------------------------------------------

// {6E2D9F41-4C3A-4C2E-9E7B-1F0A5C8D2B10}
static constexpr CLSID kMaxwellTapClsid =
    { 0x6e2d9f41, 0x4c3a, 0x4c2e, { 0x9e, 0x7b, 0x1f, 0x0a, 0x5c, 0x8d, 0x2b, 0x10 } };
// The tap CLSID XAML CoCreates. Named for the mod's internal namespace.

// __declspec(dllexport) is required even though the build passes
// -Wl,--export-all-symbols: that flag is ignored once a DLL has any explicit
// export, and Windhawk's own Wh_Mod* entry points are explicitly exported.
// Without it XAML's GetProcAddress returns null and the tap never loads, while
// the mod happily reports that it initialised.
extern "C" __declspec(dllexport)
HRESULT __stdcall DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv) {
    if (!ppv) { return E_POINTER; }
    *ppv = nullptr;
    if (!IsEqualCLSID(rclsid, kMaxwellTapClsid)) { return CLASS_E_CLASSNOTAVAILABLE; }
    auto* factory = new (std::nothrow) MaxwellShell::detail::TapFactory(&OnElementAdded);
    if (!factory) { return E_OUTOFMEMORY; }
    const HRESULT hr = factory->QueryInterface(riid, ppv);
    factory->Release();
    return hr;
}

using PFN_InitializeXamlDiagnosticsEx = decltype(&InitializeXamlDiagnosticsEx);

static bool InstallTap() {
    const HMODULE wux_dll = LoadLibraryW(L"Windows.UI.Xaml.dll");
    if (!wux_dll) { Wh_Log(L"Windows.UI.Xaml.dll not present"); return false; }
    const auto init = reinterpret_cast<PFN_InitializeXamlDiagnosticsEx>(
        GetProcAddress(wux_dll, "InitializeXamlDiagnosticsEx"));
    if (!init) { Wh_Log(L"InitializeXamlDiagnosticsEx not exported"); return false; }

    // Pass our OWN module path as the TAP DLL.
    //
    // The obvious-looking trick - hand it a sentinel name and intercept
    // LoadLibraryExW - does not work: InitializeXamlDiagnosticsEx resolves and
    // validates the path BEFORE any load happens, so a name with no file behind
    // it fails with ERROR_NOT_FOUND (0x80070490) and the hook is never reached.
    // That failure is silent from the outside: rules compile, the mod reports
    // that it initialised, and nothing is ever styled.
    //
    // Our module is a real file that already exports DllGetClassObject, so
    // pointing XAML straight at it is both simpler and correct. LoadLibrary on
    // an already-loaded module just increments its refcount.
    HMODULE self = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(&InstallTap), &self) || !self) {
        DiagLog(L"could not resolve own module handle");
        return false;
    }

    wchar_t selfPath[MAX_PATH] = {};
    if (!GetModuleFileNameW(self, selfPath, MAX_PATH)) {
        DiagLog(L"could not resolve own module path");
        return false;
    }
    DiagLog(L"tap dll path: %s", selfPath);

    // The endpoint name is NOT arbitrary. XAML resolves it against connections
    // it has already registered, named "VisualDiagConnection1", 2, 3 ... and any
    // other name - or one whose slot is taken - returns ERROR_NOT_FOUND
    // (0x80070490). There is no API to ask which slot is free, so the only way
    // is to try them in order until one is not ERROR_NOT_FOUND. Microsoft's own
    // DXamlCore.cpp is where this behaviour comes from.
    //
    // The third argument must be an empty string rather than nullptr; nullptr
    // fails the same way.
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    int slot = 0;
    for (int i = 1; i <= 64; ++i) {
        wchar_t connection[64] = {};
        swprintf_s(connection, L"VisualDiagConnection%d", i);
        hr = init(connection, GetCurrentProcessId(), L"", selfPath, kMaxwellTapClsid, nullptr);
        if (hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) { slot = i; break; }
    }

    if (FAILED(hr)) {
        Wh_Log(L"InitializeXamlDiagnosticsEx failed: 0x%08X", hr);
        DiagLog(L"InitializeXamlDiagnosticsEx FAILED 0x%08X after trying 64 connections", hr);
        return false;
    }
    DiagLog(L"tap bound on VisualDiagConnection%d", slot);
    Wh_Log(L"[%s] XAML tap installed", HostName(g_host));
    DiagLog(L"XAML tap installed OK");
    return true;
}

// ---------------------------------------------------------------------
//  Windhawk entry points
// ---------------------------------------------------------------------
static void LoadSettings() {
    g_leanMode = Wh_GetIntSetting(L"everyday.leanMode") != 0;
    g_highContrast = OpalControl::HighContrast();
    g_logUnmatched = Wh_GetIntSetting(
        L"advanced.troubleshooting.logUnmatched") != 0;
    g_diagnose = Wh_GetIntSetting(
        L"advanced.troubleshooting.diagnose") != 0;
    // The Explorer taskbar is Opal. There is no switch that returns a stock
    // bar while leaving this mod loaded.
    g_enableTaskbar = true;
    g_enableStart = Wh_GetIntSetting(L"windowsLook.enableStart") != 0;
    g_enableSearch = Wh_GetIntSetting(L"windowsLook.enableSearch") != 0;
    g_enableNotifications = Wh_GetIntSetting(
        L"windowsLook.enableNotifications") != 0;
    g_enableMotion = Wh_GetIntSetting(L"windowsLook.enableMotion") != 0;
    Wh_Log(L"[%s] fixed native-neutral material (%s mode)", HostName(g_host),
           g_leanMode ? L"lean" : L"full");
}

// ---------------------------------------------------------------------
//  Opal-owned taskbar geometry. Clock, Media, and Computer stats are internal
//  components of this same Windhawk mod, not separate packages.
// ---------------------------------------------------------------------
static bool g_geometryInit = false;

// Ensure the taskbar view module is loaded before any addon initialises.
//
// Each addon otherwise installs a LoadLibraryExW fallback to catch that DLL
// loading later - and several components hooking the same LoadLibraryExW inside
// one mod would risk a missing widget on a cold boot. With the module already
// present, every addon takes the direct symbol-hook path and NO LoadLibraryExW
// hook is installed at all. Returns immediately in the common case (after a shell
// restart the module is already loaded); the short retry only bites on a cold
// boot where it appears a beat late.
static void PreloadTaskbarView() {
    // Taskbar.View.dll is a packaged DLL under SystemApps, which is NOT on the
    // default search path - a bare-name LoadLibraryW quietly fails, the preload
    // gives up, and whichever explorer wins the shell race runs with no icon
    // geometry at all (stock oversized icons upscaled from small bitmaps).
    // Resolve the full path first; the bare names stay as a fallback for
    // builds where the DLL lives elsewhere.
    wchar_t windowsDir[MAX_PATH] = {};
    std::wstring packagedPath;
    if (GetWindowsDirectoryW(windowsDir, MAX_PATH)) {
        packagedPath = std::wstring(windowsDir) +
            L"\\SystemApps\\MicrosoftWindows.Client.Core_cw5n1h2txyewy"
            L"\\Taskbar.View.dll";
    }
    for (int i = 0; i < 15; ++i) {
        if (GetModuleHandleW(L"Taskbar.View.dll") ||
            GetModuleHandleW(L"ExplorerExtensions.dll")) { return; }
        if (!packagedPath.empty() && LoadLibraryW(packagedPath.c_str())) { return; }
        if (LoadLibraryW(L"Taskbar.View.dll") ||
            LoadLibraryW(L"ExplorerExtensions.dll")) { return; }
        Sleep(100);
    }
}

static void TaskbarGeometryInit() {
    if (g_host != Host::Explorer) { return; }
    PreloadTaskbarView();
    try { g_geometryInit = OpalAddonIcons::Init(); }
    catch (...) { g_geometryInit = false; }
}
static void TaskbarGeometryAfterInit() {
    if (g_geometryInit) { try { OpalAddonIcons::AfterInit(); } catch (...) {} }
}
static void TaskbarGeometrySettingsChanged() {
    if (g_geometryInit) { try { OpalAddonIcons::SettingsChanged(); } catch (...) {} }
}
static void TaskbarGeometryUninit() {
    if (g_geometryInit) {
        try { OpalAddonIcons::BeforeUninit(); OpalAddonIcons::Uninit(); } catch (...) {}
        g_geometryInit = false;
    }
}

static bool g_clockInit = false;
static void TaskbarClockInit() {
    if (g_host != Host::Explorer) { return; }
    try { g_clockInit = OpalAddonClock::Init(); }
    catch (...) { g_clockInit = false; }
}
static void TaskbarClockAfterInit() {
    if (g_clockInit) { try { OpalAddonClock::AfterInit(); } catch (...) {} }
}
static void TaskbarClockSettingsChanged() {
    if (!g_clockInit) { return; }
    try {
        BOOL reload = FALSE;
        OpalAddonClock::Wh_ModSettingsChanged(&reload);
    } catch (...) {}
}
static void TaskbarClockUninit() {
    if (!g_clockInit) { return; }
    try {
        OpalAddonClock::BeforeUninit();
        OpalAddonClock::Uninit();
    } catch (...) {}
    g_clockInit = false;
}

BOOL Wh_ModInit() {
    // XAML can retain the DLL across a Windhawk reload in the same Explorer.
    // Static initializers do not run again in that case.
    g_unloading.store(false, std::memory_order_release);
    g_host = DetectHost();
    if (g_host == Host::Unknown) {
        Wh_Log(L"Unrecognised host, not initialising.");
        return FALSE;
    }

    DiagLog(L"=== Wh_ModInit host=%s ===", HostName(g_host));
    LoadSettings();
#ifdef OPAL_UNIFIED_BUILD
    if (g_host == Host::Explorer) {
        if (Wh_GetIntSetting(
                L"advanced.repair.resetCrashQuarantine") != 0) {
            OpalControl::ResetPackageQuarantine(L"media");
            OpalControl::ResetPackageQuarantine(L"performance");
        }
        g_mediaComponentInit = OpalMedia_ModInit() != FALSE;
        g_performanceComponentInit = OpalPerformance_ModInit() != FALSE;
        AttachLog(L"component init pid=%lu: media=%d performance=%d",
                  GetCurrentProcessId(), g_mediaComponentInit ? 1 : 0,
                  g_performanceComponentInit ? 1 : 0);
    }
#endif
    CompileRules();
    DiagLog(L"compiled %zu rules for this host", g_compiled.size());
    const bool geometryOnly = g_host == Host::Explorer;
    if (g_compiled.empty() && !geometryOnly) {
        Wh_Log(L"[%s] no rules for this host, not initialising.", HostName(g_host));
        return FALSE;
    }

    // No LoadLibraryExW hook any more: XAML loads our real module path directly,
    // so there is nothing to intercept and one less hook in the host process.
    TaskbarGeometryInit();
    TaskbarClockInit();
    return TRUE;
}

// Sleeps in 100ms slices so unload is never held up by a long wait.
static bool SleepUnlessUnloading(DWORD ms) {
    for (DWORD waited = 0; waited < ms; waited += 100) {
        if (g_unloading.load(std::memory_order_acquire)) { return false; }
        Sleep(100);
    }
    return !g_unloading.load(std::memory_order_acquire);
}

// The XAML diagnostics endpoint is not always ready the instant Windhawk
// injects. On a fast or unusual boot the taskbar's XAML can come up a beat after
// Wh_ModAfterInit runs, and a single InstallTap() attempt then fails with
// ERROR_NOT_FOUND - the mod reports success, nothing is styled, and the dock is
// left full-width instead of floating.
//
// The flyout host is the extreme case. ShellHost.exe (notification centre and
// Quick Settings on 26200) starts at logon but creates its XAML content the
// first time a flyout opens - minutes or hours later. A 15-second retry window
// expired long before there was anything to bind to, so notifications stayed
// stock for the whole session with no error anywhere. The tap now keeps trying
// for as long as the host lives: dense for 15s, then every 3s for five minutes,
// then every 15s. A failed attempt is a handful of fast ERROR_NOT_FOUND calls.
// Runs on its own thread so it never blocks the host; stops early on unload.
static HANDLE g_tapThread = nullptr;

static DWORD WINAPI TapInstallProc(LPVOID) {
    for (int attempt = 0;; ++attempt) {
        if (g_unloading.load(std::memory_order_acquire)) { return 0; }
        if (InstallTap()) {
            AttachLog(L"%ls: XAML tap bound on attempt %d", HostName(g_host), attempt + 1);
            if (attempt > 0) {
                Wh_Log(L"[%s] XAML tap installed on attempt %d", HostName(g_host), attempt + 1);
            }
            return 0;
        }
        if (attempt == 29) {
            AttachLog(L"%ls: XAML tap not bound after 15s (host has no XAML content yet?); "
                      L"retrying in the background", HostName(g_host));
            Wh_Log(L"[%s] XAML tap did not install after 15s; will keep retrying", HostName(g_host));
        }
        const DWORD wait = attempt < 30 ? 500 : (attempt < 130 ? 3000 : 15000);
        if (!SleepUnlessUnloading(wait)) { return 0; }
    }
}

// ---------------------------------------------------------------------
//  Late attach.
//
//  On 26200 the packaged Taskbar.View.dll and SystemTray.dll are loaded through
//  a path the kernelbase LoadLibraryExW hook never observes, and after a shell
//  restart they can arrive a second or more after Wh_ModInit (the diag log shows
//  "taskbar view module not loaded yet" on both explorers of the double-start
//  race). The icon addon already polls for them and recovers. Clock, Media and
//  Performance did not: their LoadLibraryExW fallback never fired, so a session
//  that started before the modules were present ran with a stock clock, no
//  performance widget, and a media widget that only appeared once a song
//  started and its worker happened to re-apply. This poll gives all three the
//  same recovery, then exits.
// ---------------------------------------------------------------------
static HANDLE g_lateAttachThread = nullptr;

static bool TaskbarViewPresent() {
    return GetModuleHandleW(L"Taskbar.View.dll") != nullptr ||
           GetModuleHandleW(L"ExplorerExtensions.dll") != nullptr;
}

static DWORD WINAPI LateAttachProc(LPVOID) {
    // Fast boot retries, then a low-frequency health check. Do not latch a
    // historical success: Windows can replace either taskbar after docking,
    // scaling, sleep, or a display reconnect without restarting Explorer.
    unsigned tick = 0;
    bool healthy = false;
    for (;;) {
        if (!SleepUnlessUnloading(healthy ? 5000 : (tick < 120 ? 500 : 5000))) return 0;
        ++tick;
        if (!TaskbarViewPresent()) { healthy = false; continue; }
        bool clockDone = !g_clockInit;
        if (g_clockInit) {
            try {
                if (!OpalAddonClock::g_systemTrayModuleHooked &&
                    OpalAddonClock::GetSystemTrayModuleHandle()) {
                    OpalAddonClock::AfterInit();
                }
                clockDone = OpalAddonClock::g_systemTrayModuleHooked.load();
            } catch (...) {}
        }
        bool mediaDone = true, perfDone = true;
#ifdef OPAL_UNIFIED_BUILD
        if (g_mediaComponentInit) {
            try { mediaDone = OpalMedia_EnsureAttached(); } catch (...) { mediaDone = false; }
        }
        if (g_performanceComponentInit) {
            try { perfDone = OpalPerformance_EnsureAttached(); } catch (...) { perfDone = false; }
        }
#endif
        bool next = clockDone && mediaDone && perfDone;
        if (next != healthy) {
            AttachLog(L"attachment health: clock=%d media=%d performance=%d", clockDone, mediaDone, perfDone);
        }
        healthy = next;
    }
}
void Wh_ModAfterInit() {
    // Geometry and Clock can operate without shell styling. Opal 3 has no
    // accent watcher: the material is one fixed monochrome palette.
    if (!g_compiled.empty()) {
        g_tapThread = CreateThread(nullptr, 0, TapInstallProc, nullptr, 0, nullptr);
    }
    TaskbarGeometryAfterInit();
    TaskbarClockAfterInit();
#ifdef OPAL_UNIFIED_BUILD
    if (g_mediaComponentInit) OpalMedia_ModAfterInit();
    if (g_performanceComponentInit) OpalPerformance_ModAfterInit();
#endif
    if (g_host == Host::Explorer) {
        AttachLog(L"explorer AfterInit: taskbarView=%d systemTrayHooked=%d",
                  TaskbarViewPresent() ? 1 : 0,
                  OpalAddonClock::g_systemTrayModuleHooked.load() ? 1 : 0);
        g_lateAttachThread = CreateThread(nullptr, 0, LateAttachProc, nullptr, 0, nullptr);
    }
}

void Wh_ModSettingsChanged() {
    LoadSettings();
    CompileRules();   // a surface toggle changes which rules apply here
    // Rebuild fixed-material styles for newly arriving elements and refresh the
    // integrated clock. Existing surface setting changes are applied by reload.
    g_styleGeneration.fetch_add(1, std::memory_order_relaxed);
    TaskbarGeometrySettingsChanged();
    TaskbarClockSettingsChanged();
#ifdef OPAL_UNIFIED_BUILD
    if (g_host == Host::Explorer) {
        if (Wh_GetIntSetting(
                L"advanced.repair.resetCrashQuarantine") != 0) {
            OpalControl::ResetPackageQuarantine(L"media");
            OpalControl::ResetPackageQuarantine(L"performance");
        }
        if (g_mediaComponentInit) OpalMedia_ModSettingsChanged();
        if (g_performanceComponentInit) OpalPerformance_ModSettingsChanged();
    }
#endif
}

void Wh_ModBeforeUninit() {
    // Stop and drain recovery before any component destroys its XAML state.
    // Process pending sent messages if Windhawk invokes teardown on a UI thread.
    g_unloading.store(true, std::memory_order_release);
    if (g_lateAttachThread) {
        while (MsgWaitForMultipleObjects(1, &g_lateAttachThread, FALSE, INFINITE,
                                         QS_SENDMESSAGE) == WAIT_OBJECT_0 + 1) {
            MSG message{};
            PeekMessageW(&message, nullptr, 0, 0, PM_NOREMOVE | PM_QS_SENDMESSAGE);
        }
        CloseHandle(g_lateAttachThread);
        g_lateAttachThread = nullptr;
    }
#ifdef OPAL_UNIFIED_BUILD
    if (g_mediaComponentInit) OpalMedia_ModBeforeUninit();
    if (g_performanceComponentInit) OpalPerformance_ModBeforeUninit();
#endif
}

void Wh_ModUninit() {
    // Signal first so the watch thread and any queued dispatcher callbacks bail
    // before we tear down the state they touch.
    g_unloading.store(true, std::memory_order_release);
    g_animatedSurfaces.clear();
#ifdef OPAL_UNIFIED_BUILD
    if (g_mediaComponentInit) {
        OpalMedia_ModUninit();
        g_mediaComponentInit = false;
    }
    if (g_performanceComponentInit) {
        OpalPerformance_ModUninit();
        g_performanceComponentInit = false;
    }
#endif
    TaskbarClockUninit();
    TaskbarGeometryUninit();
    if (g_lateAttachThread) {
        WaitForSingleObject(g_lateAttachThread, 3000);
        CloseHandle(g_lateAttachThread);
        g_lateAttachThread = nullptr;
    }
    if (g_tapThread) {
        WaitForSingleObject(g_tapThread, 3000);
        CloseHandle(g_tapThread);
        g_tapThread = nullptr;
    }
    Wh_Log(L"[%s] unloading, %d elements styled", HostName(g_host), g_applied);
}

// ---------------------------------------------------------------------
//  Standalone entry point (Maxhawk)
//
//  Windhawk calls Wh_ModInit / Wh_ModAfterInit / Wh_ModUninit itself. Loaded
//  directly, DllMain drives the same sequence.
//
//  DllMain runs under the loader lock, so the real work is handed to a thread.
//  Doing XAML or COM work inside DllMain risks deadlocking the host - and the
//  host here is explorer.exe.
// ---------------------------------------------------------------------
#ifndef WH_MOD

static HMODULE g_selfModule = nullptr;

static DWORD WINAPI MaxhawkInitThread(LPVOID) {
    if (!Wh_ModInit()) {
        Wh_Log(L"Maxhawk: init declined for this process");
        MaxhawkLeaveLoad();          // a declined host is not a fault
        FreeLibraryAndExitThread(g_selfModule, 0);
    }
    Wh_ModAfterInit();

    // Reaching here means initialisation completed without faulting, so the
    // crash-loop guard can be reset.
    MaxhawkLeaveLoad();
    Wh_Log(L"Maxhawk: initialised");
    return 0;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    switch (reason) {
        case DLL_PROCESS_ATTACH: {
            g_selfModule = module;
            DisableThreadLibraryCalls(module);

            // Guards first, before anything else runs.
            if (MaxhawkDisabled()) { return FALSE; }
            if (!MaxhawkEnterLoad()) { return FALSE; }

            if (HANDLE t = CreateThread(nullptr, 0, MaxhawkInitThread, nullptr, 0, nullptr)) {
                CloseHandle(t);
            }
            break;
        }
        case DLL_PROCESS_DETACH:
            // Only on explicit unload; on process teardown there is nothing
            // useful left to do and touching XAML would be unsafe.
            break;
    }
    return TRUE;
}

#endif  // !WH_MOD
