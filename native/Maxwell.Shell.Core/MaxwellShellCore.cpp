#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <dxgi.h>
#include <shellapi.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../../mod/visual-clones/maxwell-shell-telemetry-protocol.h"

namespace mst = MaxwellShellTelemetry;
constexpr double kGiB = 1024.0 * 1024.0 * 1024.0;

struct Sample {
    std::uint32_t flags = 0;
    double cpu = 0.0;
    double ram = 0.0;
    double ramUsedGiB = 0.0;
    double ramTotalGiB = 0.0;
    double gpu = 0.0;
    double vram = 0.0;
    double vramUsedGiB = 0.0;
    double vramTotalGiB = 0.0;
};

std::uint64_t FileTimeValue(const FILETIME& value) {
    ULARGE_INTEGER result{};
    result.LowPart = value.dwLowDateTime;
    result.HighPart = value.dwHighDateTime;
    return result.QuadPart;
}

class CpuSampler {
  public:
    std::optional<double> Read() {
        FILETIME idleTime{}, kernelTime{}, userTime{};
        if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
            return std::nullopt;
        }
        const auto idle = FileTimeValue(idleTime);
        const auto kernel = FileTimeValue(kernelTime);
        const auto user = FileTimeValue(userTime);
        if (!initialized_) {
            initialized_ = true;
            previousIdle_ = idle;
            previousKernel_ = kernel;
            previousUser_ = user;
            return 0.0;
        }
        const auto idleDelta = idle - previousIdle_;
        const auto kernelDelta = kernel - previousKernel_;
        const auto userDelta = user - previousUser_;
        previousIdle_ = idle;
        previousKernel_ = kernel;
        previousUser_ = user;
        const auto total = kernelDelta + userDelta;
        if (!total || idleDelta > total) {
            return 0.0;
        }
        return std::clamp(100.0 * static_cast<double>(total - idleDelta) /
                              static_cast<double>(total),
                          0.0, 100.0);
    }

  private:
    bool initialized_ = false;
    std::uint64_t previousIdle_ = 0;
    std::uint64_t previousKernel_ = 0;
    std::uint64_t previousUser_ = 0;
};

struct AdapterInfo {
    std::wstring luid;
    std::uint64_t dedicatedBytes = 0;
    std::uint64_t sharedBytes = 0;
};

std::wstring Lower(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), towlower);
    return value;
}

std::optional<AdapterInfo> SelectAdapter() {
    IDXGIFactory1* factory = nullptr;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
        return std::nullopt;
    }
    std::optional<AdapterInfo> selected;
    std::uint64_t selectedScore = 0;
    for (UINT index = 0;; ++index) {
        IDXGIAdapter1* adapter = nullptr;
        const HRESULT result = factory->EnumAdapters1(index, &adapter);
        if (result == DXGI_ERROR_NOT_FOUND) {
            break;
        }
        if (FAILED(result) || !adapter) {
            continue;
        }
        DXGI_ADAPTER_DESC1 desc{};
        if (SUCCEEDED(adapter->GetDesc1(&desc)) &&
            !(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
            const auto score = desc.DedicatedVideoMemory
                                   ? desc.DedicatedVideoMemory
                                   : desc.SharedSystemMemory;
            if (!selected || score > selectedScore) {
                wchar_t luid[32]{};
                swprintf_s(luid, L"0x%08x_0x%08x",
                           static_cast<DWORD>(desc.AdapterLuid.HighPart),
                           desc.AdapterLuid.LowPart);
                selected = AdapterInfo{Lower(luid),
                                       desc.DedicatedVideoMemory,
                                       desc.SharedSystemMemory};
                selectedScore = score;
            }
        }
        adapter->Release();
    }
    factory->Release();
    return selected;
}

class GpuSampler {
  public:
    GpuSampler() : adapter_(SelectAdapter()) {
        if (PdhOpenQueryW(nullptr, 0, &query_) != ERROR_SUCCESS) {
            query_ = nullptr;
            return;
        }
        PdhAddEnglishCounterW(query_,
                              L"\\GPU Engine(*)\\Utilization Percentage", 0,
                              &gpuCounter_);
        PdhAddEnglishCounterW(query_,
                              L"\\GPU Adapter Memory(*)\\Dedicated Usage", 0,
                              &dedicatedCounter_);
        PdhAddEnglishCounterW(query_,
                              L"\\GPU Adapter Memory(*)\\Shared Usage", 0,
                              &sharedCounter_);
        PdhCollectQueryData(query_);
    }

