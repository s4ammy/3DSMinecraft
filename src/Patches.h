#pragma once

#include "Signature.h"

namespace Mc3ds {
    inline const std::string testedCiaHash = "ebd865c5656be81ed2faf3122eec5d8c93b069e4ad211f3596b879f9f78abfe3";
    inline const std::string testedCodeHash = "a4e5972ee52adfc316606f6edba7d19228eba0fe9e828d9584acc752c876650b";
    inline const std::string previousPatchedCodeHash = "d484dbb3211d7bc2d48b28bef0a1aba3b38db34a058daaabd74b5c983b791f81";
    inline const std::string patchedCodeHash = "a1c746e9efbf8bd8d26deb5abe33e1944c9cfd579f8233239536d0166e3b5740";
    inline const std::string circlePadProCodeHash = "6aaca1236211154d86f81b23c84fae9913effd48d316a3bb05e02a077c9a71e2";
    inline const std::string updateCiaHash = "8526ef24719d074c1b3de4742e23c795c003ff51d10f7ea99839c6974587a75b";
    inline const std::string updateOriginalCodeHash = "902ccd5a06d59797c52976b2905dc56a5ca0a868021432fcc7eb127ba3320d65";
    inline const std::string updateLCirclePadCodeHash = "805180e1554c29de079dd3374c99cac5ebba1a0a04d3cffb1c0602395912b04f";
    inline const std::string updateCirclePadProCodeHash = "65fd0645adcd8abe6cb595a3328efc104f0255fdcaa16c7fe472fb187afc8038";
    inline constexpr std::uint32_t lumaLayeredFsPayloadSize = 0x114;

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
}
