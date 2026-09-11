# Performance validation

## v0.4.10 aligned-allocation improvement

An uninstrumented same-save Azahar ABBA comparison measured 50.468 seconds on v0.4.9 versus 43.495 seconds with v0.4.10, a 6.973-second / 13.8% reduction to first survival HUD. This is two runs per build on one existing world. There is visible run-to-run variation; it is not a console benchmark, a proven gameplay FPS gain, or a statistically established percentage across worlds.

| Execution order | Build | Seconds |
| --- | --- | ---: |
| 1 | v0.4.9 | 53.486 |
| 2 | Aligned-allocation candidate | 45.049 |
| 3 | Aligned-allocation candidate | 41.942 |
| 4 | v0.4.9 | 47.450 |

The same private fixture, Old 3DS emulation settings, 100% CPU clock/frame limit, hidden muted display and first-HUD detector as the v0.4.9 comparison were used. Every run used a fresh process and verified identical save/shader snapshot. Boot and menu time were excluded; host filesystem caches were not flushed. Settled captures show the same terrain and X=548, Y=71, Z=0. No debugger, RPC, timing hooks or movie playback were active during these timed runs. These measurements should not be compounded with earlier-version percentages from different runs.

The common forward four-byte-aligned heap search no longer rounds each candidate address: SDK block headers are already aligned. The 72-byte payload selects the same node in the same first-fit/best-fit order and resumes original splitting and commit. Different positive alignments keep the original search with small entry overhead; negative alignment bypasses the hook. Lock ownership, zero filling, allocation group, alignment-margin handling, list layout and returned pointers are unchanged. There is no cache, new allocation, stack frame or BSS growth. Rendering, lighting, generation, readiness thresholds and the overlay are unchanged relative to v0.4.9.

Actual ARM allocation, split/commit and free routines matched full 64 KiB heap images, return values, R4-R11 and SP after 12,800 mixed operations over 160 synthetic heaps. These include 9,817 allocation requests, 4,476 allocation failures and 2,983 frees, fragmented heaps, first/best fit, both search directions, alignments 4-64, zero/small/oversized requests and zero-fill/alignment-margin options. Four-byte allocation instructions fell from 657,205 to 453,636 in this mixed workload; other-alignment instructions rose from 2,036,720 to 2,045,569. These are instruction counts, not ARM11 cycles or whole-game percentages. Invalid alignment, corrupt heaps and concurrent misuse are not covered.

Both production control profiles pass output-hash, IPS round-trip, payload/layout, untouched original search/commit/coalescing and incompatible-input rejection tests. The default production IPS independently decodes to the exact benchmarked executable. The existing v0.4.9 IPS and source save fixture are preserved; no SD files or installed titles were changed.

The production IPS also completed movement/camera checks across several chunk boundaries, Save & Quit, and a fresh-process reload. A separate v0.4.9 process loaded the same saved copy at matching X=593, Y=62, Z=-24 and underwater view, with the overlay present. These reloads took 40.997 and 47.362 seconds, respectively. They use a changed save snapshot and are excluded from the original ABBA means. This is limited smoke coverage, not a long-session or save-format equivalence proof.

Separate exact NBT key-lookup and compound END-allocation experiments passed their focused tests and reached gameplay, but their initial same-fixture loads took 46.706 and 47.028 seconds. Those single runs are close to the latest baseline and do not establish a repeatable incremental gain. Neither is included. The END experiment also needs broader caller and error-path coverage. A lighting queue that allocates one uint32 per storage block remains a future target, not an implemented or measured improvement.

Private evidence: `analysis-codex-2/global-performance-v050-20260911/`, with emulator runs in `world-loading-audit-20260911/emulator-runs/global-*`. The directory's `v050` label is a research name, not the release version. Hardware timing, long sessions, multiplayer, new-world-generation timings and broader world coverage remain unverified. No new quality reductions are introduced, but exhaustive absence of regressions is not established.

A later report concerns the published v0.3.0 build on Old 3DS: B can immediately close an interacted container, and night-time play can lag or crash. Neither is claimed fixed here. B/menu bindings and the relevant cancel callback remain unchanged in v0.4.10; the suspected opening-release ownership problem needs end-to-end validation and a separate fix. The night-time crash needs its own dump and reproduction, not attribution to unshipped optimization code.

## v0.4.9 world-loading improvement

An uninstrumented, same-save Azahar ABBA comparison measured 64.607 seconds on v0.4.8 versus 52.126 seconds with v0.4.9, a 12.481-second / 19.3% reduction to first survival HUD. This is two runs per variant on one existing world, not a console benchmark or a statistically established gain across all worlds.

| Execution order | Build | Seconds |
| --- | --- | ---: |
| 1 | v0.4.8 | 65.481 |
| 2 | Heap-search candidate | 52.273 |
| 3 | Heap-search candidate | 51.980 |
| 4 | v0.4.8 | 63.733 |

