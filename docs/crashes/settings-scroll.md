# Settings pane lifetime fault, hardware dump 7

## Symptom and cause

The user crashed while navigating the added settings menu. Hardware dump `crash_dump_00000007.dmp` faults at `0x240DD0` while the native scroll setter dereferences the pane's listener at offset 60, which contains `0x5544`.

Native analysis identified a lifetime hazard: the listener synchronously calls container layout, layout can delete and replace the pane, and the original normalized-scroll and clamp routines continue using that pane. The dump is consistent with this reproduced fault but does not record the heap-reuse sequence.

## Resolution

v0.4.19 installs a private listener only on the Mod Settings container. Its callback records a pending layout. Layout runs at render entry, after active pane input calls return. Stock Options retains its original listener.

The pending flag uses existing menu-state padding. The 404-byte allocation, BSS, settings, bindings, MC3DS4 encoding and bootstrap are unchanged. The combined IPS retains the [LayeredFS startup fix](layeredfs-path.md). The user confirmed this candidate works after replacing the SD-card patch.

## Validation and limits

`scripts/nativeScrollChecks.py` runs the actual ARM scroll, listener, layout and clamp routines with controlled allocation and external dependencies. All eight old-behavior cases read freed pane memory. Four cases modelling allocation-header reuse reproduce the dump's exact fault PC. All eight candidate cases complete with zero stale reads. Another 13 cases verify custom-only installation, layout before rendering, pending-state consumption and screen cleanup.

The full ARM settings suite, LayeredFS path injection, rendering fast paths, five host CTests and byte-for-byte native rebuild reproducibility pass. Hidden, muted Azahar completed 21 interaction cycles across all seven pages, with touch/controller selection, scrollbar drags and dropdown cancellation, followed by ten exits and reopenings with live screen-stack references checked. Camera sensitivity changes and stock Options were also checked.

This is limited hardware startup/settings confirmation. Broad device coverage and long-session stability remain unverified. The earlier [current-screen release diagnostic](screen-release.md) is a separate unresolved observation.

Private evidence: `analysis-codex-2/settings-crash7-20261014/`. The local candidate was `release/experiments/settings-scroll-fix-20261014/code.ips`; the published asset is `release/ips/v0.4.19/code.ips`.
