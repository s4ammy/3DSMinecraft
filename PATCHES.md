# Included patches

Both profiles include:

- Old 3DS launch settings and 80 MB application memory.
- A 20 MB graphics heap and 6.5 MB system heap.
- A compact 25-chunk table, render distance 1, resident radius 2, and map radius 24.
- Working buttons when the C-stick or accessory is missing.
- A choice of L + Circle Pad or Circle Pad Pro controls.
- Fancy graphics, transparent leaves, and fancy skies disabled.
- Pickup animations that expire correctly with fancy graphics disabled.
- Hooks and payloads that remain inside the bootstrap title's mapped executable page without overlapping Luma's LayeredFS injection window.

The update adds a CPU 1 fix for its initialization worker and adjusts its chunk-loading thresholds. Version 0.4.8 also adds a custom FPS/debug overlay to both update control profiles. The base-game overlay remains disabled. Game assets are unchanged.

The v0.4.14 candidate adds a third update control mode, `touch-stick`, retaining the original button bindings. It uses 16 bytes at the existing input-state BSS extension and reclaims the retired frame-timer renderer's quad/label helpers. Its remaining drawing code occupies the unused accessory payload area in this mode. It fits beside the optional debug overlay and respects both bootstrap LayeredFS reservations. Current-screen checks and a HUD-destruction hook prevent menu capture and stale HUD pointers.

All update modes now select the existing priority background rebuild path for nearby dirty chunks. The guarded instruction at `0x1d5990` supplies zero for the synchronous argument to `CLevelBuilder::buildChunk`. That function already routes zero to the queued path with near-chunk priority. The original mesh replacement, lighting, biome colours, job ownership and exhausted-pool retry logic are retained. An Old 3DS user confirmed reduced hitching with v0.4.14 but reported about one second of delayed block visuals. Base outputs are unchanged.

Version 0.4.15 consumes touch-stick gestures in all logical controller slots because they share the touchscreen. The current local revision injects camera axes only into slot 3, the final slot selected by physical input. The previous slot-0 routing could replay old menu buttons from an inactive queue. Using only slot 3 also avoids adding touch events to unused queues. It also hooks the worker scheduler's nonpositive-budget exit at `0x16fb48` to process at most one completed streaming job, using the native locked queue and callback cleanup. This gives finished mesh replacements a chance to run during late frames without starting new synchronous rebuild work. A 36-byte helper fits at `0x35d634` between the existing heap helpers. Its hardware latency benefit is pending validation.

The update profile reuses one per-call 16 KiB scratch buffer while decompressing up to six chunk streams. Its biome-layer random helper detects positive power-of-two bounds and computes the exact floor modulus with a mask, while retaining the original signed divider for every other bound. Both five-second startup presentation phases are reduced to half a second. These timing values do not replace initialization waits.

Version 0.4.6 stores scratch ownership outside `z_stream`, correcting the v0.4.5 `total_in` overlap. It also adds:

- An in-place cached block getter retaining the original cache-miss resolver and air fallback.
- Final-cell-only seed initialization in the deterministic climate edge pass, preserving final RNG state.
- Direct bounded insertion from live compressed record slices, removing their temporary allocation and copy.
- Direct inflate output into unshared string capacity when a complete 16 KiB window fits. Shared or smaller buffers retain the original scratch-and-append path. Error handling restores the original terminator.

The experimental Voronoi cache is not enabled because small requests regressed. See [measurements and validation limits](PERFORMANCE.md).

Version 0.4.7 adds:

- A direct raw chunk-light getter preserving fallback values, the original return convention, and invalid-coordinate assertions.
- Reuse of one inflate state/window allocation within a chunk call, resetting independent streams and cleaning up on success and error. The existing stack frame is unchanged and no global context is added.
- Exact modulo-eight masking for biome-color RNG, preserving every refresh, color write and final RNG state.

The ready-flag-only biome refresh cache is experimental and excluded. A counterexample with newly available neighboring chunks retained stale colors. Previous private tests checked that the original refresh call remained intact.

Version 0.4.9 adds a bidirectional search for neighboring free blocks inside the already-locked SDK heap free routine. It immediately uses the tail when freeing a later range, otherwise chooses forward or backward traversal using the head/tail address midpoint. It finds the same nodes, then resumes the original coalescing and insertion code. Allocation policy, list ordering, alignment restoration and all allocated addresses stay unchanged. No extra cache, allocation or memory is introduced. Some short-list paths execute extra instructions, so it is not a claim that every individual free is faster.

Version 0.4.10 specializes forward allocation searches for four-byte alignment. Free-block headers are already four-byte aligned, so the search omits redundant per-node address rounding. It preserves first-fit/best-fit selection, tie order and every selected address, then resumes the original split-and-commit routine. Other positive alignments fall back to the original search with small entry overhead; negative alignments bypass the new hook. The 72-byte payload adds no stack frame, allocation, cache or BSS and occupies unused space in the existing retired renderer reservation. It does not remove locks or change memory ownership.

## How it finds the code

### In-game controls (v0.4.16)

The European update uses one runtime controls payload. It adds native category-style rows to Options > Controls, including the native select-release path required for A-button activation inside a subpage. It remaps physical buttons before native gameplay/menu action dispatch and preserves stock navigation on the Controls page itself. The selected mode and three independent 27-binding profiles are serialized as `mc3ds_controls_v1` in the game's existing options file. Missing or malformed values retain defaults. Leaving Options uses the normal save path; no new SD permissions or file system implementation is introduced.

