# Minecraft Old 3DS Patcher

Patches Minecraft: New Nintendo 3DS Edition for the original 3DS, with lower memory use and a choice of L + Circle Pad or Circle Pad Pro controls.

Version 0.4.11 produces a Luma `code.ips` patch instead of rebuilding CIAs. Once the compatible Old 3DS bootstrap titles are installed, testing a new build only requires replacing `code.ips`, closing Minecraft, and launching it again.

The update profile includes a custom bottom-screen FPS/debug overlay, cached block/light lookups, reduced climate-layer work, fewer record copies and reusable inflate state. Version 0.4.10 adds an exact fast path for four-byte-aligned heap allocations, retaining v0.4.9's faster free-block search without changing selected addresses or block merging. It retains the v0.4.5 scratch-pointer correction, exact random-bound masks and shortened startup presentations. See [overlay details](DEBUG_OVERLAY.md) and [performance measurements and limits](PERFORMANCE.md).

You need your own original CIA and a console running CFW. No game files, keys, or seeds are included.

## One-time bootstrap requirement

Luma IPS patches only the installed executable. It does not replace the title's ExHeader or SMDH `icon.bin`. The retail Minecraft metadata is New 3DS-only, so an Old 3DS needs the compatible bootstrap base and update CIAs installed once. They also provide the memory layout and executable extent used by these patches.

Keep those bootstrap titles installed. Do not uninstall them, because uninstalling can remove save data. After that one-time installation, executable changes use only `code.ips` and need no further FBI installation.

## Supported versions

| European input | Title ID | Use |
| --- | --- | --- |
| Base game v0.1.0 | `000400000017ca00` | Generate from this only when the bootstrap update is not installed. |
| Update v9.11.0, game 1.9 | `0004000e0017ca00` | Generate from this when the bootstrap update is installed. |

Both outputs are loaded from `/luma/titles/000400000017CA00/`. DLC and already-patched input CIAs are not accepted. See [compatibility and hashes](COMPATIBILITY.md) for other-build limitations.

## Get the patcher

Download the Windows `.zip` or Linux `.tar.gz` from [Releases](https://github.com/s4ammy/3DSMinecraft/releases), extract it, and open a terminal in that folder. Keep the `tools` folder beside the executable.

Packages are for x64 Windows 10/11 and Ubuntu 24.04 or newer. Linux needs OpenSSL 3. Allow about 4 GB of temporary space. The generated IPS is small.

## Prepare the input

Use your original `Minecraft.cia` or `Minecraft-update.cia`. The patcher reads it but never modifies it.

The reference update needs no separate seed. The encrypted European base CIA needs its raw 16-byte title seed or a matching `seeddb.bin`. You can [download the base-game seed from Nintendo](https://kagiya-ctr.cdn.nintendo.net/title/0x000400000017ca00/ext_key?country=GB) and save the binary file as `title-seed.bin`. Check that its size is exactly 16 bytes.

Use `--seed-file "title-seed.bin"` for a raw seed, or `--seeddb "seeddb.bin"` for a database. Omit both for an already-decrypted input. A raw seed cannot be renamed into a seed database.

## Generate the Luma replacement

| Mode | Controls |
| --- | --- |
| `--controls l-circle-pad` (default) | L + Circle Pad to look, B to place/use, L + Y to drop. |
| `--controls circle-pad-pro` | Accessory right pad to look, L to place/use, B to drop. |

Choose at patch time. See [all controls](INPUTS_EXPLAINED.md).

Version 0.4.11 fixes B-use releases closing newly opened containers in the default control profile. Containers close on a fresh B press instead. The base and update have separate guarded patches; CPP retains its original controls. Existing compatible bootstraps and LayeredFS assets are unchanged.

For an installed v9.11.0 update on Linux:

```sh
./mc3ds-patcher "Minecraft-update.cia" -o "minecraft-luma"
```

On Windows PowerShell:

```powershell
.\mc3ds-patcher.exe "Minecraft-update.cia" -o "minecraft-luma"
```

For the base game with no update, use `Minecraft.cia` and add `--seed-file "title-seed.bin"`. Add `--controls circle-pad-pro` to select CPP mode. Add `--dry-run` to validate without writing files.

The output folder contains:

```text
code.ips
report.txt
```

Running the patcher again with the same output folder atomically replaces those two files. Other files and subdirectories are not changed.

## Put it on the SD card

1. Back up your saves.
2. Boot while holding SELECT and enable `Enable game patching` in Luma configuration.
3. Fully close Minecraft.
4. Copy `code.ips` into `/luma/titles/000400000017CA00/` on the SD card.
5. Leave `/luma/titles/000400000017CA00/romfs/` and its shader replacements exactly where they are.
6. Reinsert the SD card and launch Minecraft.

Remove stale `code.bin`, `exheader.bin`, or `code.bps` overrides from the same title folder. `locale.txt` and the `romfs` folder are independent and can remain.

This is restart-based swapping, not live code reloading. Luma reads the patch when the game process starts. For later changes, replace `code.ips`, fully close the game, and relaunch it.

To disable the replacement temporarily, rename or move `code.ips` out of the title folder. This does not alter the installed title.

## Build from source

Requires CMake 3.22+, a C++17 compiler, and OpenSSL development files on Linux. `MC3DS_FETCH_TOOLS=ON` downloads and verifies the pinned `ctrtool` utility.

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

The executable is in `build` on Linux or `build\Release` on Windows. MinGW-w64 also works. Run `ctest --test-dir build --output-on-failure` for payload checks. Add `-DMC3DS_UPDATE_FIXTURE=/path/to/private/update/original` when configuring to test both update profiles and compatibility rejections against your own extracted fixture.

Pre-generated ARM payload data is included. Developers changing the ARM/C++ hooks can regenerate it with `python3 scripts/buildPerformancePayloads.py --reference-code /path/to/stock/update/code.bin`, then use `--check` to verify reproducibility. This requires Clang, LLD and llvm-objcopy. Hook offsets and expected output checksums must be reviewed whenever the linked layout changes.

## Releases

Commit and push to `main` to build both platforms and publish the version from `CMakeLists.txt` to [Releases](https://github.com/s4ammy/3DSMinecraft/releases). The next version is **v0.4.7**. No separate tag push is needed.

Published releases are left alone. Bump the CMake version before the next release. Explicit matching `v*` tags still work, including prereleases such as `v0.4.7-rc1`. Other branches, pull requests, and manual runs produce build artifacts only.

For a local package, run `python scripts/packageRelease.py --platform linux --binary-dir build --version v0.4.7`. On Windows use `--platform windows --binary-dir build/Release`. Output goes to `dist`.

## Help

For seed errors, check the input title and seed format. For signature or integrity errors, start with a complete original CIA and compare the [reference hashes](COMPATIBILITY.md). If the game shows the wrong controls, check which update is installed. Run the patcher with `--help` for all options.

The patcher is [MIT licensed](LICENSE). See [dependencies and notices](THIRD_PARTY.md). This project is not affiliated with Mojang, Microsoft, or Nintendo.
