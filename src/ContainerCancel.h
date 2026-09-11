#pragma once

#include "Bytes.h"

#include <array>

namespace Mc3ds {
    struct TContainerCancelProfile {
        std::uint32_t actionOffset;
        const char *actionHash;
        std::uint32_t hookOffset;
        std::uint32_t resumeOffset;
        std::uint32_t callbackOffset;
        const char *callbackHash;
        std::uint32_t dispatchOffset;
        const char *dispatchHash;
        std::array<std::uint32_t, 3> derivedCallbackOffsets;
        const char *derivedCallbackHash;
    };

    inline constexpr std::size_t containerCancelHelperOffset = 0xa0;
    inline constexpr std::size_t containerCancelHelperSize = 16;
    inline const std::string containerCancelHelperHex = "0400a0e10110a0e3fcffffebfbffffea";
    extern const TContainerCancelProfile baseContainerCancel;
    extern const TContainerCancelProfile updateContainerCancel;

    void PatchContainerCancel(const TBytes &original, TBytes &patched, TBytes &controls,
        std::uint32_t textAddress, std::uint32_t controlsAddress, const TContainerCancelProfile &profile);
}
