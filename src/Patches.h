#pragma once

#include "Signature.h"

namespace Mc3ds {
    inline const std::string testedCiaHash = "ebd865c5656be81ed2faf3122eec5d8c93b069e4ad211f3596b879f9f78abfe3";
    inline const std::string testedCodeHash = "a4e5972ee52adfc316606f6edba7d19228eba0fe9e828d9584acc752c876650b";
    inline const std::string patchedCodeHash = "d484dbb3211d7bc2d48b28bef0a1aba3b38db34a058daaabd74b5c983b791f81";

    enum class EWriteKind {
        BYTES,
        CHUNK_BASE,
        BSS_END,
        INPUT_HOOK,
        NATIVE_HID
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

    TPatchResult PatchGame(const TBytes &code, const TBytes &exheader, const TBytes &icon, bool allowSimilar);
    std::string MakeRebuildSettings(const std::string &productCode, std::uint64_t titleId);
}
