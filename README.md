# XylenOS
![XylenOS logo](https://i.imgur.com/v5dNEFR.png) <br>
XylenOS (from Greek "ξυλοσ"), started as UselessOS or uOS, is a hobbyist project made by a Turkish and Bangladeshi Hobbyist. <br/>

## Screenshots
![Screenshot of XylenOS 0.09](https://media.discordapp.net/attachments/1352748114448224368/1375015446054572072/image.png?ex=6830266c&is=682ed4ec&hm=d32f8d5b990f56405f6a25903af8f3bf132a2e59cb04b43cee45e627b8c541c7&=&format=webp&quality=lossless)


## Features

 - Boots (finally :sob:🙏)
 - Working Bootloader 🤑
 - 32-Bit 😎
 - 64-Bit (Half Baked) :pensive:

## To-Do list

 - [x] Fixed Bootloader
 - [x] Get Kernel to load into memory
 - [ ] Text-based Userland
 - [ ] Filesystem
 
## Build Guide
1. Make sure to install `x86_64-elf-gcc` and `x86_64-elf-binutils`
2. Compile the bootloader `nasm bootloader.asm -f bin -o bootloader.bin`
3. Compile the Kernel Loader `nasm enter_kernel.asm -f elf -o enter_kernel.o`
4. Compile the Kernel `x86_64-elf-gcc -m32 -ffreestanding -c "kernel.c" -o "kernel.o"`
5. Combine the Kernel with the loader `x86_64-elf-ld -melf_i386 -o kernel.bin -Ttext 0x1000 --oformat binary enter_kernel.o *.o`
6. Combine all the code `cat bootloader.bin kernel.bin > os-image`

Finally try the image with `qemu-system-x86_64 -drive file=os-image,if=floppy,index=0,media=disk,format=raw -net none`

or here is a simple script 

    #!/bin/bash
    rm -f *.o *.bin os-image
    nasm bootloader.asm -f bin -o bootloader.bin
    nasm enter_kernel.asm -f elf -o enter_kernel.o
    for file in *.c; do
        x86_64-elf-gcc -m32 -ffreestanding -c "$file" -o "${file%.c}.o"
    done 
    x86_64-elf-ld -melf_i386 -o kernel.bin -Ttext 0x1000 --oformat binary enter_kernel.o *.o
    cat bootloader.bin kernel.bin > os-image
    qemu-system-x86_64 -drive file=os-image,if=floppy,index=0,media=disk,format=raw -net none
