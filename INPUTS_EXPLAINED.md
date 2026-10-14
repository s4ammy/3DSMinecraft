# Controls

## Native-menu experiment, 2026-10-14

The separate `release/experiments/native-menu-20261014/code.ips` adds **Mod Settings** to the title and pause menus. It uses the game's dropdowns for mode/page and discrete choices, sliders for camera/touch values, and on/off controls for booleans. Bindings still use capture buttons. The original **Options > Controls** page remains available.

This is an experimental update-only replacement IPS, not an additional patch to stack. Back up your installed patch, then replace `/luma/titles/000400000017CA00/code.ips` and fully restart. The existing compatible base installation is unchanged. Restore `release/ips/v0.4.18/code-update-controls.ips` to roll back; the settings format is unchanged. Normal patcher builds and the v0.4.18 release still produce the preserved interface.

The pre-experiment source, worktree diff and release package are preserved in `experiments/settings-menu-v018-backup-UPoAOw5N/`. The prototype builder checks the exact stock executable and preserved v0.4.18 hashes and verifies IPS application to both. The new UI adds a bounded 428-byte descriptor allocation, retained for reuse; native widgets and labels use the game's existing allocation paths. This does not make options-screen allocation failures impossible.

Rebuild separately, using your own exact stock executable and preserved v0.4.18 patched executable:

```sh
python3 scripts/buildRuntimeControls.py --reference-code /path/to/stock-code.bin --native-menu --output-header build-native-menu/RuntimeControlsData.h --artifacts-dir build-native-menu/payload
python3 scripts/buildOverlayPayload.py --reference-code /path/to/stock-code.bin --background --output-header build-native-menu/OverlayData.h --artifacts-dir build-native-menu/overlay
python3 scripts/buildNativeMenuPrototype.py --stock-code /path/to/stock-code.bin --baseline-code /path/to/v0.4.18-code.bin --payload build-native-menu/payload --overlay build-native-menu/overlay --output build-native-menu/fixture
```

`native-validation.json` records 188 ARM UI configuration/value/polling cases and 11 native dropdown focus/broadcast cases, plus the controls, overlay, texture, mesh and builder-memory regression checks. Native widget allocation and callbacks are controlled dependencies in the ARM tests. Added dropdowns ignore broadcast keys unless focused or expanded, preventing confirmation from opening neighboring dropdowns; stock dropdown IDs retain their original behavior. Private, muted Azahar tests separately exercise title/pause entry, dropdown navigation, touch and controller slider changes, on/off controls, shared-button capture, and settings persistence across restart. Console hardware, multiplayer and long-session stability are not verified by these tests.

## Stable v0.4.18 controls

The European v9.11.0 update includes all four modes and settings in one `code.ips`. Open **Options > Controls** for the patcher-generated baseline. A new installation starts with L + Circle Pad and the defaults below. Allocation troubleshooting is kept in [graphics-allocation.md](docs/crashes/graphics-allocation.md).

Select **Mode** to cycle through L + Circle Pad, Circle Pad Pro, Touch stick, and ABXY camera. Each mode remembers its own bindings and camera/touch tuning. Select **Page** to cycle through Gameplay binds, Menu binds, Camera, Touch stick, Display, and Binding tools. Select an action, release the activating input, hold the desired button or chord, and release it to finish. Multiple actions may share the same button or chord, including the camera modifier. Assigning a shared binding leaves the other assignments unchanged and activates all matching actions in that context. A held chord suppresses its individual component actions until released. Conflicting actions are your responsibility.

While capturing, tap the selected row again to unbind it, or tap another row to cancel. The bottom rows also offer Cancel capture and Clear binding. Binding tools can reset or unbind the selected action, copy another mode's bindings, and mark exact duplicate bindings with an asterisk. Unbound actions do nothing, so you can disable POV switching or any other mapped action. **Reset this mode to defaults** restores that mode's bindings and camera/touch tuning, not global display settings. Changes save with the normal game options when you leave Options. v0.4.16 bindings migrate automatically; malformed or incompatible data is ignored.

