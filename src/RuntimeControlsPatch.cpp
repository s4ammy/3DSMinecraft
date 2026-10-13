#include "RuntimeControlsPatch.h"
#include "RuntimeControlsData.h"
#include "ContainerCancel.h"

#include <algorithm>
#include <stdexcept>

void Mc3ds::InstallRuntimeControls(const TBytes &original, TPatchResult &result) {
    if (Sha256(original) != updateOriginalCodeHash) {
        throw std::runtime_error("In-game controls require the exact supported v9.11.0 update executable");
    }

    for (const auto &region : updateRuntimeControlsRegions) {
        RequireRange(original.size(), region.offset, region.capacity);
        const auto begin = original.begin() + static_cast<std::ptrdiff_t>(region.offset);
        if (Sha256(TBytes(begin, begin + static_cast<std::ptrdiff_t>(region.capacity))) != region.originalHash) {
            throw std::runtime_error("Runtime controls reservation differs from the stock executable");
        }

        const auto payload = FromHex(region.hex);
        if (payload.size() > region.capacity || Sha256(payload) != region.payloadHash) {
            throw std::runtime_error("Runtime controls payload size or checksum changed");
        }

        auto destination = result.code.begin() + static_cast<std::ptrdiff_t>(region.offset);
        std::fill_n(destination, region.capacity, std::uint8_t{0});
        std::copy(payload.begin(), payload.end(), destination);
        result.ipsOverwriteRanges.push_back({region.offset, region.capacity});
    }

    Write32(result.code, 0x25c74c, 0xe12fff1e);
    const auto branch = [&](std::uint32_t source, std::uint32_t target, bool link) {
        Write32(result.code, source - 0x100000, ArmBranch(source, target) | (link ? 0x01000000U : 0));
    };
    branch(0x108120, updateRuntimeInputHookAddress, false);
    branch(0x10b6bc, updateRuntimeAccessoryHookAddress, false);
    branch(0x10bcd4, updateRuntimeShutdownHookAddress, false);
    branch(0x235634, updateBuildControlRowsAddress, true);
    branch(0x2362cc, updateHandleControlButtonAddress, false);
    branch(0x236598, updateDispatchControlReleaseAddress, false);
    branch(0x236e0c, updateDestroyControlRowsAddress, false);
    branch(0x632cd0, updateLoadControlSettingsAddress, true);
    branch(0x635034, updateSaveControlSettingsAddress, true);
    branch(0x679344, updateRuntimeHudDestroyHookAddress, false);
    Write32(result.code, 0x579000, (ArmBranch(0x679000, updateDrawRuntimeControlsAddress) & 0x00ffffffU) | 0x0b000000U);

    auto controls = TBytes(0x100, 0);
    PatchContainerCancel(original, result.code, controls, 0x100000, 0x918824, updateContainerCancel);
    std::copy_n(controls.begin() + containerCancelHelperOffset, containerCancelHelperSize,
        result.code.begin() + 0x818824 + containerCancelHelperOffset);
}
