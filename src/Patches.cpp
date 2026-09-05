#include "Patches.h"
#include "SignatureData.h"

#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace Mc3ds {
    void Require(bool condition, const std::string &message);
    std::uint32_t CheckedAdd(std::uint32_t value, std::uint32_t amount);
    std::uint32_t RoundPage(std::uint32_t value);
    std::string NormalizedSystemProfile(TBytes exheader);
}

void Mc3ds::Require(bool condition, const std::string &message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::uint32_t Mc3ds::CheckedAdd(std::uint32_t value, std::uint32_t amount) {
    Require(value <= std::numeric_limits<std::uint32_t>::max() - amount, "Address addition overflows");

    return value + amount;
}

std::uint32_t Mc3ds::RoundPage(std::uint32_t value) {
    return CheckedAdd(value, 0xfff) & ~0xfffU;
}

std::string Mc3ds::NormalizedSystemProfile(TBytes exheader) {
    Require(exheader.size() == 0x800, "Expected a complete 0x800-byte extended header");
    exheader.resize(0x400);
    exheader[0xd] &= 0xfe;
    for (const auto &range : std::vector<std::pair<std::size_t, std::size_t>>{
             {0x10, 0x40}, {0x1c8, 0x1d0}, {0x200, 0x208}, {0x230, 0x238}}) {
        std::fill(exheader.begin() + static_cast<std::ptrdiff_t>(range.first),
            exheader.begin() + static_cast<std::ptrdiff_t>(range.second), 0);
    }

    return Sha256(exheader);
}

Mc3ds::TPatchResult Mc3ds::PatchGame(const TBytes &code, const TBytes &exheader, const TBytes &icon, bool allowSimilar) {
    const auto inputHash = Sha256(code);
    Require(inputHash != patchedCodeHash, "This executable is already patched");

    const auto known = inputHash == testedCodeHash;
    Require(known || allowSimilar, "Unrecognized executable SHA-256: " + inputHash + ". Use --allow-similar to try the guarded signature profile; compatibility is not guaranteed.");
    Require(NormalizedSystemProfile(exheader) == systemProfileHash,
        "Extended-header capabilities or dependencies differ from the supported profile");
    Require(Read32(exheader, 0x1c) == 0x40000, "Unsupported stack size");

    const auto textAddress = Read32(exheader, 0x10);
    const auto textSize = Read32(exheader, 0x18);
    const auto textPages = Read32(exheader, 0x14);
    const auto roAddress = Read32(exheader, 0x20);
    const auto roSize = Read32(exheader, 0x28);
    const auto dataAddress = Read32(exheader, 0x30);
    const auto dataSize = Read32(exheader, 0x38);
    const auto bssSize = Read32(exheader, 0x3c);

    Require(textAddress == 0x100000 && textSize >= 0x1000 && textSize < 0x2000000,
        "Unsupported executable address or size");
    Require(textPages == RoundPage(textSize) / 0x1000 && roAddress == CheckedAdd(textAddress, RoundPage(textSize)),
        "Unexpected text segment layout");
    Require(Read32(exheader, 0x24) == RoundPage(roSize) / 0x1000 &&
            dataAddress == CheckedAdd(roAddress, RoundPage(roSize)),
        "Unexpected read-only segment layout");
    Require(Read32(exheader, 0x34) == RoundPage(dataSize) / 0x1000 &&
            code.size() == static_cast<std::uint64_t>(dataAddress) - textAddress + RoundPage(dataSize),
        "Executable size does not match its segment metadata");

    const auto bssStart = CheckedAdd(dataAddress, dataSize);
    const auto bssEnd = CheckedAdd(bssStart, bssSize);
    Require((bssEnd & 3) == 0 && RoundPage(CheckedAdd(dataSize, bssSize)) == RoundPage(CheckedAdd(CheckedAdd(dataSize, bssSize), 16)),
        "Input latch would need another data page");

    const auto cave = CheckedAdd(textSize, 3) & ~3U;
    const auto controlsOffset = CheckedAdd(cave, 0x84);
    const auto caveEnd = CheckedAdd(controlsOffset, 0x100);
    Require(caveEnd <= RoundPage(textSize), "No room for the input hook in text padding");
    RequireRange(code.size(), textSize, RoundPage(textSize) - textSize);
    Require(std::all_of(code.begin() + textSize, code.begin() + RoundPage(textSize),
                [](auto value) {
                    return value == 0;
                }),
        "Text padding is occupied; refusing to overwrite it");
    Require(icon.size() == 0x36c0 && std::string(icon.begin(), icon.begin() + 4) == "SMDH", "Invalid icon format");
    Require(Read32(icon, 0x2028) == 0x11c1, "Unexpected SMDH platform flags");

    struct TLocatedPatch {
        const TPatchSpec *spec;
        std::size_t offset;
    };

    auto locations = std::vector<TLocatedPatch>{};
    auto chunkReferences = std::set<std::size_t>{};
    auto chunkBase = std::uint32_t{0};
    auto hookOffset = std::size_t{0};
    auto startupEndOffset = std::size_t{0};
    auto claimed = std::set<std::size_t>{};
    for (const auto &spec : patchSpecs) {
        const auto signature = TSignature{spec.name, FromHex(spec.signatureHex), FromHex(spec.maskHex), spec.targetOffset};
        const auto offset = FindUnique(code, signature);
        const auto before = FromHex(spec.beforeHex);
        const auto mask = FromHex(spec.beforeMaskHex);
        Require(before.size() == mask.size(), "Invalid expected-byte mask");
        RequireRange(code.size(), offset, before.size());
        for (auto index = std::size_t{0}; index < before.size(); ++index) {
            Require((code[offset + index] & mask[index]) == (before[index] & mask[index]),
                "Unexpected original bytes for " + spec.name);
            Require(claimed.insert(offset + index).second, "Patch targets overlap");
        }

        if (spec.kind == EWriteKind::CHUNK_BASE) {
            const auto value = Read32(code, offset);
            Require(chunkBase == 0 || value == chunkBase, "Chunk consumers disagree on the table base");
            chunkBase = value;
            chunkReferences.insert(offset);
        } else if (spec.kind == EWriteKind::INPUT_HOOK) {
            hookOffset = offset;
        } else if (spec.kind == EWriteKind::BSS_END) {
            startupEndOffset = offset;
            Require(offset >= 4 && Read32(code, offset - 4) == bssStart && Read32(code, offset) == bssEnd,
                "Startup BSS clearing does not match the extended header");
        }

        if (offset >= textSize) {
            Require(spec.name.find("renderDistanceOptions_") == 0 && offset >= roAddress - textAddress &&
                    offset + before.size() <= static_cast<std::uint64_t>(roAddress) - textAddress + roSize,
                "A code signature resolved outside the executable text");
        }

        locations.push_back({&spec, offset});
    }

    Require(hookOffset != 0 && startupEndOffset != 0 && chunkReferences.size() == 6, "Incomplete patch profile");
    Require(chunkBase >= bssStart && static_cast<std::uint64_t>(chunkBase) + 0x200 <= bssEnd,
        "Chunk table is not in the expected BSS allocation");

    auto allChunkReferences = std::set<std::size_t>{};
    for (auto offset = std::size_t{0}; offset + 4 <= code.size(); offset += 4) {
        if (Read32(code, offset) == chunkBase) {
            allChunkReferences.insert(offset);
        }
    }

    Require(allChunkReferences == chunkReferences, "Unrecognized chunk-table references; refusing a partial patch");

    auto result = TPatchResult{code, exheader, icon, {}, known};
    auto report = std::ostringstream{};
    report << "Executable SHA-256: " << inputHash << "\n";
    report << "Profile: " << (known ? "tested executable" : "experimental signature match") << "\n";
    for (const auto &location : locations) {
        const auto &spec = *location.spec;
        const auto offset = location.offset;
        if (spec.kind == EWriteKind::CHUNK_BASE) {
            Write32(result.code, offset, CheckedAdd(chunkBase, 24));
        } else if (spec.kind == EWriteKind::BSS_END) {
            Write32(result.code, offset, CheckedAdd(bssEnd, 16));
        } else if (spec.kind == EWriteKind::INPUT_HOOK) {
            Write32(result.code, offset, ArmBranch(CheckedAdd(textAddress, static_cast<std::uint32_t>(offset)), CheckedAdd(textAddress, controlsOffset)));
        } else if (spec.kind == EWriteKind::NATIVE_HID) {
            const auto source = CheckedAdd(textAddress, static_cast<std::uint32_t>(offset));
            Write32(result.code, offset, ArmBranch(source, CheckedAdd(source, 0x38)));
        } else {
            const auto after = FromHex(spec.afterHex);
            Require(after.size() == FromHex(spec.beforeHex).size(), "Patch must preserve instruction size");
            std::copy(after.begin(), after.end(), result.code.begin() + static_cast<std::ptrdiff_t>(offset));
        }

        report << spec.name << " at code+0x" << HexNumber(offset) << "\n";
    }

    auto legacy = FromHex(legacyCameraPayloadHex);
    auto controls = FromHex(controlsPayloadHex);
    Require(legacy.size() == 0x60 && controls.size() == 0x100, "Invalid input payload size");
    const auto resumeAddress = CheckedAdd(textAddress, static_cast<std::uint32_t>(hookOffset + 4));
    Write32(legacy, 0x5c, ArmBranch(CheckedAdd(textAddress, cave + 0x5c), resumeAddress));
    Write32(controls, 0x98, ArmBranch(CheckedAdd(textAddress, controlsOffset + 0x98), resumeAddress));
    Write32(controls, 0x9c, bssEnd);
    std::copy(legacy.begin(), legacy.end(), result.code.begin() + cave);
    std::copy(controls.begin(), controls.end(), result.code.begin() + controlsOffset);
    Write32(result.exheader, 0x3c, CheckedAdd(bssSize, 16));
    result.exheader[0x20c] = 0;
    result.exheader[0x20d] = 0;
    result.exheader[0x20e] = 0x34;
    Write32(result.exheader, 0x394, 0xff000101);
    Write32(result.icon, 0x2028, 0x1c1);
    const auto outputHash = Sha256(result.code);
    Require(!known || outputHash == patchedCodeHash, "Known executable did not reproduce the hardware-tested code");
    report << "Hook storage: code+0x" << HexNumber(cave) << "\n";
    report << "Input latch: 0x" << HexNumber(bssEnd) << " (16 bytes, no extra page)\n";
    report << "Patched executable SHA-256: " << outputHash << "\n";
    result.report = report.str();

    return result;
}

std::string Mc3ds::MakeRebuildSettings(const std::string &productCode, std::uint64_t titleId) {
    Require(productCode.size() == 10 && productCode.substr(0, 9) == "KTR-P-BD3" &&
            productCode.back() >= 'A' && productCode.back() <= 'Z',
        "Unexpected Minecraft product code");
    Require((titleId >> 32) == 0x00040000 && (titleId & 0xff) == 0, "Only base application titles are supported");

    auto settings = rebuildTemplate;
    const auto replace = [&](const std::string &name, const std::string &value) {
        auto position = std::size_t{0};
        while ((position = settings.find(name, position)) != std::string::npos) {
            settings.replace(position, name.size(), value);
            position += value.size();
        }
    };

    replace("@PRODUCT_CODE@", productCode);
    replace("@UNIQUE_ID@", "0x" + HexNumber((titleId >> 8) & 0xffffff));

    return settings;
}
