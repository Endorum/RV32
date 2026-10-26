#include "kstdio.h"
#include "panic.h"
#include "page.h"

struct free_page* free_page_list;
uint32_t free_pages = 0;

void page_init(){

    kprintf("page init...\n");

    for(uint32_t ptr=PAGES_START; ptr < PAGES_END; ptr += PAGE_SIZE){
        free_page((void*) ptr);
    }

    uint32_t exp = (PAGES_SIZE / PAGE_SIZE);
    if(free_pages != exp){
        kprintf("page_init failed, expected %d free pages, got %d\n", exp, free_pages);
        panic();
    }

}

void *alloc_page() {

    struct free_page* p = free_page_list;

    if(p == NULL) return NULL;

    free_page_list = p->next;

    free_pages--;

    return p;
}

void free_page(void *ptr) {

    struct free_page* p = (struct free_page*)(ptr);

    if((uint32_t)ptr % PAGE_SIZE != 0){ // not aligned to pagesize
        kprintf("free_page on not aligned pointer\n");
        panic();
    }

    if((uint32_t)ptr >= PAGES_END || (uint32_t)ptr < PAGES_START){
        kprintf("free_page on out of bounds pointer\n");
        panic();
    }

    free_pages++;

    p->next = free_page_list;
    free_page_list = p;

}