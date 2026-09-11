#include "UpdatePatches.h"
#include "UpdatePatchData.h"
#include "PerformanceData.h"
#include "OverlayData.h"
#include "SignatureData.h"
#include "ContainerCancel.h"

#include <algorithm>
#include <map>
#include <set>
#include <sstream>

namespace Mc3ds {
    void Require(bool condition, const std::string &message);
    std::uint32_t CheckedAdd(std::uint32_t value, std::uint32_t amount);
    std::uint32_t RoundPage(std::uint32_t value);
    std::string NormalizedSystemProfile(TBytes exheader);
    std::size_t ResolveUpdateLiteral(const TBytes &code, std::size_t instructionOffset);
    void RelocateUpdateBranch(TBytes &payload, std::uint32_t payloadAddress, std::uint32_t offset, std::uint32_t destination);
}

std::size_t Mc3ds::ResolveUpdateLiteral(const TBytes &code, std::size_t instructionOffset) {
    const auto instruction = Read32(code, instructionOffset);
    Require((instruction & 0x0f7f0000U) == 0x051f0000U, "Expected an ARM literal load in the update profile");
    const auto displacement = instruction & 0xfffU;
    const auto pcOffset = CheckedAdd(static_cast<std::uint32_t>(instructionOffset), 8);
    Require((instruction & 0x800000U) != 0 || pcOffset >= displacement, "Literal load precedes the executable");
    const auto offset = (instruction & 0x800000U) != 0 ? CheckedAdd(pcOffset, displacement) : pcOffset - displacement;
    RequireRange(code.size(), offset, 4);

    return offset;
}

void Mc3ds::RelocateUpdateBranch(TBytes &payload, std::uint32_t payloadAddress, std::uint32_t offset, std::uint32_t destination) {
    const auto instruction = Read32(payload, offset);
    Require((instruction & 0x0e000000U) == 0x0a000000U, "Expected an ARM branch relocation");
    Write32(payload, offset, (instruction & 0xff000000U) | (ArmBranch(CheckedAdd(payloadAddress, offset), destination) & 0x00ffffffU));
}

