using System.Security.Cryptography;
using System.Text.Json;
using Microsoft.Diagnostics.Tracing.Etlx;
using Microsoft.Diagnostics.Tracing.Parsers.Kernel;

bool cpuMode = args.FirstOrDefault() == "--cpu";
if (cpuMode) args = args[1..];
if (args.Length is < 2 or > 3)
{
    Console.Error.WriteLine("Usage: OpalAllocationTrace [--cpu] <local-input.etl> <new-local-output.json> [process-name|pid:123]");
    return 2;
}

string? temporaryIndex = null;
try
{
    var input = LocalPath(args[0], ".etl");
    var output = LocalPath(args[1], ".json");
    if (!File.Exists(input)) throw new ArgumentException("Input ETL does not exist.");
    if (File.Exists(output)) throw new ArgumentException("Output already exists; choose a new evidence filename.");
    // Hold a read-only sharing lock so a still-running capture cannot change
    // the evidence between indexing and hashing.
    using var inputLock = new FileStream(input, FileMode.Open, FileAccess.Read, FileShare.Read);
    var hash = Convert.ToHexString(SHA256.HashData(inputLock));
    var selector = args.Length == 3 ? args[2] : "explorer";
    int? selectedPid = null;
    if (selector.StartsWith("pid:", StringComparison.OrdinalIgnoreCase))
    {
        if (!int.TryParse(selector[4..], out var pid) || pid <= 0)
            throw new ArgumentException("PID selector must contain a positive integer.");
        selectedPid = pid;
    }
    else if (string.IsNullOrWhiteSpace(selector) || selector.IndexOfAny(['/', '\\', ':']) >= 0)
        throw new ArgumentException("Process selector must be an executable basename or pid:123.");
    var processName = WithoutExe(selector);
    const long largeThreshold = 8L * 1024 * 1024;
    const int maximumFrames = 128;
    const int conversionLimit = 10_000_000;
    var notices = new List<object>();
    var conversionTruncated = false;
    var options = new TraceLogOptions
    {
        ShouldResolveSymbols = _ => false,
        LocalSymbolsOnly = true,
        AlwaysResolveSymbols = false,
        ConversionLog = TextWriter.Null,
        MaxEventCount = conversionLimit,
        OnLostEvents = (truncated, lost, indexed) =>
        {
            conversionTruncated |= truncated;
            notices.Add(new { ConversionTruncated = truncated, LostEvents = lost, IndexedEvents = indexed });
        }
    };
    // Never use/rewrite an adjacent ETLX, which may have different conversion
    // options. Fresh private temporary indexing keeps the source ETL unchanged.
    temporaryIndex = Path.Combine(Path.GetTempPath(), "OpalAllocationTrace-" + Guid.NewGuid().ToString("N") + ".etlx");
    TraceLog.CreateFromEventTraceLogFile(input, temporaryIndex, options);
    using var log = new TraceLog(temporaryIndex);
    if (cpuMode)
    {
        WriteReport(output, CpuAnalysis.Build(log, input, hash, selector, selectedPid,
            processName, conversionLimit, conversionTruncated, notices));
        return 0;
    }
    long indexedEvents = 0, virtualEventsAllProcesses = 0, matched = 0, stacked = 0, truncatedStacks = 0;
    long allocationEvents = 0, allocationBytes = 0, belowThresholdEvents = 0;
    var totals = new Dictionary<(int Pid, string Event, string Flags, string Kind), Counter>();
    var moduleTotals = new Dictionary<(string Kind, string Module), Counter>();
    var stackTotals = new Dictionary<string, StackGroup>(StringComparer.Ordinal);
    var largeEvents = new List<object>();
    foreach (var ev in log.Events)
    {
        indexedEvents++;
        if (ev is not VirtualAllocTraceData v) continue;
        virtualEventsAllProcesses++;
        if (selectedPid is int pid ? v.ProcessID != pid :
            !WithoutExe(v.ProcessName).Equals(processName, StringComparison.OrdinalIgnoreCase)) continue;
        if (v.Length < 0) throw new InvalidDataException("Negative allocation event length; refusing misleading totals.");
        matched++;
        var flags = (int)v.Flags;
        var kind = (flags & 0xC000) != 0 ? "Deallocation" : (flags & 0x3000) != 0 ? "Allocation" : "Other";
        if (kind == "Allocation")
        {
            allocationEvents++;
            allocationBytes = checked(allocationBytes + v.Length);
            if (v.Length < largeThreshold) belowThresholdEvents++;
        }
        var frames = new List<Frame>();
        var stack = ev.CallStack();
        for (; stack != null && frames.Count < maximumFrames; stack = stack.Caller)
        {
            var module = stack.CodeAddress.ModuleName;
            frames.Add(new Frame(string.IsNullOrEmpty(module) ? "<unknown>" : module,
                                 "0x" + stack.CodeAddress.Address.ToString("X")));
        }
        if (frames.Count > 0) stacked++;
        bool stackTruncated = stack != null;
        if (stackTruncated) truncatedStacks++;
        var modules = new List<string>();
        foreach (var frame in frames)
            if (modules.Count == 0 || !modules[^1].Equals(frame.Module, StringComparison.OrdinalIgnoreCase)) modules.Add(frame.Module);
        if (modules.Count == 0) modules.Add("<no-stack>");
        Add(totals, (v.ProcessID, v.EventName, v.Flags.ToString(), kind), v.Length, frames.Count > 0);
        // Inclusive attribution: count each module once per event, regardless
        // of repeated frames. These module rows must never be summed together.
        foreach (var module in modules.Distinct(StringComparer.OrdinalIgnoreCase))
            Add(moduleTotals, (kind, module), v.Length, frames.Count > 0);
        var stackKey = JsonSerializer.Serialize(new { v.ProcessID, v.EventName, Flags = flags, Modules = modules, StackTruncated = stackTruncated });
        if (!stackTotals.TryGetValue(stackKey, out var group))
        {
            group = new StackGroup(v.ProcessID, v.EventName, v.Flags.ToString(), kind, modules.ToArray(), stackTruncated);
            stackTotals.Add(stackKey, group);
        }
        group.Totals.Add(v.Length, frames.Count > 0);
        if (v.Length >= largeThreshold)
            largeEvents.Add(new { v.ProcessID, v.TimeStampRelativeMSec, v.EventName, Flags = v.Flags.ToString(), Kind = kind,
                Bytes = v.Length, BaseAddress = "0x" + v.BaseAddr.ToString("X"), StackTruncated = stackTruncated, Frames = frames });
    }
    var report = new
    {
        SchemaVersion = 1, AnalyzerPackage = "Microsoft.Diagnostics.Tracing.TraceEvent/3.2.6",
        Input = input, InputSha256 = hash, InputBytes = new FileInfo(input).Length,
        ProcessSelector = selector, GeneratedUtc = DateTime.UtcNow,
        IndexedEventsScanned = indexedEvents, VirtualAllocationEventsAllProcesses = virtualEventsAllProcesses,
        MatchedEvents = matched, EventsWithStacks = stacked, EventsWithoutStacks = matched - stacked,
        AllocationEvents = allocationEvents, GrossAllocationEventBytes = allocationBytes,
        AllocationEventsBelowLargeThreshold = belowThresholdEvents,
        LargeEventThresholdBytes = largeThreshold, MaximumFrames = maximumFrames, TruncatedStacks = truncatedStacks,
        TraceEventsLost = log.EventsLost, ConversionEventLimit = conversionLimit, ConversionTruncated = conversionTruncated,
        ConversionNotices = notices,
        Totals = totals.OrderByDescending(x => x.Value.GrossEventBytes).Select(x => new
            { ProcessId = x.Key.Pid, x.Key.Event, x.Key.Flags, x.Key.Kind, x.Value.Count, x.Value.GrossEventBytes, x.Value.EventsWithStacks }),
        InclusiveModuleTotals = moduleTotals.OrderByDescending(x => x.Value.GrossEventBytes).Select(x => new
            { x.Key.Kind, x.Key.Module, x.Value.Count, x.Value.GrossEventBytes, x.Value.EventsWithStacks }),
        ModuleStackGroups = stackTotals.Values.OrderByDescending(x => x.Totals.GrossEventBytes),
        LargeEvents = largeEvents,
        Limitations = new[]
        {
            "Gross event bytes are address-span activity, not outstanding/live commit, working set, or leak size. Reserve/commit may overlap; frees are not subtracted.",
            "All matched events, including those below 8 MiB, contribute to totals and module-stack groups. Inclusive module rows overlap and must not be summed.",
            "Module presence is stack association, not ownership or proof of causation. Unknown modules and absent/truncated stacks remain explicit.",
            "Circular recording may omit older history even when EventsLost is zero. Nonzero lost events or conversion truncation make coverage incomplete.",
            "Indexed event counts exclude TraceEvent bookkeeping removed during conversion. Module names/addresses use trace mappings; symbol lookup is disabled.",
            "Output contains only numeric/time/path, event/flag, and module metadata. No payload strings, command lines, memory dumps, capture, upload, or symbol-network access."
        }
    };
    WriteReport(output, report);
    return 0;
}
catch (Exception error)
{
    Console.Error.WriteLine(error.GetType().Name + ": " + error.Message);
    return 1;
}
finally
{
    if (temporaryIndex != null && File.Exists(temporaryIndex)) File.Delete(temporaryIndex);
}

