#include "../inc/lux.h"
#include <stdarg.h>

/* Minimal vsnprint implementation for liblux */

static int digit(int val) {
    if (val < 10) return '0' + val;
    return 'a' + val - 10;
}

static char *printnum(char *buf, char *end, unsigned long long val, int base, int width, int pad) {
    char tmp[64];
    int i = 0;
    
    if (val == 0) tmp[i++] = '0';
    else {
        while (val != 0) {
            tmp[i++] = digit(val % base);
            val /= base;
        }
    }
    
    while (i < width && i < 63) tmp[i++] = pad;
    
    while (i > 0 && buf < end) {
        *buf++ = tmp[--i];
    }
    return buf;
}

int vsnprint(char *buf, int len, const char *fmt, va_list args) {
    char *p = buf;
    char *end = buf + len - 1; /* Leave room for null */
    const char *f = fmt;
    
    if (len <= 0) return 0;
    
    while (*f && p < end) {
        if (*f != '%') {
            *p++ = *f++;
            continue;
        }
        f++; // Skip %
        
        int width = 0;
        int pad = ' ';
        if (*f == '0') {
            pad = '0';
            f++;
        }
        while (*f >= '0' && *f <= '9') {
            width = width * 10 + (*f - '0');
            f++;
        }
        
        if (*f == 'l') f++; // Ignore lengths for now (assume largest)
        if (*f == 'l') f++;
        
        switch (*f) {
            case 'd': {
                long long val = va_arg(args, long long); // simplified varargs promotion
                if (val < 0) {
                    if (p < end) *p++ = '-';
                    val = -val;
                }
                p = printnum(p, end, (unsigned long long)val, 10, width, pad);
                break;
            }
            case 'u': {
                unsigned long long val = va_arg(args, unsigned long long);
                p = printnum(p, end, val, 10, width, pad);
                break;
            }
            case 'x': 
            case 'p': {
                unsigned long long val = va_arg(args, unsigned long long);
                if (*f == 'p' && p + 2 < end) { *p++ = '0'; *p++ = 'x'; }
                p = printnum(p, end, val, 16, width, pad);
                break;
            }
            case 's': {
                char *s = va_arg(args, char *);
                if (!s) s = "(null)";
                while (*s && p < end) *p++ = *s++;
                break;
            }
            case 'c': {
                int c = va_arg(args, int);
                *p++ = (char)c;
                break;
            }
            case '%': {
                *p++ = '%';
                break;
            }
            default: {
                *p++ = '?'; // Unsupported format
                break;
            }
        }
        f++;
    }
    *p = 0;
    return p - buf;
}

int snprint(char *buf, int len, const char *fmt, ...) {
    va_list args;
    int n;
    va_start(args, fmt);
    n = vsnprint(buf, len, fmt, args);
    va_end(args);
    return n;
}
