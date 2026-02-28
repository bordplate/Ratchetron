/*
 * Copyright (c) 1995 Patrick Powell.
 *
 * This code is based on code written by Patrick Powell <papowell@astart.com>.
 * It may be used for any purpose as long as this notice remains intact on all
 * source code distributions.
 */

/*
 * Copyright (c) 2008 Holger Weiss.
 *
 * This version of the code is maintained by Holger Weiss <holger@jhweiss.de>.
 * My changes to the code may freely be used, modified and/or redistributed for
 * any purpose.  It would be nice if additions and fixes to this file (including
 * trivial code cleanups) would be sent back in order to let me include them in
 * the version available at <http://www.jhweiss.de/software/snprintf.html>.
 * However, this is not a requirement for using or redistributing (possibly
 * modified) versions of this file, nor is leaving this notice intact mandatory.
 */

#include <cell/cell_fs.h>
#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

#define ULLONG          unsigned long long int
#define ULLONG_MAX      ULONG_MAX
#define LLONG           long long int
#define INTMAX_T        intmax_t
#define UINTMAX_T       uintmax_t
#define UINTMAX_MAX     ULLONG_MAX
#define UINTPTR_T       uintptr_t
#define PTRDIFF_T       ptrdiff_t
#define UPTRDIFF_T      ptrdiff_t
#define SSIZE_T         size_t

#define MAX_CONVERT_LENGTH  43

#define PRINT_S_DEFAULT     0
#define PRINT_S_FLAGS       1
#define PRINT_S_WIDTH       2
#define PRINT_S_DOT         3
#define PRINT_S_PRECISION   4
#define PRINT_S_MOD         5
#define PRINT_S_CONV        6

#define PRINT_F_MINUS       (1 << 0)
#define PRINT_F_PLUS        (1 << 1)
#define PRINT_F_SPACE       (1 << 2)
#define PRINT_F_NUM         (1 << 3)
#define PRINT_F_ZERO        (1 << 4)
#define PRINT_F_UP          (1 << 6)
#define PRINT_F_UNSIGNED    (1 << 7)

#define PRINT_C_CHAR        1
#define PRINT_C_SHORT       2
#define PRINT_C_LONG        3
#define PRINT_C_LLONG       4
#define PRINT_C_SIZE        6
#define PRINT_C_PTRDIFF     7
#define PRINT_C_INTMAX      8

#ifndef MAX
#define MAX(x, y) ((x >= y) ? x : y)
#endif
#ifndef CHARTOINT
#define CHARTOINT(ch) (ch - '0')
#endif
#ifndef ISDIGIT
#define ISDIGIT(ch) ('0' <= (unsigned char)ch && (unsigned char)ch <= '9')
#endif

#define OUTCHAR(str, len, size, ch)  \
{                                    \
    if (++len < size)                \
        str[len-1] = ch;             \
}

static void fmtstr(char *, size_t *, size_t, const char *, int, int, int);
static void fmtint(char *, size_t *, size_t, INTMAX_T, int, int, int, int);
static int  convert(UINTMAX_T, char *, size_t, int, int);

