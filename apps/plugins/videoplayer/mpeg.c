/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * MPEG-1/2 video with mpegplayer's libmpeg2, on frame buffers of our own:
 * two, or three when there is room for B pictures (without, they are
 * skipped). Shown pictures go straight to the screen.
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
#include "plugin.h"
#include "mpegplayer/libmpeg2/mpeg2dec_config.h"
#include "mpegplayer/libmpeg2/mpeg2.h"
#include "videoplayer.h"
#include "mpeg.h"

struct frame {
    uint8_t *yuv[3];
    bool used;
};

static mpeg2dec_t *dec;
static const mpeg2_info_t *info;
static struct frame frames[3];
static int nframes;
static uint32_t frame_bytes;            /* luma bytes a frame holds */
static uint8_t *fake_yuv[3];            /* a skipped B picture's "buffer" */
static bool want_i;                     /* skip until an I picture */
static uint32_t last_ms, frame_ms = 40;
static const struct frame *shown;      /* the last picture shown */
static int width, cwidth;               /* coded sizes of the shown one */
static bool bad_size;                   /* pictures larger than the frames */
static bool no_memory;                  /* an allocation of libmpeg2's failed */

/* libmpeg2's memory: only its decoder state and chunk buffer, as the
 * frame buffers are ours. Zeroed: mpeg2_init() leaves fields (the frame
 * buffer ids, the matrices' scaled flags) as they come. */
void *mpeg2_malloc(unsigned size, mpeg2_alloc_t reason)
{
    (void)reason;
    void *p = player_alloc(size);
    if (p)
        rb->memset(p, 0, size);
    else
        no_memory = true;
    return p;
}

void *mpeg2_bufalloc(unsigned size, mpeg2_alloc_t reason)
{
    return mpeg2_malloc(size, reason);
}

void mpeg2_mem_reset(void)
{
}

static bool add_frame(void)
{
    struct frame *f = &frames[nframes];
    uint8_t *y = player_alloc(frame_bytes + frame_bytes / 2);
    if (!y)
        return false;
    f->yuv[0] = y;
    f->yuv[1] = y + frame_bytes;
    f->yuv[2] = y + frame_bytes + frame_bytes / 4;
    f->used = false;
    nframes++;
    return true;
}

bool mpeg_init(const uint8_t *extra, int extra_len, int width, int height)
{
    no_memory = false;
    dec = mpeg2_init();
    if (!dec || no_memory)
        return false;
    info = mpeg2_info(dec);
    nframes = 0;
    shown = NULL;
    want_i = true;
    bad_size = false;
    /* two frames now, for the size the container says (in macroblocks) */
    frame_bytes = ALIGN_UP(width, 16) * ALIGN_UP(height, 16);
    if (!frame_bytes || !add_frame() || !add_frame())
        return false;
    fake_yuv[0] = frames[0].yuv[0];
    fake_yuv[1] = frames[0].yuv[1];
    fake_yuv[2] = frames[0].yuv[2];
    if (extra_len > 0)
        mpeg2_buffer(dec, (uint8_t *)extra, (uint8_t *)extra + extra_len);
    return true;
}

int mpeg_frames(void)
{
    return nframes;
}

/* At a sequence header: it must fit the frames; a third one for B
 * pictures if the memory allows by now */
static bool setup_frames(const mpeg2_sequence_t *seq)
{
    uint32_t luma = seq->width * seq->height;

    if (seq->chroma_width * seq->chroma_height * 4 > luma ||  /* not 4:2:0 */
        luma > frame_bytes)
        return false;
    if (nframes == 2)
        add_frame();
    if (seq->frame_period)
        frame_ms = MAX(seq->frame_period / 27000, 1u);
    return true;
}

void mpeg_reset(void)
{
    mpeg2_reset(dec, false);
    for (int i = 0; i < nframes; i++)
        frames[i].used = false;
    want_i = true;
    shown = NULL;
}

void mpeg_feed(const uint8_t *data, uint32_t size, uint32_t ms)
{
    static const uint8_t seq_end[4] = { 0, 0, 1, 0xb7 };

    if (!data)
    {
        /* the end: the last reference picture comes out */
        data = seq_end;
        size = sizeof(seq_end);
    }
    else if (ms != NO_TIME)
        mpeg2_tag_picture(dec, ms, 0);
    mpeg2_buffer(dec, (uint8_t *)data, (uint8_t *)data + size);
}

static void release(const mpeg2_fbuf_t *fb)
{
    if (fb && fb->id)
        ((struct frame *)fb->id)->used = false;
}

