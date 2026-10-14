.syntax unified
.arch armv6k
.arm
.text
.global FindFreeBlockNeighbors
.type FindFreeBlockNeighbors, %function

//Entered inside the locked SDK free routine with r2=freed start and r3=list headers.
//Only find the same adjacent nodes. Original unlink, merging and insertion stay intact.
//r0, r1 and r12 are scratch. Preserve r4 (freed end), r3, SP and LR.
FindFreeBlockNeighbors:
    ldr r1, [r3]
    cmp r1, #0
    beq SearchFromHead
    ldr r0, [r3, #4]
    cmp r2, r0
    bhi CoalescePrevious
    sub r12, r0, r1
    add r12, r1, r12, lsr #1
    cmp r2, r12
    blo SearchFromHead
    mov r1, r0
SearchPrevious:
    ldr r0, [r1, #8]
    cmp r2, r0
    movls r1, r0
    bls SearchPrevious
    b CoalesceNext
SearchFromHead:
    b OriginalSearch
.size FindFreeBlockNeighbors, . - FindFreeBlockNeighbors