Mc3ds::TPatchResult Mc3ds::PatchUpdateGame(const TBytes &code, const TBytes &exheader, const TBytes &icon, bool allowSimilar,
    EControlMode controlMode, bool enableOverlay) {
    const auto circlePadPro = controlMode == EControlMode::CIRCLE_PAD_PRO;
    const auto inputHash = Sha256(code);
    const auto known = inputHash == updateOriginalCodeHash;
    Require(known || allowSimilar, "Unrecognized update executable SHA-256: " + inputHash + ". Use --allow-similar to try the guarded update signatures; compatibility is not guaranteed.");
    Require(exheader.size() == 0x800, "Expected a complete update extended header");
    auto normalizedHeader = exheader;
    normalizedHeader.at(0xe) = 0;
    normalizedHeader.at(0xf) = 0;
    Require(NormalizedSystemProfile(normalizedHeader) == systemProfileHash,
        "Update capabilities or dependencies differ from the supported system profile");
    Require((Read64(exheader, 0x1c8) >> 32) == 0x0004000e &&
            (Read64(exheader, 0x200) >> 32) == 0x00040000 &&
            static_cast<std::uint32_t>(Read64(exheader, 0x1c8)) == static_cast<std::uint32_t>(Read64(exheader, 0x200)),
        "Update application identity is inconsistent");

    const auto textAddress = Read32(exheader, 0x10);
    const auto textSize = Read32(exheader, 0x18);
    const auto roAddress = Read32(exheader, 0x20);
    const auto roSize = Read32(exheader, 0x28);
    const auto dataAddress = Read32(exheader, 0x30);
    const auto dataSize = Read32(exheader, 0x38);
    const auto bssSize = Read32(exheader, 0x3c);
    Require(textAddress == 0x100000 && textSize >= 0x1000 && textSize < 0x2000000 && Read32(exheader, 0x1c) == 0x40000,
        "Unsupported update text layout or stack size");
    Require(Read32(exheader, 0x14) == RoundPage(textSize) / 0x1000 && roAddress == CheckedAdd(textAddress, RoundPage(textSize)) &&
            Read32(exheader, 0x24) == RoundPage(roSize) / 0x1000 && dataAddress == CheckedAdd(roAddress, RoundPage(roSize)) &&
            Read32(exheader, 0x34) == RoundPage(dataSize) / 0x1000 &&
            code.size() == static_cast<std::uint64_t>(dataAddress) - textAddress + RoundPage(dataSize),
        "Update segments or executable size are inconsistent");
    const auto bssStart = CheckedAdd(dataAddress, dataSize);
    const auto bssEnd = CheckedAdd(bssStart, bssSize);
    const auto extraBssSize = circlePadPro ? 0x1040U : 16U;
    Require((bssEnd & (circlePadPro ? 7U : 3U)) == 0, "Update control storage must be aligned");
    const auto controlsOffset = CheckedAdd(textSize, 3) & ~3U;
    const auto pickupOffset = CheckedAdd(controlsOffset, 0x100);
    const auto accessoryOffset = CheckedAdd(pickupOffset, 0x20);
    const auto performanceOffset = CheckedAdd(accessoryOffset, 0x27c);
    const auto caveEnd = CheckedAdd(performanceOffset, static_cast<std::uint32_t>(updatePayloadSize));
    Require(performanceOffset >= CheckedAdd(CheckedAdd(accessoryOffset, 0x14c), lumaLayeredFsPayloadSize),
        "Performance payload overlaps the installed Circle Pad Pro bootstrap LayeredFS injection range");
    Require(caveEnd <= RoundPage(textSize), "No room for update hooks in text padding");
    Require(RoundPage(textSize) - caveEnd >= lumaLayeredFsPayloadSize,
        "No room for the Luma LayeredFS payload after the update hooks");
    Require(std::all_of(code.begin() + textSize, code.begin() + RoundPage(textSize),
                [](auto value) {
                    return value == 0;
                }),
        "Update text padding is occupied");
    Require(icon.size() == 0x36c0 && std::string(icon.begin(), icon.begin() + 4) == "SMDH" && Read32(icon, 0x2028) == 0x11c1,
        "Unexpected update icon format or platform flags");

    auto anchors = std::map<std::string, std::size_t>{};
    for (const auto &signature : updateAnchorSignatures) {
        const auto offset = FindUnique(code, signature);
        Require(offset >= signature.targetOffset &&
                offset - signature.targetOffset + signature.bytes.size() <= textSize,
            "An update anchor resolved outside executable text");
        Require(anchors.emplace(signature.name, offset).second, "Duplicate update anchor name");
    }

    const auto addressOf = [&](const std::string &name) {
        return CheckedAdd(textAddress, static_cast<std::uint32_t>(anchors.at(name)));
    };
    const auto startupOffset = anchors.at("startupBssEnd");
    Require(startupOffset >= 4 && Read32(code, startupOffset - 4) == bssStart && Read32(code, startupOffset) == bssEnd,
        "Update startup BSS clearing differs from the extended header");
    const auto initializerLiteral = ResolveUpdateLiteral(code, anchors.at("chunkInitializer"));
    const auto chunkBase = Read32(code, initializerLiteral);
    Require(chunkBase >= bssStart && CheckedAdd(chunkBase, 57 * 8) <= bssEnd, "Update chunk table is outside BSS");
    const auto nativeHookAddress = addressOf("nativeHidHook");
    const auto nativeContinuation = CheckedAdd(nativeHookAddress, 0x38);
    Require(ArmBranchTarget(Read32(code, anchors.at("nativeHidHook")), nativeHookAddress) == addressOf("accessoryStatus"),
        "Native HID hook does not call the matched accessory status routine");
    const auto particleCallAddress = addressOf("particleCall");
    Require(ArmBranchTarget(Read32(code, anchors.at("particleCall")), particleCallAddress) == addressOf("particleTick"),
        "Particle call does not target the matched tick routine");
    Require(Read32(code, anchors.at("particleTick")) == 0xe92d4ff0 && Read32(code, anchors.at("particleTick") + 4) == 0xe24dd00c &&
            Read32(code, anchors.at("particleTick") + 8) == 0xe1a08000,
        "Particle tick frame differs from the pickup hook");
    const auto initializedAddress = Read32(code, ResolveUpdateLiteral(code, anchors.at("accessoryStop")));
    Require(initializedAddress >= dataAddress && initializedAddress < bssEnd, "Accessory initialization flag is outside writable data");

    const auto decompressInitializeOffset = std::size_t{0x2ddd20};
    const auto decompressAcquireOffset = std::size_t{0x2dddec};
    const auto decompressReleaseInitOffset = std::size_t{0x2dde28};
    const auto decompressReleaseErrorOffset = std::size_t{0x2dde74};
    const auto decompressRetainOffset = std::size_t{0x2ddec8};
    const auto decompressFinishOffset = std::size_t{0x2ddf18};
    const auto nextIntOffset = std::size_t{0x4c3318};
    const auto startupPresentationOneOffset = std::size_t{0x81973c};
    const auto startupPresentationTwoOffset = std::size_t{0x819744};
    RequireRange(code.size(), decompressInitializeOffset, 4);
    RequireRange(code.size(), decompressAcquireOffset, 4);
    RequireRange(code.size(), decompressReleaseInitOffset, 4);
    RequireRange(code.size(), decompressReleaseErrorOffset, 4);
    RequireRange(code.size(), decompressRetainOffset, 4);
    RequireRange(code.size(), decompressFinishOffset, 4);
    RequireRange(code.size(), nextIntOffset, 4);
    RequireRange(code.size(), startupPresentationOneOffset, 4);
    RequireRange(code.size(), startupPresentationTwoOffset, 4);
    Require(Read32(code, decompressInitializeOffset) == 0xe24dd04c &&
            Read32(code, decompressAcquireOffset) == 0xebf51bfb &&
            Read32(code, decompressReleaseInitOffset) == 0xebfc830c &&
            Read32(code, decompressReleaseErrorOffset) == 0xebfc82f9 &&
            Read32(code, decompressRetainOffset) == 0xebfc82e4 &&
            Read32(code, decompressFinishOffset) == 0xe28dd04c,
        "Chunk decompression scratch-buffer flow differs from the supported update");
    Require(Read32(code, nextIntOffset) == 0xe1a02001,
        "CLayer::NextInt entry differs from the supported update");
    Require(Read32(code, startupPresentationOneOffset) == 0x40a00000 &&
            Read32(code, startupPresentationTwoOffset) == 0x40a00000,
        "Startup presentation timing table differs from the supported update");
    for (const auto &guard : updatePerformanceGuards) {
        RequireRange(code.size(), guard.offset, guard.size);
        const auto guardedBytes = TBytes(code.begin() + static_cast<std::ptrdiff_t>(guard.offset),
            code.begin() + static_cast<std::ptrdiff_t>(guard.offset + guard.size));
        Require(Sha256(guardedBytes) == guard.hash,
            "Performance ABI differs from the supported update: " + std::string(guard.name));
    }

    for (const auto &guard : updateOverlayGuards) {
        RequireRange(code.size(), guard.offset, guard.size);
        const auto guardedBytes = TBytes(code.begin() + static_cast<std::ptrdiff_t>(guard.offset),
            code.begin() + static_cast<std::ptrdiff_t>(guard.offset + guard.size));
        Require(Sha256(guardedBytes) == guard.hash,
            "Overlay ABI differs from the supported update: " + std::string(guard.name));
    }
    Require(updateOverlayOffset + updateOverlayCapacity <= textSize &&
            updateOverlayStateAddress >= bssStart &&
            updateOverlayStateAddress + updateOverlayStateCapacity <= bssEnd,
        "Overlay replacement code or state is outside its checked segment");
    Require(Read32(code, updateOverlayGateOffset) == 0x0a000005 &&
            Read32(code, updateOverlayVblankHookOffset) == 0x4f9044,
        "Overlay drawing or submission gate changed");

    const auto blockLookupOffset = std::size_t{0x74648};
    const auto climateSeedOffset = std::size_t{0xba950};
    const auto prepareInflateOffset = std::size_t{0x2dde48};
    const auto commitInflateOffset = std::size_t{0x2ddeac};
    const auto restoreInflateOffset = std::size_t{0x2dde64};
    const auto lightLookupOffset = std::size_t{0x57240};
    const auto inflateInitializeOffset = std::size_t{0x2dde14};
    const auto inflateRetainOffset = std::size_t{0x2dded4};
    const auto biomeRemainderOffset = std::size_t{0xc7e74};
    const auto inflateCallbackOffsets = {std::size_t{0x2dddf0}, std::size_t{0x2dddf4}, std::size_t{0x2dddf8}};
    const auto storageWrites = std::map<std::size_t, std::uint32_t>{
        {0xbed0c, 0xe1a00007}, {0xbed1c, 0xe320f000}, {0xbed28, 0xe320f000}, {0xbed8c, 0xe320f000}};

    struct TLocatedUpdatePatch {
        const TPatchSpec *spec;
        std::size_t offset;
    };

    auto locations = std::vector<TLocatedUpdatePatch>{};
    auto chunkReferences = std::set<std::size_t>{initializerLiteral};
    auto claimed = std::set<std::size_t>{};
    const auto claimRange = [&](std::size_t offset, std::size_t size) {
        for (auto index = std::size_t{0}; index < size; ++index) {
            Require(claimed.insert(offset + index).second, "Update patch targets overlap");
        }
    };
    claimRange(decompressInitializeOffset, 4);
    claimRange(decompressAcquireOffset, 4);
    claimRange(decompressReleaseInitOffset, 4);
    claimRange(decompressReleaseErrorOffset, 4);
    claimRange(decompressRetainOffset, 4);
    claimRange(decompressFinishOffset, 4);
    claimRange(nextIntOffset, 4);
    claimRange(startupPresentationOneOffset, 4);
    claimRange(startupPresentationTwoOffset, 4);
    claimRange(blockLookupOffset, 0xe8);
    claimRange(climateSeedOffset, 4);
    claimRange(prepareInflateOffset, 4);
    claimRange(commitInflateOffset, 4);
    claimRange(restoreInflateOffset, 4);
    claimRange(lightLookupOffset, 4);
    claimRange(inflateInitializeOffset, 4);
    claimRange(inflateRetainOffset, 4);
    claimRange(biomeRemainderOffset, 4);
    claimRange(updateOverlayOffset, updateOverlayCapacity);
    claimRange(updateOverlayGateOffset, 4);
    claimRange(updateOverlayVblankHookOffset, 4);
    claimRange(updateHeapSearchHookOffset, 4);
    claimRange(updateAlignedAllocationHookOffset, 4);
    for (const auto offset : inflateCallbackOffsets) {
        claimRange(offset, 4);
    }
    for (const auto &write : storageWrites) {
        claimRange(write.first, 4);
    }

    for (const auto &spec : updatePatchSpecs) {
        const auto signature = TSignature{spec.name, FromHex(spec.signatureHex), FromHex(spec.maskHex), spec.targetOffset};
        const auto offset = FindUnique(code, signature);
        const auto before = FromHex(spec.beforeHex);
        const auto mask = FromHex(spec.beforeMaskHex);
        Require(before.size() == 4 && mask.size() == before.size(), "Invalid update instruction guard");
        for (auto index = std::size_t{0}; index < before.size(); ++index) {
            Require((code[offset + index] & mask[index]) == (before[index] & mask[index]), "Unexpected bytes for " + spec.name);
            Require(claimed.insert(offset + index).second, "Update patch targets overlap");
        }

        if (spec.kind == EWriteKind::CHUNK_BASE) {
            Require(Read32(code, offset) == chunkBase, "Update chunk consumers disagree on the table base");
            chunkReferences.insert(offset);
        }

        if (offset >= textSize) {
            Require(spec.name.find("renderDistanceChoice") == 0 && offset >= roAddress - textAddress &&
                    offset + before.size() <= static_cast<std::uint64_t>(roAddress) - textAddress + roSize,
                "An update code signature resolved outside executable text");
        }

        locations.push_back({&spec, offset});
    }

    // The initializer keeps the full allocation; only runtime consumers move.
    auto allChunkReferences = std::set<std::size_t>{};
    for (auto offset = std::size_t{0}; offset + 4 <= code.size(); offset += 4) {
        if (Read32(code, offset) == chunkBase) {
            allChunkReferences.insert(offset);
        }
    }

    Require(chunkReferences.size() == 5 && allChunkReferences == chunkReferences, "Unrecognized update chunk-table references");
    for (const auto &name : {"startupBssEnd", "inputHook", "nativeHidHook", "particleCall", "accessoryShutdown"}) {
        for (auto index = std::size_t{0}; index < 4; ++index) {
            Require(claimed.insert(anchors.at(name) + index).second, "Update hooks overlap a patch target");
        }
    }

    auto result = TPatchResult{code, exheader, icon, {}, known};
    auto report = std::ostringstream{};
    report << "Executable SHA-256: " << inputHash << "\nProfile: European v9.11.0 update"
           << (known ? " reference executable\n" : " experimental signature match\n")
           << "Control mode: " << ControlModeName(controlMode) << "\n";
    for (const auto &location : locations) {
        const auto &spec = *location.spec;
        if (circlePadPro && spec.kind == EWriteKind::CONTROL_BINDING) {
            continue;
        }

        if (spec.kind == EWriteKind::CHUNK_BASE) {
            Write32(result.code, location.offset, CheckedAdd(chunkBase, 24));
        } else {
            const auto after = FromHex(spec.afterHex);
            Require(after.size() == 4, "Update instruction patch must preserve size");
            std::copy(after.begin(), after.end(), result.code.begin() + static_cast<std::ptrdiff_t>(location.offset));
        }

        report << spec.name << " at code+0x" << HexNumber(location.offset) << "\n";
    }

    auto controls = FromHex(updateControlsHex);
    auto pickup = FromHex(updatePickupHex);
    auto accessory = FromHex(updateAccessoryHex);
    auto performance = FromHex(updatePerformanceHex);
    const auto blockLookup = FromHex(updateBlockLookupHex);
    const auto lightLookup = FromHex(updateLightLookupHex);
    Require(controls.size() == 0x100 && pickup.size() == 0x20 && accessory.size() == 0x14c && performance.size() == updatePerformanceSize &&
            blockLookup.size() == 0xe8 && lightLookup.size() == updateLightLookupSize && performance.size() + lightLookup.size() == updatePayloadSize,
        "Invalid update payload sizes");
    Require(Sha256(performance) == updatePerformanceHash && Sha256(blockLookup) == updateBlockLookupHash &&
            Sha256(lightLookup) == updateLightLookupHash,
        "Performance payload checksum or linked targets differ");
    const auto controlsAddress = CheckedAdd(textAddress, controlsOffset);
    const auto pickupAddress = CheckedAdd(textAddress, pickupOffset);
    const auto accessoryAddress = CheckedAdd(textAddress, accessoryOffset);
    const auto performanceAddress = CheckedAdd(textAddress, performanceOffset);
    Require(performanceAddress == 0x918bc0, "Performance payload address differs from its linked address");
    RelocateUpdateBranch(controls, controlsAddress, 0x98, CheckedAdd(addressOf("inputHook"), 4));
    Write32(controls, 0x9c, bssEnd);
    RelocateUpdateBranch(pickup, pickupAddress, 4, addressOf("particleTick"));
    RelocateUpdateBranch(pickup, pickupAddress, 0x18, addressOf("pickupTickLoop"));
    Write32(pickup, 0x1c, CheckedAdd(particleCallAddress, 4));
    std::copy(pickup.begin(), pickup.end(), result.code.begin() + pickupOffset);
    Write32(result.code, anchors.at("particleCall"), ArmBranch(particleCallAddress, pickupAddress));
    Write32(result.code, anchors.at("nativeHidHook"), ArmBranch(nativeHookAddress, circlePadPro ? accessoryAddress : nativeContinuation));
    if (circlePadPro) {
        for (const auto &relocation : std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                 {0x5c, nativeContinuation}, {0x84, addressOf("accessoryStatus")}, {0x8c, addressOf("accessoryStop")},
                 {0x90, addressOf("accessoryStreaming")}, {0xa4, addressOf("accessoryStart")},
                 {0x12c, CheckedAdd(addressOf("accessoryShutdown"), 4)}}) {
            RelocateUpdateBranch(accessory, accessoryAddress, relocation.first, relocation.second);
        }

        Write32(accessory, 0x130, bssEnd);
        Write32(accessory, 0x134, CheckedAdd(accessoryAddress, 0xe0));
        Write32(accessory, 0x138, CheckedAdd(accessoryAddress, 0x60));
        Write32(accessory, 0x13c, addressOf("threadEntry"));
        Write32(accessory, 0x140, CheckedAdd(bssEnd, extraBssSize));
        Write32(accessory, 0x144, initializedAddress);
        std::copy(accessory.begin(), accessory.end(), result.code.begin() + accessoryOffset);
        Write32(result.code, anchors.at("accessoryShutdown"), ArmBranch(addressOf("accessoryShutdown"), CheckedAdd(accessoryAddress, 0xe8)));
        report << "Accessory worker uses signature-resolved SDK entry points. Hardware validation pending.\n";
    } else {
        PatchContainerCancel(code, result.code, controls, textAddress, controlsAddress, updateContainerCancel);
        std::copy(controls.begin(), controls.end(), result.code.begin() + controlsOffset);
        Write32(result.code, anchors.at("inputHook"), ArmBranch(addressOf("inputHook"), controlsAddress));
        report << "Container B cancel uses a fresh press; opening releases cannot close containers.\n";
    }
    std::copy(performance.begin(), performance.end(), result.code.begin() + performanceOffset);
    std::copy(lightLookup.begin(), lightLookup.end(), result.code.begin() + performanceOffset + updatePerformanceSize);
    const auto performanceTarget = [&](std::size_t offset) {
        Require(offset < updatePayloadSize, "Performance symbol is outside its payload");
        return CheckedAdd(performanceAddress, static_cast<std::uint32_t>(offset));
    };
    const auto writePerformanceBranch = [&](std::size_t offset, std::uint32_t destination, bool link) {
        const auto source = CheckedAdd(textAddress, static_cast<std::uint32_t>(offset));
        Write32(result.code, offset, ArmBranch(source, destination) | (link ? 0x01000000U : 0));
    };
    writePerformanceBranch(decompressInitializeOffset, performanceTarget(updateInitializeDecompressionScratchOffset), false);
    writePerformanceBranch(decompressAcquireOffset, performanceTarget(updateAcquireDecompressionScratchOffset), true);
    writePerformanceBranch(decompressReleaseInitOffset, performanceTarget(updateReleaseDecompressionScratchOffset), true);
    writePerformanceBranch(decompressReleaseErrorOffset, performanceTarget(updateReleaseDecompressionScratchOffset), true);
    Write32(result.code, decompressRetainOffset, 0xe320f000);
    writePerformanceBranch(decompressFinishOffset, performanceTarget(updateFinishDecompressionOffset), false);
    writePerformanceBranch(nextIntOffset, performanceTarget(updateNextIntPowerOfTwoOffset), false);
    Write32(result.code, startupPresentationOneOffset, 0x3f000000);
    Write32(result.code, startupPresentationTwoOffset, 0x3f000000);
    std::copy(blockLookup.begin(), blockLookup.end(), result.code.begin() + blockLookupOffset);
    writePerformanceBranch(climateSeedOffset, performanceTarget(updateInitializeFinalClimateCellOffset), true);
    writePerformanceBranch(prepareInflateOffset, performanceTarget(updatePrepareInflateOutputOffset), true);
    writePerformanceBranch(commitInflateOffset, performanceTarget(updateCommitInflateOutputOffset), true);
    writePerformanceBranch(restoreInflateOffset, performanceTarget(updateRestoreInflateTerminatorOffset), true);
    writePerformanceBranch(lightLookupOffset, performanceTarget(updatePerformanceSize), false);
    writePerformanceBranch(inflateInitializeOffset, performanceTarget(updateInitializeOrResetInflateOffset), true);
    Write32(result.code, inflateRetainOffset, 0xe320f000);
    for (const auto offset : inflateCallbackOffsets) {
        Write32(result.code, offset, 0xe320f000);
    }
    Write32(result.code, biomeRemainderOffset, 0xe2001007);
    for (const auto &write : storageWrites) {
        Write32(result.code, write.first, write.second);
    }

    const auto overlay = FromHex(updateOverlayHex);
    Require(overlay.size() <= updateOverlayCapacity && Sha256(overlay) == updateOverlayHash &&
            updateOverlayVblankAddress >= textAddress + updateOverlayOffset &&
            updateOverlayVblankAddress < textAddress + updateOverlayOffset + overlay.size(),
        "Overlay payload checksum, size or entry changed");
    const auto heapSearch = FromHex(updateHeapSearchHex);
    Require(updateHeapSearchOffset >= updateOverlayOffset + overlay.size() &&
            updateHeapSearchOffset + heapSearch.size() <= updateOverlayOffset + updateOverlayCapacity &&
            heapSearch.size() == updateHeapSearchSize && Sha256(heapSearch) == updateHeapSearchHash,
        "Heap search payload overlaps the overlay or its checked size/checksum changed");
    Require(textAddress + updateHeapSearchOffset == 0x35d5f0 &&
            textAddress + updateHeapSearchHookOffset == 0x123bd8 &&
            Read32(code, updateHeapSearchHookOffset) == 0xe5931000,
        "Heap search linked address or entry instruction changed");
    const auto alignedAllocation = FromHex(updateAlignedAllocationHex);
    Require(updateAlignedAllocationOffset >= updateHeapSearchOffset + heapSearch.size() &&
            updateAlignedAllocationOffset + alignedAllocation.size() <= updateOverlayOffset + updateOverlayCapacity &&
            alignedAllocation.size() == updateAlignedAllocationSize && Sha256(alignedAllocation) == updateAlignedAllocationHash,
        "Aligned allocation payload overlaps another payload or its checked size/checksum changed");
    Require(textAddress + updateAlignedAllocationOffset == 0x35d658 &&
            textAddress + updateAlignedAllocationHookOffset == 0x112574 &&
            Read32(code, updateAlignedAllocationHookOffset) == 0x0a00003d,
        "Aligned allocation linked address or entry instruction changed");
    std::fill_n(result.code.begin() + updateOverlayOffset, updateOverlayCapacity, std::uint8_t{0});
    if (enableOverlay) {
        std::copy(overlay.begin(), overlay.end(), result.code.begin() + updateOverlayOffset);
        Write32(result.code, updateOverlayGateOffset, 0xe320f000);
        Write32(result.code, updateOverlayVblankHookOffset, updateOverlayVblankAddress);
    } else {
        //Keep the retired renderer inert while retaining the adjacent heap helpers.
        Write32(result.code, updateOverlayOffset, 0xe12fff1e);
        Write32(result.code, updateOverlayGateOffset, 0xea000005);
    }

    std::copy(heapSearch.begin(), heapSearch.end(), result.code.begin() + updateHeapSearchOffset);
    std::copy(alignedAllocation.begin(), alignedAllocation.end(), result.code.begin() + updateAlignedAllocationOffset);
    Write32(result.code, updateHeapSearchHookOffset, ArmBranch(0x123bd8, 0x35d5f0));
    Write32(result.code, updateAlignedAllocationHookOffset, ArmBranch(0x112574, 0x35d658));

    Write32(result.code, startupOffset, CheckedAdd(bssEnd, extraBssSize));
    Write32(result.exheader, 0x18, caveEnd);
    Write32(result.exheader, 0x3c, CheckedAdd(bssSize, extraBssSize));
    result.exheader[0x20c] = 0;
    result.exheader[0x20d] = 0;
    result.exheader[0x20e] = 0x34;
    Write32(result.exheader, 0x394, 0xff000101);
    Write32(result.icon, 0x2028, 0x1c1);
    const auto outputHash = Sha256(result.code);
    const auto &standardHash = circlePadPro ? updateCirclePadProCodeHash : updateLCirclePadCodeHash;
    const auto &overlayHash = circlePadPro ? updateCirclePadProOverlayCodeHash : updateLCirclePadOverlayCodeHash;
    const auto &expectedHash = enableOverlay ? overlayHash : standardHash;
    Require(!known || outputHash == expectedHash,
        "Update executable did not reproduce the expected control profile: " + outputHash);
    report << "Pickup animations tick and expire with low graphics enabled.\n";
    if (enableOverlay) {
        report << "FPS/debug overlay: enabled (--overlay).\n"
               << "Custom bottom-screen overlay: presented FPS, average/peak frame interval and three arena heaps.\n"
               << "Overlay replaces the stock frame-timer renderer and reuses its palette BSS without segment growth.\n";
    } else {
        report << "FPS/debug overlay: disabled (default); no custom drawing or frame-statistics callback.\n";
    }

    report << "Locked heap free-list search can start at either end; allocation layout and coalescing are unchanged.\n"
           << "Four-byte-aligned allocations skip redundant per-block rounding; selection and commit remain exact.\n"
           << "Chunk streams share one thread-safe 16 KiB scratch buffer per decompression call.\n"
           << "Scratch ownership no longer aliases z_stream.total_in (v0.4.5 regression fixed).\n"
           << "Block lookup uses the cached chunk directly and retains the original cache-miss resolver.\n"
           << "The deterministic climate edge pass initializes only the final cell seed.\n"
           << "Record loading copies compressed slices directly into their destination strings.\n"
           << "Inflate writes into unshared spare string capacity when a full 16 KiB window fits.\n"
           << "Shared or smaller output buffers retain the original scratch-and-append path.\n"
           << "Inflate state and history allocation are reset between streams and freed once per call.\n"
           << "Raw chunk light reads use a direct path, retaining invalid-coordinate assertions.\n"
           << "Biome color randomness uses exact modulo-eight masking; color refreshes remain intact.\n"
           << "CLayer::NextInt uses exact masking for positive power-of-two bounds.\n"
           << "Both five-second startup presentation phases use half-second durations.\n"
           << "Patched payload end: code+0x" << HexNumber(caveEnd) << "\n"
           << "Performance payload follows both bootstrap LayeredFS injection windows.\n"
           << "Patched executable SHA-256: " << outputHash << "\n"
           << "Replacement targets the matching installed update.\n";
    result.report = report.str();

    return result;
}
