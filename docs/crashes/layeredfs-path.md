# LayeredFS startup collision, hardware dump 6

## Symptom and cause

Replacing the installed IPS with the earlier native-menu build caused a startup failure. Hardware dump `crash_dump_00000006.dmp` reaches `CStringLength` at `0x2FE9B4` with the invalid pointer `0x2F3A666C`, the bytes `lf:/`.

The compatible bootstrap declares read-only data ending at `0xA28464`. After IPS application, Luma writes its 39-byte redirected-file path at that address. The earlier native-menu candidates and v0.4.18 release placed the compact dictation-binding table there too. The loader's path therefore overwrote binding data.

## Resolution

v0.4.19 reserves 40 aligned bytes for Luma and repacks the table at `0xA2848C`. Payload construction and packaging reject overlapping writes and require the reservation to remain unused. Text-side LayeredFS padding, bootstrap metadata, BSS, settings, bindings and MC3DS4 encoding are preserved. No new bootstrap is required for an existing compatible installation.

The separate local `release/experiments/layeredfs-fix-20261014/code.ips` was tested before promotion. The user confirmed improved startup after installing it, then confirmed the combined v0.4.19 candidate works after the settings fix.

## Validation and limits

`scripts/checkLayeredFsCompatibility.py` checks the reservation against the compatible bootstrap ExHeader and preserves text-side loader padding. All nine input-builder cases execute before and after path injection, giving 18 passing cases. The negative control reproduces the earlier patch's exact hardware PC, LR, invalid read and table register.

The full ARM settings suite and rendering fast paths pass after path injection. Five host CTests, ten release-validation tests and native payload reproducibility pass. These checks model the path write rather than a complete Luma boot. The hardware report is limited startup/settings confirmation, not broad device coverage.

Run the settings suite with `--layeredfs-exheader BASELINE_EXHEADER` to include the loader-side write. Private diagnosis evidence: `analysis-codex-2/layeredfs-collision-20261014/`.

This is separate from the older [text-hook overlap](legacy-layeredfs.md).
