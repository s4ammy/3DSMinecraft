# Current-screen release diagnostic

Status: unconfirmed cause. No fix is claimed for this separate observation.

The earlier private `settings-crash7-20261014/after-02` emulator run failed after leaving settings. It logged an invalid current-screen read at `0x3E8980` during button-release dispatch. This dereferences the current screen, rather than the scrolling-pane listener from [hardware dump 7](settings-scroll.md).

That diagnostic run used unsupported single-step/breakpoint handling. The diagnostic helper was corrected to use one-shot breakpoints and preserve the first exception instead of repeatedly resuming it. This correction improves the evidence collection; it does not establish the game failure's cause.

A fresh hidden, muted session completed 21 interaction cycles across all seven pages and ten exit/reopen cycles without reproducing the failure. Live screen-stack references were checked, and stock Options and the Play menu were visited successfully. The absence of a repeat is not proof of a resolution.

Private evidence: `analysis-codex-2/settings-crash7-20261014/`.
