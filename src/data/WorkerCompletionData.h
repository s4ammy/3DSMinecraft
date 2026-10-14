#pragma once

#include "core/Bytes.h"

namespace Mc3ds {
    inline constexpr std::size_t updateLateCompletionOffset = 0x25d634;
    inline constexpr std::size_t updateLateCompletionHookOffset = 0x6fb48;
    inline constexpr std::size_t updateLateCompletionSize = 0x24;
    inline const std::string updateLateCompletionHash = "30f1aa74cfec21cea254849a4aa14ef5ab86ce96e0c5d2a3c6d97dcc8b12270a";
    inline const std::string updateLateCompletionHex = "040094e5081094e5010050e17149f80a000090e50010d0e5000051e34db3f61b6c49f8ea";

    struct TWorkerGuard {
        const char *name;
        std::size_t offset;
        std::size_t size;
        const char *hash;
    };

    inline const TWorkerGuard updateWorkerGuards[] = {
        {"worker frame budget", 0x6fb28, 0x198, "478131a173d8ba2a408d390c7f0973909e2aaea1c96ea4e888116761abed9ded"},
        {"worker completion dequeue", 0xa38c, 0x170, "1b60d321737808ca180963d2151532fbfae4ce1a998cbaf79b07750e73c45663"},
        {"worker pool groups", 0xf520, 0x3b8, "8c58518cbbb93c5ac40c4c70a7db470ac827e2023d42ae7ae85d5598d9cc7a0b"},
    };
}
