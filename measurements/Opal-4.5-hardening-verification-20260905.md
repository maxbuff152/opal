# Opal 4.5 hardening verification — September 5, 2026

The recommended reliability repairs and a targeted status-write optimization are implemented and installed locally. Sixteen validation scripts pass. Both final controlled rounds passed their runtime and attachment checks. **Current performance acceptance remains failed because Explorer's working-set overhead exceeds the unchanged 16-MB limit.** CPU and private-commit overhead pass.

## Installed changes

- Telemetry waits now have a sampling deadline, allowing fallback/recovery when the publisher stops signaling. An abandoned odd seqlock sequence also reaches the throttled companion restart helper.
- Media's four asynchronous acquisition/read paths observe cancellation and deadlines instead of using unconditional blocking `.get()` calls. No late completion callback can call into an unloaded DLL. Shutdown still waits for the worker's actual exit.
- Weather accepts only a complete successful response, rejects partial/empty/oversized bodies, caps the body at 16 KiB, and checks cancellation and an overall deadline between operations. Shared mappings/events remain alive until the weather worker exits.
- Performance acceptance requires current build identity, freshness, sufficient paired samples, finite available metrics, valid round positions, real elapsed time, and recomputed CPU arithmetic/medians. Missing metrics and historical builds cannot pass as current evidence.
- A separate same-process soak detects sustained handle/GDI/USER growth using early/middle/late windows. It rejects missing counters, replaced processes, repeated/sparse timestamps, wrong builds, and short runs.
- The first live benchmark exposed a cold-start initialization failure. Unified Performance now relies on the shell's existing late-attachment poll when Taskbar.View is absent, without the redundant loader-hook dependency. Runtime state stays inactive until initialization succeeds; enabled initialization failures no longer count as healthy. The exact original hook API error was not captured.
- The metrics worker publishes runtime status on transitions, with a refresh due after 30 seconds on the next worker iteration. Enabled settings changes preserve the worker's suspension status; disabling publishes inactive after the worker drains. This avoids unchanged INI/registry writes every sample.

## Controlled Explorer overhead

Two rounds in ABBA then BAAB order, four samples per configuration, 45 seconds settling and 15 seconds sampling per leg. Each enabled leg verified the exact installed DLL and widget attachment; stock legs verified DLL absence. Original settings were restored afterward.

| Metric | Stock median | Full Opal median | Added vs stock | Existing limit | Result |
|---|---:|---:|---:|---:|---|
| CPU, % of one core | 0.000 | 0.156 | +0.156 | +0.250 | Pass |
| CPU, % of machine | 0.0000 | 0.0065 | +0.0065 | Equivalent to one-core limit | Pass |
| Private commit | 313.08 MB | 326.52 MB | +13.44 MB | +20 MB | Pass |
| Working set | 363.44 MB | 386.22 MB | +22.78 MB | +16 MB | **Fail** |
| Handles | 5,361 | 5,486 | +125 | Diagnostic | — |
| GDI objects | 304 | 304 | 0 | Diagnostic | — |
| USER objects | 440 | 461 | +21 | Diagnostic | — |
| Threads | 204 | 216.5 | +12.5 | Diagnostic | — |

The prior completed hardening run, before the status-write optimization, measured +0.364% of one core, +6.98 MB private commit, and +19.22 MB working set. Its CPU and working-set checks failed. Both receipts are retained. The final CPU result is lower and inside budget; these short runs do not establish a precise causal percentage improvement. Memory varied between runs. No limit was relaxed, and no memory trimming was used to manufacture a pass.

This measures Explorer overhead on the active desktop. It does not include stock-relative companion/Windhawk costs or certify every shell host. A zero CPU median is a short-window measurement, not a claim that stock Windows never uses CPU.

## Soak and final live readback

The same-process soak completed 21 samples over 300.60 seconds. Explorer,
Maxwell.Shell.Core, and Windhawk retained their PIDs throughout. The sustained
object-growth gate passed; this is not a general memory/leak pass.