The Controls page itself always uses stock navigation so custom menu bindings cannot prevent editing it. Touch navigation remains available. Outside that page, menu remapping includes confirm, cancel, contextual X/Y actions, group changes, directional buttons, START, SELECT, and ZL/ZR hotbar actions. Gameplay also exposes the alternate hotbar bindings and camera modifier. Circle Pad axes, touch targets, HOME, POWER, and system controls are not remapped. Standard button prompts follow the active bindings; chords, START/SELECT and unbound actions use compact text in the original icon slot. Long chords can be small. Analog diagrams and the Controls page's recovery prompts remain unchanged.

The base game without the update retains the two patch-time choices: `--controls l-circle-pad` (default) and `--controls circle-pad-pro`. It does not have the new Controls page. Do not pass a control-mode flag when building the update; choose its mode in-game instead. The legacy `--overlay` flag is accepted for updates but produces the same IPS; choose the overlay in-game.

## ABXY camera

This dedicated mode frees the four face buttons for looking while the Circle Pad still moves. Camera > Look up/down/left/right lets you rebind or disable each direction. Opposing directions cancel; diagonal speed is normalized. Menus keep their independent normal bindings.

| Default input | Gameplay action |
| --- | --- |
| X / B / Y / A | Look up / down / left / right |
| D-pad Up / Down | Jump / sneak |
| L / R | Use / attack |
| SELECT / START | Inventory / pause |
| L + R | Crafting |
| L + Down | Drop |
| Left / Right, ZL / ZR | Previous / next hotbar slot |
| Unbound | Camera perspective and alternate pause |

Press L before R for the crafting chord. You can use the touch crafting button instead. Defaults for the other three modes are unchanged.

## Camera, touch and display settings

Camera provides the game's shared native sensitivity, invert Y and auto-jump, separate per-mode horizontal/vertical gains (25-200%), and hold/toggle camera modifier for L + Circle Pad mode. Toggle resets when opening menus, switching modes or changing settings. Reset camera tuning restores sensitivity, inversion, gains and hold behavior, not auto-jump. Reset this mode leaves the shared native options unchanged.

Touch-stick settings adjust centre X/Y, radius, dead zone, opacity and hiding the idle stick. Defaults retain the existing circle. Custom positioning can overlap native touch targets; a gesture beginning inside the stick belongs to the stick.

Display settings are shared between modes:

- Overlay: Off (default), FPS, or Full. Drawn only in gameplay.
- Particles: Normal, Reduced (half draw distance), or Hidden. Simulation and cleanup continue.
- Weather visuals: Normal or Hidden. Weather simulation continues.
- Map refresh: Normal, 4 Hz, 1 Hz, or Paused. A map can still initialize once when first needed.
- Low-memory textures: Off by default. After saving and fully restarting, eligible three-mip terrain atlases use their existing lower mip levels, halving their dimensions. Other textures and incompatible resource-pack atlases remain unchanged. This is a quality/memory tradeoff, not a proven graphics-allocation fix.

There is no added FPS cap.

## Touch stick

Touch-stick mode draws a translucent circle over the bottom-screen map. Press inside it and move the stylus away from its centre to turn the camera. Distance controls turn speed, with a small dead zone and a capped maximum speed. Release to stop. The Circle Pad continues to move the player while you aim.

| Input | Action |
| --- | --- |
| Touch stick | Look around. |
| Circle Pad | Walk or strafe. |
| L | Place/use/interact. Previous category where supported in menus. |
| R | Attack or break blocks. |
| B | Drop in gameplay; back/cancel in menus. |
| A | Jump or confirm/select. |
| X / Y | Inventory / crafting. |
| D-pad Left / Right | Previous / next hotbar slot. |
| D-pad Up / Down | Camera perspective / sneak. |
| START / SELECT | Pause. |
| Other touchscreen areas | Original hotbar and HUD buttons. |

The circle is centred at (104, 160) on the 320 x 240 touch screen, over the map and clear of the three buttons on the right. A press beginning outside the circle stays with the original interface even if dragged inside. A press beginning inside stays with the stick until release, including drags beyond the circle. Opening a menu cancels camera movement; lift the stylus before starting a new gesture. The stick is hidden in menus.

