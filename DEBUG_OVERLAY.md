# Custom update overlay

## New settings experiment

`release/experiments/settings-next-20261014/code.ips` adds **Mod Settings > Page: Overlay** with four corners, scale, background opacity and an optional frame-time graph. Enable FPS or Full on the Display page. The background stays RGB 20, 20, 20, with 75% opacity by default. Scale defaults to 100%; long Full-mode lines shrink further if needed to stay within the screen. Off and menus suppress the overlay, and it never captures touch.

Full mode adds `Largest M/G/S`, the largest free payload in each Main/Graphics/Stream arena. These values are sampled once per completed timing window under native arena locks. They can reveal fragmentation, but are not alignment-aware promises that a request will succeed. Main and Graphics are nested, so do not sum their totals.

The optional graph displays the most recent 32 displayed-frame intervals, with a 33.3 ms guide. Bars above 34 ms are orange and heights are capped at 100 ms. It is not CPU/GPU utilization. Its history occupies 144 bytes including the heap samples, within the existing runtime heap allocation. Rendering and free-list traversal add unmeasured overhead; use Off for comparisons without diagnostics.

Scaled text uses a scoped native model-view transform and restores it after each draw. It does not change the font's scale field, which would bypass the native font cache. The same helper fits long textual button prompts into their original slot. Native tests check the transform/shadow ABI and graph winding; final emulator rendering is checked separately because mocked renderer tests alone cannot establish visible geometry.

The candidate is built as one integrated native-menu payload, not by combining the separate overlay builder with it. Its release directory contains build and validation reports. The preserved v0.4.18 and original native-menu experiment below remain unchanged.

## Earlier native-menu background experiment

The separate native-menu experiment in `release/experiments/native-menu-20261014/code.ips` adds a padded background behind the visible overlay lines: RGB **20, 20, 20**, alpha **0.75**. Width follows native font measurement and height follows FPS versus Full mode. It draws before the text, does not capture touch, and is absent when the overlay is off or a menu is open. The preserved v0.4.18 release is unchanged.

The experimental renderer is 2,308 bytes, bounded before the existing heap helpers. It formats at most five stack buffers, measures them with the stock font and submits one native GUI rectangle. ARM checks verify geometry, RGBA, draw order, all three modes, menu suppression and callback preservation. Native rendering is checked separately in private muted Azahar; these checks do not establish hardware performance or long-session stability.

The European v9.11.0 update includes an optional bottom-screen overlay. In v0.4.17 choose **Options > Controls > Page: Display > Overlay**, then select Off (default), FPS, or Full. Every input mode uses the same setting and IPS. The base-game build does not support this overlay.

The legacy `--overlay` flag is still accepted for update inputs but does not enable it or produce a separate patch. The overlay appears only in gameplay. Its top-left position keeps it above the default touch stick, although it overlaps part of the native hotbar and custom stick positions can overlap it.

- FPS: newly accepted upper-screen buffers per second, averaged over a window of at least one second.
- The adjacent milliseconds value: average interval between those frames.
- Peak: longest interval in the last completed window, not CPU or GPU execution time.
- Main, Gfx and Stream: used / total arena memory in MiB, sampled once per completed valid timing window. These are game heaps, not total system RAM or GPU utilization.

FPS mode draws only the first timing line. Full adds peak and heap lines. Unsampled timings show `--`. A backwards clock or gap longer than five seconds resets the window. Font positions have fixed spacing and shadowed text. The overlay does not intercept touch input.

## Installation

Generate the unified update patch with:

```sh
./mc3ds-patcher "Minecraft-update.cia" -o "minecraft-luma"
```

Keep the compatible Old 3DS bootstrap base and update installed. Back up the current `code.ips`, then replace only `/luma/titles/000400000017CA00/code.ips` with the matching control-profile output. Fully close and relaunch Minecraft with Luma game patching enabled.

No CIA reinstall, shader replacement or ExHeader override is required. Do not add `code.bin` or `exheader.bin`. Enable or disable the overlay in-game and leave Options to save.

## Implementation and safety checks

The freestanding C++ renderer replaces the 3,032-byte stock frame-timer function at `0x35CADC`. Its state fits the stock palette's 112-byte BSS slot at `0xAC3818`. There is no executable-page, bootstrap-BSS or LayeredFS-tail growth.

The renderer and callback wrapper are always installed. Off returns before drawing or heap queries, skips timing collection, and resets the sampling window while still calling the original display callback. FPS skips heap queries. Heap optimization helpers retain their fixed positions later in the same retired renderer reservation.

The display callback pointer at `0x4F903C` redirects to an authored wrapper. It counts only upper-screen callbacks with a pending new buffer, then preserves the original callback's state transitions and return value. Repeated VBlanks, lower-screen callbacks and extra draw calls do not increment FPS. The GPU submission path itself is unchanged.

The callback owns the timing accumulator. The drawing thread reads a small atomic, sequence-checked publication and skips a draw if publication is in progress. Heap queries remain on the drawing thread. The overlay uses the game's shadowed font ABI, with a bounded stack buffer and negative shared-string reference count so retained cache entries copy the text. The existing font cache can still allocate; this is not a zero-allocation or zero-cost overlay.

The patcher verifies checksums of the renderer, dispatch, font, tick, heap and display-callback routines. It validates state bounds, patch overlap, the unified output hash and IPS round trips. `--allow-similar` does not bypass these guards.

The monotonic tick frequency is `268111856` Hz, consistent with the ARM11 constant in [libctru's OS header](https://github.com/devkitPro/libctru/blob/master/libctru/include/3ds/os.h).

## Validation

The v0.4.17 ARM checks cover 12 Off/FPS/Full, menu/gameplay and upper/lower display-callback cases, including preserved registers, original callback results and skipped heap/tick calls. Private muted Azahar checks show Full in gameplay and no overlay over menus. These checks supplement, rather than repeat, the historical timing tests below.

Previous private native tests covered timing/formatting, payload validation and both stock-update fixture profiles. ARM execution tests covered 1, 5, 20, 30 and 60 FPS, original callback execution, repeated callbacks/draws, multiple pending slots, clock rollover, pause-gap reset, bounded writes, stack balance, R4-R11/D8-D15 preservation, font arguments and concurrent publication rejection. Font rendering and heap APIs used checked stubs in those ARM tests. The test suite is no longer included in the source build; these results describe historical validation.

Private muted Azahar tests verify actual text rendering separately. Early prototypes incorrectly counted 90-120 render/submission calls per second; those prototypes are not the release. The display-callback build shows approximately 59-60 FPS in the title/menu test and successfully loads the existing test world. A gameplay capture shows 58.8 FPS with Azahar reporting 59 FPS. Emulator timing is not evidence of Old 3DS hardware performance. Console boot, physical sleep/wake, long sessions, multiplayer and accessory behavior remain unverified for this overlay.

## Rebuild and verify payload

The generated `src/data/OverlayData.h` is built with Clang/LLD and LLVM objcopy/nm:

```sh
python3 scripts/buildOverlayPayload.py --reference-code /path/to/stock/update/exefs/code.bin --artifacts-dir /path/to/overlay-artifacts
python3 scripts/buildOverlayPayload.py --reference-code /path/to/stock/update/exefs/code.bin --check
cmake --build build
```

Payload regeneration requires your own reference executable. The normal source build uses the generated header and needs no private fixture. No game binary is committed or packaged.

The default-control Azahar run also completed Save & Quit and returned to the title with the overlay still updating. CPP has build/fixture coverage but no separate overlay gameplay run. Detailed private evidence is retained under `analysis-codex-2/overlay-v048-20260911/` in the analysis workspace.
