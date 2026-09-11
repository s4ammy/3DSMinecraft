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
        EControlMode controlMode = EControlMode::L_CIRCLE_PAD;
        bool enableOverlay = false;
        bool allowSimilar = false;
        bool dryRun = false;
    };

    void RunPatcher(const TOptions &options);
}
