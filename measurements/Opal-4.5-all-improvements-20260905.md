# Opal 4.5 approved performance improvements

Date: September 5, 2026. Owner: Codex. This implements the approved follow-up to `Opal-4.5-performance-debate-20260905.md`. Local build/install, tests, bounded Explorer restarts, and the existing Slack discussion were authorized by Maxwell.

## Delivered changes

| Recommendation | Implementation and verification |
|---|---|
| One collector per signal | Preserved the preceding thermal-only PDH change. External temperature collection does not duplicate the companion's GPU counters. Production demand tests remain passing. |
| Count participating processes | Preserved shared-interval Explorer, companion, and Windhawk CPU/private sampling. The new long experiment verifies the same process-name membership across legs. Working sets remain separate. |
| Show paired variation | Preserved the paired-round analyzer and unchanged acceptance gate. Added a separate first/last-minute long-run comparison that cannot turn an incomplete or unrestored run into valid evidence. |
| Investigate allocation ownership | Added four counterbalanced eight-minute Stock/FullSuite legs, five-second samples, and read-only region/module metadata at startup, completion, and private-memory steps of at least 64 MB. Added a bounded WPR profile and reproducible local ETL analyzer with module stack groups, stack coverage, and loss/truncation reporting. |
| Replace sliced recovery sleeps | Replaced 100-ms unload polling with a manual-reset stop event. Both recovery threads drain before event/component teardown, including retained-DLL reload; sent-message pumping is preserved. Production lifecycle tests cover creation failure, stop-before-wait, reload, failed drain, and teardown ordering. |
| Measure response time | Added opt-in, thread-safe storage for the latest 64 numeric samples per metric. Existing Copy hardware report exports attachment/recovery work, media dispatch/update/UI timing, and hardware-panel open/refresh timing. Added a strict analyzer with caller-supplied p95 limits and a default minimum of 20 samples. Missing evidence cannot pass. |
| Let demand govern work | `media.showArtwork=false` now skips thumbnail acquisition/decoding, releases retained snapshot artwork, and rejects republishing an in-flight read after disabling. Re-enabling refreshes. Compact mirrors skip unchanged metadata/accessibility writes while retaining geometry and attachment checks. |
| Isolate Media only with demonstrated benefit | The condition remains evidence-dependent. The trace below does not demonstrate a Media stall or retained allocation owner. No Media process migration was introduced. Existing cooperative cancellation cannot preempt a permanently stuck synchronous provider call. |

## Installed artifact

The integrated build and installation succeeded. DLL SHA-256: `5389ACBE2F1DDB7BD5876D8DBE4B592E6685490B9BBBE42B862E444596662E05`. Companion SHA-256 remains `CC167B72241E37A504B2A460F8A7A421937F75B669753A299198C31D7FE9CAA2`.

Installation preserved 110 existing settings, including normal full-feature operation and artwork enabled. Collect performance timings defaults off. Rollback bundle: `C:\Users\maxwe\AppData\Local\Maxwell\Opal\rollback-20260905-015033`. The expected new direct PE import is Windows' `api-ms-win-core-profile-l1-1-0.dll`, used for timing. No third-party production dependency was added.

## Allocation trace

The initial approximately seven-minute named WPR capture used the preceding installed `83EE94...` build, before the new installation. It used 64 one-MiB buffers, was stopped successfully, and was confirmed inactive afterward. The raw trace remains local in ignored `build/`; no memory dump, heap-tracing registry configuration, symbol lookup, or upload was performed.

The reproducible analyzer found 40 Explorer virtual-allocation/free events: 23 with stacks, 17 without, zero reported lost events, and no conversion truncation. All 23 allocation events were below 8 MiB. Opal appeared in 14 allocation stacks covering 7,397,376 gross event bytes. This is inclusive stack association, not retained commit, component-level ownership, or evidence of a leak. Reserve/commit spans can overlap, and a circular trace can omit earlier history even with zero lost events.

The standalone analyzer uses Microsoft TraceEvent 3.2.6, exactly pinned with all eight transitive packages in a lockfile. Its build and 18 real-ETL checks passed. The dated NuGet advisory audit found no known vulnerabilities. This dependency belongs to the offline diagnostic utility, not the Opal DLL or companion.

