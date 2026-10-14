# Crash investigations and resolutions

Each record keeps the reported symptom, established cause, resolution, validation and remaining limits together. Resolved routine faults do not establish general hardware or long-session stability. Related hangs and input regressions are included where they were investigated alongside a crash report.

| Record | Status | Resolution |
| --- | --- | --- |
| [LayeredFS startup, dump 6](layeredfs-path.md) | Fixed, user hardware confirmation | v0.4.19 reserves Luma's path storage |
| [Settings scrolling, dump 7](settings-scroll.md) | Reproduced lifetime fault fixed, user hardware confirmation | v0.4.19 defers pane layout |
| [Graphics allocation failures](graphics-allocation.md) | Unresolved | Extra memory headroom is not a general fix |
| [Current-screen release diagnostic](screen-release.md) | Unconfirmed cause | Did not recur in fresh testing |
| [Inflate scratch-pointer corruption](inflate-scratch.md) | Fixed in routine tests | v0.4.6 moves ownership outside `z_stream` |
| [Legacy LayeredFS hook overlap](legacy-layeredfs.md) | Corrected bootstrap layout | v0.3.1 text metadata correction |
| [Legacy bootstrap IPS reconstruction](legacy-bootstrap-ips.md) | Fixed in experimental test 2 | Reconstruct all expected executable bytes |
| [Container closes on B release](container-release.md) | Fixed in ARM and update emulator tests | v0.4.11 closes on cancel press |
| [Night-time slowdown/crash report](night-time.md) | Unresolved | Needs its own dump and reproduction |
| [Mixed chunk-distance loading stall](chunk-loading-stall.md) | Fixed in targeted emulator test | Preserve the generation margin |

Private evidence paths beginning with `analysis-codex-2/` are relative to the canonical reversal workspace, `/home/admin/Desktop/Reversing/3DSMCRE`. They describe local evidence, not downloadable game data. Performance measurements remain in [PERFORMANCE.md](../../PERFORMANCE.md); installation prerequisites remain in [COMPATIBILITY.md](../../COMPATIBILITY.md).
