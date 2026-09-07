#pragma once

#include <cwchar>
#include <string>
#include <vector>

// Small helpers shared by Opal's internal Media and Performance components.
// Windhawk itself owns all user settings; this file contains no second control
// plane, message hook, polling watcher, or resident process.
namespace OpalControl {

inline constexpr wchar_t kRegistryPath[] = L"Software\\Maxwell\\Opal";
inline constexpr wchar_t kMediaRuntimeActiveValue[] = L"MediaRuntimeActive";
inline constexpr wchar_t kMediaRuntimePidValue[] = L"MediaRuntimePid";
inline constexpr wchar_t kPerformanceRuntimeActiveValue[] = L"PerformanceRuntimeActive";
inline constexpr wchar_t kPerformanceRuntimePidValue[] = L"PerformanceRuntimePid";

enum class MonitorTarget : DWORD { Primary = 0, Secondary = 1, Both = 2 };

struct QuarantineState {
    bool quarantined = false;
    DWORD crashCount = 0;
    std::wstring reason;
};

struct TaskbarWindows {
    HWND primary = nullptr;
    HWND secondary = nullptr;
    std::vector<HWND> secondaries;
};

inline TaskbarWindows CurrentProcessTaskbars() {
    TaskbarWindows result;
    EnumWindows(
        [](HWND window, LPARAM context) -> BOOL {
            auto* result = reinterpret_cast<TaskbarWindows*>(context);
            DWORD processId = 0;
            wchar_t className[64]{};
            if (!GetWindowThreadProcessId(window, &processId) ||
                processId != GetCurrentProcessId() ||
                !GetClassNameW(window, className, ARRAYSIZE(className))) {
                return TRUE;
            }
            if (_wcsicmp(className, L"Shell_TrayWnd") == 0) {
                result->primary = window;
            } else if (_wcsicmp(className, L"Shell_SecondaryTrayWnd") == 0) {
                result->secondaries.push_back(window);
                if (!result->secondary) result->secondary = window;
            }
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&result));
    return result;
}

inline bool MonitorCoveredByExclusiveFullscreen(HMONITOR monitor) {
    if (!monitor) return false;
    HWND foreground = GetForegroundWindow();
    if (!foreground || IsIconic(foreground)) return false;
    DWORD processId = 0;
    if (!GetWindowThreadProcessId(foreground, &processId) ||
        processId == GetCurrentProcessId()) {
        return false;
    }
    if (MonitorFromWindow(foreground, MONITOR_DEFAULTTONULL) != monitor) {
        return false;
    }
    RECT windowRect{};
    MONITORINFO monitorInfo{sizeof(monitorInfo)};
    if (!GetWindowRect(foreground, &windowRect) ||
        !GetMonitorInfoW(monitor, &monitorInfo)) {
        return false;
    }
    constexpr LONG tolerance = 2;
    return windowRect.left <= monitorInfo.rcMonitor.left + tolerance &&
           windowRect.top <= monitorInfo.rcMonitor.top + tolerance &&
           windowRect.right >= monitorInfo.rcMonitor.right - tolerance &&
           windowRect.bottom >= monitorInfo.rcMonitor.bottom - tolerance;
}

inline bool ForegroundIsExclusiveFullscreen() {
    HWND foreground = GetForegroundWindow();
    if (!foreground) return false;
    return MonitorCoveredByExclusiveFullscreen(
        MonitorFromWindow(foreground, MONITOR_DEFAULTTONULL));
}

inline bool TaskbarOccluded(HWND window) {
    if (!window || !IsWindow(window)) return true;
    if (!IsWindowVisible(window)) return true;
    return MonitorCoveredByExclusiveFullscreen(
        MonitorFromWindow(window, MONITOR_DEFAULTTONULL));
}

inline HWND FullViewWindow(MonitorTarget target, bool preferSecondaryForBoth) {
    const auto windows = CurrentProcessTaskbars();
    if (target == MonitorTarget::Primary)
        return windows.primary ? windows.primary : windows.secondary;
    if (target == MonitorTarget::Secondary)
        return windows.secondary ? windows.secondary : windows.primary;
    if (preferSecondaryForBoth && windows.secondary) return windows.secondary;
    return windows.primary ? windows.primary : windows.secondary;
}

// Keep display ownership stable while a fullscreen window covers a taskbar.
// The other display already has a mirror; moving the full XAML tree on focus
// changes creates duplicate/stale views. Fall back only when a bar is absent.
inline HWND VisibleFullViewWindow(MonitorTarget target,
                                  bool preferSecondaryForBoth) {
    const auto windows = CurrentProcessTaskbars();
    std::vector<HWND> candidates;
    auto consider = [&](HWND window) {
        if (!window) return;
        for (HWND existing : candidates) {
            if (existing == window) return;
        }
        candidates.push_back(window);
    };
    if (target == MonitorTarget::Primary) {
        consider(windows.primary);
        if (!windows.primary) consider(windows.secondary);
    } else if (target == MonitorTarget::Secondary) {
        consider(windows.secondary);
        if (!windows.secondary) consider(windows.primary);
    } else if (preferSecondaryForBoth) {
        consider(windows.secondary);
        consider(windows.primary);
        for (HWND secondary : windows.secondaries) consider(secondary);
    } else {
        consider(windows.primary);
        consider(windows.secondary);
        for (HWND secondary : windows.secondaries) consider(secondary);
    }
    return candidates.empty() ? nullptr : candidates.front();
}

inline std::vector<HWND> OtherTaskbarWindows(MonitorTarget target, HWND fullWindow) {
    std::vector<HWND> result;
    if (target != MonitorTarget::Both || !fullWindow) return result;
    const auto windows = CurrentProcessTaskbars();
    auto consider = [&](HWND window) {
        if (window && window != fullWindow) {
            for (HWND existing : result) {
                if (existing == window) return;
            }
            result.push_back(window);
        }
    };
    consider(windows.primary);
    for (HWND secondary : windows.secondaries) consider(secondary);
    return result;
}

inline HWND MirrorViewWindow(MonitorTarget target, HWND fullWindow) {
    const auto others = OtherTaskbarWindows(target, fullWindow);
    return others.empty() ? nullptr : others.front();
}

inline bool BuildLocalAppDataPath(const wchar_t* leaf, std::wstring* path) {
    if (!leaf || !path) return false;
    wchar_t localAppData[32768]{};
    const DWORD length = GetEnvironmentVariableW(
        L"LOCALAPPDATA", localAppData, ARRAYSIZE(localAppData));
    if (!length || length >= ARRAYSIZE(localAppData)) return false;
    *path = localAppData;
    path->append(L"\\Maxwell\\Opal\\");
    path->append(leaf);
    return true;
}

inline void EnsureLocalDirectory() {
    std::wstring path;
    if (!BuildLocalAppDataPath(L"placeholder", &path)) return;
    const auto slash = path.find_last_of(L'\\');
    if (slash == std::wstring::npos) return;
    const std::wstring opal = path.substr(0, slash);
    const auto parentSlash = opal.find_last_of(L'\\');
    if (parentSlash != std::wstring::npos)
        CreateDirectoryW(opal.substr(0, parentSlash).c_str(), nullptr);
    CreateDirectoryW(opal.c_str(), nullptr);
}

inline MonitorTarget ReadMonitorSetting(const wchar_t* name,
                                        MonitorTarget fallback) {
    const wchar_t* value = Wh_GetStringSetting(name);
    MonitorTarget result = fallback;
    if (value) {
        if (_wcsicmp(value, L"primary") == 0) result = MonitorTarget::Primary;
        else if (_wcsicmp(value, L"secondary") == 0) result = MonitorTarget::Secondary;
        else if (_wcsicmp(value, L"both") == 0) result = MonitorTarget::Both;
        Wh_FreeStringSetting(value);
    }
    return result;
}

inline std::wstring ReadStringSetting(const wchar_t* name,
                                      const wchar_t* fallback = L"") {
    const wchar_t* value = Wh_GetStringSetting(name);
    std::wstring result = value && *value ? value : fallback;
    if (value) Wh_FreeStringSetting(value);
    return result;
}

inline bool ReducedMotion() {
    BOOL enabled = TRUE;
    return SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &enabled, 0) && !enabled;
}

