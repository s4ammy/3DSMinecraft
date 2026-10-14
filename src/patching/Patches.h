#pragma once

#include "core/Signature.h"

namespace Mc3ds {
    inline const std::string testedCiaHash = "ebd865c5656be81ed2faf3122eec5d8c93b069e4ad211f3596b879f9f78abfe3";
    inline const std::string testedCodeHash = "a4e5972ee52adfc316606f6edba7d19228eba0fe9e828d9584acc752c876650b";
    inline const std::string previousPatchedCodeHash = "d484dbb3211d7bc2d48b28bef0a1aba3b38db34a058daaabd74b5c983b791f81";
    inline const std::string patchedCodeHash = "a1c746e9efbf8bd8d26deb5abe33e1944c9cfd579f8233239536d0166e3b5740";
    inline const std::string circlePadProCodeHash = "6aaca1236211154d86f81b23c84fae9913effd48d316a3bb05e02a077c9a71e2";
    inline const std::string updateCiaHash = "8526ef24719d074c1b3de4742e23c795c003ff51d10f7ea99839c6974587a75b";
    inline const std::string updateOriginalCodeHash = "902ccd5a06d59797c52976b2905dc56a5ca0a868021432fcc7eb127ba3320d65";
    inline const std::string updateLCirclePadCodeHash = "77c33974d6957b7d866151d50a5f808beb5e28bbf55f80f459845fa1ca575501";
    inline const std::string updateCirclePadProCodeHash = "3fa92c2bedbe5ce21db9e56f7b49bcac341d04be4b0e27aa44339d038da71daa";
    inline const std::string updateLCirclePadOverlayCodeHash = "39c355ab69a2bd803ad90db24db13a8a9b3689baa8246d89716e6a68d4501d18";
    inline const std::string updateCirclePadProOverlayCodeHash = "2f703fe2b24ec7a89998d95b5eb096feeba110939554f0c5d4f2492ea3d426f7";
    inline const std::string previousUpdateTouchStickCodeHash = "e2622f4eb7ef960a62da06c4893b35449d4e1d685654b699ed3ad9560608a7bd";
    inline const std::string previousUpdateTouchStickOverlayCodeHash = "886e823e5c9e2dd5444db5b359f0f8a8013fc8cb38596f24672a77023a2d69b0";
    inline const std::string updateTouchStickCodeHash = "56253fce9ff61c4e1be81acf552f536fc17c487c984c9201912cb83e683daaae";
    inline const std::string updateTouchStickOverlayCodeHash = "cd07826c69a129bf681e8897cb1b9b6be26bf21b28c1d148a0b13964cb8521ee";
    inline const std::string updateRuntimeControlsCodeHash = "b4a45114e7fbc85b56f500cb26d8d7c454417446ea23a3a1cf593e7cae03f3c0";
    inline const std::string updateRuntimeControlsOverlayCodeHash = updateRuntimeControlsCodeHash;
    inline constexpr std::uint32_t lumaLayeredFsPayloadSize = 0x114;

    enum class EControlMode {
        L_CIRCLE_PAD,
        CIRCLE_PAD_PRO,
        TOUCH_STICK
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
        std::vector<TByteRange> ipsOverwriteRanges;
    };

    EControlMode ParseControlMode(const std::string &name);
    std::string ControlModeName(EControlMode controlMode);
    TPatchResult PatchGame(const TBytes &code, const TBytes &exheader, const TBytes &icon, bool allowSimilar,
        EControlMode controlMode = EControlMode::L_CIRCLE_PAD, bool enableOverlay = false);
}
