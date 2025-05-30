#!/bin/bash
set -e

# This script builds the operating system image from source files.
# It assembles the bootloader, compiles the kernel and its components,
# links them together, and produces a final binary image.
# Ensure the necessary tools are installed
if ! command -v nasm &> /dev/null || ! command -v i686-elf-gcc &> /dev/null || ! command -v i686-elf-ld &> /dev/null || ! command -v i686-elf-objcopy &> /dev/null; then
    echo "Required tools are not installed. Please install nasm, i686-elf-gcc, i686-elf-ld, and i686-elf-objcopy."
    exit 1
fi
# Notify about build start
echo "Starting XylenOS build process..."

# Assemble the bootloader
nasm bootloader.asm -f bin -o bootloader.bin

# Assemble the kernel entry stub
nasm enter_kernel.asm -f elf -o enter_kernel.o

# Compile all the C source files
i686-elf-gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -O2 -c kernel/main.c -o kernel_main.o
i686-elf-gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -O2 -c lib/string.c -o string.o
i686-elf-gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -O2 -c lib/memory.c -o memory.o
i686-elf-gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -O2 -c drivers/keyboard.c -o keyboard.o
i686-elf-gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -O2 -c drivers/vga.c -o vga.o
i686-elf-gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -O2 -c drivers/cmos.c -o cmos.o
i686-elf-gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -O2 -c drivers/ata.c -o ata.o
i686-elf-gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -O2 -c system/reboot.c -o reboot.o
i686-elf-gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -O2 -c fs/zadfs.c -o zadfs.o

# Link all object files together
i686-elf-ld -m elf_i386 -Ttext 0x1000 -e _start -o kernel.elf enter_kernel.o kernel_main.o string.o memory.o keyboard.o vga.o cmos.o ata.o reboot.o zadfs.o

# Convert the ELF to a flat binary
i686-elf-objcopy -O binary kernel.elf kernel.bin

# Combine bootloader and kernel into a single image
cat bootloader.bin kernel.bin > os-image

# Calculate and display the size
size_bytes=$(stat -c%s kernel.bin)
sectors=$(( (size_bytes + 511) / 512 ))
echo "kernel.bin size: $size_bytes bytes"
echo "Sectors needed (512 bytes each): $sectors"

# Clean up intermediate files
rm -f bootloader.bin enter_kernel.o kernel_main.o string.o memory.o keyboard.o vga.o cmos.o ata.o reboot.o zadfs.o kernel.elf kernel.bin

# Notify the user of successful build
echo "Build completed successfully. Output file: os-image"