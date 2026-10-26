#include "kmalloc.h"

#include "kstdio.h"
#include "panic.h"


struct Block* free_block_list;
uint32_t free_blocks = 0; 

void kmalloc_dump(){
    struct Block* current = free_block_list;

    while(current){
        kprintf("Block: address: %08X, size: %d, next: %08X\n", (uint32_t)current, current->size, (uint32_t)current->next);

        current = current->next;
    }

    return;
}

void kmalloc_init(){

    kprintf("kmalloc init...\n");

    free_block_list = (struct Block*)(HEAP_START);
    free_block_list->next = NULL;
    free_block_list->size = HEAP_SIZE;

}

void *kmalloc(size_t size) {

    // round up to nearest multiple of 8
    size_t n = (size_t)(size + 7) & ~7;
    size_t need = n + sizeof(struct Block);
    
    struct Block* current   = free_block_list;
    struct Block* prev      = NULL;
    while(current){

        if(current->size >= need){
            // found fitting block


            uint32_t rest = current->size - need;

            if(rest >= 16){ // rest is big enough for another block plus some
                // split
                struct Block* new = (struct Block*)((uint8_t*)current + need);
                new->size = rest;
                new->next = current->next;
                current->size = need;

                // replace current in the list with new
                if(prev == NULL) free_block_list = new;
                else             prev->next = new;   

            }
            
            else{
                // take whole block
                if(prev == NULL) free_block_list = current->next;
                else             prev->next = current->next;
            }

            return current + 1;
        }

        prev = current;
        current = current->next;
    }
    
    // didnt find any fitting one
    return NULL;
}

void kfree(void *ptr) {

    if(ptr == NULL) return;

    struct Block* block = (struct Block*)ptr - 1;

    struct Block* prev = NULL;
    struct Block* current = free_block_list;

    // run while the free blocks are before block
    while(current && current < block){
        prev = current;
        current = current->next;
    }

    // prev is the last free gap bevor the block 
    // current is the first free gap behind that

    // place the block between prev and current
    block->next = current;
    if(prev == NULL)    free_block_list = block;
    else                prev->next = block;

    // glue to the right
    if(current && (uint8_t*)block + block->size == (uint8_t*)current){
        block->size += current->size;
        block->next  = current->next;
    }

    // glue to the left
    if(prev && (uint8_t*)prev + prev->size == (uint8_t*)block){
        prev->size += block->size;
        prev->next  = block->next;
    }


}
