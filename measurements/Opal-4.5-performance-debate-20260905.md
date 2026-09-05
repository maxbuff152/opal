# Opal 4.5: performance architecture debate

Date: September 5, 2026. Owner: Codex. Scope: local Opal performance and a bounded Slack debate authorized directly by Maxwell.

## Decision

Keep the current process split for this change. Make each provider collect only the signals its consumer needs, and measure costs across the participating processes. Moving Media out of Explorer remains a reliability option; the present evidence does not establish that it would reduce total machine cost.

The selected runtime change removes duplicate GPU PDH counter acquisition from the external-temperature path. The companion already supplies GPU metrics. Explorer now registers only the thermal-zone counter for Auto/WindowsNative, and opens no PDH query for Disabled, SharedMemory, GadgetRegistry or HwInfoAuto sources. Full in-process fallback is preserved. Demand transitions discard obsolete queries/completion handles and clear unrelated retry deadlines; repeated same-demand failures retain backoff.

## Actual debate and participants

[Slack discussion](https://sellersfirsth-tmx5709.slack.com/archives/C0BUNNLJSDV/p1788588305446069).

- Meitner, local Codex child, argued for a thin Explorer presentation layer and out-of-process failure containment, then favored removing duplicate provider work first because a process split could add total cost.
- The minimalist local Codex child independently found the same unnecessary GPU counters. Existing graph histories, closed-panel rendering and artwork dimensions already have bounds; generic caching changes were not supported by the inspected code.
- Arendt, local Codex child, challenged causal attribution and the Explorer-only benchmark. The prior stock receipt already contained a 400.84 MB private-memory sample. This weakens attribution of the late allocation to Opal, without proving the allocations share a cause.
- Cursor replied through its native Slack bot using GPT-5.6 Sol, [session](https://cursor.com/agents/bc-714a5d1e-de0e-52e9-892a-ca943bafeb00). It independently critiqued supplied evidence, favored the bounded fix and challenged stale-source/fallback transitions. It did not inspect local source or change files.
- Codex relayed explicitly attributed local-agent views through the shared Slack connector, moderated, implemented the measurement changes and owns final verification. Local reviewers are not independent Slack bot identities. Claude and Grok did not participate in this task.

## Proof and limits

Compiled production-code regression: 22 demand/source/failure/retry assertions pass. The demand-change patch received independent review with no blocking defect. Paired analysis has 14 regression cases; aggregate process sampling has 11. All 19 repository validation scripts have passing recorded results. The install-sensitive safe-dock check was rerun after installation; separate final runtime and attachment checks verify the installed build. The initial safe-dock check correctly failed because it ran between build and install; the original failure and successful post-install readback are both preserved.

Real Windows PDH microprobes used frozen production-function copies in fresh processes and repeated warm calls. All observed registrations/collections succeeded. Auto requested four counters before and one after; non-PDH temperature selection requested three before and zero after. Warm Auto setup/collection/cleanup measured 3.58–4.22 ms before versus 2.70–4.13 ms after. Approximately 1–1.8 MiB of transient private allocation was removed in that isolated probe. Cold initialization was much more expensive and retained much of PDH's base memory. These small samples do not establish Explorer-wide savings or a fixed memory budget. The warm probe ran repeated calls without a 30-second pause and did not read/render temperatures.

Live installed benchmark completed: eight legs, two ABBA/BAAB rounds, 45-second settle and 15-second sample per leg. All legs verified expected runtime/DLL state; the full-feature configuration was restored. Installed DLL SHA256 is `83EE94A762C62CF983FA9F3E8B0A24480BA9A6841244367E42518465EE31D475`; companion hash is unchanged. Installation preserved 110 settings, including lean mode off and automatic temperature sourcing. Rollback: `C:\Users\maxwe\AppData\Local\Maxwell\Opal\rollback-20260905-011158`.

| Explorer metric | Pooled FullSuite minus Stock | Median paired-round difference | Interpretation |
|---|---:|---:|---|
| CPU, percent of one core | -13.94 | -12.51 | High variable stock/full activity; cannot support an idle-CPU speedup claim |
| Private commit | +15.93 MB | +32.66 MB | Pooled gate passes 20 MB; paired rounds vary +15.21 to +50.12 MB |
| Working set | +18.24 MB | +22.45 MB | Still exceeds the unchanged 16 MB budget; paired rounds +20.34 to +24.56 MB |

The unchanged gate fails overall on working set. This run is not proof of a whole-shell performance improvement. Explorer CPU ranged from 0.21 to 46.77 percent of one core, including 27.42–41.73 in Stock. The high activity is present without Opal; its cause was not attributed here. A FullSuite endpoint also reached 437.56 MB private commit. Matched workload/allocation tracing remains necessary before a memory-causality conclusion.

For the expanded measured process set, pooled private commit was 340.89 MB Stock and 356.89 MB FullSuite, a +15.99 MB descriptive difference. Paired-round private differences were +16.23 and +50.15 MB. Companion/Windhawk costs are visible in the raw receipt; no aggregate working set is presented. There is no equivalent pre-change combined-process receipt, so this cannot establish before/after total-cost savings.

After restoration, the companion exited gracefully in 7 ms and recovered automatically within the 381-ms probe interval. Fresh loaded-DLL and attachment readbacks passed. Both taskbars were visually inspected at 01:22; normal media, performance, clocks and weather were present without visible clipping. This recovery probe is not a forced prolonged provider stall. Direct PE import comparison found no added/removed imported libraries; no third-party dependency changes were introduced. A scoped secret scan is recorded separately.

## Eight improvements, in priority order

1. **One collector per signal — implemented here.** Explorer's external-temperature branch no longer constructs the companion's GPU counters. Retain fallback acquisition only when needed.
2. **Count the participating processes — implemented here.** The benchmark records all interactive-session Explorer, Maxwell.Shell.Core and Windhawk processes over the same interval, with CPU and private-commit totals. Per-process working sets remain separate because shared pages can be counted more than once. Other shell hosts/services are explicitly outside this measured set.
3. **Use paired rounds and show variation — implemented here.** Each ABBA/BAAB round reports mean FullSuite minus mean Stock, median round delta and observed range. Pooled medians remain separately labeled. Existing acceptance limits and acceptance gate are unchanged; two rounds are not a confidence interval.
4. **Attribute the late allocation before changing memory ownership — next investigation.** Compare matched long stock/full runs, then capture region/module information and allocation stacks if the step repeats. A large stock Explorer already exists in the evidence.
5. **Replace sliced recovery sleeps with a stop event — proposed.** Preserve the five-second health check but eliminate its 100-ms unload polling. This is source-confirmed wakeup overhead; its machine-level impact has not been measured.
6. **Budget visible response time as well as idle cost — proposed.** Measure taskbar attach, media interaction and hardware-panel latency under the same workloads. Faster cold startup or fewer idle counters must not trade away recovery or response time.
7. **Use demand and visibility to govern future provider work — proposed principle.** Audit each additional collector against its visible consumer before adding caches or background threads. Several existing UI histories/artwork paths are already bounded; change only a measured owner.
8. **Move Media providers out of Explorer only with a demonstrated benefit — deferred.** Require evidence of provider stalls or allocation ownership, then compare combined CPU/private commit, response time and recovery against the simpler design. A lower Explorer-only number would not prove a total-cost win.

## Evidence

- `opal45-demand-tests-20260905.json`: first validation pass, including expected pre-install DLL drift.
- `opal45-demand-tests-final-20260905.json`: final validation results after installed hash matches.
- `opal45-demand-build-20260905.json`, `opal45-demand-install-20260905.json`: build/install provenance and recoverability.
- `pdh-demand-20260905/`: frozen probe sources and raw cold/warm observations. `Run-Probe.ps1` records the original scratch-based generation procedure; frozen `.cpp` files provide the exact measured versions.
- `package-cost-opal45-demand-20260905-abba.json`: live paired benchmark with per-process measurements.
- `opal45-demand-budget-20260905.json`, `opal45-demand-paired-analysis-20260905.json`: unchanged acceptance gate and separate descriptive paired analysis.
- `opal45-demand-process-analysis-20260905.json`: expanded measured-process descriptive totals and round differences.
- `opal45-demand-recovery-20260905.json`, `opal45-demand-readback-20260905.json`, `opal45-demand-final-attachment-20260905.json`: final owning-system proof.
- `opal45-demand-imports-20260905.json`: direct PE dependency comparison, not an OS/transitive vulnerability scan.

This change does not claim indefinite leak freedom, a completed sleep/display stress test, or a Media process migration.
