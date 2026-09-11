.syntax unified
.arch armv6k
.arm
.section .text,"ax",%progbits
.global BeginVoronoiCache
.global InitializeVoronoiCorner
.global NextVoronoiRandom

BeginVoronoiCache:
    push {r0-r4, lr}
    sub sp, sp, #0x160
    ldr r12, [sp, #0x178]
    str r12, [sp]
    ldr r12, [sp, #0x17c]
    str r12, [sp, #4]
    sub r2, r2, #2
    bic r2, r2, #3
    str r2, [sp, #8]
    sub r3, r3, #2
    bic r3, r3, #3
    str r3, [sp, #12]
    mov r0, #0
    mov r1, #83
    add r12, sp, #16

ClearVoronoiCache:
    str r0, [r12], #4
    subs r1, r1, #1
    bne ClearVoronoiCache
    add r12, sp, #0x160
    ldm r12, {r0-r3}
    bl EnterOriginalVoronoi
    add sp, sp, #0x160
    add sp, sp, #16
    pop {r4, pc}

EnterOriginalVoronoi:
    push {r0-r12, lr}
    ldr pc, =0x1b97d8

InitializeVoronoiCorner:
    push {r4, r5}
    add r4, sp, #0x1000
    add r4, r4, #0xb0
    ldr r1, [r4]
    cmp r1, #28
    ldrls r1, [r4, #4]
    cmpls r1, #28
    bhi UncachedVoronoiCorner
    ldr r5, [r4, #8]
    sub r1, r2, r5
    mov r1, r1, lsr #2
    ldr r5, [sp, #8]
    ldr r12, [r4, #12]
    sub r5, r5, r12
    mov r5, r5, lsr #2
    add r5, r5, r5, lsl #3
    add r1, r1, r5
    add r1, r4, r1, lsl #2
    add r1, r1, #24
    str r1, [r4, #16]
    ldr r5, [r1]
    mov r1, #0
    str r1, [r4, #20]
    tst r5, #0x80000000
    pop {r4, r5}
    bxne lr
    ldr pc, =0x5c31cc

UncachedVoronoiCorner:
    mov r1, #0
    str r1, [r4, #16]
    pop {r4, r5}
    ldr pc, =0x5c31cc

NextVoronoiRandom:
    add r12, sp, #0x1000
    add r12, r12, #0xa8
    ldr r2, [r12, #16]
    cmp r2, #0
    ldreq pc, =0x5c330c
    ldr r3, [r2]
    tst r3, #0x80000000
    beq GenerateVoronoiRandom
    ldr r1, [r12, #20]
    eor r2, r1, #1
    str r2, [r12, #20]
    add r1, r1, r1, lsl #2
    mov r1, r1, lsl #1
    mov r0, r3, lsr r1
    mov r0, r0, lsl #22
    mov r0, r0, lsr #22
    bx lr

GenerateVoronoiRandom:
    push {r4-r6, lr}
    mov r4, r12
    mov r5, r2
    ldr r6, [r4, #20]
    ldr r12, =0x5c330c
    blx r12
    cmp r6, #0
    streq r0, [r5]
    ldrne r1, [r5]
    orrne r1, r1, r0, lsl #10
    orrne r1, r1, #0x80000000
    strne r1, [r5]
    eor r6, r6, #1
    str r6, [r4, #20]
    pop {r4-r6, pc}

.balign 4
.ltorg
