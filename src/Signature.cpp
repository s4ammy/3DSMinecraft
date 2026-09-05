#include "Signature.h"

#include <stdexcept>

std::vector<std::size_t> Mc3ds::FindMatches(const TBytes &data, const TSignature &signature, std::size_t limit) {
    if (signature.bytes.empty() || signature.bytes.size() != signature.mask.size() || limit == 0 ||
        signature.targetOffset >= signature.bytes.size()) {
        throw std::runtime_error("Invalid signature definition: " + signature.name);
    }

    auto fixed = std::size_t{0};
    while (fixed < signature.mask.size() && signature.mask[fixed] == 0) {
        ++fixed;
    }

    if (fixed == signature.mask.size()) {
        throw std::runtime_error("Signature has no fixed bits: " + signature.name);
    }

    auto matches = std::vector<std::size_t>{};
    if (data.size() < signature.bytes.size()) {
        return matches;
    }

    for (auto offset = std::size_t{0}; offset <= data.size() - signature.bytes.size(); offset += 4) {
        if ((data[offset + fixed] & signature.mask[fixed]) != (signature.bytes[fixed] & signature.mask[fixed])) {
            continue;
        }

        auto equal = true;
        for (auto index = std::size_t{0}; index < signature.bytes.size(); ++index) {
            if ((data[offset + index] & signature.mask[index]) != (signature.bytes[index] & signature.mask[index])) {
                equal = false;
                break;
            }
        }

        if (equal) {
            matches.push_back(offset + signature.targetOffset);
            if (matches.size() == limit) {
                break;
            }
        }
    }

    return matches;
}

std::size_t Mc3ds::FindUnique(const TBytes &data, const TSignature &signature) {
    const auto matches = FindMatches(data, signature);
    if (matches.size() != 1) {
        throw std::runtime_error("Signature '" + signature.name + "' has " +
            (matches.empty() ? "no match" : "multiple matches") + "; this build is not safely supported");
    }

    return matches.front();
}

std::uint32_t Mc3ds::ArmBranch(std::uint32_t source, std::uint32_t target) {
    const auto distance = static_cast<std::int64_t>(target) - static_cast<std::int64_t>(source) - 8;
    if ((source & 3) != 0 || (target & 3) != 0 || distance < -33554432 || distance > 33554428) {
        throw std::runtime_error("ARM branch is unaligned or outside its range");
    }

    return 0xea000000U | (static_cast<std::uint32_t>(distance / 4) & 0x00ffffffU);
}

std::uint32_t Mc3ds::ArmBranchTarget(std::uint32_t instruction, std::uint32_t source) {
    if ((instruction & 0x0e000000U) != 0x0a000000U) {
        throw std::runtime_error("Expected an ARM B/BL instruction");
    }

    auto words = static_cast<std::int64_t>(instruction & 0x00ffffffU);
    if ((words & 0x00800000) != 0) {
        words -= 0x01000000;
    }

    const auto target = static_cast<std::int64_t>(source) + 8 + words * 4;
    if (target < 0 || target > 0xffffffffLL) {
        throw std::runtime_error("ARM branch target overflows address space");
    }

    return static_cast<std::uint32_t>(target);
}
