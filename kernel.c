/* kernel.c – Minimal 32-bit kernel in C */

#include <stdint.h>

/* I/O port access (inline assembly) */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %%dx, %%al" : "=a"(ret) : "d"(port));
    return ret;
}
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %%al, %%dx" :: "a"(val), "d"(port));
}

/* VGA text-mode buffer at physical address 0xB8000 */
volatile uint16_t * const VGA = (uint16_t*)0xB8000;
static int cursor_x = 0, cursor_y = 0;
static const uint8_t VGA_COLOR = 0x0F;  // White on black

/* Advance to next line, scrolling if needed */
static void newline() {
    cursor_x = 0;
    cursor_y++;
    if (cursor_y >= 25) {
        // Scroll up: copy lines 1–24 to 0–23
        for(int y = 1; y < 25; y++) {
            for(int x = 0; x < 80; x++) {
                VGA[(y-1)*80 + x] = VGA[y*80 + x];
            }
        }
        // Clear the last line
        for(int x = 0; x < 80; x++) {
            VGA[(24)*80 + x] = (uint16_t)(' ' | (VGA_COLOR << 8));
        }
        cursor_y = 24;
    }
}

/* Output a character at the current cursor position */
static void putchar(char c) {
    if (c == '\r') return;      // ignore carriage return
    if (c == '\n') {            // handle newline
        newline();
        return;
    }
    // Write character with attribute byte
    VGA[cursor_y*80 + cursor_x] = (uint16_t)(c | (VGA_COLOR << 8));
    cursor_x++;
    if (cursor_x >= 80) {
        newline();
    }
}

/* Print a null-terminated string */
static void prints(const char *s) {
    while (*s) putchar(*s++);
}

/* Clear the screen (fill with spaces) */
static void clear_screen() {
    for(int i = 0; i < 80*25; i++) {
        VGA[i] = (uint16_t)(' ' | (VGA_COLOR << 8));
    }
    cursor_x = 0;
    cursor_y = 0;
}

/* Keyboard scan-code to ASCII (US layout, set 1) */
static const char kbdus[128] = {
    0,  27, '1','2','3','4','5','6','7','8',  // 0–9
   '9','0','-','=', '\b','\t','q','w','e','r', // 10–19
   't','y','u','i','o','p','[',']','\n',  0,    // 20–29 (0x1C = Enter)
   'a','s','d','f','g','h','j','k','l',';','\'','`', // 30–41
    0,'\\','z','x','c','v','b','n','m',',','.','/', // 42–53
    0,'*', 0, ' ', 0,0,0,0,0,0,0,           // 54–64 (0x39 = Space)
    0,0,0,0,0,0,0,0,0,0,0,0,0,            // 65–77
    0,0,0,0,0,0,0                        // 78–84
};

/* Read a key from the keyboard (polling PS/2 controller) */
static char get_key() {
    uint8_t status, sc;
    while (1) {
        status = inb(0x64);            // read PS/2 status register
        if (status & 0x01) {          // if output buffer full (bit 0)
            sc = inb(0x60);           // read scan code from data port
            if (sc == 0) continue;
            if (sc & 0x80) continue;   // ignore key release (break code)
            return kbdus[sc];         // translate to ASCII
        }
    }
}

/* String compare (kernel cannot use libc) */
static int strcmp(const char *s1, const char *s2) {
    while(*s1 && *s1 == *s2) { s1++; s2++; }
    return (uint8_t)*s1 - (uint8_t)*s2;
}

/* Reboot the system via PS/2 keyboard controller (write 0xFE to port 0x64) */
static void do_reboot() {
    /* Wait until the input buffer is empty (bit 1 of status) */
    while (inb(0x64) & 0x02) { }
    outb(0x64, 0xFE);      // Command 0xFE pulses the reset line
    for (;;) { asm volatile ("hlt"); }
}

/* Kernel entry point */
void kernel_main() {
    clear_screen();
    prints("Welcome to XylenOS v0.1\n");
    prints("Type 'help' for commands.\n");
    while (1) {
        prints("> ");               // prompt
        char line[80];
        int len = 0;
        // Read a line of input
        while (1) {
            char c = get_key();
            if (c == '\n') {
                putchar('\n');
                line[len] = '\0';
                break;
            } else if (c == '\b') {
                // Handle backspace
                if (len > 0) {
                    len--;
                    if (cursor_x > 0) {
                        cursor_x--;
                        VGA[cursor_y*80 + cursor_x] = (uint16_t)(' ' | (VGA_COLOR << 8));
                    }
                }
            } else {
                if (len < (int)sizeof(line)-1) {
                    line[len++] = c;
                    putchar(c);
                }
            }
        }
        // Execute commands
        if (strcmp(line, "version") == 0) {
            prints("MyOS version 0.1\n");
        } else if (strcmp(line, "help") == 0) {
            prints("Commands: version, clear, help, reboot\n");
        } else if (strcmp(line, "clear") == 0) {
            clear_screen();
        } else if (strcmp(line, "reboot") == 0) {
            prints("Rebooting...\n");
            do_reboot();
        } else if (len > 0) {
            prints("Unknown command\n");
        }
    }
}
