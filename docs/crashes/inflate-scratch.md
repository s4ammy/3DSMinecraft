# Inflate scratch-pointer corruption

Status: fixed in v0.4.6 routine tests. Do not use v0.4.5 as a known-good saved-world loading baseline.

The v0.4.5 hook stored scratch ownership at `SP+0x18`. The live `z_stream` occupies `SP+0x10` through `SP+0x47`, and `SP+0x18` is `total_in`. Real zlib resets and increments that counter, corrupting the pointer and potentially causing invalid writes or frees. A six-stream regression test reproduced an invalid write in the old build.

v0.4.6 stores ownership at `SP+0x48`, outside the stream but inside the existing frame. It adds no stack-frame growth or global scratch buffer. The fixed control and candidate match stock decompression in 128 tests, covering truncated streams, invalid checksums and invalid headers. Timing comparisons use stock decompression and a v0.4.5 control with this bug corrected.

The corrected control and combined build reached gameplay in private Azahar. Their navigation movies used different worlds and therefore do not establish a loading gain. Physical-console and long-session validation remain separate from the routine-level result.

Private evidence and executable-dependent harnesses: `analysis-codex-2/performance-v046-20260911/`. Performance results remain in [PERFORMANCE.md](../../PERFORMANCE.md#retained-v046-changes).
