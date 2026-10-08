
#include "../include/uart.h"
#include "../../../global/memory_map.h"

#include "stdint.h"

void uart_init(){
  
}

void uart_putc(const char c){
  int LSR = *((volatile unsigned char*)(UART_START + 0x05));
  
  while(!(LSR & 0x20)); // wait until ready
  *((volatile unsigned char*)(UART_START + 0x00)) = c; // after waiting, print   
}

void uart_puts(const char* s){
  while(*s){
    uart_putc(*s++);
  }
}

#define UART_RBR  (*(volatile uint8_t *)(UART_START + 0))   // Receive Buffer
#define UART_LSR  (*(volatile uint8_t *)(UART_START + 5))   // Line Status
#define LSR_DR    0x01                                      // Data Ready


char uart_getc(){
    while (!(UART_LSR & LSR_DR)){
        ; // wait until buffer contains a char
    }
    return UART_RBR;
}

int uart_gets(char* buf, int max){
    int i=0;

    while(1){
        char c = uart_getc();
        
        if(c == '\r' || c == '\n'){
            uart_puts("\r\n");
            break;
        }

        if ((c == '\b' || c == 0x7F) && i > 0){ // backspace / DEL
            i--;
            uart_puts("\b \b");
            continue;
        }

        if(i < max - 1 && c >= ' ') {
            buf[i++] = c;
            uart_putc(c);                       // echo
        }
    }

    buf[i] = '\0';
    return i;
}