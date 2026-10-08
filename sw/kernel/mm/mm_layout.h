#pragma once

#include "ram_layout.h"

/*

RAM_USER_START                HEAP_END = PAGES_START                PAGES_END
     |                                |                                  |
     [ Heap for kmalloc (HEAP_SIZE)   ][ pages for alloc_page            ]

*/

#define HEAP_START  RAM_USER_START
#define HEAP_SIZE   0x800000 // 8 MiB
#define HEAP_END    (HEAP_START + HEAP_SIZE)

#define PAGES_START HEAP_END
#define PAGES_END   (RAM_USER_START + RAM_USER_SIZE)
#define PAGES_SIZE  (PAGES_END - PAGES_START)