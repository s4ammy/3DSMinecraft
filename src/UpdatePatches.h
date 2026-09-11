#pragma once

#include "Patches.h"

namespace Mc3ds {
    TPatchResult PatchUpdateGame(const TBytes &code, const TBytes &exheader, const TBytes &icon, bool allowSimilar,
        EControlMode controlMode, bool enableOverlay = false);
}
