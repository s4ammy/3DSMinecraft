.syntax unified
.arm

.extern inputFunctionResume

.section .old3dsCameraHook, "ax", %progbits
.global old3dsCameraHook
.type old3dsCameraHook, %function
old3dsCameraHook:
    b old3dsCirclePadCamera
.size old3dsCameraHook, . - old3dsCameraHook

.section .old3dsCameraCode, "ax", %progbits
.global old3dsCirclePadCamera
.type old3dsCirclePadCamera, %function
old3dsCirclePadCamera:
    add r2, r5, #0x160
    ldr r3, [r2, #8]
    tst r3, #0x200
    beq inputFunctionContinue
    bic r3, r3, #0x200
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
.size old3dsCirclePadCamera, . - old3dsCirclePadCamera
