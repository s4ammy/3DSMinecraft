# Minecraft Old 3DS Patcher

Patches Minecraft: New Nintendo 3DS Edition to run on the original 3DS, with lower memory use and controls that do not need a C-stick.

Version 0.1.1 fixes dropped items remaining visible after pickup with the low-graphics preset. Rebuild the patcher and patch your original CIA again to get the correction. Already-patched CIAs are not accepted.

You need your own CIA and a console running CFW. No game files, keys, or seeds are included. This guide builds the patcher, downloads the title seed separately, and creates a patched CIA.

## Before you start

- Use a 64-bit x86 Windows or Linux PC with CMake 3.22 or newer and a C++17 compiler.
- Allow about 3 GB of free space on the PC's temporary drive plus 500 MB for the output, in addition to the original CIA and build tools.
- Start with an original base-game CIA. Updates, DLC, and already-patched executables are not supported.
- The console needs CFW with signature patches. The output will not work on stock firmware.

Only this input has been verified:

| Property | Supported reference |
| --- | --- |
| Game | Minecraft: New Nintendo 3DS Edition |
| Region | Europe |
| Version | v0.1.0, CIA version 16 |
| Product code | `KTR-P-BD3P` |
| Title ID | `000400000017ca00` |

See [compatibility and hashes](COMPATIBILITY.md) for the reference CIA's SHA-256 and testing limitations. Other versions and regions are unverified, and the seed URL below is specifically for this European title.

## 1. Get the source

On this repository's GitHub page, choose **Code > Download ZIP** and extract it. Alternatively, if Git is installed:

```sh
git clone https://github.com/s4ammy/3DSMinecraft.git
cd 3DSMinecraft
```

Run all remaining PC commands from the extracted or cloned folder containing `CMakeLists.txt`. Follow the commands for your operating system only.

## 2. Build the patcher

### Linux

On Debian/Ubuntu, install the prerequisites:

```sh
sudo apt update
sudo apt install build-essential cmake ninja-build libssl-dev ca-certificates curl
```

Build the patcher:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DMC3DS_FETCH_TOOLS=ON
cmake --build build
```

The executable is `build/mc3ds-patcher`.

### Windows

Install Visual Studio 2022 Build Tools with the **Desktop development with C++** workload, including the Windows SDK and CMake tools. Open **Developer PowerShell for VS 2022**, then change to the source folder:

```powershell
cd "C:\path\to\3DSMinecraft"
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DMC3DS_FETCH_TOOLS=ON
cmake --build build --config Release
```

Replace the example folder with your actual path. The executable is `build\Release\mc3ds-patcher.exe`. The download commands below use `curl.exe`, available on current Windows 10 and Windows 11 installations, rather than PowerShell's `curl` alias.

### Required CIA tools

`MC3DS_FETCH_TOOLS=ON` downloads the pinned `ctrtool` and `makerom` utilities into a `tools` folder beside the executable and checks their archive hashes. It does not download Minecraft or its seed. Keep that folder with the executable if you move it.

MinGW-w64 is also supported. To supply the utilities yourself, leave `MC3DS_FETCH_TOOLS` off and add `--tools-dir "folder"` to the patching commands. See [dependencies](THIRD_PARTY.md).

## 3. Prepare your CIA and title seed

Copy your original CIA into the source folder and name the copy `Minecraft.cia`, or substitute its actual quoted path in the commands below. The patcher leaves its input untouched.

The encrypted reference CIA requires a raw 16-byte title seed. Download it separately from Nintendo using this exact URL:

```text
https://kagiya-ctr.cdn.nintendo.net/title/0x000400000017ca00/ext_key?country=GB
```

You can also [open the seed download](https://kagiya-ctr.cdn.nintendo.net/title/0x000400000017ca00/ext_key?country=GB) in a browser and save the downloaded file as `title-seed.bin`. It is binary data: save the file itself without opening it in a text editor or copying the displayed characters.

### Linux download

```sh
curl --fail --location --output "title-seed.bin" "https://kagiya-ctr.cdn.nintendo.net/title/0x000400000017ca00/ext_key?country=GB" &&
wc -c < "title-seed.bin"
```

### Windows PowerShell download

```powershell
curl.exe --fail --location --output "title-seed.bin" "https://kagiya-ctr.cdn.nintendo.net/title/0x000400000017ca00/ext_key?country=GB"
if ($LASTEXITCODE -ne 0) {
    throw "Seed download failed. See the troubleshooting section."
}

