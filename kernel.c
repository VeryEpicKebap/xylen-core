#include <stdint.h>

// --- Keyboard US mapping arrays, supporting shift and all ASCII symbols ---
static const char kbdus[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static const char kbdus_shift[128] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', '\t',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// --- I/O Port Functions ---
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %%dx, %%al" : "=a"(ret) : "d"(port));
    return ret;
}
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %%al, %%dx" :: "a"(val), "d"(port));
}
static inline void outw(uint16_t port, uint16_t val) {
    asm volatile ("outw %%ax, %%dx" :: "a"(val), "d"(port));
}

// --- VGA Text Functions ---
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEM ((uint16_t*)0xB8000)
static int cursor_x = 0, cursor_y = 0;
static const uint8_t VGA_COLOR = 0x0F;

static void move_cursor() {
    uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;
    outb(0x3D4, 0x0F); outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E); outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}
static void enable_cursor() {
    outb(0x3D4, 0x0A); outb(0x3D5, inb(0x3D5) & 0xC0);
    outb(0x3D4, 0x0B); outb(0x3D5, 0x0F);
}
static void scroll() {
    if (cursor_y < VGA_HEIGHT) return;
    for (int y = 1; y < VGA_HEIGHT; y++)
        for (int x = 0; x < VGA_WIDTH; x++)
            VGA_MEM[(y-1)*VGA_WIDTH + x] = VGA_MEM[y*VGA_WIDTH + x];
    for (int x = 0; x < VGA_WIDTH; x++)
        VGA_MEM[(VGA_HEIGHT-1)*VGA_WIDTH + x] = (uint16_t)(' ' | (VGA_COLOR << 8));
    cursor_y = VGA_HEIGHT - 1;
}
static void putchar(char c) {
    if (c == '\r') return;
    if (c == '\n') {
        cursor_x = 0; cursor_y++;
        scroll();
        move_cursor();
        return;
    }
    VGA_MEM[cursor_y*VGA_WIDTH + cursor_x] = (uint16_t)(c | (VGA_COLOR << 8));
    cursor_x++;
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0; cursor_y++;
        scroll();
    }
    move_cursor();
}
static void prints(const char *s) { while (*s) putchar(*s++); }
static void clear_screen() {
    for (int i = 0; i < VGA_WIDTH*VGA_HEIGHT; i++)
        VGA_MEM[i] = (uint16_t)(' ' | (VGA_COLOR << 8));
    cursor_x = cursor_y = 0;
    move_cursor();
}

// --- Keyboard Functions with Shift Support (fixed for shift hold) ---
static char get_key() {
    static int shift = 0;
    while (1) {
        if (inb(0x64) & 0x01) {
            uint8_t sc = inb(0x60);
            if (sc == 0x2A) { shift = 1; continue; }
            if (sc == 0x36) { shift = 1; continue; }
            if (sc == 0xAA) { shift = 0; continue; }
            if (sc == 0xB6) { shift = 0; continue; }
            if (sc & 0x80) continue;
            char c = shift ? kbdus_shift[sc] : kbdus[sc];
            if (c) return c;
        }
    }
}

// --- String & Memory ---
static int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (uint8_t)*a - (uint8_t)*b;
}
static int strncmp(const char *a, const char *b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i] || !a[i] || !b[i]) return (uint8_t)a[i] - (uint8_t)b[i];
    }
    return 0;
}
static int strlen(const char *s) {
    int l = 0; while (s[l]) l++; return l;
}
static char *strncpy(char *d, const char *s, int n) {
    int i = 0; for (; i < n && s[i]; i++) d[i] = s[i];
    for (; i < n; i++) d[i] = 0; return d;
}
static void *memcpy(void *d, const void *s, int n) {
    char *dd = d; const char *ss = s; for (int i = 0; i < n; i++) dd[i] = ss[i]; return d;
}

// --- Reboot ---
static void do_reboot() {
    while (inb(0x64) & 0x02) {}
    outb(0x64, 0xFE);
    for (;;) asm volatile ("hlt");
}

// --- CMOS Time ---
static uint8_t bcd_to_bin(uint8_t bcd) { return (bcd & 0x0F) + ((bcd >> 4) * 10); }
static uint8_t read_cmos(uint8_t reg) {
    outb(0x70, reg); return inb(0x71);
}
static void print_num2(int num) {
    putchar('0' + (num / 10)); putchar('0' + (num % 10));
}
static void print_time() {
    uint8_t sec = read_cmos(0x00), min = read_cmos(0x02), hour = read_cmos(0x04);
    uint8_t day = read_cmos(0x07), mon = read_cmos(0x08), year = read_cmos(0x09);
    uint8_t status_b = read_cmos(0x0B);
    if (!(status_b & 0x04)) {
        sec = bcd_to_bin(sec); min = bcd_to_bin(min); hour = bcd_to_bin(hour);
        day = bcd_to_bin(day); mon = bcd_to_bin(mon); year = bcd_to_bin(year);
    }
    prints("20"); print_num2(year); putchar('-');
    print_num2(mon); putchar('-'); print_num2(day); putchar(' ');
    print_num2(hour); putchar(':'); print_num2(min); putchar(':'); print_num2(sec); putchar('\n');
}

