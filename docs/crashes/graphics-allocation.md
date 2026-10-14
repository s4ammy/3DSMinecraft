# Graphics allocation failures

## Evidence and status

The general allocation failure remains unresolved. These observations must not be treated as one proven cause:

| Observation | Established evidence |
| --- | --- |
| Failed 51,840-byte request | 656,188 bytes were free, but no block exceeded 17,100 bytes. This demonstrates fragmentation in that sampled run. |
| Failed 2 MiB request during an in-world Options transition | The resource kind follows native texture storage. The ultimate caller and any relationship to the new UI remain unconfirmed. Upload staging uses a different memory kind. |
| Block-rebuild diagnostic failure, 2026-10-03 | One diagnostic run failed allocation. Its evidence is retained separately from successful mesh-latency repeats. |

The in-world Options report was recorded in `analysis-codex-2/runtime-controls-20261013/`. Routine and follow-up evidence is under `analysis-codex-2/runtime-settings-20261013/`, `analysis-codex-2/memory-stability-20261013/` and `analysis-codex-2/settings-next-20261014/`.

## Changes made and their limits

v0.4.18 caps deferred chunk builders at four, preserving the immediate builder and native empty-pool retry. Fewer builders provide [measured memory headroom](../../PERFORMANCE.md#v0418-chunk-builder-memory-headroom), but do not establish a fragmentation or resource-lifetime fix. The original allocation/failure policy remains in place. An experimental parent-heap fallback is excluded because synthetic success does not establish safe live graphics ownership.

The low-memory texture option skips the largest stored mip in eligible terrain atlases after a full restart. The source-byte counter is not a measurement of resident graphics memory. Lower chunk distances, fewer builders and visual presets are configurable reductions, not proof that these failures are resolved.

If an in-world Options visit fails, the title-screen settings page is an alternative for changing settings. An exact dump and reproduction are still needed to establish the failing resource and ownership path.

## Follow-up validation

The low-memory/ABXY run completed Controls without a monitored native allocation-failure breakpoint. Afterwards, the graphics arena had 1,653,032 free bytes and a largest block of 1,567,004 bytes. That repeat does not resolve the earlier failure.

The builder-memory candidate passed block edits, movement, item pickup, saving and same-process reload without a monitored allocation failure. Its baseline diagnostic did not reproduce the 2 MiB request during loading, Controls or Save & Quit. The first candidate's full main-heap inventory was interrupted because it was too intrusive and is not counted as completed validation.

The settings/diagnostics candidate also passed menu changes, gameplay, saving and fresh-process reload. Its diagnostic repeat did not reproduce a request of at least 2 MiB, leaving the ultimate texture caller unresolved. Largest-free-payload overlay values are not alignment-aware allocation guarantees, and nested heap totals must not be added together. Hardware timing, long sessions and multiplayer remain outstanding.
