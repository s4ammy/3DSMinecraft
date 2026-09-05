# Minecraft Old 3DS Patcher

Patches Minecraft: New Nintendo 3DS Edition to run on the original 3DS, with lower memory use and controls that do not need a C-stick.

You need your own CIA and a console running CFW. No game files or keys are included. Only the European v0.1.0 build has been verified. See [compatibility and hashes](COMPATIBILITY.md).

## Build

Use CMake 3.22 or newer and a C++17 compiler on a 64-bit Windows or Linux PC. These commands also download the required CIA tools.

### Linux

On Debian/Ubuntu, install `build-essential cmake ninja-build libssl-dev ca-certificates`, then run:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DMC3DS_FETCH_TOOLS=ON
cmake --build build
./build/mc3ds-patcher "Minecraft.cia" -o "Minecraft-old3ds.cia"
```

### Windows

Install Visual Studio 2022 Build Tools with the C++ desktop workload and CMake. From a developer PowerShell:

```powershell
cmake -S . -B build -A x64 -DMC3DS_FETCH_TOOLS=ON
cmake --build build --config Release
.\build\Release\mc3ds-patcher.exe "Minecraft.cia" -o "Minecraft-old3ds.cia"
```

MinGW-w64 is also supported. The automatic tool downloads are for x86-64. To supply tools yourself, leave `MC3DS_FETCH_TOOLS` off and use `--tools-dir "folder"`. See [dependencies](THIRD_PARTY.md).

## Encrypted CIAs

The verified retail CIA needs its title seed. Put your `seeddb.bin` beside the CIA or patcher, or pass `--seeddb "file"`. A raw 16-byte seed can be supplied with `--seed-file "file"`. Decrypted CIAs do not need this. The patcher does not download seeds.

## Using the output

The patcher decrypts, patches, rebuilds, and verifies the CIA automatically. It leaves the original untouched and refuses to overwrite existing files. Allow about 3 GB of temporary space plus 500 MB for the output.

The result requires CFW signature patches. Back up your saves first: it uses the same title ID and replaces the installed game. Installed game updates can override the patched code and are not supported. The electronic manual and banner are omitted.

Hold L + Circle Pad to look around, use B to place/use, and L + Y to drop. See [all controls](INPUTS_EXPLAINED.md) and [included patches](PATCHES.md).

For other builds, `--allow-similar` tries signature matching but keeps all safety checks. Compatibility is not guaranteed. `--dry-run` checks an input without building a CIA; `--help` lists all options.

## Licence

The patcher is [MIT licensed](LICENSE). Minecraft and external dependencies keep their own licences. This project is not affiliated with Mojang, Microsoft, or Nintendo. Do not upload game files, seeds, or keys in bug reports.
