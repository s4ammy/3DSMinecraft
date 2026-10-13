#include "Patches.h"

#include <iostream>

int main(int argc, char **argv) {
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: buildRuntimeFixture <stock fixture> <output directory> [overlay]\n";
        return 2;
    }

    try {
        const auto input = std::filesystem::path(argv[1]);
        const auto output = std::filesystem::path(argv[2]);
        const auto code = Mc3ds::ReadFile(input / "exefs/code.bin");
        const auto result = Mc3ds::PatchGame(code, Mc3ds::ReadFile(input / "exheader.bin"),
            Mc3ds::ReadFile(input / "exefs/icon.bin"), false, Mc3ds::EControlMode::L_CIRCLE_PAD, argc == 4);
        std::filesystem::create_directories(output);
        Mc3ds::WriteFile(output / "code.bin", result.code);
        Mc3ds::WriteFile(output / "code.ips", Mc3ds::CreateIpsPatch(code, result.code, result.ipsOverwriteRanges));
        Mc3ds::WriteFile(output / "exheader.bin", result.exheader);
        std::cout << result.report;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
