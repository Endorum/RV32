#pragma once

void uart_init();
void uart_putc(const char c);
void uart_puts(const char* s);


char uart_getc();
int uart_gets(char* buf, int max);