static string LocalPath(string value, string extension)
{
    var full = Path.GetFullPath(value);
    if (full.StartsWith("\\\\", StringComparison.Ordinal) || new DriveInfo(Path.GetPathRoot(full)!).DriveType == DriveType.Network)
        throw new ArgumentException("Only local filesystem paths are supported.");
    if (!Path.GetExtension(full).Equals(extension, StringComparison.OrdinalIgnoreCase))
        throw new ArgumentException("Expected " + extension + " path.");
    return full;
}

static string WithoutExe(string value) => value.EndsWith(".exe", StringComparison.OrdinalIgnoreCase) ? value[..^4] : value;

static void WriteReport(string output, object report)
{
    Directory.CreateDirectory(Path.GetDirectoryName(output)!);
    using (var stream = new FileStream(output, FileMode.CreateNew, FileAccess.Write, FileShare.None))
        JsonSerializer.Serialize(stream, report, new JsonSerializerOptions { WriteIndented = true });
    Console.WriteLine(output);
}

static void Add<TKey>(Dictionary<TKey, Counter> table, TKey key, long bytes, bool stacked) where TKey : notnull
{
    if (!table.TryGetValue(key, out var counter)) table.Add(key, counter = new Counter());
    counter.Add(bytes, stacked);
}

record Frame(string Module, string Address);
record StackGroup(int ProcessId, string Event, string Flags, string Kind, string[] Modules, bool StackTruncated)
{
    public Counter Totals { get; } = new();
}
sealed class Counter
{
    public long Count { get; private set; }
    public long GrossEventBytes { get; private set; }
    public long EventsWithStacks { get; private set; }
    public void Add(long bytes, bool stacked)
    {
        Count++;
        GrossEventBytes = checked(GrossEventBytes + bytes);
        if (stacked) EventsWithStacks++;
    }
}
