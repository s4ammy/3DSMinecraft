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
ControlsSetFloat = 0x6329b8;
ControlsSetBoolean = 0x632644;
ControlsReadTick = 0x4cf728;
ControlsLocalize = 0x11d954;
ControlsMeasureText = 0x71ad28;
ControlsDrawText = 0x55bb84;
controlsPromptResume = 0x471078;
controlsParticlesResume = 0x280250;
controlsWeatherResume = 0x2e6d44;
controlsMapResume = 0x67548c;
controlsTextureResume = 0x249948;
controlsOriginalFaceBranch = 0x30a804;
controlsOriginalFaceCleanup = 0x30abe0;
controlsBiomesById = 0xb0f8f0;
controlsOriginalBiomeBody = 0x17aa40;
ControlsReadPairs = 0x1e0144;
ControlsWritePairs = 0x1dffac;
ControlsAppendStringPair = 0x7e3c40;
ControlsRegisterBinding = 0x8f3474;
ControlsAppendModifiedBinding = 0x8f2fec;
ControlsAppendDictation = 0x8f20d0;
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
NativeClearReferences = 0x8f96e8;
NativeCreateSlider = 0x234d40;
NativeCreateBoolean = 0x234f10;
NativeCreateDropdown = 0x235228;
NativeDropdownSelect = 0x5d90b4;
NativeMapSelection = 0x3dcb78;
NativeUpdateSelection = 0x3dc978;
NativeSetScroll = 0x23faf0;
NativeOriginalRender = 0x236bec;
NativeLockArena = 0x123170;
NativeUnlockArena = 0x1231b0;
NativeEffectiveRenderDistance = 0x6a6980;
NativeSelectCategory = 0x23464c;
NativeOriginalOptionsInitialize = 0x236b50;
NativeOriginalOptionsBack = 0x234ac4;
NativeOriginalMainInitialize = 0x26eda4;
NativeOriginalPauseInitialize = 0x1a182c;
NativeAddMenuButton = 0x1a0ea0;
NativeOpenOptions = 0x23c430;
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