inline bool HighContrast() {
    HIGHCONTRASTW state{sizeof(state)};
    return SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(state), &state, 0) &&
           (state.dwFlags & HCF_HIGHCONTRASTON) != 0;
}

inline bool IsSecondaryTaskbar(HWND window) {
    wchar_t className[64]{};
    return window && GetClassNameW(window, className, ARRAYSIZE(className)) &&
           _wcsicmp(className, L"Shell_SecondaryTrayWnd") == 0;
}

inline bool MatchesMonitorTarget(HWND window, MonitorTarget target) {
    const bool secondary = IsSecondaryTaskbar(window);
    return target == MonitorTarget::Both ||
           (target == MonitorTarget::Primary && !secondary) ||
           (target == MonitorTarget::Secondary && secondary);
}

// Both remains a single-owner mode: the foreground monitor wins, so Opal never
// keeps duplicate media trees or duplicate sensor views alive.
inline HMONITOR ForegroundMonitor() {
    HWND foreground = GetForegroundWindow();
    return foreground ? MonitorFromWindow(foreground, MONITOR_DEFAULTTONEAREST) : nullptr;
}

inline std::wstring HealthPath(const wchar_t* package) {
    std::wstring leaf = L"health-";
    leaf += package;
    leaf += L".ini";
    std::wstring path;
    BuildLocalAppDataPath(leaf.c_str(), &path);
    return path;
}

