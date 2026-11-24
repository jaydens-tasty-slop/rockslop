/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * strtol and strtoul, written from ISO C99 7.20.1.4.
 */
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>

/* Shared parser: returns the magnitude, sets *neg for a leading '-', and
 * *ovf if it does not fit in an unsigned long. */
static unsigned long parse_magnitude(const char *nptr, char **endptr, int base,
                                     int *neg, int *ovf)
{
    const char *s = nptr;
    const char *digits;
    unsigned long v = 0;
    int c;

    *neg = 0;
    *ovf = 0;

    while (isspace((unsigned char)*s))
        s++;

    if (*s == '-' || *s == '+')
        *neg = (*s++ == '-');

    if (base != 0 && (base < 2 || base > 36))
    {
        errno = EINVAL;
        if (endptr)
            *endptr = (char *)nptr;
        return 0;
    }

    /* A "0x"/"0X" prefix only counts when a hex digit follows it. */
    if ((base == 0 || base == 16) && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')
        && isxdigit((unsigned char)s[2]))
    {
        s += 2;
        base = 16;
    }
    else if (base == 0)
        base = (s[0] == '0') ? 8 : 10;

    digits = s;
    for (;; s++)
    {
        c = (unsigned char)*s;
        if (c >= '0' && c <= '9')
            c -= '0';
        else if (c >= 'a' && c <= 'z')
            c -= 'a' - 10;
        else if (c >= 'A' && c <= 'Z')
            c -= 'A' - 10;
        else
            break;
        if (c >= base)
            break;

        if (!*ovf)
        {
            if (v > (ULONG_MAX - c) / base)
                *ovf = 1;
            else
                v = v * base + c;
        }
    }

    if (endptr)
        *endptr = (char *)(s == digits ? nptr : s);
    return v;
}

unsigned long strtoul(const char *nptr, char **endptr, int base)
{
    int neg, ovf;
    unsigned long v = parse_magnitude(nptr, endptr, base, &neg, &ovf);

    if (ovf)
    {
        errno = ERANGE;
        return ULONG_MAX;
    }
    return neg ? 0ul - v : v;
}

long strtol(const char *nptr, char **endptr, int base)
{
    int neg, ovf;
    unsigned long v = parse_magnitude(nptr, endptr, base, &neg, &ovf);

    if (neg)
    {
        if (ovf || v > (unsigned long)LONG_MAX + 1ul)
        {
            errno = ERANGE;
            return LONG_MIN;
        }
        return (long)(0ul - v);
    }
    if (ovf || v > (unsigned long)LONG_MAX)
    {
        errno = ERANGE;
        return LONG_MAX;
    }
    return (long)v;
}
