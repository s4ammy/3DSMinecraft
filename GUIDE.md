# Setup guide

This guide is for the European base game v0.1.0 and update v9.11.0. The patcher reads your original CIA and makes a Luma `code.ips` file. It never changes the input CIA.

## Before you start

You need a console with CFW, your own original Minecraft CIA, and a compatible Old 3DS bootstrap base installed. If you use the update, you also need the matching bootstrap update installed. Back up your saves before installing or changing titles.

**First-time setup has a gap:** this version of the patcher does not make bootstrap CIAs. A stock Minecraft CIA is New 3DS-only, and `code.ips` cannot change the installed title's ExHeader or icon. If you only have stock titles, you cannot finish Old 3DS setup with this version alone. An older patched CIA is not automatically compatible; in particular, a v0.3.0-only installation needs its bootstrap metadata checked before using the current IPS. See [compatibility](COMPATIBILITY.md).

## Install the bootstrap CIAs once

If you already have compatible bootstrap CIAs but have not installed them:

1. Back up your Minecraft saves and copy the CIAs to a `cias` folder on the SD card.
2. Open FBI on the console. Under **SD > cias**, select the bootstrap base CIA and choose **Install CIA**.
3. If you use the update, install its matching bootstrap CIA the same way.

Keep these titles installed. Do not uninstall them when you change patches, because uninstalling can remove save data. You only need to do this CIA installation once.

## Get the patcher

Download the Windows `.zip` or Linux `.tar.gz` from [Releases](https://github.com/s4ammy/3DSMinecraft/releases) and extract it. Keep the `tools` folder beside the patcher. Packages are for x64 Windows 10/11 and Ubuntu 24.04 or newer. Linux needs OpenSSL 3. Allow about 4 GB of temporary PC space.

## Choose your input CIA

Use the original update CIA if the bootstrap update is installed. If there is no update installed, use the original base CIA. Do not use a patched CIA, DLC, or the base CIA while an update is installed. Both versions place `code.ips` under the base title ID, `000400000017CA00`.

The reference update needs no separate seed. An encrypted European base CIA needs its raw 16-byte title seed or a matching `seeddb.bin`. You can [download the base-game seed from Nintendo](https://kagiya-ctr.cdn.nintendo.net/title/0x000400000017ca00/ext_key?country=GB) and save the binary file as `title-seed.bin`. Check that the file is exactly 16 bytes. Use `--seed-file "title-seed.bin"` for that file or `--seeddb "seeddb.bin"` for a database. An already-decrypted CIA needs neither. Renaming a raw seed to `seeddb.bin` does not turn it into a database.

## Make the patch

Open a terminal in the extracted patcher folder. For an installed update, run:

```sh
./mc3ds-patcher "Minecraft-update.cia" -o "minecraft-luma"
```

On Windows PowerShell, run:

```powershell
.\mc3ds-patcher.exe "Minecraft-update.cia" -o "minecraft-luma"
```

For the base game without an update, replace the CIA name with `Minecraft.cia` and add `--seed-file "title-seed.bin"` if it is encrypted. Use your actual file paths if they differ from these examples.

The default controls use L + Circle Pad to look, B to place or use, and L + Y to drop. Add `--controls circle-pad-pro` if you use a Circle Pad Pro. Add `--overlay` to show the FPS/debug overlay on the update; it is off by default and is unavailable for the base game. Add `--dry-run` to check the CIA and patch profile without writing output. See [all controls](INPUTS_EXPLAINED.md) and [overlay details](DEBUG_OVERLAY.md).

The output folder contains `code.ips` and `report.txt`. Running the patcher again with the same output folder replaces those two files.

## Put it on the SD card

1. Boot while holding SELECT and turn on **Enable game patching** in Luma configuration.
2. Fully close Minecraft.
3. Copy `code.ips` to `/luma/titles/000400000017CA00/` on the SD card.
4. Leave the `romfs` folder and any shader replacements in place. Remove stale `code.bin`, `exheader.bin`, or `code.bps` overrides from the same title folder.
5. Reinsert the SD card and launch Minecraft.

Luma reads `code.ips` when the game starts. For later changes, replace that file, fully close the game, and launch it again. You do not need to reinstall the bootstrap CIAs with FBI. To turn off the replacement temporarily, rename or move `code.ips` out of the title folder.

If the patcher reports a seed error, check the seed format and title. For signature or integrity errors, start with a complete original CIA and compare the [reference hashes](COMPATIBILITY.md). If the game uses the wrong controls, check which update is installed. Run `mc3ds-patcher --help` for all options.

## Build from source

You need CMake 3.22+, a C++17 compiler, and OpenSSL development files on Linux. `MC3DS_FETCH_TOOLS=ON` downloads and verifies the pinned `ctrtool` utility.

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

For a local package, run `python scripts/packageRelease.py --platform linux --binary-dir build --version v0.4.12`. On Windows, use `--platform windows --binary-dir build/Release`. Output goes to `dist`.