inline unsigned long long NowFileTime100ns() {
    FILETIME now{};
    GetSystemTimeAsFileTime(&now);
    return (static_cast<unsigned long long>(now.dwHighDateTime) << 32) | now.dwLowDateTime;
}

// Crash protection counts only what it can attribute to this package.
//
// The first version marked a session dirty the moment Wh_ModInit ran and
// counted every unclean Explorer exit as a crash. Two things that are not
// crashes then quarantined both widgets within a day: the second explorer.exe
// of the shell's double-start race (it dies within seconds, before any widget
// exists), and an Explorer that ran for hours and was killed by a shell
// restart, an update, or the user. Now:
//
//   * a session is dirty only once the package has actually injected UI
//     (MarkPackageSessionLive); a process that never reached the taskbar
//     cannot have crashed because of the widget;
//   * an unclean exit after a LONG live session (>= 2 minutes) resets the
//     count instead of raising it - a crash loop is fast, a healthy session
//     that was killed later is not evidence of anything;
//   * a clean exit resets the count.
inline constexpr unsigned long long kCrashWindow100ns = 120ULL * 10'000'000ULL;

inline QuarantineState BeginPackageSession(const wchar_t* package) {
    EnsureLocalDirectory();
    QuarantineState state;
    const std::wstring path = HealthPath(package);
    if (path.empty()) return state;
    const DWORD dirty = GetPrivateProfileIntW(L"Health", L"Dirty", 0, path.c_str());
    state.crashCount = GetPrivateProfileIntW(L"Health", L"CrashCount", 0, path.c_str());
    wchar_t liveSinceText[32]{};
    GetPrivateProfileStringW(L"Health", L"LiveSince", L"0", liveSinceText,
                             ARRAYSIZE(liveSinceText), path.c_str());
    const unsigned long long liveSince = _wcstoui64(liveSinceText, nullptr, 10);
    std::wstring plannedRestartPath;
    const bool plannedRestart =
        BuildLocalAppDataPath(L"planned-explorer-restart", &plannedRestartPath) &&
        GetFileAttributesW(plannedRestartPath.c_str()) != INVALID_FILE_ATTRIBUTES;
    if (plannedRestart) {
        DeleteFileW(plannedRestartPath.c_str());
    }
    if (dirty) {
        const unsigned long long now = NowFileTime100ns();
        // A session that never injected UI cannot have crashed because of it.
        // Planned Explorer restarts and exclusive-fullscreen game coverage are
        // also not widget crash loops.
        const bool neverLived = liveSince == 0;
        const bool shortSession =
            !neverLived &&
            (now < liveSince || (now - liveSince) < kCrashWindow100ns);
        if (neverLived || plannedRestart || ForegroundIsExclusiveFullscreen()) {
            // Keep the existing count; do not treat this as a new crash.
        } else if (shortSession) {
            state.crashCount++;
        } else {
            state.crashCount = 0;
        }
    } else {
        state.crashCount = 0;
    }
    state.quarantined =
        GetPrivateProfileIntW(L"Health", L"Quarantined", 0, path.c_str()) != 0 ||
        state.crashCount >= 3;
    state.reason = state.quarantined
        ? L"Three unclean Explorer exits; reset crash protection in Opal settings."
        : L"";
    wchar_t number[32]{};
    swprintf_s(number, L"%lu", state.crashCount);
    WritePrivateProfileStringW(L"Health", L"CrashCount", number, path.c_str());
    WritePrivateProfileStringW(L"Health", L"Dirty", L"0", path.c_str());
    WritePrivateProfileStringW(L"Health", L"LiveSince", L"0", path.c_str());
    WritePrivateProfileStringW(L"Health", L"Quarantined",
                               state.quarantined ? L"1" : L"0", path.c_str());
    WritePrivateProfileStringW(L"Health", L"Reason", state.reason.c_str(), path.c_str());
    return state;
}

