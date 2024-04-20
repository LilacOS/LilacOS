#include <stdarg.h>
#include "types.h"
#include "ulib.h"
#include "syscall.h"

static char digits[] = "0123456789abcdef";

static void printint(int xx, int base, int sign) {
    char buf[16];
    int i;
    uint x;

    if (sign && (sign = xx < 0))
        x = -xx;
    else
        x = xx;

    i = 0;
    do {
        buf[i++] = digits[x % base];
    } while ((x /= base) != 0);

    if (sign)
        buf[i++] = '-';

    while (--i >= 0)
        putchar(buf[i]);
}

static void printptr(usize x) {
    int i;
    putchar('0');
    putchar('x');
    for (i = 0; i < (sizeof(usize) * 2); i++, x <<= 4)
        putchar(digits[x >> (sizeof(usize) * 8 - 4)]);
}

void __printf(char *fmt, va_list ap) {
    int i;
    char c, *s;

    if (!fmt)
        panic("[printf] null fmt");

    for (i = 0; (c = fmt[i] & 0xff) != 0; i++) {
        if (c != '%') {
            putchar(c);
            continue;
        }
        c = fmt[++i] & 0xff;
        if (c == 0)
            break;
        switch (c) {
        case 'd':
            printint(va_arg(ap, int), 10, 1);
            break;
        case 'x':
            printint(va_arg(ap, int), 16, 1);
            break;
        case 'p':
            printptr(va_arg(ap, usize));
            break;
        case 's':
            if ((s = va_arg(ap, char *)) == 0)
                s = "(null)";
            for (; *s; s++)
                putchar(*s);
            break;
        case '%':
            putchar('%');
            break;
        default:
            putchar('%');
            putchar(c);
            break;
        }
    }
}

void printf(char *fmt, ...) {
    if (!fmt)
        panic("[printf] null fmt");
    va_list ap;
    va_start(ap, fmt);
    __printf(fmt, ap);
    va_end(ap);
}

void panic(char *fmt, ...) {
    if (fmt) {
        va_list ap;
        va_start(ap, fmt);
        __printf(fmt, ap);
        va_end(ap);
    }
    exit();
}