// --- ZadFS Persistent Filesystem ---
#define ZADFS_MAX_FILES      128
#define ZADFS_MAX_FILENAME   128
#define ZADFS_DATA_SIZE      4096
#define ZADFS_MAGIC          0x5ADF55
#define ZADFS_FILE_ENTRY_SIZE (ZADFS_MAX_FILENAME + sizeof(int)*2 + 1)

typedef struct {
    char name[ZADFS_MAX_FILENAME];
    int size, data_offset;
    uint8_t used;
} zadfs_file_t;

typedef struct {
    int magic, num_files, next_data_offset, use_hdd;
    zadfs_file_t files[ZADFS_MAX_FILES];
    char data[ZADFS_DATA_SIZE];
} zadfs_t;
static zadfs_t zadfs;

// --- HDD Operations ---
static int detect_hdd() {
    outb(0x1F6, 0xA0);
    for (volatile int i = 0; i < 1000; i++);
    uint8_t status = inb(0x1F7);
    if (status == 0xFF) return 0;
    for (int i = 0; i < 4; i++) {
        status = inb(0x1F7);
        if ((status & 0xC0) == 0x40) break;
        for (volatile int j = 0; j < 10000; j++);
    }
    return (status & 0xC0) == 0x40;
}
static void ata_wait() {
    while ((inb(0x1F7) & 0xC0) != 0x40);
}
static int ata_write_sector(uint32_t lba, const uint8_t* buf) {
    ata_wait();
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, 1);
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x30);
    ata_wait();
    for (int i = 0; i < 256; i++) {
        uint16_t w = buf[2*i] | (buf[2*i+1] << 8);
        outw(0x1F0, w);
    }
    ata_wait();
    return 1;
}
static int ata_read_sector(uint32_t lba, uint8_t* buf) {
    ata_wait();
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, 1);
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x20);
    ata_wait();
    for (int i = 0; i < 256; i++) {
        uint16_t w;
        asm volatile ("inw %%dx, %%ax" : "=a"(w) : "d"(0x1F0));
        buf[2*i] = w & 0xFF;
        buf[2*i+1] = (w >> 8) & 0xFF;
    }
    return 1;
}

// --- ZadFS Functions (fixed for large tables) ---
static void zadfs_format() {
    zadfs.magic = ZADFS_MAGIC;
    zadfs.num_files = 0;
    zadfs.next_data_offset = 0;
    for (int i = 0; i < ZADFS_MAX_FILES; i++) zadfs.files[i].used = 0;
    for (int i = 0; i < ZADFS_DATA_SIZE; i++) zadfs.data[i] = 0;
}
static int zadfs_validate() {
    if (zadfs.magic != ZADFS_MAGIC) return 0;
    if (zadfs.num_files < 0 || zadfs.num_files > ZADFS_MAX_FILES) return 0;
    if (zadfs.next_data_offset < 0 || zadfs.next_data_offset > ZADFS_DATA_SIZE) return 0;
    return 1;
}
static void zadfs_ls() {
    prints("Files:\n");
    int found = 0;
    for (int i = 0; i < ZADFS_MAX_FILES; i++) {
        if (zadfs.files[i].used) {
            prints("  "); prints(zadfs.files[i].name); prints(" (");
            char sz[12]; int n = zadfs.files[i].size, p = 0;
            if (n == 0) sz[p++] = '0';
            else {
                int t = n, d = 1; while (t >= 10) { t /= 10; d *= 10; }
                t = n; while (d > 0) { sz[p++] = '0' + (t / d); t %= d; d /= 10; }
            }
            sz[p] = 0; prints(sz); prints(" bytes)\n"); found = 1;
        }
    }
    if (!found) prints("  (none)\n");
}

