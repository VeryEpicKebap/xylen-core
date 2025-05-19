#define VGA_WIDTH 80
#define VGA_HEIGHT 25

__attribute__((section(".multiboot")))
const unsigned int multiboot_header[] = {
    0xE85250D6,
    0,
    8,
    0x100000000 - (0xE85250D6 + 0 + 8),
    0, 0, 8
};

void kmain(void);

volatile unsigned short* vga_buffer = (unsigned short*)0xB8000;
unsigned int cursor_x = 0;
unsigned int cursor_y = 0;
unsigned char color = 0x0F;

void clear_screen(void) {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            unsigned int index = y * VGA_WIDTH + x;
            vga_buffer[index] = (unsigned short)' ' | (unsigned short)color << 8;
        }
    }
    cursor_x = 0;
    cursor_y = 0;
}

void scroll(void) {
    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            unsigned int index_prev = (y - 1) * VGA_WIDTH + x;
            unsigned int index_curr = y * VGA_WIDTH + x;
            vga_buffer[index_prev] = vga_buffer[index_curr];
        }
    }

    for (int x = 0; x < VGA_WIDTH; x++) {
        unsigned int index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
        vga_buffer[index] = (unsigned short)' ' | (unsigned short)color << 8;
    }

    cursor_y = VGA_HEIGHT - 1;
    cursor_x = 0;
}

void putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\t') {
        cursor_x = (cursor_x + 8) & ~(8 - 1);
    } else {
        unsigned int index = cursor_y * VGA_WIDTH + cursor_x;
        vga_buffer[index] = (unsigned short)c | (unsigned short)color << 8;
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= VGA_HEIGHT) {
        scroll();
    }
}

void print(const char* str) {
    while (*str) {
        putchar(*str++);
    }
}

void _start(void) {
    kmain();
    while (1) {
        __asm__ volatile ("hlt");
    }
}

void kmain(void) {
    clear_screen();
    print("XylenOS 32-bit Kernel\n");
    print("----------------------\n");
    print("we did it\n");
}

