#include <stdint.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef unsigned int size_t;

// --- Minimal strncat function for kernel ---
static char *strncat(char *dest, const char *src, size_t n) {
    char *d = dest;
    while (*d) d++;
    while (n-- && *src) *d++ = *src++;
    *d = 0;
    return dest;
}

static char *strcpy(char *dst, const char *src) {
    char *ret = dst;
    while ((*dst++ = *src++));
    return ret;
}

// --- String & Memory ---
static int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (uint8_t)*a - (uint8_t)*b;
}
static int strncmp(const char *a, const char *b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i] || !a[i] || !b[i]) return (uint8_t)a[i] - (uint8_t)*b;
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
static void *memset(void *dst, int val, int n) {
    unsigned char *d = (unsigned char*)dst;
    for(int i=0;i<n;i++) d[i] = (unsigned char)val;
    return dst;
}

// --- Keyboard ---
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

// --- VGA Text ---
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

// --- Reboot ---
static void do_reboot() {
    while (inb(0x64) & 0x02) {}
    outb(0x64, 0xFE);
    for (;;) asm volatile ("hlt");
}

// --- ATA HDD ---
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

// --- ZadFS v2.0 ---
#define ZADFS_MAX_FILES      128
#define ZADFS_MAX_FILENAME   32
#define ZADFS_MAX_PATH       128
#define ZADFS_DATA_SIZE      4096
#define ZADFS_MAGIC          0x5ADF55
#define ZADFS_SECTOR_SIZE    512

typedef enum { ZADFS_FILE=0, ZADFS_DIR=1 } zadfs_type_t;
typedef struct zadfs_entry {
    char name[ZADFS_MAX_FILENAME];
    zadfs_type_t type;
    int size;
    int data_offset;
    int parent;
    int first_child;
    int next_sibling;
    uint8_t used;
} zadfs_entry_t;
typedef struct {
    int magic;
    int root_idx;
    int num_entries;
    int next_data_offset;
    int hdd_mode; // 1=use HDD, 0=RAM only
    zadfs_entry_t entries[ZADFS_MAX_FILES];
    char data[ZADFS_DATA_SIZE];
} zadfs_t;
static zadfs_t zadfs;

