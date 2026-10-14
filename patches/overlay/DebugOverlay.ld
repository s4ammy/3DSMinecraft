SECTIONS {
    . = 0x35cae0;
    .text : {
        *(.text.DrawDebugOverlay)
        *(.text*)
        *(.rodata*)
    }
    .bss (NOLOAD) : { *(.bss*) *(COMMON) }
    .data : { *(.data*) }
    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.extab*) *(.comment*) }
    ASSERT(SIZEOF(.text) <= 0xbd4, "Debug overlay exceeds the replaced stock renderer")
    ASSERT(SIZEOF(.bss) == 0, "Debug overlay must use its checked original BSS slot")
    ASSERT(SIZEOF(.data) == 0, "Debug overlay must not add writable data")
}
