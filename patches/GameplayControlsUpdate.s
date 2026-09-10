.syntax unified
.arm

.extern inputFunctionResume
.extern controlsState

.section .ControlsHookUpdate, "ax", %progbits
.global ControlsHookUpdate
.type ControlsHookUpdate, %function
ControlsHookUpdate:
    b GameplayControlsUpdate
.size ControlsHookUpdate, . - ControlsHookUpdate

.section .controlsCode, "ax", %progbits
.global GameplayControlsUpdate
.type GameplayControlsUpdate, %function
GameplayControlsUpdate:
    cmp r8, #4
    bhs InputFunctionContinue
    add r2, r5, #0x160
    ldr r0, [r2, #8]
    ldr r3, =controlsState
    add r3, r3, r8, lsl #2
    ldr r12, [r3]
    tst r0, #0x800
    moveq r12, #0
    tst r0, #0x200
    andne r12, r0, #0x800
    str r12, [r3]
    bic r3, r0, #0x200
    cmp r12, #0
    bicne r3, r3, #0x800
    andne r12, r0, #0x200
    orrne r3, r3, r12
    str r3, [r2, #8]
    tst r0, #0x200
    beq InputFunctionContinue
    bic r3, r3, #0xf0000000
    str r3, [r2, #8]
    mov r3, #166
    ldrsh r12, [r2]
    mul r12, r3, r12
    cmp r12, #0
    addlt r12, r12, #255
    asr r12, r12, #8
    strh r12, [r2, #4]
    ldrsh r12, [r2, #2]
    mul r12, r3, r12
    cmp r12, #0
    addlt r12, r12, #255
    asr r12, r12, #8
    strh r12, [r2, #6]
    mov r12, #0
    str r12, [r2]
InputFunctionContinue:
    ldr r0, [r5, #0x14]
    b inputFunctionResume
.size GameplayControlsUpdate, . - GameplayControlsUpdate
.ltorg
.space 256 - (. - GameplayControlsUpdate), 0
