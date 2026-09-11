#pragma once

#include "Bytes.h"

namespace Mc3ds {
    inline constexpr std::size_t updatePerformanceSize = 0x1d4;
    inline constexpr std::size_t updateLightLookupSize = 0xc4;
    inline constexpr std::size_t updatePayloadSize = 0x298;

    inline constexpr std::size_t updateHeapSearchOffset = 0x25d5f0;
    inline constexpr std::size_t updateHeapSearchHookOffset = 0x23bd8;
    inline constexpr std::size_t updateHeapSearchSize = 0x44;

    inline constexpr std::size_t updateAlignedAllocationOffset = 0x25d658;
    inline constexpr std::size_t updateAlignedAllocationHookOffset = 0x12574;
    inline constexpr std::size_t updateAlignedAllocationSize = 0x48;

    inline constexpr std::size_t updateAcquireDecompressionScratchOffset = 0x20;
    inline constexpr std::size_t updateCommitInflateOutputOffset = 0x180;
    inline constexpr std::size_t updateFinishDecompressionOffset = 0x84;
    inline constexpr std::size_t updateInitializeDecompressionScratchOffset = 0x0;
    inline constexpr std::size_t updateInitializeFinalClimateCellOffset = 0x110;
    inline constexpr std::size_t updateInitializeOrResetInflateOffset = 0xf8;
    inline constexpr std::size_t updateNextIntPowerOfTwoOffset = 0x98;
    inline constexpr std::size_t updatePerformancePayloadStartOffset = 0x0;
    inline constexpr std::size_t updatePrepareInflateOutputOffset = 0x130;
    inline constexpr std::size_t updateReleaseDecompressionScratchOffset = 0x44;
    inline constexpr std::size_t updateRestoreInflateTerminatorOffset = 0x1b4;

    inline const std::string updatePerformanceHash = "b226211ee2e164c1926195510703c3ec8fc52afeffefe1e03e5e1d813455319e";
    inline const std::string updatePerformanceHex =
        "4cd04de20000a0e348008de52c008de530008de534008de538008de5bcf09fe548209de5000052e30200a0111eff2f11"
        "10402de9a8c09fe53cff2fe150008de51080bde810402de934009de5000050e30200000a18008de288c09fe53cff2fe1"
        "50409de5000054e30400000a0400a0e174c09fe53cff2fe10000a0e350008de51080bde8eeffffeb4cd08de20100a0e3"
        "028bbdecf08fbde8d041c7e1000056e3060000da012046e2020016e10300001a240ca0e1050480e1022000e02cf09fe5"
        "0620a0e1c63fa0e124cca0e1451ca0e105048ce118c09fe53cff2fe10cf09fe524dd3d00e04d1200203b7c0060ea2f00"
        "34335c00c45e12001c3090e5000053e300f09f0500f09fe56c3b7c00c43c7c0001c084e206005ce11eff2f111cc09de5"
        "01c04ce20c0055e11eff2f1198f09fe50030a0e30c308de5200882e8003099e50cc013e501c08ce201005ce31eff2f81"
        "08c013e5041013e501005ce10500003a01c04ce00b005ce10200003a013083e00c308de5003082e50010a0e31eff2fe1"
        "0cc09de500005ce30800000a00c09de500005ce30500000a0c1081e0003090e5041003e50020a0e30120c3e71eff2fe1"
        "18f09fe50c009de5000050e30010a0130010c01510008de21eff2fe1cc315c0021111000";

    inline const std::string updateBlockLookupHash = "ddf49008a99bd2880a7c6e382d15a93d57dd8dffe468502acd0cfae9f0f600af";
    inline const std::string updateBlockLookupHex =
        "f0482de908d04de2044092e5000054e32c00004af831d1e1030054e1290000aa003092e50270a0e1082092e54352a0e1"
        "4232a0e1342091e500508de504308de5000052e30400000a206092e5050056e124609205030056010900000a7c309fe5"
        "0d20a0e10050a0e10100a0e10210a0e133ff2fe10020a0e10500a0e1000052e31000000a7c1092e5240251e10d00009a"
        "2412a0e1011182e05c1091e5000051e30800000a002097e5083097e50f2002e2021481e00f2003e2021281e00f2004e2"
        "0210d1e7010000ea14109fe50010d1e50010c0e50100a0e108d08de2f088bde83cab17002447a300";

    inline const std::string updateLightLookupHash = "e8cb5e8e15933c3e967a9e88417e8c14e825afc6149db1ce92f09fd2e676e4e5";
    inline const std::string updateLightLookupHex =
        "70402de908d04de20040a0e1f200d2e10150a0e17c1091e5400251e10e00009a0010d2e50120d2e54062a0e10f0051e3"
        "100052930f00003a7000ffe60510cde574109fe50f0000e20600cde505008de20720cde531ff2fe1090000ea440bd5e5"
        "0000c4e5450bd5e50100c4e50400a0e108d08de27080bde80f0000e2010480e1020280e1061185e0062ba0e35c1091e5"
        "021091e724209fe5000051e3002081100010d2e50f0001e22112a0e10100c4e50010c4e508d08de27080bde82c3d6e00"
        "b89b9800";