    ~GpuSampler() {
        if (query_) {
            PdhCloseQuery(query_);
        }
    }

    void Read(Sample& sample) {
        if (!query_ || PdhCollectQueryData(query_) != ERROR_SUCCESS) {
            return;
        }
        if (const auto usage = ReadGpuUsage()) {
            sample.gpu = *usage;
            sample.flags |= mst::MetricGpu;
        }
        if (!adapter_) {
            return;
        }
        const bool dedicated = adapter_->dedicatedBytes > 0;
        const auto totalBytes = dedicated ? adapter_->dedicatedBytes
                                          : adapter_->sharedBytes;
        if (!totalBytes) {
            return;
        }
        if (const auto used =
                dedicated ? ReadMemoryUsage(dedicatedCounter_, dedicatedBuffer_)
                          : ReadMemoryUsage(sharedCounter_, sharedBuffer_)) {
            sample.vramUsedGiB = *used / kGiB;
            sample.vramTotalGiB = static_cast<double>(totalBytes) / kGiB;
            sample.vram = std::clamp(100.0 * *used /
                                         static_cast<double>(totalBytes),
                                     0.0, 100.0);
            sample.flags |= mst::MetricVram;
        }
    }

  private:
    // The GPU Engine counter is a wildcard over every (process, engine) pair,
    // which on a busy desktop is hundreds of instances per tick. Everything
    // below is written to walk that array without allocating: the PDH buffers
    // are reused between ticks, instance names are compared in place, and the
    // per-engine totals live in a vector whose capacity is retained.
    bool ReadArray(PDH_HCOUNTER counter,
                   std::vector<std::uint8_t>& buffer,
                   DWORD& count) {
        if (!counter) {
            return false;
        }
        auto* items =
            buffer.empty()
                ? nullptr
                : reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
        DWORD bytes = static_cast<DWORD>(buffer.size());
        count = 0;
        auto status = PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE,
                                                   &bytes, &count, items);
        if (status == static_cast<PDH_STATUS>(PDH_MORE_DATA)) {
            if (!bytes) {
                return false;
            }
            buffer.resize(bytes);
            items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(
                buffer.data());
            status = PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE,
                                                  &bytes, &count, items);
        }
        return status == ERROR_SUCCESS && count > 0;
    }

    static bool StartsWithNoCase(const wchar_t* text, const wchar_t* prefix) {
        for (; *prefix; ++text, ++prefix) {
            if (!*text || towlower(*text) != towlower(*prefix)) {
                return false;
            }
        }
        return true;
    }

    static const wchar_t* FindNoCase(const wchar_t* haystack,
                                     const wchar_t* needle) {
        if (!haystack || !needle || !*needle) {
            return nullptr;
        }
        for (const wchar_t* cursor = haystack; *cursor; ++cursor) {
            if (StartsWithNoCase(cursor, needle)) {
                return cursor;
            }
        }
        return nullptr;
    }

    bool MatchesAdapter(const wchar_t* instance) const {
        return !adapter_ ||
               FindNoCase(instance, adapter_->luid.c_str()) != nullptr;
    }

    static bool IsUsableValue(const PDH_FMT_COUNTERVALUE& formatted) {
        return (formatted.CStatus == PDH_CSTATUS_VALID_DATA ||
                formatted.CStatus == PDH_CSTATUS_NEW_DATA) &&
               std::isfinite(formatted.doubleValue) &&
               formatted.doubleValue >= 0.0;
    }

    std::optional<double> ReadGpuUsage() {
        DWORD count = 0;
        if (!ReadArray(gpuCounter_, gpuBuffer_, count)) {
            return std::nullopt;
        }
        auto* items =
            reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(gpuBuffer_.data());

        // Sum utilization across every process on one engine, then report the
        // busiest engine. Keys are views into gpuBuffer_, which outlives this
        // loop; distinct engines number in the tens, so a linear scan beats a
        // tree and allocates nothing once the capacity has settled.
        engineTotals_.clear();
        bool found = false;
        for (DWORD i = 0; i < count; ++i) {
            const auto& formatted = items[i].FmtValue;
            if (!IsUsableValue(formatted)) {
                continue;
            }
            const wchar_t* instance = items[i].szName;
            if (!instance || !MatchesAdapter(instance)) {
                continue;
            }
            const wchar_t* luidAt = FindNoCase(instance, L"luid_");
            const std::wstring_view key(luidAt ? luidAt : instance);
            bool merged = false;
            for (auto& entry : engineTotals_) {
                if (entry.first == key) {
                    entry.second += formatted.doubleValue;
                    merged = true;
                    break;
                }
            }
            if (!merged) {
                engineTotals_.emplace_back(key, formatted.doubleValue);
            }
            found = true;
        }
        double busiest = 0.0;
        for (const auto& entry : engineTotals_) {
            busiest = std::max(busiest, entry.second);
        }
        return found ? std::optional<double>(std::clamp(busiest, 0.0, 100.0))
                     : std::optional<double>(0.0);
    }

    std::optional<double> ReadMemoryUsage(PDH_HCOUNTER counter,
                                          std::vector<std::uint8_t>& buffer) {
        DWORD count = 0;
        if (!ReadArray(counter, buffer, count)) {
            return std::nullopt;
        }
        auto* items =
            reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
        double total = 0.0;
        bool found = false;
        for (DWORD i = 0; i < count; ++i) {
            const auto& formatted = items[i].FmtValue;
            if (!IsUsableValue(formatted)) {
                continue;
            }
            if (items[i].szName && MatchesAdapter(items[i].szName)) {
                total += formatted.doubleValue;
                found = true;
            }
        }
        return found ? std::optional<double>(total) : std::nullopt;
    }

    std::optional<AdapterInfo> adapter_;
    PDH_HQUERY query_ = nullptr;
    PDH_HCOUNTER gpuCounter_ = nullptr;
    PDH_HCOUNTER dedicatedCounter_ = nullptr;
    PDH_HCOUNTER sharedCounter_ = nullptr;
    std::vector<std::uint8_t> gpuBuffer_;
    std::vector<std::uint8_t> dedicatedBuffer_;
    std::vector<std::uint8_t> sharedBuffer_;
    std::vector<std::pair<std::wstring_view, double>> engineTotals_;
};

