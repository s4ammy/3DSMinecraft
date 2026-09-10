# Compatibility

The European base game and update have been reported working on an Old 3DS with the default L + Circle Pad controls. Both include the pickup animation fix. Circle Pad Pro still needs physical hardware testing.

| Input | Product code | Title ID | CIA version |
| --- | --- | --- | --- |
| Base v0.1.0 | `KTR-P-BD3P` | `000400000017ca00` | 16 |
| Update v9.11.0, game 1.9 | `KTR-U-BD3P` | `0004000e0017ca00` | 9392 |

The update needs the matching patched base game. The patcher preserves each title's identity, version, and save mapping. Back up saves before updating.

## Reference SHA-256 hashes

| File | SHA-256 |
| --- | --- |
| Original base CIA | `ebd865c5656be81ed2faf3122eec5d8c93b069e4ad211f3596b879f9f78abfe3` |
| Original update CIA | `8526ef24719d074c1b3de4742e23c795c003ff51d10f7ea99839c6974587a75b` |
| Base executable, L + Circle Pad | `c2ed3f1670cab5b461c29758a7bdb0cd0b00dd1105553f2f37cc0a1c1f71af6f` |
| Base executable, Circle Pad Pro | `18dff470eeb11111dc6ed4ee3edec7c8ea9e6516e994beb5594f7bb9de684264` |
| Update executable, L + Circle Pad | `f20d5063fe2b77daaec640de1a81d5c42817a746c0f5d355d0ab8f1d04b7b04c` |
| Update executable, Circle Pad Pro | `e296f8c6a5e66ce9f2be5918e927cae8008fae824bd3bbec60069f54fda54040` |

Base outputs are unchanged from 0.2.0. Rebuilt CIA hashes may vary with regenerated installation metadata; executable hashes identify the patch mode.

## Limits

Other regions and revisions are unverified. `--allow-similar` tries them with strict signature and layout checks. The update profile supports this in both modes; base-game CPP mode still requires its exact reference executable. Missing or ambiguous matches are rejected. DLC and already-patched inputs are unsupported.

Azahar checks covered base-game CPP controls and connection failures, plus update startup, saved-world loading, movement, camera, and dropping. The update's CPP gameplay has not been checked in Azahar. Emulation cannot confirm the accessory's physical IR link, battery, calibration, or sleep/wake behavior.

Performance varies by world. Multiplayer, long sessions, and a steady minimum frame rate have not been established. The FPS/memory overlay is available in the base game only.