## Response-time evidence boundary

The numeric collector and analyzer have executable regression coverage, including concurrent publishers, cross-translation-unit storage, bounded retention, settings epochs, malformed exports, and insufficient samples. No live interaction latency claim follows from those tests. The available Computer Use surface did not expose the taskbar as a targetable window, so an actual hardware-panel/media interaction export was not obtained in this pass. Diagnostics remain off for normal operation.

The labels intentionally distinguish the next applied media update from provider acknowledgment and panel `Opened` from pixel presentation. Attachment timing covers only a successful verification/repair pass and excludes earlier retry delays. The exported text lacks build/PID/timestamp/raw samples, so its analyzer is descriptive and cannot serve as the installed-build release gate.

## Review

Meitner implemented the recovery event lifecycle; Dewey implemented media demand and numerical diagnostics; Arendt implemented the matched-run evidence tools. Meitner and Dewey independently reviewed each other's production changes and reported no concrete introduced defects. Codex integrated, built, installed, and owns live verification. These are local agent reviews; no additional independent Slack bot participation is claimed.

## Long-run results and final verification

The first experiment is invalid and preserved as `opal-all-improvements-matched-soak-20260905.json`. Its first Stock leg completed 485 seconds and included a 144.93 MB startup step with Opal unloaded. The first FullSuite leg stopped after 290.50 seconds when its generic runtime check failed; it had not observed a 64 MB step. Restoration verified the original settings and attachment in Explorer PID 40236. Those partial observations are not a completed matched comparison.

The initial exception did not record whether the mismatching field was the Disabled flag or DLL aliases. No crash/hang event was found for sampled PID 6688 in the scoped interval; its attachment file remained active at 02:05:48. A later, different startup PID 40204 faulted in Windows.CloudStore.dll at 02:05:52. It would be incorrect to attribute the first mismatch to that later crash. Scheduled guard readbacks do not establish that a guard caused the initial mismatch. The sampler now preserves exact mismatch state, serializes empty aliases correctly, and gives Windows automatic recovery a grace period before a single conditional explicit launch. The acceptance benchmark also freezes Explorer PID and start time before settling and rejects replacement during measurement.

The separate CPU capture completed with zero reported dropped events and the same Explorer PID before/after. Of 1,017 recorded samples, 996 had stacks, 21 did not, 415 had an unknown leaf mapping, and 69 were DPC/ISR contexts. UIAutomationCore appeared in 558 process-context stacks, Opal in 65, and MediaControl in five. These inclusive counts overlap and are not CPU percentages. They support investigating accessibility/inspection activity, but do not identify its initiator or prove observer causality. The capture is preserved locally, and the production CPU analyzer passed deterministic and real-trace reconciliation tests.

The rerun completed all four eight-minute BAAB legs, but its final restoration check failed during a startup crash. The original receipt remains `Complete=true`, `Valid=false`, and `Restoration.Verified=false`; the strict analyzer rejects it. No values in that receipt were repaired or relabeled. A separate fresh readback at 07:51:39 UTC verified restored FullSuite settings, the installed hash and both attachments in Explorer PID 25544.

The following are independently calculated descriptive observations from the completed raw legs, not a passing experiment or an acceptance decision:

| Leg | Scenario | Explorer private, first → last 60-second mean | Later private step ≥64 MB |
|---|---|---:|---|
| 1 | FullSuite | 208.54 → 283.29 MB | +91.31 MB at 210.58 s |
| 2 | Stock | 177.73 → 269.10 MB | +92.12 MB at 330.34 s |
| 3 | Stock | 171.35 → 235.79 MB | +103.16 MB at 445.31 s |
| 4 | FullSuite | 203.59 → 186.30 MB | None |

Stock additionally had large startup steps near five seconds. Each recorded leg retained one Explorer lifetime and runtime-verification flags. The late private-memory steps therefore are not unique to Opal, and did not repeat in both FullSuite legs. Equal-weight late-window working-set means were 245.41 MB Stock and 323.38 MB FullSuite; these mixed observations do not establish a speed or memory improvement.

