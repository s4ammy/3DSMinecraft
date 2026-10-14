.syntax unified
.arm
.fpu vfpv2

.section .touchHooks, "ax", %progbits
.global TouchInputHook
.type TouchInputHook, %function
TouchInputHook:
    push {r0-r3, r12, lr}
    vpush {d0-d7}
    mov r0, r5
    mov r1, r8
    bl UpdateTouchStick
    vpop {d0-d7}
    pop {r0-r3, r12, lr}
    ldr r0, [r5, #0x14]
    b touchInputResume
.size TouchInputHook, . - TouchInputHook

.global TouchHudDestroyHook
.type TouchHudDestroyHook, %function
TouchHudDestroyHook:
    push {r1-r3, lr}
    ldr r1, =touchStickState
    ldr r2, [r1]
    cmp r2, r0
    moveq r2, #0
    streq r2, [r1]
    pop {r1-r3, lr}
    .word 0xe92d41f0
    b touchHudDestroyResume
.size TouchHudDestroyHook, . - TouchHudDestroyHook
.ltorg
