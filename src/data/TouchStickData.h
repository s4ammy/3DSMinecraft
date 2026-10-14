#pragma once

#include "core/Bytes.h"

namespace Mc3ds {
    inline constexpr std::uint32_t updateTouchInputHookAddress = 0x918824;
    inline constexpr std::uint32_t updateTouchHudDestroyHookAddress = 0x918848;
    inline constexpr std::uint32_t updateDrawTouchStickAddress = 0x35c7dc;
    inline constexpr std::size_t updateTouchMainOffset = 0x25c750;
    inline const std::string updateTouchMainHash = "8e655b312a2a979de7a41813970ce22e5a959663e96d3bcbbad9d442bf159c47";
    inline const std::string updateTouchMainHex =
        "fc472de90040a0e18a00a0e3010b80e370a09fe5f09094e18800a0e3010b80e30a60a0e1f07094e1040096e40180a0e1"
        "8c54d4e56d0000eb000055e30150a01300008de50600a0e10710a0e10920a0e10530a0e1730000eb030058e30400001a"
        "b420dae1591f84e2b020c1e1b620dae1b220c1e1000050e30000a0139404c4158c04c415fc87bde830fcb30070402de9"
        "140b2ded0040a0e1b3660ceb14619fe50400a0e1004086e5500000eb000050e33e00000a00019fe530108de23c0090e8"
        "f8009fe53c0081e83c0090e8f0009fe520108de23c0081e83c0090e810108de23c0081e8dc209fe50000a0e30d10a0e1"
        "100050e30500000a005082e0000a95ed003081e0040080e2000a83edf7ffffeab0409fe5b0109fe50400a0e13534f9eb"
        "a8509fe5121ea0e30500a0e1db54f9eb278a9fed279a9fed271a9fed30008de2480ab0ee490af0ee29f016eb231a9fed"
        "480ab0ee490af0ee20008de224f016ebf400d6e11e1a9fed100a00eef600d6e1c00ab8ee018a00ee100a00ee10008de2"
        "c00ab8ee419a00ee161a9fed480ab0ee490af0ee16f016eb4c109fe50500a0e10354f9eb0d10a0e10400a0e11134f9eb"
        "140bbdec7080bde830fcb300a0ca3500c0ca3500b0ca350040ddb200b0e4b200d842ac000000d0420000204300005042"
        "00004842cdcccc3e0000204198fcab0030482de90040a0e3000050e30700000a0050a0e1040090e5f942f7eb050050e1"
        "0200001ab80095e5100f6fe1a042a0e10400a0e13088bde8681041e200c0a0e3101a00eea01062e200209de5101a01ee"
        "b2c0c0e1b0c0c0e1000053e3c00ab8eec11ab8ee012a21ee002a00ee0b00000a0410d0e5010011e30800001a313a9fed"
        "0010a0e30610c0e5432ab4ee10faf1ee0110a093011002e00510c0e5000000ea0510d0e5000053e30430c0e52000000a"
        "000052e30120a0030620c005010011e31e00000a0620d0e5000052e31b00001a1d3a9fed432ab4ee10faf1ee170000da"
        "c22ab1ee1c5a9fed183a9fed184a9fed452ab4ee10faf1ee033a32ee043a23ee144a9fed434ab04e011a24ee000a24ee"
        "021a81ee020a80eec11abdeec00abdee102a11eeb220c0e1102a10eeb020c0e1020000ea0020a0e30620c0e50520c0e5"
        "010001e21eff2fe100002945000044420000e0c0421a24400000c8420000384285eb513fae47613f1f856b3f6666263f"
        "6666663f3333733f0000803f9a99593f0ad7a33d8fc2f53dec51383e3d0ad73e";
    inline constexpr std::size_t updateTouchHooksOffset = 0x818824;
    inline const std::string updateTouchHooksHash = "74926c0e8d93686ef2a9e509b6812da35cd8503e45c4e049046e1b2c39ffe91d";
    inline const std::string updateTouchHooksHex =
        "0f502de9100b2ded0500a0e10810a0e1c50fe9eb100bbdec0f50bde8140095e536bedfea0e402de918109fe5002091e5"
        "000052e10020a003002081050e40bde8f0412de9b682f5ea30fcb300";
    inline constexpr std::size_t updateTouchDiskOffset = 0x818944;
    inline const std::string updateTouchDiskHash = "293267bd61c8b01d80544d59ed87315b300140a328e888161108e7e2ab4fa84f";
    inline const std::string updateTouchDiskHex =
        "30482de9108b2dedac409fe50010a0e141bab0ee0400a0e1608ab0ee409ab0ee6565e2eb25aa9fed25ca9fed25da9fed"
        "25ea9fed4afab0ee1850a0e3000055e31b00000a490ab0ee480af0ee4a1ab0ee0400a0e1a966e2eb0b0a39ee0f0a78ee"
        "4a1ab0ee0400a0e1a466e2eb0c8a6fee4a1ab0ee0400a0e10d8a4bee0eba2bee280a39ee0dba0fee0b0a78ee9b66e2eb"
        "490ab0ee480af0ee4a1ab0ee0400a0e19666e2eb4bfab0ee015045e268bab0eee1ffffea108bbdec3088bde8d842ac00"
        "00000000ee83843eea46773fee8384be";

