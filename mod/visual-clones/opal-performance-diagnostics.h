#pragma once

#include <windows.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <string>

// Opt-in numerical diagnostics. Collection has fixed storage, performs no I/O,
// creates no callbacks or threads, and owns no provider or XAML references.
namespace OpalPerformanceDiagnostics {
enum class Metric : size_t {
    ShellAttachment, MediaCommandDispatch, MediaCommandNextUpdate,
    MediaSnapshotToUi, MediaUiWork, HardwareOpen, HardwareRefresh, Count
};
inline constexpr size_t kCapacity = 64;
inline constexpr size_t kMetricCount = static_cast<size_t>(Metric::Count);
struct Stamp { LONGLONG ticks = 0; uint64_t epoch = 0; };
struct Samples {
    std::array<double, kCapacity> milliseconds{};
    uint64_t total = 0;
    size_t next = 0;
    size_t count = 0;
};
inline std::atomic<bool> g_enabled{false};
inline std::atomic<uint64_t> g_epoch{1};
inline SRWLOCK g_lock = SRWLOCK_INIT;
inline std::array<Samples, kMetricCount> g_samples{};

inline void SetEnabled(bool enabled) noexcept {
    AcquireSRWLockExclusive(&g_lock);
    if (g_enabled.load(std::memory_order_relaxed) != enabled) {
        g_samples = {};
        g_epoch.fetch_add(1, std::memory_order_relaxed);
        g_enabled.store(enabled, std::memory_order_release);
    }
    ReleaseSRWLockExclusive(&g_lock);
}

inline Stamp Begin() noexcept {
    if (!g_enabled.load(std::memory_order_acquire)) return {};
    const auto epoch = g_epoch.load(std::memory_order_acquire);
    LARGE_INTEGER now{};
    if (!QueryPerformanceCounter(&now)) return {};
    return {now.QuadPart, epoch};
}

inline void Record(Metric metric, Stamp start) noexcept {
    const size_t index = static_cast<size_t>(metric);
    if (!start.ticks || index >= kMetricCount ||
        !g_enabled.load(std::memory_order_acquire)) return;
    LARGE_INTEGER now{}, frequency{};
    if (!QueryPerformanceCounter(&now) || !QueryPerformanceFrequency(&frequency) ||
        frequency.QuadPart <= 0 || now.QuadPart < start.ticks) return;
    const double ms = static_cast<double>(now.QuadPart - start.ticks) * 1000.0 /
                      static_cast<double>(frequency.QuadPart);
    AcquireSRWLockExclusive(&g_lock);
    if (g_enabled.load(std::memory_order_relaxed) &&
        start.epoch == g_epoch.load(std::memory_order_relaxed)) {
        auto& samples = g_samples[index];
        samples.milliseconds[samples.next] = ms;
        samples.next = (samples.next + 1) % kCapacity;
        samples.count = std::min(samples.count + 1, kCapacity);
        if (samples.total != UINT64_MAX) ++samples.total;
    }
    ReleaseSRWLockExclusive(&g_lock);
}

class Scope {
public:
    explicit Scope(Metric metric) noexcept : metric_(metric), start_(Begin()) {}
    ~Scope() { Record(metric_, start_); }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
private:
    Metric metric_;
    Stamp start_;
};

inline std::wstring Summary() {
    std::array<Samples, kMetricCount> snapshot{};
    AcquireSRWLockShared(&g_lock);
    const bool enabled = g_enabled.load(std::memory_order_relaxed);
    const uint64_t epoch = g_epoch.load(std::memory_order_relaxed);
    if (enabled) snapshot = g_samples;
    ReleaseSRWLockShared(&g_lock);
    if (!enabled) return {};
    constexpr const wchar_t* names[] = {
        L"Shell successful attachment/recovery pass (excluding backoff)", L"Media command dispatch",
        L"Media command to next update (not acknowledgment)",
        L"Media snapshot to UI", L"Media UI work",
        L"Hardware panel to Opened (not pixel-present)", L"Hardware panel refresh"
    };
    std::wstring result = L"\r\nOpal latency diagnostics (milliseconds; latest 64 samples per metric)\r\n";
    for (size_t i = 0; i < kMetricCount; ++i) {
        auto& s = snapshot[i];
        result += names[i];
        if (!s.count) { result += L": no samples\r\n"; continue; }
        std::sort(s.milliseconds.begin(), s.milliseconds.begin() + s.count);
        const size_t p95 = (95 * s.count + 99) / 100 - 1;
        wchar_t row[180]{};
        swprintf_s(row, L": n=%zu total=%llu median=%.3f p95=%.3f max=%.3f\r\n",
                   s.count, static_cast<unsigned long long>(s.total),
                   s.count % 2 ? s.milliseconds[s.count / 2]
                               : (s.milliseconds[s.count / 2 - 1] + s.milliseconds[s.count / 2]) / 2,
                   s.milliseconds[p95], s.milliseconds[s.count - 1]);
        result += row;
    }
    // Export provenance is gathered only on the existing report action. It
    // identifies the process lifetime and capture time without a watcher,
    // file writer, module enumeration or additional hot-path work.
    FILETIME created{}, exited{}, kernel{}, user{}, captured{};
    if (GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user)) {
        GetSystemTimeAsFileTime(&captured);
        const auto ticks = [](FILETIME value) -> unsigned long long {
            return (static_cast<unsigned long long>(value.dwHighDateTime) << 32) |
                   value.dwLowDateTime;
        };
        wchar_t provenance[220]{};
        swprintf_s(provenance,
            L"Opal latency provenance: version=1 pid=%lu processStartFileTime=%llu capturedFileTime=%llu epoch=%llu\r\n",
            GetCurrentProcessId(), ticks(created), ticks(captured),
            static_cast<unsigned long long>(epoch));
        result.insert(0, provenance);
    }
    return result;
}
}  // namespace OpalPerformanceDiagnostics