/* A new picture starts: give it a frame (or skip it) */
static void start_picture(void)
{
    int type = info->current_picture->flags & PIC_MASK_CODING_TYPE;
    bool skip = (want_i && type != PIC_FLAG_CODING_TYPE_I) ||
                (type == PIC_FLAG_CODING_TYPE_B && nframes < 3);

    /* an I or P picture ends the oldest reference's use now */
    release(info->discard_fbuf);
    if (type == PIC_FLAG_CODING_TYPE_I)
        want_i = false;
    mpeg2_skip(dec, skip);
    if (!skip)
    {
        /* not the one on screen if there is another: it may be redrawn */
        struct frame *f = NULL;
        for (int i = 0; i < nframes; i++)
            if (!frames[i].used && (!f || f == shown))
                f = &frames[i];
        if (f)
        {
            f->used = true;
            mpeg2_set_buf(dec, f->yuv, f);
            return;
        }
    }
    /* nothing will be written there */
    mpeg2_set_buf(dec, fake_yuv, NULL);
}

/* BT.601 studio range to the screen's RGB565 */
static inline unsigned clip8(int v)
{
    return v < 0 ? 0 : v > 255 ? 255 : v;
}

static inline fb_data yuv_pixel(int y, int rv, int guv, int bu)
{
    y = (y - 16) * 298;
    return FB_RGBPACK(clip8((y + rv) >> 8), clip8((y + guv) >> 8),
                      clip8((y + bu) >> 8));
}

static void convert(const struct frame *f, const struct video_target *t)
{
    const uint8_t *Y = f->yuv[0], *U = f->yuv[1], *V = f->yuv[2];

    for (int r = 0; r < t->disp_h; r++)
    {
        int sy = (r + t->crop_y) << t->shift;
        const uint8_t *yr = Y + sy * width;
        const uint8_t *ur = U + (sy >> 1) * cwidth;
        const uint8_t *vr = V + (sy >> 1) * cwidth;
        fb_data *p = t->row0 + r * t->dy;
        for (int c = 0; c < t->disp_w; c++, p += t->dx)
        {
            int sx = (c + t->crop_x) << t->shift;
            int cx = sx >> 1;
            int d = ur[cx] - 128, e = vr[cx] - 128;
            int rv = 409 * e + 128, guv = 128 - 100 * d - 208 * e;
            int bu = 516 * d + 128;
            int y = yr[sx];
            if (t->shift)   /* the 2x2 average */
                y = (y + yr[sx + 1] + yr[sx + width] + yr[sx + width + 1] + 2) >> 2;
            *p = yuv_pixel(y, rv, guv, bu);
        }
    }
}

static bool show_picture(video_layout_fn *layout, const struct frame *f)
{
    width = info->sequence->width;
    cwidth = info->sequence->chroma_width;
    const struct video_target *t = layout(info->sequence->picture_width,
                                          info->sequence->picture_height);
    if (!t || t->shift > 1)
        return false;
    convert(f, t);
    return true;
}

bool mpeg_next(video_layout_fn *layout, bool show, uint32_t *ms)
{
    for (;;)
    {
        mpeg2_state_t state = mpeg2_parse(dec);
        switch (state)
        {
        case STATE_BUFFER:
            return false;
        case STATE_SEQUENCE:
            /* (the sequence header turns custom frames off again) */
            mpeg2_custom_fbuf(dec, 1);
            bad_size = !setup_frames(info->sequence);
            break;
        case STATE_PICTURE:
            if (bad_size)   /* no room for it: skip every picture */
            {
                mpeg2_skip(dec, 1);
                mpeg2_set_buf(dec, fake_yuv, NULL);
                break;
            }
            start_picture();
            break;
        case STATE_SLICE:
        case STATE_END:
        case STATE_INVALID_END:
        {
            const mpeg2_picture_t *pic = info->display_picture;
            const mpeg2_fbuf_t *fb = info->display_fbuf;
            bool ready = fb && fb->id && pic && !(pic->flags & PIC_FLAG_SKIP);
            if (ready)
            {
                last_ms = (pic->flags & PIC_FLAG_TAGS) ? pic->tag
                                                       : last_ms + frame_ms;
                shown = fb->id;
                if (show)
                    show_picture(layout, shown);
            }
            /* a B picture is done with once shown; an I or P picture's
               discard was its start's, whose frame holds another now */
            if (state != STATE_SLICE ||
                (info->current_picture->flags & PIC_MASK_CODING_TYPE) ==
                PIC_FLAG_CODING_TYPE_B)
                release(info->discard_fbuf);
            if (ready)
            {
                *ms = last_ms;
                return true;
            }
            break;
        }
        default:
            break;
        }
    }
}

bool mpeg_redraw(video_layout_fn *layout)
{
    return shown && show_picture(layout, shown);
}
