# Performance validation

## v0.4.19 native menu crash fixes

`release/ips/v0.4.19/code.ips` promotes the tested compact UI, rendering fast paths, LayeredFS fix below and settings scrolling fix. Mod Settings scroll layout waits until render entry, after the active pane input call has returned. The private listener is installed only on the custom screen's container. Stock Options retains its original listener. The pending flag uses existing menu-state padding, so settings, bindings, MC3DS4 encoding, bootstrap, BSS and the 404-byte menu allocation are unchanged.

Hardware dump `crash_dump_00000007.dmp` faults at `0x240DD0` while the native scroll setter dereferences the pane's listener at offset 60, which contains `0x5544`. Native analysis identified a lifetime hazard: the listener synchronously calls container layout, layout can delete and replace the pane, and the original normalized-scroll and clamp routines continue using that pane. The new callback records a pending layout instead of rebuilding during that stack.

`scripts/nativeScrollChecks.py` executes the actual ARM scroll, listener, layout and clamp routines with controlled allocation and external dependencies. All eight old-behavior cases read freed pane memory; four cases that model allocation-header reuse reproduce the dump's exact fault PC. All eight candidate cases complete with zero stale reads. Another 13 integration cases verify custom-only installation, layout before rendering, pending-state consumption and screen cleanup. This establishes the reproduced lifetime hazard and its fix; the hardware dump alone does not prove the sequence of heap reuse.

The full ARM settings suite, LayeredFS path injection, rendering fast-path checks, five host CTests and byte-for-byte native rebuild reproducibility pass. Hidden, muted Azahar completed 21 cycles across all seven pages, using touch/controller page selection, scrollbar drags and dropdown cancellation, then ten exits and reopenings with live screen-stack references checked. A camera sensitivity change was also exercised. Stock Options opened with its original listener intact. The user confirmed the fixed IPS works on hardware after replacing the previous SD-card patch. This is a limited startup/settings check; broad hardware coverage and long-session stability remain unverified. Private evidence is under `analysis-codex-2/settings-crash7-20261014/`.

An earlier diagnostic emulator run also failed after leaving settings at the current-screen button-release dispatch (`0x3E8980`), rather than the pane listener. That run used unsupported single-step/breakpoint handling. Its cause remains unconfirmed and it is recorded separately from dump 7; the fresh 21-cycle run and ten exit/reopen cycles did not reproduce it.

## 2026-10-14 LayeredFS path collision fix candidate

`release/experiments/layeredfs-fix-20261014/code.ips` fixes the native-menu payload's collision with Luma's LayeredFS path storage. The existing bootstrap declares read-only data ending at `0xA28464`; Luma writes its 39-byte redirected-file path there after applying IPS. The earlier native-menu candidates and published IPS placed the compact dictation-binding table at that same address. Hardware dump `crash_dump_00000006.dmp` reaches `CStringLength` at `0x2FE9B4` with the invalid pointer `0x2F3A666C`, the bytes `lf:/`.

The builder now reserves 40 aligned bytes for Luma and repacks the table at `0xA2848C`. Packaging rejects writes into the path reservation and checks that it remains unused. The text-side LayeredFS reservation, bootstrap metadata, BSS, settings, bindings and MC3DS4 encoding are unchanged. This candidate retains the compact UI, original grey switches and previously tested rendering fast paths. No new bootstrap is required for this fix. The original v0.4.18 release has the collision; v0.4.19 includes the fix.

`scripts/checkLayeredFsCompatibility.py` checks the reservation against the compatible bootstrap ExHeader, preserves text-side loader padding, and executes all nine input-builder cases both before and after injecting Luma's path: 18 passing cases. Its negative control reproduces the published patch's exact hardware PC, LR, invalid read and table register. The full ARM settings suite and rendering fast-path suite also pass after path injection. Five host CTests, ten release-validation tests and native payload reproducibility pass. These checks model the path write, not a full Luma boot; console validation remains pending. Separate diagnosis evidence is under `analysis-codex-2/layeredfs-collision-20261014/`.

