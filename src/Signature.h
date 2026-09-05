#pragma once

#include "Bytes.h"

namespace Mc3ds {
    struct TSignature {
        std::string name;
        TBytes bytes;
        TBytes mask;
        std::size_t targetOffset;
    };

    std::vector<std::size_t> FindMatches(const TBytes &data, const TSignature &signature, std::size_t limit = 2);
    std::size_t FindUnique(const TBytes &data, const TSignature &signature);
    std::uint32_t ArmBranch(std::uint32_t source, std::uint32_t target);
    std::uint32_t ArmBranchTarget(std::uint32_t instruction, std::uint32_t source);
}
