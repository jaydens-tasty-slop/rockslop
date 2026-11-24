/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2010 by Thomas Martitz
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __LOAD_CODE_H__
#define __LOAD_CODE_H__

#include "config.h"

/* this struct needs to be the first part of other headers
 * (lc_open() casts the other header to this one to load to the correct
 * address)
 */
struct lc_header {
    unsigned long magic;
    unsigned short target_id;
    unsigned short api_version;
    unsigned char *load_addr;
    unsigned char *end_addr;
#ifdef HAVE_EXACT_MODULE_API
    unsigned long build_id; /* MODULE_BUILD_ID */
#endif
};

#ifdef HAVE_EXACT_MODULE_API
/* Hash of the module API headers, set by the core and module linker
 * scripts (module_build_id.lds, see apps/apps.make); hosted builds (the
 * simulator) link without them */
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
extern char module_build_id[];
#define MODULE_BUILD_ID ((unsigned long)module_build_id)
#else
#define MODULE_BUILD_ID 0ul
#endif
#define LC_HEADER_BUILD_ID , MODULE_BUILD_ID
#else
#define LC_HEADER_BUILD_ID
#endif

#if CONFIG_BINFMT == BINFMT_ROCK
# include "lc-rock.h"
#elif CONFIG_BINFMT == BINFMT_DLOPEN
# include "lc-dlopen.h"
#else
# error "Unsupported CONFIG_BINFMT!"
#endif

#endif /* __LOAD_CODE_H__ */