Each run used a fresh private emulator process, identical verified save/shader snapshot, Old 3DS mode, 100% emulated CPU clock and 100% frame limit. Boot/menu time was excluded. The timer starts with selecting the saved world and ends at its first HUD; sampling brackets are about 0.10 seconds. Settled captures show the same terrain and X=548, Y=71, Z=0. Host filesystem caches were not flushed. No debugger, RPC, timing hooks or input movie were active. Azahar was hidden on a private virtual display and muted, while guest audio initialization remained intact.

Stack sampling in separate diagnostic runs identified repeated traversal of the SDK expandable heap's address-sorted free list during structure-template parsing and cleanup. The new 68-byte payload searches from either end, with a direct tail case. It resumes the original neighboring-block merge and insertion code. Allocation policy, heap layout, alignment handling, mutex ownership and live allocations are unchanged. There is no cache, extra allocation, BSS growth or additional stack frame. The existing overlay, generation, lighting, view distances, sound preloading and readiness thresholds are unchanged relative to v0.4.8.

Differential execution of the actual ARM free routine matched the entire 64 KiB heap, return value, R4-R11 and stack balance after 11,440 frees across 180 synthetic heaps. Cases include 1-512 blocks, alignment padding, empty/fragmented free lists and ascending/descending/random free sequences. Instruction counts fell from 3,787,672 to 1,723,530 in this workload. That 54.5% reduction is not a loading-time percentage; some short-list cases have extra instructions. Corrupt heaps, concurrent misuse and physical timing are outside that test.

Both production control profiles reproduce their checked hashes and pass IPS round-trip, payload/layout, untouched-coalescing and incompatible-input rejection tests. The heap payload uses reserved space after the overlay and leaves bootstrap metadata and Luma injection padding unchanged. The overlay generator and patcher reject overlaps.

The default candidate completed Save & Quit, then fresh processes running both the candidate and previous v0.4.8 build successfully loaded that saved copy at the same visible terrain and position with the overlay present. These additional runs took 48.735 seconds with the candidate and 65.762 seconds with v0.4.8. This is only one pair on the changed save snapshot, so it is excluded from the original ABBA averages. A prior same-process reload attempt was interrupted by the manual test harness deadline, not an observed game crash.

Private evidence: `analysis-codex-2/world-loading-audit-20260911/`. The local index-packing experiment and empty structure-temporary removal were not promoted because they did not demonstrate a loading gain. END-tag allocation elision remains a separate unbenchmarked experiment. Only the layout-preserving heap search is added in v0.4.9.

Hardware timing, long sessions, multiplayer and broad world coverage remain outstanding. There are no new quality reductions, but these tests cannot guarantee the absence of every regression.

## Earlier v0.4.8 overlay

Version 0.4.8 retains the v0.4.7 optimizations below and adds a [custom FPS/debug overlay](DEBUG_OVERLAY.md). Its measurements are displayed-frame intervals, not CPU/GPU utilization or a claim of a new performance gain. Text rendering and once-per-second heap queries have overhead that has not been measured on hardware.

This build reduces specific CPU and memory-copy costs. It does not establish a whole-game FPS or world-loading percentage improvement on Old 3DS hardware.

## New changes compared with v0.4.6

| Routine / representative case | v0.4.6 | v0.4.7 | Measurement |
| --- | --- | --- | --- |
| Raw chunk light, present section | 65 | 32 | Guest ARM instructions, including entry redirect |
| Raw chunk light, out-of-range section | 14 | 16 | Guest ARM instructions, two extra instructions |
| Inflate six 32 KiB streams | 19 allocations / 452,380 requested bytes | 9 allocations / 252,960 requested bytes | Ten allocation/free pairs removed |
| Normal biome-color pass, 16 x 16 | 1,021,594 | 991,959 | Guest instructions with controlled external dependencies, about 2.9% fewer |

The light replacement passed 20,000 cases, including returning assertion handlers for invalid X/Z, preserved R4-R11 and stack balance. Integrated decompression passed 512 cases covering malformed streams at all six positions, absent streams and shared/unshared outputs, plus injected state/window allocation failures. Every tested zlib allocation was freed. The biome modulo change passed 128 cases comparing colors, callback/write sequence, return value and complete final MT state. Relocated existing code passed 512 climate cases and 10,000 NextInt cases. Both production control profiles pass guarded fixture and IPS round-trip tests.

The decompressor reuses state at its original `z_stream.state` field, retains allocator callbacks between streams and invokes actual `inflateReset`. Scratch ownership stays at `SP+0x48`; no stack-frame growth or global context is introduced. Reset itself is also called by the initial initializer, so six total reset entries mean one initialization plus five reuses in the six-stream example.

Reduced cumulative allocation traffic is not reduced peak memory. Peak live allocations rose from 253,091 to 253,169 bytes in the six-stream example and by as much as 32,768 bytes in the sampled differential cases. Longer-lived history windows may change low-memory behavior on hardware. These are test-allocator requested-byte counts, not console heap measurements.