int vsnprintf(char *str, size_t size, const char *format, va_list args)
{
    if (!format)
        return 0;

    INTMAX_T value;
    unsigned char cvalue;
    const char *strvalue;
    size_t len = 0;
    int overflow = 0;
    int base = 0;
    int cflags = 0;
    int flags = 0;
    int width = 0;
    int precision = -1;
    int state = PRINT_S_DEFAULT;
    char ch = *format++;

    if ((str == NULL) && (size != 0))
        size = 0;

    while (ch != '\0')
        switch (state) {
        case PRINT_S_DEFAULT:
            if (ch == '%')
                state = PRINT_S_FLAGS;
            else
                OUTCHAR(str, len, size, ch);
            ch = *format++;
            break;
        case PRINT_S_FLAGS:
            switch (ch) {
            case '-': flags |= PRINT_F_MINUS; ch = *format++; break;
            case '+': flags |= PRINT_F_PLUS;  ch = *format++; break;
            case ' ': flags |= PRINT_F_SPACE; ch = *format++; break;
            case '#': flags |= PRINT_F_NUM;   ch = *format++; break;
            case '0': flags |= PRINT_F_ZERO;  ch = *format++; break;
            default:  state = PRINT_S_WIDTH;  break;
            }
            break;
        case PRINT_S_WIDTH:
            if (ISDIGIT(ch)) {
                ch = CHARTOINT(ch);
                if (width > (INT_MAX - ch) / 10) { overflow = 1; goto out; }
                width = 10 * width + ch;
                ch = *format++;
            } else if (ch == '*') {
                if ((width = va_arg(args, int)) < 0) {
                    flags |= PRINT_F_MINUS;
                    width = -width;
                }
                ch = *format++;
                state = PRINT_S_DOT;
            } else
                state = PRINT_S_DOT;
            break;
        case PRINT_S_DOT:
            if (ch == '.') { state = PRINT_S_PRECISION; ch = *format++; }
            else             state = PRINT_S_MOD;
            break;
        case PRINT_S_PRECISION:
            if (precision == -1) precision = 0;
            if (ISDIGIT(ch)) {
                ch = CHARTOINT(ch);
                if (precision > (INT_MAX - ch) / 10) { overflow = 1; goto out; }
                precision = 10 * precision + ch;
                ch = *format++;
            } else if (ch == '*') {
                if ((precision = va_arg(args, int)) < 0) precision = -1;
                ch = *format++;
                state = PRINT_S_MOD;
            } else
                state = PRINT_S_MOD;
            break;
        case PRINT_S_MOD:
            switch (ch) {
            case 'h':
                ch = *format++;
                if (ch == 'h') { ch = *format++; cflags = PRINT_C_CHAR; }
                else             cflags = PRINT_C_SHORT;
                break;
            case 'l':
                ch = *format++;
                if (ch == 'l') { ch = *format++; cflags = PRINT_C_LLONG; }
                else             cflags = PRINT_C_LONG;
                break;
            case 'j': cflags = PRINT_C_INTMAX;  ch = *format++; break;
            case 't': cflags = PRINT_C_PTRDIFF; ch = *format++; break;
            case 'z': cflags = PRINT_C_SIZE;    ch = *format++; break;
            }
            state = PRINT_S_CONV;
            break;
        case PRINT_S_CONV:
            switch (ch) {
            case 'd':
            case 'i':
                switch (cflags) {
                case PRINT_C_CHAR:    value = (signed char)va_arg(args, int);   break;
                case PRINT_C_SHORT:   value = (short int)va_arg(args, int);     break;
                case PRINT_C_LONG:    value = va_arg(args, long int);           break;
                case PRINT_C_LLONG:   value = va_arg(args, LLONG);              break;
                case PRINT_C_SIZE:    value = va_arg(args, SSIZE_T);            break;
                case PRINT_C_INTMAX:  value = va_arg(args, INTMAX_T);           break;
                case PRINT_C_PTRDIFF: value = va_arg(args, PTRDIFF_T);          break;
                default:              value = va_arg(args, int);                break;
                }
                fmtint(str, &len, size, value, 10, width, precision, flags);
                break;
            case 'X': flags |= PRINT_F_UP; 
            case 'x': base = 16;
            case 'o': if (base == 0) base = 8;
            case 'u':
                if (base == 0) base = 10;
                flags |= PRINT_F_UNSIGNED;
                switch (cflags) {
                case PRINT_C_CHAR:    value = (unsigned char)va_arg(args, unsigned int);       break;
                case PRINT_C_SHORT:   value = (unsigned short int)va_arg(args, unsigned int);  break;
                case PRINT_C_LONG:    value = va_arg(args, unsigned long int);                 break;
                case PRINT_C_LLONG:   value = va_arg(args, ULLONG);                            break;
                case PRINT_C_SIZE:    value = va_arg(args, size_t);                            break;
                case PRINT_C_INTMAX:  value = va_arg(args, UINTMAX_T);                         break;
                case PRINT_C_PTRDIFF: value = va_arg(args, UPTRDIFF_T);                        break;
                default:              value = va_arg(args, unsigned int);                      break;
                }
                fmtint(str, &len, size, value, base, width, precision, flags);
                break;
            case 'c':
                cvalue = va_arg(args, int);
                OUTCHAR(str, len, size, cvalue);
                break;
            case 's':
                strvalue = va_arg(args, char *);
                fmtstr(str, &len, size, strvalue, width, precision, flags);
                break;
            case 'p':
                if ((strvalue = (const char*)va_arg(args, void *)) == NULL)
                    fmtstr(str, &len, size, "(nil)", width, -1, flags);
                else {
                    flags |= PRINT_F_NUM | PRINT_F_UNSIGNED;
                    fmtint(str, &len, size, (UINTPTR_T)strvalue, 16, width, precision, flags);
                }
                break;
            case '%':
                OUTCHAR(str, len, size, ch);
                break;
            default:
                break;
            }
            ch = *format++;
            state = PRINT_S_DEFAULT;
            base = cflags = flags = width = 0;
            precision = -1;
            break;
        }
out:
    if (len < size)
        str[len] = '\0';
    else if (size > 0)
        str[size - 1] = '\0';

    if (overflow || len >= INT_MAX)
        return -1;
    return (int)len;
}

