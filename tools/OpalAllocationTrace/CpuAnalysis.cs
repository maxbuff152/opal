using System.Text.Json;
using Microsoft.Diagnostics.Tracing.Etlx;
using Microsoft.Diagnostics.Tracing.Parsers.Kernel;

static class CpuAnalysis
{
    public static object Build(TraceLog log, string input, string hash, string selector, int? pid,
        string processName, int conversionLimit, bool conversionTruncated, List<object> notices)
    {
        long indexedEvents = 0, allCpuEvents = 0;
        const int maximumFrames = 128;
        var samples = new CpuSampleAggregator();
        foreach (var ev in log.Events)
        {
            indexedEvents++;
            if (ev is not SampledProfileTraceData sample) continue;
            allCpuEvents++;
            var name = sample.ProcessName;
            if (name.EndsWith(".exe", StringComparison.OrdinalIgnoreCase)) name = name[..^4];
            if (pid is int selected ? sample.ProcessID != selected :
                !name.Equals(processName, StringComparison.OrdinalIgnoreCase)) continue;
            var modules = new List<string>();
            var stack = sample.CallStack();
            for (; stack != null && modules.Count < maximumFrames; stack = stack.Caller)
                modules.Add(stack.CodeAddress.ModuleName);
            // The sampled instruction pointer is the exclusive leaf; stack
            // availability is independent. Do not silently use a root caller.
            var leaf = sample.IntructionPointerCodeAddress()?.ModuleName;
            samples.Add(sample.ProcessID, sample.ThreadID, leaf, modules, stack != null,
                sample.ExecutingDPC, sample.ExecutingISR, sample.Count);
        }
        return new
        {
            SchemaVersion = 1, AnalysisMode = "SampledCpu", AnalyzerPackage = "Microsoft.Diagnostics.Tracing.TraceEvent/3.2.6",
            Input = input, InputSha256 = hash, InputBytes = new FileInfo(input).Length,
            ProcessSelector = selector, GeneratedUtc = DateTime.UtcNow,
            IndexedEventsScanned = indexedEvents, SampledProfileEventsAllProcesses = allCpuEvents,
            MaximumFrames = maximumFrames, TraceEventsLost = log.EventsLost,
            ConversionEventLimit = conversionLimit, ConversionTruncated = conversionTruncated,
            ConversionNotices = notices, Summary = samples.Summary(),
            Limitations = new[]
            {
                "Counts describe recorded SampledProfile events, not elapsed CPU time or a causal CPU percentage. ReportedCountTotal preserves the event Count payload separately without assuming a sampling interval.",
                "Exclusive leaf attribution uses the sampled instruction pointer's trace-mapped module. Inclusive counts count each module once per event, overlap, and must not be summed.",
                "DPC/ISR samples are labeled separately because they may execute in a process context without being work caused by that process.",
                "Stack module presence is association, not ownership or proof of causation. Missing leaf mappings, absent stacks, and truncated stacks remain explicit.",
                "Zero EventsLost does not establish complete history in a circular recording. Nonzero loss or conversion truncation reduces coverage. Indexed counts exclude conversion bookkeeping.",
                "No symbol lookup, event payload strings, command lines, memory dumps, uploads, or live capture. Paths, module mappings, identifiers, flags, and numerical counts only."
            }
        };
    }
}

// Independent numerical aggregation seam, exercised with deterministic samples
// without pretending a synthetic fixture is a recorded Windows CPU trace.
public sealed class CpuSampleAggregator
{
    public long MatchedEvents { get; private set; }
    public long EventsWithStacks { get; private set; }
    public long EventsWithoutStacks => MatchedEvents - EventsWithStacks;
    public long TruncatedStacks { get; private set; }
    public long UnknownLeafEvents { get; private set; }
    public long NonProcessEvents { get; private set; }
    public long ReportedCountTotal { get; private set; }
    public long NonPositiveReportedCountEvents { get; private set; }
    public Dictionary<(string Context, string Module), long> Exclusive { get; } = new();
    public Dictionary<(string Context, string Module), long> Inclusive { get; } = new();
    public Dictionary<(int Pid, int ThreadId, string Context), long> Threads { get; } = new();
    public Dictionary<string, CpuStackGroup> Groups { get; } = new(StringComparer.Ordinal);

    public void Add(int pid, int threadId, string? leafModule, IReadOnlyList<string> frames,
        bool truncated, bool dpc, bool isr, int reportedCount)
    {
        MatchedEvents++;
        bool hasStack = frames.Count != 0;
        if (hasStack) EventsWithStacks++;
        if (truncated) TruncatedStacks++;
        if (dpc || isr) NonProcessEvents++;
        ReportedCountTotal = checked(ReportedCountTotal + reportedCount);
        if (reportedCount <= 0) NonPositiveReportedCountEvents++;
        string context = dpc && isr ? "DPC+ISR" : dpc ? "DPC" : isr ? "ISR" : "Process";
        string leaf = Module(leafModule);
        if (leaf == "<unknown>") UnknownLeafEvents++;
        Increment(Exclusive, (context, leaf));
        Increment(Threads, (pid, threadId, context));
        var modules = new List<string>();
        foreach (var raw in frames)
        {
            var module = Module(raw);
            if (modules.Count == 0 || modules[^1] != module) modules.Add(module);
        }
        // Include a known sampled leaf even when its accompanying stack is
        // missing or starts at another frame. Preserve missing-stack coverage.
        var inclusive = new HashSet<string>(modules, StringComparer.Ordinal);
        inclusive.Add(leaf);
        foreach (var module in inclusive) Increment(Inclusive, (context, module));
        if (!hasStack) modules.Add("<no-stack>");
        var key = JsonSerializer.Serialize(new { pid, context, leaf, modules, truncated });
        if (!Groups.TryGetValue(key, out var group))
            Groups.Add(key, group = new CpuStackGroup(pid, context, leaf, modules.ToArray(), truncated));
        group.Count++;
    }

    public object Summary() => new
    {
        MatchedEvents, EventsWithStacks, EventsWithoutStacks, TruncatedStacks, UnknownLeafEvents,
        NonProcessEvents, ReportedCountTotal, NonPositiveReportedCountEvents,
        ExclusiveLeafModuleCounts = Exclusive.OrderByDescending(x => x.Value).Select(x => new { x.Key.Context, x.Key.Module, Count = x.Value }),
        InclusiveModuleCounts = Inclusive.OrderByDescending(x => x.Value).Select(x => new { x.Key.Context, x.Key.Module, Count = x.Value }),
        ThreadCounts = Threads.OrderByDescending(x => x.Value).Select(x => new { ProcessId = x.Key.Pid, x.Key.ThreadId, x.Key.Context, Count = x.Value }),
        ModuleStackGroups = Groups.Values.OrderByDescending(x => x.Count)
    };

    private static string Module(string? value) => string.IsNullOrEmpty(value) ? "<unknown>" : value.ToLowerInvariant();
    private static void Increment<TKey>(Dictionary<TKey, long> counts, TKey key) where TKey : notnull
    { counts.TryGetValue(key, out var previous); counts[key] = checked(previous + 1); }
}

public sealed record CpuStackGroup(int ProcessId, string Context, string LeafModule, string[] Modules, bool StackTruncated)
{
    public long Count { get; set; }
}
