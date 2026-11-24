/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * RockTrack: undo/redo. A ring of small entries, grouped per gesture
 * (undo_begin); repeated edits of one cell within a gesture coalesce.
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
#include "rt.h"

#if MEMORYSIZE <= 2
#define UNDO_N 512
#else
#define UNDO_N 2048
#endif

struct uent {
    uint8_t kind;
    uint8_t first;      /* first entry of a group */
    uint8_t c;
    uint16_t a, b;
    union {
        struct cell cell[2];
        int16_t v[2];
    };
};

static struct uent ring[UNDO_N];
static int ustart, ulen, upos;
static bool new_group = true;
static int serial;
struct undo_where undo_where;

static struct uent *entry(int i)
{
    return &ring[(ustart + i) % UNDO_N];
}

void undo_clear(void)
{
    ustart = ulen = upos = 0;
    new_group = true;
    serial++;
}

void undo_begin(void)
{
    new_group = true;
}

int undo_serial(void)
{
    return serial;
}

static struct uent *append(int kind)
{
    ulen = upos;                    /* drop the redo tail */
    if (ulen == UNDO_N) {
        ustart = (ustart + 1) % UNDO_N;
        ulen--;
        upos--;
        entry(0)->first = 1;
    }
    struct uent *e = entry(ulen++);
    upos = ulen;
    e->kind = kind;
    e->first = new_group;
    new_group = false;
    serial++;
    return e;
}

void undo_cell(int pat, int row, int ch, const struct cell *old,
               const struct cell *new)
{
    if (!rb->memcmp(old, new, sizeof(*old)))
        return;
    if (!new_group && upos > 0) {
        struct uent *l = entry(upos - 1);
        if (l->kind == U_CELL && l->a == pat && l->b == row && l->c == ch) {
            l->cell[1] = *new;
            serial++;
            return;
        }
    }
    struct uent *e = append(U_CELL);
    e->a = pat;
    e->b = row;
    e->c = ch;
    e->cell[0] = *old;
    e->cell[1] = *new;
}

void undo_order(int kind, int pos, int old, int new)
{
    if (kind == U_ORDER && old == new)
        return;
    struct uent *e = append(kind);
    e->a = pos;
    e->v[0] = old;
    e->v[1] = new;
}

void undo_iparam(int ins, int param, int old, int new)
{
    if (old == new)
        return;
    if (!new_group && upos > 0) {
        struct uent *l = entry(upos - 1);
        if (l->kind == U_IPARAM && l->a == ins && l->b == param) {
            l->v[1] = new;
            serial++;
            return;
        }
    }
    struct uent *e = append(U_IPARAM);
    e->a = ins;
    e->b = param;
    e->v[0] = old;
    e->v[1] = new;
}

static void apply(const struct uent *e, bool forward)
{
    int k = forward ? 1 : 0;
    undo_where.kind = e->kind;
    undo_where.a = e->a;
    undo_where.b = e->b;
    undo_where.c = e->c;
    switch (e->kind) {
    case U_CELL: {
        struct cell *c = cell_w(e->a, e->b, e->c);
        if (c)
            *c = e->cell[k];
        break;
    }
    case U_ORDER:
        song.ord[e->a] = e->v[k];
        break;
    case U_ORDINS:
        if (forward)
            order_insert(e->a, e->v[1]);
        else
            order_delete(e->a);
        break;
    case U_ORDDEL:
        if (forward)
            order_delete(e->a);
        else
            order_insert(e->a, e->v[0]);
        break;
    case U_IPARAM:
        instr_param_set(e->a, e->b, e->v[k]);
        break;
    }
}

bool undo_undo(void)
{
    if (upos == 0)
        return false;
    const struct uent *e;
    do {
        e = entry(--upos);
        apply(e, false);
    } while (!e->first && upos > 0);
    new_group = true;
    serial++;
    return true;
}

bool undo_redo(void)
{
    if (upos == ulen)
        return false;
    do
        apply(entry(upos++), true);
    while (upos < ulen && !entry(upos)->first);
    new_group = true;
    serial++;
    return true;
}