static void
fmtstr(char *str, size_t *len, size_t size, const char *value, int width,
       int precision, int flags)
{
    int padlen, strln;
    int noprecision = (precision == -1);

    if (value == NULL) value = "(null)";

    for (strln = 0; value[strln] != '\0' && (noprecision || strln < precision); strln++)
        continue;

    if ((padlen = width - strln) < 0) padlen = 0;
    if (flags & PRINT_F_MINUS) padlen = -padlen;

    while (padlen > 0)  { OUTCHAR(str, *len, size, ' '); padlen--; }
    while (*value != '\0' && (noprecision || precision-- > 0)) { OUTCHAR(str, *len, size, *value); value++; }
    while (padlen < 0)  { OUTCHAR(str, *len, size, ' '); padlen++; }
}

static void
fmtint(char *str, size_t *len, size_t size, INTMAX_T value, int base, int width,
       int precision, int flags)
{
    UINTMAX_T uvalue;
    char iconvert[MAX_CONVERT_LENGTH];
    char sign = 0;
    char hexprefix = 0;
    int spadlen = 0;
    int zpadlen = 0;
    int pos;
    int noprecision = (precision == -1);

    if (flags & PRINT_F_UNSIGNED)
        uvalue = value;
    else {
        uvalue = (value >= 0) ? value : -value;
        if (value < 0)             sign = '-';
        else if (flags & PRINT_F_PLUS)  sign = '+';
        else if (flags & PRINT_F_SPACE) sign = ' ';
    }

    pos = convert(uvalue, iconvert, sizeof(iconvert), base, flags & PRINT_F_UP);

    if (flags & PRINT_F_NUM && uvalue != 0) {
        switch (base) {
        case 8:  if (precision <= pos) precision = pos + 1; break;
        case 16: hexprefix = (flags & PRINT_F_UP) ? 'X' : 'x'; break;
        }
    }

    zpadlen = precision - pos;
    spadlen = width - MAX(precision, pos) - (sign ? 1 : 0) - (hexprefix ? 2 : 0);

    if (zpadlen < 0) zpadlen = 0;
    if (spadlen < 0) spadlen = 0;

    if (flags & PRINT_F_MINUS)
        spadlen = -spadlen;
    else if (flags & PRINT_F_ZERO && noprecision) {
        zpadlen += spadlen;
        spadlen = 0;
    }

    while (spadlen > 0) { OUTCHAR(str, *len, size, ' '); spadlen--; }
    if (sign)            OUTCHAR(str, *len, size, sign);
    if (hexprefix != 0) { OUTCHAR(str, *len, size, '0'); OUTCHAR(str, *len, size, hexprefix); }
    while (zpadlen > 0) { OUTCHAR(str, *len, size, '0'); zpadlen--; }
    while (pos > 0)     { pos--; OUTCHAR(str, *len, size, iconvert[pos]); }
    while (spadlen < 0) { OUTCHAR(str, *len, size, ' '); spadlen++; }
}

static int
convert(UINTMAX_T value, char *buf, size_t size, int base, int caps)
{
    const char *digits = caps ? "0123456789ABCDEF" : "0123456789abcdef";
    size_t pos = 0;
    do {
        buf[pos++] = digits[value % base];
        value /= base;
    } while (value != 0 && pos < size);
    return (int)pos;
}

int vsprintf(char *buf, const char *fmt, va_list args)
{
    return vsnprintf(buf, INT_MAX, fmt, args);
}

int sprintf(char *buffer, const char *fmt, ...)
{
    va_list args;
    int i;
    va_start(args, fmt);
    i = vsprintf(buffer, fmt, args);
    va_end(args);
    return i;
}

int snprintf(char *buffer, size_t len, const char *fmt, ...)
{
    va_list args;
    int i;
    va_start(args, fmt);
    i = vsnprintf(buffer, len, fmt, args);
    va_end(args);
    return i;
}

#define PRINTF_MAX 128