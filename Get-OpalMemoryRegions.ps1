# Read-only address-space metadata. This helper never reads process-memory bytes.
function Initialize-OpalMemoryRegionReader {
    if ('OpalMemoryRegionReader' -as [type]) { return }
    Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Runtime.InteropServices;

public static class OpalMemoryRegionReader {
    [StructLayout(LayoutKind.Sequential)] struct SystemInfo {
        public ushort Architecture, Reserved;
        public uint PageSize;
        public IntPtr MinimumAddress, MaximumAddress;
        public UIntPtr ActiveProcessorMask;
        public uint ProcessorCount, ProcessorType, AllocationGranularity;
        public ushort ProcessorLevel, ProcessorRevision;
    }
    [StructLayout(LayoutKind.Explicit, Size=48)] struct MemoryInfo {
        [FieldOffset(0)] public ulong BaseAddress;
        [FieldOffset(8)] public ulong AllocationBase;
        [FieldOffset(16)] public uint AllocationProtect;
        [FieldOffset(24)] public ulong RegionSize;
        [FieldOffset(32)] public uint State;
        [FieldOffset(36)] public uint Protect;
        [FieldOffset(40)] public uint Type;
    }
    public sealed class RegionClass {
        public string State { get; set; }
        public string Type { get; set; }
        public uint Protection { get; set; }
        public ulong Bytes { get; set; }
        public int Regions { get; set; }
    }
    [DllImport("kernel32.dll", SetLastError=true)] static extern IntPtr OpenProcess(uint access, bool inherit, int pid);
    [DllImport("kernel32.dll", SetLastError=true)] static extern UIntPtr VirtualQueryEx(IntPtr process, IntPtr address, out MemoryInfo info, UIntPtr size);
    [DllImport("kernel32.dll")] static extern void GetNativeSystemInfo(out SystemInfo info);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr handle);
    public static RegionClass[] Capture(int pid) {
        if (IntPtr.Size != 8) throw new InvalidOperationException("64-bit PowerShell is required.");
        IntPtr process = OpenProcess(0x0400, false, pid); // PROCESS_QUERY_INFORMATION only
        if (process == IntPtr.Zero) throw new Win32Exception(Marshal.GetLastWin32Error());
        try {
            SystemInfo system; GetNativeSystemInfo(out system);
            ulong address = 0, maximum = (ulong)system.MaximumAddress.ToInt64();
            var groups = new Dictionary<string, RegionClass>();
            while (address <= maximum) {
                MemoryInfo info;
                if (VirtualQueryEx(process, new IntPtr((long)address), out info, new UIntPtr(48)).ToUInt64() != 48)
                    throw new Win32Exception(Marshal.GetLastWin32Error(), "Incomplete address-space metadata capture.");
                if (info.RegionSize == 0 || info.BaseAddress > ulong.MaxValue - info.RegionSize)
                    throw new InvalidOperationException("Invalid address-space region.");
                string state = info.State == 0x1000 ? "Committed" : info.State == 0x2000 ? "Reserved" : info.State == 0x10000 ? "Free" : "Other";
                string type = info.Type == 0x20000 ? "Private" : info.Type == 0x40000 ? "Mapped" : info.Type == 0x1000000 ? "Image" : "None";
                string key = state + "/" + type + "/" + info.Protect;
                RegionClass item;
                if (!groups.TryGetValue(key, out item)) {
                    item = new RegionClass { State=state, Type=type, Protection=info.Protect };
                    groups.Add(key, item);
                }
                checked { item.Bytes += info.RegionSize; item.Regions++; }
                ulong next = info.BaseAddress + info.RegionSize;
                if (next <= address) throw new InvalidOperationException("Address-space scan did not advance.");
                address = next;
            }
            var result = new RegionClass[groups.Count]; groups.Values.CopyTo(result, 0); return result;
        } finally { CloseHandle(process); }
    }
}
'@
}

function Get-OpalMemoryRegions {
    [CmdletBinding()]
    param([Parameter(Mandatory)][int]$ProcessId)
    Initialize-OpalMemoryRegionReader
    $process = Get-Process -Id $ProcessId -ErrorAction Stop
    $startedAt = $process.StartTime.ToUniversalTime().ToString('o')
    $timer = [Diagnostics.Stopwatch]::StartNew()
    $classes = [OpalMemoryRegionReader]::Capture($ProcessId)
    $modules = @($process.Modules | ForEach-Object {
        [pscustomobject]@{ Name=$_.ModuleName; Path=$_.FileName; MappedImageBytes=$_.ModuleMemorySize }
    })
    $after = Get-Process -Id $ProcessId -ErrorAction Stop
    if ($after.StartTime.ToUniversalTime().ToString('o') -ne $startedAt) { throw 'Process identity changed during memory metadata capture.' }
    [pscustomobject]@{
        CapturedAtUtc=[DateTimeOffset]::UtcNow.ToString('o'); Pid=$ProcessId; StartedAtUtc=$startedAt
        CaptureSeconds=$timer.Elapsed.TotalSeconds; RegionClasses=@($classes); Modules=$modules
        Limitation='Aggregated VirtualQueryEx metadata and loaded-image paths only; no memory bytes, stacks or private dumps. Private committed address-space regions are not identical to the process Private Bytes counter.'
    }
}
