.syntax unified
.arch armv6k
.arm
.fpu vfpv2
.type ControlsStringAssign, %function

.section .controlsHooks, "ax", %progbits
.global RuntimeInputHook
.type RuntimeInputHook, %function
RuntimeInputHook:
    push {r0-r3, r12, lr}
    vpush {d0-d7}
    mov r0, r5
    mov r1, r8
    bl UpdateRuntimeControls
    vpop {d0-d7}
    pop {r0-r3, r12, lr}
    ldr r0, [r5, #0x14]
    b controlsInputResume

.global RuntimeHudDestroyHook
.type RuntimeHudDestroyHook, %function
RuntimeHudDestroyHook:
    push {r0-r3, r12, lr}
    bl ForgetRuntimeHud
    pop {r0-r3, r12, lr}
    .word 0xe92d41f0
    b controlsHudDestroyResume

.global RuntimeAccessoryHook
.type RuntimeAccessoryHook, %function
RuntimeAccessoryHook:
    push {r0-r3, r12, lr}
    vpush {d0-d7}
    bl StartControlAccessory
    vpop {d0-d7}
    pop {r0-r3, r12, lr}
    b controlsControllerConnected

//The existing fresh-press container-cancel helper is installed in this slot.
.org 0xa0
.space 16

.global RuntimeShutdownHook
.type RuntimeShutdownHook, %function
RuntimeShutdownHook:
    push {r0-r3, r12, lr}
    bl StopControlAccessory
    pop {r0-r3, r12, lr}
    push {r4, lr}
    b controlsShutdownResume

.global ControlsOriginalCategory
.type ControlsOriginalCategory, %function
ControlsOriginalCategory:
    vldr s0, controlsOne
    b controlsCategoryResume
controlsOne:
    .float 1.0

.global ControlsOriginalDestructor
.type ControlsOriginalDestructor, %function
ControlsOriginalDestructor:
    ldr r2, =0x9a6fa4
    b controlsDestructorResume
.ltorg

.global ControlsOriginalRelease
.type ControlsOriginalRelease, %function
ControlsOriginalRelease:
    push {r0, r4-r11, lr}
    b controlsReleaseResume

.section .controlsThread, "ax", %progbits
.global ControlsCreateThread
.type ControlsCreateThread, %function
ControlsCreateThread:
    push {r4-r6, lr}
    mov r5, r0
    mov r3, r1
    ldr r0, =ControlsThreadFinished
    str r0, [r5, #16]
    ldr r0, =ControlAccessoryWorker
    str r0, [r5, #20]
    str r5, [r5, #24]
    mov r0, #0x38
    ldr r1, =controlsThreadEntry
    add r2, r5, #16
    mvn r4, #1
    svc #8
    str r0, [r5, #12]
    tst r0, #0x80000000
    movne r1, #0
    str r1, [r5]
    pop {r4-r6, pc}

.type ControlsThreadFinished, %function
ControlsThreadFinished:
    mov r0, #0
    bx lr

.global ControlsJoinThread
.type ControlsJoinThread, %function
ControlsJoinThread:
    push {r4, lr}
    mov r4, r0
    mvn r2, #0
    mvn r3, #0
    svc #0x24
    mov r0, r4
    svc #0x23
    pop {r4, pc}

.global ControlsSleep
.type ControlsSleep, %function
ControlsSleep:
    ldr r0, =100000000
    mov r1, #0
    svc #0x0a
    bx lr
.ltorg

.section .controlsMemory, "ax", %progbits
.global ControlsStringConstruct
.type ControlsStringConstruct, %function
ControlsStringConstruct:
    ldr r12, =0x2ff221
    bx r12

.global ControlsStringRelease
.type ControlsStringRelease, %function
ControlsStringRelease:
    ldr r12, =0x2febbd
    bx r12

.global ControlsStringAssign
.type ControlsStringAssign, %function
ControlsStringAssign:
    ldr r12, =0x100aa1
    bx r12
.ltorg

.global __aeabi_memcpy
.global __aeabi_memcpy4
.type __aeabi_memcpy, %function
.type __aeabi_memcpy4, %function
__aeabi_memcpy:
__aeabi_memcpy4:
    cmp r2, #0
    bxeq lr
    ldrb r3, [r1], #1
    strb r3, [r0], #1
    subs r2, r2, #1
    bne __aeabi_memcpy
    bx lr

.section .controlsDivision, "ax", %progbits
.global __aeabi_uidiv
.global __aeabi_uidivmod
.type __aeabi_uidiv, %function
.type __aeabi_uidivmod, %function
__aeabi_uidiv:
__aeabi_uidivmod:
    cmp r1, #0
    moveq r0, #0
    bxeq lr
    mov r2, #0
    mov r3, #0
    mov r12, #32
1:
    movs r0, r0, lsl #1
    adcs r3, r3, r3
    cmpcc r3, r1
    subcs r3, r3, r1
    adc r2, r2, r2
    subs r12, r12, #1
    bne 1b
    mov r0, r2
    mov r1, r3
    bx lr

.global __aeabi_idiv
.type __aeabi_idiv, %function
__aeabi_idiv:
    push {r4, lr}
    eor r4, r0, r1
    cmp r0, #0
    rsbmi r0, r0, #0
    cmp r1, #0
    rsbmi r1, r1, #0
    bl __aeabi_uidiv
    cmp r4, #0
    rsbmi r0, r0, #0
    pop {r4, pc}
