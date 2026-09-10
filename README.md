# Minecraft Old 3DS Patcher

Patches Minecraft: New Nintendo 3DS Edition for the original 3DS, with lower memory use and a choice of L + Circle Pad or Circle Pad Pro controls.

**0.3.0 adds the European v9.11.0 update**, shown in-game as version 1.9. The default controls have been reported working on an Old 3DS. Base-game patches are unchanged; physical Circle Pad Pro testing is still pending.

You need your own original CIA and a console running CFW. No game files, keys, or seeds are included.

## Supported versions

| European input | Title ID | Installation |
| --- | --- | --- |
| Base game v0.1.0 | `000400000017ca00` | Patch and install the base game. |
| Update v9.11.0 / game 1.9 | `0004000e0017ca00` | Patch separately and install alongside the patched base. |

The patcher detects which profile to use. DLC and already-patched CIAs are not accepted. See [compatibility and hashes](COMPATIBILITY.md) for other-build limitations.

## Get the patcher

Download the Windows `.zip` or Linux `.tar.gz` from [Releases](https://github.com/s4ammy/3DSMinecraft/releases), extract it, and open a terminal in that folder. Keep `tools` beside the executable.

Packages are for x64 Windows 10/11 and Ubuntu 24.04 or newer. Linux needs OpenSSL 3. Allow about 4 GB of temporary space and 600 MB per output CIA.

## Prepare the input

Use copies named `Minecraft.cia` and, if wanted, `Minecraft-update.cia`. The patcher leaves them untouched.

The reference update needs no separate seed. The encrypted European base CIA needs its raw 16-byte title seed or a matching `seeddb.bin`. You can [download the base-game seed from Nintendo](https://kagiya-ctr.cdn.nintendo.net/title/0x000400000017ca00/ext_key?country=GB) and save the binary file as `title-seed.bin`. Check that its size is exactly 16 bytes. If the download fails, use a seed or seed database you already have.

Use `--seed-file "title-seed.bin"` for a raw seed, or `--seeddb "seeddb.bin"` for a database. Omit both for an already-decrypted input. A raw seed cannot be renamed into a seed database.

## Patch

| Mode | Controls |
| --- | --- |
| `--controls l-circle-pad` (default) | L + Circle Pad to look, B to place/use, L + Y to drop. |
| `--controls circle-pad-pro` | Accessory right pad to look, L to place/use, B to drop. |

Choose at patch time. To change modes, patch the original CIA again. See [all controls](INPUTS_EXPLAINED.md).

On Linux:

```sh
./mc3ds-patcher "Minecraft.cia" --seed-file "title-seed.bin" -o "Minecraft-old3ds.cia"
./mc3ds-patcher "Minecraft-update.cia" -o "Minecraft-update-old3ds.cia"
```

On Windows PowerShell:

```powershell
.\mc3ds-patcher.exe "Minecraft.cia" --seed-file "title-seed.bin" -o "Minecraft-old3ds.cia"
.\mc3ds-patcher.exe "Minecraft-update.cia" -o "Minecraft-update-old3ds.cia"
```

Run the second command only if you want the update. Add `--controls circle-pad-pro` to select CPP mode. Add `--dry-run` to check an input without building a CIA.

Wait for `Done:`. Each output has a `.report.txt` containing its hashes and verification results. Existing outputs are never overwritten; choose a different `-o` name for another run. Use `--tools-dir "folder"` if your helper utilities are elsewhere.

## Install

1. Back up your saves. Worlds saved with the update may need it to load again.
2. Copy the patched CIA files into `cias` on your SD card.
3. Use FBI to install the patched base, then the patched update if wanted.
4. Launch Minecraft. Keep the patched update installed; an unpatched update overrides the base game's patches.

If you only want the base game, it must have no update installed. The output needs CFW with signature patches; it will not launch on stock firmware. The base game's manual and banner are omitted.

## Build from source

Requires CMake 3.22+, a C++17 compiler, and OpenSSL development files on Linux. `MC3DS_FETCH_TOOLS=ON` downloads and verifies the pinned `ctrtool` and `makerom` utilities.

Linux, from the source folder:

```sh
sudo apt install build-essential cmake ninja-build libssl-dev ca-certificates
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DMC3DS_FETCH_TOOLS=ON
cmake --build build
```

Windows, from Developer PowerShell for Visual Studio 2022 with the C++ workload installed:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DMC3DS_FETCH_TOOLS=ON
cmake --build build --config Release
```

The executable is in `build` on Linux or `build\Release` on Windows. MinGW-w64 also works.

## Releases

Commit and push to `main` to build both platforms and publish the version from `CMakeLists.txt` to [Releases](https://github.com/s4ammy/3DSMinecraft/releases). The next version is **v0.3.0**. No separate tag push is needed.

Published releases are left alone. Bump the CMake version before the next release. Explicit matching `v*` tags still work, including prereleases such as `v0.3.0-rc1`. Other branches, pull requests, and manual runs produce build artifacts only.

The workflow publishes after both builds, package startup checks, and checksum checks pass. Packages contain the patcher, helper tools, docs, and required source/licence notices. Development tests and game files are not included.

For a local package, run `python scripts/packageRelease.py --platform linux --binary-dir build --version v0.3.0`. On Windows use `--platform windows --binary-dir build/Release`. Output goes to `dist`.

## Help

For seed errors, check the input title and seed format. For signature or integrity errors, start with a complete original CIA and compare the [reference hashes](COMPATIBILITY.md). If the game shows the wrong controls, check which update is installed. Run the patcher with `--help` for all options.

The patcher is [MIT licensed](LICENSE). See [dependencies and notices](THIRD_PARTY.md). This project is not affiliated with Mojang, Microsoft, or Nintendo.
