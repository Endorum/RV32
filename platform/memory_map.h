// Hardware parts of the memory map

#ifndef MAP_H
#define MAP_H

#define ROM_START 0x00000000
#define ROM_SIZE  0x10000    

#define HD_START 0x10000000
#define HD_SIZE  0x10000000 

#define RAM_START 0x20000000
#define RAM_SIZE  0x10000000 

#define CLINT_START 0x40000000 
#define CLINT_SIZE  0x10000     

#define UART_START  0x50000000
#define UART_SIZE   0x100       

#define SCREEN_START 0x60000000
#define SCREEN_SIZE  0xE100

#define FLOPPYB_START  0x70000000
#define FLOPPYB_SIZE   0x00168000 // 1.44mb

#define FLOPPYA_START  0x70100000
#define FLOPPYA_SIZE   0x00168000 // 1.44mb


#endif // MAP_H
