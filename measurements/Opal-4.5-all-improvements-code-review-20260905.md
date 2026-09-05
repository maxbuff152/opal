# Opal 4.5 performance improvements: code review

Date: 2026-09-05. Review baseline: `da7a8e0874c88923e24202052df4e5206e3b7915`. Review target: current uncommitted changes against that baseline, plus the explicitly scoped new files below. This is a code and evidence-contract review, not a completed performance acceptance report.

## Scope and standards

Reviewed production changes in `mod/visual-clones/maxwell-shell.wh.cpp`, `maxwell-opal-media.wh.cpp`, `maxwell-taskbar-system-info.wh.cpp`, and new `opal-performance-diagnostics.h`; assembly integration in `Assemble-Opal.ps1`; supporting documentation in `README.md`.

Reviewed diagnostic surfaces: `Get-OpalMemoryRegions.ps1`, `Get-OpalMatchedSoakAnalysis.ps1`, `Get-OpalLatencyAnalysis.ps1`, `Measure-OpalMatchedSoak.ps1`, `config/OpalAllocation.wprp`, `config/OpalCpu.wprp`, and `tools/OpalAllocationTrace`. Corresponding new tests cover recovery waits, media demand, diagnostics, latency evidence, matched-soak evidence, allocation profiling, and CPU profiling. The later bounded restart correction additionally covers `Measure-OpalPackageCost.ps1` and `tests/Test-OpalPackageRestart.ps1`. Preexisting untracked measurement artifacts were not treated as new production changes.

Standards owners: `CONTRIBUTING.md` and Maxwell's supplied AGENTS operating contract. Canonical source ownership, preservation of unrelated work, focused verification, and explicit authorization govern this review. Maxwell authorized a local current-branch commit; PR creation and push are outside this task's scope.

**Standards finding count: zero hard documented violations; one low-severity maintainability observation, mitigated by a regression contract.** Metric labels and the export header are duplicated between the C++ producer and PowerShell parser. Their small fixed mapping does not justify a broader format refactor now. Naming, duplication, ownership, data grouping, switching, change dispersion, speculative abstractions, and delegation heuristics were assessed as judgment aids, not mandatory rules. No remaining blocking production issue was identified in this review scope; that is not a guarantee of defect absence.

## Reviewer corrections resolved

- **Timing producer/parser contract:** independently authored synthetic exports could miss producer-only label changes. The test now derives the current C++ header, metric labels, numeric row, and empty-row literals and checks the actual parser. The coordinating reviewer reports **42 passing checks**. This resolves the missing contract coverage while retaining the small duplicated mapping.
- **Package benchmark restart:** scenario setup and restoration previously launched Explorer unconditionally after stopping it. A shared procedure now allows five seconds for automatic recovery, permits only one explicit fallback while no session Explorer exists, and requires exactly one new PID within the existing 20-second deadline. Sampling retains that identity. A second review caught lazy `Process.StartTime` evaluation; acceptance now immediately freezes scalar PID and birth time. **29 mocked production-function/restoration checks passed**, including a mutable lazy-property PID-reuse regression. No live restart was performed by these tests.

The package correction preserves acceptance limits, receipt schema, rounds, settle duration, and sample duration. Its focused diff check passed; the preceding 26-check revision also passed `Test-OpalSuite.ps1 -StaticOnly`. The final scalar-identity delta received fresh readback and the expanded 29-check test.

## Restoration readiness follow-up

The 32-minute rerun completed all four measurement legs but remained invalid because restoration readback encountered transient Explorer absence. Its restart event records PID 23056 and no explicit launch. The coordinating task identified a Windows.CloudStore.dll startup fault; the later independent restoration readback verifies PID 25544, original settings, expected DLL hash, and both attachments. That later proof does not retroactively validate the raw run, which retains `Complete=true`, `Valid=false`, and its restoration error.

`Wait-RestorationReady` now retries absence, multiplicity, module/attachment observation errors, and settings mismatches inside the existing readiness window, preserving attempt count and the last observation error. The owner's final expanded suite reports **40 passing targeted cases**; review confirms the tests execute the actual helper with mocked observations. Measurement-time assertions and the strict analyzer remain unchanged. This addresses premature restoration-readback failure; it does not establish or fix the underlying startup crash cause.

The additional restoration-proof race is resolved on final source readback: the helper immediately freezes candidate PID and birth time, requires exactly one attachment result with the matching PID, and rechecks the same process lifetime before returning success. Attachment-PID replacement, final-PID replacement, and same-PID/new-birth fixtures each require a retry before accepting the stable replacement. No remaining blocker was identified in this helper review. This correction does not show that the completed measurement legs changed identity and does not modify the preserved raw receipt or the separate later restoration proof.

## Specification and evidence boundaries

Specification coverage remains **partial at the live-evidence layer**. Numeric timing collection and parsing are tested, but no real hardware-panel/media interaction export was obtained. Attachment measures a successful verification/repair pass excluding prior retry delays; media timing ends at the next applied update, not provider acknowledgment; panel `Opened` is not pixel presentation. Exported timing text lacks build/PID/time/raw-sample provenance and cannot establish an installed-build release pass.

Allocation results describe gross event bytes and inclusive module-stack association, not outstanding allocations, retained commit, ownership, or a leak. Circular-history limits, missing stacks, unknown modules, dropped events, and conversion truncation remain explicit. CPU sample counts likewise do not establish causal CPU percentages or observer causality. The offline utility pins TraceEvent 3.2.6 with transitive dependencies locked and audited; it performs no symbol lookup, memory dump, or upload.

**The completed soak remains invalid because its original restoration verification failed.** The rebound trace saved and stopped, but its observation sequence failed a delayed-sample check; allocation ownership remains unproven. The corrected eight-leg acceptance run completed and failed the unchanged CPU (+0.260 versus +0.250 percentage points of one core) and working-set (+23.40 versus +16 MB) limits; private commit passed (+18.06 versus +20 MB). All 28 final validation scripts passed. These final results are integrated into the main report, which remains authoritative for the experimental details. This artifact claims neither a speed improvement nor a fully performance-qualified outcome.