static void zadfs_save_to_hdd() {
    int files_bytes = ZADFS_MAX_FILES * ZADFS_FILE_ENTRY_SIZE;
    int meta_bytes = 8 + files_bytes; // 8 bytes for magic, num_files, next_data_offset, plus file table
    int meta_sectors = (meta_bytes + 511) / 512;

    uint8_t sector[512];
    int offset = 0;
    for (int i = 0; i < 512; i++) sector[i] = 0;
    *((int*)sector) = zadfs.magic;
    sector[4] = zadfs.num_files;
    sector[5] = zadfs.next_data_offset;
    offset = 8;

    int file_idx = 0;
    int meta_sector_id = 0;
    for (; meta_sector_id < meta_sectors; meta_sector_id++) {
        if (meta_sector_id > 0) {
            for (int i = 0; i < 512; i++) sector[i] = 0;
            offset = 0;
        }
        while (file_idx < ZADFS_MAX_FILES && offset + ZADFS_FILE_ENTRY_SIZE <= 512) {
            zadfs_file_t *f = &zadfs.files[file_idx];
            for (int j = 0; j < ZADFS_MAX_FILENAME; j++)
                sector[offset + j] = f->name[j];
            *((int*)(sector + offset + ZADFS_MAX_FILENAME)) = f->size;
            *((int*)(sector + offset + ZADFS_MAX_FILENAME + 4)) = f->data_offset;
            sector[offset + ZADFS_MAX_FILENAME + 8] = f->used;
            offset += ZADFS_FILE_ENTRY_SIZE;
            file_idx++;
        }
        ata_write_sector(meta_sector_id, sector);
    }

    int data_bytes = ZADFS_DATA_SIZE;
    int data_sectors = (data_bytes + 511) / 512;
    for (int b = 0; b < data_sectors; b++) {
        for (int i = 0; i < 512; i++)
            sector[i] = (b*512 + i < ZADFS_DATA_SIZE) ? zadfs.data[b*512 + i] : 0;
        ata_write_sector(meta_sectors + b, sector);
    }
}

static void zadfs_load_from_hdd() {
    int files_bytes = ZADFS_MAX_FILES * ZADFS_FILE_ENTRY_SIZE;
    int meta_bytes = 8 + files_bytes;
    int meta_sectors = (meta_bytes + 511) / 512;

    uint8_t sector[512];
    int offset = 0;

    ata_read_sector(0, sector);
    zadfs.magic = *((int*)sector);
    zadfs.num_files = sector[4];
    zadfs.next_data_offset = sector[5];
    offset = 8;

    int file_idx = 0;
    int meta_sector_id = 0;
    for (; meta_sector_id < meta_sectors; meta_sector_id++) {
        if (meta_sector_id > 0) {
            ata_read_sector(meta_sector_id, sector);
            offset = 0;
        }
        while (file_idx < ZADFS_MAX_FILES && offset + ZADFS_FILE_ENTRY_SIZE <= 512) {
            zadfs_file_t *f = &zadfs.files[file_idx];
            for (int j = 0; j < ZADFS_MAX_FILENAME; j++)
                f->name[j] = sector[offset + j];
            f->size = *((int*)(sector + offset + ZADFS_MAX_FILENAME));
            f->data_offset = *((int*)(sector + offset + ZADFS_MAX_FILENAME + 4));
            f->used = sector[offset + ZADFS_MAX_FILENAME + 8];
            offset += ZADFS_FILE_ENTRY_SIZE;
            file_idx++;
        }
    }

    int data_bytes = ZADFS_DATA_SIZE;
    int data_sectors = (data_bytes + 511) / 512;
    for (int b = 0; b < data_sectors; b++) {
        ata_read_sector(meta_sectors + b, sector);
        for (int i = 0; i < 512 && b*512 + i < ZADFS_DATA_SIZE; i++)
            zadfs.data[b*512 + i] = sector[i];
    }
}

static void zadfs_create(const char *name, const char *content) {
    if (zadfs.num_files >= ZADFS_MAX_FILES) { prints("No more file slots!\n"); return; }
    int len = strlen(content);
    if (zadfs.next_data_offset + len > ZADFS_DATA_SIZE) { prints("Not enough storage!\n"); return; }
    int idx = -1; for (int i = 0; i < ZADFS_MAX_FILES; i++) if (!zadfs.files[i].used) { idx = i; break; }
    if (idx == -1) { prints("Internal error: no free slot\n"); return; }
    int nlen = 0; while (name[nlen] && nlen < ZADFS_MAX_FILENAME-1) { zadfs.files[idx].name[nlen] = name[nlen]; nlen++; }
    zadfs.files[idx].name[nlen] = '\0';
    zadfs.files[idx].size = len; zadfs.files[idx].data_offset = zadfs.next_data_offset; zadfs.files[idx].used = 1;
    for (int i = 0; i < len; i++) zadfs.data[zadfs.next_data_offset + i] = content[i];
    zadfs.next_data_offset += len; zadfs.num_files++;
    if (zadfs.use_hdd) zadfs_save_to_hdd();
    prints("File created!\n");
}
static void zadfs_cat(const char *name) {
    for (int i = 0; i < ZADFS_MAX_FILES; i++) {
        if (zadfs.files[i].used && strcmp(zadfs.files[i].name, name) == 0) {
            for (int j = 0; j < zadfs.files[i].size; j++)
                putchar(zadfs.data[zadfs.files[i].data_offset + j]);
            putchar('\n'); return;
        }
    }
    prints("File not found\n");
}

