# Included patches

This includes the full patch set from the working Old 3DS build:

- Old 3DS launch flags, 80 MB application mode, normal CPU/cache settings, and no extra-core requirement.
- 20 MB graphics heap and 6.5 MB system heap.
- Compact 25-chunk table, render distance 1, resident radius 2, and map update radius 24.
- Input fix so the missing C-stick/accessory no longer blocks buttons.
- A patch-time control choice: L + Circle Pad camera with B Place/Use and L + Y Drop, or Circle Pad Pro with the original bindings.
- Fancy graphics, transparent leaves, and fancy skies forced off.
- Pickup animations tick and expire with fancy graphics off, fixing items that remained visible after entering the inventory.
- The game's existing FPS/memory overlay enabled.

The patcher finds 59 code sites by signature and refuses missing or ambiguous matches. It also checks the complete pickup animation tick/deletion loop, its connection to the particle update call, and the stack frame used by the trampoline. Memory-layout checks and output verification remain enabled. The game assets are unchanged.

Patch definitions are in `src/SignatureData.h`, patching logic is in `src/Patches.cpp`, and the authored camera, input, accessory, and pickup assembly is in `patches/`.

## Pickup correction in 0.1.1

Disabling fancy graphics also skips the particle engine's update. Picking up an item still transfers it to inventory, but its separate visual animation never ages or gets deleted. The correction sends the low-graphics branch through the game's existing pickup animation loop. Ordinary particles and weather retain their low-graphics behavior; the full-graphics branch retains its original update.

For the supported executable, the hook is at `0x25e578` and uses 24 bytes of text padding at `0x680398`. It enters the pickup loop at `0x232a18` and returns to `0x25e594`. The patcher derives these addresses from signatures and the executable layout. It adds no persistent allocation or memory pages. Source: [ItemPickup.s](patches/ItemPickup.s).

The corrected executable passed 348 ARM execution cases covering animation aging, deletion, both graphics branches, and register/stack preservation. Azahar checks covered repeated drop/pickup cycles, mining, and Save & Quit. See [compatibility](COMPATIBILITY.md) for hardware testing limits.

## Circle Pad Pro mode in 0.2.0

`--controls circle-pad-pro` retains the original camera processing and button bindings. It uses the game's existing `ir:USER` accessory support, including packet decoding and calibration. It does not install the L + Circle Pad camera hook or the B / L + Y remapping.

The original input routine connects to the accessory synchronously and can block ordinary controls when the accessory is absent. CPP mode marks the normal controller available and moves accessory connection attempts to a separate worker. The worker checks state every 100 ms, stops a failed stream before reconnecting, and waits 2.5 seconds between attempts. It uses the game's SDK thread entry to initialize and clean up thread-local state. Shutdown requests worker exit and joins the thread before releasing accessory resources.

For the supported executable, the worker uses 332 bytes of text padding at `0x6803b0`, 64 bytes of state at `0x8cb290`, and a 4 KB stack. This adds one mapped data page. The connection hook is at `0x10b608`, and the cleanup hook is at `0x10bc40`. Source: [CirclePadPro.s](patches/CirclePadPro.s), with its checked machine-code payload in [CirclePadProData.h](src/CirclePadProData.h).

These SDK entry points and storage addresses are specific to the verified executable. CPP mode requires its full SHA-256, including when `--allow-similar` is supplied. L + Circle Pad mode keeps its existing signature matching and produces the same executable, extended header, and icon as 0.1.1.
