# 3DSMinecraft beginner guide

This guide gets the European Minecraft update running with the latest 3DSMinecraft patch. You do not need to build the project from source.

If the patch already works on your console and you only want the new version, skip to [updating the patch](#updating-the-patch).

## What you need

- A 3DS with custom firmware, Luma3DS and FBI already installed. This guide does not install custom firmware.
- Your own original European Minecraft base CIA, v0.1.0, and update CIA, v9.11.0. Other regions are not supported by this release IPS.
- A Windows or Linux computer, an SD-card reader and a backup of your Minecraft saves.
- The files from [3DSMinecraft Releases](https://github.com/s4ammy/3DSMinecraft/releases).

There are two parts to setup. The **bootstrap CIAs** are compatible game installations that you make from your own files and install once. **`code.ips`** is the small patch you replace when a new build comes out. Downloading the IPS alone does not replace first-time bootstrap setup.

## 1. Download the PC tools

Open [Releases](https://github.com/s4ammy/3DSMinecraft/releases) and choose the latest release.

On Windows, download the file ending in `windows-x64.zip`. On Linux, download the file ending in `linux-x64.tar.gz`. Extract the whole archive into a new folder. Keep the included `tools` folder beside the patcher.

Also download the separate **`code.ips`** asset. Use that downloaded IPS for the latest in-game menu after completing bootstrap setup. The PC tools generate an earlier controls interface, so their generated update IPS is not the latest release patch.

## 2. Make the bootstrap files

The optional GUI provides a **First-time setup** page. It needs Python 3.11 or newer and PySide6. Open a terminal in the extracted folder and run the commands for your computer.

Windows PowerShell:

```powershell
py -m venv .gui-venv
.\.gui-venv\Scripts\python.exe -m pip install -r gui-requirements.txt
.\.gui-venv\Scripts\python.exe mc3ds-gui.py
```

Linux:

```sh
python3 -m venv .gui-venv
.gui-venv/bin/python -m pip install -r gui-requirements.txt
.gui-venv/bin/python mc3ds-gui.py
```

In the GUI:

1. Open **First-time setup**.
2. Choose your original European base CIA.
3. Choose your original European update CIA too. The downloadable release IPS requires this update.
4. If the base CIA needs a seed, choose the matching seed file or seed database. The supported update needs no separate seed. See [choosing your input CIA](GUIDE.md#choose-your-input-cia) for the base seed and its required format.
5. Choose a new output folder and click **Build bootstrap CIAs**.
6. Wait for it to finish successfully. The output has `bootstrap-base` and `bootstrap-update` folders, each containing a `bootstrap.cia`.

Keep your original CIAs unchanged. If you prefer commands instead of the GUI, use the [terminal setup instructions](GUIDE.md#make-the-bootstrap-cias-from-a-terminal).

## 3. Install the bootstraps once

Back up your saves before changing the installed game. If compatible bootstrap titles are already installed, skip this step.

1. Turn the console off and put its SD card into your computer.
2. Create a `cias` folder on the SD card if needed.
3. Copy the two generated `bootstrap.cia` files into it. Rename the copies to `bootstrap-base.cia` and `bootstrap-update.cia` so you can tell them apart.
4. Put the SD card back into the console and open FBI.
5. Go to **SD > cias** and install `bootstrap-base.cia` first.
6. Install `bootstrap-update.cia` afterwards.

Keep the installed bootstrap titles. Do not uninstall Minecraft when updating the patch, because uninstalling can remove save data. An old v0.3.0-only installation may need fresh compatible bootstraps; see [compatibility](COMPATIBILITY.md).

## 4. Put the release patch on the SD card

1. Fully close Minecraft, turn the console off and put the SD card into your computer.
2. Open the SD card and create this folder path if it does not exist:

   ```text
   luma/titles/000400000017CA00/
   ```

3. Copy the **downloaded release `code.ips`** into that folder. Its name must remain `code.ips`, with no extra `.ips` extension.
4. Keep any existing `romfs` folder and shader replacements. If the title folder contains old `code.bin`, `exheader.bin` or `code.bps` overrides, move them into a backup folder outside the title folder.
5. Put the SD card back into the console.
6. Hold **SELECT** while powering on to open Luma configuration. Turn on **Enable game patching**, then save and continue booting.
7. Launch Minecraft.

The final location is `SD card/luma/titles/000400000017CA00/code.ips`. The title-folder number is the same for the supported base game and update.

## 5. Choose your controls

Open **Mod Settings** from the title menu or pause menu. You can use the touchscreen or the physical buttons. Inside the settings editor, **A** selects and **B** goes back.

The **Mode** dropdown offers L + Circle Pad, Circle Pad Pro, Touch stick and ABXY camera. A new configuration starts with L + Circle Pad: hold L and move the Circle Pad to look. Touch stick adds a camera stick on the bottom screen. ABXY camera uses the face buttons for looking. Circle Pad Pro is for the physical accessory.

Use the **Page** dropdown to choose gameplay bindings, menu bindings, camera, touch-stick, display, binding tools or overlay settings.

To change a binding, select its action, release the button that opened capture, press your desired button or combination, then release it to finish. Shared buttons are allowed, so check for actions that would activate together. Binding tools can clear the selected binding, copy a mode's bindings or reset a mode to its defaults. Clearing a binding disables that action. The settings editor retains its recovery controls so you can edit custom menu bindings.

Leave the settings menu normally to save changes. Modes remember their own bindings and camera/touch tuning.

## 6. Adjust visuals and performance

Start with the defaults on an Old 3DS. The **Display** page includes visual presets, particles, weather, map refresh, low-memory textures and chunk settings. Higher distances use more memory; fewer mesh builders can save scratch memory but delay mesh rebuilding.

Changing **low-memory textures**, **render distance**, **nearby chunk radius**, **map radius** or **mesh builders** requires saving, fully closing Minecraft and launching it again. Returning to the title screen alone does not apply them. The menu shows a restart-pending message when needed.

Enable **FPS** or **Full** on Display if you want the overlay. The separate **Overlay** page changes its position, size, background opacity and frame graph. See [all setting ranges](GUIDE.md#experimental-chunk-settings) and [control details](INPUTS_EXPLAINED.md).

## Updating the patch

With compatible bootstraps installed, later updates are simple:

1. Download the new release's `code.ips`.
2. Fully close Minecraft and turn the console off before removing the SD card.
3. Back up the old IPS, then replace `luma/titles/000400000017CA00/code.ips` with the new file.
4. Reinsert the SD card and launch Minecraft.

You do not need to reinstall the game or make new bootstrap CIAs for a normal IPS update. One IPS includes every control mode and the settings menu. Do not stack old control-mode patches on it.

The current settings format is `MC3DS4`. Older patches cannot read it, so keep a settings backup if you plan to roll back.

## If something does not work

If **Mod Settings** is missing, check that you installed the supported European update, copied the downloaded release IPS rather than the PC-generated baseline, used the exact folder/name, enabled game patching and fully restarted Minecraft.

If the GUI cannot build the bootstraps, check the original CIA files and base seed against the [detailed setup guide](GUIDE.md). Keep `ctrtool` and `makerom` in the included `tools` folder.

If the game crashes, keep the Luma dump from `luma/dumps/arm11/` on the SD card. When reporting it, include the patch version, console model, game region/update and what you were doing. See [known crash investigations](docs/crashes/README.md) or ask in the [Discord server](https://discord.gg/6FCyzkDvX3).
