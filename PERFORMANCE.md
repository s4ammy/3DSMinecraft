# Performance validation

## 2026-10-03: block rebuild stage measurements

The saved mesh-latency investigation measured six block edits in private muted Azahar. Worker rebuilds took about 220-222 ms, including about 174 ms of tessellation; mesh installation took under 0.5 ms. These guest elapsed intervals include waits and diagnostic overhead and do not establish physical Old 3DS or GPU presentation timing. One diagnostic run hit an unresolved allocation failure; its evidence is retained separately from the successful repeats. A separate hidden-face prototype passed 258 native control-flow cases. A matched six-edit comparison reduced median worker time from 220.53 to 218.51 ms, about 0.9%; this small experiment remains outside test 3 while larger gains are investigated. Test 3 remains unchanged and hardware testing is pending.

## 2026-10-03: corrected full test and biome lookup optimization

The current performance test 3 retains touch controls, background block rebuilding, late-frame completion and all earlier optimization experiments. Test 2 fixed an IPS compatibility defect that left 82 incorrect executable bytes on the actual private legacy bootstrap. The defect reproduced an Azahar startup fault; its contribution to the user's hardware lag is not yet established. Test 3 changes only the touch-controller slot comparison, preventing old menu presses from being replayed when camera input selects an inactive controller.

A new guarded biome lookup uses the game's current chunk cache directly. Valid hits take 26 versus 103 guest instructions. A controlled 6,400-query refresh trace, including cache misses, uses about 40.6% fewer lookup instructions. This is not a whole-game FPS percentage. The helper adds no allocations or persistent cache, and misses retain the original path with some extra instructions.

The exact candidate passed 2,636 native biome cases, touch/HUD, block-dispatch and completion checks, plus private muted Azahar loading, block breaking/replacement, short movement, Save & Quit and fresh-process reload. Single-run loading observations are about 40 seconds and do not establish a speedup. An unresolved 4-FPS OCR outlier is retained in the report; a subsequent movement check showed 55-60 emulator FPS. Physical Old 3DS speed and exact block mesh latency remain unverified.

Those component results are from test 2; all performance bytes are unchanged in test 3. Fresh test-3 checks cover the first touch after loading, physical-input alternation, deliberate inventory use, three blocks broken/replaced, walking and jumping. Its retained block recording showed 56-60 emulator FPS. The native controller regression also verifies no inactive-queue growth across 120 held-touch frames. Hardware testing remains pending.

Use the new test's top-level `code.ips`. Its optional comparison file preserves all earlier features and optimizations and isolates only the new biome fast path. The regular patcher now includes the IPS writer fix; the runtime experiments remain in the separate local test package. Canonical annotations have been imported, saved through IDA and exported, and the dashboard was refreshed with no stale warning.

The older sections below record the evidence available at each earlier stage. Their blocked-emulator/export statements are historical and are superseded by the current investigation above.

## Combined hardware test: v0.4.15-performance-test-1

The separate combined test package includes the v0.4.15 touch/input and block-scheduling fixes, lighting ring queue, earlier lighting-coordinate reuse and biome surface lookup, and all 246 selected division/copy call changes. It provides the three control modes with the FPS/debug overlay disabled, plus the exact matching v0.4.15 rollback IPS files. The touch-stick circle remains visible. The regular patcher stays at v0.4.15.

Validation runs against the exact combined binaries, with fresh payload compilation, complete component-delta checks, rejected address overlaps and IPS reconstruction. Native ARM routine tests cover lighting, queue lifetime, biome fallback, instruction helpers, touch capture, block dispatch and worker completions. This is a hardware-test candidate: there is no fresh whole-game emulator or Old 3DS performance result, and the full block-visibility delay is not established as eliminated. The test queue still needs physical low-memory and long-session coverage.

## 2026-10-02 shared-helper instruction research

The instruction audit adds an isolated candidate for 85 unsigned power-of-two division calls and 161 fixed-size aligned copies. Matching division paths use 9 guest instructions versus 10 to 124 in the representative original cases. The 17/25-byte copies use 9/11 instructions versus 19, and reduce stack reads plus writes from 64 to 16 bytes per call. These are helper-level measurements, not gameplay FPS or loading-time gains.

Native ARM checks cover 57,600 division cases, 48 zero-handler entries, 1,024 copy cases, all 246 patched calls, and the static copy-size setup proofs. Division quotient/remainder, flags and preserved registers match; copies also match complete test memory and all observed core registers. Runtime divisor mismatches retain the original division path with two extra instructions. The three no-overlay control profiles each build guarded division, copy and combined variants with IPS round-trip checks. The 296-byte payload adds no allocations or BSS. The normal patcher does not include this research, and whole-game/Old 3DS timing remains pending.

## 2026-10-02 broader optimization research

The loading and gameplay audit ranks lighting-queue allocations, repeated biome-color work during mesh rebuilds, resource loading, entity collisions and worker/render scheduling. Its separate builder adapts the earlier ring-queue experiment to the three exact v0.4.15 no-overlay profiles. This research is not integrated into the production patcher.