(Get-Item ".\title-seed.bin").Length
```

The size check must print `16`. A different size, an HTML error page, or a failed download is not a usable seed. If you get a certificate error, follow [the troubleshooting instructions](#seed-download-certificate-error). Nintendo controls this endpoint, so its continued availability is not guaranteed.

### If you already have a seed or decrypted CIA

- **Raw seed:** use `--seed-file "path/to/title-seed.bin"`. The patcher does not automatically discover raw seed files.
- **Seed database:** replace `--seed-file "title-seed.bin"` in the commands below with `--seeddb "path/to/seeddb.bin"`. The database must contain this title's entry. You can also put `seeddb.bin` beside the input CIA or executable and omit the seed option.
- **Already-decrypted CIA:** skip the download and omit `--seed-file "title-seed.bin"` from the commands below. The original executable still needs to pass the compatibility checks.

Use either `--seed-file` or `--seeddb`, not both. Renaming a raw seed to `seeddb.bin` does not turn it into a database.

## 4. Check and patch the CIA

First, run an optional dry run. This decrypts the input, verifies its integrity, and checks the patch profile without writing an output CIA.

### Linux

```sh
./build/mc3ds-patcher "Minecraft.cia" --seed-file "title-seed.bin" --dry-run
```

### Windows PowerShell

```powershell
.\build\Release\mc3ds-patcher.exe "Minecraft.cia" --seed-file "title-seed.bin" --dry-run
```

If it finishes with `Dry run passed. No output CIA was written.`, build the patched CIA:

### Linux

```sh
./build/mc3ds-patcher "Minecraft.cia" --seed-file "title-seed.bin" -o "Minecraft-old3ds.cia"
```

### Windows PowerShell

```powershell
.\build\Release\mc3ds-patcher.exe "Minecraft.cia" --seed-file "title-seed.bin" -o "Minecraft-old3ds.cia"
```

The patcher decrypts, decompresses, patches, rebuilds, and verifies the CIA automatically. Wait for the final `Done:` message. With these commands, the source folder will contain:

- `Minecraft-old3ds.cia`: the patched, unencrypted CIA for installation on CFW.
- `Minecraft-old3ds.cia.report.txt`: hashes, title information, and patch verification results.

It refuses to overwrite either an existing output CIA or its report. For another run, choose a new output name such as `-o "Minecraft-old3ds-2.cia"`.

## 5. Install and play

1. Back up any existing Minecraft saves before installation. The patched CIA uses the same title ID and replaces the installed base game.
2. Make sure no Minecraft update is installed. Updates can override the patched code and are not supported. If removing an update, check that you selected the update entry, not the base game or its save data.
3. Power off the console and copy `Minecraft-old3ds.cia` to a folder such as `cias` on its SD card. Leave enough free SD space for both the CIA file and the installed game. The seed and patch report stay on your PC.
4. Reinsert the card, start the console, and open [FBI](https://github.com/Steveice10/FBI). Browse **SD > cias**, select `Minecraft-old3ds.cia`, choose **Install CIA**, and confirm. Wait for installation to finish.
5. Return to the HOME Menu and launch Minecraft. Do not install a game update over the patched build.

Hold L + Circle Pad to look around, use B to place/use, and L + Y to drop. See [all controls](INPUTS_EXPLAINED.md) and [included patches](PATCHES.md). The electronic manual and banner are omitted.

The preceding build has been reported working on an Old 3DS. The pickup correction has passed Azahar testing and still needs confirmation on hardware. Performance varies by world; a steady minimum of 20 FPS has not been established. See [compatibility](COMPATIBILITY.md) for details.

## Troubleshooting

### Seed download certificate error

The Nintendo endpoint produced `curl: (60) ... unable to get local issuer certificate` during development and when this guide was checked. A browser may also reject its certificate.

Prefer a seed or seed database you already have, or configure curl with a trusted certificate for the endpoint using `--cacert`. See [curl's certificate documentation](https://curl.se/docs/sslcerts.html).

If you accept an unverified connection for this seed download, repeat the download command with `--insecure` immediately after `curl` on Linux or `curl.exe` on Windows. This was the workaround used during development. It disables server certificate verification for that request, so curl cannot authenticate the server. Do not make it a global setting. Repeat the 16-byte size check before patching; size alone does not establish authenticity, and the patcher also checks whether the seed matches the title.

### Other errors

| Message or symptom | What to do |
| --- | --- |
| Seed download returns an HTTP error or times out | Retry later or use an existing local seed/database. Do not use the downloaded error page as a seed. |
| `This encrypted CIA needs its title seed` | Supply `--seed-file` or `--seeddb`, or put a matching `seeddb.bin` beside the CIA or executable. |
| `A raw title seed must be exactly 16 bytes` | Download the binary file again and check its size. A database belongs in `--seeddb`, not `--seed-file`. |
| `The supplied seed does not match this title` | Check the CIA's region and title ID. The URL in this guide is for `000400000017ca00`. |
| `Missing ctrtool` or `Missing makerom` | Keep the downloaded `tools` folder beside the executable, or pass `--tools-dir` pointing to both utilities. |
| `Output or report already exists` | Choose a new output filename with `-o`. The patcher never overwrites existing files. |
| Integrity, decoding, or patch-profile error | Check that the input is a complete original base-game CIA and that the seed matches. Compare it with the [verified input](COMPATIBILITY.md). |
| Game still requires New 3DS hardware or patched controls are absent | Check that you installed the patched output and that no game update is overriding it. |
| Dropped items enter the inventory but remain visible on the ground | Rebuild patcher version 0.1.1 or later, patch an original CIA, and install the new output. The correction keeps pickup animations updating with low graphics enabled. |

For other builds, `--allow-similar` attempts signature matching with strict checks, but compatibility is not guaranteed. It does not make the European seed valid for another title. Run the patcher with `--help` to list all options.

## Licence

The patcher is [MIT licensed](LICENSE). Minecraft and external dependencies keep their own licences. This project is not affiliated with Mojang, Microsoft, or Nintendo. Do not upload game files, seeds, or keys in bug reports.
