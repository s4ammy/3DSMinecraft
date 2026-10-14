import argparse
import hashlib
import tempfile
from pathlib import Path

from buildPerformancePayloads import Compile, projectPath, referenceHash


def Main():
    parser = argparse.ArgumentParser(description="Build the late-frame completion hook without rebuilding unrelated payloads.")
    parser.add_argument("--reference-code", type=Path, required=True)
    parser.add_argument("--check", action="store_true")
    arguments = parser.parse_args()
    reference = arguments.reference_code.read_bytes()
    if hashlib.sha256(reference).hexdigest() != referenceHash:
        raise RuntimeError("The input is not the supported stock update executable")
    with tempfile.TemporaryDirectory(prefix="mc3ds-worker-build-") as temporaryDirectory:
        payload, _ = Compile("performance/LateWorkerCompletion.s", 0x35D634, "CompleteStreamingJobWhenLate", Path(temporaryDirectory),
            linkedSymbols={"CompleteOneWorkerJob": 0x10A38C, "WorkerBudgetExhausted": 0x16FC0C})
    if len(payload) != 36:
        raise RuntimeError("Completion hook no longer fits the reserved gap")
    lines = ["#pragma once", "", '#include "core/Bytes.h"', "", "namespace Mc3ds {",
             "    inline constexpr std::size_t updateLateCompletionOffset = 0x25d634;",
             "    inline constexpr std::size_t updateLateCompletionHookOffset = 0x6fb48;",
             "    inline constexpr std::size_t updateLateCompletionSize = 0x24;",
             f'    inline const std::string updateLateCompletionHash = "{hashlib.sha256(payload).hexdigest()}";',
             f'    inline const std::string updateLateCompletionHex = "{payload.hex()}";', "",
             "    struct TWorkerGuard {", "        const char *name;", "        std::size_t offset;",
             "        std::size_t size;", "        const char *hash;", "    };", "",
             "    inline const TWorkerGuard updateWorkerGuards[] = {"]
    for name, offset, size in (("worker frame budget", 0x6FB28, 0x198),
                              ("worker completion dequeue", 0xA38C, 0x170),
                              ("worker pool groups", 0xF520, 0x3B8)):
        digest = hashlib.sha256(reference[offset:offset + size]).hexdigest()
        lines.append(f'        {{"{name}", 0x{offset:x}, 0x{size:x}, "{digest}"}},')
    lines += ["    };", "}", ""]
    content = "\n".join(lines)
    target = projectPath / "src/data/WorkerCompletionData.h"
    if arguments.check:
        if target.read_text() != content:
            raise RuntimeError("WorkerCompletionData.h does not match the rebuilt payload")
    else:
        target.write_text(content)
    print("Worker completion payload: 36 bytes, checked stock ABI")


if __name__ == "__main__":
    Main()
