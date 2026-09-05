# Frozen PDH demand experiment

These files preserve the actual September 5, 2026 before/after microprobe. Baseline is the PDH query code from local commit `b94b9be`; after is the thermal-demand change in this worktree. Each generated C++ file contains the measured production functions and probe instrumentation. CPU timing is quantized by Windows process CPU counters.

`results.json`: eight fresh-process variants in counterbalanced order. `warm-results.json`: repeated calls within one process for each variant; warm observations exclude its first initialization call. No 30-second cadence, rendering or temperature-reading workload is simulated. Working-set/private deltas describe the probe process, not Explorer.

The original `Run-Probe.ps1` uses a scratch snapshot to generate sources and is retained as a procedural record. Reproduction should compile the frozen `.cpp` variants into an ignored build directory with the Windhawk clang++ toolchain (`-std=c++20 -O2 -target x86_64-w64-mingw32 -static`, linked with `pdh` and `psapi`) and save new results separately. Do not overwrite the original observations.
