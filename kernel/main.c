#include "types.h"
#include "../lib/string.h"
#include "../drivers/keyboard.h"
#include "../drivers/vga.h"
#include "../drivers/cmos.h"
#include "../drivers/ata.h"
#include "../system/reboot.h"
#include "../fs/zadfs.h"
#include <stdint.h>

void kernel_main() {
    clear_screen(); 
    enable_cursor();
    prints("Welcome to XylenOS 0.3\nType 'help' for commands.\n");

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
                    putchar('\b');  // Let putchar handle the display
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
                putchar('\b');  // Let putchar handle the display
            }
            else if(len<(int)sizeof(line)-1 && c!='\b') { line[len++]=c; putchar(c); }
        }
        if(strcmp(line,"help")==0) {
            prints("Commands: help, clear, time, ls [dir], mkdir <dir>, rmdir <dir>, touch <file> <content>, cat <file>, rm <file|dir>, cp <src> <dst>, cd <dir>, savefs, loadfs, reboot, version\n");
        } else if(strcmp(line,"clear")==0) {
            clear_screen();
        } else if(strcmp(line,"version")==0) {
        	prints("XylenOS Pre-Alpha 0.3 Official Public Build Revision-5");
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
