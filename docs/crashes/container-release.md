# Container closes on B release

This is a related input regression, not an established cause of the [night-time crash report](night-time.md).

The published v0.3.0 Old 3DS report described B immediately closing an interacted container. v0.4.10 left the relevant callbacks unchanged. v0.4.11 corrects the default-controls path: the common container back-state callback and three derived callbacks, including crafting, ignore releases.

The container `menu_cancel` action performs its original selected-slot cleanup, then invokes the common callback with an explicit nonzero state to close through the screen's own virtual method. Game-level back dispatch and non-container screens retain stock routing. This prevents gameplay B-use release from being treated as Cancel without a timer or suppression of gameplay use.

Base and update relocate a shared 16-byte ARM helper inside the existing 256-byte controls reservation. There is no new BSS, bootstrap metadata, shader or save format. CPP does not receive the change and its executable hashes are unchanged. Action-handler, callback and action-dispatcher checksum guards reject incompatible layouts or overlapping patches.

## Validation and limits

`scripts/checkContainerCancelPayload.py` checks the helper against its assembly. Historical private fixture tests checked exact output hashes, original CPP behavior, IPS round trips and modified-routine rejection. Restoring the five changed instructions and clearing the helper reproduced the previous executable hash exactly. Those historical fixture tests and their CMake options are no longer included in the source build.

Private Unicorn tests executed the actual ARM action dispatcher and callbacks, checking selected-slot cleanup order, non-cancel actions, release handling, stack balance and preserved registers. The final update passed hidden, muted Azahar crafting-table and furnace tap/hold opening and closing, plus inventory and Options cancellation. Base validation is limited to fixtures and ARM tests. Physical Old 3DS validation remains outstanding.

Private evidence: `analysis-codex-2/reported-v030-20260911/FIX-v0411.md`.
