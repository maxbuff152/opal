// ==WindhawkMod==
// @id              opal-addon-media
// @name            Opal Media
// @description     Maxwell-owned adaptive Windows-native media controls for the Opal taskbar
// @version         4.5.0
// @author          Maxbuff152
// @github          https://github.com/Maxbuff152
// @license         GPL-3.0
// @include         explorer.exe
// @architecture    x86-64
// @compilerOptions -lole32 -loleaut32 -lruntimeobject -lwindowsapp -lshell32 -lwindowscodecs -lshlwapi -luser32 -lkernel32
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Opal Media

Opal Media is Maxwell's own focused media surface for Opal. It uses Windows'
native Global System Media Transport Controls session API and draws a compact
XAML interface in device-independent pixels: native-resolution artwork,
title/artist hierarchy, a context-aware media description, previous/play/next
controls, and a live draggable progress rail. Drag any non-control surface to
place the capsule anywhere on the taskbar; the position persists. Scroll the
capsule to change system volume, Shift-scroll to seek ten seconds, scroll its
artwork to move between media sessions, or click any non-control surface to
open the owning app.

It is deliberately not a theme engine. The surrounding chrome follows the
native Windows taskbar's neutral material, spacing, input, and accessibility
conventions. Album artwork and official app identity keep their source colors
because they are content, not decoration.

The layout adapts between full, compact, and play-only densities according to
the real space before Start. A UI interpolation timer advances the two-pixel
progress rail while playback is active; session and artwork
work is event-driven off Explorer's UI thread. Glyph-only controls publish
native accessibility names. There is no audio capture, FFT, visualizer,
wallpaper sampler, accent watcher, or polling process.

Taskbar discovery follows public Windhawk taskbar-integration techniques used
by m417z and retained here under GPL-3.0 attribution. The media product,
interaction model, adaptive layout, and implementation are Maxwell-owned.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- minimumWidth: 232
  $name: Minimum width
  $description: Compact lower bound in device-independent pixels.
- preferredWidth: 304
  $name: Preferred width
  $description: Full-density target width in device-independent pixels.
- maximumWidth: 336
  $name: Maximum width
  $description: Maximum adaptive width in device-independent pixels.
- height: 50
  $name: Height
- showArtwork: true
  $name: Show artwork
- showArtist: true
  $name: Show artist
- hideWithoutSession: false
  $name: Hide without media
- smoothProgress: true
  $name: Smooth progress
  $description: Interpolate the progress rail four times per second while playing.
*/
// ==/WindhawkModSettings==

// Opal Media - Copyright (C) 2026 Maxbuff152.
// Licensed under the GNU General Public License v3.0.

#include <windows.h>
#include <robuffer.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <windhawk_utils.h>

// winbase.h defines GetCurrentTime as a function-like macro, which collides
// with the Windows Runtime XAML animation ABI method of the same name.
#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cwctype>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.Input.h>
#include <winrt/Windows.UI.Text.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Automation.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Controls.Primitives.h>
#include <winrt/Windows.UI.Xaml.Input.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Media.Imaging.h>
#include <winrt/Windows.UI.Xaml.Shapes.h>

#include "opal-control.h"

namespace {

using Microsoft::WRL::ComPtr;
using winrt::Windows::Foundation::IInspectable;
using winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSession;
using winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager;
using winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionMediaProperties;
using winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Controls;
using namespace winrt::Windows::UI::Xaml::Input;
using namespace winrt::Windows::UI::Xaml::Media;
using namespace winrt::Windows::UI::Xaml::Media::Imaging;
using XamlRectangle = winrt::Windows::UI::Xaml::Shapes::Rectangle;

constexpr wchar_t kWidgetName[] = L"MaxwellOpalMedia";
constexpr wchar_t kMirrorWidgetName[] = L"MaxwellOpalMediaMirror";
constexpr wchar_t kSystemInfoWidgetName[] = L"OpalSystemInfo";
constexpr wchar_t kSystemInfoMirrorWidgetName[] = L"OpalSystemInfoMirror";
constexpr wchar_t kLegacySystemInfoWidgetName[] = L"WindhawkTaskbarSystemInfo";
constexpr wchar_t kLegacySystemInfoMirrorWidgetName[] =
    L"WindhawkTaskbarSystemInfoMirror";
constexpr int kPlayingUiIntervalMs = 250;
constexpr int kLeanPlayingUiIntervalMs = 1000;
constexpr int kPausedUiIntervalMs = 1000;
constexpr int kIdleUiIntervalMs = 2000;
constexpr uint64_t kLeanIdleReleaseMs = 30 * 1000;
constexpr uint64_t kMaximumArtworkBytes = 1024 * 1024;
constexpr DWORD kWorkerRetryMs = 5000;
constexpr int64_t kSeekStep100ns = 10LL * 1000LL * 1000LL;
constexpr double kMinimumSafeWidth = 120.0;
constexpr double kAutomaticLaneReserve = 240.0;
constexpr double kDualMirrorLaneReserve = 204.0;
constexpr double kWidgetDragThreshold = 5.0;
constexpr wchar_t kPrimaryWidgetLeftValue[] = L"MediaPrimaryLeft";
constexpr wchar_t kSecondaryWidgetLeftValue[] = L"MediaSecondaryLeft";
constexpr wchar_t kStartNames[][24] = {
    L"StartButton", L"StartMenuButton", L"StartMenuLaunchButton",
    L"LaunchListButton"};

struct Settings {
    int minimumWidth = 232;
    int preferredWidth = 304;
    int maximumWidth = 336;
    int height = 50;
    bool showArtwork = true;
    bool showArtist = true;
    bool hideWithoutSession = false;
    bool smoothProgress = true;
    bool wideInsideCapsule = false;
};

struct MediaSnapshot {
    uint64_t sequence = 0;
    bool hasSession = false;
    bool playing = false;
    bool canPrevious = true;
    bool canToggle = true;
    bool canNext = true;
    bool canSeek = false;
    std::wstring title;
    std::wstring description;
    std::wstring source;
    std::shared_ptr<const std::vector<uint8_t>> artwork;
    int64_t start100ns = 0;
    int64_t position100ns = 0;
    int64_t end100ns = 0;
    uint64_t capturedTick = 0;
};

struct ButtonEventTokens {
    winrt::event_token pointerEntered{};
    winrt::event_token pointerExited{};
    winrt::event_token pointerPressed{};
    winrt::event_token pointerReleased{};
    winrt::event_token click{};
};

struct MediaMirrorSlot {
    HWND window = nullptr;
    winrt::weak_ref<FrameworkElement> taskbarFrame;
    Grid parent{nullptr};
    Grid widget{nullptr};
    FrameworkElement repeater{nullptr};
    double reservedMargin = 0.0;
    TextBlock title{nullptr};
    TextBlock status{nullptr};
};

Settings g_settings;
bool g_mediaEnabled = true;
bool g_leanMode = true;
bool g_highContrast = false;
bool g_reducedMotion = false;
bool g_manualLayout = false;
bool g_fullViewOnPrimary = false;
bool g_mirrorDetailed = true;
double g_widgetTextScale = 1.0;
uint8_t g_widgetBackgroundAlpha = 0x9C;
OpalControl::MonitorTarget g_monitorTarget = OpalControl::MonitorTarget::Secondary;
OpalControl::QuarantineState g_quarantine;
std::atomic<bool> g_unloading{false};
std::atomic<bool> g_taskbarViewHooked{false};
std::atomic<HWND> g_taskbarWindow{nullptr};
std::atomic<DWORD> g_taskbarThreadId{0};

std::mutex g_mediaMutex;
[[clang::no_destroy]] MediaSnapshot g_snapshot;
[[clang::no_destroy]] GlobalSystemMediaTransportControlsSessionManager g_manager{nullptr};
[[clang::no_destroy]] GlobalSystemMediaTransportControlsSession g_session{nullptr};
winrt::event_token g_managerCurrentToken{};
winrt::event_token g_managerSessionsToken{};
winrt::event_token g_mediaPropertiesToken{};
winrt::event_token g_playbackToken{};
winrt::event_token g_timelineToken{};
bool g_managerSubscribed = false;
bool g_sessionSubscribed = false;
HANDLE g_workerThread = nullptr;
HANDLE g_workerStop = nullptr;
HANDLE g_workerRefresh = nullptr;
std::atomic<int> g_sessionCycleRequest{0};
std::atomic<bool> g_followSystemSession{false};
bool g_manualSessionSelection = false;

[[clang::no_destroy]] Grid g_parent{nullptr};
[[clang::no_destroy]] winrt::weak_ref<FrameworkElement> g_taskbarFrame;
[[clang::no_destroy]] Grid g_widget{nullptr};
[[clang::no_destroy]] Border g_shell{nullptr};
[[clang::no_destroy]] Border g_artworkHost{nullptr};
[[clang::no_destroy]] ColumnDefinition g_artworkColumn{nullptr};
[[clang::no_destroy]] ColumnDefinition g_controlColumn{nullptr};
[[clang::no_destroy]] Image g_artwork{nullptr};
[[clang::no_destroy]] StackPanel g_identityPanel{nullptr};
[[clang::no_destroy]] TextBlock g_title{nullptr};
[[clang::no_destroy]] TextBlock g_artist{nullptr};
[[clang::no_destroy]] Button g_seekBackward{nullptr};
[[clang::no_destroy]] Button g_previous{nullptr};
[[clang::no_destroy]] Button g_playPause{nullptr};
[[clang::no_destroy]] Button g_next{nullptr};
[[clang::no_destroy]] Button g_seekForward{nullptr};
[[clang::no_destroy]] Grid g_progressHost{nullptr};
[[clang::no_destroy]] XamlRectangle g_progressFill{nullptr};
[[clang::no_destroy]] ScaleTransform g_progressScale{nullptr};
[[clang::no_destroy]] DispatcherTimer g_uiTimer{nullptr};
[[clang::no_destroy]] FrameworkElement g_taskItemsRepeater{nullptr};
[[clang::no_destroy]] std::vector<MediaMirrorSlot> g_mediaMirrors;
winrt::event_token g_rootSizeToken{};
winrt::event_token g_taskItemsSizeToken{};
winrt::event_token g_uiTimerTickToken{};
winrt::event_token g_widgetPointerPressedToken{};
winrt::event_token g_widgetPointerMovedToken{};
winrt::event_token g_widgetPointerReleasedToken{};
winrt::event_token g_widgetPointerCaptureLostToken{};
winrt::event_token g_artworkWheelToken{};
winrt::event_token g_shellTappedToken{};
winrt::event_token g_shellWheelToken{};
winrt::event_token g_progressPointerPressedToken{};
winrt::event_token g_progressPointerMovedToken{};
winrt::event_token g_progressPointerReleasedToken{};
winrt::event_token g_progressPointerCaptureLostToken{};
std::array<ButtonEventTokens, 5> g_buttonEventTokens{};
[[clang::no_destroy]] std::optional<
    std::list<FrameworkElement::Loaded_revoker>> g_frameLoadedRevokers{
        std::in_place};
[[clang::no_destroy]] MediaSnapshot g_uiSnapshot;
uint64_t g_uiSequence = 0;
uint64_t g_artworkHash = 0;
uint64_t g_noSessionSinceTick = 0;
int g_uiIntervalMs = 0;
bool g_scrubbing = false;
uint32_t g_scrubPointerId = 0;
double g_scrubRatio = 0.0;
bool g_hasLayoutSpace = true;
double g_reservedMargin = 0.0;
bool g_userLeftLoaded = false;
int g_userLeft = -1;
bool g_widgetDragPending = false;
bool g_widgetDragging = false;
uint32_t g_widgetDragPointerId = 0;
double g_widgetDragStartX = 0.0;
double g_widgetDragStartLeft = 0.0;

void UpdateWidgetVisibility();

using TaskbarFrame_Constructor_t = void*(WINAPI*)(void*);
TaskbarFrame_Constructor_t TaskbarFrame_Constructor_Original = nullptr;

using CTaskBand_GetTaskbarHost_t = void*(WINAPI*)(void*, void*);
using TaskbarHost_FrameHeight_t = int(WINAPI*)(void*);
using RefCountBase_Decref_t = void(WINAPI*)(void*);
CTaskBand_GetTaskbarHost_t CTaskBand_GetTaskbarHost_Original = nullptr;
CTaskBand_GetTaskbarHost_t CSecondaryTaskBand_GetTaskbarHost_Original = nullptr;
TaskbarHost_FrameHeight_t TaskbarHost_FrameHeight_Original = nullptr;
RefCountBase_Decref_t RefCountBase_Decref_Original = nullptr;
void* CTaskBand_ITaskListWndSite_vftable = nullptr;
void* CSecondaryTaskBand_ITaskListWndSite_vftable = nullptr;

using LoadLibraryExW_t = decltype(&LoadLibraryExW);
LoadLibraryExW_t LoadLibraryExW_Original = nullptr;

winrt::Windows::UI::Color MakeColor(uint8_t alpha, uint8_t red, uint8_t green,
                                    uint8_t blue) {
    return winrt::Windows::UI::Color{alpha, red, green, blue};
}

SolidColorBrush Brush(uint8_t alpha, uint8_t value) {
    return SolidColorBrush(MakeColor(alpha, value, value, value));
}

void LoadSettings() {
    g_mediaEnabled = Wh_GetIntSetting(L"media.mediaEnabled") != 0;
    g_leanMode = Wh_GetIntSetting(L"everyday.leanMode") != 0;
    g_manualLayout = OpalControl::ReadStringSetting(
        L"everyday.layoutMode", L"automatic") == L"custom";
    g_fullViewOnPrimary = OpalControl::ReadStringSetting(
        L"screens.mediaFullDisplay", L"primary") == L"primary";
    g_mirrorDetailed = OpalControl::ReadStringSetting(
        L"screens.mirrorStyle", L"detailed") != L"minimal";
    const std::wstring textSize = OpalControl::ReadStringSetting(
        L"everyday.widgetTextSize", L"standard");
    g_widgetTextScale = textSize == L"small" ? 0.90
                          : (textSize == L"large" ? 1.15 : 1.0);
    const std::wstring background = OpalControl::ReadStringSetting(
        L"everyday.widgetBackgroundStrength", L"glass");
    g_widgetBackgroundAlpha = background == L"subtle" ? 0x70
                                : (background == L"strong" ? 0xD0 : 0x9C);
    g_monitorTarget = OpalControl::ReadMonitorSetting(
        L"screens.mediaMonitor", OpalControl::MonitorTarget::Primary);
    g_highContrast = OpalControl::HighContrast();
    g_reducedMotion = OpalControl::ReducedMotion();
    g_settings.minimumWidth = std::clamp(Wh_GetIntSetting(L"minimumWidth"), 150, 240);
    g_settings.preferredWidth = std::clamp(Wh_GetIntSetting(L"preferredWidth"),
                                           g_settings.minimumWidth, 320);
    g_settings.maximumWidth = std::clamp(Wh_GetIntSetting(L"maximumWidth"),
                                         g_settings.preferredWidth, 360);
    g_settings.height = std::clamp(Wh_GetIntSetting(L"height"), 44, 58);
    g_settings.showArtwork = Wh_GetIntSetting(L"media.showArtwork") != 0;
    g_settings.showArtist = Wh_GetIntSetting(L"media.showArtist") != 0;
    g_settings.hideWithoutSession =
        Wh_GetIntSetting(L"media.hideWithoutSession") != 0;
    g_settings.smoothProgress =
        Wh_GetIntSetting(L"media.smoothProgress") != 0;
    const std::wstring size = OpalControl::ReadStringSetting(
        L"media.mediaSize", L"standard");
    if (size == L"compact") {
        g_settings.minimumWidth = 176;
        g_settings.preferredWidth = 216;
        g_settings.maximumWidth = 248;
        g_settings.wideInsideCapsule = false;
    } else if (size == L"wide") {
        // Wide adds detail inside the capsule. The reserved taskbar lane stays
        // at the standard width so one click cannot shove the rest of the bar.
        g_settings.minimumWidth = 232;
        g_settings.preferredWidth = 304;
        g_settings.maximumWidth = 336;
        g_settings.wideInsideCapsule = true;
    } else {
        g_settings.minimumWidth = 232;
        g_settings.preferredWidth = 304;
        g_settings.maximumWidth = 336;
        g_settings.wideInsideCapsule = false;
    }
    if (Wh_GetIntSetting(L"advanced.repair.resetWidgetPositions") != 0) {
        Wh_SetIntValue(kPrimaryWidgetLeftValue, -1);
        Wh_SetIntValue(kSecondaryWidgetLeftValue, -1);
        Wh_SetIntValue(L"ForceCanonicalLayout", 1);
        g_manualLayout = false;
        g_settings.minimumWidth = 232;
        g_settings.preferredWidth = 304;
        g_settings.maximumWidth = 336;
        g_settings.hideWithoutSession = false;
        OpalControl::ResetPackageQuarantine(L"media");
    }
    g_userLeftLoaded = false;
}

std::wstring Lowercase(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](wchar_t character) {
                       return static_cast<wchar_t>(std::towlower(character));
                   });
    return value;
}

