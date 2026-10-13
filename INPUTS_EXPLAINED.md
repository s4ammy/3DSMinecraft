# Controls

The European v9.11.0 update now includes all three modes in one `code.ips`. Open **Options > Controls** from the title screen or pause menu. A new installation starts with L + Circle Pad and the defaults below.

Select **Mode** to cycle through L + Circle Pad, Circle Pad Pro, and Touch stick. Each mode remembers its own gameplay and menu bindings. Select **Bindings** to switch the group being edited, then select an action, release the activating input, hold the desired button or chord, and release it to finish. Multiple actions may share the same button or chord, including the camera modifier. Assigning a shared binding leaves the other assignments unchanged and activates all matching actions in that context. Conflicting actions are your responsibility, so rebind or unbind any you do not want to trigger together.

While capturing, tap the selected row again to unbind it, or tap another row to cancel. **Reset this mode to defaults** restores both groups for that mode only. Changes are saved with the normal game options when you leave Options. Malformed or incompatible saved control data is ignored.

The Controls page itself always uses stock navigation so custom menu bindings cannot prevent editing it. Touch navigation remains available. Outside that page, menu remapping includes confirm, cancel, contextual X/Y actions, group changes, directional buttons, START, and SELECT. Gameplay also exposes the alternate ZL/ZR hotbar bindings and the camera modifier. Circle Pad axes, touch targets, HOME, POWER, and system controls are not remapped. Native on-screen button hints still show their original buttons.

The base game without the update retains the two patch-time choices: `--controls l-circle-pad` (default) and `--controls circle-pad-pro`. It does not have the new Controls page. Do not pass a control-mode flag when building the update; choose its mode in-game instead. The optional `--overlay` remains a separate build choice.

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

This mode defaults to the original game's button bindings, including L for use and B for drop. It does not require a Circle Pad Pro. `--overlay` can be combined with it, though the debug text also occupies part of the map. Physical Old 3DS behavior still needs validation.

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

Button hints are unchanged: gameplay hints showing L for Use now mean B; B for Drop now means L + Y. The electronic manual is not included.

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