static void zadfs_init() {
    zadfs.magic = ZADFS_MAGIC;
    zadfs.num_entries = 1;
    zadfs.next_data_offset = 0;
    zadfs.hdd_mode = 0;
    for(int i=0;i<ZADFS_MAX_FILES;i++) zadfs.entries[i].used=0;
    zadfs_entry_t *root = &zadfs.entries[0];
    root->used=1; root->type=ZADFS_DIR; root->parent=-1;
    root->name[0]=0; root->first_child=-1; root->next_sibling=-1; root->size=0;
    zadfs.root_idx = 0;
    for(int i=0;i<ZADFS_DATA_SIZE;i++) zadfs.data[i]=0;
}
static int zadfs_find(const char *path) {
    if(!path || !*path || path[0]!='/') return -1;
    int idx = zadfs.root_idx;
    const char *p = path+1;
    char part[ZADFS_MAX_FILENAME];
    while(*p) {
        int j=0;
        while(*p && *p!='/') part[j++]=*p++;
        part[j]=0;
        if(*p=='/') p++;
        int found=-1;
        int child = zadfs.entries[idx].first_child;
        while(child!=-1) {
            if(zadfs.entries[child].used && strcmp(zadfs.entries[child].name,part)==0) {
                found=child; break;
            }
            child = zadfs.entries[child].next_sibling;
        }
        if(found==-1) return -1;
        idx=found;
    }
    return idx;
}
static int zadfs_split_path(const char *path, char *parent_out, char *name_out) {
    int len = strlen(path);
    if(len==0 || path[0]!='/') return 0;
    int lastslash = -1;
    for(int i=0;i<len;i++) if(path[i]=='/') lastslash=i;
    if(lastslash==0) strcpy(parent_out,"/"); else { strncpy(parent_out,path,lastslash); parent_out[lastslash]=0; }
    strcpy(name_out,path+lastslash+1);
    return 1;
}
static int zadfs_alloc_entry() {
    for(int i=0;i<ZADFS_MAX_FILES;i++) if(!zadfs.entries[i].used) return i;
    return -1;
}
static void zadfs_mkdir(const char *path) {
    char dirpath[ZADFS_MAX_PATH], dname[ZADFS_MAX_FILENAME];
    if(!zadfs_split_path(path, dirpath, dname)) { prints("Invalid path!\n"); return; }
    int parent_idx = zadfs_find(dirpath);
    if(parent_idx==-1 || zadfs.entries[parent_idx].type!=ZADFS_DIR) { prints("Parent dir not found!\n"); return; }
    int child = zadfs.entries[parent_idx].first_child;
    while(child!=-1) {
        if(zadfs.entries[child].used && strcmp(zadfs.entries[child].name,dname)==0) { prints("Already exists!\n"); return; }
        child = zadfs.entries[child].next_sibling;
    }
    int idx = zadfs_alloc_entry();
    if(idx==-1) { prints("Too many files!\n"); return; }
    zadfs_entry_t *e = &zadfs.entries[idx];
    e->used=1; e->type=ZADFS_DIR; e->parent=parent_idx; strncpy(e->name,dname,ZADFS_MAX_FILENAME);
    e->first_child=-1; e->next_sibling=zadfs.entries[parent_idx].first_child;
    zadfs.entries[parent_idx].first_child=idx; zadfs.entries[parent_idx].size++;
    zadfs.num_entries++;
    prints("Directory created!\n");
}
static void zadfs_ls(const char *path, int cwd_idx) {
    int idx;
    if(path && *path) {
        if(path[0] == '/') idx = zadfs_find(path);
        else {
            char abs[ZADFS_MAX_PATH*2];
            if (cwd_idx == zadfs.root_idx) {
                abs[0] = '/'; abs[1] = 0;
                strncat(abs, path, ZADFS_MAX_PATH-2);
            } else {
                int stack[ZADFS_MAX_FILES], sp=0, walk=cwd_idx;
                while(walk!=-1 && walk!=zadfs.root_idx) { stack[sp++] = walk; walk = zadfs.entries[walk].parent; }
                abs[0] = '/'; abs[1] = 0;
                for(int i=sp-1;i>=0;i--) { strncat(abs, zadfs.entries[stack[i]].name, ZADFS_MAX_FILENAME-1); strncat(abs, "/", 1); }
                strncat(abs, path, ZADFS_MAX_PATH-2);
            }
            idx = zadfs_find(abs);
        }
    } else idx = cwd_idx;
    if(idx==-1 || zadfs.entries[idx].type!=ZADFS_DIR) { prints("No such directory!\n"); return; }
    prints("Contents:\n");
    int child = zadfs.entries[idx].first_child, empty=1;
    while(child!=-1) {
        zadfs_entry_t *e = &zadfs.entries[child];
        if(e->used) { prints("  "); prints(e->name); if(e->type==ZADFS_DIR) prints("/"); prints("\n"); empty=0; }
        child = e->next_sibling;
    }
    if(empty) prints("  (empty)\n");
}
static void zadfs_create_file(const char *path, const char *content, int cwd_idx) {
    char dirpath[ZADFS_MAX_PATH], fname[ZADFS_MAX_FILENAME];
    if(!zadfs_split_path(path, dirpath, fname)) { prints("Invalid path!\n"); return; }
    int parent_idx;
    if(dirpath[0] == '/') parent_idx = zadfs_find(dirpath);
    else {
        char abs[ZADFS_MAX_PATH*2];
        if (cwd_idx == zadfs.root_idx) { abs[0] = '/'; abs[1] = 0; strncat(abs, dirpath, ZADFS_MAX_PATH-2); }
        else {
            int stack[ZADFS_MAX_FILES], sp=0, walk=cwd_idx;
            while(walk!=-1 && walk!=zadfs.root_idx) { stack[sp++] = walk; walk = zadfs.entries[walk].parent; }
            abs[0] = '/'; abs[1] = 0;
            for(int i=sp-1;i>=0;i--) { strncat(abs, zadfs.entries[stack[i]].name, ZADFS_MAX_FILENAME-1); strncat(abs, "/", 1); }
            strncat(abs, dirpath, ZADFS_MAX_PATH-2);
        }
        parent_idx = zadfs_find(abs);
    }
    if(parent_idx==-1 || zadfs.entries[parent_idx].type!=ZADFS_DIR) { prints("Parent dir not found!\n"); return; }
    int child = zadfs.entries[parent_idx].first_child;
    while(child!=-1) {
        if(zadfs.entries[child].used && strcmp(zadfs.entries[child].name,fname)==0) { prints("Already exists!\n"); return; }
        child = zadfs.entries[child].next_sibling;
    }
    int idx = zadfs_alloc_entry();
    if(idx==-1) { prints("Too many files!\n"); return; }
    int len = strlen(content);
    if(zadfs.next_data_offset+len > ZADFS_DATA_SIZE) { prints("Out of space!\n"); return; }
    zadfs_entry_t *e = &zadfs.entries[idx];
    e->used=1; e->type=ZADFS_FILE; e->parent=parent_idx; strncpy(e->name,fname,ZADFS_MAX_FILENAME);
    e->size=len; e->data_offset=zadfs.next_data_offset; e->next_sibling=zadfs.entries[parent_idx].first_child;
    zadfs.entries[parent_idx].first_child=idx; zadfs.entries[parent_idx].size++;
    memcpy(zadfs.data+zadfs.next_data_offset, content, len);
    zadfs.next_data_offset+=len; zadfs.num_entries++;
    prints("File created!\n");
}
static void zadfs_cat(const char *path, int cwd_idx) {
    int idx;
    if(path && *path) {
        if(path[0]=='/') idx = zadfs_find(path);
        else {
            char abs[ZADFS_MAX_PATH*2];
            if (cwd_idx == zadfs.root_idx) { abs[0] = '/'; abs[1] = 0; strncat(abs, path, ZADFS_MAX_PATH-2); }
            else {
                int stack[ZADFS_MAX_FILES], sp=0, walk=cwd_idx;
                while(walk!=-1 && walk!=zadfs.root_idx) { stack[sp++] = walk; walk = zadfs.entries[walk].parent; }
                abs[0] = '/'; abs[1] = 0;
                for(int i=sp-1;i>=0;i--) { strncat(abs, zadfs.entries[stack[i]].name, ZADFS_MAX_FILENAME-1); strncat(abs, "/", 1); }
                strncat(abs, path, ZADFS_MAX_PATH-2);
            }
            idx = zadfs_find(abs);
        }
    } else { prints("cat: missing file\n"); return; }
    if(idx==-1 || zadfs.entries[idx].type!=ZADFS_FILE) { prints("No such file!\n"); return; }
    zadfs_entry_t *e = &zadfs.entries[idx];
    for(int i=0;i<e->size;i++) putchar(zadfs.data[e->data_offset+i]);
    prints("\n");
}
static void zadfs_rm(const char *path, int cwd_idx) {
    int idx;
    if(path && *path) {
        if(path[0]=='/') idx = zadfs_find(path);
        else {
            char abs[ZADFS_MAX_PATH*2];
            if (cwd_idx == zadfs.root_idx) { abs[0] = '/'; abs[1] = 0; strncat(abs, path, ZADFS_MAX_PATH-2); }
            else {
                int stack[ZADFS_MAX_FILES], sp=0, walk=cwd_idx;
                while(walk!=-1 && walk!=zadfs.root_idx) { stack[sp++] = walk; walk = zadfs.entries[walk].parent; }
                abs[0] = '/'; abs[1] = 0;
                for(int i=sp-1;i>=0;i--) { strncat(abs, zadfs.entries[stack[i]].name, ZADFS_MAX_FILENAME-1); strncat(abs, "/", 1); }
                strncat(abs, path, ZADFS_MAX_PATH-2);
            }
            idx = zadfs_find(abs);
        }
    } else { prints("rm: missing file/dir\n"); return; }
    if(idx==-1) { prints("No such entry!\n"); return; }
    zadfs_entry_t *e = &zadfs.entries[idx];
    if(e->type==ZADFS_DIR && e->first_child!=-1) { prints("Dir not empty!\n"); return; }
    int par = e->parent;
    int *link = &zadfs.entries[par].first_child;
    while(*link!=-1) {
        if(*link==idx) { *link=e->next_sibling; break; }
        link = &zadfs.entries[*link].next_sibling;
    }
    zadfs.entries[par].size--;
    e->used=0; zadfs.num_entries--;
    prints("Removed!\n");
}
static void zadfs_cp(const char *src, const char *dst, int cwd_idx) {
    int idx;
    if(src && *src) {
        if(src[0]=='/') idx = zadfs_find(src);
        else {
            char abs[ZADFS_MAX_PATH*2];
            if (cwd_idx == zadfs.root_idx) { abs[0] = '/'; abs[1] = 0; strncat(abs, src, ZADFS_MAX_PATH-2); }
            else {
                int stack[ZADFS_MAX_FILES], sp=0, walk=cwd_idx;
                while(walk!=-1 && walk!=zadfs.root_idx) { stack[sp++] = walk; walk = zadfs.entries[walk].parent; }
                abs[0] = '/'; abs[1] = 0;
                for(int i=sp-1;i>=0;i--) { strncat(abs, zadfs.entries[stack[i]].name, ZADFS_MAX_FILENAME-1); strncat(abs, "/", 1); }
                strncat(abs, src, ZADFS_MAX_PATH-2);
            }
            idx = zadfs_find(abs);
        }
    } else { prints("cp: missing src\n"); return; }
    if(idx==-1 || zadfs.entries[idx].type!=ZADFS_FILE) { prints("No such file!\n"); return; }
    zadfs_entry_t *e = &zadfs.entries[idx];
    char buf[ZADFS_DATA_SIZE];
    memcpy(buf, zadfs.data+e->data_offset, e->size);
    buf[e->size]=0;
    zadfs_create_file(dst, buf, cwd_idx);
}
static void zadfs_save_to_hdd() {
    uint8_t *p = (uint8_t*)&zadfs;
    int total = sizeof(zadfs_t);
    int sector = 0;
    for(int o=0;o<total;o+=ZADFS_SECTOR_SIZE,sector++) {
        uint8_t buf[ZADFS_SECTOR_SIZE];
        int len = (o+ZADFS_SECTOR_SIZE<=total?ZADFS_SECTOR_SIZE:total-o);
        memcpy(buf, p+o, len);
        if(len<ZADFS_SECTOR_SIZE) for(int i=len;i<ZADFS_SECTOR_SIZE;i++) buf[i]=0;
        ata_write_sector(sector, buf);
    }
}
static void zadfs_load_from_hdd() {
    uint8_t *p = (uint8_t*)&zadfs;
    int total = sizeof(zadfs_t);
    int sector = 0;
    for(int o=0;o<total;o+=ZADFS_SECTOR_SIZE,sector++) {
        uint8_t buf[ZADFS_SECTOR_SIZE];
        ata_read_sector(sector, buf);
        int len = (o+ZADFS_SECTOR_SIZE<=total?ZADFS_SECTOR_SIZE:total-o);
        memcpy(p+o, buf, len);
    }
    if(zadfs.magic!=ZADFS_MAGIC) { prints("Invalid FS, formatting.\n"); zadfs_init(); }
}

