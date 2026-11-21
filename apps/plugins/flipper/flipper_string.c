/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Flipper Zero compatibility layer: FuriString. */
#include <furi.h>

struct FuriString {
    char *s;
    size_t len, cap;
};

static void reserve(FuriString *f, size_t n)
{
    if (n + 1 <= f->cap) return;
    f->cap = (n + 1 + 15) & ~15u;
    f->s = realloc(f->s, f->cap);
    if (!f->s) furi_crash("string alloc");
}

FuriString *furi_string_alloc(void)
{
    FuriString *f = calloc(1, sizeof(*f));
    if (!f) furi_crash("string alloc");
    reserve(f, 0);
    f->s[0] = 0;
    return f;
}

void furi_string_set_str(FuriString *f, const char *str)
{
    size_t n = strlen(str);
    reserve(f, n);
    memcpy(f->s, str, n + 1);
    f->len = n;
}

FuriString *furi_string_alloc_set_str(const char *str)
{
    FuriString *f = furi_string_alloc();
    furi_string_set_str(f, str);
    return f;
}

void furi_string_free(FuriString *f)
{
    if (!f) return;
    free(f->s);
    free(f);
}

const char *furi_string_get_cstr(const FuriString *f) { return f->s; }
size_t furi_string_size(const FuriString *f) { return f->len; }
char furi_string_get_char(const FuriString *f, size_t i) { return i < f->len ? f->s[i] : 0; }
void furi_string_reset(FuriString *f) { f->len = 0; f->s[0] = 0; }

void furi_string_cat_str(FuriString *f, const char *str)
{
    size_t n = strlen(str);
    reserve(f, f->len + n);
    memcpy(f->s + f->len, str, n + 1);
    f->len += n;
}

void furi_string_right(FuriString *f, size_t start)
{
    if (start >= f->len) { furi_string_reset(f); return; }
    memmove(f->s, f->s + start, f->len - start + 1);
    f->len -= start;
}

int furi_string_vprintf(FuriString *f, const char *format, va_list args)
{
    va_list again;
    int n;
    va_copy(again, args);
    n = vsnprintf(f->s, f->cap, format, args);
    if (n >= 0 && (size_t)n >= f->cap) {
        reserve(f, n);
        n = vsnprintf(f->s, f->cap, format, again);
    }
    va_end(again);
    f->len = n < 0 ? 0 : strlen(f->s);
    return n;
}

int furi_string_printf(FuriString *f, const char *format, ...)
{
    va_list ap;
    int n;
    va_start(ap, format);
    n = furi_string_vprintf(f, format, ap);
    va_end(ap);
    return n;
}

int furi_string_cat_printf(FuriString *f, const char *format, ...)
{
    FuriString *tail = furi_string_alloc();
    va_list ap;
    int n;
    va_start(ap, format);
    n = furi_string_vprintf(tail, format, ap);
    va_end(ap);
    furi_string_cat_str(f, tail->s);
    furi_string_free(tail);
    return n;
}

FuriString *furi_string_alloc_printf(const char *format, ...)
{
    FuriString *f = furi_string_alloc();
    va_list ap;
    va_start(ap, format);
    furi_string_vprintf(f, format, ap);
    va_end(ap);
    return f;
}

size_t strlcpy(char *dst, const char *src, size_t size)
{
    size_t n = strlen(src);
    if (size) {
        size_t c = n < size - 1 ? n : size - 1;
        memcpy(dst, src, c);
        dst[c] = 0;
    }
    return n;
}

size_t strlcat(char *dst, const char *src, size_t size)
{
    size_t d = 0;
    while (d < size && dst[d]) d++;
    if (d == size) return size + strlen(src);
    return d + strlcpy(dst + d, src, size - d);
}

#ifndef strstr    /* the simulator build has the host libc one */
char *strstr(const char *hay, const char *needle)
{
    size_t n = strlen(needle);
    if (!n) return (char *)hay;
    for (; *hay; hay++)
        if (!strncmp(hay, needle, n)) return (char *)hay;
    return NULL;
}
#endif
