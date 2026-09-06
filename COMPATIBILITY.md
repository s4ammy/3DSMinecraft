# Compatibility

Verified input: European Minecraft v0.1.0, product code `KTR-P-BD3P`, title ID `000400000017ca00`, CIA version 16.

Original CIA SHA-256:

```text
ebd865c5656be81ed2faf3122eec5d8c93b069e4ad211f3596b879f9f78abfe3
```

Patched executable SHA-256 (patcher 0.1.1, including the pickup correction):

```text
c2ed3f1670cab5b461c29758a7bdb0cd0b00dd1105553f2f37cc0a1c1f71af6f
```

The preceding executable (`d484dbb3211d7bc2d48b28bef0a1aba3b38db34a058daaabd74b5c983b791f81`) was reported working well on an Old 3DS. Version 0.1.1 adds only the pickup animation hook and its trampoline to that code. Its extended header and icon are unchanged.

The corrected executable passed 348 ARM execution cases and Azahar testing in Old 3DS mode at 100% CPU timing. Three consecutive drop/pickup cycles and a mined block returned items to inventory without leaving visual copies. Save & Quit also passed. This correction still needs confirmation on physical Old 3DS hardware.

Version 0.1.1 completed full CIA rebuilds on Linux and with the Windows executable under Wine. Both runs re-extracted and verified the corrected executable, icon, and unchanged game assets. Private profile checks also passed for altered instructions, ambiguous signatures, occupied padding, and relocated pickup hooks.

The patcher preserves the original CIA version 16. Whole CIA hashes can differ from development packages because installation metadata is regenerated; the executable hash above identifies the corrected game code.

Other versions and regions are unverified. `--allow-similar` tries them with strict signature checks, but does not guarantee they will work. Updates, DLC, and already-patched executables are not supported. Start with an original CIA.

Performance depends on the world. A steady minimum of 20 FPS has not been established on hardware. Multiplayer and long-running worlds are also unverified.