// --- Path helper for prompt ---
static void zadfs_get_cwd_path(int idx, char *out) {
    if(idx==0) { out[0] = '/'; out[1] = 0; return; }
    int stack[ZADFS_MAX_FILES], sp=0, walk=idx;
    while(walk!=-1 && walk!=zadfs.root_idx) { stack[sp++] = walk; walk = zadfs.entries[walk].parent; }
    out[0] = '/'; out[1] = 0;
    for(int i=sp-1;i>=0;i--) { strncat(out, zadfs.entries[stack[i]].name, ZADFS_MAX_FILENAME-1); strncat(out, "/", 1); }
    int l = strlen(out); if(l>1 && out[l-1]=='/') out[l-1]=0;
}

// --- Main kernel logic, command loop ---
void kernel_main() {
    clear_screen(); enable_cursor();
    prints("Welcome to XylenOS 0.2.5\nType 'help' for commands.\n");

    int hdd_found = detect_hdd();
    zadfs_init();
    if(hdd_found) {
        prints("HDD detected! Type 'loadfs' to load, or 'formatfs' to format new FS, or any other key for RAM only.\n~# ");
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
        if(strcmp(cmd,"loadfs")==0) {
            zadfs_load_from_hdd();
            zadfs.hdd_mode = 1;
            prints("FS loaded from HDD.\n");
        } else if(strcmp(cmd,"formatfs")==0) {
            zadfs_init();
            zadfs.hdd_mode = 1;
            zadfs_save_to_hdd();
            prints("FS initialized and saved to HDD.\n");
        } else {
            zadfs.hdd_mode = 0;
            prints("Using RAM only.\n");
        }
    } else {
        zadfs_init();
        zadfs.hdd_mode = 0;
        prints("No HDD found. Using RAM only.\n");
    }

    int cwd_idx = zadfs.root_idx;

    while(1) {
        char cwd[ZADFS_MAX_PATH*2];
        zadfs_get_cwd_path(cwd_idx, cwd);
        prints(cwd); prints(" ~# ");
        char line[256]; int len=0;
        while(1) {
            char c=get_key();
            if(c=='\n'){ putchar('\n'); line[len]=0; break; }
            if(c=='\b' && len>0) {
                len--;
                if (cursor_x > 0) {
                    cursor_x--;
                    VGA_MEM[cursor_y*VGA_WIDTH + cursor_x] = (uint16_t)(' ' | (VGA_COLOR << 8));
                    move_cursor();
                }
            }
            else if(len<(int)sizeof(line)-1 && c!='\b') { line[len++]=c; putchar(c); }
        }
        if(strcmp(line,"help")==0) {
            prints("Commands: help, clear, time, ls [dir], mkdir <dir>, rmdir <dir>, touch <file> <content>, cat <file>, rm <file|dir>, cp <src> <dst>, cd <dir>, savefs, loadfs, reboot, version\n");
        } else if(strcmp(line,"clear")==0) {
            clear_screen();
        } else if(strcmp(line,"version")==0) {
        	prints("XylenOS Pre-Alpha 0.2.5 Public Testing Build Revision-4");
        	putchar('\n');
        } else if(strcmp(line,"time")==0) {
            prints("Current date and time: "); print_time();
        } else if(strcmp(line,"reboot")==0) {
            prints("Rebooting...\n"); do_reboot();
        } else if(strncmp(line,"ls",2)==0) {
            const char *p=line+2; while(*p==' ') p++;
            if(!*p) zadfs_ls(NULL, cwd_idx); else zadfs_ls(p, cwd_idx);
        } else if(strncmp(line,"mkdir ",6)==0) {
            zadfs_mkdir(line+6);
            if(zadfs.hdd_mode) zadfs_save_to_hdd();
        } else if(strncmp(line,"rmdir ",6)==0) {
            zadfs_rm(line+6, cwd_idx);
            if(zadfs.hdd_mode) zadfs_save_to_hdd();
        } else if(strncmp(line,"touch ",6)==0) {
            const char *p=line+6; while(*p==' ') p++;
            const char *fname=p, *content=p;
            while(*content && *content!=' ') content++;
            if(*content==' ') {
                int fnlen=content-fname;
                char f[ZADFS_MAX_PATH];
                strncpy(f,fname,fnlen); f[fnlen]=0;
                content++; while(*content==' ') content++;
                zadfs_create_file(f,content,cwd_idx);
                if(zadfs.hdd_mode) zadfs_save_to_hdd();
            } else {
                prints("Usage: touch <file> <content>\n");
            }
        } else if(strncmp(line,"cat ",4)==0) {
            zadfs_cat(line+4, cwd_idx);
        } else if(strncmp(line,"rm ",3)==0) {
            zadfs_rm(line+3, cwd_idx);
            if(zadfs.hdd_mode) zadfs_save_to_hdd();
        } else if(strncmp(line,"cp ",3)==0) {
            const char *p=line+3; while(*p==' ') p++;
            const char *src=p, *dst=p;
            while(*dst && *dst!=' ') dst++;
            if(*dst==' ') {
                int slen=dst-src;
                char s[ZADFS_MAX_PATH];
                strncpy(s,src,slen); s[slen]=0;
                dst++; while(*dst==' ') dst++;
                zadfs_cp(s,dst,cwd_idx);
                if(zadfs.hdd_mode) zadfs_save_to_hdd();
            } else prints("Usage: cp <src> <dst>\n");
        } else if(strncmp(line,"cd ",3)==0) {
            const char *p = line+3; while(*p==' ') p++;
            int new_idx;
            if(*p==0) new_idx = zadfs.root_idx;
            else if(p[0]=='/') new_idx = zadfs_find(p);
            else {
                char abs[ZADFS_MAX_PATH*2];
                if (cwd_idx == zadfs.root_idx) { abs[0] = '/'; abs[1] = 0; strncat(abs, p, ZADFS_MAX_PATH-2); }
                else {
                    int stack[ZADFS_MAX_FILES], sp=0, walk=cwd_idx;
                    while(walk!=-1 && walk!=zadfs.root_idx) { stack[sp++] = walk; walk = zadfs.entries[walk].parent; }
                    abs[0] = '/'; abs[1] = 0;
                    for(int i=sp-1;i>=0;i--) { strncat(abs, zadfs.entries[stack[i]].name, ZADFS_MAX_FILENAME-1); strncat(abs, "/", 1); }
                    strncat(abs, p, ZADFS_MAX_PATH-2);
                }
                new_idx = zadfs_find(abs);
            }
            if(new_idx == -1 || zadfs.entries[new_idx].type != ZADFS_DIR) { prints("No such directory!\n"); }
            else cwd_idx = new_idx;
        } else if(strcmp(line,"savefs")==0) {
            zadfs_save_to_hdd(); prints("FS saved!\n");
        } else if(strcmp(line,"loadfs")==0) {
            zadfs_load_from_hdd(); prints("FS loaded!\n");
        } else if(len>0) {
            prints("Unknown command\n");
        }
    }
}
