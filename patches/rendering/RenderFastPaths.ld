OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)

NativeCeilFloat = 0x74f1b0;
NativeResolveSprite = 0x702174;
NativeInterpolateU = 0x7095a4;
NativeInterpolateV = 0x709630;

SECTIONS {
    .vertex 0x1b0f68 : { KEEP(*(.text.AppendColorUv)) }
    .spriteU 0x709544 : { KEEP(*(.text.SpriteCoordinateU)) }
    .spriteV 0x7095d0 : { KEEP(*(.text.SpriteCoordinateV)) }
    ASSERT(SIZEOF(.vertex) <= 0xc4, "Vertex writer exceeds original body and literal")
    ASSERT(SIZEOF(.spriteU) <= 0x60, "U wrapper overlaps native interpolation")
    ASSERT(SIZEOF(.spriteV) <= 0x60, "V wrapper overlaps native interpolation")
    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.extab*) *(.comment*) *(.note*) }
}