The restoration crash was Explorer PID 23056, approximately 3.245 seconds after creation, in Windows.CloudStore.dll 10.0.26100.9278 at offset b96e6, exception c0000409 with fast-fail data 7 (`FAST_FAIL_FATAL_APP_EXIT`). Its signature matches the earlier startup PID 40204 event. This time no explicit Explorer launch was requested and no multiplicity was observed. Opal and Windhawk were loaded, but the inspected metadata contains no failing stack or initiating caller. Neither the faulting Windows module nor the loaded Opal module establishes causality. This remains an unresolved startup reliability incident. The restoration helper now retries transient readiness failures within the existing deadline, freezes identity, matches attachment PID, and verifies the final lifetime; that procedural fix does not claim to fix the crash.

## Separate allocation follow-up

The five-minute one-shot capture targeted restored Explorer PID 25544 with the current installed DLL. It observed private memory increasing from 199.18 to 290.60 MB between 33.91 and 36.02 seconds. It did not observe the required preceding 64 MB decline. A later poll exceeded the allowed interval, so the observation sequence is invalid and preserved as such. The owned WPR trace nevertheless saved successfully, and status confirmed the recorder inactive.

The saved 89,128,960-byte trace contains 132 selected virtual-allocation/free events: 75 with stacks, 57 without, zero reported lost events, and no conversion truncation. All 75 allocation events were below 8 MiB. Gross allocation-event bytes total 21,106,688; Opal appeared inclusively in 45 allocation stacks covering 20,279,296 gross bytes. These span counts are not outstanding commit and do not explain or attribute the observed 91 MB counter increase. Circular history and missing stacks remain limitations. This does not justify a Media process migration or a leak verdict.

## Acceptance and handoff

The corrected eight-leg acceptance benchmark completed two randomized paired rounds (ABBA, then BAAB), with 45 seconds settling and 15 seconds sampling per leg. It matches the current build and unchanged source inputs. The unchanged gate **fails overall**:

| Explorer metric | Stock median | FullSuite median | FullSuite − Stock | Existing limit | Result |
|---|---:|---:|---:|---:|---|
| CPU, percent of one core | 0.520% | 0.780% | +0.260 percentage points | +0.250 | Fail |
| Private commit | 184.74 MB | 202.80 MB | +18.06 MB | +20.00 MB | Pass |
| Working set | 230.90 MB | 254.30 MB | +23.40 MB | +16.00 MB | Fail |

CPU round deltas were +0.208 and +0.260 percentage points; private-memory round deltas were +26.85 and +2.69 MB; working-set round deltas were +27.47 and +16.01 MB. The different paired and pooled summaries are not interchangeable. The measured Explorer/companion/Windhawk process set had pooled private medians of 195.61 MB Stock and 213.73 MB FullSuite (+18.11 MB). Per-process working sets are not added. These observations do not establish a causal improvement over the preceding installed build.

All **28 final validation scripts passed**, including the integrated live suite, attachment, existing recovery/demand tests, 32 event-lifecycle assertions, 17 timing-collector assertions, 14 media-demand cases, 40 matched-evidence cases, 42 latency-export/parser cases, 29 package-restart assertions, and 12 bounded-capture cases. The allocation and CPU utilities also passed their recorded-trace checks. The passing code checks do not override the failed performance gate or the invalid experiments.

Final owning-system readback confirms the expected DLL and unchanged companion, no source drift, original FullSuite settings (artwork enabled, lean mode off), and both attachments. Diagnostics remain off by default. No Explorer Application Error/Hang/WER event was found in the scoped acceptance-run interval at readback. All three task-owned recorder instances were confirmed inactive. Both taskbars were visually inspected at approximately 03:04 local: normal media/performance, clocks and weather were present without visible clipping. This is a screenshot check, not a live latency result or prolonged stability guarantee.

The implementation is delivered with a preserved rollback. Performance acceptance, startup-crash attribution, and real interaction latency remain unresolved; no claim of a fully performance-qualified release is made. Raw ETLs remain local in ignored build storage. The detailed review is `Opal-4.5-all-improvements-code-review-20260905.md`.
