.syntax unified
.arch armv6k
.arm
.section .text,"ax",%progbits
.global CompleteStreamingJobWhenLate

CompleteStreamingJobWhenLate:
    //R4 is the worker pool. Poll one completion without executing a new job.
    ldr r0, [r4, #4]
    ldr r1, [r4, #8]
    cmp r0, r1
    beq WorkerBudgetExhausted
    ldr r0, [r0]
    ldrb r1, [r0]
    cmp r1, #0
    blne CompleteOneWorkerJob
    b WorkerBudgetExhausted
