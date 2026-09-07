#pragma once

#include "Signature.h"

namespace Mc3ds {
    inline const std::string testedCiaHash = "ebd865c5656be81ed2faf3122eec5d8c93b069e4ad211f3596b879f9f78abfe3";
    inline const std::string testedCodeHash = "a4e5972ee52adfc316606f6edba7d19228eba0fe9e828d9584acc752c876650b";
    inline const std::string previousPatchedCodeHash = "d484dbb3211d7bc2d48b28bef0a1aba3b38db34a058daaabd74b5c983b791f81";
    inline const std::string patchedCodeHash = "c2ed3f1670cab5b461c29758a7bdb0cd0b00dd1105553f2f37cc0a1c1f71af6f";
    inline const std::string circlePadProCodeHash = "18dff470eeb11111dc6ed4ee3edec7c8ea9e6516e994beb5594f7bb9de684264";

    enum class EControlMode {
        L_CIRCLE_PAD,
        CIRCLE_PAD_PRO
    };

    enum class EWriteKind {
        BYTES,
        CHUNK_BASE,
        BSS_END,
        INPUT_HOOK,
        NATIVE_HID,
        PICKUP_HOOK,
        CONTROL_BINDING
    };

    struct TPatchSpec {
        std::string name;
        EWriteKind kind;
        std::string signatureHex;
        std::string maskHex;
        std::size_t targetOffset;
        std::string beforeHex;
        std::string beforeMaskHex;
        std::string afterHex;
    };

    struct TPatchResult {
        TBytes code;
        TBytes exheader;
        TBytes icon;
        std::string report;
        bool knownExecutable;
    };

    EControlMode ParseControlMode(const std::string &name);
    std::string ControlModeName(EControlMode controlMode);
    TPatchResult PatchGame(const TBytes &code, const TBytes &exheader, const TBytes &icon, bool allowSimilar, EControlMode controlMode = EControlMode::L_CIRCLE_PAD);
    std::string MakeRebuildSettings(const std::string &productCode, std::uint64_t titleId);
}