Run the settings suite with `--layeredfs-exheader BASELINE_EXHEADER` to include this loader-side write in future native-menu validation.

## 2026-10-14 green enabled switches withdrawn

The green-switch experiment is withdrawn. User testing showed green leaking into the sliders and scrollbar. The widget tint writes and boolean-renderer instruction override have been removed, restoring Minecraft's original toggle drawing. Settings, bindings, save format and the other compact-UI changes are unchanged. Use `release/experiments/compact-ui-20261014/code.ips`, not the retained green-switch experiment.

The experiment's isolated switch tests and brief emulator checks missed the surrounding-control regression. The retained 56 ARM switch cases now require the stock color-load instruction, unchanged white widget tint in both states, native thumb positions and reset of the shared draw color. Historical green-switch screenshots and reports under `analysis-codex-2/green-switches-20261014/` are not evidence of a regression-free full menu. The earlier compact-UI candidate and tracked release remain unchanged.

After reverting, the rebuilt executable and IPS match the preserved compact-UI candidate byte-for-byte. The full ARM settings suite, including all 56 stock switch cases, and five host CTests pass. The private desktop preview uses that restored candidate with audio disabled.

## 2026-10-14 compact UI and rendering candidate

`release/experiments/compact-ui-20261014/code.ips` is a separate EU update test candidate. The tracked v0.4.18 release IPS and the PC patcher's embedded payload bytes are unchanged. All seven pages, four control modes, shared/chord/unbound bindings, defaults and the existing MC3DS4 encoding are retained. `controls/ControlSettings.cpp` logic and its persisted layout are unchanged.

The smaller UI layer is an original implementation using our native ABI adapter, informed by NUMC3DS's bounded-control and modal-input approach. No NUMC3DS source, module loader, storage format or full forms framework is imported. `NativeUi` handles native widget creation, value conversion and unchanged-label/selection suppression. Compact read-only row descriptors replace duplicated page-specific widget branches. Menu state shrinks from 488 to 404 requested bytes; runtime state and BSS do not grow. Native screen and pane ownership still release old rows before replacement, and page rebuilds still occur in polling rather than widget callbacks.

An expanded dropdown consumes controller navigation without moving the parent focus or scroll position, including mixed touch/controller use. Binding capture also consumes parent navigation. Stock Options keeps its original navigation route. Native ARM checks cover 234 setting/value cases, 147 adapter construction/label cases, 73 modal-navigation cases, scroll memory and the existing dropdown broadcast tests.

The optional allocation-free rendering payload replaces three original function slots, with no trampoline allocation or added cache:

- The format-27 vertex attribute writer emits the same packed color and four rounded 16-bit UV channels. Normal bounded values use direct VFP conversion with a ceiling correction. Negative, non-finite and large values retain native ceil/conversion behavior. Across 1,000 normal sampled vertices, guest instructions fall from 151,917 to 94,000. Another 1,019 edge/random-bit cases match output bytes, state and preserved registers.
- Sprite U/V interpolation bypasses redundant calls and VFP saves on a valid native cache hit. It retains the original interpolation arithmetic and native sprite resolution for empty/stale caches, including the absent-atlas revision sentinel. Across 1,680 cases, cache-hit instructions fall from 20,160 to 12,880 and tested refresh paths from 71,960 to 59,080.

These are routine-level guest instruction counts, not hardware FPS measurements. GPU allocation policy, resource lifetimes and fragmentation are unchanged. Frustum simplification, particle collision shortcuts, retained lighting queues and additional chunk-storage caches are not included. Existing chunk, mesh, light, texture, overlay and prompt checks also pass; hardware timing and long-session stability remain for testing.

Private muted Azahar exercised mixed touch/controller page selection, dropdown cancel, a sensitivity-slider change, and the touch-stick preview. Parent selection remained fixed while the touched dropdown handled direction input. The copied legacy MC3DS3 profile retained every existing binding and setting byte when saved by the unchanged MC3DS4 migration code, adding only the four established chunk defaults. The test also loaded an existing world with the combined rendering payload, reopened Mod Settings from pause, and returned to the title through Save & Quit. The private emulator was closed after testing. Evidence is kept separately under `analysis-codex-2/compact-ui-20261014/`; this is not a timing benchmark.

