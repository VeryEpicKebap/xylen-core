# XylenOS
![XylenOS logo](https://cdn.discordapp.com/attachments/1371783823297089537/1375498387075371028/Baslksz102_20250523183835.png?ex=6831e833&is=683096b3&hm=2aa554e98f435e342f9ee41a676a3785ec8de56e51c5129ad904f95515a4e782&) <br>
XylenOS (from Greek "ξυλοσ"), started as UselessOS or uOS, is a hobbyist project made by a Turkish and Bangladeshi Hobbyist. <br/>


## Screenshots
![Screenshot of XylenOS 0.1.5](https://media.discordapp.net/attachments/1375015758999982151/1375573560482988072/image.png?ex=68322e35&is=6830dcb5&hm=90b2a42c5bafd11f369b15fb1e8f29dbe33dc83060f61dc24ebd1b58e022bd0c&=&format=webp&quality=lossless)


## Features

 - 32-bit with support for 64-bit hardware
 - Simple Text-Based Userland aka Shell
 - Simple Commands like touchfile, time, ls, cat
 - Fully fledged Filesystem called ZadFS™ 😎

## To-Do list (Roadmap of May/June)

 - [x] Fixed Bootloader
 - [x] Get Kernel to load into memory
 - [x] Text-based Userland aka Shell
 - [x] Live Clock
 - [x] Filesystem
 - [ ] Extended Filesystem
 - [ ] Executable Loader (.elf)
 - [ ] Text-User Interface
 - [ ] Simplification of Installation onto Hard Drive
 - [ ] Mount Tool
 - 2nd Dev Phase
 - [ ] Text Editor
 - [ ] Beeper Application
 - [ ] Working NAT
 - [ ] Brainfuck Interpreter
 - [ ] Package Manager
 
## Build Guide
Make sure you have `x86_64-elf-gcc` and `x86_64-elf-binutils`  installed.

Here is a simple Build script.
````
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
````
