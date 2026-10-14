.syntax unified
.arch armv6k
.fpu vfpv2
.arm

.equ AllocateScratchBuffer, 0x124de0
.equ FreeScratchBuffer, 0x2fea60
.equ SignedDivide64, 0x125ec4
.equ DecompressLoopStart, 0x3ddd24
.equ NextIntContinue, 0x5c3334

.section .text,"ax",%progbits
.global PerformancePayloadStart
.global InitializeDecompressionScratch
.global AcquireDecompressionScratch
.global ReleaseDecompressionScratch
.global FinishDecompression
.global NextIntPowerOfTwo
.global InitializeOrResetInflate

PerformancePayloadStart:
InitializeDecompressionScratch:
    sub sp, sp, #0x4c
    mov r0, #0
    //Keep ownership outside the z_stream at sp+0x10 through sp+0x47.
    str r0, [sp, #0x48]
    str r0, [sp, #0x2c]
    str r0, [sp, #0x30]
    str r0, [sp, #0x34]
    str r0, [sp, #0x38]
    ldr pc, =DecompressLoopStart

AcquireDecompressionScratch:
    ldr r2, [sp, #0x48]
    cmp r2, #0
    movne r0, r2
    bxne lr
    push {r4, lr}
    ldr r12, =AllocateScratchBuffer
    blx r12
    str r0, [sp, #0x50]
    pop {r4, pc}

ReleaseDecompressionScratch:
    push {r4, lr}
    ldr r0, [sp, #0x34]
    cmp r0, #0
    beq DecompressionStateReleased
    add r0, sp, #0x18
    ldr r12, =0x7c3b20
    blx r12

DecompressionStateReleased:
    ldr r4, [sp, #0x50]
    cmp r4, #0
    beq DecompressionScratchReleased
    mov r0, r4
    ldr r12, =FreeScratchBuffer
    blx r12
    mov r0, #0
    str r0, [sp, #0x50]

DecompressionScratchReleased:
    pop {r4, pc}

FinishDecompression:
    bl ReleaseDecompressionScratch

DecompressionFinished:
    add sp, sp, #0x4c
    mov r0, #1
    vpop {d8}
    pop {r4-r11, pc}

NextIntPowerOfTwo:
    ldrd r4, r5, [r7, #0x10]
    cmp r6, #0
    ble NextIntDivide
    sub r2, r6, #1
    tst r6, r2
    bne NextIntDivide
    mov r0, r4, lsr #24
    orr r0, r0, r5, lsl #8
    and r2, r0, r2
    ldr pc, =NextIntContinue

NextIntDivide:
    mov r2, r6
    mov r3, r6, asr #31
    mov r12, r4, lsr #24
    mov r1, r5, asr #24
    orr r0, r12, r5, lsl #8
    ldr r12, =SignedDivide64
    blx r12
    ldr pc, =NextIntContinue

.balign 4
.ltorg

InitializeOrResetInflate:
    ldr r3, [r0, #0x1c]
    cmp r3, #0
    ldreq pc, =0x7c3b6c
    ldr pc, =0x7c3cc4

.balign 4
.ltorg

.global InitializeFinalClimateCell

InitializeFinalClimateCell:
    add r12, r4, #1
    cmp r12, r6
    bxne lr
    ldr r12, [sp, #0x1c]
    sub r12, r12, #1
    cmp r5, r12
    bxne lr
    ldr pc, =0x5c31cc

.global PrepareInflateOutput
.global CommitInflateOutput
.global RestoreInflateTerminator

PrepareInflateOutput:
    mov r3, #0
    str r3, [sp, #0xc]
    stm r2, {r5, r11}
    ldr r3, [r9]
    ldr r12, [r3, #-12]
    add r12, r12, #1
    cmp r12, #1
    bxhi lr
    ldr r12, [r3, #-8]
    ldr r1, [r3, #-4]
    cmp r12, r1
    bcc UseScratchOutput
    sub r12, r12, r1
    cmp r12, r11
    bcc UseScratchOutput
    add r3, r3, r1
    str r3, [sp, #0xc]
    str r3, [r2]

UseScratchOutput:
    mov r1, #0
    bx lr

CommitInflateOutput:
    ldr r12, [sp, #0xc]
    cmp r12, #0
    beq AppendScratchOutput
    ldr r12, [sp]
    cmp r12, #0
    beq AppendScratchOutput
    add r1, r1, r12
    ldr r3, [r0]
    str r1, [r3, #-4]
    mov r2, #0
    strb r2, [r3, r1]
    bx lr

AppendScratchOutput:
    ldr pc, =0x101121

RestoreInflateTerminator:
    ldr r0, [sp, #0xc]
    cmp r0, #0
    movne r1, #0
    strbne r1, [r0]
    add r0, sp, #0x10
    bx lr

.balign 4
.ltorg
