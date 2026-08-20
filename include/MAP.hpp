#ifndef MAP_H
#define MAP_H

#define ROM_START 0x00000000
#define ROM_SIZE 0x10000 // 64kb

#define HD_START 0x10000000
#define HD_SIZE  0x10000000 // 256mb size is the MAX SIZE, the file size changes dynamically

#define RAM_START 0x20000000
#define RAM_SIZE 0x10000000 // 256mb

#define CLINT_START 0x40000000 
#define CLINT_SIZE  0x10000 // 64kb

#define UART_START 0x50000000
#define UART_SIZE 0x100 // 256 Bytes


#endif // MAP_H
