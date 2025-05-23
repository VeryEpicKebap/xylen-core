#!/bin/bash
set -e

# Assemble the bootloader
nasm bootloader.asm -f bin -o bootloader.bin

# Assemble the kernel entry stub
nasm enter_kernel.asm -f elf -o enter_kernel.o

# Compile the kernel C code
i686-elf-gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -O2 -c kernel.c -o kernel.o

# Link the kernel
i686-elf-ld -m elf_i386 -Ttext 0x1000 -e _start -o kernel.elf enter_kernel.o kernel.o

# Convert the ELF to a flat binary
i686-elf-objcopy -O binary kernel.elf kernel.bin

# Combine bootloader and kernel into a single image
cat bootloader.bin kernel.bin > os-image

# Calculate and display the size
size_bytes=$(stat -c%s kernel.bin)
sectors=$(( (size_bytes + 511) / 512 ))
echo "kernel.bin size: $size_bytes bytes"
echo "Sectors needed (512 bytes each): $sectors"