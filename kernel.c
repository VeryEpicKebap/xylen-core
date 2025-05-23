#include <stdint.h>

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %%dx, %%al" : "=a"(ret) : "d"(port));
    return ret;
}
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %%al, %%dx" :: "a"(val), "d"(port));
}

volatile uint16_t * const VGA = (uint16_t*)0xB8000;
static int cursor_x = 0, cursor_y = 0;
static const uint8_t VGA_COLOR = 0x0F;

static void enable_cursor() {
    outb(0x3D4, 0x0A);
    outb(0x3D5, inb(0x3D5) & 0xC0);
    outb(0x3D4, 0x0B);
    outb(0x3D5, 0x0F);
}

static void update_cursor() {
    uint16_t pos = cursor_y * 80 + cursor_x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static void newline() {
    cursor_x = 0;
    cursor_y++;
    if (cursor_y >= 25) {
        for(int y = 1; y < 25; y++)
            for(int x = 0; x < 80; x++)
                VGA[(y-1)*80 + x] = VGA[y*80 + x];
        for(int x = 0; x < 80; x++)
            VGA[(24)*80 + x] = (uint16_t)(' ' | (VGA_COLOR << 8));
        cursor_y = 24;
    }
    update_cursor();
}

static void putchar(char c) {
    if (c == '\r') return;
    if (c == '\n') {
        newline();
        return;
    }
    VGA[cursor_y*80 + cursor_x] = (uint16_t)(c | (VGA_COLOR << 8));
    cursor_x++;
    if (cursor_x >= 80) newline();
    update_cursor();
}

static void prints(const char *s) {
    while (*s) putchar(*s++);
}

static void clear_screen() {
    for(int i = 0; i < 80*25; i++)
        VGA[i] = (uint16_t)(' ' | (VGA_COLOR << 8));
    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}

static const char kbdus[128] = {
    0,27,'1','2','3','4','5','6','7','8',
   '9','0','-','=', '\b','\t','q','w','e','r',
   't','y','u','i','o','p','[',']','\n',0,
   'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',
    0,'*',0,' ',0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0
};

static char get_key() {
    uint8_t status, sc;
    while (1) {
        status = inb(0x64);
        if (status & 0x01) {
            sc = inb(0x60);
            if (sc == 0 || (sc & 0x80)) continue;
            return kbdus[sc];
        }
    }
}

static int strcmp(const char *s1, const char *s2) {
    while(*s1 && *s1 == *s2) { s1++; s2++; }
    return (uint8_t)*s1 - (uint8_t)*s2;
}

static void do_reboot() {
    while (inb(0x64) & 0x02) { }
    outb(0x64, 0xFE);
    for (;;) { asm volatile ("hlt"); }
}

static uint8_t bcd_to_bin(uint8_t bcd) {
    return (bcd & 0x0F) + ((bcd >> 4) * 10);
}

static uint8_t read_cmos(uint8_t reg) {
    outb(0x70, reg);
    return inb(0x71);
}

static void print_num2(int num) {
    putchar('0' + (num / 10));
    putchar('0' + (num % 10));
}

static void print_time() {
    uint8_t sec  = read_cmos(0x00);
    uint8_t min  = read_cmos(0x02);
    uint8_t hour = read_cmos(0x04);
    uint8_t day  = read_cmos(0x07);
    uint8_t mon  = read_cmos(0x08);
    uint8_t year = read_cmos(0x09);
    uint8_t status_b = read_cmos(0x0B);

    if (!(status_b & 0x04)) {
        sec  = bcd_to_bin(sec);
        min  = bcd_to_bin(min);
        hour = bcd_to_bin(hour);
        day  = bcd_to_bin(day);
        mon  = bcd_to_bin(mon);
        year = bcd_to_bin(year);
    }

    prints("20");
    print_num2(year);
    putchar('-');
    print_num2(mon);
    putchar('-');
    print_num2(day);
    putchar(' ');
    print_num2(hour);
    putchar(':');
    print_num2(min);
    putchar(':');
    print_num2(sec);
    putchar('\n');
}

void kernel_main() {
    clear_screen();
    enable_cursor();
    prints("Welcome to XylenOS v0.1.5\n");
    prints("Type 'help' for commands.\n");

    while (1) {
        prints("~# ");
        char line[80];
        int len = 0;
        while (1) {
            char c = get_key();
            if (c == '\n') {
                putchar('\n');
                line[len] = '\0';
                break;
            } else if (c == '\b') {
                if (len > 0) {
                    len--;
                    if (cursor_x > 0) {
                        cursor_x--;
                        VGA[cursor_y*80 + cursor_x] = (uint16_t)(' ' | (VGA_COLOR << 8));
                        update_cursor();
                    }
                }
            } else {
                if (len < (int)sizeof(line)-1) {
                    line[len++] = c;
                    putchar(c);
                }
            }
        }

        if (strcmp(line, "version") == 0) {
            prints("XylenOS Pre-Alpha 0.1.5 (Public Test Build 0.1.5-R1)\n");
        } else if (strcmp(line, "help") == 0) {
            prints("Commands: version, clear, help, reboot, time\n");
        } else if (strcmp(line, "clear") == 0) {
            clear_screen();
        } else if (strcmp(line, "reboot") == 0) {
            prints("Rebooting...\n");
            do_reboot();
        } else if (strcmp(line, "time") == 0) {
            prints("Current date and time: ");
            print_time();
        } else if (len > 0) {
            prints("Unknown command\n");
        }
    }
}
