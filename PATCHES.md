# Included patches

This includes the full patch set from the working Old 3DS build:

- Old 3DS launch flags, 80 MB application mode, normal CPU/cache settings, and no extra-core requirement.
- 20 MB graphics heap and 6.5 MB system heap.
- Compact 25-chunk table, render distance 1, resident radius 2, and map update radius 24.
- Input fix so the missing C-stick/accessory no longer blocks buttons.
- L + Circle Pad camera, B Place/Use, and L + Y Drop.
- Fancy graphics, transparent leaves, and fancy skies forced off.
- Pickup animations tick and expire with fancy graphics off, fixing items that remained visible after entering the inventory.
- The game's existing FPS/memory overlay enabled.

The patcher finds 59 code sites by signature and refuses missing or ambiguous matches. It also checks the complete pickup animation tick/deletion loop, its connection to the particle update call, and the stack frame used by the trampoline. Memory-layout checks and output verification remain enabled. The game assets are unchanged.

Patch definitions are in `src/SignatureData.h`, patching logic is in `src/Patches.cpp`, and the authored camera, input, and pickup assembly is in `patches/`.

## Pickup correction in 0.1.1

Disabling fancy graphics also skips the particle engine's update. Picking up an item still transfers it to inventory, but its separate visual animation never ages or gets deleted. The correction sends the low-graphics branch through the game's existing pickup animation loop. Ordinary particles and weather retain their low-graphics behavior; the full-graphics branch retains its original update.

For the supported executable, the hook is at `0x25e578` and uses 24 bytes of text padding at `0x680398`. It enters the pickup loop at `0x232a18` and returns to `0x25e594`. The patcher derives these addresses from signatures and the executable layout. It adds no persistent allocation or memory pages. Source: [ItemPickup.s](patches/ItemPickup.s).

The corrected executable passed 348 ARM execution cases covering animation aging, deletion, both graphics branches, and register/stack preservation. Azahar checks covered repeated drop/pickup cycles, mining, and Save & Quit. See [compatibility](COMPATIBILITY.md) for hardware testing limits.