The source-folder cleanup is behavior-neutral: the candidate IPS is byte-identical before and after moving files, and the rebuilt host patcher produces the same baseline executable and IPS. Fresh Linux builds pass all five available CTests, including private update-profile checks; a Windows MinGW cross-build also succeeds. Ten release-validation tests and the pinned release checksum pass. The ARM settings and rendering suites pass again against the reorganized build.

Known pre-existing rebuild limitation: `--check` for the legacy performance, touch-stick, standalone-overlay and non-native-controls payload headers does not match the checked-in data with the current source/toolchain. Each fails on the pre-cleanup commit too, and each generator produces identical bytes before and after the folder moves. Their embedded byte arrays were not regenerated. This is separate from the reproducible native-menu candidate and ordinary host builds. The comparison is recorded in `legacy-builder-checks.json` beside the candidate's validation reports.

Reproduce with Clang/LLD, LLVM tools, Python and Unicorn, using private original EU game data. `buildRuntimeFixture` supplies the preserved baseline and sibling `code.ips`; substitute local paths below. Keep all generated artifacts separate from the pinned release.

```sh
python3 scripts/buildRuntimeControls.py --reference-code STOCK_CODE --native-menu --output-header build-ui-layer/RuntimeControlsData.h --artifacts-dir build-ui-layer/payload
python3 scripts/buildRenderFastPaths.py --reference-code STOCK_CODE --output build-ui-layer/render
python3 scripts/buildNativeMenuPrototype.py --stock-code STOCK_CODE --baseline-code BASELINE_CODE --payload build-ui-layer/payload --render-fast-paths build-ui-layer/render --output build-ui-layer/fixture
python3 scripts/checkRenderFastPaths.py --reference-code STOCK_CODE --patched-code build-ui-layer/fixture/code.bin --output build-ui-layer/render-checks.json
python3 scripts/checkRuntimeSettings.py --reference-code STOCK_CODE --patched-code build-ui-layer/fixture/code.bin --symbols build-ui-layer/payload/symbols.json --world-baseline BASELINE_CODE --overlay-symbols build-ui-layer/payload/symbols.json --overlay-background --texture-romfs STOCK_ROMFS --output build-ui-layer/settings-checks.json
```

## Restart-frozen chunk settings experiment

The separate `release/experiments/chunk-settings-20261014/` candidate adds render distance, nearby chunk radius, map radius and deferred mesh-builder count to the native Display page. Defaults preserve radius 1 rendering, the exact existing 25-entry snake-order neighborhood, a 32-block resident expansion, a 24-block map radius and four deferred builders. Existing release files and production payload headers are preserved.

Radius 1/2/3 neighborhoods contain 9/25/49 offsets. All four consumers and the native shuffle share the same extent. Their loading totals are 18/50/98 stages, with strict readiness comparisons against 17/45/89 and matching floating-point progress divisors. Sound preloading and transition callbacks are retained. The nearby table also drives chunk ticking and entity checks, so this is not a graphics-only or exact loaded-chunk-count option.

The original 57-entry BSS allocation contains the bounded 49-entry maximum table after its existing three-entry prefix, followed by 32 bytes of active settings in the unused tail. Two straight-line initializer ranges are replaced by a bounded loop; intervening literals and the surrounding constructor remain intact. No writable executable memory, new BSS, allocation, heap fallback or live-world resize is introduced. Runtime settings state grows by eight bytes for the four serialized values. Active settings are copied once after options load, then remain immutable until restart. Larger radii need physical-hardware memory/stability testing.

