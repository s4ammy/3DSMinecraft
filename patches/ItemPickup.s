.syntax unified
.arch armv6k
.arm

.extern clientTickResume
.extern pickupTickLoop

.section .pickupHook, "ax", %progbits
.global PickupHook
.type PickupHook, %function
PickupHook:
    beq PickupTick
.size PickupHook, . - PickupHook

.section .pickupCode, "ax", %progbits
.global PickupTick
.type PickupTick, %function
PickupTick:
    ldr r0, [r4, #0x68]
    ldr lr, =clientTickResume
    //Match the particle tick stack frame before entering its pickup-only loop.
    push {r3-r11, lr}
    mov r8, r0
    b pickupTickLoop
.size PickupTick, . - PickupTick
.pool
