OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)

runtimeControls = 0xb3fc30;
ControlsAllocate = 0x11493c;
ControlsUpdateButtonLabel = 0x5e60e4;
ControlsCreateButton = 0x235eb4;
ControlsAppendReference = 0x8f8db4;
ControlsReleaseReference = 0x8afa58;
ControlsAddWorldRows = 0x2358f0;
ControlsGetOptions = 0x225ec4;
ControlsReadPairs = 0x1e0144;
ControlsWritePairs = 0x1dffac;
ControlsAppendStringPair = 0x7e3c40;
ControlsRegisterBinding = 0x8f3474;
ControlsAppendModifiedBinding = 0x8f2fec;
ControlsAccessoryStatus = 0x112e94;
ControlsAccessoryStreaming = 0x11bfbc;
ControlsAccessoryStart = 0x112da8;
ControlsAccessoryStop = 0x112d68;
controlsAccessoryInitialized = 0xa35742;
controlsThreadEntry = 0x1215cc;
controlsInputResume = 0x108124;
controlsHudDestroyResume = 0x679348;
controlsControllerConnected = 0x10b6f4;
controlsShutdownResume = 0x10bcd8;
controlsCategoryResume = 0x2362d0;
controlsDestructorResume = 0x236e10;
controlsReleaseResume = 0x23659c;
TouchGetCurrentScreen = 0x12d544;
TouchRenderBottomHud = 0x6762bc;
TouchSetGuiColor = 0x1a9948;
TouchBegin = 0x1b1bf0;
TouchColor = 0x1b1f00;
TouchVertex = 0x1b2444;
TouchDraw = 0x1b1904;
touchTessellator = 0xac42d8;
touchGuiMaterial = 0xabfc98;
touchGuiColorState = 0xb2dd40;
touchWhiteColor = 0xb2e4b0;

/* The builder packs whole compiler sections into these guarded reservations. */
SECTIONS {
    @SECTIONS@
    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.extab*) *(.comment*) *(.note*) }
}
