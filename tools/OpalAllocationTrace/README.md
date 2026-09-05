# Opal allocation trace summary

This Windows/.NET 10 utility reads an existing local ETL and writes a new JSON summary. It does not start a capture, inspect live processes, read memory dumps, resolve symbols, or upload information. Module names and addresses come from the trace's image mappings. It emits no command lines, arbitrary event payload strings, or captured memory contents.

`Microsoft.Diagnostics.Tracing.TraceEvent` is pinned to exactly **3.2.6**. `packages.lock.json` also pins all eight transitive packages and their content hashes. Restore uses the official NuGet feed configured in this directory. Package restoration and an explicit vulnerability audit require network access; running the restored analyzer does not.

From the repository root:

```powershell
dotnet restore .\tools\OpalAllocationTrace\OpalAllocationTrace.csproj --locked-mode
dotnet build .\tools\OpalAllocationTrace\OpalAllocationTrace.csproj -c Release --no-restore
dotnet .\tools\OpalAllocationTrace\bin\Release\net10.0\OpalAllocationTrace.dll `
  D:\Opal\build\capture.etl D:\Opal\build\capture.summary.json explorer
```

The optional final selector defaults to `explorer`. It accepts an executable basename, with or without `.exe`, or a positive PID such as `pid:1234`. A process name covers every matching PID in the captured interval. PID-only selection does not distinguish reused process IDs across a long trace.

Both paths must be local. The output must be a new `.json` file: the utility refuses to overwrite evidence. The ETL is held read-only during analysis and its SHA-256 is recorded. A fresh ETLX is created in the local temporary directory and removed after analysis; an existing index next to the ETL is neither reused nor changed. Keep raw traces and summaries under ignored `build/`, outside the repository, or in this tool directory where trace extensions are ignored. The original ETL can contain broader metadata than the deliberately limited summary; do not commit it.

The existing capture profile is `config/OpalAllocation.wprp`. It requests process/thread and loader metadata plus virtual-allocation events and stacks. This tool only consumes a completed capture; it does not invoke WPR.

## Reading the result

- `Totals` partitions matched events by PID, event name, flags, and allocation/deallocation kind. Allocation/deallocation classification uses the trace's commit/reserve/decommit/release flags.
- `ModuleStackGroups` includes **every matched event**, including events below 8 MiB. Consecutive identical module frames collapse into one entry. Each event contributes to exactly one group; missing stacks remain explicit.
- `InclusiveModuleTotals` counts an event once for each distinct module in its stack. **These rows overlap; never add their bytes together.** A module's appearance is an association, not proof that it owns an allocation.
- `LargeEvents` separately lists individual events of at least 8 MiB, with bounded module/address stacks. Their bytes are already included in the aggregate tables.
- `GrossAllocationEventBytes` measures address-span activity. It is **not live/outstanding commit, working set, retained memory, or leak size**. Reserve and commit events can cover the same pages, repeated commitments count again, and frees are not subtracted. Live-memory attribution needs additional lifetime analysis and process measurements.
- `TraceEventsLost`, conversion notices, stack coverage, and truncation fields expose incomplete evidence. Zero lost events does not prove complete history in a circular trace. Conversion is capped at 10 million indexed events and reports truncation. Stacks are capped at 128 frames and report truncation independently.

The JSON distinguishes event counts from stack coverage, and never turns missing evidence into an ownership or leak conclusion.

## Optional sampled CPU analysis

Prepend `--cpu` to analyze `SampledProfile` events from an already completed local trace:

```powershell
dotnet .\tools\OpalAllocationTrace\bin\Release\net10.0\OpalAllocationTrace.dll --cpu `
  D:\Opal\build\cpu.etl D:\Opal\build\cpu.summary.json explorer
.\tests\Test-OpalCpuProfile.ps1
# Optional verification against an existing recorded CPU trace:
.\tests\Test-OpalCpuProfile.ps1 -TracePath D:\Opal\build\cpu.etl
```

`config/OpalCpu.wprp` declares a bounded memory collector with **64 x 1024 KB buffers**, process/thread and loader metadata, and sampled-profile events/stacks. This is the collector's configured buffer storage, not a guarantee that total WPR process memory is exactly 64 MiB. The test inspects this profile with `wpr -profiledetails`; it never starts recording.

CPU output separates exclusive sampled-instruction-pointer modules, inclusive module counts, thread counts, and module stack groups. Inclusive rows count each module once per event and overlap. Missing leaf mappings, absent/truncated stacks, trace loss, and conversion truncation remain visible. The sampled instruction pointer can have a known module even when its associated stack is unavailable.

The unit is **recorded events**, not CPU percentage or elapsed processor time. The event's `Count` payload is preserved separately; the analyzer assumes no sample interval or processor-count denominator. DPC and ISR contexts remain separate because kernel work can run in a process context without being caused by that process. Comparing Stock and Opal traces can reveal stack associations; module presence alone does not prove causal overhead.

The deterministic CPU fixture checks numerical aggregation and output using synthetic module lists. It is explicitly not evidence of a successful live capture. An optional ETL argument runs a separate recorded-trace reconciliation check. Allocation mode and its original CLI remain unchanged.

## Validation and dependency audit

```powershell
.\tests\Test-OpalAllocationProfile.ps1 -TracePath D:\Opal\build\capture.etl
dotnet list .\tools\OpalAllocationTrace\OpalAllocationTrace.csproj package --vulnerable --include-transitive --format json
```

The test requires an existing Explorer trace with some stacks and sub-8-MiB events. It builds only this utility, reconciles all-size grouping totals, repeats analysis with an independently selected PID, verifies the original hash, and checks overwrite/network-path rejection. It never starts recording.

On 2026-09-05, the existing local 63,963,136-byte fixture passed 18 assertions: 40 matched virtual-allocation/free events, 23 with stacks, 17 without, zero reported lost events, and no conversion truncation. The transitive audit reported no known vulnerable packages. This is a dated advisory result, not a guarantee against undisclosed vulnerabilities; rerun it when dependencies or advisories change.

Primary references: [TraceLog indexing and conversion](https://github.com/microsoft/perfview/blob/main/src/TraceEvent/TraceLog.cs), [VirtualAllocTraceData and allocation flags](https://github.com/microsoft/perfview/blob/main/src/TraceEvent/Parsers/KernelTraceEventParser.cs), [official TraceEvent 3.2.6 package](https://www.nuget.org/packages/Microsoft.Diagnostics.Tracing.TraceEvent/3.2.6). The installed 3.2.6 XML documentation was also checked for `ShouldResolveSymbols`, `LocalSymbolsOnly`, `OnLostEvents`, and `EventsLost`; unversioned upstream source can move beyond the pinned release.
