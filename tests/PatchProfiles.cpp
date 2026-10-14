#include "Patches.h"
#include "UpdatePatchData.h"
#include "RuntimeControlsData.h"

#include <iostream>
#include <stdexcept>

namespace PatchProfileTests {
    void Require(bool condition, const char *message);
    void CheckUpdate(const std::filesystem::path &fixture);
}

void PatchProfileTests::Require(bool condition, const char *message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void PatchProfileTests::CheckUpdate(const std::filesystem::path &fixture) {
    const auto code = Mc3ds::ReadFile(fixture / "exefs/code.bin");
    const auto header = Mc3ds::ReadFile(fixture / "exheader.bin");
    const auto icon = Mc3ds::ReadFile(fixture / "exefs/icon.bin");
    Require(Mc3ds::Sha256(code) == Mc3ds::updateOriginalCodeHash, "Expected the stock update fixture");
    const auto mode = Mc3ds::EControlMode::L_CIRCLE_PAD;
    for (const auto overlay : {false, true}) {
        const auto result = Mc3ds::PatchGame(code, header, icon, false, mode, overlay);
        const auto ips = Mc3ds::CreateIpsPatch(code, result.code, result.ipsOverwriteRanges);
        Require(Mc3ds::ApplyIpsPatch(code, ips) == result.code, "IPS round trip failed");
        auto legacyControls = code;
        for (const auto &range : result.ipsOverwriteRanges) {
            for (auto index = std::size_t{0}; index < range.size; ++index) {
                legacyControls[range.offset + index] = 0xa5;
            }
        }

        Require(Mc3ds::ApplyIpsPatch(legacyControls, ips) == result.code,
            "IPS left legacy control instructions, literal bytes or physical bindings behind");
        Require(result.knownExecutable, "Fixture should be recognized");
        Require(Mc3ds::ArmBranchTarget(Mc3ds::Read32(result.code, 0xd87a0), 0x1d87a0) ==
                Mc3ds::updateLimitDeferredBuilderPoolAddress,
            "Deferred builders are not bounded");
        Require(Mc3ds::Read32(result.code, 0xd5990) == 0xe3a02000, "Nearby edits still rebuild synchronously");
        Require((Mc3ds::Read32(result.code, 0x6fb48) & 0xff000000U) == 0x9a000000U &&
                Mc3ds::ArmBranchTarget(Mc3ds::Read32(result.code, 0x6fb48), 0x16fb48) == 0x35d634,
            "Late frames do not enter the completion hook");
        Require(Mc3ds::Read32(result.exheader, 0x18) == 0x818e58, "Bootstrap text extent changed");
        const auto bssGrowth = 16U;
        Require(Mc3ds::Read32(result.exheader, 0x3c) == Mc3ds::Read32(header, 0x3c) + bssGrowth, "Unexpected state allocation");
        {
            for (const auto &spec : Mc3ds::updatePatchSpecs) {
                if (spec.kind != Mc3ds::EWriteKind::CONTROL_BINDING) {
                    continue;
                }

                const auto signature = Mc3ds::TSignature{spec.name, Mc3ds::FromHex(spec.signatureHex),
                    Mc3ds::FromHex(spec.maskHex), spec.targetOffset};
                const auto offset = Mc3ds::FindUnique(code, signature);
                Require(Mc3ds::Read32(result.code, offset) == Mc3ds::Read32(code, offset), "Original gameplay binding changed");
            }
        }

        auto rejected = false;
        try {
            Mc3ds::PatchGame(result.code, header, icon, true, mode, overlay);
        } catch (const std::exception &) {
            rejected = true;
        }
        Require(rejected, "Already patched input was accepted");
    }

    auto changed = code;
    changed[0xd5990] ^= 1;
    auto rejected = false;
    try {
        Mc3ds::PatchGame(changed, header, icon, true);
    } catch (const std::exception &) {
        rejected = true;
    }
    Require(rejected, "Changed rebuild ABI passed --allow-similar");
    changed = code;
    changed[0xa398] ^= 1;
    rejected = false;
    try {
        Mc3ds::PatchGame(changed, header, icon, true);
    } catch (const std::exception &) {
        rejected = true;
    }
    Require(rejected, "Changed completion-queue ABI passed --allow-similar");
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: patchProfileTests <private stock update fixture directory>\n";
        return 2;
    }

    try {
        PatchProfileTests::CheckUpdate(argv[1]);
        std::cout << "Unified update and legacy flag passed hashes, IPS round trips, original bindings, layout and rejection checks\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    return 0;
}