The ready-flag-only biome cache was implemented as a research prototype and rejected. In a controlled case, neighboring chunks became available while the target's ready state remained 2; skipping the refresh left 256 column colors different from the original. Actual biome getters ran with controlled chunk-source/color/noise dependencies. This is not a whole-game reproduction or a complete invalidation audit. The release retains original refresh calls and changes only the exact remainder calculation.

Evidence and executable-dependent harnesses: `analysis-codex-2/performance-v047-20260911/` in the private analysis workspace. The earlier evidence below remains under `performance-v046-20260911/`.

## Retained v0.4.6 changes

| Routine / representative case | Before | v0.4.6 | Measurement |
| --- | --- | --- | --- |
| Cached block lookup, present subchunk | 98 | 44 | ARM instructions, 55% fewer |
| Climate edge pass, 16 x 16 | 26,697 | 8,150 | ARM instructions, 69% fewer |
| Read six 1 KiB compressed slices | 12 allocations / 12,388 copied bytes | 6 allocations / 6,244 copied bytes | Test allocator and copy counters |
| Inflate six 32 KiB streams | 294,912 copied bytes | 98,304 copied bytes | 192 KiB less copying |

The first two examples are not ARM11 cycle counts. The storage figures include other copies in the tested routine, not just the removed operation. They are not SD-card throughput measurements. The decompression comparison uses the corrected scratch-buffer control described below.

Some paths gain nothing or have minor overhead. The block-cache miss example increases from 60 to 64 instructions with its backing resolver stubbed. A 1 x 1 climate request increases from 168 to 176 instructions. Shared or insufficient-capacity inflate output retains the original path plus checks. No claim of universally faster execution is made.

## Correctness tests

- Block lookup: 20,000 synthetic valid-layout cases, including signed coordinate extremes, height boundaries, missing chunks and missing subchunks. Output, return value, cache state and resolver requests match. The backing chunk-source resolver is stubbed.
- Climate pass: 512 integrated routine cases matching output, parent requests and final seed state, including zero dimensions and signed coordinate extremes.
- Record loading: 512 integrated cases with the original ARM/Thumb string routines and a test allocation backend. Includes shared/unshared strings, embedded NULs and absent streams; compares metadata, version, terminators and source-record immutability.
- Inflate: 128 cases running the game's actual inflate/init/end and string routines. Outputs and error behavior match stock and the corrected control, including truncated streams, invalid checksums and invalid headers.
- Production patcher: both control-profile output checksums, IPS round trips, payload hashes and rejection of altered guarded routines or occupied padding pass.

The authored Voronoi cache passed 512 cases but is excluded from the default build. It saved about 6.4% of instructions for a 16 x 16 example and 8.5% for 28 x 28, but increased the 1 x 1 case from 1,724 to 2,288 instructions.

## v0.4.5 scratch-buffer correction

The previous hook stored scratch ownership at `SP+0x18`. The live `z_stream` occupies `SP+0x10` through `SP+0x47`, and `SP+0x18` is `total_in`. Real zlib resets and increments that counter, corrupting the pointer. A six-stream regression test reproduced an invalid write in the old build.

v0.4.6 uses `SP+0x48`, outside the stream but still inside the existing frame. It does not grow the stack frame or add a global scratch buffer. The fixed control and candidate match stock decompression in the 128 tests. The old build is unsuitable as a known-good decompression timing baseline.

## Emulator and hardware limits

Tests use a private Azahar data copy, virtual display and disabled audio. Scripted boot/navigation tests include fixed presentation delays, menu dwell and world work. Their elapsed time is not a pure loading benchmark; unlocked emulator FPS is not console FPS. Hardware timing, long-session behavior and multiplayer validation remain outstanding.

In the earlier v0.4.6 tests, the corrected control and combined build both reached gameplay. Their extended navigation movies completed in 151.71 and 147.23 seconds, respectively, but the generated worlds had different saved seed values and spawn terrain. Those elapsed times are therefore not a valid same-world A/B comparison and do not establish a speed gain. Test processes were forcibly cleaned up after playback completion; that cleanup is not an in-game crash result.

The v0.4.7 baseline and candidate navigation runs also reached visible gameplay with Old 3DS mode enabled. Different generated worlds and a movie-runner cleanup issue prevent using their elapsed times as benchmarks. A separate candidate test without movie playback successfully loaded an existing world from a private save copy, completed Save & Quit, and reloaded that saved world in a fresh emulator process at the same position and terrain. Screenshots and logs are retained under `performance-v047-20260911/`.

Detailed private evidence and reproducible differential harnesses are in the analysis workspace at `analysis-codex-2/performance-v046-20260911/`. That directory is not shipped because the harnesses depend on the user's own executable and emulator data.

## Installation

Keep the existing compatible bootstrap base and update installed. Replace only the matching `code.ips` in `/luma/titles/000400000017CA00/`, then fully close and relaunch Minecraft. Do not copy extracted `exheader.bin` or `code.bin` alongside it. Leave the LayeredFS shader files unchanged. Back up saves before hardware testing.
