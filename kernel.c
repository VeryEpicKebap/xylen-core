#include <stdint.h>

static inline uint8_t inb(uint16_t p) { uint8_t r; asm volatile ("inb %%dx, %%al" : "=a"(r) : "d"(p)); return r; }
static inline void outb(uint16_t p, uint8_t v) { asm volatile ("outb %%al, %%dx" :: "a"(v), "d"(p)); }
static inline void outw(uint16_t p, uint16_t v) { asm volatile ("outw %%ax, %%dx" :: "a"(v), "d"(p)); }

#define VW 80
#define VH 25
#define VMEM ((uint16_t*)0xB8000)
static int cx = 0, cy = 0;
static const uint8_t vclr = 0x0F;

static void ucurs() { uint16_t p = cy * VW + cx; outb(0x3D4, 0x0F); outb(0x3D5, (uint8_t)(p & 0xFF)); outb(0x3D4, 0x0E); outb(0x3D5, (uint8_t)((p >> 8) & 0xFF)); }
static void ecurs() { outb(0x3D4, 0x0A); outb(0x3D5, inb(0x3D5) & 0xC0); outb(0x3D4, 0x0B); outb(0x3D5, 0x0F); }
static void vscroll() {
    if (cy < VH) return;
    for (int y = 1; y < VH; y++)
        for (int x = 0; x < VW; x++)
            VMEM[(y-1)*VW + x] = VMEM[y*VW + x];
    for (int x = 0; x < VW; x++)
        VMEM[(VH-1)*VW + x] = (uint16_t)(' ' | (vclr << 8));
    cy = VH - 1;
}
static void putc(char c) {
    if (c == '\r') return;
    if (c == '\n') { cx = 0; cy++; vscroll(); ucurs(); return; }
    VMEM[cy*VW + cx] = (uint16_t)(c | (vclr << 8));
    if (++cx >= VW) { cx = 0; cy++; vscroll(); }
    ucurs();
}
static void prints(const char *s) { while (*s) putc(*s++); }
static void cls() { for (int i = 0; i < VW*VH; i++) VMEM[i] = (uint16_t)(' ' | (vclr << 8)); cx = cy = 0; ucurs(); }

static const char kbmap[128] = {
    0,27,'1','2','3','4','5','6','7','8','9','0','-','=', '\b','\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
    'z','x','c','v','b','n','m',',','.','/',0,'',0,' ',0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static char getk() {
    while (1) {
        if (inb(0x64) & 0x01) {
            uint8_t sc = inb(0x60);
            if (sc == 0 || (sc & 0x80)) continue;
            return kbmap[sc];
        }
    }
}

static int scmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (uint8_t)*a - (uint8_t)*b;
}
static int sncmp(const char *a, const char *b, int n) {
    for (int i = 0; i < n; i++) if (a[i] != b[i] || !a[i] || !b[i]) return (uint8_t)a[i] - (uint8_t)b[i];
    return 0;
}
static int slen(const char *s) { int l = 0; while (s[l]) l++; return l; }
static char *sncpy(char *d, const char *s, int n) { int i = 0; for (; i < n && s[i]; i++) d[i] = s[i]; for (; i < n; i++) d[i] = 0; return d; }
static void *mcpy(void *d, const void *s, int n) { char *dd = d; const char *ss = s; for (int i = 0; i < n; i++) dd[i] = ss[i]; return d; }

static void reboot() { while (inb(0x64) & 0x02) {} outb(0x64, 0xFE); for (;;) asm volatile ("hlt"); }

static uint8_t bcd2bin(uint8_t b) { return (b & 0x0F) + ((b >> 4) * 10); }
static uint8_t cmos_read(uint8_t r) { outb(0x70, r); return inb(0x71); }
static void pnum2(int n) { putc('0' + (n / 10)); putc('0' + (n % 10)); }
static void ptime() {
    uint8_t s = cmos_read(0x00), m = cmos_read(0x02), h = cmos_read(0x04);
    uint8_t d = cmos_read(0x07), mo = cmos_read(0x08), y = cmos_read(0x09);
    uint8_t st = cmos_read(0x0B);
    if (!(st & 0x04)) {
        s = bcd2bin(s); m = bcd2bin(m); h = bcd2bin(h); d = bcd2bin(d); mo = bcd2bin(mo); y = bcd2bin(y);
    }
    prints("20"); pnum2(y); putc('-'); pnum2(mo); putc('-'); pnum2(d); putc(' '); pnum2(h); putc(':'); pnum2(m); putc(':'); pnum2(s); putc('\n');
}

#define ZADFS_MAXF 16
#define ZADFS_MAXN 16
#define ZADFS_DSZ 1024
#define ZADFS_MAGIC 0x5ADF55

typedef struct { char name[ZADFS_MAXN]; int sz, off, used; } zfile_t;