    struct TTouchGuard {
        const char *name;
        std::size_t offset;
        std::size_t size;
        const char *hash;
    };

    inline const TTouchGuard updateTouchGuards[] = {
        {"retired frame-timer quad and label helpers", 0x25c74c, 0x390, "d595cde72bbe8ab90688e741dce95e15013f7f50b0809a6fc2da230f54116c41"},
        {"controller loop and touch dispatch", 0x7f30, 0x3a4, "dd63d2f262afb9b8c18df3356fa108129d07f2346ca7f053d4ebe401cb56f1e2"},
        {"controller sample layout", 0xb4dc, 0x15c, "86889c9ca6087438a6f0cbbdc6a88d88bd42bd67a661801473db40c33db1da03"},
        {"touch sampling", 0x12ce4, 0xc0, "264faa89a8f964920f1520e362c7e3cdd39de4e2ce7ab9cf5a168af62a0eb4d1"},
        {"current client screen", 0x2d544, 0x8, "3221d006e90708ad4dee9720694a6a181ab6cb72e16eda152e6ba6c0e0999c50"},
        {"bottom HUD renderer", 0x5762bc, 0xc78, "96c667dc4f8bbd54287eede87896c90bd8861bf67929399e59189ec706f6c6ed"},
        {"HUD render dispatch", 0x578fd0, 0x1ec, "33e8437678a2cb4e3fb6c2cae96821fb2b9184127b0bb283849820c58533c2b0"},
        {"HUD destructor", 0x579340, 0x3e0, "bfed3e3c288101e4c6caa60e5055b2f8235dedd595746ddd48172dcfd22da341"},
        {"GUI color state", 0xa9948, 0x30, "1811f0841cc868c9b8f6002a4449e06be819dfcc548ef63df49e823532d01f17"},
        {"GUI tessellator begin", 0xb1bf0, 0x218, "39d8ce8f490d8e57ce695f346be0e0eb2f1d42ef4cf9cf76125301a24557a53c"},
        {"GUI tessellator color", 0xb1f00, 0xd4, "fa54ef9afb9ecb5b8cb3ec14e1dafa43f528171c227b80386cfcb27a1511db98"},
        {"GUI tessellator vertex", 0xb2444, 0x3fc, "fb8270e96f4f68aa062647c29b8e2e105fde8a46fdee3df0a12a5b59d7aed162"},
        {"GUI tessellator draw", 0xb1904, 0x7c, "9dac2dd64132c61bbdd82157d399b8031330d99e153ee40512805d2c9c3679ee"},
        {"native solid GUI quad", 0x5cff68, 0xb8, "8df844f6a7b6520f9b814de1b72c140bf9c28e1a08f0467dfbe2158cf2e6e67e"},
    };
}
