.syntax unified
.arch armv6k
.arm

.extern accessoryState
.extern accessoryStackTop
.extern accessoryInitialized
.extern threadEntry
.extern accessoryStatus
.extern accessoryStreaming
.extern accessoryStart
.extern accessoryStop
.extern controllerConnected
.extern accessoryShutdownResume

.section .accessoryCode, "ax", %progbits
.global AccessoryConnectHook
.type AccessoryConnectHook, %function
AccessoryConnectHook:
    push {r0-r5, r12, lr}
    ldr r5, =accessoryState
    ldr r0, [r5, #8]
    cmp r0, #0
    bne AccessoryConnectDone
    mov r0, #1
    str r0, [r5, #8]
    //Use the game's thread entry to initialize and clean up SDK thread-local state.
    ldr r0, =AccessoryThreadFinished
    str r0, [r5, #16]
    ldr r0, =AccessoryWorker
    str r0, [r5, #20]
    str r5, [r5, #24]
    mov r0, #0x38
    ldr r1, =threadEntry
    add r2, r5, #16
    ldr r3, =accessoryStackTop
    mvn r4, #1
    svc #8
    str r0, [r5, #12]
    tst r0, #0x80000000
    movne r1, #0
    str r1, [r5]
AccessoryConnectDone:
    pop {r0-r5, r12, lr}
    b controllerConnected
.size AccessoryConnectHook, . - AccessoryConnectHook

.global AccessoryWorker
.type AccessoryWorker, %function
AccessoryWorker:
    push {r4, r5, r12, lr}
    mov r4, r0
AccessoryPoll:
    ldr r0, [r4, #4]
    cmp r0, #0
    bne AccessoryWorkerDone
    ldr r0, =accessoryInitialized
    ldrb r0, [r0]
    cmp r0, #0
    beq AccessoryIdle
    bl accessoryStatus
    cmp r0, #2
    bleq accessoryStop
    bl accessoryStreaming
    cmp r0, #0
    bne AccessoryIdle
    mov r0, #0
    mov r1, #8
    bl accessoryStart
    str r0, [r4, #12]
    //Retry after 2.5 seconds, checking shutdown between short sleeps.
    mov r5, #25
    b AccessoryWait
AccessoryIdle:
    mov r5, #1
AccessoryWait:
    ldr r0, [r4, #4]
    cmp r0, #0
    bne AccessoryWorkerDone
    ldr r0, =100000000
    mov r1, #0
    svc #0x0a
    subs r5, r5, #1
    bne AccessoryWait
    b AccessoryPoll
AccessoryWorkerDone:
    pop {r4, r5, r12, pc}
.size AccessoryWorker, . - AccessoryWorker

.type AccessoryThreadFinished, %function
AccessoryThreadFinished:
    mov r0, #0
    bx lr
.size AccessoryThreadFinished, . - AccessoryThreadFinished

.global AccessoryShutdownHook
.type AccessoryShutdownHook, %function
AccessoryShutdownHook:
    //Preserve the original shutdown frame, and join before library memory is released.
    push {r4, lr}
    ldr r4, =accessoryState
    mov r0, #1
    str r0, [r4, #4]
    ldr r0, [r4]
    cmp r0, #0
    beq AccessoryShutdownDone
    mvn r2, #0
    mvn r3, #0
    svc #0x24
    ldr r0, [r4]
    svc #0x23
AccessoryShutdownDone:
    mov r0, #0
    str r0, [r4]
    str r0, [r4, #4]
    str r0, [r4, #8]
    str r0, [r4, #12]
    b accessoryShutdownResume
.size AccessoryShutdownHook, . - AccessoryShutdownHook
.pool
