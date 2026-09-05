# Included patches

This includes the full patch set from the working Old 3DS build:

- Old 3DS launch flags, 80 MB application mode, normal CPU/cache settings, and no extra-core requirement.
- 20 MB graphics heap and 6.5 MB system heap.
- Compact 25-chunk table, render distance 1, resident radius 2, and map update radius 24.
- Input fix so the missing C-stick/accessory no longer blocks buttons.
- L + Circle Pad camera, B Place/Use, and L + Y Drop.
- Fancy graphics, transparent leaves, and fancy skies forced off.
- The game's existing FPS/memory overlay enabled.

The patcher finds 58 code sites by signature and refuses missing or ambiguous matches. Memory-layout checks and output verification remain enabled. The game assets are unchanged.

Patch definitions are in `src/SignatureData.h`, patching logic is in `src/Patches.cpp`, and the authored camera/input assembly is in `patches/`.
