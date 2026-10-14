.syntax unified
.arch armv6k
.arm
.fpu vfpv2

//Keep native ceil/conversion for unusual UVs; the bounded path avoids four libm calls per vertex.
.section .text.AppendColorUv,"ax",%progbits
.global AppendColorUv
.type AppendColorUv,%function
AppendColorUv:
    push {r4-r8, lr}
    vpush {d8}
    mov r4, r0
    ldr r1, [r4, #60]
    ldr r0, [r4, #128]
    str r0, [r1]
    ldr r0, [r4, #60]
    add r0, r0, #4
    str r0, [r4, #60]
    add r6, r4, #104
    add r7, r6, #16
    vldr s16, uvScale
uvLoop:
    ldr r5, [r4, #60]
    vldr s0, [r6]
    vmul.f32 s0, s0, s16
    vmov r0, s0
    cmp r0, #0x48000000
    bhi uvUnusual
    vcvt.u32.f32 s1, s0
    vcvt.f32.u32 s2, s1
    vcmp.f32 s0, s2
    vmrs APSR_nzcv, fpscr
    vmov r0, s1
    addgt r0, r0, #1
    b uvStore
uvUnusual:
    bl NativeCeilFloat
    vcvt.u32.f32 s0, s0
    vmov r0, s0
uvStore:
    strh r0, [r5]
    ldr r0, [r4, #60]
    add r0, r0, #2
    str r0, [r4, #60]
    add r6, r6, #4
    cmp r6, r7
    bne uvLoop
    vpop {d8}
    pop {r4-r8, pc}
uvScale:
    .float 65535.0
.size AppendColorUv, .-AppendColorUv

//The null-atlas revision is the native sentinel value, not an atlas pointer.
.macro SpriteCoordinate name, tail
.section .text.\name,"ax",%progbits
.global \name
.type \name,%function
\name:
    ldr r1, [r0, #4]
    cmp r1, #0
    ldrne r1, [r1, #136]
    ldreq r1, =0x91ba44
    ldreq r1, [r1]
    ldr r2, [r0, #20]
    cmp r2, #0
    beq 1f
    ldr r3, [r0, #24]
    cmp r3, r1
    bne 1f
    mov r0, r2
    b \tail
1:
    push {r0, r1, r4, lr}
    vpush {d0}
    ldr r1, [r0, #16]
    bl NativeResolveSprite
    vpop {d0}
    pop {r1, r2, r4, lr}
    str r0, [r1, #20]
    str r2, [r1, #24]
    b \tail
.ltorg
.size \name, .-\name
.endm

SpriteCoordinate SpriteCoordinateU, NativeInterpolateU
SpriteCoordinate SpriteCoordinateV, NativeInterpolateV
