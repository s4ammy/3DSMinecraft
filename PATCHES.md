# Included patches

Both profiles include:

- Old 3DS launch settings and 80 MB application memory.
- A 20 MB graphics heap and 6.5 MB system heap.
- A compact 25-chunk table, render distance 1, resident radius 2, and map radius 24.
- Working buttons when the C-stick or accessory is missing.
- A choice of L + Circle Pad or Circle Pad Pro controls.
- Fancy graphics, transparent leaves, and fancy skies disabled.
- Pickup animations that expire correctly with fancy graphics disabled.

The base profile also enables the game's FPS/memory overlay. The update adds a CPU 1 fix for its initialization worker and adjusts its chunk-loading thresholds. Game assets are unchanged.

## How it finds the code

The base profile uses `SignatureData.h`; the update uses `UpdatePatchData.h` and `UpdatePatches.cpp`. The update has 57 edit signatures and 13 anchors. Hook locations, SDK entry points, global pointers, and extra storage are resolved from matches and the executable layout. No fixed game addresses are used as update patch targets.

The patcher rejects missing or ambiguous signatures, unexpected instructions, incompatible memory layouts, and unrecognized chunk-table references. It rebuilds and re-extracts the CIA to verify the result. Other-build matching remains experimental.

## Pickup and accessory behavior

Low graphics skipped the particle engine, leaving pickup animations visible after items entered inventory. The pickup hook runs the game's existing animation tick and deletion loop. The update needs its own hook because its stack frame and object layout differ.

CPP mode uses the game's native IR accessory support. Connection attempts run in a background worker so a missing accessory does not freeze ordinary controls. Shutdown joins that worker before releasing accessory resources. There is no automatic L + Circle Pad fallback.

Authored assembly is in [patches/](patches/). See [controls](INPUTS_EXPLAINED.md) and [compatibility](COMPATIBILITY.md).
