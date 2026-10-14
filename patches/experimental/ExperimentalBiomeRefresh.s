.syntax unified
.arch armv6k
.arm
.section .text,"ax",%progbits
.global RefreshCachedBiomeColors

RefreshCachedBiomeColors:
    //Research only: R5 is the chunk at the mesh-builder call site.
    ldr r12, [r5, #0x44]
    cmp r12, #0
    bxne lr
    ldr pc, =0x1c7ac8

.balign 4
.ltorg
