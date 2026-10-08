#include "kstdio.h"
#include "uart.h"

#include "stdint.h"

#include "kstring.h"

int print_dec(int v) {
    char buf[10];               // 2^32 hat max. 10 Dezimalstellen
    int i = 0, n = 0;
    unsigned u;

    if (v < 0) {
        uart_putc('-');
        n++;
        u = 0u - (unsigned)v;   // auch für INT_MIN korrekt
    } else {
        u = (unsigned)v;
    }

    do {
        buf[i++] = '0' + (u % 10);
        u /= 10;
    } while (u);

    while (i--) { uart_putc(buf[i]); n++; }
    return n;
}


int print_hex(unsigned v, int upper, int width) {
    char buf[8];
    int i = 0, n = 0;

    do {
        int d = v & 0xF;
        buf[i++] = d < 10 ? '0' + d : (upper ? 'A' : 'a') + d - 10;
        v >>= 4;
    } while (v);

    for (int pad = width - i; pad > 0; pad--) { uart_putc('0'); n++; }
    while (i--) { uart_putc(buf[i]); n++; }
    return n;
}

static int put_str(const char *s) {
    int n = 0;
    if (!s) s = "(null)";
    while (*s) { uart_putc(*s++); n++; }
    return n;
}

int kvprintf(const char *fmt, va_list ap) {
    int count = 0;

    while (*fmt) {
        if (*fmt != '%') {
            uart_putc(*fmt++);
            count++;
            continue;
        }
        fmt++;                       // skip '%'

        // Flags/Breite VOR der Conversion: %08x
        int zero  = 0;
        int width = 0;
        if (*fmt == '0') { zero = 1; fmt++; }
        while (*fmt >= '0' && *fmt <= '9')
            width = width * 10 + (*fmt++ - '0');

        if (!*fmt) break;            // '%' am Ende

        switch (*fmt) {
            case 'd': count += print_dec(va_arg(ap, int));                    break;
            case 'X': count += print_hex(va_arg(ap, unsigned), 1, zero ? width : 0); break;
            case 'x': count += print_hex(va_arg(ap, unsigned), 0, zero ? width : 0); break;
            case 's': count += put_str(va_arg(ap, const char *));             break;
            case 'c': uart_putc(va_arg(ap, int)); count++;                    break;
            case 'p':
                count += put_str("0x");
                count += print_hex((uintptr_t)va_arg(ap, void *), 0, 8);
                break;
            case '%': uart_putc('%'); count++;                                break;
            default:  uart_putc('%'); uart_putc(*fmt); count += 2;            break;
        }
        fmt++;
    }
    return count;
}

int kprintf(const char *fmt, ...) {

    va_list ap;
    va_start(ap, fmt);
    int r = kvprintf(fmt, ap);
    va_end(ap);

    return r;
}

uint32_t parse_hex(const char *s, int* bytes) {
    uint32_t v = 0;
    int n = 0;

    if(!s) return 0;
    if(bytes) *bytes = 4;



    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        s += 2;

    
    for(;; s++, n++){
        char c = *s;
        if      (c >= '0' && c <= '9') v = (v << 4) | (c - '0');
        else if (c >= 'a' && c <= 'f') v = (v << 4) | (c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') v = (v << 4) | (c - 'A' + 10);
        else break;
    }


    if(bytes) *bytes = (n + 1) / 2;
    return v;
}
