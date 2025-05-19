; Minimal bootloader that prints a message and loads a kernel from disk
; Assumes the kernel is immediately after the boot sector (i.e., on the 2nd sector)
; The kernel is loaded at 0x1000 and execution jumps there

[org 0x7c00]

KERNEL_OFFSET equ 0x1000

start:
    ; Store the boot drive number (set by BIOS in DL)
    mov [bootdrv], dl

    ; Print message
    mov si, msg
.print:
    lodsb
    cmp al, 0
    je .done_print
    mov ah, 0x0E
    int 0x10
    jmp .print
.done_print:

    ; Set up segment registers for loading
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax

    ; Set up destination for kernel
    mov bx, KERNEL_OFFSET

    ; Read 1 sector (the next sector; sector 2) from disk into 0x1000:0000
    mov ah, 0x02        ; BIOS read sector function
    mov al, 1           ; Number of sectors to read
    mov ch, 0           ; Cylinder 0
    mov dh, 0           ; Head 0
    mov cl, 2           ; Sector 2 (first sector after bootloader)
    mov dl, [bootdrv]   ; Boot drive
    int 0x13            ; BIOS disk interrupt

    jc disk_error       ; If carry flag set, there was an error

    ; Jump to loaded kernel (far jump: new CS:IP)
    jmp 0x0000:KERNEL_OFFSET

disk_error:
    mov si, err_msg
.print_err:
    lodsb
    cmp al, 0
    je .hang
    mov ah, 0x0E
    int 0x10
    jmp .print_err
.hang:
    jmp $

msg db "Booting kernel...", 0
err_msg db "Disk read error!", 0
bootdrv db 0

times 510-($-$$) db 0
dw 0xAA55
