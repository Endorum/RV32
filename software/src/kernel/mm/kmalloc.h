#pragma once

#include "stdint.h"
#include "stddef.h"

#include "mm_layout.h"




struct Block{
    uint32_t size; // size INCLUDES the header size of 8 bytes
    struct Block* next;
};

void kmalloc_dump();
void kmalloc_init();
void* kmalloc(size_t size);
void kfree(void* ptr);