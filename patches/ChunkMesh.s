.syntax unified
.arch armv6k
.arm

.section .text.ReadPackedLight,"ax",%progbits
.global ReadPackedLight
.type ReadPackedLight, %function
ReadPackedLight:
    push {r4, r5}
    ldr r4, [r0, #380]
    ldrb r3, [r0, #396]
    mov r2, r1, lsr r3
    and r2, r2, r4
    ldrb r3, [r0, #392]
    mov r12, r1, lsr r3
    and r12, r12, r4
    add r2, r12, r2, lsl #2
    ldrb r3, [r0, #388]
    mov r12, r1, lsr r3
    and r12, r12, r4
    add r2, r12, r2, lsl #2
    add r3, r0, r2
    mov r12, #1
    strb r12, [r3, #240]
    ldr r5, [r0, r2, lsl #2]
    cmp r5, #0
    ldrbeq r0, [r0, #349]
    beq PackedLightReturn
    add r5, r5, #6144
    ldr r5, [r5]
    cmp r5, #0
    ldreq r0, =0x989d32
    beq PackedLightByte
    ldr r2, [r0, #352]
    ldr r3, [r0, #356]
    ldrb r4, [r0, #360]
    and r2, r2, r1
    and r3, r3, r1
    orr r2, r2, r3, lsr r4
    ldr r3, [r0, #368]
    ldrb r4, [r0, #372]
    and r3, r3, r1
    orr r2, r2, r3, lsr r4
    add r0, r5, r2
PackedLightByte:
    ldrb r0, [r0]
PackedLightReturn:
    pop {r4, r5}
    bx lr
.ltorg
.size ReadPackedLight, . - ReadPackedLight

.section .text.LimitDeferredBuilderPool,"ax",%progbits
.global LimitDeferredBuilderPool
.type LimitDeferredBuilderPool, %function
LimitDeferredBuilderPool:
    //Keep the native empty-pool retry and the separate immediate builder.
    cmp r0, #4
    movgt r0, #4
    str r0, [r4, #0x14]
    ldr pc, =0x1d87a4
.size LimitDeferredBuilderPool, . - LimitDeferredBuilderPool
.ltorg

.section .text.GetBiomeFromCachedChunk,"ax",%progbits
.global GetBiomeFromCachedChunk
.type GetBiomeFromCachedChunk, %function
GetBiomeFromCachedChunk:
    ldr r2, [r0, #52]
    cmp r2, #0
    beq OriginalBiome
    ldr r3, [r1]
    ldr r12, [r2, #32]
    cmp r12, r3, asr #4
    bne OriginalBiome
    ldr r3, [r1, #8]
    ldr r12, [r2, #36]
    cmp r12, r3, asr #4
    bne OriginalBiome
    ldrsh r12, [r0, #24]
    cmp r12, #0
    ble OriginalBiome
    //The native biome query always uses Y=0.
    and r3, r3, #15
    ldrb r12, [r1]
    and r12, r12, #15
    add r3, r12, r3, lsl #4
    add r2, r2, #128
    ldrb r2, [r2, r3]
    ldr r3, =controlsBiomesById
    ldr r2, [r3, r2, lsl #2]
    cmp r2, #0
    movne r0, r2
    bxne lr
OriginalBiome:
    push {r4, r5, lr}
    sub sp, sp, #28
    b controlsOriginalBiomeBody
.ltorg
