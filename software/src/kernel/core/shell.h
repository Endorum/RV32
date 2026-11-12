#pragma once

#include <stdint.h>

/*


UNIX like commands

- ls 
- cat <filename.ext>


- help

*/


struct ShellCtx{
    char letter;
    uint16_t cwd_cluster; // THE current directoy: 0 = root
    char path[256]; // just to print
};

int shell();
void start_shell();