    inline const std::string updateHeapSearchHash = "cf7d366e45f9fa58600467ea877061af8c0edbf82af6927914cd7a5c1b28f1e9";
    inline const std::string updateHeapSearchHex =
        "001093e5000051e30c00000a040093e5000052e18b19f78a01c040e0acc081e00c0052e10500003a0010a0e1080091e5"
        "000052e10010a091fbffff9a7419f7ea6919f7ea";

    inline const std::string updateAlignedAllocationHash = "80f4d643cafd30b2d126a4c0072051ca8d6c98bcbd204292575362dfbc055e1a";
    inline const std::string updateAlignedAllocationHex =
        "04d4f60a040052e3c4d3f61a04e09ce503005ee10e0056210600009a0c10a0e10e60a0e110808ce2000057e3cfd3f61a"
        "03005ee1cdd3f60a0cc09ce500005ce3f1ffff1ac9d3f6ea";

    struct TPerformanceGuard {
        const char *name;
        std::size_t offset;
        std::size_t size;
        const char *hash;
    };

    inline const TPerformanceGuard updatePerformanceGuards[] = {
        {"block getter", 0x74648, 0x144, "c75af723a4e914a80216f713e3eb7c17d12bc571a1e684f65d4d57b1a5a9ad17"},
        {"chunk cache resolver", 0x7ab3c, 0x78, "1dd087a740cc6e79315e7ee75072dd42707fd7a0a03d8615a25adea2c0b834cf"},
        {"climate edge pass", 0xba7d8, 0x25c, "c2715b4843a9729b7c147d957afc80a3ffefcc88c405971b13319aff99eef2d6"},
        {"chunk record reader", 0xbeb80, 0x23c, "699caaa7c479d0c12356f7bc73a2fd27d6fb9bafefe68a15a8c1edd440b6314f"},
        {"chunk decompressor", 0x2ddd08, 0x220, "33378ccb3e97af688bdaa4bc03f07378aceba20c69ab9995bf3b2370e5532291"},
        {"shared string replacement", 0x1120, 0x174, "1811d15d4bb82fd29a3ebbccf9c5f9d017acac3bc666d0fc5ef8f88dd2210c7a"},
        {"shared string allocation", 0x1feb28, 0x76, "5526a22539e77cb004fb90c2ce6b331fbaf810e4e6e31508460455a82d05763d"},
        {"zlib stream layout", 0x6c3cc4, 0xa8, "d90d4cb4f9dba2bdd06617617d21f3880f448e7320ef52e6a452b2e93f76f781"},
        {"chunk light getter", 0x57240, 0xac, "64b525ac0f5c17c92cee700c25f86ba885dc430dee96f13ec4bcb30a931a000a"},
        {"subchunk index", 0x5e3d2c, 0x7c, "6f20a176c25b524dd99f5e82a1e32725444af7dfbe5c2e3ef64dc6d252193445"},
        {"packed light getter", 0x6346dc, 0x20, "a8437476eedbb4f95df55b2d7a4feb7807931cf84f95251909a24461aca0b63a"},
        {"biome color pass", 0xc7ac8, 0x49c, "ab48bd7205fa49f54f5174b9ffcf8fb42cf3199511f249a3834497433533a662"},
        {"inflate init", 0x6c3b6c, 0x158, "d0a1e1633640d2d09b8bd27558f5b4f12f9fc38d96888f89bb6b837fe909b209"},
        {"inflate end", 0x6c3b20, 0x4c, "ddbab90869c02796df280d6e6d15e6ac623b5cd0a975660266c396ef8a9194c8"},
        {"SDK heap free and coalescing", 0x23b98, 0x138, "210cb801ae63df156b246f19d165e84c16ef782b6f2fe37f4e44ed1fd264c4aa"},
        {"locked arena free wrapper", 0x1bbf4, 0x20, "69565c014361ad92c5f21bc788beaf7a49b1967e02e3a6c67898abdfc7c54e33"},
        {"SDK aligned allocation", 0x12530, 0x148, "a9b191222984b002e0f7bf790f7234a952e31381527b258dcb563f0a9cbf9878"},
        {"SDK split and commit", 0x1bcd4, 0x1dc, "fecd0524ed100337e376226b7ab24c945e758270332b72860363f35752733dd1"},
        {"arena allocation options", 0xba88, 0x64, "3d12d62d2ca128210e48084e6eab71055bb67cc12304c2e8be6f15579e82eb00"},
        {"four-byte allocation wrapper", 0x1493c, 0x10, "3bfea1d7e57a9333665e4a51519b5f8aacd9500613779a3fd04ac8aa7ccd793e"},
    };
}