This mode defaults to the original game's button bindings, including L for use and B for drop. It does not require a Circle Pad Pro. An enabled overlay may overlap custom stick positions. Physical Old 3DS behavior still needs validation.

## L + Circle Pad: gameplay

| Input | Action |
| --- | --- |
| Circle Pad | Walk or strafe. Tap forward twice, then hold to sprint. |
| Hold L + Circle Pad | Look around without walking input. Release L to walk again. |
| L alone | Camera modifier only. |
| A | Jump or contextual dismount. In Creative, double-tap to toggle flight; hold to rise. |
| B | Place a block or use/interact. Hold for actions such as eating or drawing a bow; release to shoot. |
| R | Attack or break blocks. Hold to mine. |
| X | Inventory. |
| Y | Crafting or Creative item selection. |
| Hold L, then press Y | Drop from the selected hotbar slot. Release Y before repeating. |
| D-pad Left / Right | Previous / next hotbar slot. Replaces ZL / ZR. |
| D-pad Up | Change camera perspective. |
| D-pad Down | Sneak on the ground or descend while flying. |
| START / SELECT | Pause. START also works at the title screen. |
| Touch a hotbar slot | Select it. |
| Touch chest / crafting / pause icons | Open inventory / crafting / pause. |

L + B and L + R work while aiming. You cannot walk and look with the Circle Pad simultaneously. Holding L stops walking input, not falling or knockback. No C-stick or Circle Pad Pro is needed.

Press L before Y to drop. Y stays suppressed until released so crafting does not open when you let go of L. Use this shortcut only during gameplay.

## L + Circle Pad: menus and inventory

| Input | Action |
| --- | --- |
| Circle Pad / D-pad | Navigate. |
| A | Confirm/select, or take/place stacks as shown on screen. |
| B | Back, cancel, or close the screen. |
| Y | Contextual action, such as Take Half or Clear Hotbar. Do not hold L. |
| X / START / SELECT | Follow the current screen's normal prompts. |
| R | Next category where supported. |
| L | Unbound by default outside the Controls page. Bind Previous Group or touch a category tab. |
| Touchscreen | Select buttons, tabs, items, and slots. No new touchscreen camera gesture is added. |

Standard button hints now reflect the bindings above. The electronic manual is not included.

From v0.4.11, tap B to open a crafting table, furnace or other container, then release it normally. A fresh B press closes the container. Its opening release is no longer treated as Cancel. Other menus retain their original back-button behavior, and held gameplay use is unchanged.

## Circle Pad Pro

CPP mode defaults to the game's original button bindings. The left Circle Pad and accessory right pad work independently, so you can walk and look at the same time.

| Input | Action |
| --- | --- |
| Circle Pad | Walk or strafe. |
| Accessory right pad | Look around. |
| L | Place/use/interact. Previous category where supported in menus. |
| R | Attack or break blocks. |
| B | Drop in gameplay; back/cancel in menus. |
| ZL / ZR or D-pad Left / Right | Previous / next hotbar slot. |
| A | Jump or confirm/select. |
| X / Y | Inventory / crafting. |
| D-pad Up / Down | Camera perspective / sneak. |
| START / SELECT | Pause. |
| Touchscreen | Normal buttons, tabs, items, and hotbar slots. |

L does not switch the left Circle Pad into camera input, and L + Y is not a drop shortcut in this mode. On-screen L and B hints match the original bindings.

Use the Circle Pad Pro model that fits your console, with a working battery. The patch retries the game's native IR accessory connection in the background. Basic buttons and the left Circle Pad remain available while connection attempts fail; looking requires the accessory. There is no automatic L + Circle Pad fallback. Physical connection, calibration, sleep/wake, and reconnection still need hardware testing.

HOME, POWER, volume, the lid, and the 3D slider are unchanged. Some retained actions, including sprinting, flight, eating, and contextual inventory operations, have not all been individually verified.
