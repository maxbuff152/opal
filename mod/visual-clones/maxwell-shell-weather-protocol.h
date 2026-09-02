#pragma once

#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cwchar>

// Separate from SnapshotV1 telemetry. Explorer asks; Maxwell.Shell.Core.exe
// fetches wttr.in so explorer.exe does not own the WinINet session.
namespace MaxwellShellWeather {

inline constexpr wchar_t kMappingName[] = L"Local\\Maxwell.Shell.Weather.v1";
inline constexpr wchar_t kRequestMappingName[] =
    L"Local\\Maxwell.Shell.WeatherRequest.v1";
inline constexpr wchar_t kChangedEventName[] =
    L"Local\\Maxwell.Shell.WeatherChanged.v1";
inline constexpr wchar_t kWakeEventName[] =
    L"Local\\Maxwell.Shell.WeatherWake.v1";

inline constexpr std::uint32_t kMagic = 0x3157534d;  // "MSW1"
inline constexpr std::uint16_t kVersion = 1;

enum Units : std::uint32_t {
    UnitsAuto = 0,
    UnitsUscs = 1,
    UnitsMetric = 2,
    UnitsMetricMsWind = 3,
};

enum Status : std::uint32_t {
    StatusEmpty = 0,
    StatusOk = 1,
    StatusError = 2,
};

struct alignas(8) RequestV1 {
    volatile LONG64 sequence;
    std::uint32_t magic;
    std::uint16_t version;
    std::uint16_t structSize;
    std::uint32_t units;
    std::uint32_t readerPid;
    std::uint64_t heartbeatTickMs;
    wchar_t location[96];
    wchar_t format[192];
};

struct alignas(8) SnapshotV1 {
    volatile LONG64 sequence;
    std::uint32_t magic;
    std::uint16_t version;
    std::uint16_t structSize;
    std::uint32_t status;
    std::uint32_t reserved;
    std::uint64_t fetchedTickMs;
    wchar_t content[384];
};

static_assert(sizeof(RequestV1) == 608);
static_assert(sizeof(SnapshotV1) == 800);

template <typename T>
inline bool ReadSeqlock(const T* view, T& copy, std::uint32_t magic) {
    if (!view) {
        return false;
    }
    for (int attempt = 0; attempt < 4; ++attempt) {
        const LONG64 before = view->sequence;
        if (before & 1) {
            SwitchToThread();
            continue;
        }
        MemoryBarrier();
        std::memcpy(&copy, view, sizeof(copy));
        MemoryBarrier();
        const LONG64 after = view->sequence;
        if (before == after && !(after & 1) && copy.magic == magic &&
            copy.version == kVersion && copy.structSize == sizeof(T)) {
            return true;
        }
    }
    return false;
}

inline void CopyBounded(wchar_t* dest, size_t destCount, const wchar_t* source) {
    if (!destCount) {
        return;
    }
    dest[0] = 0;
    if (!source) {
        return;
    }
    size_t i = 0;
    for (; i + 1 < destCount && source[i]; ++i) {
        dest[i] = source[i];
    }
    dest[i] = 0;
}

}  // namespace MaxwellShellWeather
