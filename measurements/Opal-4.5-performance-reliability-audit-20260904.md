# Opal 4.5 performance and reliability audit

Opal has low observed CPU use on this machine, but its failure recovery and performance acceptance gate need work. Existing tests pass while isolated adversarial probes reproduce defects. Prioritize recovery and trustworthy acceptance evidence before optimizing normal idle operation.

Audited September 4, 2026, America/Chicago. Source commit: `1797d8d5158c220c531f592439707e8e4a73bb47`. This is an audit, not a production repair or release certification.

## Verified running state

- Live Windhawk mod: `local@opal` 4.5.0. Explorer PID 40912 maps `local_at_opal_4.5.0_owned.dll`.
- Installed DLL SHA-256 matches the build: `D7A5366D62F8D928FB2714C25E5F198011373AEB5D3878A818D0186B5B3D714E`. All 15 recorded source inputs match current source.
- One telemetry companion is running. Its installed binary matches its build, and the live probe reports fresh CPU/RAM telemetry.
- Media and Performance each have fresh attachment proof for the configured primary taskbar.
- Eleven existing test scripts passed: suite, attachment, companion, weather scheduling, monitor routing, taskbar resilience, recovery, rollback traversal, density, unified control, and SafeDock. Some are source assertions or extracted-function simulations, so passing is narrower than full fault tolerance.
- No matching Explorer, companion, or Windhawk Application Error/Hang events (IDs 1000/1002) were found in the three-hour query ending 23:42:36. This does not exclude other error channels or failures outside that interval.
- Machine snapshot: i7-13700HX, 24 logical processors, about 31.7 GiB usable RAM and 14.9 GiB free. C: and D: both had ample free space.

## Observed resource use

Three separate 60-second samples during ordinary desktop activity and this audit:

| Process | Total-machine CPU range | Private memory at sample ends | Working set at sample ends |
| --- | ---: | ---: | ---: |
| Explorer, including Windows shell and injected Opal | 0.013–0.102% | 209.39 / 209.61 / 299.86 MB | 272.23 / 275.52 / 283.95 MB |
| Maxwell.Shell.Core | approximately 0.0022% | 8.86–9.23 MB | 8.95–14.01 MB |
| Windhawk, both processes combined | 0–0.0022% | 3.99–4.11 MB | 26.22–26.28 MB |

CPU calculation: CPU seconds / sample duration / 24 logical processors × 100. Memory uses the script's binary MB convention.

Explorer private commit increased about 90 MB between the second and third sample endpoints. That is an unresolved observation, not an Opal leak attribution. Its handle deltas were +29, -12, and -8 within the respective windows; companion deltas were +6, -2, and 0. Explorer GDI grew by seven objects in the second window and was unchanged in the others. A fourth 30-second follow-up ended at 299.77 MB Explorer private memory, with no handle or GDI growth during that window, so the higher commit persisted. These short, noncontiguous samples cannot establish long-run leak freedom or Opal's incremental memory overhead. Explorer totals include Windows and other activity. The historical 4.0 paired benchmark does not establish 4.5 performance.

## Findings, ordered by practical priority

### 1. High: companion failure can leave computer stats frozen indefinitely

At [maxwell-taskbar-system-info.wh.cpp:2902](D:/Opal/mod/visual-clones/maxwell-taskbar-system-info.wh.cpp:2902), the worker waits on its internal wake event and the companion's data-change event with `INFINITE` whenever the data event exists. It does not include the publisher process handle or a freshness deadline. Publisher liveness and restart checks occur later in `TryReadExternalTelemetry` at line 989, after the wait returns. The attachment health check verifies the XAML views; it does not wake this metrics worker.

