.syntax unified
.arch armv6k
.arm

.extern clientTickResume
.extern particleTick
.extern pickupTickLoop

.section .pickupCode, "ax", %progbits
.global PickupTickUpdate
.type PickupTickUpdate, %function
PickupTickUpdate:
    ldr lr, =clientTickResume
    bne particleTick
    ldr r0, [r5, #0x644]
    push {r4-r11, lr}
    sub sp, sp, #12
    mov r8, r0
    b pickupTickLoop
.size PickupTickUpdate, . - PickupTickUpdate
.pool
