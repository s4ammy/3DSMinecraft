# Minecraft Old 3DS Patcher

[Discord Server](https://discord.gg/6FCyzkDvX3)

This patcher makes a Luma `code.ips` file for Minecraft: New Nintendo 3DS Edition. It adds Old 3DS controls and memory changes, with in-game control, camera and visual settings for the update.

**Start with the [setup guide](GUIDE.md).** It covers the one-time console setup, the Python GUI or PC commands, and copying the patch to your SD card.

You need your own original European Minecraft base CIA and a console with CFW. For first-time setup, add `--bootstrap-cia` to make the base CIA, and make the matching update CIA if you use the update. Install those CIAs once with FBI. No game files, keys, or seeds are included.

Prefer a window? Run `mc3ds-gui.py` with Python and PySide6. The GUI walks through both the first-time CIA setup and later IPS updates. See the [GUI steps](GUIDE.md#use-the-gui) to get it running.

If your compatible European bootstrap update is installed, download the single `code.ips` asset from [Releases](https://github.com/s4ammy/3DSMinecraft/releases). Copy it to `/luma/titles/000400000017CA00/` with Luma game patching enabled. Keep the bootstrap titles installed. Later patch changes only need a new `code.ips`, not another FBI installation.

The release IPS includes all four input modes, including ABXY camera, in the dedicated **Mod Settings** menu. It includes customizable bindings, camera and touch controls, overlay and visual settings, and chunk/render-distance settings. No separate control-mode or overlay IPS is needed. This asset requires update v9.11.0; it does not support base-only installations.

The PC patcher still generates the earlier **Options > Controls** update payload and the two base-only control profiles. Use it for bootstrap CIA creation, then install the downloaded release IPS for the latest native menu and chunk settings. See the [setup guide](GUIDE.md#make-the-luma-patch).

The patcher supports the European base game v0.1.0 and update v9.11.0. Use the original update CIA when the bootstrap update is installed, or the original base CIA when no update is installed. See the guide for Windows commands, the base-game seed, controls, and the optional overlay.

The current controls revision adds persistent per-mode gameplay/menu button bindings, button chords, unbinding, and reset-to-defaults to the native update settings page. It preserves the existing performance patches and bootstrap layout. The base-only settings UI and physical Circle Pad Pro validation are not included in this revision. See [controls](INPUTS_EXPLAINED.md).

Version 0.4.18 reduces chunk-builder scratch reservations by about 2.8 MiB with a smaller deferred builder pool. It remains experimental: this is memory headroom, not a demonstrated fix for every graphics-allocation crash or a proven FPS gain. See [validation and limits](PERFORMANCE.md#v0418-chunk-builder-memory-headroom).

For more detail, see [compatibility and tested hashes](COMPATIBILITY.md), [controls](INPUTS_EXPLAINED.md), [included patches](PATCHES.md), [performance results](PERFORMANCE.md), and [overlay details](DEBUG_OVERLAY.md). Source builds and release packaging are covered in the [guide](GUIDE.md#build-from-source).

## Source layout

The host patcher and the ARM code injected into the game are kept separate:

```text
src/
    app/           CLI entry point and CIA/IPS pipeline
    core/          Binary I/O, platform helpers and signature matching
    patching/      Patch selection, guards and installation
    data/          Embedded payload bytes, signatures and bootstrap data
patches/
    controls/      Input modes, bindings, saved settings and touch stick
    ui/            Native settings controls, prompts and menu hooks
    overlay/       In-game diagnostics and metrics
    rendering/     Visual settings, textures and rendering fast paths
    world/         Chunk, block, lighting and item-pickup patches
    performance/   Shared performance, allocation and worker hooks
    experimental/  Research candidates not enabled by the release
```

Payload builders and ARM regression checks live in `scripts/`; host-side tests live in `tests/`. Regenerate embedded payload headers with their builders rather than editing the byte arrays in `src/data/`. Build instructions and script entry points are unchanged.

The patcher is [MIT licensed](LICENSE). See [dependencies and notices](THIRD_PARTY.md). This project is not affiliated with Mojang, Microsoft, or Nintendo.