Seven controlled scenes now execute the native ARM light propagation and recalculation routines, comparing block/light storage, section touched flags and queued-position order. All match. Guest instructions fall by 8.28% to 9.26% with the test allocator. In the open-air source-add/remove case, allocation calls fall from 47,693 to 9 and peak live eight-byte-aligned queue allocations from 253,736 to 98,304 bytes. The ring retains 65,536 bytes until context destruction in that case; its larger contiguous requests still need fragmented-heap and low-memory checks. Six native constructor/destructor cycles and the existing 6,336-operation FIFO check also pass.

These are routine-level results, not loading-time or gameplay-FPS measurements. Fresh private Azahar execution is blocked by this session's display/socket restrictions. Missing-section allocation, full sky-column initialization, long sessions and hardware behavior remain unverified. Full hashes, counts, reproduction steps and public-source references are in the audit directory. Canonical IDA import/dashboard refresh is also blocked by the read-only analysis workspace; the staged annotations and temporary IDA snapshot do not update the stale dashboard.

## v0.4.15 touch input and block visibility candidate

User hardware evidence, 2026-10-02: on an Old 3DS, v0.4.14 stopped the block-edit hitch and the touch camera rotated correctly. The user reported that broken blocks remained visible for about one second after collision disappeared, and stick gestures also triggered crouch or inventory. These are user observations, not instrumented timings.

The touch leak came from handling only logical controller 0. The native input loop samples the same touchscreen in each active controller iteration. Later slots could therefore send a captured gesture to the HUD. Version 0.4.15 consumes captured touches and their releases in every slot, while injecting camera axes only into slot 0. Movement, physical buttons, stick geometry and sensitivity are preserved.

The queued rebuild from v0.4.14 remains: update address `0x1d5990` passes zero to `CLevelBuilder::buildChunk`, selecting the existing priority background path. Collision changes immediately; `endRebuild` replaces the old mesh on the main thread. An empty builder pool leaves the chunk dirty for retry.

Further tracing found that `CWorkerPool::runCoroutines` returns before polling completed jobs when the remaining frame budget is zero or negative. Thus a finished rebuild can wait for a later frame, and its builder remains unavailable meanwhile. Version 0.4.15 redirects only this early-return branch at `0x16fb48`: it polls one completion from the first asynchronous streaming worker through the original locked dequeue and callback cleanup, then returns the original budget-exhausted result. It starts no new worker job on the main thread. Frames with a positive budget retain their original path. The existing pool has one streaming worker; file I/O and main-thread job groups are not polled by the added hook.

The 36-byte completion helper occupies the checked gap between the existing heap helpers at `0x35d634`. Text extent, BSS size and bootstrap LayeredFS reservations remain compatible with v0.4.14. No new bootstrap installation is needed for its existing compatible touch-stick setup.

Validation completed locally:

- The new touch regression runs all four iterations of the native ARM input loop with controlled controller handles and hardware samples. The v0.4.14 executable leaks stick events to native HUD dispatch; v0.4.15 suppresses them through press, dragging outside, release and menu transition. Ordinary HUD touches and physical buttons remain available.
- The worker regression executes native scheduling and completion dequeue with controlled callback bodies and OS locks. Across 60 consecutive late frames, the old executable processes no completions; the candidate processes at most one per frame in FIFO order and calls cleanup exactly once. Empty queues, empty groups, synchronous workers, positive budgets and register/stack preservation pass. This is a scheduling regression, not a hardware latency benchmark.
- The rebuild dispatch, all six update profile hashes, IPS round trips, original button bindings, unchanged bootstrap layout and incompatible-input rejection are checked.
- Host gesture tests cover all 76,800 screen coordinates and transitions. Compiled ARM hooks also check registers/VFP, native HUD argument forwarding, tint restoration, circle geometry and HUD destruction. Engine rendering and screen lookup are mocked.

Actual mesh construction still takes time, and a completion callback itself has a cost. The new hook removes a demonstrated source of starvation; it does not prove the user's entire one-second delay is resolved. Test single and rapid edits, chunk boundaries, lighting, touch gestures and save/reload on the Old 3DS. The separate night-time slowdown/crash remains outside this change.

Azahar validation remains blocked by this session's denied socket operations. Previous private Xvfb and Qt offscreen attempts failed; audio was disabled and no physical display or keyboard/mouse input was used. No new emulator timing or screenshot is claimed.

Canonical IDA saves and dashboard refresh remain blocked because `/home/admin/Desktop/Reversing/3DSMCRE` is read-only in this session. Update-only names and comments are staged in `experiments/touchStick/idaAnnotations.json` and applied to a temporary IDA copy. The existing stale dashboard export is unchanged. Names and static annotations are separate from the validation above.

Reproduce with `ctest --test-dir build-touch --output-on-failure` and a private `MC3DS_UPDATE_FIXTURE`. `scripts/buildTouchStickPayload.py` and `scripts/buildWorkerCompletionPayload.py` accept `--check --reference-code ...`. The Unicorn checks are `scripts/checkTouchStickPayload.py`, `scripts/checkBlockRebuildDispatch.py` and `scripts/checkWorkerCompletion.py`; their `--help` lists the required private fixtures.

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
