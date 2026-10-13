# Minecraft Old 3DS Patcher

[Discord Server](https://discord.gg/6FCyzkDvX3)

This patcher makes a Luma `code.ips` file for Minecraft: New Nintendo 3DS Edition. It adds Old 3DS controls and memory changes, with an optional FPS overlay for the update.

**Start with the [setup guide](GUIDE.md).** It covers the one-time console setup, the Python GUI or PC commands, and copying the patch to your SD card.

You need your own original European Minecraft base CIA and a console with CFW. For first-time setup, add `--bootstrap-cia` to make the base CIA, and make the matching update CIA if you use the update. Install those CIAs once with FBI. No game files, keys, or seeds are included.

Prefer a window? Run `mc3ds-gui.py` with Python and PySide6. The GUI walks through both the first-time CIA setup and later IPS updates. See the [GUI steps](GUIDE.md#use-the-gui) to get it running.

If your compatible bootstrap update is installed, the usual command is:

```sh
./mc3ds-patcher "Minecraft-update.cia" -o "minecraft-luma"
```

Copy the resulting `code.ips` to `/luma/titles/000400000017CA00/` with Luma game patching enabled. Keep the bootstrap titles installed. Later patch changes only need a new `code.ips`, not another FBI installation.

This build uses one update IPS for all input modes, with a separate optional-overlay variant. Select the mode and edit its button bindings in **Options > Controls**. Base-only installations still have two patch-time control choices. See the [setup guide](GUIDE.md#make-the-luma-patch).

The patcher supports the European base game v0.1.0 and update v9.11.0. Use the original update CIA when the bootstrap update is installed, or the original base CIA when no update is installed. See the guide for Windows commands, the base-game seed, controls, and the optional overlay.

The current controls revision adds persistent per-mode gameplay/menu button bindings, button chords, unbinding, and reset-to-defaults to the native update settings page. It preserves the existing performance patches and bootstrap layout. The base-only settings UI and physical Circle Pad Pro validation are not included in this revision. See [controls](INPUTS_EXPLAINED.md).

This revision is experimental. Functional tests passed, but an unresolved graphics-memory allocation crash occurred during one in-world Options transition. See [validation and limits](PATCHES.md#in-game-controls-v0416).

For more detail, see [compatibility and tested hashes](COMPATIBILITY.md), [controls](INPUTS_EXPLAINED.md), [included patches](PATCHES.md), [performance results](PERFORMANCE.md), and [overlay details](DEBUG_OVERLAY.md). Source builds and release packaging are covered in the [guide](GUIDE.md#build-from-source).

The patcher is [MIT licensed](LICENSE). See [dependencies and notices](THIRD_PARTY.md). This project is not affiliated with Mojang, Microsoft, or Nintendo.
