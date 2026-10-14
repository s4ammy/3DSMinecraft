# Mixed chunk-distance loading stall

This was a loading hang, not a recorded data-abort crash. The targeted final configuration passed a fresh emulator test.

With render distance 1 and nearby radius 3, the earlier experiment stayed at progress `0.752357` in loading state 2 while the tick counter continued advancing. The resident/ticking neighborhood needed an independent generation margin; simply requesting the selected visible distance was insufficient.

The final chunk-radius request is `max(min(render, nearby), nearby - 1)`. It retains native local/server acceptance and the source's original 32-block padding. Render-frame requests separately cap visibility by the selected distance and native acceptance.

For render 1 / nearby 3, the final executable requests radius 2 while limiting visible distance to 16 blocks. A fresh private run reached loading state 16/progress 1 and entered the saved world with that configuration and one deferred builder. No loading thresholds or sound-preload stages were bypassed.

The earlier maximum/minimum runs are recorded separately from this final mixed-distance check. See [chunk settings validation](../../PERFORMANCE.md#restart-frozen-chunk-settings-experiment) and [settings ranges](../../GUIDE.md#experimental-chunk-settings). These runs are not hardware performance or maximum-memory safety guarantees.
