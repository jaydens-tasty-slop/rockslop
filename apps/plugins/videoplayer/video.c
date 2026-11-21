/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Video player pictures.
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
#include "video.h"
#include "mjpeg.h"
#include "mpeg.h"

static const struct vformat *vid;
static enum rotation rot_pref;
static fb_data *fb;

/* Layout for pictures of src_w x src_h: the screen as the picture sees it */
static int src_w, src_h;
static bool turned;
static struct canvas canvas;
static int step;                /* raw: every step-th pixel */
static int out_w, out_h;        /* the scaled picture */
static bool need_clear;
static struct video_target target;
/* The last whole-packet picture, to draw again */
static const uint8_t *last_data;
static uint32_t last_size;

/* The downscale for a picture: the one that fits, or one step less if
 * cropping that loses at most a fifth of it */
static int fit_shift(int w, int h, int max_shift)
{
    int s = 0;
    while (s < max_shift && ((w >> s) > canvas.w || (h >> s) > canvas.h))
        s++;
    if (s > 0)
    {
        int cw = w >> (s - 1), ch = h >> (s - 1);
        if (MIN(cw, canvas.w) * MIN(ch, canvas.h) * 5 >= cw * ch * 4)
            s--;
    }
    return s;
}

static void layout(int w, int h)
{
    src_w = w;
    src_h = h;
    turned = rot_pref != ROTATE_NEVER && w > h;
    canvas.w = turned ? LCD_HEIGHT : LCD_WIDTH;
    canvas.h = turned ? LCD_WIDTH : LCD_HEIGHT;
    if (!turned)
    {
        canvas.origin = fb;
        canvas.dx = 1;
        canvas.dy = LCD_WIDTH;
    }
    else if (rot_pref == ROTATE_CW)
    {
        canvas.origin = fb + LCD_WIDTH - 1;
        canvas.dx = LCD_WIDTH;
        canvas.dy = -1;
    }
    else
    {
        canvas.origin = fb + (LCD_HEIGHT - 1) * LCD_WIDTH;
        canvas.dx = -LCD_WIDTH;
        canvas.dy = 1;
    }

    int shift = 0;
    switch (vid->codec)
    {
    case VC_MJPEG:  /* power-of-two sizes only: the decoder averages */
        shift = fit_shift(w, h, 3);
        break;
    case VC_MPEG:
        shift = fit_shift(w, h, 1);
        break;
    default:
        step = MAX((w + canvas.w - 1) / canvas.w, (h + canvas.h - 1) / canvas.h);
        break;
    }
    if (vid->codec == VC_MJPEG || vid->codec == VC_MPEG)
    {
        out_w = w >> shift;
        out_h = h >> shift;
    }
    else
    {
        out_w = w / step;
        out_h = h / step;
    }
    int disp_w = MIN(out_w, canvas.w), disp_h = MIN(out_h, canvas.h);
    target = (struct video_target){
        canvas.origin + (canvas.w - disp_w) / 2 * canvas.dx +
                        (canvas.h - disp_h) / 2 * canvas.dy,
        canvas.dx, canvas.dy, shift, (out_w - disp_w) / 2,
        (out_h - disp_h) / 2, disp_w, disp_h };
    need_clear = true;
}

void video_close(void)
{
    vid = NULL;
}

bool video_init(const struct track *t, const uint8_t *extra, int extra_len,
                enum rotation rotation)
{
    struct viewport *vp = rb->lcd_set_viewport(NULL);

    vid = &t->u.v;
    rot_pref = rotation;
    fb = vp->buffer->fb_ptr;
    last_data = NULL;
    if ((vid->codec == VC_MJPEG && !mjpeg_init()) ||
        (vid->codec == VC_MPEG &&
         !mpeg_init(extra, extra_len, vid->width, vid->height)))
    {
        vid = NULL;     /* no picture: messages go on the plain screen */
        return false;
    }
    layout(vid->width, vid->height);
    return true;
}

void video_set_rotation(enum rotation rotation)
{
    rot_pref = rotation;
    layout(src_w, src_h);
}