typedef struct { int magic, nfiles, next_off, use_hdd; zfile_t files[ZADFS_MAXF]; char data[ZADFS_DSZ]; } zadfs_t;
static zadfs_t zadfs;

static int hdd_detect() {
    outb(0x1F6, 0xA0);
    for (volatile int i = 0; i < 1000; i++);
    uint8_t st = inb(0x1F7);
    if (st == 0xFF) return 0;
    for (int i = 0; i < 4; i++) {
        st = inb(0x1F7);
        if ((st & 0xC0) == 0x40) break;
        for (volatile int j = 0; j < 10000; j++);
    }
    return (st & 0xC0) == 0x40;
}
static void ata_wait() { while ((inb(0x1F7) & 0xC0) != 0x40); }
static int ata_write(uint32_t lba, const uint8_t *buf) {
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
static int ata_read(uint32_t lba, uint8_t* buf) {
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

static void zadfs_init() {
    zadfs.magic = ZADFS_MAGIC;
    zadfs.nfiles = 0;
    zadfs.next_off = 0;
    for (int i = 0; i < ZADFS_MAXF; i++) zadfs.files[i].used = 0;
    for (int i = 0; i < ZADFS_DSZ; i++) zadfs.data[i] = 0;
}
static int zadfs_ok() {
    if (zadfs.magic != ZADFS_MAGIC) return 0;
    if (zadfs.nfiles < 0 || zadfs.nfiles > ZADFS_MAXF) return 0;
    if (zadfs.next_off < 0 || zadfs.next_off > ZADFS_DSZ) return 0;
    return 1;
}
static void zadfs_ls() {
    prints("Files:\n");
    int found = 0;
    for (int i = 0; i < ZADFS_MAXF; i++) {
        if (zadfs.files[i].used) {
            prints("  ");
            prints(zadfs.files[i].name);
            prints(" (");
            char sz[8];
            int n = zadfs.files[i].sz, p = 0;
            if (n == 0) sz[p++] = '0';
            else {
                int t = n, d = 1;
                while (t >= 10) { t /= 10; d = 10; }
                t = n;
                while (d > 0) { sz[p++] = '0' + (t / d); t %= d; d /= 10; }
            }
            sz[p] = 0;
            prints(sz);
            prints(" bytes)\n");
            found = 1;
        }
    }
    if (!found) prints("  (none)\n");
}
static void zadfs_save() {
    uint8_t s[512];
    for (int i = 0; i < 512; i++) s[i] = 0;
    *((int*)s) = zadfs.magic;
    s[4] = zadfs.nfiles;
    s[5] = zadfs.next_off;
    for (int i = 0; i < ZADFS_MAXF; i++) {
        int b = 8 + i * 25;
        for (int j = 0; j < ZADFS_MAXN; j++) s[b + j] = zadfs.files[i].name[j];
        *((int*)(s + b + 16)) = zadfs.files[i].sz;
        *((int*)(s + b + 20)) = zadfs.files[i].off;
        s[b + 24] = zadfs.files[i].used;
    }
    ata_write(0, s);
    for (int b = 0; b < (ZADFS_DSZ + 511) / 512; b++) {
        for (int i = 0; i < 512; i++) s[i] = (b*512 + i < ZADFS_DSZ) ? zadfs.data[b*512 + i] : 0;
        ata_write(1 + b, s);
    }
}
static void zadfs_load() {
    uint8_t s[512];
    ata_read(0, s);
    zadfs.magic = *((int*)s);
    zadfs.nfiles = s[4];
    zadfs.next_off = s[5];
    for (int i = 0; i < ZADFS_MAXF; i++) {
        int b = 8 + i * 25;
        for (int j = 0; j < ZADFS_MAXN; j++) zadfs.files[i].name[j] = s[b + j];
        zadfs.files[i].sz = *((int*)(s + b + 16));
        zadfs.files[i].off = *((int*)(s + b + 20));
        zadfs.files[i].used = s[b + 24];
    }
    for (int b = 0; b < (ZADFS_DSZ + 511) / 512; b++) {
        ata_read(1 + b, s);
        for (int i = 0; i < 512 && b*512 + i < ZADFS_DSZ; i++) zadfs.data[b*512 + i] = s[i];
    }
}
static void zadfs_new(const char *n, const char *c) {
    if (zadfs.nfiles >= ZADFS_MAXF) { prints("No more file slots!\n"); return; }
    int l = slen(c);
    if (zadfs.next_off + l > ZADFS_DSZ) { prints("Not enough storage!\n"); return; }
    int idx = -1;
    for (int i = 0; i < ZADFS_MAXF; i++) if (!zadfs.files[i].used) { idx = i; break; }
    if (idx == -1) { prints("Internal error: no free slot\n"); return; }
    int nl = 0;
    while (n[nl] && nl < ZADFS_MAXN-1) { zadfs.files[idx].name[nl] = n[nl]; nl++; }
    zadfs.files[idx].name[nl] = '\0';
    zadfs.files[idx].sz = l;
    zadfs.files[idx].off = zadfs.next_off;
    zadfs.files[idx].used = 1;
    for (int i = 0; i < l; i++) zadfs.data[zadfs.next_off + i] = c[i];
    zadfs.next_off += l;
    zadfs.nfiles++;
    if (zadfs.use_hdd) zadfs_save();
    prints("File created!\n");
}
static void zadfs_cat(const char *n) {
    for (int i = 0; i < ZADFS_MAXF; i++) {
        if (zadfs.files[i].used && scmp(zadfs.files[i].name, n) == 0) {
            for (int j = 0; j < zadfs.files[i].sz; j++) putc(zadfs.data[zadfs.files[i].off + j]);
            putc('\n');
            return;
        }
    }
    prints("File not found\n");
}

void kernel_main() {
    cls();
    ecurs();
    prints("Welcome to XylenOS v0.1.6\n");

    int hdd = hdd_detect();
    zadfs.use_hdd = 0;

    if (hdd) {
        prints("HDD detected! Type 'install' to format OS/filesystem, or 'skip' to use HDD without formatting, or any other key for RAM only.\n~# ");
        char cmd[16]; int l = 0;
        while (1) {
            char c = getk();
            if (c == '\n') { putc('\n'); cmd[l] = '\0'; break; }
            else if (c == '\b') {
                if (l > 0) {
                    l--;
                    if (cx > 0) {
                        cx--;
                        VMEM[cy*VW + cx] = (uint16_t)(' ' | (vclr << 8));
                        ucurs();
                    }
                }
            } else if (l < 15) {
                cmd[l++] = c; putc(c);
            }
        }

        if (scmp(cmd, "install") == 0) {
            zadfs.use_hdd = 1;
            zadfs_init();
            zadfs_save();
            prints("Installed! All files will now go to HDD.\n");
        } else if (scmp(cmd, "skip") == 0) {
            zadfs.use_hdd = 1;
            zadfs_load();
            if (zadfs_ok()) {
                prints("Loaded FS from HDD.\n");
            } else {
                prints("No valid FS found, formatting new one.\n");
                zadfs_init();
                zadfs_save();
            }
        } else {
            zadfs.use_hdd = 0;
            zadfs_init();
            prints("Using RAM only.\n");
        }
    } else {
        zadfs.use_hdd = 0;
        zadfs_init();
        prints("No HDD found. Using RAM only.\n");
    }

    prints("Type 'help' for commands.\n");

    while (1) {
        prints("~# ");
        char line[80]; int l = 0;
        while (1) {
            char c = getk();
            if (c == '\n') { putc('\n'); line[l] = '\0'; break; }
            if (c == '\b') { if (l > 0) { l--; if (cx > 0) { cx--; VMEM[cy*VW + cx] = ' ' | (vclr << 8); ucurs(); } } }
            else if (l < (int)sizeof(line)-1) { line[l++] = c; putc(c); }
        }

        if (scmp(line, "version") == 0) {
            prints("XylenOS Pre-Alpha 0.1.6 (Stable Build)\n");
        } else if (scmp(line, "help") == 0) {
            prints("Commands: version, clear, help, reboot, time, mkfs, ls, touchfile <name> <content>, cat <name>, loadfs\n");
        } else if (scmp(line, "clear") == 0) {
            cls();
        } else if (scmp(line, "reboot") == 0) {
            prints("Rebooting...\n"); reboot();
        } else if (scmp(line, "time") == 0) {
            prints("Current date and time: "); ptime();
        } else if (scmp(line, "ls") == 0) {
            zadfs_ls();
        } else if (scmp(line, "mkfs") == 0) {
            zadfs_init();
            if (zadfs.use_hdd) zadfs_save();
            prints("Formatted FS!\n");
        } else if (scmp(line, "loadfs") == 0 && zadfs.use_hdd) {
            zadfs_load();
            if (zadfs_ok()) {
                prints("Loaded FS from HDD.\n");
            } else {
                prints("FS validation failed! Formatted new FS.\n");
                zadfs_init();
                zadfs_save();
            }
        } else if (line[0] == 'c' && line[1] == 'a' && line[2] == 't' && line[3] == ' ') {
            const char *n = line + 4; while (*n == ' ') n++;
            zadfs_cat(n);
        } else if (sncmp(line, "touchfile ", 10) == 0) {
            const char *a = line + 10; while (*a == ' ') a++;
            const char *n = a, *c = a;
            while (*c && *c != ' ') c++;
            if (*c == ' ') {
                int nl = c - n;
                if (nl >= ZADFS_MAXN) { prints("Filename too long!\n"); continue; }
                c++;
                char fn[ZADFS_MAXN];
                sncpy(fn, n, nl); fn[nl] = '\0';
                zadfs_new(fn, c);
            } else {
                prints("Usage: touchfile <name> <content>\n");
            }
        } else if (l > 0) {
            prints("Unknown command\n");
        }
    }
}