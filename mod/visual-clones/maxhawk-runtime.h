// =====================================================================
//  maxhawk-runtime.h
//
//  The small piece of Windhawk that maxwell-shell actually needs, so the same
//  source can build as a Windhawk mod OR as a standalone Maxhawk DLL.
//
//  WHY THIS IS SMALL
//
//  Windhawk is a large system: it injects into ~325 of the 397 processes on this
//  machine, downloads Microsoft PDBs at runtime, and resolves unexported symbols
//  so mods can hook internal taskbar functions.
//
//  maxwell-shell needs none of that. It uses no function hooks and no symbol
//  resolution - only the documented XAML diagnostics API. Its entire dependency
//  on the host is:
//
//      Wh_Log            -> a file
//      Wh_GetIntSetting  -> the registry
//      Wh_Mod* lifecycle -> DllMain
//
//  So the replacement is a header, not a framework. The four taskbar mods
//  (clock, icons, media, system-info) DO hook by symbol and still require
//  Windhawk; nothing here changes that.
//
//  SAFETY
//
//  This DLL is loaded into explorer.exe. If it faults at load, the shell can
//  fail on every boot - strictly worse than a Windhawk mod, which the engine can
//  disable. Two guards are therefore unconditional and come first:
//
//    1. A kill switch file. If it exists, the DLL does nothing at all. Recovery
//       never requires running code that might itself be broken.
//    2. A load counter. Repeated loads without a clean unload arm the kill
//       switch automatically, so a crash loop stops itself.
// =====================================================================
#pragma once

#ifndef WH_MOD   // Windhawk builds define this; everything here is the standalone path.

#include <windows.h>
#include <shlwapi.h>

#include <cstdarg>
#include <cstdio>
#include <string>

// ---------------------------------------------------------------------
//  Paths
// ---------------------------------------------------------------------
inline std::wstring MaxhawkStateDir() {
    wchar_t local[MAX_PATH] = {};
    if (!GetEnvironmentVariableW(L"LOCALAPPDATA", local, MAX_PATH)) { return L""; }
    std::wstring dir(local);
    dir += L"\\Maxwell";
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

inline std::wstring MaxhawkKillSwitchPath() {
    const std::wstring dir = MaxhawkStateDir();
    return dir.empty() ? L"" : dir + L"\\maxhawk-disabled";
}

// ---------------------------------------------------------------------
//  Kill switch
//
//  Deliberately a FILE, not a registry value: it can be created from any shell,
//  from another machine over a share, or from a recovery environment, without
//  needing this DLL or any of its tooling to work.
// ---------------------------------------------------------------------
inline bool MaxhawkDisabled() {
    const std::wstring path = MaxhawkKillSwitchPath();
    if (path.empty()) { return true; }   // no state dir - fail closed
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

inline void MaxhawkArmKillSwitch(const wchar_t* reason) {
    const std::wstring path = MaxhawkKillSwitchPath();
    if (path.empty()) { return; }
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) { return; }
    if (reason) {
        const int bytes = WideCharToMultiByte(CP_UTF8, 0, reason, -1, nullptr, 0, nullptr, nullptr);
        if (bytes > 1) {
            std::string utf8(bytes - 1, '\0');
            WideCharToMultiByte(CP_UTF8, 0, reason, -1, utf8.data(), bytes, nullptr, nullptr);
            DWORD written = 0;
            WriteFile(h, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
        }
    }
    CloseHandle(h);
}

// ---------------------------------------------------------------------
//  Crash-loop guard
//
//  A counter is bumped on load and cleared on clean unload. If the DLL is
//  loaded repeatedly without ever unloading cleanly, it is faulting during
//  init - so arm the kill switch and stop.
// ---------------------------------------------------------------------
inline constexpr const wchar_t* kMaxhawkRegKey = L"Software\\Maxwell\\Maxhawk";
inline constexpr int kMaxhawkMaxDirtyLoads = 3;

inline DWORD MaxhawkReadDword(const wchar_t* name, DWORD fallback) {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kMaxhawkRegKey, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
        return fallback;
    }
    DWORD value = fallback, size = sizeof(value), type = 0;
    if (RegQueryValueExW(key, name, nullptr, &type, reinterpret_cast<LPBYTE>(&value), &size) != ERROR_SUCCESS ||
        type != REG_DWORD) {
        value = fallback;
    }
    RegCloseKey(key);
    return value;
}

inline void MaxhawkWriteDword(const wchar_t* name, DWORD value) {
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kMaxhawkRegKey, 0, nullptr, 0,
                        KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) {
        return;
    }
    RegSetValueExW(key, name, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));
    RegCloseKey(key);
}

