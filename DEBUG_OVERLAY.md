# Custom update overlay

Version 0.4.8 adds an always-visible bottom-screen overlay for the European v9.11.0 update, with either control profile. It does not add a settings toggle or modify the base-game build.

- FPS: newly accepted upper-screen buffers per second, averaged over a window of at least one second.
- The adjacent milliseconds value: average interval between those frames.
- Peak: longest interval in the last completed window, not CPU or GPU execution time.
- Main, Gfx and Stream: used / total arena memory in MiB, sampled once per completed valid timing window. These are game heaps, not total system RAM or GPU utilization.

Unsampled timings show `--`. A backwards clock or gap longer than five seconds resets the window. Font positions have fixed spacing and shadowed text. The overlay occupies part of the map/menu area; it does not intercept touch input.

## Installation

Keep the compatible Old 3DS bootstrap base and update installed. Back up the current `code.ips`, then replace only `/luma/titles/000400000017CA00/code.ips` with the matching control-profile output. Fully close and relaunch Minecraft with Luma game patching enabled.

No CIA reinstall, shader replacement or ExHeader override is required. Do not add `code.bin` or `exheader.bin`. Restoring the previous IPS removes this overlay and returns to the previous build.

## Implementation and safety checks

The freestanding C++ renderer replaces the 3,032-byte stock frame-timer function at `0x35CADC`. The payload is 2,836 bytes. Its 104-byte state reuses the stock palette's 112-byte BSS slot at `0xAC3818`. There is no executable-page, bootstrap-BSS or LayeredFS-tail growth.

The display callback pointer at `0x4F903C` redirects to an authored wrapper. It counts only upper-screen callbacks with a pending new buffer, then preserves the original callback's state transitions and return value. Repeated VBlanks, lower-screen callbacks and extra draw calls do not increment FPS. The GPU submission path itself is unchanged.

The callback owns the timing accumulator. The drawing thread reads a small atomic, sequence-checked publication and skips a draw if publication is in progress. Heap queries remain on the drawing thread. The overlay uses the game's shadowed font ABI, with a bounded stack buffer and negative shared-string reference count so retained cache entries copy the text. The existing font cache can still allocate; this is not a zero-allocation or zero-cost overlay.

The patcher verifies checksums of the renderer, dispatch, font, tick, heap and display-callback routines. It validates state bounds, patch overlap, both profile hashes and IPS round trips. `--allow-similar` does not bypass these guards.

The monotonic tick frequency is `268111856` Hz, consistent with the ARM11 constant in [libctru's OS header](https://github.com/devkitPro/libctru/blob/master/libctru/include/3ds/os.h).

## Validation

Native CTest covers timing/formatting, payload validation and both stock-update fixture profiles. ARM execution tests cover 1, 5, 20, 30 and 60 FPS, original callback execution, repeated callbacks/draws, multiple pending slots, clock rollover, pause-gap reset, bounded writes, stack balance, R4-R11/D8-D15 preservation, font arguments and concurrent publication rejection. Font rendering and heap APIs use checked stubs in those ARM tests.

Private muted Azahar tests verify actual text rendering separately. Early prototypes incorrectly counted 90-120 render/submission calls per second; those prototypes are not the release. The display-callback build shows approximately 59-60 FPS in the title/menu test and successfully loads the existing test world. A gameplay capture shows 58.8 FPS with Azahar reporting 59 FPS. Emulator timing is not evidence of Old 3DS hardware performance. Console boot, physical sleep/wake, long sessions, multiplayer and accessory behavior remain unverified for this overlay.

## Rebuild and test

The generated `src/OverlayData.h` is built with Clang/LLD and LLVM objcopy/nm:

```sh
python3 scripts/buildOverlayPayload.py --reference-code /path/to/stock/update/exefs/code.bin --artifacts-dir /path/to/overlay-artifacts
python3 scripts/buildOverlayPayload.py --reference-code /path/to/stock/update/exefs/code.bin --check
cmake --build build
ctest --test-dir build --output-on-failure
python3 tests/verifyOverlayArm.py --reference-code /path/to/stock/update/exefs/code.bin --artifacts-dir /path/to/overlay-artifacts
```

The last command needs the Unicorn Python package. Configure `MC3DS_UPDATE_FIXTURE` to the private extraction directory to include full fixture tests. No game binary is committed or packaged.

The default-control Azahar run also completed Save & Quit and returned to the title with the overlay still updating. CPP has build/fixture coverage but no separate overlay gameplay run. Detailed private evidence is retained under `analysis-codex-2/overlay-v048-20260911/` in the analysis workspace.
