# Legacy LayeredFS text-hook overlap

Versions through v0.3.0 placed patch hooks after the declared text extent. Luma LayeredFS used the same location for its runtime payload, so enabling a replacement could overwrite those hooks and crash on hardware.

v0.3.1 corrected the extended-header text metadata so Luma places its payload after the hooks. The compatible bootstrap ExHeader declares that corrected extent and preserves the SD-application capability. Generated IPS files reserve the required text padding. Existing shader replacements need no modification.

An IPS does not replace installed icon or ExHeader metadata. A v0.3.0-only installation must be checked for compatible bootstrap metadata before using the current IPS workflow. See [first-time setup](../../GUIDE.md#install-the-bootstrap-cias). Existing compatible bootstrap installations need no reinstall for the v0.4.19 fixes.

This older text-side overlap is distinct from the v0.4.18 [read-only binding-table/path collision](layeredfs-path.md).
