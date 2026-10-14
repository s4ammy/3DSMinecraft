# Legacy bootstrap IPS reconstruction defect

Status: corrected in the experimental performance test 2, then retained by test 3. Its contribution to the user's physical-console lag is unconfirmed.

The first combined performance experiment left 82 incorrect executable bytes when its IPS was applied to the actual private legacy bootstrap. The defect reproduced an Azahar startup fault. Test 2 corrected IPS compatibility, and validation checked complete component deltas, rejected address overlaps and reconstructed the exact combined executable.

Test 3 retained that correction and changed the touch-controller slot comparison to prevent old menu presses from being replayed when camera input selects an inactive controller. It passed native checks and private muted Azahar loading, block edits, movement, saving and fresh-process reload. These are experimental packages, not additional optimizations silently included in the normal patcher.

See [test 3 validation](../../PERFORMANCE.md#2026-10-03-corrected-full-test-and-biome-lookup-optimization). Physical Old 3DS timing and the original lag report remain unverified by these tests.