// Returns false when the guard has tripped and init must not proceed.
inline bool MaxhawkEnterLoad() {
    const DWORD dirty = MaxhawkReadDword(L"DirtyLoads", 0);
    if (dirty >= kMaxhawkMaxDirtyLoads) {
        MaxhawkArmKillSwitch(L"crash-loop guard: too many loads without a clean unload");
        return false;
    }
    MaxhawkWriteDword(L"DirtyLoads", dirty + 1);
    return true;
}

inline void MaxhawkLeaveLoad() { MaxhawkWriteDword(L"DirtyLoads", 0); }

// ---------------------------------------------------------------------
//  Wh_* surface
// ---------------------------------------------------------------------
inline void Wh_Log(const wchar_t* fmt, ...) {
    wchar_t line[1024] = {};
    va_list args;
    va_start(args, fmt);
    _vsnwprintf_s(line, _TRUNCATE, fmt, args);
    va_end(args);

    const std::wstring dir = MaxhawkStateDir();
    if (dir.empty()) { return; }
    const std::wstring path = dir + L"\\maxhawk.log";

    HANDLE h = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) { return; }

    wchar_t stamp[32] = {};
    SYSTEMTIME st{};
    GetLocalTime(&st);
    swprintf_s(stamp, L"%02d:%02d:%02d.%03d  ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    std::wstring text = stamp;
    text += line;
    text += L"\r\n";

    const int bytes = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (bytes > 1) {
        std::string utf8(bytes - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, utf8.data(), bytes, nullptr, nullptr);
        DWORD written = 0;
        WriteFile(h, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
    }
    CloseHandle(h);
}

// Settings live under HKCU so they can be changed without elevation - the same
// place the injector and any future UI would write.
inline int Wh_GetIntSetting(const wchar_t* name) {
    return static_cast<int>(MaxhawkReadDword(name, 0));
}

// String settings are read from the same key as REG_SZ. The caller frees the
// result with Wh_FreeStringSetting, matching the Windhawk contract.
inline const wchar_t* Wh_GetStringSetting(const wchar_t* name) {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kMaxhawkRegKey, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
        return _wcsdup(L"");
    }
    DWORD size = 0, type = 0;
    if (RegQueryValueExW(key, name, nullptr, &type, nullptr, &size) != ERROR_SUCCESS || type != REG_SZ) {
        RegCloseKey(key);
        return _wcsdup(L"");
    }
    auto* buf = static_cast<wchar_t*>(malloc(size + sizeof(wchar_t)));
    if (!buf) { RegCloseKey(key); return _wcsdup(L""); }
    if (RegQueryValueExW(key, name, nullptr, nullptr, reinterpret_cast<LPBYTE>(buf), &size) != ERROR_SUCCESS) {
        free(buf); RegCloseKey(key); return _wcsdup(L"");
    }
    buf[size / sizeof(wchar_t)] = L'\0';
    RegCloseKey(key);
    return buf;
}

inline void Wh_FreeStringSetting(const wchar_t* s) { if (s) { free(const_cast<wchar_t*>(s)); } }

#endif  // !WH_MOD