The chunk-radius request retains the native local/server path and acceptance. Its requested value is `max(min(render, nearby), nearby - 1)`, preserving a generation margin around the ticking neighborhood. The native source still adds its original 32-block padding. The render-frame call separately caps visible blocks by the selected render distance and native acceptance. This avoids a reproduced loading stall at render 1 / nearby 3 without bypassing completion checks. Defaults retain the original request of one. Map-radius changes remain separate from map redraw throttling. Builder limits retain native queue retry and the independent immediate builder. Visual presets do not change these four settings.

Focused ARM tests cover all 144 allowed combinations, 4,032 hook fragments, guarded table writes, register/flag contracts, 12 real loading-loop scenarios, three native shuffle extents, nine local radius requests, nine initial-constructor radius requests, 45 accepted-radius bounds at the actual render-frame call site, restart freezing and default-table equivalence. The constructor has its own initial-radius path, which bypasses the regular request function and therefore needs a separate hook. Other menu, texture, prompt, overlay, lighting and builder regression checks remain separate. Emulator scenarios and their exact executable hashes are recorded in the candidate's validation reports; they are not hardware benchmarks.

Private muted Azahar loaded the copied world with the 49-offset neighborhood and an accepted render radius of three, then exercised movement, jumping and Save & Quit. A fresh packaged-IPS process loaded the saved position with nine offsets, map radius eight and one deferred builder, broke blocks with visible mesh updates and saved again. Selecting render radius three with nearby radius one correctly produced an accepted render radius of one. These runs used low-memory textures, hidden weather/particles and the diagnostic overlay; they do not establish that maximum distances are safe with every texture setting or world.

The mixed render-1/nearby-3 test exposed a separate loading dependency: progress remained at 0.752357 in loading state 2 while the tick counter advanced. The final executable requests radius two for that combination, keeping the selected visible distance at 16 blocks through the render-frame cap. A fresh private run reached loading state 16/progress 1 and entered the saved world with this configuration and one builder. The report distinguishes these final checks from the earlier maximum/minimum runs. No loading thresholds or sound-preload stages were bypassed.

## 2026-10-14 settings and diagnostics experiment

The separate candidate in `release/experiments/settings-next-20261014/` extends the native Mod Settings menu. The v0.4.18 release and the earlier `native-menu-20261014` IPS remain preserved. The normal patcher's generated payload headers are not promoted to this experiment.

Menu rebuilding releases the previous pane/navigation references before constructing replacement rows, skips seven unused stock widgets on the custom screen, and avoids rewriting unchanged button labels. The skipped widgets account for at least 2,220 requested object bytes, excluding owned strings and rendering resources. These are ownership/allocation changes, not a measured net heap saving. Native menu state grows from 428 to 488 bytes; runtime state grows from 4,808 to 4,960 bytes, including the 144-byte diagnostics history. There is no BSS or bootstrap expansion, new heap fallback, or changed graphics allocation policy.

The native light-propagation getter now calculates its section index once, uses it for both lookup and the touched-section write, and delays local-coordinate decoding until a brightness buffer exists. It has no cache, allocation or queue-policy change. Across 4,500 native differential cases, present-section paths use 39 rather than 42 guest instructions, missing-section paths use 22 rather than 36, and missing-light paths use 28 rather than 42. Seven integrated propagation scenes match block/light values, touched sections and processing order, using 2.22% to 2.75% fewer guest instructions. These controlled scenes do not measure whole-game FPS or physical block-edit latency. The older experimental ring queue is not included.

New performance presets change only particles, weather visuals, map refresh and texture quality:

| Preset | Particles | Weather | Map | Textures |
| --- | --- | --- | --- | --- |
| Quality | Normal | On | Normal | Normal |
| Balanced | Reduced distance | Off | 4 Hz | Normal |
| Low memory | Hidden | Off | 1 Hz | Low memory |

Custom is derived from the individual values, not a fourth stored preset. Presets do not alter bindings, camera, simulation or overlay settings. Texture changes still require a full restart; the menu shows a pending-restart notice until the selected and active texture modes match.

