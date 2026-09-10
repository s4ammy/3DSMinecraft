#include "UpdatePatches.h"
#include "UpdatePatchData.h"
#include "SignatureData.h"

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

Mc3ds::TPatchResult Mc3ds::PatchUpdateGame(const TBytes &code, const TBytes &exheader, const TBytes &icon, bool allowSimilar, EControlMode controlMode) {
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
    Require(circlePadPro || RoundPage(CheckedAdd(dataSize, bssSize)) == RoundPage(CheckedAdd(CheckedAdd(dataSize, bssSize), extraBssSize)),
        "Update input latch would require another data page");
    const auto controlsOffset = CheckedAdd(textSize, 3) & ~3U;
    const auto pickupOffset = CheckedAdd(controlsOffset, 0x100);
    const auto accessoryOffset = CheckedAdd(pickupOffset, 0x20);
    Require(CheckedAdd(accessoryOffset, circlePadPro ? 0x14c : 0) <= RoundPage(textSize), "No room for update hooks in text padding");
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

    struct TLocatedUpdatePatch {
        const TPatchSpec *spec;
        std::size_t offset;
    };

    auto locations = std::vector<TLocatedUpdatePatch>{};
    auto chunkReferences = std::set<std::size_t>{initializerLiteral};
    auto claimed = std::set<std::size_t>{};
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
    Require(controls.size() == 0x100 && pickup.size() == 0x20 && accessory.size() == 0x14c, "Invalid update payload sizes");
    const auto controlsAddress = CheckedAdd(textAddress, controlsOffset);
    const auto pickupAddress = CheckedAdd(textAddress, pickupOffset);
    const auto accessoryAddress = CheckedAdd(textAddress, accessoryOffset);
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
        std::copy(controls.begin(), controls.end(), result.code.begin() + controlsOffset);
        Write32(result.code, anchors.at("inputHook"), ArmBranch(addressOf("inputHook"), controlsAddress));
    }

    Write32(result.code, startupOffset, CheckedAdd(bssEnd, extraBssSize));
    Write32(result.exheader, 0x3c, CheckedAdd(bssSize, extraBssSize));
    result.exheader[0x20c] = 0;
    result.exheader[0x20d] = 0;
    result.exheader[0x20e] = 0x34;
    Write32(result.exheader, 0x394, 0xff000101);
    Write32(result.icon, 0x2028, 0x1c1);
    const auto outputHash = Sha256(result.code);
    Require(!known || outputHash == (circlePadPro ? updateCirclePadProCodeHash : updateLCirclePadCodeHash),
        "Update executable did not reproduce the expected control profile");
    report << "Pickup animations tick and expire with low graphics enabled.\n"
           << "Patched executable SHA-256: " << outputHash << "\n"
           << "Install this update alongside the matching patched base game.\n";
    result.report = report.str();

    return result;
}
