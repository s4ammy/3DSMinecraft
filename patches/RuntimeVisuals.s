.syntax unified
.arch armv6k
.arm
.fpu vfpv2

.section .text.ControlsOriginalPrompt, "ax", %progbits
.global ControlsOriginalPrompt
.type ControlsOriginalPrompt, %function
ControlsOriginalPrompt:
    push {r4-r11, lr}
    b controlsPromptResume

.section .text.ControlsOriginalParticles, "ax", %progbits
.global ControlsOriginalParticles
.type ControlsOriginalParticles, %function
ControlsOriginalParticles:
    push {r4, lr}
    b controlsParticlesResume

.macro VisualGate name, predicate, resume
.section .text.\name, "ax", %progbits
.global \name
.type \name, %function
\name:
    push {r0-r3, r12, lr}
    vpush {d0-d7}
    bl \predicate
    cmp r0, #0
    vpop {d0-d7}
    pop {r0-r3, r12, lr}
    moveq r0, #0
    bxeq lr
    push {r4-r11, lr}
    b \resume
.endm

VisualGate RuntimeWeatherHook, RuntimeWeatherEnabled, controlsWeatherResume
VisualGate RuntimeMapHook, RuntimeMapRefresh, controlsMapResume

.section .text.RuntimeTextureHook, "ax", %progbits
.global RuntimeTextureHook
.type RuntimeTextureHook, %function
RuntimeTextureHook:
    push {r0-r3, r12, lr}
    vpush {d0-d7}
    mov r2, r0
    add r0, sp, #96
    add r1, sp, #440
    add r3, sp, #128
    bl RuntimeTextureLength
    str r0, [sp, #64]
    vpop {d0-d7}
    pop {r0-r3, r12, lr}
    ldr r1, [sp, #12]
    b controlsTextureResume

.section .text.SkipHiddenBlockFaces, "ax", %progbits
.global SkipHiddenBlockFaces
.type SkipHiddenBlockFaces, %function
SkipHiddenBlockFaces:
    cmp r6, #0
    beq controlsOriginalFaceBranch
    ldr r0, [r6]
    and r0, r0, #63
    cmp r0, #63
    beq controlsOriginalFaceCleanup
    cmp r6, #0
    b controlsOriginalFaceBranch