// Call once the package has UI in the taskbar. From here until
// EndPackageSession an unclean exit is attributable to this package.
inline void MarkPackageSessionLive(const wchar_t* package) {
    const std::wstring path = HealthPath(package);
    if (path.empty()) return;
    if (GetPrivateProfileIntW(L"Health", L"Dirty", 0, path.c_str()) != 0) return;
    wchar_t now[32]{};
    swprintf_s(now, L"%llu", NowFileTime100ns());
    WritePrivateProfileStringW(L"Health", L"LiveSince", now, path.c_str());
    WritePrivateProfileStringW(L"Health", L"Dirty", L"1", path.c_str());
}

inline void EndPackageSession(const wchar_t* package) {
    const std::wstring path = HealthPath(package);
    if (!path.empty())
        WritePrivateProfileStringW(L"Health", L"Dirty", L"0", path.c_str());
}

inline void MarkPlannedExplorerRestart() {
    EnsureLocalDirectory();
    std::wstring path;
    if (!BuildLocalAppDataPath(L"planned-explorer-restart", &path)) return;
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
}

inline void ResetPackageQuarantine(const wchar_t* package) {
    EnsureLocalDirectory();
    const std::wstring path = HealthPath(package);
    if (path.empty()) return;
    WritePrivateProfileStringW(L"Health", L"Dirty", L"0", path.c_str());
    WritePrivateProfileStringW(L"Health", L"CrashCount", L"0", path.c_str());
    WritePrivateProfileStringW(L"Health", L"Quarantined", L"0", path.c_str());
    WritePrivateProfileStringW(L"Health", L"Reason", L"", path.c_str());
}

inline void PublishAttachmentProof(const wchar_t* component, HWND fullWindow,
                                   unsigned expected, unsigned attached) {
    wchar_t leaf[64]{};
    swprintf_s(leaf, L"runtime-%lu.ini", GetCurrentProcessId());
    std::wstring path;
    if (!BuildLocalAppDataPath(leaf, &path)) return;
    FILETIME now{};
    GetSystemTimeAsFileTime(&now);
    ULARGE_INTEGER stamp{};
    stamp.LowPart = now.dwLowDateTime;
    stamp.HighPart = now.dwHighDateTime;
    wchar_t proof[160]{};
    swprintf_s(proof, L"%llu|%llu|%u|%u", stamp.QuadPart,
               static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(fullWindow)),
               expected, attached);
    const std::wstring key = std::wstring(component) + L"Attachment";
    WritePrivateProfileStringW(L"Runtime", key.c_str(), proof, path.c_str());
}

