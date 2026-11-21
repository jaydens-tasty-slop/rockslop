/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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
#ifndef __CORE_ALLOC_TOP_H__
#define __CORE_ALLOC_TOP_H__
/* Not in core_alloc.h or buflib.h: plugin.h includes those, and the module
 * build ID is a hash of it (apps/apps.make), so a declaration there makes
 * every installed codec and plugin fail to load. */
#include "core_alloc.h"

#if defined(HAVE_CORE_ALLOC_TOP) && CONFIG_BUFLIB_BACKEND == BUFLIB_BACKEND_MEMPOOL
/**
 * \brief Allocate at the top of the highest free space
 *
 * As buflib_alloc_ex(), for a long-lived block that will be pinned: placed
 * first-fit after a short-lived one, it splits the heap in two once that is
 * freed. Leaves room for the handle table to grow. Falls back to
 * buflib_alloc_ex() when no free space holds it.
 */
int buflib_alloc_top(struct buflib_context *ctx, size_t size,
                     struct buflib_callbacks *ops);
/* A long-lived block to pin, at the heap's top (buflib_alloc_top()) */
int core_alloc_top(size_t size);
#else
#define core_alloc_top core_alloc
#endif

#endif /* __CORE_ALLOC_TOP_H__ */
