/* C library calls routed through the Rockbox plugin API. */
#ifndef CGL_LIBC_H
#define CGL_LIBC_H

#include "plugin.h"

#define strlen rb->strlen
#define strchr rb->strchr
#define strcpy rb->strcpy
#define strncpy rb->strncpy
#define strcat rb->strcat
#define snprintf rb->snprintf
#define vsnprintf rb->vsnprintf

char *strncat_(char *dst, const char *src, size_t n);
#define strncat strncat_

#endif