| Process | Private memory, start → end | Handle delta | GDI delta | USER delta | Average CPU, % of machine |
|---|---:|---:|---:|---:|---:|
| Explorer, PID 44760 | 317.68 → 410.18 MB | -856 | +2 | -64 | 0.0217 |
| Companion, PID 17268 | 8.81 → 8.73 MB | 0 | 0 | 0 | 0.0024 |
| Windhawk, PID 42584 | 2.18 → 2.18 MB | 0 | 0 | 0 | 0.0000 |

**Explorer private memory increased 92.50 MB net.** It stayed around 311–318 MB
through the first 286 seconds, then increased about 99 MB at the final sample.
A readback at 00:52:28 still showed 410.41 MB. This repeats the scale of the
earlier audit's unresolved observation. There are no allocation stacks or
matched stock soak to attribute the increase to Opal or label it a leak.

After the soak, the final-build live recovery test exited the companion in
7 ms and automatically relaunched it in 1.06 seconds (PID 17268 → 48360).
Fresh attachment readback passed for both widgets. Taskbar strips on both
displays were visually inspected at 00:45:47; Media, Performance, and weather
rendered. Both component health files reported zero crashes and no quarantine.

## Validation and recoverability

- All 16 scripts passed, including the existing suite, attachment, companion, weather retry, routing, taskbar resilience, recovery, rollback traversal, density, unified control, and SafeDock tests.
- New production-code regressions cover asynchronous failures/deadlines and recovery integration (20 assertions), invalid performance receipts (16 cases), soak evidence (8 cases), cold-start hook behavior (2 cases), and runtime-publication/settings transitions (10 cases).
- The earlier and final-build live companion exit checks both recovered automatically; the final result is 1.06 seconds.
- Two review axes examined code correctness and evidence validity. Their findings were corrected and rechecked. No remaining blocker was reported in the reviewed changes.
- Three implementation commits were scanned with Gitleaks; no secrets were found. No new third-party dependencies were added. The existing compiler warning about `DllGetClassObject` export redeclaration remains.
- The installer preserved 110 recognized settings and created rollback bundles. The original pre-change bundle is `C:\Users\maxwe\AppData\Local\Maxwell\Opal\rollback-20260905-000935`; the final install's safety bundle is `rollback-20260905-003406`. Nothing was pushed or published externally. Earlier untracked evidence was preserved.

Current DLL SHA-256: `A3815E7DCA4E7C93FF631838CC033C37B8EFBE5673F88F72138CDB4434C6DCCF`.
Companion SHA-256: `CC167B72241E37A504B2A460F8A7A421937F75B669753A299198C31D7FE9CAA2`.
Implementation commits: `be042f9`, `95cc5e0`, `2c8636e`.

## Remaining limits

The working-set excess and roughly 99-MB late Explorer allocation step need allocation/module profiling before claiming full performance acceptance. The five-minute observational soak does not establish indefinite leak freedom. Physical display transitions, sleep/resume, live network disruption, repeated widget open/close cycles, and an unresponsive real media provider were not exercised in this pass; deterministic failure tests cover the repaired I/O and provider paths. Cancellation is cooperative between synchronous Windows calls and is not a hard shutdown guarantee if such a call itself stalls. The 100-ms sliced shell helper waits remain a lower-priority optimization candidate.

## Evidence

- [Final paired samples](package-cost-opal45-final-20260905-abba.json) and [budget result](opal45-final-budget-20260905.json)
- [Prior completed comparison](package-cost-opal45-before-speed-20260905-abba.json) and [prior budget result](opal45-before-speed-budget-20260905.json)
- [Cold-start failure evidence](opal45-hardening-cold-start-failure-20260905.json)
- [Test results](opal45-hardening-tests-20260905.json), [build identity](opal45-hardening-build-20260905.json), [installation](opal45-hardening-install-20260905.json)
- [Soak samples](opal45-final-soak-20260905.json) and [soak result](opal45-final-soak-result-20260905.json)
- [Final live recovery](opal45-final-live-recovery-20260905.json) and [attachment readback](opal45-final-attachment-20260905.json)
- [Original audit](Opal-4.5-performance-reliability-audit-20260904.md)
