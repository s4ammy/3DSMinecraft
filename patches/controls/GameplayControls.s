.syntax unified
.arm

.extern inputFunctionResume
.extern controlsState

.section .controlsHook, "ax", %progbits
.global controlsHook
.type controlsHook, %function
controlsHook:
    b old3dsGameplayControls
.size controlsHook, . - controlsHook

.section .controlsCode, "ax", %progbits
.global old3dsGameplayControls
.type old3dsGameplayControls, %function
old3dsGameplayControls:
    cmp r7, #4
    bhs inputFunctionContinue
    add r2, r5, #0x160
    ldr r0, [r2, #8]
    ldr r3, =controlsState
    add r3, r3, r7, lsl #2
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
    beq inputFunctionContinue
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
inputFunctionContinue:
    ldr r0, [r5, #0x14]
    b inputFunctionResume
.size old3dsGameplayControls, . - old3dsGameplayControls
.ltorg
.space 256 - (. - old3dsGameplayControls), 0