The Full overlay adds largest-free-payload figures for Main, Graphics and Stream, sampled by walking each native free list under its arena mutex. They are not alignment-aware allocation guarantees, and the nested heaps must not be added together. The optional graph shows the last 32 displayed-frame intervals, with a 33.3 ms guide and a 100 ms display ceiling. Graph and heap diagnostics have overhead that remains unmeasured on hardware.

The historical 2 MiB allocation failure is consistent with texture storage: its resource kind follows the native texture allocation path. The exact ultimate caller has not been established. Upload staging uses a different memory kind and is not evidence for that failure's cause. The candidate does not claim to fix general fragmentation, GPU resource lifetime, or every low-memory failure.

Validation reports ship separately from the IPS. Native checks cover menu value mapping, quantization, 14 scroll-memory cases, dropdown focus isolation, 72 overlay layouts, graph history and locked free-list traversal, alongside the retained texture, prompt, mesh and builder tests. Host tests cover version-2 migration, camera curves and presets. Private emulator evidence is under `analysis-codex-2/settings-next-20261014/`; hardware timing, long sessions and multiplayer remain outstanding.

Private muted Azahar also exercised shared B capture, preset changes and restart notices, native slider changes, page-scroll restoration, visible frame-time graph rendering, block breaking with mesh updates, movement, jumping, item pickup and Save & Quit. A fresh process using the packaged IPS reloaded the edited terrain, position and collected dirt. The final mixed touch/controller dropdown guard passed both native tests and emulator navigation. No monitored allocation failure was observed. The diagnostic repeat did not reproduce a request of at least 2 MiB, so it did not identify the historical failing texture's ultimate caller. These flows are correctness smoke tests, not timing or net memory-saving measurements. The packaged emulator report distinguishes early revisions from final-executable checks.

## v0.4.18 chunk-builder memory headroom

The update caps the native deferred chunk-builder pool at four instead of ten, while retaining the independent immediate builder. Each builder owns a 491,600-byte array of ten render-layer position lists. Avoiding six builders removes 2,949,600 bytes of these requested allocations, plus their other owned buffers. All ten render layers and the full per-layer position capacity remain intact. The native queue retries dirty chunks when no builder is available; completed builders return through the existing worker lifecycle. This trades queue depth for memory, not render distance or world simulation.

There is no graphics-to-main fallback, heap-budget increase, GPU-buffer relocation, shared cross-thread scratch array or new persistent allocation. Bootstrap metadata, BSS size, save format and base-only outputs are unchanged from v0.4.17.

The native regression checks twelve cap inputs, including zero and negative error values, eight create/clear cycles with controlled resource constructors and allocators, and 120 empty-pool retries with unchanged state. Existing settings, texture, prompt and mesh regressions also pass against the exact candidate. `scripts/nativeMemoryChecks.py` is run by `scripts/checkRuntimeSettings.py`. These checks do not prove GPU lifetime safety or hardware performance. The earlier fragmentation and 2 MiB allocation failures remain historical failures, not demonstrated fixes.

In private muted Old 3DS-mode Azahar runs loading the same copied world and settings, graphics free space was 2,102,256 bytes before this change and 5,125,632 bytes with it. The largest free block rose from 1,884,812 to 5,109,020 bytes. This is about 2.88 MiB more free graphics memory in those snapshots, not a guaranteed gain in every world or an FPS measurement. After cycling all four control modes and all six settings pages, the candidate still had 5,125,672 free bytes with a largest block of 4,935,980 bytes. A consistent paused graphics allocation-list read found one remaining 491,600-byte scratch allocation in that arena. The main and graphics heaps are nested and must not be added together.

The production candidate also passed block breaking with visible mesh updates, walking, jumping, item pickup, saving back to the title screen and same-process world reload. The broken terrain and collected dirt persisted, and the control mode, bindings and settings were unchanged. After reload the graphics arena had 5,628,900 free bytes with a largest block of 5,467,100 bytes; the parent main arena had 5,844,260 free bytes with a largest block of 2,095,880 bytes. This does not establish that every arena avoids fragmentation. No monitored native allocation failure occurred during this smoke test. Scripted menu navigation needed retries and included debugger pauses, so this is not a loading or UI timing benchmark. The copied source saves remained unchanged.

