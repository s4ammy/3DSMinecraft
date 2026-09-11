#include "ContainerCancel.h"
#include "Signature.h"

#include <algorithm>
#include <stdexcept>

const Mc3ds::TContainerCancelProfile Mc3ds::baseContainerCancel = {
    0x1f19a0, "fd11fe5945280b0ba596db023f3a45bbdc281c9573c35f0da75c0e42b56d610b",
    0x1f19d4, 0x1f19d8, 0x1f20f0, "b608aa29508d0cd042cc48618947768861b8ca06162b0ef6d217e314ebee4dd1",
    0x1f2804, "c43c84a5a9f0351dbee4e6895674803dbe8b4b9deabd17468d129b6d3e02136e",
    {0x1173f4, 0x235b84, 0x246c8c}, "c5c1b5293ed40ad3b59fcf43474b2a548da35c1f33307cf757075c890a0d8090"};

const Mc3ds::TContainerCancelProfile Mc3ds::updateContainerCancel = {
    0x2ae770, "1c1dbc8db2beac4c0bba042fc4783e7d8afb08fd9956c07db83439f3c27f5b41",
    0x2ae7a4, 0x2ae7a8, 0x2af078, "0b7a46b77ba16e403ef9f736a8f478ae83d872f4d014c16cc292cc1c0c958b6e",
    0x2af9ac, "b2e8b95c55970936753f99f845eb324add067c2d1fe3e9739e94a53b8d1f8425",
    {0x1543cc, 0x32e954, 0x35a90c}, "dd49557e080c2180ec0ab5dc6c23e4a3ef69879ada248e93a9cae7c3ebb1568b"};

void Mc3ds::PatchContainerCancel(const TBytes &original, TBytes &patched, TBytes &controls,
    std::uint32_t textAddress, std::uint32_t controlsAddress, const TContainerCancelProfile &profile) {
    const auto checkRange = [&](std::size_t offset, std::size_t size, const char *expectedHash) {
        RequireRange(original.size(), offset, size);
        RequireRange(patched.size(), offset, size);
        const auto first = original.begin() + static_cast<std::ptrdiff_t>(offset);
        const auto last = first + static_cast<std::ptrdiff_t>(size);
        if (Sha256(TBytes(first, last)) != expectedHash ||
            !std::equal(first, last, patched.begin() + static_cast<std::ptrdiff_t>(offset))) {
            throw std::runtime_error("Container cancel ABI differs or overlaps another patch");
        }
    };
    checkRange(profile.actionOffset, 0x3c, profile.actionHash);
    checkRange(profile.callbackOffset, 0x44, profile.callbackHash);
    checkRange(profile.dispatchOffset, 0x6c, profile.dispatchHash);
    for (const auto offset : profile.derivedCallbackOffsets) {
        checkRange(offset, 0x20, profile.derivedCallbackHash);
    }

    RequireRange(controls.size(), containerCancelHelperOffset, containerCancelHelperSize);
    const auto destination = controls.begin() + containerCancelHelperOffset;
    if (!std::all_of(destination, destination + containerCancelHelperSize, [](auto value) {
            return value == 0;
        })) {
        throw std::runtime_error("Container cancel helper overlaps the input payload");
    }

    auto helper = FromHex(containerCancelHelperHex);
    if (textAddress != 0x100000 || helper.size() != containerCancelHelperSize || controlsAddress > 0x02000000) {
        throw std::runtime_error("Unexpected container cancel helper layout");
    }

    const auto helperAddress = controlsAddress + static_cast<std::uint32_t>(containerCancelHelperOffset);
    Write32(helper, 8, ArmBranch(helperAddress + 8, textAddress + profile.callbackOffset) | 0x01000000U);
    Write32(helper, 12, ArmBranch(helperAddress + 12, textAddress + profile.resumeOffset));
    std::copy(helper.begin(), helper.end(), destination);

    Write32(patched, profile.hookOffset, ArmBranch(textAddress + profile.hookOffset, helperAddress));
    Write32(patched, profile.callbackOffset + 12, 0x0a000008);
    for (const auto offset : profile.derivedCallbackOffsets) {
        Write32(patched, offset + 8, 0x0a000002);
    }
}