Reproduced in an isolated process using the exact production wait block: a one-second active sampling interval did not wake the worker after 1.25 seconds of publisher silence. Only the deliberate internal wake released it. No live companion was killed. The indefinite-wait conclusion follows the code and [Microsoft's wait contract](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitformultipleobjects).

Repair: retain event-driven sampling but add a bounded freshness timeout or wait on publisher termination, then execute stale-data fallback and restart logic. Test silent hangs as well as exits.

### 2. High: a stalled media provider can block disable or unload

[maxwell-opal-media.wh.cpp:984](D:/Opal/mod/visual-clones/maxwell-opal-media.wh.cpp:984) blocks on `TryGetMediaPropertiesAsync().get()`. Session-manager acquisition and artwork reads also use blocking `.get()` calls at lines 877, 609, and 618. `StopWorker` signals an event then waits indefinitely at line 1097; the blocked calls do not observe that event. Live Media disable invokes this path at line 2661, as does component teardown.

This is a source-confirmed unbounded dependency, not a reproduced live Explorer hang. Repair requires cancellation/deadline handling and safe worker lifetime management. Merely timing out the join and unloading would introduce execution in an unloaded DLL.

### 3. High validation gap: the performance gate can approve missing or unrelated evidence

[Test-OpalPerformanceBudget.ps1:36](D:/Opal/Test-OpalPerformanceBudget.ps1:36) casts absent numeric fields to zero and treats missing sample arrays as no growth. There is no required build identity, sample-count/freshness validation, or linkage to the installed artifact.

Reproduced: a JSON receipt containing only scenario names `Stock` and `FullSuite`, with no measurements, returned `passed=True`. The archived 4.0 receipt also passed while 4.5 is installed. Historical analysis is legitimate, but this function cannot certify a current release without validating the expected artifact.

Repair: require schema, finite numeric fields, nonempty sufficient samples, available counters, and an explicit expected build hash/version. Distinguish historical comparisons from current-release acceptance. Leak acceptance should analyze sustained growth rather than treating every positive short-window object change as a leak.

### 4. Medium: failed weather reads can be treated as successful, with no body-size limit

[MaxwellShellCore.cpp:626](D:/Opal/native/Maxwell.Shell.Core/MaxwellShellCore.cpp:626) exits its read loop for either a read error or clean EOF, then accepts any bytes already received. The caller consequently uses the successful ten-minute refresh interval instead of the 30-second failed-fetch interval. The loop also appends arbitrary response length without a total-body cap.

Reproduced with the actual production `FetchUrl` function and deterministic WinINet mocks: seven bytes followed by a connection-aborted error returned `PARTIAL` as success. A 2,097,152-character response was fully accepted. No internet request was made by the probe. [Microsoft requires a successful read with zero bytes to establish complete EOF](https://learn.microsoft.com/en-us/windows/win32/api/wininet/nf-wininet-internetreadfile).

Repair: distinguish error from clean EOF, reject incomplete reads, cap this small weather response, and enforce a total request deadline with cancellation.

### 5. Medium: weather shutdown releases resources before confirmed worker exit

[MaxwellShellCore.cpp:841](D:/Opal/native/Maxwell.Shell.Core/MaxwellShellCore.cpp:841) waits eight seconds for the weather thread, ignores the result, then closes its thread handle and unmaps its request/snapshot memory. The fetch configures 15-second network timeouts, and the read loop has no stop-event checks or total deadline. A pending fetch can therefore outlast the join and resume against released state during teardown.

This is a source-review race, not a captured production crash. [Closing a thread handle does not terminate its thread](https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-closehandle). The process normally exits immediately after cleanup, which narrows the race window but does not make the cleanup order sound.

Repair: cancel the active fetch, confirm worker completion, and only then release shared mappings and events. Add a shutdown-during-download fault test.

## Secondary speed opportunities and remaining evidence

The recovery helper in `maxwell-shell.wh.cpp:1653` implements long sleeps as 100-ms slices, causing roughly ten wakeups per second per active helper. Runtime publication also rewrites five INI fields and two registry values per metrics iteration (`opal-control.h:378`), and attachment checks write timestamp proofs every five seconds. These are candidates for stop-event waits, write-on-change state, and a shared-memory heartbeat. They are not demonstrated dominant costs; current CPU measurements do not justify an aggressive rewrite.

The next performance acceptance run should use a 4.5 build-bound, controlled stock-versus-Opal comparison and a longer resource soak with media changes, widget open/close cycles, network failures, and display transitions. No stock toggle, Explorer restart, physical monitor reconnect, reboot, sleep/resume, or live provider hang was performed in this audit. Visual layout quality and interaction latency were not measured.

Production code, installed binaries, and settings were unchanged. Existing untracked evidence was preserved. The only repository additions are audit evidence and a rerunnable isolated probe under `measurements`; generated probe binaries are under `build/opal45-audit-probes`.

## Evidence

- [Installed/source identity](opal45-audit-identity-20260904.json)
- [Existing test results](opal45-audit-tests-20260904.json)
- [Adversarial results](opal45-audit-adversarial-20260904.json) and [rerunnable probe](Invoke-Opal45AuditProbes.ps1)
- [Resource sample 1](opal45-audit-live-20260904-2338.json), [sample 2](opal45-audit-live-2-20260904.json), [sample 3](opal45-audit-live-3-20260904.json), [30-second follow-up](opal45-audit-live-4-20260904.json)
- [Application event query](opal45-audit-events-20260904.json)
- Microsoft API raw captures, hashes, URLs, retrieval times, and limitations: `D:\LLM Research Repository\01-CAPTURES\2026\2026-09-04\opal45-audit-api-provenance.json`.
