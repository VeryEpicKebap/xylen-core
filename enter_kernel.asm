; enter_kernel.asm
[bits 32]
[extern kernel_main]

global _start

_start:
    ; Set up the stack pointer
    mov esp, 0x90000

    ; Call the C kernel's main function
    call kernel_main

.hang:
    hlt
    jmp .hang