bool ContainsAny(const std::wstring& value,
                 std::initializer_list<PCWSTR> needles) {
    for (PCWSTR needle : needles) {
        if (value.find(needle) != std::wstring::npos) {
            return true;
        }
    }
    return false;
}

std::wstring FriendlySourceName(const std::wstring& source) {
    std::wstring lower = Lowercase(source);
    if (lower.find(L"spotify") != std::wstring::npos) return L"Spotify";
    if (lower.find(L"youtube") != std::wstring::npos) return L"YouTube";
    if (lower.find(L"vlc") != std::wstring::npos) return L"VLC";
    if (lower.find(L"musicbee") != std::wstring::npos) return L"MusicBee";
    if (lower.find(L"foobar") != std::wstring::npos) return L"foobar2000";
    if (lower.find(L"audible") != std::wstring::npos) return L"Audible";
    if (lower.find(L"chrome") != std::wstring::npos) return L"Chrome";
    if (lower.find(L"msedge") != std::wstring::npos ||
        lower.find(L"microsoftedge") != std::wstring::npos) {
        return L"Microsoft Edge";
    }
    if (lower.find(L"firefox") != std::wstring::npos) return L"Firefox";
    if (lower.find(L"zunemusic") != std::wstring::npos ||
        lower.find(L"media player") != std::wstring::npos) {
        return L"Media Player";
    }

    std::wstring compact = source;
    if (size_t bang = compact.find(L'!'); bang != std::wstring::npos) {
        compact.resize(bang);
    }
    if (size_t slash = compact.find_last_of(L"\\/"); slash != std::wstring::npos) {
        compact.erase(0, slash + 1);
    }
    if (size_t underscore = compact.find(L'_'); underscore != std::wstring::npos) {
        compact.resize(underscore);
    }
    if (size_t dot = compact.find_last_of(L'.'); dot != std::wstring::npos &&
        dot + 1 < compact.size()) {
        compact.erase(0, dot + 1);
    }
    return compact;
}

std::wstring BuildSmartDescription(
    const GlobalSystemMediaTransportControlsSessionMediaProperties& properties,
    const std::wstring& source) {
    std::wstring title = properties.Title().c_str();
    std::wstring artist = properties.Artist().c_str();
    std::wstring subtitle = properties.Subtitle().c_str();
    std::wstring album = properties.AlbumTitle().c_str();
    std::wstring searchable = Lowercase(title + L" " + artist + L" " +
                                        subtitle + L" " + album + L" " + source);

    std::wstring kind;
    if (ContainsAny(searchable, {L"podcast", L"episode"})) {
        kind = L"Podcast";
    } else if (ContainsAny(searchable, {L"audiobook", L"audio book"})) {
        kind = L"Audiobook";
    } else if (ContainsAny(searchable, {L"live stream", L"livestream", L" live "})) {
        kind = L"Live";
    } else {
        auto playbackType = properties.PlaybackType();
        if (playbackType) {
            switch (playbackType.Value()) {
                case winrt::Windows::Media::MediaPlaybackType::Music:
                    kind = L"Music";
                    break;
                case winrt::Windows::Media::MediaPlaybackType::Video:
                    kind = L"Video";
                    break;
                case winrt::Windows::Media::MediaPlaybackType::Image:
                    kind = L"Image";
                    break;
                default:
                    break;
            }
        }
    }
    if (kind.empty()) {
        kind = artist.empty() ? L"Media" : L"Music";
    }

    std::wstring identity = !artist.empty() ? artist
                             : !subtitle.empty() ? subtitle
                             : album;
    std::wstring sourceName = FriendlySourceName(source);
    std::wstring description = identity;
    if (!kind.empty() && Lowercase(identity) != Lowercase(kind)) {
        if (!description.empty()) description += L"  ·  ";
        description += kind;
    }
    if (!sourceName.empty() &&
        Lowercase(description).find(Lowercase(sourceName)) == std::wstring::npos) {
        if (!description.empty()) description += L"  ·  ";
        description += sourceName;
    }
    return description.empty() ? L"Now playing" : description;
}

uint64_t HashBytes(const std::vector<uint8_t>& bytes) {
    uint64_t hash = 1469598103934665603ULL;
    for (uint8_t value : bytes) {
        hash ^= value;
        hash *= 1099511628211ULL;
    }
    return hash;
}

WriteableBitmap DecodeArtwork(const std::vector<uint8_t>& bytes) {
    if (bytes.empty()) {
        return nullptr;
    }

    ComPtr<IStream> stream;
    stream.Attach(SHCreateMemStream(bytes.data(), static_cast<UINT>(bytes.size())));
    if (!stream) {
        return nullptr;
    }

    ComPtr<IWICImagingFactory> factory;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                                CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)))) {
        return nullptr;
    }

    ComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(factory->CreateDecoderFromStream(stream.Get(), nullptr,
                                                WICDecodeMetadataCacheOnLoad,
                                                &decoder))) {
        return nullptr;
    }

    ComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(decoder->GetFrame(0, &frame))) {
        return nullptr;
    }

    UINT sourceWidth = 0;
    UINT sourceHeight = 0;
    if (FAILED(frame->GetSize(&sourceWidth, &sourceHeight)) || !sourceWidth ||
        !sourceHeight) {
        return nullptr;
    }

    // Decode to the real display cell instead of a fixed source-size ceiling.
    // WIC scales during decode, so a 4K cover never becomes a full-size bitmap
    // inside Explorer only to be shrunk by XAML a moment later.
    UINT dpi = g_taskbarWindow ? GetDpiForWindow(g_taskbarWindow) : 96;
    UINT targetPixels = std::clamp<UINT>(
        static_cast<UINT>(std::lround(46.0 * dpi / 96.0)), 46, 192);
    double scale = std::min(1.0, static_cast<double>(targetPixels) /
                                    std::max(sourceWidth, sourceHeight));
    UINT width = std::max(1U, static_cast<UINT>(std::lround(sourceWidth * scale)));
    UINT height = std::max(1U, static_cast<UINT>(std::lround(sourceHeight * scale)));

    IWICBitmapSource* source = frame.Get();
    ComPtr<IWICBitmapScaler> scaler;
    if (width != sourceWidth || height != sourceHeight) {
        if (FAILED(factory->CreateBitmapScaler(&scaler)) ||
            FAILED(scaler->Initialize(frame.Get(), width, height,
                                      WICBitmapInterpolationModeFant))) {
            return nullptr;
        }
        source = scaler.Get();
    }

    ComPtr<IWICFormatConverter> converter;
    if (FAILED(factory->CreateFormatConverter(&converter)) ||
        FAILED(converter->Initialize(source, GUID_WICPixelFormat32bppPBGRA,
                                     WICBitmapDitherTypeNone, nullptr, 0,
                                     WICBitmapPaletteTypeCustom))) {
        return nullptr;
    }

    WriteableBitmap bitmap(static_cast<int>(width), static_cast<int>(height));
    auto buffer = bitmap.PixelBuffer();
    auto access = buffer.as<Windows::Storage::Streams::IBufferByteAccess>();
    BYTE* pixels = nullptr;
    if (FAILED(access->Buffer(&pixels)) || !pixels) {
        return nullptr;
    }
    UINT stride = width * 4;
    UINT byteCount = stride * height;
    if (FAILED(converter->CopyPixels(nullptr, stride, byteCount, pixels))) {
        return nullptr;
    }
    buffer.Length(byteCount);
    bitmap.Invalidate();
    return bitmap;
}

std::vector<uint8_t> ReadArtwork(
    const winrt::Windows::Storage::Streams::IRandomAccessStreamReference& reference) {
    std::vector<uint8_t> bytes;
    if (!reference) {
        return bytes;
    }
    try {
        auto stream = reference.OpenReadAsync().get();
        uint64_t size64 = stream.Size();
        if (!size64 || size64 > kMaximumArtworkBytes) {
            return bytes;
        }
        auto buffer = winrt::Windows::Storage::Streams::Buffer(
            static_cast<uint32_t>(size64));
        auto result = stream.ReadAsync(
            buffer, static_cast<uint32_t>(size64),
            winrt::Windows::Storage::Streams::InputStreamOptions::None).get();
        bytes.resize(result.Length());
        auto reader = winrt::Windows::Storage::Streams::DataReader::FromBuffer(result);
        reader.ReadBytes(bytes);
        // The provider stream has no work after this bounded copy. Releasing it
        // here prevents artwork sources from staying live for the XAML lifetime.
        stream.Close();
    } catch (...) {
        bytes.clear();
    }
    return bytes;
}

void SignalWorker() {
    HANDLE refresh = g_workerRefresh;
    if (refresh) {
        SetEvent(refresh);
    }
}

void RequestSessionCycle(int step) {
    if (!step) {
        return;
    }
    g_sessionCycleRequest.fetch_add(step);
    SignalWorker();
}

