# Compatibility

Verified input: European Minecraft v0.1.0, product code `KTR-P-BD3P`, title ID `000400000017ca00`, CIA version 16.

Original CIA SHA-256:

```text
ebd865c5656be81ed2faf3122eec5d8c93b069e4ad211f3596b879f9f78abfe3
```

L + Circle Pad executable SHA-256 (patchers 0.1.1 and 0.2.0, including the pickup correction):

```text
c2ed3f1670cab5b461c29758a7bdb0cd0b00dd1105553f2f37cc0a1c1f71af6f
```

Circle Pad Pro executable SHA-256 (patcher 0.2.0):

```text
18dff470eeb11111dc6ed4ee3edec7c8ea9e6516e994beb5594f7bb9de684264
```

The preceding executable (`d484dbb3211d7bc2d48b28bef0a1aba3b38db34a058daaabd74b5c983b791f81`) was reported working well on an Old 3DS. Version 0.1.1 adds only the pickup animation hook and its trampoline to that code. Its extended header and icon are unchanged.

The corrected executable passed 348 ARM execution cases and Azahar testing in Old 3DS mode at 100% CPU timing. Three consecutive drop/pickup cycles and a mined block returned items to inventory without leaving visual copies. Save & Quit also passed. This correction still needs confirmation on physical Old 3DS hardware.

Version 0.1.1 completed full CIA rebuilds on Linux and with the Windows executable under Wine. Both runs re-extracted and verified the corrected executable, icon, and unchanged game assets. Private profile checks also passed for altered instructions, ambiguous signatures, occupied padding, and relocated pickup hooks.

Version 0.2.0's CPP mode also completed full CIA rebuilds on Linux and with the Windows executable under Wine. Both verified the expected CPP executable and unchanged game assets. Profile checks confirmed that the default mode still produces the 0.1.1 executable and that neither mode accepts already-patched input. The accessory worker passed 80 private ARM execution cases for thread creation, retries, shutdown, and register/stack preservation.

Azahar 2126 in Old 3DS mode at 100% CPU timing verified CPP camera input on both axes, simultaneous movement and camera input, ZL / ZR hotbar selection, L placement, B drop/pickup, menu input, and Save & Quit. Debugger-injected connection failures, including a five-second blocked connection attempt, left gameplay and menus responsive. Restoring the native connection routine allowed the accessory stream and camera to recover. Invoking the cleanup routine through the debugger joined both accessory threads and cleared their state before returning.

Azahar emulates the IR accessory using its C-stick and ZL / ZR mappings, so it can exercise the game's accessory path. It does not validate a physical Circle Pad Pro's IR link, battery, calibration, duplicate R button, or sleep/wake behavior. CPP mode has not been tested on physical hardware. Those checks and physical reconnection are still pending.

The patcher preserves the original CIA version 16. Whole CIA hashes can differ from development packages because installation metadata is regenerated; the executable hashes above identify each control mode.

Other versions and regions are unverified. `--allow-similar` tries them with strict signature checks in L + Circle Pad mode, but does not guarantee they will work. CPP mode requires the exact supported executable even with that option. Updates, DLC, and already-patched executables are not supported. Start with an original CIA.

Performance depends on the world. A steady minimum of 20 FPS has not been established on hardware. Multiplayer and long-running worlds are also unverified.