bool IsFullscreenApplicationActive() {
    const HWND window = GetForegroundWindow();
    if (!window || IsIconic(window)) {
        return false;
    }
    RECT windowRect{};
    MONITORINFO monitorInfo{};
    monitorInfo.cbSize = sizeof(monitorInfo);
    const HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONULL);
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

void Publish(mst::SnapshotV1* target,
             HANDLE changedEvent,
             const Sample& sample,
             std::uint32_t intervalMs) {
    InterlockedIncrement64(&target->sequence);
    MemoryBarrier();
    target->magic = mst::kMagic;
    target->version = mst::kVersion;
    target->structSize = sizeof(*target);
    target->flags = sample.flags;
    target->publisherPid = GetCurrentProcessId();
    target->sampleIntervalMs = intervalMs;
    target->sampledTickMs = GetTickCount64();
    target->cpuPercent = sample.cpu;
    target->ramPercent = sample.ram;
    target->ramUsedGiB = sample.ramUsedGiB;
    target->ramTotalGiB = sample.ramTotalGiB;
    target->gpuPercent = sample.gpu;
    target->vramPercent = sample.vram;
    target->vramUsedGiB = sample.vramUsedGiB;
    target->vramTotalGiB = sample.vramTotalGiB;
    MemoryBarrier();
    InterlockedIncrement64(&target->sequence);
    SetEvent(changedEvent);
}

Sample Collect(CpuSampler& cpu, GpuSampler& gpu) {
    Sample sample;
    if (const auto cpuValue = cpu.Read()) {
        sample.cpu = *cpuValue;
        sample.flags |= mst::MetricCpu;
    }
    MEMORYSTATUSEX memory{};
    memory.dwLength = sizeof(memory);
    if (GlobalMemoryStatusEx(&memory)) {
        sample.ram = static_cast<double>(memory.dwMemoryLoad);
        sample.ramTotalGiB = static_cast<double>(memory.ullTotalPhys) / kGiB;
        sample.ramUsedGiB =
            static_cast<double>(memory.ullTotalPhys - memory.ullAvailPhys) /
            kGiB;
        sample.flags |= mst::MetricRam;
    }
    gpu.Read(sample);
    return sample;
}

bool ReadStable(const mst::SnapshotV1* source, mst::SnapshotV1& copy) {
    for (int attempt = 0; attempt < 4; ++attempt) {
        const LONG64 before = source->sequence;
        if (before & 1) {
            SwitchToThread();
            continue;
        }
        MemoryBarrier();
        std::memcpy(&copy, source, sizeof(copy));
        MemoryBarrier();
        const LONG64 after = source->sequence;
        if (before == after && !(after & 1) && copy.magic == mst::kMagic &&
            copy.version == mst::kVersion && copy.structSize == sizeof(copy)) {
            return true;
        }
    }
    return false;
}