// Refresh the proof timestamp without walking XAML. Returns false when the
// stored HWND is gone so the caller can run a real attach pass.
inline bool TouchAttachmentProof(const wchar_t* component) {
    wchar_t leaf[64]{};
    swprintf_s(leaf, L"runtime-%lu.ini", GetCurrentProcessId());
    std::wstring path;
    if (!component || !BuildLocalAppDataPath(leaf, &path)) return false;
    const std::wstring key = std::wstring(component) + L"Attachment";
    wchar_t existing[160]{};
    GetPrivateProfileStringW(L"Runtime", key.c_str(), L"", existing,
                             ARRAYSIZE(existing), path.c_str());
    unsigned long long stamp = 0;
    unsigned long long hwndValue = 0;
    unsigned expected = 0;
    unsigned attached = 0;
    if (swscanf_s(existing, L"%llu|%llu|%u|%u", &stamp, &hwndValue, &expected,
                  &attached) != 4 ||
        !hwndValue || !expected || attached != expected) {
        return false;
    }
    HWND window = reinterpret_cast<HWND>(static_cast<uintptr_t>(hwndValue));
    if (!IsWindow(window)) return false;
    PublishAttachmentProof(component, window, expected, attached);
    return true;
}

inline void PublishRuntimeState(const wchar_t* activeName, const wchar_t* pidName,
                                bool active, bool suspended = false,
                                const wchar_t* reason = L"", bool quarantined = false) {
    EnsureLocalDirectory();
    const DWORD pidValue = GetCurrentProcessId();
    wchar_t runtimeLeaf[64]{};
    swprintf_s(runtimeLeaf, L"runtime-%lu.ini", pidValue);
    std::wstring runtimeFile;
    if (BuildLocalAppDataPath(runtimeLeaf, &runtimeFile)) {
        wchar_t pidText[16]{};
        swprintf_s(pidText, L"%lu", pidValue);
        WritePrivateProfileStringW(L"Runtime", activeName, active ? L"1" : L"0", runtimeFile.c_str());
        WritePrivateProfileStringW(L"Runtime", pidName, pidText, runtimeFile.c_str());
        const wchar_t* prefix = wcscmp(activeName, kMediaRuntimeActiveValue) == 0
                                    ? L"Media" : L"Performance";
        const std::wstring suspendedName = std::wstring(prefix) + L"Suspended";
        const std::wstring reasonName = std::wstring(prefix) + L"Reason";
        const std::wstring quarantineName = std::wstring(prefix) + L"Quarantined";
        WritePrivateProfileStringW(L"Runtime", suspendedName.c_str(),
                                   suspended ? L"1" : L"0", runtimeFile.c_str());
        WritePrivateProfileStringW(L"Runtime", reasonName.c_str(), reason, runtimeFile.c_str());
        WritePrivateProfileStringW(L"Runtime", quarantineName.c_str(),
                                   quarantined ? L"1" : L"0", runtimeFile.c_str());
    }

    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRegistryPath, 0, nullptr, 0,
                        KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) return;
    const DWORD activeValue = active ? 1 : 0;
    RegSetValueExW(key, activeName, 0, REG_DWORD,
                   reinterpret_cast<const BYTE*>(&activeValue), sizeof(activeValue));
    RegSetValueExW(key, pidName, 0, REG_DWORD,
                   reinterpret_cast<const BYTE*>(&pidValue), sizeof(pidValue));
    RegCloseKey(key);
}

}  // namespace OpalControl
