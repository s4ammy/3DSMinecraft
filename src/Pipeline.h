#pragma once

#include "Platform.h"
#include "Patches.h"

namespace Mc3ds {
    struct TOptions {
        std::filesystem::path input;
        std::filesystem::path output;
        std::filesystem::path toolsDirectory;
        std::filesystem::path seedDatabase;
        std::filesystem::path seedFile;
        bool allowSimilar = false;
        bool dryRun = false;
    };

    void RunPatcher(const TOptions &options);
}