int Probe(const wchar_t* outputPath) {
    const HANDLE mapping =
        OpenFileMappingW(FILE_MAP_READ, FALSE, mst::kMappingName);
    if (!mapping) {
        return 2;
    }
    const auto* view = static_cast<const mst::SnapshotV1*>(
        MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, sizeof(mst::SnapshotV1)));
    mst::SnapshotV1 sample{};
    const bool valid = view && ReadStable(view, sample);
    if (view) {
        UnmapViewOfFile(view);
    }
    CloseHandle(mapping);
    if (!valid) {
        return 3;
    }
    char json[1024]{};
    const int length = snprintf(
        json, sizeof(json),
        "{\n  \"protocolVersion\": %u,\n  \"publisherPid\": %u,\n  "
        "\"sequence\": %lld,\n  \"sampleAgeMs\": %llu,\n  "
        "\"sampleIntervalMs\": %u,\n  \"flags\": %u,\n  "
        "\"cpuPercent\": %.2f,\n  \"ramPercent\": %.2f,\n  "
        "\"ramUsedGiB\": %.2f,\n  \"ramTotalGiB\": %.2f,\n  "
        "\"gpuPercent\": %.2f,\n  \"vramPercent\": %.2f,\n  "
        "\"vramUsedGiB\": %.2f,\n  \"vramTotalGiB\": %.2f\n}\n",
        sample.version, sample.publisherPid, sample.sequence,
        GetTickCount64() - sample.sampledTickMs, sample.sampleIntervalMs,
        sample.flags, sample.cpuPercent, sample.ramPercent, sample.ramUsedGiB,
        sample.ramTotalGiB, sample.gpuPercent, sample.vramPercent,
        sample.vramUsedGiB, sample.vramTotalGiB);
    if (length <= 0) {
        return 4;
    }
    const HANDLE file = CreateFileW(outputPath, GENERIC_WRITE, FILE_SHARE_READ,
                                    nullptr, CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return 5;
    }
    DWORD written = 0;
    const BOOL ok = WriteFile(file, json, static_cast<DWORD>(length), &written,
                              nullptr);
    CloseHandle(file);
    return ok && written == static_cast<DWORD>(length) ? 0 : 6;
}

struct TelemetryContext {
    mst::SnapshotV1* view = nullptr;
    mst::ReaderRequestV1* readerView = nullptr;
    HANDLE changedEvent = nullptr;
    HANDLE readerWakeEvent = nullptr;
    HANDLE stopEvent = nullptr;
};

// Reads the reader's requested cadence with the same seqlock discipline the
// reader uses on the snapshot. Returns false when no live reader is attached.
bool ReadReaderRequest(const mst::ReaderRequestV1* readerView,
                       std::uint32_t& desiredIntervalMs) {
    if (!readerView) {
        return false;
    }
    mst::ReaderRequestV1 wire{};
    bool stable = false;
    for (int attempt = 0; attempt < 4; ++attempt) {
        const LONG64 before = readerView->sequence;
        if (before & 1) {
            SwitchToThread();
            continue;
        }
        MemoryBarrier();
        std::memcpy(&wire, readerView, sizeof(wire));
        MemoryBarrier();
        const LONG64 after = readerView->sequence;
        if (before == after && !(after & 1)) {
            stable = true;
            break;
        }
    }
    if (!stable || wire.magic != mst::kReaderMagic ||
        wire.version != mst::kVersion || wire.structSize != sizeof(wire)) {
        return false;
    }

    // A reader that stopped heartbeating is gone; treat it as absent rather
    // than sampling forever on its behalf.
    const std::uint64_t now = GetTickCount64();
    const std::uint64_t staleAfter =
        std::max<std::uint64_t>(mst::kIdleIntervalMs,
                                wire.desiredIntervalMs * 3ull);
    if (wire.heartbeatTickMs > now ||
        now - wire.heartbeatTickMs > staleAfter) {
        return false;
    }

    desiredIntervalMs = std::clamp(wire.desiredIntervalMs, mst::kMinIntervalMs,
                                   mst::kMaxIntervalMs);
    return true;
}

