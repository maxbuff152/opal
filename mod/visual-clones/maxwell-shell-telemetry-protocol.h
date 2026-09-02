#pragma once

#include <windows.h>
#include <cstdint>

namespace MaxwellShellTelemetry {

inline constexpr wchar_t kMappingName[] =
    L"Local\\Maxwell.Shell.Telemetry.v1";
inline constexpr wchar_t kChangedEventName[] =
    L"Local\\Maxwell.Shell.TelemetryChanged.v1";
inline constexpr wchar_t kStopEventName[] =
    L"Local\\Maxwell.Shell.Core.Stop.v1";
inline constexpr wchar_t kInstanceMutexName[] =
    L"Local\\Maxwell.Shell.Core.Instance.v1";
// Reader -> publisher channel. The snapshot mapping is opened read-only by
// readers, so the requested cadence and the liveness heartbeat travel in this
// separate small mapping. The publisher creates it; readers open it writable.
inline constexpr wchar_t kReaderMappingName[] =
    L"Local\\Maxwell.Shell.TelemetryReader.v1";
inline constexpr wchar_t kReaderWakeEventName[] =
    L"Local\\Maxwell.Shell.TelemetryReaderWake.v1";

inline constexpr std::uint32_t kMagic = 0x3154534d;  // "MST1"
inline constexpr std::uint16_t kVersion = 1;

enum MetricFlags : std::uint32_t {
    MetricCpu = 1u << 0,
    MetricRam = 1u << 1,
    MetricGpu = 1u << 2,
    MetricVram = 1u << 3,
};

// The writer changes sequence to odd, writes one complete sample, then changes
// it to even. Readers accept only two matching even sequence reads. Keep this
// structure POD and append fields in future protocol versions.
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

// Written by readers with the same seqlock discipline as SnapshotV1.
// desiredIntervalMs is the cadence the reader will actually consume at; the
// publisher honours the request and idles when the heartbeat goes stale, so
// nothing samples on behalf of a widget that is not there.
inline constexpr std::uint32_t kReaderMagic = 0x3152534d;  // "MSR1"
inline constexpr std::uint32_t kMinIntervalMs = 5000;
inline constexpr std::uint32_t kMaxIntervalMs = 30000;
inline constexpr std::uint32_t kIdleIntervalMs = 30000;

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
