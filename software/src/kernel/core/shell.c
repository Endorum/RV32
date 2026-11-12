#include "shell.h"
#include "fat12.h"
#include "kmalloc.h"
#include "kstdio.h"
#include "uart.h"
#include "kstring.h"

static struct ShellCtx shellctx;
    

void shell_ls();
void shell_cd(const char* dir_name);
void shell_cat(const char* filename);

void print_help(){
    kprintf("- ls               : lists content of current directory\n");
    kprintf("- cd  <directory>  : change directory\n");
    kprintf("- cat <file.ext>   : writes contets of file to console \n");
    
    kprintf("- exit             : exits the shell\n");
    kprintf("- help             : shows this screen\n");
}

void shell_ls(){
    // somethig like this
    fat12_read_dir(shellctx.cwd_cluster);
}

void shell_cd(const char* dir_name){
    
    if(strcmp(dir_name, "/") == 0){
        shellctx.cwd_cluster = 0;
        shellctx.path[0] = '/';
        shellctx.path[1] = '\0';
        return;
    }
    
    
    struct fat12_entry entry;    
    
    // if file exists and its a directory
    if(fat12_find(shellctx.cwd_cluster, dir_name, &entry) && (entry.attr & FAT_ATTR_DIR)){
        shellctx.cwd_cluster = entry.first_cluster;
        char name[13];
        fat_name_to_str(&entry, name);
        
        if(strcmp(name, ".") == 0){
            // do nothing to the path
        }
        else if(strcmp(name, "..") == 0){
            // find last / remove everything in front of it
            for(int i=strlen(shellctx.path) - 1; i>=0; i--){
                if(shellctx.path[i] == '/'){
                    shellctx.path[i + 1] = '\0';
                    break;
                }
            }
            
        }
        
        else{
            strcat(shellctx.path, name);
        }
        return;
        
    }else{
        kprintf("Could not find directory '%s'\n", dir_name);
    }
}

void shell_cat(const char* filename){
    // something like this
    struct fat12_entry entry;


    if(fat12_find(shellctx.cwd_cluster, filename, &entry)){
        uint8_t* buf = kmalloc(entry.size + 1);
        fat12_read_file(&entry, buf, entry.size);
        buf[entry.size] = '\0';

        kprintf("%s", buf);

        kfree(buf);
    }else{
        kprintf("Could not find file '%s'\n", filename);
    } 
}

int tokenize(char* s, char** argv, int max){
    int argc = 0;

    while(*s && argc < max){
        while(*s == ' ') s++; // skip space
        if(!*s) break;

        argv[argc++] = s;

        if(*s == '"'){
            s++;
            while(*s && *s != '"') s++; // until closing "

        }else{
            while(*s && *s != ' ') s++; // until next space
        }

        if (*s) *s++ = '\0'; // terminate token
    }

    return argc;
}


int shell(){
    
    // 1. show current path and then '>'
    // eg A:/> or A:/folder/> etc.
    kprintf("%c:%s>", shellctx.letter, shellctx.path);
    
    // 2. get user input wait
    char input[256];
    uart_gets(input, 256);
    
    // 3. eval input
    char* argv[4];
    int argc = tokenize(input, argv, 4);
    if(argc == 0) return 0;
        
    const char* cmd = argv[0];
    char *arg1 = argc > 1 ? argv[1] : NULL;
    char *arg2 = argc > 2 ? argv[2] : NULL;
    char *arg3 = argc > 3 ? argv[3] : NULL;
    
    if(strcmp(cmd, "ls") == 0) {
        shell_ls();
    }
    else if(strcmp(cmd, "cd") == 0) {
        if(arg1)
            shell_cd(arg1);
    }
    
    else if(strcmp(cmd, "cat") == 0){
        if(arg1)
            shell_cat(arg1);
    }
    
    else if(strcmp(cmd, "exit") == 0){
        return 1;
    }
    
    else if(strcmp(cmd, "help") == 0){
        print_help();
    }


    
    else {
        kprintf("unknown command '%s'\n", cmd);
    }
    
    // update shell context
    
    return 0;
}

void start_shell(){
    
    kprintf("Entering Kernel Shell...\n");
    
    
    shellctx.letter = 'A';
    shellctx.cwd_cluster = 0; // start at root
    shellctx.path[0] = '/';
    shellctx.path[1] = '\0';
    
    
    while (1) {
        int code = shell();
        
        if(code > 0) break;
    }
    
}