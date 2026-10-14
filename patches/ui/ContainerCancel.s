.syntax unified
.arm

.text
.global CloseContainerOnMenuCancel
.type CloseContainerOnMenuCancel, %function
CloseContainerOnMenuCancel:
    mov r0, r4
    mov r1, #1
    bl ContainerCancel
    b CancelReturn
.size CloseContainerOnMenuCancel, . - CloseContainerOnMenuCancel
