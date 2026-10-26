#pragma once

#include "stdint.h"
#include "stddef.h"
#include "mm_layout.h"

#define PAGE_SIZE 4096

struct free_page{
    struct free_page* next;
};


void page_init();
void* alloc_page();
void free_page(void* ptr);