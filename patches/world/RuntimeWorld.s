.syntax unified
.arch armv6k
.fpu vfpv2
.arm

.section .text.InitializeRuntimeChunks,"ax",%progbits
.global InitializeRuntimeChunks
.type InitializeRuntimeChunks, %function
InitializeRuntimeChunks:
    push {r4, lr}
    bl InitializeWorldSettings
    pop {r4, lr}
    ldr r0, =0xb16368
    mov r1, #1
    mov r2, #4
    mov r3, #4
    ldr pc, =0x7bc084
.ltorg
.size InitializeRuntimeChunks, . - InitializeRuntimeChunks

.macro ChunkEnd name, result, base, next, shift=0
.section .text.\name,"ax",%progbits
.global \name
.type \name, %function
\name:
    push {r12}
    ldr r12, =0xb16348
    ldr r12, [r12, #16]
    .if \shift
    add \result, \base, r12, lsr #\shift
    .else
    add \result, \base, r12
    .endif
    pop {r12}
    ldr pc, =\next
.ltorg
.size \name, . - \name
.endm

ChunkEnd RuntimeNeighborEnd, r9, r5, 0x1e75a0
ChunkEnd RuntimeVisibleEnd, r0, r7, 0x3c74c0
ChunkEnd RuntimeShuffleEnd, r7, r6, 0x5cde2c
ChunkEnd RuntimeLoadingEnd, r8, r4, 0x607484
ChunkEnd RuntimeStageTotal, r7, r7, 0x1999b0, 2

.macro ChunkThreshold name, value, next
.section .text.\name,"ax",%progbits
.global \name
.type \name, %function
\name:
    push {r12}
    ldr r12, =0xb16348
    ldr r12, [r12, #20]
    cmp \value, r12
    pop {r12}
    ldr pc, =\next
.ltorg
.size \name, . - \name
.endm

ChunkThreshold RuntimeLoadingReady, r0, 0x199bb8
ChunkThreshold RuntimeLocalReady, r6, 0x1e8d80
ChunkThreshold RuntimePeerReady, r0, 0x1e8d90

.section .text.RuntimeProgressDivisor,"ax",%progbits
.global RuntimeProgressDivisor
.type RuntimeProgressDivisor, %function
RuntimeProgressDivisor:
    push {r12}
    ldr r12, =0xb16348
    vldr s1, [r12, #24]
    pop {r12}
    ldr pc, =0x199bb0
.ltorg
.size RuntimeProgressDivisor, . - RuntimeProgressDivisor

.section .text.RuntimeResidentRadius,"ax",%progbits
.global RuntimeResidentRadius
.type RuntimeResidentRadius, %function
RuntimeResidentRadius:
    ldr r2, =0xb16348
    ldr r2, [r2, #4]
    lsl r2, r2, #4
    ldr pc, =0x3266d0
.ltorg
.size RuntimeResidentRadius, . - RuntimeResidentRadius

.section .text.RuntimeMapRadius,"ax",%progbits
.global RuntimeMapRadius
.type RuntimeMapRadius, %function
RuntimeMapRadius:
    ldr r0, =0xb16348
    ldr r0, [r0, #8]
    ldr pc, =0x72c49c
.ltorg
.size RuntimeMapRadius, . - RuntimeMapRadius

.section .text.RuntimeRenderRadius,"ax",%progbits
.global RuntimeRenderRadius
.type RuntimeRenderRadius, %function
RuntimeRenderRadius:
    ldr r1, =0xb16348
    ldm r1, {r1, r12}
    cmp r1, r12
    movhi r1, r12
    sub r12, r12, #1
    cmp r1, r12
    movlo r1, r12
    push {r4, r5, lr}
    ldr pc, =0x1971dc
.ltorg
.size RuntimeRenderRadius, . - RuntimeRenderRadius

.section .text.RuntimeInitialRenderRadius,"ax",%progbits
.global RuntimeInitialRenderRadius
.type RuntimeInitialRenderRadius, %function
RuntimeInitialRenderRadius:
    push {r1, r2}
    mrs r2, cpsr
    ldr r0, =0xb16348
    ldm r0, {r0, r1}
    cmp r0, r1
    movhi r0, r1
    sub r1, r1, #1
    cmp r0, r1
    movlo r0, r1
    msr cpsr_f, r2
    pop {r1, r2}
    ldr pc, =0x19a064
.ltorg
.size RuntimeInitialRenderRadius, . - RuntimeInitialRenderRadius

.section .text.RuntimeRenderBlocks,"ax",%progbits
.global RuntimeRenderBlocks
.type RuntimeRenderBlocks, %function
RuntimeRenderBlocks:
    push {r4, lr}
    bl NativeEffectiveRenderDistance
    ldr r1, =0xb16348
    ldr r1, [r1]
    lsl r1, r1, #4
    cmp r0, r1
    movhi r0, r1
    pop {r4, pc}
.ltorg
.size RuntimeRenderBlocks, . - RuntimeRenderBlocks

.section .text.RuntimeBuilderLimit,"ax",%progbits
.global RuntimeBuilderLimit
.type RuntimeBuilderLimit, %function
RuntimeBuilderLimit:
    push {r12}
    ldr r12, =0xb16348
    ldr r12, [r12, #12]
    cmp r0, r12
    movgt r0, r12
    pop {r12}
    str r0, [r4, #20]
    ldr pc, =0x1d87a4
.ltorg
.size RuntimeBuilderLimit, . - RuntimeBuilderLimit
