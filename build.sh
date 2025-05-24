#!/bin/bash
set -e
nasm bootloader.asm -f bin -o bootloader.bin
nasm enter_kernel.asm -f elf -o enter_kernel.o
i686-elf-gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -O2 -c kernel.c -o kernel.o
i686-elf-ld -m elf_i386 -Ttext 0x1000 -e _start -o kernel.elf enter_kernel.o kernel.o
i686-elf-objcopy -O binary kernel.elf kernel.bin
cat bootloader.bin kernel.bin > os-image
size_bytes=$(stat -c%s kernel.bin)
sectors=$(( (size_bytes + 511) / 512 ))
echo "kernel.bin size: $size_bytes bytes"
echo "Sectors needed (512 bytes each): $sectors"