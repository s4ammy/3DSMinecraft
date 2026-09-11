# Compatibility

The European bootstrap base and update have been reported working on an Old 3DS with the default L + Circle Pad controls. Both include the pickup animation fix and are safe to use with Luma LayeredFS. Version 0.4.4 outputs a Luma IPS executable patch for fast iteration after the one-time bootstrap installation. Circle Pad Pro still needs physical hardware testing.

| Input | Product code | Title ID | CIA version |
| --- | --- | --- | --- |
| Base v0.1.0 | `KTR-P-BD3P` | `000400000017ca00` | 16 |
| Update v9.11.0, game 1.9 | `KTR-U-BD3P` | `0004000e0017ca00` | 9392 |

When the bootstrap update is installed, generate the IPS from the original update CIA. Generate from the base CIA only when no update is installed. Both use Luma directory `/luma/titles/000400000017CA00/`.

The retail SMDH is marked New 3DS-only, and retail ExHeader settings are incompatible with this Old 3DS patch. Luma IPS does not replace either one. Existing bootstrap installations provide those prerequisites and should not be uninstalled.

## Reference SHA-256 hashes

| File | SHA-256 |
| --- | --- |
| Original base CIA | `ebd865c5656be81ed2faf3122eec5d8c93b069e4ad211f3596b879f9f78abfe3` |
| Original update CIA | `8526ef24719d074c1b3de4742e23c795c003ff51d10f7ea99839c6974587a75b` |
| Base executable, L + Circle Pad | `a1c746e9efbf8bd8d26deb5abe33e1944c9cfd579f8233239536d0166e3b5740` |
| Base executable, Circle Pad Pro | `6aaca1236211154d86f81b23c84fae9913effd48d316a3bb05e02a077c9a71e2` |
| Update executable, L + Circle Pad, overlay off (default) | `cb943e340bdb701c09fa70a0bb537f42d97d7172748531fe8011b7834535d998` |
| Update executable, Circle Pad Pro, overlay off (default) | `a25bd838bf5142032a2eae357c7f319c9e6467d6595596bbc3cfef2ac5aeaa46` |
| Update executable, L + Circle Pad, `--overlay` | `805180e1554c29de079dd3374c99cac5ebba1a0a04d3cffb1c0602395912b04f` |
| Update executable, Circle Pad Pro, `--overlay` | `65fd0645adcd8abe6cb595a3328efc104f0255fdcaa16c7fe472fb187afc8038` |

Version 0.3.1 changed extended-header text metadata so Luma places its LayeredFS payload after the patcher's hooks. Version 0.3.2 forced on the update's retained frame-timer display. Version 0.4.0 fixed its invalid `2147483647 FPS` current value and replaced CIA rebuilding with external `code.bin` and `exheader.bin` output. Version 0.4.1 added the SD-application capability required for resource-pack access. Version 0.4.2 switched to the reliable bootstrap plus `code.ips` route. Version 0.4.3 adjusted the retained frame-timer layout. Version 0.4.4 added an experimental Mods UI proof of concept. Version 0.4.5 removes that UI experiment and all debug-overlay patches, then adds guarded chunk decompression, biome-layer RNG, and startup presentation optimizations.

## Limits

Version 0.4.6 fixes a v0.4.5 regression: the reusable scratch pointer overlapped zlib's consumed-byte counter and could cause invalid writes or frees. Do not use v0.4.5 as a known-good saved-world loading baseline. Comparisons use stock decompression and a v0.4.5 control with that bug corrected. Version 0.4.6 additionally optimizes block lookup, deterministic climate work, compressed-record copying and inflate output. See [the validation report](PERFORMANCE.md); these changes still require Old 3DS hardware testing.

Version 0.4.7 adds direct raw-light lookup, per-call inflate-context reuse and exact modulo-eight masking in biome-color generation. It keeps the same bootstrap requirements and LayeredFS shader files. The experimental biome-ready cache is excluded because a controlled neighbor-availability test produced stale colors. Context reuse reduced allocator traffic but raised peak live test allocations by up to 32 KiB in the sampled cases; low-memory hardware behavior remains unverified.

Other regions and revisions are unverified. `--allow-similar` tries them with strict signature and layout checks. The update profile supports this in both modes; base-game CPP mode still requires its exact reference executable. Missing or ambiguous matches are rejected. DLC and already-patched inputs are unsupported.

Azahar checks covered base-game CPP controls and connection failures, plus update startup, saved-world loading, movement, camera, and dropping. The update's CPP gameplay has not been checked in Azahar. Emulation cannot confirm the accessory's physical IR link, battery, calibration, or sleep/wake behavior.

Versions through 0.3.0 placed patch hooks after the declared text extent. Luma LayeredFS used the same location for its runtime payload, so enabling a LayeredFS replacement could overwrite those hooks and crash on hardware. The bootstrap ExHeader declares the corrected extent and preserves the SD-application capability. The generated IPS reserves space after its hooks for Luma. The replacement assets themselves do not need modification.

Version 0.4.8 adds the [custom FPS/debug overlay](DEBUG_OVERLAY.md) to the European update without changing the bootstrap requirements, tail padding or shaders. The old broken frame-timer renderer is replaced, not re-enabled unchanged. The base-game build is unchanged.

Version 0.4.12 makes that overlay optional and disabled by default. Add `--overlay` for either update control profile; enabled output is byte-identical to the corresponding v0.4.11 executable. Disabled output retains the stock display callback and skips the retired renderer while keeping its adjacent heap helpers. Base outputs and bootstrap metadata are unchanged; `--overlay` is rejected for base inputs.

Version 0.4.9 changes only the update's locked heap free-list neighbor search relative to v0.4.8. It uses 68 bytes after the overlay in the same retired renderer reservation, with no new BSS, heap allocation, stack frame or bootstrap metadata. Original merging and insertion code remains intact. The patcher rejects overlay growth into the reserved search payload. See [measured loading results and validation limits](PERFORMANCE.md). The existing bootstrap installation and LayeredFS files are retained.

Version 0.4.10 adds only the four-byte-aligned allocation search fast path relative to v0.4.9. Its 72-byte payload preserves allocation policy, selected addresses, original splitting and commit logic, and locking. Both control profiles pass exact output-hash and guarded fixture tests. No bootstrap, BSS, LayeredFS padding, shader or save-format change is required. Measured world-loading gains are from one saved-world fixture in Azahar, not console timing or a demonstrated gameplay FPS gain.

Performance varies by world. Multiplayer, long sessions, and a steady minimum frame rate have not been established. The update optimizations, custom overlay and Circle Pad Pro profile still need Old 3DS hardware testing.

Version 0.4.11 changes the default-profile common container back handler and three derived handlers, including crafting, to ignore releases. A 16-byte helper closes the container from its menu_cancel press action after the original selected-slot cleanup. Both versions use existing input-payload padding, with no new BSS, bootstrap metadata, shader or save changes. CPP executable hashes remain unchanged. This does not address the separately reported night-time slowdown/crash. A public v0.3.0-only install must first be checked for compatible bootstrap metadata before using the current IPS workflow.