void AdjustSystemVolume(int wheelDelta) {
    if (!wheelDelta) {
        return;
    }
    WORD key = wheelDelta > 0 ? VK_VOLUME_UP : VK_VOLUME_DOWN;
    int presses = std::clamp(std::abs(wheelDelta) / WHEEL_DELTA, 1, 4);
    for (int i = 0; i < presses; i++) {
        INPUT input[2]{};
        input[0].type = INPUT_KEYBOARD;
        input[0].ki.wVk = key;
        input[1].type = INPUT_KEYBOARD;
        input[1].ki.wVk = key;
        input[1].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(ARRAYSIZE(input), input, sizeof(INPUT));
    }
}

void LaunchSourceApp() {
    std::wstring source = g_uiSnapshot.source;
    if (source.empty()) {
        return;
    }
    if (source.find(L'!') != std::wstring::npos) {
        source = L"shell:AppsFolder\\" + source;
    }
    ShellExecuteW(nullptr, L"open", source.c_str(), nullptr, nullptr,
                  SW_SHOWNORMAL);
}

bool IsMediaControlSurface(DependencyObject source) {
    try {
        DependencyObject current = source;
        while (current && current != g_shell) {
            if (current.try_as<Button>() || current == g_progressHost) {
                return true;
            }
            current = VisualTreeHelper::GetParent(current);
        }
    } catch (...) {
        // Never turn an uncertain routed event into an accidental app launch.
        return true;
    }
    return false;
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

// TransformToVisual(parent) reports a sibling's position from the parent's OUTER
// edge, while a child's Margin.Left is measured from the padded content edge.
// Opal styles the taskbar RootGrid with horizontal padding, so an anchor read
// from Start or the performance widget must drop that padding before it becomes
// a margin - otherwise the widget lands padding-width too far right and
// overlaps the Start button (14 DIP of padding gave a 6 DIP overlap).
double ParentPaddingLeft(Grid const& parent) {
    try { return parent ? parent.Padding().Left : 0.0; } catch (...) { return 0.0; }
}

double ParentContentWidth(Grid const& parent) {
    try {
        if (!parent) return 0.0;
        const auto padding = parent.Padding();
        return parent.ActualWidth() - padding.Left - padding.Right;
    } catch (...) {
        return 0.0;
    }
}

double ClampWidgetLeft(double left) {
    double rootWidth = ParentContentWidth(g_parent);
    double widgetWidth = g_widget ? g_widget.ActualWidth() : 0.0;
    if (!std::isfinite(widgetWidth) || widgetWidth <= 0.0) {
        widgetWidth = g_widget ? g_widget.Width()
                               : static_cast<double>(g_settings.preferredWidth);
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

void AttachWidgetDragHandlers(Grid root) {
    g_widgetPointerPressedToken = root.PointerPressed(
        [](IInspectable const&, PointerRoutedEventArgs const& args) {
            if (!g_manualLayout) return;
            if (!g_parent || !g_widget) {
                return;
            }
            auto point = args.GetCurrentPoint(g_parent);
            auto source = args.OriginalSource().try_as<DependencyObject>();
            if (!point.Properties().IsLeftButtonPressed() ||
                IsMediaControlSurface(source)) {
                return;
            }
            g_widgetDragPending = true;
            g_widgetDragging = false;
            g_widgetDragPointerId = args.Pointer().PointerId();
            g_widgetDragStartX = point.Position().X;
            g_widgetDragStartLeft = g_widget.Margin().Left;
        });
    g_widgetPointerMovedToken = root.PointerMoved(
        [](IInspectable const&, PointerRoutedEventArgs const& args) {
            if (!g_manualLayout) return;
            if (!g_widgetDragPending || !g_parent || !g_widget ||
                args.Pointer().PointerId() != g_widgetDragPointerId) {
                return;
            }
            auto point = args.GetCurrentPoint(g_parent);
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
                g_widget.CapturePointer(args.Pointer());
            }
            ApplyUserPosition(g_widgetDragStartLeft + delta);
            UpdateWidgetVisibility();
            args.Handled(true);
        });
    g_widgetPointerReleasedToken = root.PointerReleased(
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
    g_widgetPointerCaptureLostToken = root.PointerCaptureLost(
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

void UnsubscribeSession() {
    if (!g_sessionSubscribed || !g_session) {
        g_sessionSubscribed = false;
        return;
    }
    try { g_session.MediaPropertiesChanged(g_mediaPropertiesToken); } catch (...) {}
    try { g_session.PlaybackInfoChanged(g_playbackToken); } catch (...) {}
    try { g_session.TimelinePropertiesChanged(g_timelineToken); } catch (...) {}
    g_sessionSubscribed = false;
}

void BindSession(GlobalSystemMediaTransportControlsSession session) {
    if (g_session == session) {
        return;
    }
    UnsubscribeSession();
    {
        std::lock_guard lock(g_mediaMutex);
        g_session = session;
    }
    if (!g_session) {
        return;
    }
    g_mediaPropertiesToken = g_session.MediaPropertiesChanged(
        [](auto&&, auto&&) { SignalWorker(); });
    g_playbackToken = g_session.PlaybackInfoChanged(
        [](auto&&, auto&&) { SignalWorker(); });
    g_timelineToken = g_session.TimelinePropertiesChanged(
        [](auto&&, auto&&) { SignalWorker(); });
    g_sessionSubscribed = true;
}

void ReleaseSessionManager() {
    UnsubscribeSession();
    if (g_managerSubscribed && g_manager) {
        try { g_manager.CurrentSessionChanged(g_managerCurrentToken); } catch (...) {}
        try { g_manager.SessionsChanged(g_managerSessionsToken); } catch (...) {}
    }
    g_managerSubscribed = false;
    {
        std::lock_guard lock(g_mediaMutex);
        g_session = nullptr;
    }
    g_manager = nullptr;
}

bool EnsureSessionManager() {
    if (g_manager) {
        return true;
    }
    try {
        g_manager = GlobalSystemMediaTransportControlsSessionManager::
            RequestAsync().get();
        g_managerCurrentToken = g_manager.CurrentSessionChanged(
            [](auto&&, auto&&) {
                g_followSystemSession = true;
                SignalWorker();
            });
        g_managerSessionsToken = g_manager.SessionsChanged(
            [](auto&&, auto&&) { SignalWorker(); });
        g_managerSubscribed = true;
        return true;
    } catch (...) {
        Wh_Log(L"Opal Media session manager unavailable: %08X",
               winrt::to_hresult());
        ReleaseSessionManager();
        return false;
    }
}

GlobalSystemMediaTransportControlsSession PickSession() {
    if (!g_manager) {
        return nullptr;
    }
    auto sessions = g_manager.GetSessions();
    uint32_t count = sessions.Size();
    if (!count) {
        g_manualSessionSelection = false;
        return nullptr;
    }

    GlobalSystemMediaTransportControlsSession bound{nullptr};
    {
        std::lock_guard lock(g_mediaMutex);
        bound = g_session;
    }
    auto findIndex = [&](GlobalSystemMediaTransportControlsSession const& target,
                         uint32_t& index) {
        if (!target) {
            return false;
        }
        for (uint32_t i = 0; i < count; i++) {
            if (sessions.GetAt(i) == target) {
                index = i;
                return true;
            }
        }
        return false;
    };

    if (g_followSystemSession.exchange(false)) {
        g_manualSessionSelection = false;
    }
    int requestedStep = g_sessionCycleRequest.exchange(0);
    if (requestedStep) {
        uint32_t currentIndex = 0;
        findIndex(bound, currentIndex);
        int normalized = requestedStep % static_cast<int>(count);
        int nextIndex = (static_cast<int>(currentIndex) + normalized +
                         static_cast<int>(count)) % static_cast<int>(count);
        g_manualSessionSelection = true;
        return sessions.GetAt(static_cast<uint32_t>(nextIndex));
    }

    uint32_t boundIndex = 0;
    if (g_manualSessionSelection && findIndex(bound, boundIndex)) {
        return sessions.GetAt(boundIndex);
    }
    g_manualSessionSelection = false;

    auto current = g_manager.GetCurrentSession();
    if (current) {
        try {
            auto playback = current.GetPlaybackInfo();
            if (playback && playback.PlaybackStatus() ==
                GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing) {
                return current;
            }
        } catch (...) {}
    }
    for (auto const& session : sessions) {
        try {
            auto playback = session.GetPlaybackInfo();
            if (playback && playback.PlaybackStatus() ==
                GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing) {
                return session;
            }
        } catch (...) {}
    }
    return current ? current : sessions.GetAt(0);
}

void PublishEmptySnapshot() {
    std::lock_guard lock(g_mediaMutex);
    MediaSnapshot empty;
    empty.sequence = g_snapshot.sequence + 1;
    empty.capturedTick = GetTickCount64();
    g_snapshot = std::move(empty);
}

bool RefreshMediaSnapshot() {
    try {
        auto session = PickSession();
        BindSession(session);
        if (!session) {
            PublishEmptySnapshot();
            return true;
        }

        auto properties = session.TryGetMediaPropertiesAsync().get();
        auto playback = session.GetPlaybackInfo();
        auto timeline = session.GetTimelineProperties();
        // A media session can disappear between enumeration and this refresh.
        // C++/WinRT represents that race as a null interface; calling through
        // it raises a native access violation before the catch below can run.
        if (!playback) {
            return false;
        }

        MediaSnapshot next;
        next.hasSession = true;
        next.playing = playback.PlaybackStatus() ==
                       GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing;
        next.title = properties.Title().c_str();
        next.source = session.SourceAppUserModelId().c_str();
        next.description = BuildSmartDescription(properties, next.source);
        if (next.title.empty()) {
            next.title = next.source.empty() ? L"Now Playing" : next.source;
        }
        auto controls = playback.Controls();
        if (controls) {
            next.canPrevious = controls.IsPreviousEnabled();
            next.canToggle = controls.IsPlayPauseToggleEnabled();
            next.canNext = controls.IsNextEnabled();
        }
        next.start100ns = timeline.StartTime().count();
        next.position100ns = timeline.Position().count();
        next.end100ns = timeline.EndTime().count();
        next.canSeek = next.end100ns > next.start100ns;
        next.capturedTick = GetTickCount64();
        {
            std::lock_guard lock(g_mediaMutex);
            if (g_snapshot.title == next.title &&
                g_snapshot.description == next.description &&
                g_snapshot.source == next.source) {
                next.artwork = g_snapshot.artwork;
            }
        }
        if (!next.artwork) {
            auto artwork = ReadArtwork(properties.Thumbnail());
            if (!artwork.empty()) {
                next.artwork = std::make_shared<const std::vector<uint8_t>>(
                    std::move(artwork));
            }
        }

        std::lock_guard lock(g_mediaMutex);
        next.sequence = g_snapshot.sequence + 1;
        g_snapshot = std::move(next);
        return true;
    } catch (...) {
        Wh_Log(L"Opal Media snapshot refresh failed: %08X",
               winrt::to_hresult());
        PublishEmptySnapshot();
        return false;
    }
}

DWORD WINAPI MediaWorker(void*) {
    winrt::init_apartment(winrt::apartment_type::multi_threaded);
    HANDLE waits[] = {g_workerStop, g_workerRefresh};
    while (WaitForSingleObject(g_workerStop, 0) != WAIT_OBJECT_0) {
        bool refreshed = EnsureSessionManager() && RefreshMediaSnapshot();
        if (!refreshed) {
            ReleaseSessionManager();
        }
        DWORD result = WaitForMultipleObjects(
            2, waits, FALSE, refreshed ? INFINITE : kWorkerRetryMs);
        if (result == WAIT_OBJECT_0) {
            break;
        }
    }

    ReleaseSessionManager();
    winrt::uninit_apartment();
    return 0;
}

bool StartWorker() {
    g_workerStop = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    g_workerRefresh = CreateEventW(nullptr, FALSE, TRUE, nullptr);
    if (!g_workerStop || !g_workerRefresh) {
        if (g_workerStop) {
            CloseHandle(g_workerStop);
            g_workerStop = nullptr;
        }
        if (g_workerRefresh) {
            CloseHandle(g_workerRefresh);
            g_workerRefresh = nullptr;
        }
        return false;
    }
    g_workerThread = CreateThread(nullptr, 0, MediaWorker, nullptr, 0, nullptr);
    if (!g_workerThread) {
        CloseHandle(g_workerStop);
        CloseHandle(g_workerRefresh);
        g_workerStop = nullptr;
        g_workerRefresh = nullptr;
        return false;
    }
    return true;
}

void StopWorker() {
    if (g_workerStop) {
        SetEvent(g_workerStop);
    }
    if (g_workerThread) {
        // The DLL must not unload while the worker is still executing one of
        // its callbacks. Media providers normally return immediately; waiting
        // for the real thread exit is safer than closing a timed-out handle and
        // leaving code running in an unloaded module.
        WaitForSingleObject(g_workerThread, INFINITE);
        CloseHandle(g_workerThread);
        g_workerThread = nullptr;
    }
    if (g_workerStop) {
        CloseHandle(g_workerStop);
        g_workerStop = nullptr;
    }
    if (g_workerRefresh) {
        CloseHandle(g_workerRefresh);
        g_workerRefresh = nullptr;
    }
}

enum class MediaAction { SeekBackward, Previous, Toggle, Next, SeekForward };

int64_t CurrentUiPosition() {
    int64_t position = g_uiSnapshot.position100ns;
    if (g_uiSnapshot.playing) {
        uint64_t elapsedMs = GetTickCount64() - g_uiSnapshot.capturedTick;
        position += static_cast<int64_t>(elapsedMs) * 10000;
    }
    return std::clamp(position, g_uiSnapshot.start100ns,
                      g_uiSnapshot.end100ns);
}

void SeekToPosition(int64_t position100ns) {
    if (!g_uiSnapshot.canSeek ||
        g_uiSnapshot.end100ns <= g_uiSnapshot.start100ns) {
        return;
    }
    GlobalSystemMediaTransportControlsSession session{nullptr};
    {
        std::lock_guard lock(g_mediaMutex);
        session = g_session;
    }
    if (!session) {
        return;
    }
    position100ns = std::clamp(position100ns, g_uiSnapshot.start100ns,
                               g_uiSnapshot.end100ns);
    try {
        auto operation = session.TryChangePlaybackPositionAsync(position100ns);
        (void)operation;
    } catch (...) {}
}

void SeekToRatio(double ratio) {
    ratio = std::clamp(ratio, 0.0, 1.0);
    long double duration = static_cast<long double>(
        g_uiSnapshot.end100ns - g_uiSnapshot.start100ns);
    int64_t position = g_uiSnapshot.start100ns +
                       static_cast<int64_t>(duration * ratio);
    SeekToPosition(position);
}

void SetProgressRatio(double ratio) {
    ratio = std::clamp(ratio, 0.0, 1.0);
    if (g_progressScale &&
        std::abs(g_progressScale.ScaleX() - ratio) >= 0.0005) {
        // RenderTransform updates are handled by composition and don't force a
        // taskbar measure/arrange pass like changing Rectangle.Width did.
        g_progressScale.ScaleX(ratio);
    }
}

void UpdateScrubVisual(PointerRoutedEventArgs const& args) {
    if (!g_progressHost) {
        return;
    }
    double width = g_progressHost.ActualWidth();
    if (width <= 0) {
        return;
    }
    auto point = args.GetCurrentPoint(g_progressHost);
    g_scrubRatio = std::clamp(static_cast<double>(point.Position().X) / width,
                              0.0, 1.0);
    SetProgressRatio(g_scrubRatio);
}

void RunMediaAction(MediaAction action) {
    GlobalSystemMediaTransportControlsSession session{nullptr};
    {
        std::lock_guard lock(g_mediaMutex);
        session = g_session;
    }
    if (!session) {
        return;
    }
    try {
        if (action == MediaAction::SeekBackward) {
            SeekToPosition(CurrentUiPosition() - kSeekStep100ns);
        } else if (action == MediaAction::SeekForward) {
            SeekToPosition(CurrentUiPosition() + kSeekStep100ns);
        } else if (action == MediaAction::Previous) {
            auto operation = session.TrySkipPreviousAsync();
            (void)operation;
        } else if (action == MediaAction::Next) {
            auto operation = session.TrySkipNextAsync();
            (void)operation;
        } else {
            auto operation = session.TryTogglePlayPauseAsync();
            (void)operation;
        }
    } catch (...) {}
}

Button MakeButton(PCWSTR glyph, PCWSTR accessibleName, MediaAction action,
                  size_t slot, bool emphasized = false) {
    Button button;
    button.Width(28);
    button.Height(28);
    button.Padding(Thickness{0, 0, 0, 0});
    button.Margin(Thickness{1, 0, 1, 0});
    button.CornerRadius(CornerRadius{14, 14, 14, 14});
    button.Background(emphasized ? Brush(0x20, 0xF5) : Brush(0x00, 0x00));
    button.BorderThickness(Thickness{0, 0, 0, 0});
    button.UseSystemFocusVisuals(false);
    winrt::Windows::UI::Xaml::Automation::AutomationProperties::SetName(
        button, accessibleName);
    ToolTipService::SetToolTip(button, winrt::box_value(accessibleName));
    button.Opacity(emphasized ? 1.0 : 0.82);

    ScaleTransform hoverScale;
    hoverScale.ScaleX(1.0);
    hoverScale.ScaleY(1.0);
    button.RenderTransformOrigin(winrt::Windows::Foundation::Point{0.5f, 0.5f});
    button.RenderTransform(hoverScale);

    TextBlock icon;
    icon.Text(glyph);
    icon.FontFamily(FontFamily(L"Segoe Fluent Icons"));
    icon.FontSize((emphasized ? 14.0 : 13.0) * g_widgetTextScale);
    icon.Foreground(Brush(0xF5, 0xF5));
    icon.HorizontalAlignment(HorizontalAlignment::Center);
    icon.VerticalAlignment(VerticalAlignment::Center);
    button.Content(icon);

    auto& tokens = g_buttonEventTokens.at(slot);
    tokens.pointerEntered = button.PointerEntered(
        [hoverScale, emphasized](IInspectable const& sender,
                                 PointerRoutedEventArgs const&) {
        if (auto target = sender.try_as<Button>()) {
            target.Opacity(1.0);
            target.Background(emphasized ? Brush(0x30, 0xF5)
                                         : Brush(0x12, 0xF5));
            if (!g_reducedMotion) {
                hoverScale.ScaleX(1.08);
                hoverScale.ScaleY(1.08);
            }
        }
    });
    tokens.pointerExited = button.PointerExited(
        [hoverScale, emphasized](IInspectable const& sender,
                                 PointerRoutedEventArgs const&) {
        if (auto target = sender.try_as<Button>()) {
            target.Background(emphasized ? Brush(0x20, 0xF5)
                                         : Brush(0x00, 0x00));
            target.Opacity(emphasized ? 1.0 : 0.82);
            hoverScale.ScaleX(1.0);
            hoverScale.ScaleY(1.0);
        }
    });
    tokens.pointerPressed = button.PointerPressed(
        [hoverScale](IInspectable const&, PointerRoutedEventArgs const&) {
        if (!g_reducedMotion) {
            hoverScale.ScaleX(0.92);
            hoverScale.ScaleY(0.92);
        }
    });
    tokens.pointerReleased = button.PointerReleased(
        [hoverScale](IInspectable const&, PointerRoutedEventArgs const&) {
        if (!g_reducedMotion) {
            hoverScale.ScaleX(1.08);
            hoverScale.ScaleY(1.08);
        }
    });
    tokens.click = button.Click([action](IInspectable const&, RoutedEventArgs const&) {
        RunMediaAction(action);
    });
    return button;
}

Grid BuildWidget() {
    Grid root;
    root.Name(kWidgetName);
    root.Height(g_settings.height);
    root.Width(g_settings.preferredWidth);
    root.UseLayoutRounding(true);
    root.VerticalAlignment(VerticalAlignment::Center);
    root.HorizontalAlignment(HorizontalAlignment::Left);
    AttachWidgetDragHandlers(root);

    g_shell = Border();
    g_shell.Name(L"OpalMediaGlass");
    // Readable translucent fallback until Opal's XAML tap applies the shared
    // compositor frost used by Clock, tray, and System Info.
    g_shell.Background(SolidColorBrush(g_highContrast
        ? MakeColor(0xFF, 0x00, 0x00, 0x00)
        : MakeColor(g_widgetBackgroundAlpha, 0x16, 0x18, 0x1D)));
    g_shell.BorderBrush(g_highContrast ? Brush(0xFF, 0xFF) : Brush(0x00, 0x00));
    g_shell.BorderThickness(g_highContrast ? Thickness{1, 1, 1, 1}
                                           : Thickness{0, 0, 0, 0});
    g_shell.CornerRadius(CornerRadius{25, 25, 25, 25});
    g_shell.Padding(Thickness{3, 2, 3, 2});

    Grid content;
    g_artworkColumn = ColumnDefinition();
    g_artworkColumn.Width(GridLength{50, GridUnitType::Pixel});
    ColumnDefinition textColumn;
    textColumn.Width(GridLength{1, GridUnitType::Star});
    g_controlColumn = ColumnDefinition();
    g_controlColumn.Width(GridLength{150, GridUnitType::Pixel});
    content.ColumnDefinitions().Append(g_artworkColumn);
    content.ColumnDefinitions().Append(textColumn);
    content.ColumnDefinitions().Append(g_controlColumn);

    g_artworkHost = Border();
    g_artworkHost.Width(46);
    g_artworkHost.Height(46);
    g_artworkHost.CornerRadius(CornerRadius{10, 10, 10, 10});
    g_artworkHost.Background(Brush(0x1A, 0xFF));
    g_artworkHost.BorderBrush(Brush(0x00, 0x00));
    g_artworkHost.BorderThickness(Thickness{0, 0, 0, 0});
    g_artworkHost.VerticalAlignment(VerticalAlignment::Center);
    winrt::Windows::UI::Xaml::Automation::AutomationProperties::SetName(
        g_artworkHost, L"Album artwork");
    winrt::Windows::UI::Xaml::Automation::AutomationProperties::SetHelpText(
        g_artworkHost,
        L"Click to open the media app. Scroll to switch media sessions.");
    g_artworkWheelToken = g_artworkHost.PointerWheelChanged(
        [](IInspectable const&, PointerRoutedEventArgs const& args) {
            int delta = args.GetCurrentPoint(g_artworkHost)
                            .Properties().MouseWheelDelta();
            RequestSessionCycle(delta > 0 ? -1 : 1);
            args.Handled(true);
        });
    g_artwork = Image();
    g_artwork.Stretch(Stretch::UniformToFill);
    g_artworkHost.Child(g_artwork);
    Grid::SetColumn(g_artworkHost, 0);
    content.Children().Append(g_artworkHost);

    g_identityPanel = StackPanel();
    g_identityPanel.VerticalAlignment(VerticalAlignment::Center);
    g_identityPanel.Margin(Thickness{6, 0, 5, 0});
    g_identityPanel.Spacing(-1);
    g_title = TextBlock();
    g_title.FontFamily(FontFamily(L"Segoe UI Variable Text"));
    g_title.FontWeight(winrt::Windows::UI::Text::FontWeights::SemiBold());
    g_title.FontSize(14.0 * g_widgetTextScale);
    g_title.Foreground(Brush(0xF5, 0xF5));
    g_title.TextTrimming(TextTrimming::CharacterEllipsis);
    g_title.TextWrapping(TextWrapping::NoWrap);
    g_artist = TextBlock();
    g_artist.FontFamily(FontFamily(L"Segoe UI Variable Text"));
    g_artist.FontWeight(winrt::Windows::UI::Text::FontWeight{400});
    g_artist.FontSize(11.0 * g_widgetTextScale);
    g_artist.Foreground(SolidColorBrush(MakeColor(0xC0, 0xD6, 0xD6, 0xD8)));
    g_artist.TextTrimming(TextTrimming::CharacterEllipsis);
    g_artist.TextWrapping(TextWrapping::NoWrap);
    g_identityPanel.Children().Append(g_title);
    g_identityPanel.Children().Append(g_artist);
    Grid::SetColumn(g_identityPanel, 1);
    content.Children().Append(g_identityPanel);

    StackPanel controls;
    controls.Orientation(Orientation::Horizontal);
    controls.HorizontalAlignment(HorizontalAlignment::Right);
    controls.VerticalAlignment(VerticalAlignment::Center);
    g_seekBackward = MakeButton(L"\uE7A7", L"Back 10 seconds",
                                MediaAction::SeekBackward, 0);
    g_previous = MakeButton(L"\uE892", L"Previous track",
                            MediaAction::Previous, 1);
    g_playPause = MakeButton(L"\uE768", L"Play", MediaAction::Toggle, 2, true);
    g_next = MakeButton(L"\uE893", L"Next track", MediaAction::Next, 3);
    g_seekForward = MakeButton(L"\uE72A", L"Forward 10 seconds",
                               MediaAction::SeekForward, 4);
    controls.Children().Append(g_seekBackward);
    controls.Children().Append(g_previous);
    controls.Children().Append(g_playPause);
    controls.Children().Append(g_next);
    controls.Children().Append(g_seekForward);
    Grid::SetColumn(controls, 2);
    content.Children().Append(controls);
    g_shell.Child(content);
    winrt::Windows::UI::Xaml::Automation::AutomationProperties::SetName(
        g_shell, L"Now playing");
    winrt::Windows::UI::Xaml::Automation::AutomationProperties::SetHelpText(
        g_shell,
        L"Click outside the controls to open the media app. Drag that surface to move this capsule. Scroll to change system volume. Hold Shift while scrolling to seek 10 seconds.");
    g_shellTappedToken = g_shell.Tapped(
        [](IInspectable const&, TappedRoutedEventArgs const& args) {
            auto source = args.OriginalSource().try_as<DependencyObject>();
            if (!IsMediaControlSurface(source)) {
                LaunchSourceApp();
                args.Handled(true);
            }
        });
    g_shellWheelToken = g_shell.PointerWheelChanged(
        [](IInspectable const&, PointerRoutedEventArgs const& args) {
            int delta = args.GetCurrentPoint(g_shell)
                            .Properties().MouseWheelDelta();
            if (GetKeyState(VK_SHIFT) & 0x8000) {
                SeekToPosition(CurrentUiPosition() +
                               (delta > 0 ? kSeekStep100ns
                                          : -kSeekStep100ns));
            } else {
                AdjustSystemVolume(delta);
            }
            args.Handled(true);
        });
    root.Children().Append(g_shell);

    g_progressHost = Grid();
    g_progressHost.Height(8);
    g_progressHost.Margin(Thickness{10, 0, 10, 1});
    g_progressHost.VerticalAlignment(VerticalAlignment::Bottom);
    g_progressHost.Background(Brush(0x00, 0x00));
    g_progressHost.IsHitTestVisible(true);
    winrt::Windows::UI::Xaml::Automation::AutomationProperties::SetName(
        g_progressHost, L"Playback timeline");
    winrt::Windows::UI::Xaml::Automation::AutomationProperties::SetHelpText(
        g_progressHost, L"Drag left or right to seek.");
    ToolTipService::SetToolTip(g_progressHost,
                               winrt::box_value(L"Drag to seek"));
    g_progressPointerPressedToken = g_progressHost.PointerPressed(
        [](IInspectable const&, PointerRoutedEventArgs const& args) {
            auto point = args.GetCurrentPoint(g_progressHost);
            if (!g_uiSnapshot.canSeek ||
                !point.Properties().IsLeftButtonPressed()) {
                return;
            }
            g_scrubbing = true;
            g_scrubPointerId = args.Pointer().PointerId();
            g_progressHost.CapturePointer(args.Pointer());
            UpdateScrubVisual(args);
            args.Handled(true);
        });
    g_progressPointerMovedToken = g_progressHost.PointerMoved(
        [](IInspectable const&, PointerRoutedEventArgs const& args) {
            if (!g_scrubbing ||
                args.Pointer().PointerId() != g_scrubPointerId) {
                return;
            }
            UpdateScrubVisual(args);
            args.Handled(true);
        });
    g_progressPointerReleasedToken = g_progressHost.PointerReleased(
        [](IInspectable const&, PointerRoutedEventArgs const& args) {
            if (!g_scrubbing ||
                args.Pointer().PointerId() != g_scrubPointerId) {
                return;
            }
            UpdateScrubVisual(args);
            g_scrubbing = false;
            g_progressHost.ReleasePointerCapture(args.Pointer());
            SeekToRatio(g_scrubRatio);
            args.Handled(true);
        });
    g_progressPointerCaptureLostToken = g_progressHost.PointerCaptureLost(
        [](IInspectable const&, PointerRoutedEventArgs const&) {
            g_scrubbing = false;
            g_scrubPointerId = 0;
        });
    XamlRectangle track;
    track.Height(2);
    track.RadiusX(1);
    track.RadiusY(1);
    track.HorizontalAlignment(HorizontalAlignment::Stretch);
    track.Fill(Brush(0x2A, 0xFF));
    g_progressFill = XamlRectangle();
    g_progressFill.Height(2);
    g_progressFill.RadiusX(1);
    g_progressFill.RadiusY(1);
    g_progressFill.HorizontalAlignment(HorizontalAlignment::Stretch);
    g_progressFill.Fill(Brush(0xE8, 0xF5));
    g_progressScale = ScaleTransform();
    g_progressScale.ScaleX(0.0);
    g_progressScale.ScaleY(1.0);
    g_progressFill.RenderTransformOrigin(
        winrt::Windows::Foundation::Point{0.0F, 0.5F});
    g_progressFill.RenderTransform(g_progressScale);
    g_progressHost.Children().Append(track);
    g_progressHost.Children().Append(g_progressFill);
    Canvas::SetZIndex(g_progressHost, 3);
    root.Children().Append(g_progressHost);
    return root;
}

FrameworkElement FindChildRecursive(DependencyObject root,
                                    const std::function<bool(FrameworkElement)>& predicate) {
    if (!root) {
        return nullptr;
    }
    int count = VisualTreeHelper::GetChildrenCount(root);
    for (int i = 0; i < count; i++) {
        auto childObject = VisualTreeHelper::GetChild(root, i);
        auto child = childObject.try_as<FrameworkElement>();
        if (child && predicate(child)) {
            return child;
        }
        if (auto nested = FindChildRecursive(childObject, predicate)) {
            return nested;
        }
    }
    return nullptr;
}

FrameworkElement FindNamedChild(DependencyObject root, PCWSTR name) {
    return FindChildRecursive(root, [name](FrameworkElement element) {
        return element.Name() == name;
    });
}

FrameworkElement FindSystemInfoLane(DependencyObject root) {
    for (PCWSTR name : {kSystemInfoWidgetName, kSystemInfoMirrorWidgetName,
                        kLegacySystemInfoWidgetName,
                        kLegacySystemInfoMirrorWidgetName}) {
        if (auto found = FindNamedChild(root, name)) return found;
    }
    return nullptr;
}

FrameworkElement FindStartButton(DependencyObject root) {
    return FindChildRecursive(root, [](FrameworkElement element) {
        std::wstring name = element.Name().c_str();
        for (auto const& candidate : kStartNames) {
            if (name == candidate) {
                return true;
            }
        }
        try {
            auto id = winrt::Windows::UI::Xaml::Automation::
                AutomationProperties::GetAutomationId(element);
            return id == L"StartButton" || id == L"Start";
        } catch (...) {
            return false;
        }
    });
}

void ApplyMediaReservedSpace(bool visible) {
    if (!g_taskItemsRepeater) {
        return;
    }

    // Reserve the requested lane independently of the measured capsule. Using
    // its temporarily compressed width here fed the next layout measurement
    // back into the reservation and stranded Media at artwork/play-only width
    // after startup. Native taskbar layout owns the remaining app space.
    double mediaLaneReserve = std::clamp(
        static_cast<double>(g_settings.preferredWidth),
        static_cast<double>(g_settings.minimumWidth),
        static_cast<double>(g_settings.maximumWidth)) + 16.0;
    if (g_settings.wideInsideCapsule) {
        mediaLaneReserve = std::min(mediaLaneReserve, kAutomaticLaneReserve + 8.0);
    }
    double next = visible && g_userLeft < 0
        ? mediaLaneReserve
        : 0.0;
    if (std::abs(next - g_reservedMargin) < 0.5) {
        return;
    }

    // Each addon owns only its own contribution to the shared task-item
    // margin. Media can release its lane in the exact event that hides it,
    // while System Info keeps its independent reservation intact.
    Thickness margin = g_taskItemsRepeater.Margin();
    margin.Left -= g_reservedMargin;
    g_reservedMargin = next;
    margin.Left += g_reservedMargin;
    g_taskItemsRepeater.Margin(margin);
}

void ApplyDensity(double width);

void UpdateWidgetVisibility() {
    if (!g_widget) {
        return;
    }
    bool sessionVisible = g_uiSnapshot.hasSession ||
                          !g_settings.hideWithoutSession;
    bool visible = sessionVisible;
    if (!g_hasLayoutSpace) {
        ApplyDensity(kMinimumSafeWidth);
        visible = sessionVisible;
    }
    Visibility desired = visible ? Visibility::Visible
                                 : Visibility::Collapsed;
    if (g_widget.Visibility() != desired) {
        g_widget.Visibility(desired);
    }
    if (visible && !g_uiSnapshot.hasSession && g_title) {
        if (g_title.Text() != L"Nothing playing") {
            g_title.Text(L"Nothing playing");
        }
        if (g_artist) {
            g_artist.Text(L"");
        }
    }
    ApplyMediaReservedSpace(sessionVisible);
}

void ApplyDensity(double width) {
    if (!g_artist || !g_seekBackward || !g_previous || !g_next ||
        !g_seekForward) {
        return;
    }
    bool transport = width >= (g_settings.wideInsideCapsule ? 160.0 : 176.0);
    bool showIdentity = width >= (g_settings.wideInsideCapsule ? 176.0 : 220.0);
    // The secondary line now carries the artist plus a compact smart media
    // description. Keep it visible whenever the identity lane itself fits.
    bool showArtist = showIdentity;
    // Artwork is the compact identity. The coordinated 232-DIP lane keeps a
    // large cover, a bold title, and the expected previous/play/next controls.
    bool artwork = g_settings.showArtwork && width >= kMinimumSafeWidth;
    if (g_artworkHost && g_artworkColumn) {
        g_artworkHost.Visibility(artwork ? Visibility::Visible
                                        : Visibility::Collapsed);
        g_artworkColumn.Width(GridLength{artwork ? 50.0 : 0.0,
                                         GridUnitType::Pixel});
    }
    if (g_controlColumn) {
        g_controlColumn.Width(GridLength{transport ? 90.0 : 30.0,
                                         GridUnitType::Pixel});
    }
    if (g_identityPanel) {
        g_identityPanel.Visibility(showIdentity ? Visibility::Visible
                                                : Visibility::Collapsed);
    }
    g_artist.Visibility(g_settings.showArtist && showArtist
                            ? Visibility::Visible
                            : Visibility::Collapsed);
    // Directional buttons always mean track navigation. Ten-second seeking is
    // still available through Shift-scroll and the precise scrubber, so the
    // compact glyphs never change meaning as the capsule grows.
    g_seekBackward.Visibility(Visibility::Collapsed);
    g_previous.Visibility(transport ? Visibility::Visible
                                    : Visibility::Collapsed);
    g_next.Visibility(transport ? Visibility::Visible
                                : Visibility::Collapsed);
    g_seekForward.Visibility(Visibility::Collapsed);
    if (g_progressHost) {
        // The rail belongs to track metadata, not the artwork or controls.
        // Keep its hit target entirely inside the center identity lane.
        g_progressHost.Margin(Thickness{artwork ? 58.0 : 10.0, 0,
                                         transport ? 100.0 : 40.0, 1});
    }
}

double DesiredWidth() {
    size_t length = g_uiSnapshot.title.size();
    double extra = std::min<size_t>(length, 22) * 2.4;
    return std::clamp(static_cast<double>(g_settings.minimumWidth) + extra,
                      static_cast<double>(g_settings.minimumWidth),
                      static_cast<double>(g_settings.maximumWidth));
}

void PositionWidget() {
    if (!g_parent || !g_widget) {
        return;
    }
    try {
        LoadUserPosition();
        double desired = std::max(static_cast<double>(g_settings.preferredWidth),
                                  DesiredWidth());
        if (g_userLeft >= 0) {
            double rootWidth = g_parent.ActualWidth();
            if (std::isfinite(rootWidth) && rootWidth > 0.0) {
                if (rootWidth < kMinimumSafeWidth) {
                    g_hasLayoutSpace = false;
                    desired = kMinimumSafeWidth;
                    if (std::abs(g_widget.Width() - desired) > 0.5) {
                        g_widget.Width(desired);
                    }
                    ApplyDensity(desired);
                    UpdateWidgetVisibility();
                    return;
                }
                desired = std::clamp(desired, kMinimumSafeWidth,
                                     std::min(rootWidth,
                                              static_cast<double>(g_settings.maximumWidth)));
            }
            g_hasLayoutSpace = true;
            if (std::abs(g_widget.Width() - desired) > 0.5) {
                g_widget.Width(desired);
            }
            ApplyUserPosition(static_cast<double>(g_userLeft));
            ApplyDensity(desired);
            UpdateWidgetVisibility();
            return;
        }

        auto start = FindStartButton(g_parent);
        const double contentInset = ParentPaddingLeft(g_parent);
        double zoneLeft = 8.0;
        auto systemInfo = FindSystemInfoLane(g_parent);
        if (systemInfo && systemInfo != g_widget &&
            systemInfo.Visibility() == Visibility::Visible) {
            auto point = systemInfo.TransformToVisual(g_parent).TransformPoint({0, 0});
            double width = systemInfo.ActualWidth();
            if (!std::isfinite(width) || width < 0.0) {
                width = 0.0;
            }
            zoneLeft = std::max(zoneLeft,
                                static_cast<double>(point.X) - contentInset + width + 8.0);
        }
        double left = zoneLeft;
        if (start) {
            auto point = start.TransformToVisual(g_parent).TransformPoint({0, 0});
            point.X -= static_cast<float>(contentInset);
            double available = std::max(
                0.0, static_cast<double>(point.X) - zoneLeft - 8.0);
            if (available < kMinimumSafeWidth) {
                g_hasLayoutSpace = false;
                desired = kMinimumSafeWidth;
                if (std::abs(g_widget.Width() - desired) > 0.5) {
                    g_widget.Width(desired);
                }
                ApplyDensity(desired);
                UpdateWidgetVisibility();
                return;
            }
            desired = std::min(desired, available);
            desired = std::min(desired,
                               static_cast<double>(g_settings.maximumWidth));
            desired = std::max(desired, kMinimumSafeWidth);
            left = std::max(zoneLeft,
                            static_cast<double>(point.X) - desired - 8.0);
        }
        g_hasLayoutSpace = true;
        if (std::abs(g_widget.Width() - desired) > 0.5) {
            g_widget.Width(desired);
        }
        auto margin = g_widget.Margin();
        if (std::abs(margin.Left - left) > 0.5) {
            g_widget.Margin(Thickness{left, 0, 0, 0});
        }
        ApplyDensity(desired);
        UpdateWidgetVisibility();
    } catch (...) {}
}

void UpdateArtwork(const std::vector<uint8_t>& bytes) {
    uint64_t hash = HashBytes(bytes);
    if (hash == g_artworkHash) {
        return;
    }
    g_artworkHash = hash;
    if (!g_artwork) {
        return;
    }
    try {
        auto bitmap = DecodeArtwork(bytes);
        g_artwork.Source(bitmap);
        g_artwork.Opacity(bitmap ? 1.0 : 0.0);
    } catch (...) {
        g_artwork.Source(nullptr);
        g_artwork.Opacity(0.0);
    }
}

int DesiredUiIntervalMs() {
    if (!g_uiSnapshot.hasSession) {
        return kIdleUiIntervalMs;
    }
    if (!g_settings.smoothProgress || !g_uiSnapshot.playing) {
        return kPausedUiIntervalMs;
    }
    return g_leanMode ? kLeanPlayingUiIntervalMs : kPlayingUiIntervalMs;
}

void ApplyUiTimerInterval(int intervalMs) {
    if (!g_uiTimer || g_uiIntervalMs == intervalMs) {
        return;
    }
    g_uiIntervalMs = intervalMs;
    g_uiTimer.Interval(winrt::Windows::Foundation::TimeSpan{
        std::chrono::milliseconds(intervalMs)});
}

void RemoveWidget();
bool AttachMediaVisuals(Grid root);
bool InjectWidget(FrameworkElement taskbarFrame);
bool InjectMediaMirror(FrameworkElement taskbarFrame, HWND window);
void UpdateMediaMirror();

void UpdateUiTick() {
    if (g_unloading) {
        return;
    }

    MediaSnapshot latest;
    bool hasUpdate = false;
    {
        std::lock_guard lock(g_mediaMutex);
        if (g_snapshot.sequence != g_uiSequence) {
            latest = g_snapshot;
            hasUpdate = true;
        }
    }
    if (hasUpdate) {
        g_uiSequence = latest.sequence;
        g_uiSnapshot = latest;
    }
    UpdateMediaMirror();

    if (!g_uiSnapshot.hasSession && g_leanMode &&
        g_settings.hideWithoutSession) {
        if (!g_noSessionSinceTick) {
            g_noSessionSinceTick = GetTickCount64();
        } else if (g_widget &&
                   GetTickCount64() - g_noSessionSinceTick >=
                       kLeanIdleReleaseMs) {
            auto frame = g_taskbarFrame.get();
            RemoveWidget();
            if (frame) {
                InjectWidget(frame);
            }
            return;
        }
    } else {
        g_noSessionSinceTick = 0;
    }

    if (!g_widget) {
        if ((!g_leanMode || g_uiSnapshot.hasSession ||
             !g_settings.hideWithoutSession) && g_parent) {
            if (!AttachMediaVisuals(g_parent)) {
                return;
            }
        } else {
            ApplyUiTimerInterval(kIdleUiIntervalMs);
            return;
        }
    }

    // In Both mode the Performance mirror can arrive after Media. Re-evaluate
    // the shared lane so the two lightweight views never overlap.
    if (g_monitorTarget == OpalControl::MonitorTarget::Both) {
        PositionWidget();
    }

    if (hasUpdate) {
        g_title.Text(latest.hasSession ? latest.title : L"Nothing playing");
        g_artist.Text(latest.hasSession ? latest.description : L"");
        if (auto glyph = g_playPause.Content().try_as<TextBlock>()) {
            glyph.Text(latest.playing ? L"\uE769" : L"\uE768");
        }
        g_previous.IsEnabled(latest.canPrevious);
        g_playPause.IsEnabled(latest.canToggle);
        g_next.IsEnabled(latest.canNext);
        g_seekBackward.IsEnabled(latest.canSeek);
        g_seekForward.IsEnabled(latest.canSeek);
        g_progressHost.IsHitTestVisible(latest.canSeek);
        winrt::Windows::UI::Xaml::Automation::AutomationProperties::SetName(
            g_playPause, latest.playing ? L"Pause" : L"Play");
        std::wstring accessibleText = latest.title;
        std::wstring secondary = latest.description;
        if (!secondary.empty()) {
            accessibleText += L", " + secondary;
        }
        winrt::Windows::UI::Xaml::Automation::AutomationProperties::SetName(
            g_shell, accessibleText.empty() ? L"Now playing" : accessibleText);
        ToolTipService::SetToolTip(
            g_shell, winrt::box_value(winrt::hstring(accessibleText)));
        if (latest.artwork) {
            UpdateArtwork(*latest.artwork);
        } else {
            UpdateArtwork({});
        }
        UpdateWidgetVisibility();
        PositionWidget();
    }
    ApplyUiTimerInterval(DesiredUiIntervalMs());

    if (!g_uiSnapshot.hasSession ||
        g_uiSnapshot.end100ns <= g_uiSnapshot.start100ns) {
        SetProgressRatio(0.0);
        return;
    }
    int64_t position = g_uiSnapshot.position100ns;
    if (g_settings.smoothProgress && g_uiSnapshot.playing) {
        position += static_cast<int64_t>(GetTickCount64() -
                                        g_uiSnapshot.capturedTick) * 10000;
    }
    position = std::clamp(position, g_uiSnapshot.start100ns,
                          g_uiSnapshot.end100ns);
    double ratio = std::clamp(
        static_cast<double>(position - g_uiSnapshot.start100ns) /
            static_cast<double>(g_uiSnapshot.end100ns -
                                g_uiSnapshot.start100ns),
        0.0, 1.0);
    if (!g_scrubbing) {
        SetProgressRatio(ratio);
    }
}

void StartUiTimer() {
    if (g_uiTimer) {
        g_uiTimer.Stop();
        if (g_uiTimerTickToken.value) {
            try { g_uiTimer.Tick(g_uiTimerTickToken); } catch (...) {}
            g_uiTimerTickToken = {};
        }
    }
    g_uiTimer = DispatcherTimer();
    g_uiIntervalMs = 0;
    ApplyUiTimerInterval(kIdleUiIntervalMs);
    g_uiTimerTickToken = g_uiTimer.Tick([](IInspectable const&, IInspectable const&) {
        UpdateUiTick();
    });
    g_uiTimer.Start();
}

void DetachFrameLoadedHandlers() {
    g_frameLoadedRevokers.reset();
    if (!g_unloading) {
        g_frameLoadedRevokers.emplace();
    }
}

void DetachWidgetEventHandlers() {
    if (g_widget) {
        try {
            if (g_widgetPointerPressedToken.value) {
                g_widget.PointerPressed(g_widgetPointerPressedToken);
            }
            if (g_widgetPointerMovedToken.value) {
                g_widget.PointerMoved(g_widgetPointerMovedToken);
            }
            if (g_widgetPointerReleasedToken.value) {
                g_widget.PointerReleased(g_widgetPointerReleasedToken);
            }
            if (g_widgetPointerCaptureLostToken.value) {
                g_widget.PointerCaptureLost(g_widgetPointerCaptureLostToken);
            }
        } catch (...) {}
    }
    g_widgetPointerPressedToken = {};
    g_widgetPointerMovedToken = {};
    g_widgetPointerReleasedToken = {};
    g_widgetPointerCaptureLostToken = {};

    std::array<Button, 5> buttons{
        g_seekBackward, g_previous, g_playPause, g_next, g_seekForward};
    for (size_t i = 0; i < buttons.size(); i++) {
        auto const& button = buttons[i];
        auto& tokens = g_buttonEventTokens[i];
        if (button) {
            try {
                if (tokens.pointerEntered.value) {
                    button.PointerEntered(tokens.pointerEntered);
                }
                if (tokens.pointerExited.value) {
                    button.PointerExited(tokens.pointerExited);
                }
                if (tokens.pointerPressed.value) {
                    button.PointerPressed(tokens.pointerPressed);
                }
                if (tokens.pointerReleased.value) {
                    button.PointerReleased(tokens.pointerReleased);
                }
                if (tokens.click.value) {
                    button.Click(tokens.click);
                }
            } catch (...) {}
        }
        tokens = {};
    }

    if (g_artworkHost && g_artworkWheelToken.value) {
        try { g_artworkHost.PointerWheelChanged(g_artworkWheelToken); } catch (...) {}
    }
    g_artworkWheelToken = {};
    if (g_shell) {
        try {
            if (g_shellTappedToken.value) {
                g_shell.Tapped(g_shellTappedToken);
            }
            if (g_shellWheelToken.value) {
                g_shell.PointerWheelChanged(g_shellWheelToken);
            }
        } catch (...) {}
    }
    g_shellTappedToken = {};
    g_shellWheelToken = {};

    if (g_progressHost) {
        try {
            if (g_progressPointerPressedToken.value) {
                g_progressHost.PointerPressed(g_progressPointerPressedToken);
            }
            if (g_progressPointerMovedToken.value) {
                g_progressHost.PointerMoved(g_progressPointerMovedToken);
            }
            if (g_progressPointerReleasedToken.value) {
                g_progressHost.PointerReleased(g_progressPointerReleasedToken);
            }
            if (g_progressPointerCaptureLostToken.value) {
                g_progressHost.PointerCaptureLost(
                    g_progressPointerCaptureLostToken);
            }
        } catch (...) {}
    }
    g_progressPointerPressedToken = {};
    g_progressPointerMovedToken = {};
    g_progressPointerReleasedToken = {};
    g_progressPointerCaptureLostToken = {};

    if (g_parent && g_rootSizeToken.value) {
        try { g_parent.SizeChanged(g_rootSizeToken); } catch (...) {}
    }
    if (g_taskItemsRepeater && g_taskItemsSizeToken.value) {
        try { g_taskItemsRepeater.SizeChanged(g_taskItemsSizeToken); } catch (...) {}
    }
    g_rootSizeToken = {};
    g_taskItemsSizeToken = {};
    DetachFrameLoadedHandlers();
}

void RemoveWidget() {
    if (g_uiTimer) {
        g_uiTimer.Stop();
        if (g_uiTimerTickToken.value) {
            try { g_uiTimer.Tick(g_uiTimerTickToken); } catch (...) {}
        }
        g_uiTimerTickToken = {};
    }
    DetachWidgetEventHandlers();
    if (g_uiTimer) {
        g_uiTimer = nullptr;
    }
    g_uiIntervalMs = 0;
    ApplyMediaReservedSpace(false);
    if (g_parent && g_widget) {
        try {
            uint32_t index = 0;
            if (g_parent.Children().IndexOf(g_widget, index)) {
                g_parent.Children().RemoveAt(index);
            }
        } catch (...) {}
    }
    g_parent = nullptr;
    g_widget = nullptr;
    g_shell = nullptr;
    g_artworkHost = nullptr;
    g_artworkColumn = nullptr;
    g_controlColumn = nullptr;
    g_artwork = nullptr;
    g_identityPanel = nullptr;
    g_title = nullptr;
    g_artist = nullptr;
    g_seekBackward = nullptr;
    g_previous = nullptr;
    g_playPause = nullptr;
    g_next = nullptr;
    g_seekForward = nullptr;
    g_progressHost = nullptr;
    g_progressFill = nullptr;
    g_progressScale = nullptr;
    g_taskItemsRepeater = nullptr;
    g_reservedMargin = 0.0;
    g_uiSequence = 0;
    g_uiSnapshot = {};
    g_artworkHash = 0;
    g_noSessionSinceTick = 0;
    g_scrubbing = false;
    g_scrubPointerId = 0;
    g_scrubRatio = 0.0;
    g_hasLayoutSpace = true;
    g_widgetDragPending = false;
    g_widgetDragging = false;
    g_widgetDragPointerId = 0;
}

bool AttachMediaVisuals(Grid root) {
    if (!root || g_widget) {
        return g_widget != nullptr;
    }
    g_widget = BuildWidget();
    // A remounted visual must consume the current snapshot even if playback
    // has not changed since the old tree was removed.
    g_uiSequence = UINT64_MAX;
    Grid::SetColumn(g_widget, 0);
    Grid::SetColumnSpan(g_widget,
                        std::max(1, static_cast<int>(root.ColumnDefinitions().Size())));
    Canvas::SetZIndex(g_widget, 10001);
    root.Children().Append(g_widget);
    g_rootSizeToken = root.SizeChanged([](IInspectable const&, SizeChangedEventArgs const&) {
        PositionWidget();
    });
    g_taskItemsRepeater = FindNamedChild(root, L"TaskbarFrameRepeater");
    if (g_taskItemsRepeater) {
        g_taskItemsSizeToken = g_taskItemsRepeater.SizeChanged(
            [](IInspectable const&, SizeChangedEventArgs const&) {
                PositionWidget();
            });
    }
    PositionWidget();
    return true;
}

void RemoveMediaMirrorSlot(MediaMirrorSlot& slot, bool keepFrame) {
    if (slot.repeater && slot.reservedMargin != 0.0) {
        try {
            auto margin = slot.repeater.Margin();
            margin.Left -= slot.reservedMargin;
            slot.repeater.Margin(margin);
        } catch (...) {}
    }
    slot.repeater = nullptr;
    slot.reservedMargin = 0.0;
    if (slot.parent && slot.widget) {
        try {
            uint32_t index = 0;
            if (slot.parent.Children().IndexOf(slot.widget, index)) {
                slot.parent.Children().RemoveAt(index);
            }
        } catch (...) {}
    }
    slot.parent = nullptr;
    slot.widget = nullptr;
    slot.title = nullptr;
    slot.status = nullptr;
    if (!keepFrame) {
        slot.taskbarFrame = {};
        slot.window = nullptr;
    }
}

void RemoveMediaMirrorVisuals(bool keepFrame = false) {
    for (auto& slot : g_mediaMirrors) RemoveMediaMirrorSlot(slot, keepFrame);
    if (!keepFrame) g_mediaMirrors.clear();
}

void PositionMediaMirror(MediaMirrorSlot& slot) {
    if (!slot.parent || !slot.widget) return;
    try {
        const double contentInset = ParentPaddingLeft(slot.parent);
        double zoneLeft = 8.0;
        double systemInfoLeft = -1.0;
        auto systemInfo = FindSystemInfoLane(slot.parent);
        if (systemInfo && systemInfo.Visibility() == Visibility::Visible) {
            auto point = systemInfo.TransformToVisual(slot.parent)
                             .TransformPoint({0, 0});
            double width = std::max(0.0, systemInfo.ActualWidth());
            systemInfoLeft = static_cast<double>(point.X) - contentInset;
            zoneLeft = std::max(zoneLeft, systemInfoLeft + width + 8.0);
        }
        double desired = g_mirrorDetailed ? 196.0 : 140.0;
        double left = zoneLeft;
        if (auto start = FindStartButton(slot.parent)) {
            auto point = start.TransformToVisual(slot.parent)
                             .TransformPoint({0, 0});
            point.X -= static_cast<float>(contentInset);
            double available = std::max(
                0.0, static_cast<double>(point.X) - zoneLeft - 8.0);
            if (available < 140.0) {
                double beforePerformance = systemInfoLeft - 16.0;
                if (systemInfoLeft < 0.0 || beforePerformance < 140.0) {
                    slot.widget.Visibility(Visibility::Collapsed);
                    return;
                }
                desired = std::min(desired, beforePerformance);
                left = std::max(8.0, systemInfoLeft - desired - 8.0);
                slot.widget.Width(desired);
                slot.widget.Margin(Thickness{left, 0, 0, 0});
                slot.widget.Visibility(Visibility::Visible);
                return;
            }
            desired = std::min(desired, available);
            left = std::max(zoneLeft,
                            static_cast<double>(point.X) - desired - 8.0);
        }
        slot.widget.Width(desired);
        slot.widget.Margin(Thickness{left, 0, 0, 0});
        slot.widget.Visibility(Visibility::Visible);
    } catch (...) {}
}

void PositionMediaMirror() {
    for (auto& slot : g_mediaMirrors) PositionMediaMirror(slot);
}

MediaMirrorSlot& MediaMirrorForWindow(HWND window) {
    for (auto& slot : g_mediaMirrors) {
        if (slot.window == window) return slot;
    }
    g_mediaMirrors.push_back({});
    g_mediaMirrors.back().window = window;
    return g_mediaMirrors.back();
}

bool InjectMediaMirror(FrameworkElement taskbarFrame, HWND window) {
    if (!taskbarFrame || g_monitorTarget != OpalControl::MonitorTarget::Both)
        return false;
    auto& slot = MediaMirrorForWindow(window);
    slot.taskbarFrame = taskbarFrame;
    slot.window = window;
    if (g_leanMode && !g_uiSnapshot.hasSession &&
        g_settings.hideWithoutSession) return true;
    auto root = FindNamedChild(taskbarFrame, L"RootGrid").try_as<Grid>();
    if (!root) return false;
    RemoveMediaMirrorSlot(slot, true);
    slot.repeater = FindNamedChild(root, L"TaskbarFrameRepeater");
    if (slot.repeater) {
        slot.reservedMargin = (g_mirrorDetailed ? 196.0 : 140.0) + 16.0;
        auto margin = slot.repeater.Margin();
        margin.Left += slot.reservedMargin;
        slot.repeater.Margin(margin);
    }
    for (uint32_t i = 0; i < root.Children().Size();) {
        auto child = root.Children().GetAt(i).try_as<FrameworkElement>();
        if (child && (child.Name() == kMirrorWidgetName ||
                      child.Name() == L"MaxwellOpalMediaMirror"))
            root.Children().RemoveAt(i);
        else
            i++;
    }

    Grid widget;
    widget.Name(kMirrorWidgetName);
    widget.Height(g_settings.height);
    widget.Width(196.0);
    widget.HorizontalAlignment(HorizontalAlignment::Left);
    widget.VerticalAlignment(VerticalAlignment::Center);
    widget.UseLayoutRounding(true);
    widget.IsHitTestVisible(true);
    Grid::SetColumn(widget, 0);
    Grid::SetColumnSpan(widget,
                        std::max(1, static_cast<int>(root.ColumnDefinitions().Size())));
    Canvas::SetZIndex(widget, 10001);

    Border shell;
    shell.CornerRadius(CornerRadius{25, 25, 25, 25});
    shell.Padding(Thickness{12, 4, 12, 4});
    shell.Background(SolidColorBrush(g_highContrast
        ? MakeColor(0xFF, 0x00, 0x00, 0x00)
        : MakeColor(g_widgetBackgroundAlpha, 0x16, 0x18, 0x1D)));
    shell.BorderBrush(g_highContrast ? Brush(0xFF, 0xFF)
                                     : Brush(0x16, 0xFF));
    shell.BorderThickness(Thickness{1, 1, 1, 1});

    StackPanel text;
    text.VerticalAlignment(VerticalAlignment::Center);
    text.Spacing(1.0);
    slot.title = TextBlock();
    slot.title.FontFamily(FontFamily(L"Segoe UI Variable Text"));
    slot.title.FontWeight(winrt::Windows::UI::Text::FontWeights::SemiBold());
    slot.title.FontSize(12.5 * g_widgetTextScale);
    slot.title.TextTrimming(TextTrimming::CharacterEllipsis);
    slot.title.TextWrapping(TextWrapping::NoWrap);
    slot.title.Foreground(Brush(0xF5, 0xF5));
    slot.status = TextBlock();
    slot.status.FontFamily(FontFamily(L"Segoe UI Variable Text"));
    slot.status.FontSize(10.5 * g_widgetTextScale);
    slot.status.TextTrimming(TextTrimming::CharacterEllipsis);
    slot.status.TextWrapping(TextWrapping::NoWrap);
    slot.status.Foreground(Brush(0xB8, 0xD1));
    slot.status.Visibility(g_mirrorDetailed ? Visibility::Visible
                                            : Visibility::Collapsed);
    text.Children().Append(slot.title);
    text.Children().Append(slot.status);
    shell.Child(text);
    widget.Children().Append(shell);
    widget.Tapped([](IInspectable const&, TappedRoutedEventArgs const& args) {
        RunMediaAction(MediaAction::Toggle);
        args.Handled(true);
    });
    widget.PointerWheelChanged(
        [](IInspectable const&, PointerRoutedEventArgs const& args) {
            int delta = args.GetCurrentPoint(nullptr)
                            .Properties().MouseWheelDelta();
            if (delta) AdjustSystemVolume(delta);
            args.Handled(true);
        });
    root.Children().Append(widget);
    slot.parent = root;
    slot.widget = widget;
    PositionMediaMirror(slot);
    return true;
}

void UpdateMediaMirror() {
    if (g_monitorTarget != OpalControl::MonitorTarget::Both) {
        RemoveMediaMirrorVisuals();
        return;
    }
    if (!g_uiSnapshot.hasSession && g_settings.hideWithoutSession) {
        RemoveMediaMirrorVisuals(true);
        return;
    }
    for (auto& slot : g_mediaMirrors) {
        if (!slot.widget) {
            if (auto frame = slot.taskbarFrame.get())
                InjectMediaMirror(frame, slot.window);
        }
        if (!slot.widget || !slot.title || !slot.status) continue;
        std::wstring title = g_uiSnapshot.title.empty()
                                 ? L"Media" : g_uiSnapshot.title;
        std::wstring status = g_uiSnapshot.playing ? L"Playing" : L"Paused";
        if (!g_uiSnapshot.description.empty())
            status += L"  ·  " + g_uiSnapshot.description;
        slot.title.Text(g_mirrorDetailed ? title : L"Media  ·  " + status);
        slot.status.Text(status);
        winrt::Windows::UI::Xaml::Automation::AutomationProperties::SetName(
            slot.widget, title + L", " + status);
        PositionMediaMirror(slot);
    }
}

bool InjectWidget(FrameworkElement taskbarFrame) {
    auto root = FindNamedChild(taskbarFrame, L"RootGrid").try_as<Grid>();
    if (!root) {
        return false;
    }
    RemoveWidget();
    for (uint32_t i = 0; i < root.Children().Size();) {
        auto child = root.Children().GetAt(i).try_as<FrameworkElement>();
        if (child && child.Name() == kWidgetName) {
            root.Children().RemoveAt(i);
        } else {
            i++;
        }
    }
    g_taskbarFrame = taskbarFrame;
    g_parent = root;
    g_taskItemsRepeater = FindNamedChild(root, L"TaskbarFrameRepeater");
    // From here an unclean Explorer exit is attributable to Media.
    OpalControl::MarkPackageSessionLive(L"media");
    if (!g_leanMode || g_uiSnapshot.hasSession) {
        if (!AttachMediaVisuals(root)) {
            return false;
        }
    }
    StartUiTimer();
    UpdateUiTick();
    return true;
}

bool IsReadable(const void* address, size_t size) {
    MEMORY_BASIC_INFORMATION info{};
    if (!VirtualQuery(address, &info, sizeof(info)) ||
        info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
        return false;
    }
    auto begin = reinterpret_cast<uintptr_t>(address);
    auto end = begin + size;
    auto regionEnd = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
    return end >= begin && end <= regionEnd;
}

XamlRoot GetTaskbarXamlRoot(HWND taskbarWindow) {
    WCHAR className[64]{};
    GetClassNameW(taskbarWindow, className, ARRAYSIZE(className));
    bool secondary = _wcsicmp(className, L"Shell_SecondaryTrayWnd") == 0;
    HWND taskBandWindow = secondary
        ? FindWindowExW(taskbarWindow, nullptr, L"WorkerW", nullptr)
        : reinterpret_cast<HWND>(GetPropW(taskbarWindow, L"TaskbandHWND"));
    if (!taskBandWindow) {
        return nullptr;
    }
    auto taskBand = reinterpret_cast<void*>(GetWindowLongPtrW(taskBandWindow, 0));
    if (!taskBand) {
        return nullptr;
    }
    void* expectedVtable = secondary
        ? CSecondaryTaskBand_ITaskListWndSite_vftable
        : CTaskBand_ITaskListWndSite_vftable;
    auto getHost = secondary
        ? CSecondaryTaskBand_GetTaskbarHost_Original
        : CTaskBand_GetTaskbarHost_Original;
    if (!expectedVtable || !getHost || !TaskbarHost_FrameHeight_Original ||
        !RefCountBase_Decref_Original) {
        return nullptr;
    }
    void* site = taskBand;
    for (int i = 0; i <= 20; i++) {
        if (!IsReadable(site, sizeof(void*))) {
            return nullptr;
        }
        if (*reinterpret_cast<void**>(site) == expectedVtable) {
            break;
        }
        if (i == 20) {
            return nullptr;
        }
        site = reinterpret_cast<void**>(site) + 1;
    }
    void* shared[2]{};
    getHost(site, shared);
    if (!shared[0] || !shared[1]) {
        if (shared[1]) {
            RefCountBase_Decref_Original(shared[1]);
        }
        return nullptr;
    }
    size_t offset = 0;
#if defined(_M_X64)
    auto code = reinterpret_cast<const BYTE*>(TaskbarHost_FrameHeight_Original);
    if (IsReadable(code, 8) && code[0] == 0x48 && code[1] == 0x83 &&
        code[2] == 0xEC && code[4] == 0x48 && code[5] == 0x83 &&
        code[6] == 0xC1 && code[7] <= 0x7F) {
        offset = code[7];
    }
#endif
    if (!offset || !IsReadable(static_cast<BYTE*>(shared[0]) + offset,
                               sizeof(void*))) {
        RefCountBase_Decref_Original(shared[1]);
        return nullptr;
    }
    auto unknown = *reinterpret_cast<::IUnknown**>(
        static_cast<BYTE*>(shared[0]) + offset);
    FrameworkElement element{nullptr};
    if (unknown) {
        unknown->QueryInterface(winrt::guid_of<FrameworkElement>(),
                                winrt::put_abi(element));
    }
    auto result = element ? element.XamlRoot() : nullptr;
    RefCountBase_Decref_Original(shared[1]);
    return result;
}

HWND FindPreferredTaskbarWindow() {
    return OpalControl::VisibleFullViewWindow(g_monitorTarget,
                                              !g_fullViewOnPrimary);
}

using WindowCallback = void (*)(void*);

bool RunFromWindowThread(HWND window, WindowCallback callback, void* context) {
    static const UINT message = RegisterWindowMessageW(
        L"Windhawk_RunFromWindowThread_Media_" WH_MOD_ID);
    struct CallbackContext {
        WindowCallback callback;
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
        [](int code, WPARAM wParam, LPARAM parameter) -> LRESULT {
            if (code == HC_ACTION) {
                auto data = reinterpret_cast<const CWPSTRUCT*>(parameter);
                if (data->message == message) {
                    auto call = reinterpret_cast<CallbackContext*>(data->lParam);
                    call->callback(call->context);
                    call->invoked = true;
                }
            }
            return CallNextHookEx(nullptr, code, wParam, parameter);
        }, nullptr, threadId);
    if (!hook) {
        return false;
    }
    CallbackContext call{callback, context, false};
    SendMessageW(window, message, 0, reinterpret_cast<LPARAM>(&call));
    UnhookWindowsHookEx(hook);
    return call.invoked;
}

struct MediaApplyContext {
    HWND window;
    bool mirror;
    bool repairOnly = false;
    bool attached = false;
};

void ApplyPreferredTaskbar(void* value) {
    auto* context = reinterpret_cast<MediaApplyContext*>(value);
    HWND window = context ? context->window : FindPreferredTaskbarWindow();
    if (!window) {
        return;
    }
    try {
        auto xamlRoot = GetTaskbarXamlRoot(window);
        auto content = xamlRoot ? xamlRoot.Content().try_as<FrameworkElement>()
                                : nullptr;
        auto frame = FindChildRecursive(content, [](FrameworkElement element) {
            return winrt::get_class_name(element) == L"Taskbar.TaskbarFrame";
        });
        if (frame) {
            auto root = FindNamedChild(frame, L"RootGrid").try_as<Grid>();
            if (context && context->repairOnly && root) {
                uint32_t index = 0;
                if (context->mirror) {
                    for (auto const& slot : g_mediaMirrors) {
                        if (slot.window == window && slot.parent == root &&
                            slot.widget && root.Children().IndexOf(slot.widget, index)) {
                            context->attached = true;
                            return;
                        }
                    }
                } else if (g_taskbarWindow.load() == window && g_parent == root &&
                           ((g_widget && root.Children().IndexOf(g_widget, index)) ||
                            (g_leanMode && !g_uiSnapshot.hasSession &&
                             g_settings.hideWithoutSession))) {
                    context->attached = true;
                    return;
                }
            }
            if (context && context->mirror) {
                context->attached = InjectMediaMirror(frame, window);
            } else {
                g_taskbarWindow = window;
                g_taskbarThreadId = GetWindowThreadProcessId(window, nullptr);
                g_userLeftLoaded = false;
                bool attached = InjectWidget(frame);
                if (context) context->attached = attached;
                if (!attached) {
                    g_taskbarWindow = nullptr;
                    g_taskbarThreadId = 0;
                }
            }
        }
    } catch (...) {
        Wh_Log(L"Opal Media injection failed: %08X", winrt::to_hresult());
    }
}

void RemoveMediaMirrorForWindow(void* value) {
    HWND window = static_cast<HWND>(value);
    for (auto it = g_mediaMirrors.begin(); it != g_mediaMirrors.end(); ++it) {
        if (it->window == window) {
            RemoveMediaMirrorSlot(*it, false);
            g_mediaMirrors.erase(it);
            return;
        }
    }
}

bool ApplyOnTaskbarThread(bool repairOnly = false) {
    HWND fullWindow = FindPreferredTaskbarWindow();
    if (!fullWindow) {
        OpalControl::PublishAttachmentProof(L"Media", nullptr, 1, 0);
        return false;
    }
    MediaApplyContext full{fullWindow, false, repairOnly};
    RunFromWindowThread(fullWindow, ApplyPreferredTaskbar, &full);
    bool attached = full.attached;
    unsigned attachedViews = full.attached ? 1 : 0;
    auto mirrors = OpalControl::OtherTaskbarWindows(g_monitorTarget, fullWindow);
    std::vector<HWND> leftover;
    for (auto const& slot : g_mediaMirrors) {
        bool wanted = false;
        for (HWND mirrorWindow : mirrors) {
            if (slot.window == mirrorWindow) { wanted = true; break; }
        }
        if (!wanted && slot.window) leftover.push_back(slot.window);
    }
    for (HWND window : leftover) {
        RunFromWindowThread(IsWindow(window) ? window : fullWindow, RemoveMediaMirrorForWindow,
                            reinterpret_cast<void*>(window));
    }
    for (HWND mirrorWindow : mirrors) {
        MediaApplyContext mirror{mirrorWindow, true, repairOnly};
        RunFromWindowThread(mirrorWindow, ApplyPreferredTaskbar, &mirror);
        attached = attached && mirror.attached;
        attachedViews += mirror.attached ? 1 : 0;
    }
    OpalControl::PublishAttachmentProof(L"Media", fullWindow,
        static_cast<unsigned>(1 + mirrors.size()), attachedViews);
    return attached;
}

void RemoveOnTaskbarThread(void*) {
    RemoveWidget();
    RemoveMediaMirrorVisuals();
    g_taskbarWindow = nullptr;
    g_taskbarThreadId = 0;
}

void* WINAPI TaskbarFrame_Constructor_Hook(void* self) {
    void* result = TaskbarFrame_Constructor_Original(self);
    if (g_unloading || !g_mediaEnabled || g_quarantine.quarantined) {
        return result;
    }
    FrameworkElement frame{nullptr};
    reinterpret_cast<::IUnknown**>(self)[1]->QueryInterface(
        winrt::guid_of<FrameworkElement>(), winrt::put_abi(frame));
    if (frame && g_frameLoadedRevokers) {
        g_frameLoadedRevokers->emplace_back();
        auto revoker = std::prev(g_frameLoadedRevokers->end());
        *revoker = frame.Loaded(
            winrt::auto_revoke_t{},
            [revoker](IInspectable const&, RoutedEventArgs const&) {
                if (!g_frameLoadedRevokers) return;
                g_frameLoadedRevokers->erase(revoker);
                if (!g_unloading) ApplyOnTaskbarThread();
            });
    }
    return result;
}

HMODULE GetTaskbarViewModule() {
    HMODULE module = GetModuleHandleW(L"Taskbar.View.dll");
    return module ? module : GetModuleHandleW(L"ExplorerExtensions.dll");
}

bool HookTaskbarView(HMODULE module) {
    WindhawkUtils::SYMBOL_HOOK hooks[] = {{
        {LR"(public: __cdecl winrt::Taskbar::implementation::TaskbarFrame::TaskbarFrame(void))"},
        &TaskbarFrame_Constructor_Original,
        TaskbarFrame_Constructor_Hook,
    }};
    return WindhawkUtils::HookSymbols(module, hooks, ARRAYSIZE(hooks));
}

HMODULE WINAPI LoadLibraryExW_Hook(LPCWSTR fileName, HANDLE file, DWORD flags) {
    HMODULE module = LoadLibraryExW_Original(fileName, file, flags);
    if (module && !g_taskbarViewHooked && GetTaskbarViewModule() == module &&
        !g_taskbarViewHooked.exchange(true)) {
        if (HookTaskbarView(module)) {
            Wh_ApplyHookOperations();
        }
    }
    return module;
}

bool HookTaskbarDll() {
    HMODULE module = LoadLibraryExW(L"taskbar.dll", nullptr,
                                    LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!module) {
        return false;
    }
    WindhawkUtils::SYMBOL_HOOK hooks[] = {
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
    return WindhawkUtils::HookSymbols(module, hooks, ARRAYSIZE(hooks));
}

void ApplyMediaControlChange(DWORD) {
    const bool wasEnabled = g_mediaEnabled && !g_quarantine.quarantined;
    if (Wh_GetIntSetting(
            L"advanced.repair.resetCrashQuarantine") != 0) {
        OpalControl::ResetPackageQuarantine(L"media");
        g_quarantine = OpalControl::BeginPackageSession(L"media");
    }
    LoadSettings();
    if (g_quarantine.quarantined) g_mediaEnabled = false;
    OpalControl::PublishRuntimeState(OpalControl::kMediaRuntimeActiveValue,
        OpalControl::kMediaRuntimePidValue, g_mediaEnabled, false,
        g_quarantine.reason.c_str(), g_quarantine.quarantined);
    if (!g_mediaEnabled) {
        StopWorker();
        HWND window = g_taskbarWindow ? g_taskbarWindow.load()
                                      : FindPreferredTaskbarWindow();
        if (window) RunFromWindowThread(window, RemoveOnTaskbarThread, nullptr);
        return;
    }
    if (!wasEnabled && !StartWorker()) {
        Wh_Log(L"Opal Media could not start after a live control change.");
        return;
    }
    SignalWorker();
    ApplyOnTaskbarThread();
}

}  // namespace

BOOL Wh_ModInit() {
    g_unloading = false;
    if (!g_frameLoadedRevokers) g_frameLoadedRevokers.emplace();
    LoadSettings();
    g_quarantine = OpalControl::BeginPackageSession(L"media");
    if (g_quarantine.quarantined) g_mediaEnabled = false;
    OpalControl::PublishRuntimeState(OpalControl::kMediaRuntimeActiveValue,
                                     OpalControl::kMediaRuntimePidValue,
                                     g_mediaEnabled, false,
                                     g_quarantine.reason.c_str(),
                                     g_quarantine.quarantined);
    if (!HookTaskbarDll()) {
        return FALSE;
    }
    if (HMODULE module = GetTaskbarViewModule()) {
        g_taskbarViewHooked = true;
        if (!HookTaskbarView(module)) {
            return FALSE;
        }
    } else {
        HMODULE kernel = GetModuleHandleW(L"kernelbase.dll");
        if (!kernel) {
            kernel = GetModuleHandleW(L"kernel32.dll");
        }
        auto load = kernel ? reinterpret_cast<LoadLibraryExW_t>(
                                 GetProcAddress(kernel, "LoadLibraryExW"))
                           : nullptr;
        if (!load || !WindhawkUtils::SetFunctionHook(
                         load, LoadLibraryExW_Hook, &LoadLibraryExW_Original)) {
            return FALSE;
        }
    }
    return !g_mediaEnabled || StartWorker() ? TRUE : FALSE;
}

void Wh_ModAfterInit() {
    if (!g_mediaEnabled) {
        return;
    }
    if (!g_taskbarViewHooked) {
        if (HMODULE module = GetTaskbarViewModule()) {
            if (!g_taskbarViewHooked.exchange(true) && HookTaskbarView(module)) {
                Wh_ApplyHookOperations();
            }
        }
    }
    ApplyOnTaskbarThread();
}

void Wh_ModSettingsChanged() {
    ApplyMediaControlChange(0);
}

void Wh_ModBeforeUninit() {
    g_unloading = true;
    StopWorker();
    HWND window = g_taskbarWindow;
    if (!window) {
        window = FindPreferredTaskbarWindow();
    }
    if (window) {
        RunFromWindowThread(window, RemoveOnTaskbarThread, nullptr);
    }
}

void Wh_ModUninit() {
    StopWorker();
    OpalControl::EndPackageSession(L"media");
}

#ifdef OPAL_UNIFIED_BUILD
// Late attach, driven by the shell's poll (LateAttachProc in the shell source).
//
// When Taskbar.View.dll arrives after Wh_ModInit through a loader path the
// LoadLibraryExW hook never sees, the TaskbarFrame constructor hook is installed
// too late for the frame that already exists, and Wh_ModAfterInit ran before
// Shell_TrayWnd existed - so nothing ever injected. Returns true once the
// widget's parent is known (in lean mode the visuals follow the media session)
// or when there is nothing to attach.
bool OpalMedia_EnsureAttached() {
    if (g_unloading || !g_mediaEnabled || g_quarantine.quarantined) return true;
    if (!g_taskbarViewHooked) {
        if (HMODULE module = GetTaskbarViewModule()) {
            if (!g_taskbarViewHooked.exchange(true) && HookTaskbarView(module)) {
                Wh_ApplyHookOperations();
            }
        }
    }
    // Resolve the current XAML root and every expected view on its UI thread.
    // A non-null parent from an old taskbar is not attachment evidence.
    return ApplyOnTaskbarThread(true);
}
#endif
