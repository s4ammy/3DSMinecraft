.syntax unified
.arch armv6k
.arm
.text
.global AllocateAligned4
.type AllocateAligned4, %function

//Entered after the original free-head null comparison and register setup.
//Four-byte-aligned block headers need no per-node alignment rounding.
AllocateAligned4:
    beq AllocationFailed
    cmp r2, #4
    bne OriginalForwardSearch
AlignedLoop:
    ldr lr, [r12, #4]
    cmp lr, r3
    cmpcs r6, lr
    bls NextBlock
    mov r1, r12
    mov r6, lr
    add r8, r12, #16
    cmp r7, #0
    bne CommitSelectedBlock
    cmp lr, r3
    beq CommitSelectedBlock
NextBlock:
    ldr r12, [r12, #12]
    cmp r12, #0
    bne AlignedLoop
    b CommitSelectedBlock
.size AllocateAligned4, . - AllocateAligned4