DWORD WINAPI TelemetryLoop(void* rawContext) {
    auto* context = static_cast<TelemetryContext*>(rawContext);
    CpuSampler cpu;
    GpuSampler gpu;
    cpu.Read();
    if (WaitForSingleObject(context->stopEvent, 500) == WAIT_OBJECT_0) {
        return 0;
    }
    const HANDLE waits[2] = {context->stopEvent, context->readerWakeEvent};
    const DWORD waitCount = context->readerWakeEvent ? 2u : 1u;
    while (true) {
        // The consumer owns the cadence: its scene engine already decides
        // whether this is a 5 s desktop, a 10 s game, or a 15 s battery-saver
        // tick. Sampling faster than the widget renders is pure waste, and with
        // no reader attached there is nothing to sample for at all.
        std::uint32_t intervalMs = mst::kIdleIntervalMs;
        const bool readerAttached =
            ReadReaderRequest(context->readerView, intervalMs);
        if (readerAttached && IsFullscreenApplicationActive()) {
            intervalMs = std::max(intervalMs, 10000u);
        }
        Publish(context->view, context->changedEvent, Collect(cpu, gpu),
                intervalMs);
        // A reader attaching mid-idle signals the wake event so it does not
        // have to sit through the remainder of a 30 s idle interval.
        const DWORD result =
            WaitForMultipleObjects(waitCount, waits, FALSE, intervalMs);
        if (result == WAIT_OBJECT_0) {
            return 0;
        }
    }
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    int argumentCount = 0;
    wchar_t** arguments = CommandLineToArgvW(GetCommandLineW(), &argumentCount);
    if (arguments && argumentCount == 3 &&
        _wcsicmp(arguments[1], L"--probe") == 0) {
        const int result = Probe(arguments[2]);
        LocalFree(arguments);
        return result;
    }
    LocalFree(arguments);

    const HANDLE instanceMutex =
        CreateMutexW(nullptr, FALSE, mst::kInstanceMutexName);
    if (!instanceMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (instanceMutex) {
            CloseHandle(instanceMutex);
        }
        return 0;
    }
    const HANDLE mapping = CreateFileMappingW(
        INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
        sizeof(mst::SnapshotV1), mst::kMappingName);
    const HANDLE changedEvent =
        CreateEventW(nullptr, FALSE, FALSE, mst::kChangedEventName);
    const HANDLE stopEvent =
        CreateEventW(nullptr, TRUE, FALSE, mst::kStopEventName);
    auto* view = mapping ? static_cast<mst::SnapshotV1*>(MapViewOfFile(
                               mapping, FILE_MAP_ALL_ACCESS, 0, 0,
                               sizeof(mst::SnapshotV1)))
                         : nullptr;
    if (!mapping || !changedEvent || !stopEvent || !view) {
        if (view) UnmapViewOfFile(view);
        if (stopEvent) CloseHandle(stopEvent);
        if (changedEvent) CloseHandle(changedEvent);
        if (mapping) CloseHandle(mapping);
        CloseHandle(instanceMutex);
        return 1;
    }
    std::memset(view, 0, sizeof(*view));

    // The reader channel is optional: if it cannot be created the loop simply
    // falls back to the idle cadence and keeps publishing.
    const HANDLE readerMapping = CreateFileMappingW(
        INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
        sizeof(mst::ReaderRequestV1), mst::kReaderMappingName);
    auto* readerView =
        readerMapping ? static_cast<mst::ReaderRequestV1*>(MapViewOfFile(
                            readerMapping, FILE_MAP_ALL_ACCESS, 0, 0,
                            sizeof(mst::ReaderRequestV1)))
                      : nullptr;
    if (readerView) {
        std::memset(readerView, 0, sizeof(*readerView));
    }
    const HANDLE readerWakeEvent =
        CreateEventW(nullptr, FALSE, FALSE, mst::kReaderWakeEventName);

    TelemetryContext context{view, readerView, changedEvent, readerWakeEvent,
                             stopEvent};
    // Maxwell.Shell.Core is deliberately telemetry-only. The retired Adaptive
    // Dock (Command/Media/Focus/Capture tabs) no longer creates a window,
    // registers hotkeys, indexes commands, or subscribes to media sessions.
    const int telemetryResult = static_cast<int>(TelemetryLoop(&context));
    UnmapViewOfFile(view);
    if (readerWakeEvent) CloseHandle(readerWakeEvent);
    if (readerView) UnmapViewOfFile(readerView);
    if (readerMapping) CloseHandle(readerMapping);
    CloseHandle(stopEvent);
    CloseHandle(changedEvent);
    CloseHandle(mapping);
    CloseHandle(instanceMutex);
    return telemetryResult;
}
