OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)

touchStickState = 0xb3fc30;
touchTessellator = 0xac42d8;
touchGuiMaterial = 0xabfc98;
touchGuiColorState = 0xb2dd40;
touchWhiteColor = 0xb2e4b0;
TouchGetCurrentScreen = 0x12d544;
TouchRenderBottomHud = 0x6762bc;
TouchSetGuiColor = 0x1a9948;
TouchBegin = 0x1b1bf0;
TouchColor = 0x1b1f00;
TouchVertex = 0x1b2444;
TouchDraw = 0x1b1904;
touchInputResume = 0x108124;
touchHudDestroyResume = 0x679348;

SECTIONS {
    .touchMain 0x35c750 : {
        *(.text.UpdateTouchStick)
        *(.text.DrawTouchStick)
        *(.text._ZN10TouchStick10IsGameplayEPv)
        *(.text._ZN10TouchStick6Update*)
        *(.rodata*)
    }
    .touchHooks 0x918824 : { *(.touchHooks) }
    .touchDisk 0x918944 : { *(.text._ZN10TouchStick8DrawDisk*) }
    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.extab*) *(.comment*) *(.note*) }
}
ASSERT(SIZEOF(.touchMain) <= 0x38c, "Touch stick overlaps debug overlay")
ASSERT(SIZEOF(.touchHooks) <= 0x100, "Touch hooks overlap pickup payload")
ASSERT(SIZEOF(.touchDisk) <= 0x14c, "Touch drawing overlaps bootstrap LayeredFS reservation")
