.syntax unified
.arch armv6k
.arm
.fpu vfpv2

.section .text.NativeSkipStockRows,"ax",%progbits
.global NativeSkipStockRows
.type NativeSkipStockRows,%function
NativeSkipStockRows:
    ldrb r2, [r4, #280]
    cmp r2, #2
    ldreq pc, =0x235830
    ldr r2, =0xa31c64
    ldr pc, =0x23563c
.ltorg

.section .text.NativeOriginalMainButton,"ax",%progbits
.global NativeOriginalMainButton
.type NativeOriginalMainButton,%function
NativeOriginalMainButton:
    push {r4-r6, lr}
    ldr pc, =0x26e844
.ltorg

.section .text.NativeOriginalDropdownKey,"ax",%progbits
.global NativeOriginalDropdownKey
.type NativeOriginalDropdownKey,%function
NativeOriginalDropdownKey:
    push {r4-r6, lr}
    ldr pc, =0x5d9300
.ltorg

.section .text.NativeOriginalPauseButton,"ax",%progbits
.global NativeOriginalPauseButton
.type NativeOriginalPauseButton,%function
NativeOriginalPauseButton:
    vldr s0, nativeMenuOne
    ldr pc, =0x1a11b0
nativeMenuOne:
    .float 1.0
.ltorg
