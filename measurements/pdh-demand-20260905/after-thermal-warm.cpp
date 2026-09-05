#include <windows.h>
#include <pdh.h>
#include <psapi.h>
#include <chrono>
#include <cstdio>
#include <cstdint>
int opens=0,adds=0,addFailures=0,collects=0,collectFailures=0;
unsigned long lastAddStatus=0,lastCollectStatus=0;
PDH_STATUS ProbeOpen(LPCWSTR s,DWORD_PTR n,PDH_HQUERY* q){++opens;return PdhOpenQueryW(s,n,q);}
PDH_STATUS ProbeAdd(PDH_HQUERY q,LPCWSTR p,DWORD_PTR n,PDH_HCOUNTER* c){++adds;auto s=PdhAddEnglishCounterW(q,p,n,c);if(s){++addFailures;lastAddStatus=s;}return s;}
PDH_STATUS ProbeCollect(PDH_HQUERY q){++collects;auto s=PdhCollectQueryData(q);if(s){++collectFailures;lastCollectStatus=s;}return s;}
#define PdhOpenQueryW ProbeOpen
#define PdhAddEnglishCounterW ProbeAdd
#define PdhCollectQueryData ProbeCollect
#define Wh_Log(...) ((void)0)
enum class TemperatureSource {Auto,WindowsNative,Disabled};
struct ModSettings{TemperatureSource temperatureSource=TemperatureSource::Auto;};
PDH_HQUERY g_pdhQuery=nullptr;
HANDLE g_pdhCompletionEvent=nullptr;
PDH_HCOUNTER g_gpuCounter=nullptr,g_vramCounter=nullptr,g_sharedVramCounter=nullptr,g_thermalZoneCounter=nullptr;
std::chrono::steady_clock::time_point g_nextPdhCounterRetry{};
constexpr auto kPdhCounterRetryInterval=std::chrono::seconds(30);
struct Sample {size_t privateBytes,workingSet;DWORD handles;uint64_t cpu;};
uint64_t Ft(FILETIME f){ULARGE_INTEGER x{};x.LowPart=f.dwLowDateTime;x.HighPart=f.dwHighDateTime;return x.QuadPart;}
Sample Take(){PROCESS_MEMORY_COUNTERS_EX m{};m.cb=sizeof(m);GetProcessMemoryInfo(GetCurrentProcess(),(PROCESS_MEMORY_COUNTERS*)&m,sizeof(m));DWORD h=0;GetProcessHandleCount(GetCurrentProcess(),&h);FILETIME a{},b{},k{},u{};GetProcessTimes(GetCurrentProcess(),&a,&b,&k,&u);return {m.PrivateUsage,m.WorkingSetSize,h,Ft(k)+Ft(u)};}
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


int main(){for(int trial=0;trial<8;++trial){ opens=adds=addFailures=collects=collectFailures=0;lastAddStatus=lastCollectStatus=0;
 auto baseline=Take();auto t0=std::chrono::steady_clock::now();
 ModSettings settings;
 EnsurePdhQuery(settings,PdhQueryDemand::ThermalOnly);
 if(g_pdhQuery)PdhCollectQueryData(g_pdhQuery);
 auto active=Take();int counters=(g_gpuCounter?1:0)+(g_vramCounter?1:0)+(g_sharedVramCounter?1:0)+(g_thermalZoneCounter?1:0);
 ClosePdhQuery();auto end=Take();double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count();
 printf("{\"wallMs\":%.4f,\"cpuMs\":%.4f,\"baselinePrivate\":%llu,\"activePrivateDelta\":%lld,\"endPrivateDelta\":%lld,\"activeWorkingSetDelta\":%lld,\"endWorkingSetDelta\":%lld,\"activeHandleDelta\":%ld,\"endHandleDelta\":%ld,\"counters\":%d,\"opens\":%d,\"adds\":%d,\"addFailures\":%d,\"lastAddStatus\":%lu,\"collects\":%d,\"collectFailures\":%d,\"lastCollectStatus\":%lu}\n",ms,(end.cpu-baseline.cpu)/10000.0,(unsigned long long)baseline.privateBytes,(long long)active.privateBytes-baseline.privateBytes,(long long)end.privateBytes-baseline.privateBytes,(long long)active.workingSet-baseline.workingSet,(long long)end.workingSet-baseline.workingSet,(long)active.handles-(long)baseline.handles,(long)end.handles-(long)baseline.handles,counters,opens,adds,addFailures,lastAddStatus,collects,collectFailures,lastCollectStatus);
}
}