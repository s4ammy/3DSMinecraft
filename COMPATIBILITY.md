# Compatibility

Verified input: European Minecraft v0.1.0, product code `KTR-P-BD3P`, title ID `000400000017ca00`, CIA version 16.

Original CIA SHA-256:

```text
ebd865c5656be81ed2faf3122eec5d8c93b069e4ad211f3596b879f9f78abfe3
```

Patched executable SHA-256:

```text
d484dbb3211d7bc2d48b28bef0a1aba3b38db34a058daaabd74b5c983b791f81
```

The patched code, extended header, and icon match the build reported working well on an Old 3DS. The patcher completed full runs on Linux and with the Windows executable under Wine. The new CIA package has not had a separate console installation check.

The whole CIA hash differs from the earlier build because installation metadata is regenerated and the original version 16 is preserved instead of 0. The patched game code is identical.

Other versions and regions are unverified. `--allow-similar` tries them with strict signature checks, but does not guarantee they will work. Updates, DLC, and already-patched executables are not supported. Start with an original CIA.

Performance depends on the world. A steady minimum of 20 FPS has not been established on hardware. Multiplayer and long-running worlds are also unverified.