The payload reclaims three unrolled keyboard-binding builders by replacing them with equivalent table-driven builders, without deleting their mappings. Original entry points remain callable. ARM/Thumb string API interworking is explicit. Other code occupies retired timer-renderer helpers and existing input/accessory reservations. The existing 16-byte BSS extension holds the state pointer; a 4,576-byte heap allocation contains profiles, UI/input state and the accessory worker stack. Bootstrap segment sizes and LayeredFS reservations are unchanged. The container fresh-press cancellation fix applies to every runtime mode.

`scripts/buildRuntimeControls.py` regenerates `src/RuntimeControlsData.h` using the private stock executable and Clang/LLVM 23.1.1. Its `--check` mode verifies reproducibility. Region hashes, exact input/output hashes, overlap checks and fixed capacities guard installation. `scripts/checkControlBuilders.py` compares stock and replacement ARM execution for eight keyboard-builder/layout cases with checked string/vector stubs. CTest covers defaults, every exposed action, shared buttons and chords in both groups and all three modes, held-button transitions, malformed settings, IPS round trips, both overlay variants and ABI rejection. These focused checks are separate from real-device validation.

Private, muted Azahar testing covers rendering, touch and A-button activation, mode changes, custom chords, unbinding, resetting defaults, custom menu confirmation and settings persistence across an emulator restart. The latest revision allows duplicate bindings without swapping or clearing other actions; a focused Controls-page check assigned Jump to B while leaving Use/Place on B, then verified both assignments after saving and restarting the emulator. Physical accessory input, console sleep/wake and long-session stability still need hardware testing. The canonical update IDA database contains the relevant names and annotations; its dashboard export was refreshed separately from these validation results.

The same candidate also loaded a saved world, exercised both camera modes, returned from repeated in-world Controls visits, and opened Inventory using a custom A+X binding. One earlier in-world Options transition failed a native 2 MiB graphics allocation. Short repeat tests passed, but the cause and any relationship to the new UI are unresolved. Treat this as an experimental build, not a stability-validated release. Private evidence is under `analysis-codex-2/runtime-controls-20261013/` in the reversal workspace.

Version 0.4.11 adds a default-controls container input fix. The common container back-state callback and three derived callbacks, including crafting, ignore releases. The container menu_cancel action performs its original selected-slot cleanup, then invokes the common callback with an explicit nonzero state to close through the screen's own virtual close method. The game-level back dispatcher and non-container screens keep their stock routing. This avoids treating the gameplay B-use release as Cancel without a timer or suppressing gameplay use. Base and update relocate a shared 16-byte ARM helper inside the existing 256-byte controls reservation. Action-handler, callback and action-dispatcher checksum guards reject incompatible layouts or overlapping patches. CPP does not receive this change.

`python3 scripts/checkContainerCancelPayload.py` checks the helper template against its assembly. Previous private fixture tests checked exact output hashes, original CPP behavior, IPS round trips and rejection of modified routines. Restoring the five changed instructions and clearing the helper reproduced the previous executable hash exactly. The test suite and fixture CMake options are no longer included in the source build.

Previous private Unicorn tests ran the actual ARM action dispatcher and callbacks, checking selected-slot cleanup order, non-cancel actions, release handling, stack balance and preserved registers. These historical routine tests do not replace in-game input testing.

The final update build passed hidden, muted Azahar tests for crafting-table and furnace tap/hold opening and closing, plus inventory and Options cancellation. Base validation is limited to fixture and ARM tests. Physical Old 3DS validation is still needed. The private evidence is recorded in `analysis-codex-2/reported-v030-20260911/FIX-v0411.md` in the analysis workspace.

The [v0.4.8 overlay](DEBUG_OVERLAY.md) replaces the obsolete frame-timer renderer in place. It counts new upper-screen buffers at the display callback, not every render call or VBlank. It shows a one-second FPS average, average/peak frame intervals and used/total main, graphics and streaming arena memory. No game settings or Mods page interception is added.

From v0.4.12 the overlay is disabled by default. `--overlay` installs the renderer and frame-statistics callback for update inputs only. Otherwise the renderer entry returns immediately, its draw gate unconditionally skips it, and the original display callback is retained. The heap helpers in the shared retired-renderer reservation remain installed in both cases. Each control/overlay combination has a separate expected output checksum.

The base profile uses `SignatureData.h`; the update uses `UpdatePatchData.h`, `PerformanceData.h`, `OverlayData.h`, `RuntimeControlsData.h` and `UpdatePatches.cpp`. Core hook locations, SDK entry points, global pointers, and extra storage are resolved from matches and the executable layout. Performance and overlay routines additionally have exact-layout checksum guards. Unified update controls require the exact v9.11.0 executable, including when `--allow-similar` is supplied.

The patcher rejects missing or ambiguous signatures, unexpected instructions, incompatible memory layouts, and unrecognized chunk-table references. It verifies all CIA content hashes during extraction, applies the generated IPS back to the original executable, verifies the result, and leaves the input unchanged. Other-build matching remains experimental.

The one-time bootstrap CIAs provide the Old 3DS ExHeader, SMDH compatibility, BSS size, processor affinity, SD-application capability, and LayeredFS-safe executable extent. The performance payload is placed after the LayeredFS injection windows used by both bootstrap control profiles and remains in the same mapped executable page. The generated `code.ips` contains only executable deltas. Do not place external `code.bin` or `exheader.bin` overrides beside it.

## Pickup and accessory behavior

Low graphics skipped the particle engine, leaving pickup animations visible after items entered inventory. The pickup hook runs the game's existing animation tick and deletion loop. The update needs its own hook because its stack frame and object layout differ.

CPP mode uses the game's native IR accessory support. Connection attempts run in a background worker so a missing accessory does not freeze ordinary controls. Shutdown joins that worker before releasing accessory resources. There is no automatic L + Circle Pad fallback.

Authored assembly is in [patches/](patches/). See [controls](INPUTS_EXPLAINED.md) and [compatibility](COMPATIBILITY.md).
