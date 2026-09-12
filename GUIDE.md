# Setup guide

This guide is for the European base game v0.1.0 and update v9.11.0. The patcher reads your original CIAs and makes Luma `code.ips` files. For first-time setup, it can also make the bootstrap CIAs that you install once. It never changes your input files.

## Before you start

You need a console with CFW and your own original Minecraft base CIA. If you want the update, you need its original CIA too. Back up your saves before installing or changing titles. The bootstrap CIAs use the same title IDs as the game and require CFW with signature patches.

## Get the patcher

Download the Windows `.zip` or Linux `.tar.gz` from [Releases](https://github.com/s4ammy/3DSMinecraft/releases) and extract it. Keep the `tools` folder, containing `ctrtool` and `makerom`, beside the patcher. Packages are for x64 Windows 10/11 and Ubuntu 24.04 or newer. Linux needs OpenSSL 3. Allow about 4 GB of temporary PC space and 600 MB for each bootstrap CIA you make.

## Use the GUI

The optional Python GUI gives you a **First-time setup** page and a **Make a Luma patch** page. It runs the same patcher as the commands below and shows its output in the window. You need Python 3.11 or newer and PySide6. In the extracted folder, set up a small Python environment once:

On Linux:

```sh
python3 -m venv .gui-venv
.gui-venv/bin/python -m pip install -r gui-requirements.txt
.gui-venv/bin/python mc3ds-gui.py
```

On Windows PowerShell:

```powershell
py -m venv .gui-venv
.\.gui-venv\Scripts\python.exe -m pip install -r gui-requirements.txt
.\.gui-venv\Scripts\python.exe mc3ds-gui.py
```

For first-time setup, choose your original base CIA, its [seed](#choose-your-input-cia) if needed, and your original update CIA if you use the update. Choose an output folder and click **Build bootstrap CIAs**. The GUI makes the base first, then the update. Follow [the installation steps below](#install-the-bootstrap-cias) after it finishes. If the bootstrap titles are already installed, use **Make a Luma patch** to choose the matching original CIA and generate only `code.ips`.

The rest of this guide gives the equivalent terminal commands and explains the files the GUI makes.

## Choose your input CIA

Start with your original base CIA, and use the original update CIA if you want game version 1.9. Do not give the patcher an already-patched CIA or DLC. Both versions place `code.ips` under the base title ID, `000400000017CA00`.

The reference update needs no separate seed. An encrypted European base CIA needs its raw 16-byte title seed or a matching `seeddb.bin`. You can [download the base-game seed from Nintendo](https://kagiya-ctr.cdn.nintendo.net/title/0x000400000017ca00/ext_key?country=GB) and save the binary file as `title-seed.bin`. Check that the file is exactly 16 bytes. If the download fails, use a seed or seed database you already have. Use `--seed-file "title-seed.bin"` for a raw seed or `--seeddb "seeddb.bin"` for a database. An already-decrypted CIA needs neither. Renaming a raw seed to `seeddb.bin` does not turn it into a database.

## Make the bootstrap CIAs from a terminal

If you have not installed compatible Old 3DS bootstrap titles, open a terminal in the extracted patcher folder. On Linux, make the base CIA first:

```sh
./mc3ds-patcher "Minecraft.cia" --seed-file "title-seed.bin" --bootstrap-cia -o "bootstrap-base"
```

If you want the update, make its CIA too:

```sh
./mc3ds-patcher "Minecraft-update.cia" --bootstrap-cia -o "bootstrap-update"
```

On Windows PowerShell, use the same options with `mc3ds-patcher.exe`:

```powershell
.\mc3ds-patcher.exe "Minecraft.cia" --seed-file "title-seed.bin" --bootstrap-cia -o "bootstrap-base"
.\mc3ds-patcher.exe "Minecraft-update.cia" --bootstrap-cia -o "bootstrap-update"
```

Run the update command only if you want the update. Omit `--seed-file` for a decrypted base CIA, or use `--seeddb` if you have a matching seed database. Add `--controls circle-pad-pro` to both commands if you want that control mode. Use your actual file paths if they differ from these examples.

Each output folder contains `bootstrap.cia`, `code.ips`, and `report.txt`. The CIA has the original executable with the Old 3DS metadata it needs. The Luma IPS adds the gameplay changes, so copy it before launching the game. The patcher verifies the CIA before saving it and will not overwrite an existing `bootstrap.cia`.

## Install the bootstrap CIAs

1. Back up your Minecraft saves and copy `bootstrap-base/bootstrap.cia` to a `cias` folder on the SD card. If you made an update, copy `bootstrap-update/bootstrap.cia` there too.
2. Open FBI on the console. Under **SD > cias**, select the base CIA and choose **Install CIA**.
3. If you made an update, install its CIA the same way, after the base.

Keep these titles installed. Do not uninstall them when you change patches, because uninstalling can remove save data. Luma IPS changes only the executable; the installed CIAs supply the Old 3DS icon and ExHeader settings it cannot replace.

If compatible bootstrap titles are already installed, skip this section. A v0.3.0-only installation may have an incompatible executable layout, so generate and install fresh bootstraps from your original CIAs before using the current IPS. See [compatibility](COMPATIBILITY.md).

## Make the Luma patch

If you just made the bootstrap CIAs, you already have `code.ips`. Use the one from `bootstrap-update` when the update is installed, or `bootstrap-base` when it is not.

For later changes, run the patcher again without `--bootstrap-cia`. With an installed update, use:

```sh
./mc3ds-patcher "Minecraft-update.cia" -o "minecraft-luma"
```

On Windows PowerShell, run:

```powershell
.\mc3ds-patcher.exe "Minecraft-update.cia" -o "minecraft-luma"
```

For the base game without an update, replace the CIA name with `Minecraft.cia` and add `--seed-file "title-seed.bin"` if it is encrypted. Use your actual file paths if they differ from these examples.

The default controls use L + Circle Pad to look, B to place or use, and L + Y to drop. Add `--controls circle-pad-pro` if you use a Circle Pad Pro. Add `--overlay` to show the FPS/debug overlay on the update; it is off by default and is unavailable for the base game. Add `--dry-run` to check the CIA and patch profile without writing output. See [all controls](INPUTS_EXPLAINED.md) and [overlay details](DEBUG_OVERLAY.md).

The output folder contains `code.ips` and `report.txt`. Running the patcher again with the same output folder replaces those two files. It leaves a `bootstrap.cia` there alone.

## Put it on the SD card

1. Boot while holding SELECT and turn on **Enable game patching** in Luma configuration.
2. Fully close Minecraft.
3. Copy `code.ips` to `/luma/titles/000400000017CA00/` on the SD card.
4. Leave the `romfs` folder and any shader replacements in place. Remove stale `code.bin`, `exheader.bin`, or `code.bps` overrides from the same title folder.
5. Reinsert the SD card and launch Minecraft.

Luma reads `code.ips` when the game starts. For later changes, replace that file, fully close the game, and launch it again. You do not need to reinstall the bootstrap CIAs with FBI. To turn off the replacement temporarily, rename or move `code.ips` out of the title folder.

If the patcher reports a seed error, check the seed format and title. For signature or integrity errors, start with a complete original CIA and compare the [reference hashes](COMPATIBILITY.md). If the game uses the wrong controls, check which update is installed. Run `mc3ds-patcher --help` for all options.

## Build from source

You need CMake 3.22+, a C++17 compiler, and OpenSSL development files on Linux. `MC3DS_FETCH_TOOLS=ON` downloads and verifies the pinned `ctrtool` and `makerom` utilities.

On Linux:

```sh
sudo apt install build-essential cmake ninja-build libssl-dev ca-certificates
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DMC3DS_FETCH_TOOLS=ON
cmake --build build
```

On Windows, from Developer PowerShell for Visual Studio 2022 with the C++ workload installed:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DMC3DS_FETCH_TOOLS=ON
cmake --build build --config Release
```

The executable is in `build` on Linux or `build\Release` on Windows. MinGW-w64 also works. Pre-generated ARM payload data is included. Developers changing the ARM/C++ hooks can regenerate it with `python3 scripts/buildPerformancePayloads.py --reference-code /path/to/stock/update/code.bin`, then use `--check` to verify reproducibility. That step needs Clang, LLD, and llvm-objcopy. Review hook offsets and output checksums whenever the linked layout changes.

## Release packaging

Pushing to `main` builds both platforms and publishes the version from `CMakeLists.txt`. Published releases are left alone, so bump that version before the next release. An explicit matching `v*` tag also works, including prereleases. Other branches, pull requests, and manual runs produce build artifacts only.

For a local package, run `python scripts/packageRelease.py --platform linux --binary-dir build --version v0.4.13`. On Windows, use `--platform windows --binary-dir build/Release`. Output goes to `dist`.