void video_reset(void)
{
    if (vid->codec == VC_MPEG)
        mpeg_reset();
    last_data = NULL;
}

void video_clear(void)
{
    need_clear = true;
}

bool video_turned(void)
{
    return turned;
}

const struct canvas *video_canvas(void)
{
    static struct canvas screen;
    if (vid)
        return &canvas;
    screen = (struct canvas){ rb->lcd_set_viewport(NULL)->buffer->fb_ptr, 1,
                              LCD_WIDTH, LCD_WIDTH, LCD_HEIGHT };
    return &screen;
}

static void clear_screen(void)
{
    rb->memset(fb, 0, LCD_WIDTH * LCD_HEIGHT * sizeof(fb_data));
    need_clear = false;
}

static const struct video_target *picture_layout(int w, int h)
{
    /* stock players never check the frame size: neither do their files */
    if (w != src_w || h != src_h)
        layout(w, h);
    if (need_clear)
        clear_screen();
    return &target;
}

static inline fb_data rgb565(unsigned v)
{
#if LCD_PIXELFORMAT == RGB565SWAPPED
    return swap16(v);
#else
    return v;
#endif
}

static bool decode_raw(const uint8_t *data, uint32_t size)
{
    int bpp = vid->codec == VC_BGR24 ? 3 : 2;
    uint32_t stride = (src_w * bpp + 3) & ~3u;  /* DIB rows end on 4 bytes */
    const struct video_target *t = picture_layout(src_w, src_h);

    if (size < stride * src_h)
        return false;
    for (int r = 0; r < t->disp_h; r++)
    {
        int sy = (r + t->crop_y) * step;
        if (vid->bottom_up)
            sy = src_h - 1 - sy;
        const uint8_t *s = data + sy * stride + t->crop_x * step * bpp;
        fb_data *p = t->row0 + r * t->dy;
        int i = t->disp_w;
        switch (vid->codec)
        {
        case VC_RGB565:
            for (; i > 0; i--, s += step * 2, p += t->dx)
                *p = rgb565(s[0] | (s[1] << 8));
            break;
        case VC_RGB555:
            for (; i > 0; i--, s += step * 2, p += t->dx)
            {
                unsigned v = s[0] | (s[1] << 8);
                /* 0rrrrrgg gggbbbbb -> rrrrrggg ggxbbbbb (top green bit
                   copied down) */
                *p = rgb565(((v & 0x7fe0) << 1) | ((v >> 4) & 0x20) | (v & 0x1f));
            }
            break;
        default:
            for (; i > 0; i--, s += step * 3, p += t->dx)
                *p = FB_RGBPACK(s[2], s[1], s[0]);
            break;
        }
    }
    return true;
}

bool video_decode(const uint8_t *data, uint32_t size)
{
    last_data = data;
    last_size = size;
    if (vid->codec == VC_MJPEG)
        return mjpeg_decode(data, size, picture_layout);
    return decode_raw(data, size);
}

bool video_streamed(void)
{
    return vid->codec == VC_MPEG;
}

void video_feed(const uint8_t *data, uint32_t size, uint32_t ms)
{
    mpeg_feed(data, size, ms);
}

bool video_next(bool show, uint32_t *ms)
{
    return mpeg_next(picture_layout, show, ms);
}

bool video_redraw(void)
{
    if (vid->codec == VC_MPEG)
        return mpeg_redraw(picture_layout);
    return last_data && video_decode(last_data, last_size);
}

void video_describe(char *buf, size_t size)
{
    int scale = (vid->codec == VC_MJPEG || vid->codec == VC_MPEG) ?
                1 << target.shift : step;
    const char *how = target.disp_w < out_w || target.disp_h < out_h ?
                      ", cropped" : "";
    if (scale > 1)
        rb->snprintf(buf, size, "%dx%d, 1/%d%s", src_w, src_h, scale, how);
    else
        rb->snprintf(buf, size, "%dx%d%s", src_w, src_h, how);
}