The baseline's large-allocation diagnostic did not reproduce the earlier 2 MiB request during its world load, Controls visit and Save & Quit. Its ultimate caller therefore remains unresolved. The candidate retains the original allocation/failure policy and does not claim to fix fragmentation generally. The first candidate's full main-heap inventory was deliberately interrupted because it was too intrusive; it is not counted as a completed validation run. Evidence is retained under `analysis-codex-2/memory-stability-20261013/` in the private reversal workspace. Canonical ownership names and annotations were saved through IDA and the dashboard export refreshed separately from these runtime results.

## v0.4.17 settings and mesh work

The unified settings patch integrates two previously researched mesh-construction fast paths: skipping fully hidden block faces and reading biomes through an already-valid native chunk cache. Native execution of the exact release candidate passes 258 hidden-face cases and 2,636 biome cases. The 6,400-query biome trace uses 399,360 rather than 672,000 guest instructions; representative cache hits use 26 rather than 103. Misses can cost more. These are routine-level instruction counts, not Old 3DS FPS gains. The earlier roughly 0.9% hidden-face worker-time experiment below remains the only matched emulator timing for that component.

Optional reductions affect rendering, not world simulation: particles can use half draw distance or be hidden, weather rendering can be hidden, and the bottom map can refresh normally, at 4 Hz, at 1 Hz or be paused after initialization. Low-memory textures promote existing mip levels for strictly validated three-mip terrain atlases after a full restart. They halve atlas dimensions and omit the largest stored level; they do not alter unrelated textures, assets on disk or allocator policy. No FPS cap is added.

Validation also passes 1,204 native integer-division cases, 25 visual-gate cases, 32 prompt-rendering ABI cases, 12 overlay mode/context/callback cases, and 24 terrain-loader cases using all 12 eligible reference RomFS atlases with the option both off and on. Texture tests execute the native read/descriptor path with controlled archive/GPU allocation backends and compare exact payload bytes, dimensions, offsets and mip count. Host tests cover version-1 migration, malformed settings, shared/unbound bindings, ABXY directions/chords, camera toggling, touch geometry and texture guards, including an AddressSanitizer/UndefinedBehaviorSanitizer run. Five configured CTest tests pass. Native builder equivalence passes nine keyboard/dictation cases. Reproduction uses `scripts/checkRuntimeSettings.py` and `scripts/checkControlBuilders.py` with private stock/patched executables and Unicorn; no game data is shipped.

Private Azahar testing uses Old 3DS mode, an isolated virtual display, muted audio and copied user data. It covers the settings pages, custom camera chords, unbinding a camera direction, Jump on B, persistence across a fresh process, ABXY/chord camera movement in a loaded world, and an in-world cycle through all four modes. Low-memory textures loaded an existing world and skipped 1 MiB of source atlas payload in that run. This counter is not a measured reduction in resident graphics memory. Console timing, long sessions, multiplayer, physical accessory input and graphics-allocation stability remain unverified.

The existing graphics-arena failure is not claimed fixed. Earlier sampled evidence showed a failed 51,840-byte request with 656,188 free bytes but no block larger than 17,100 bytes, demonstrating fragmentation in that run. A separate in-world Options transition requested 2 MiB and failed. The latest low-memory/ABXY run completed its Controls visit without hitting a monitored native allocation-failure breakpoint; afterwards the graphics arena had 1,653,032 free bytes, with a largest block of 1,567,004 bytes. A repeat pass does not resolve the earlier failure. An experimental parent-heap fallback is excluded: synthetic success is insufficient evidence for safe live graphics ownership. The current patch retains native allocation/failure policy and reports this limitation. Evidence: `analysis-codex-2/runtime-settings-20261013/` in the private reversal workspace.

The dated sections below record separate research packages, not additional optimizations silently included in v0.4.17. In particular, the experimental lighting queues and broad helper-call substitutions are not part of this build.

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