// --- Main Kernel Logic ---
void kernel_main() {
    clear_screen();
    enable_cursor();
    prints("Welcome to XylenOS v0.2.5\n");

    int hdd_found = detect_hdd();
    zadfs.use_hdd = 0;

    if (hdd_found) {
        prints("HDD detected! Type 'install' to format OS/filesystem, or 'skip' to use HDD without formatting, or any other key for RAM only.\n~# ");
        char cmd[16]; int len = 0;
        while (1) {
            char c = get_key();
            if (c == '\n') { putchar('\n'); cmd[len] = '\0'; break; }
            else if (c == '\b') {
                if (len > 0) {
                    len--;
                    if (cursor_x > 0) {
                        cursor_x--;
                        VGA_MEM[cursor_y*VGA_WIDTH + cursor_x] = (uint16_t)(' ' | (VGA_COLOR << 8));
                        move_cursor();
                    }
                }
            } else if (len < 15) {
                cmd[len++] = c; putchar(c);
            }
        }

        if (strcmp(cmd, "install") == 0) {
            zadfs.use_hdd = 1;
            zadfs_format();
            zadfs_save_to_hdd();
            prints("Installed! All files will now go to HDD.\n");
        } else if (strcmp(cmd, "skip") == 0) {
            zadfs.use_hdd = 1;
            zadfs_load_from_hdd();
            if (zadfs_validate()) {
                prints("Loaded ZadFS from HDD.\n");
            } else {
                prints("No valid ZadFS found, formatting new one.\n");
                zadfs_format();
                zadfs_save_to_hdd();
            }
        } else {
            zadfs.use_hdd = 0;
            zadfs_format();
            prints("Using RAM only.\n");
        }
    } else {
        zadfs.use_hdd = 0;
        zadfs_format();
        prints("No HDD found. Using RAM only.\n");
    }

    prints("Type 'help' for commands.\n");

    while (1) {
        prints("~# ");
        char line[512]; int len = 0;
        while (1) {
            char c = get_key();
            if (c == '\n') { putchar('\n'); line[len] = '\0'; break; }
            if (c == '\b') {
    if (len > 0) {
        len--;
        if (cursor_x > 0) {
            cursor_x--;
        } else if (cursor_y > 0) {
            cursor_y--;
            cursor_x = VGA_WIDTH - 1;
        }
        VGA_MEM[cursor_y*VGA_WIDTH + cursor_x] = ' ' | (VGA_COLOR << 8);
        move_cursor();
    }
}            else if (len < (int)sizeof(line)-1) { line[len++] = c; putchar(c); }
        }

        if (strcmp(line, "version") == 0) {
            prints("XylenOS Pre-Alpha 0.2.5 Public Testing Build DEV\n");
        } else if (strcmp(line, "help") == 0) {
            prints("Commands: version, clear, help, reboot, time, format, ls, touchfile <name> <content>, cat <name>, loadfs\n");
        } else if (strcmp(line, "clear") == 0) {
            clear_screen();
        } else if (strcmp(line, "reboot") == 0) {
            prints("Rebooting...\n"); do_reboot();
        } else if (strcmp(line, "time") == 0) {
            prints("Current date and time: "); print_time();
        } else if (strcmp(line, "ls") == 0) {
            zadfs_ls();
        } else if (strcmp(line, "format") == 0) {
            zadfs_format();
            if (zadfs.use_hdd) zadfs_save_to_hdd();
            prints("Formatted ZadFS!\n");
        } else if (strcmp(line, "loadfs") == 0 && zadfs.use_hdd) {
            zadfs_load_from_hdd();
            if (zadfs_validate()) {
                prints("Loaded ZadFS from HDD.\n");
            } else {
                prints("Filesystem validation failed! Formatted new ZadFS.\n");
                zadfs_format();
                zadfs_save_to_hdd();
            }
        } else if (line[0] == 'c' && line[1] == 'a' && line[2] == 't' && line[3] == ' ') {
            const char *filename = line + 4; while (*filename == ' ') filename++;
            zadfs_cat(filename);
        } else if (strncmp(line, "touchfile ", 10) == 0) {
            const char *args = line + 10; while (*args == ' ') args++;
            const char *filename = args, *content = args;
            while (*content && *content != ' ') content++;
            if (*content == ' ') {
                int filename_len = content - filename;
                if (filename_len >= ZADFS_MAX_FILENAME) { prints("Filename too long!\n"); continue; }
                content++; // skip space
                char fname[ZADFS_MAX_FILENAME];
                strncpy(fname, filename, filename_len); fname[filename_len] = '\0';
                zadfs_create(fname, content);
            } else {
                prints("Usage: touchfile <name> <content>\n");
            }
        } else if (len > 0) {
            prints("Unknown command\n");
        }
    }
}
