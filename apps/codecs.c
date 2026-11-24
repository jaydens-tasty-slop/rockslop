/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2002 Björn Stenberg
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
#include "config.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <timefuncs.h>
#include <ctype.h>
#include <stdarg.h>
#include "string-extra.h"
#include "load_code.h"
#include "debug.h"
#include "button.h"
#include "dir.h"
#include "file.h"
#include "kernel.h"
#include "screens.h"
#include "misc.h"
#include "codecs.h"
#include "lang.h"
#include "keyboard.h"
#include "buffering.h"
#include "backlight.h"
#include "storage.h"
#include "talk.h"
#include "mp3data.h"
#include "powermgmt.h"
#include "system.h"
#include "sound.h"
#include "splash.h"
#include "general.h"
#include "rbpaths.h"
#include "panic.h"
#include "devlog.h"

#define LOGF_ENABLE
#include "logf.h"

#if (CONFIG_PLATFORM & PLATFORM_SDL)
#define PREFIX(_x_) sim_ ## _x_
#else
#define PREFIX(_x_) _x_
#endif

#if (CONFIG_PLATFORM & PLATFORM_HOSTED)
/* For PLATFORM_HOSTED this buffer must be define here. */
static unsigned char codecbuf[CODEC_SIZE];
#else
/* For PLATFORM_NATIVE this buffer is defined in *.lds files. */
extern unsigned char codecbuf[];
#endif

static size_t codec_size;
#ifdef TARGET_ARENA_PCM_SIZE
/* Arena bytes codecs may use: fewer while the PCM ring is in its top */
static size_t codec_limit = CODEC_SIZE;
#else
#define codec_limit CODEC_SIZE
#endif

struct codec_api ci = {

    0,    /* filesize */
    0,    /* curpos */
    NULL, /* id3 */
    ERR_HANDLE_NOT_FOUND, /* audio_hid */
    NULL, /* struct dsp_config *dsp */
    NULL, /* codec_get_buffer */
    NULL, /* pcmbuf_insert */
    NULL, /* set_elapsed */
    NULL, /* read_filebuf */
    NULL, /* request_buffer */
    NULL, /* advance_buffer */
    NULL, /* seek_buffer */
    NULL, /* seek_complete */
    NULL, /* set_offset */
    NULL, /* configure */
    NULL, /* get_command */
    NULL, /* loop_track */
    NULL, /* strip_filesize */

    /* kernel/ system */
#if defined(ARM_NEED_DIV0)
    __div0,
#endif
    sleep,
    yield,

#if NUM_CORES > 1
    create_thread,
    thread_thaw,
    thread_wait,
    semaphore_init,
    semaphore_wait,
    semaphore_release,
#endif

    commit_dcache,
    commit_discard_dcache,
    commit_discard_idcache,

    /* strings and memory */
    strcpy,
    strlen,
    strcmp,
    strcat,
    memset,
    memcpy,
    memmove,
    memcmp,
    memchr,
#if defined(HAVE_DEVLOG)
    dev_log,
#elif defined(DEBUG) || defined(SIMULATOR)
    debugf,
#endif
#ifdef ROCKBOX_HAS_LOGF
    logf,
#endif

    (void *)qsort,

#ifdef RB_PROFILE
    profile_thread,
    profstop,
    __cyg_profile_func_enter,
    __cyg_profile_func_exit,
#endif

#ifdef HAVE_RECORDING
    NULL, /* enc_pcmbuf_read */
    NULL, /* enc_pcmbuf_advance */
    NULL, /* enc_encbuf_get_buffer */
    NULL, /* enc_encbuf_finish_buffer */
    NULL, /* enc_stream_read */
    NULL, /* enc_stream_lseek */
    NULL, /* enc_stream_write */
    round_value_to_list32,
#endif /* HAVE_RECORDING */

    /* new stuff at the end, sort into place next time
       the API gets incompatible */
    panicf,
#ifdef HAVE_CODEC_EXTRA_BUFFER
    NULL, /* codec_get_extra_buffer */
#endif

};

void codec_get_full_path(char *path, const char *codec_root_fn)
{
    snprintf(path, MAX_PATH-1, CODECS_DIR "/" CODEC_PREFIX "%s."
            CODEC_EXTENSION, codec_root_fn);
}

/* Returns pointer to and size of free codec RAM. Aligns to MEM_ALIGN_SIZE. */
void *codec_get_buffer_callback(size_t *size)
{
    void *buf = &codecbuf[codec_size];
    ssize_t s = codec_limit - codec_size;

    if (s <= 0)
        return NULL;

    *size = s;
    ALIGN_BUFFER(buf, *size, MEM_ALIGN_SIZE);

    return buf;
}

/** codec loading and call interface **/
static void *curr_handle = NULL;
static struct codec_header *c_hdr = NULL;

#include "module_arena.h"
#ifdef TARGET_ARENA_PCM_SIZE
#include "codec_thread.h"
#endif

#ifdef HAVE_XIP_CODECS
/* The codec of this root name in the codec flash area's directory */
static const struct codec_xip_header *codec_find_xip(const char *name)
{
    const struct codec_xip_dir *dir = (const void *)
        (XIP_CODEC_AREA_START + XIP_CODEC_AREA_SIZE - CODEC_XIP_DIR_SIZE);

    if (dir->magic != CODEC_XIP_DIR_MAGIC
        || dir->count > (CODEC_XIP_DIR_SIZE - sizeof(*dir)) / sizeof(dir->entry[0]))
        return NULL;
    for (uint32_t i = 0; i < dir->count; i++)
        if (!strncmp(dir->entry[i].name, name, sizeof(dir->entry[i].name)))
            return dir->entry[i].hdr;
    return NULL;
}

bool codec_in_flash(const char *codec)
{
    return codec_find_xip(codec) != NULL;
}

/* Check an XIP codec's header in flash, load its RAM image (data, hot code
 * and tables) into the arena; codec_start() clears the bss. */
static struct codec_header *codec_map_xip(struct codec_header *hdr)
{
    const struct codec_xip_header *img = (void *)hdr;
    const unsigned char *area = (const unsigned char *)XIP_CODEC_AREA_START;
    unsigned char *data = img->hdr.lc_hdr.load_addr;
    size_t data_size = img->data_end - data;

    if (img->flash_start != (const void *)img   /* e.g. an old card stub */
        || img->flash_start < area
        || img->flash_end > area + XIP_CODEC_AREA_SIZE - CODEC_XIP_DIR_SIZE
        || img->flash_start >= img->flash_end
        || img->data_load < img->flash_start
        || data != codecbuf
        || img->data_end < data
        || img->data_end > img->hdr.lc_hdr.end_addr
        || img->hdr.lc_hdr.end_addr > codecbuf + codec_limit
        || data_size > (size_t)(img->flash_end - img->data_load))
    {
        dev_log("xip codec: bad header at %p", img);
        return NULL;
    }

    memcpy(data, img->data_load, data_size);
    return hdr;
}
#endif /* HAVE_XIP_CODECS */

static int codec_load_ram(struct codec_api *api)
{
    struct lc_header *hdr;

    c_hdr = lc_get_header(curr_handle);
#ifdef HAVE_XIP_CODECS
    if (c_hdr && c_hdr->lc_hdr.magic == CODEC_XIP_MAGIC)
        c_hdr = codec_map_xip(c_hdr);
#endif
    hdr   = c_hdr ? &c_hdr->lc_hdr : NULL;

    if (hdr == NULL
        || (hdr->magic != CODEC_MAGIC
#ifdef HAVE_RECORDING
             && hdr->magic != CODEC_ENC_MAGIC
#endif
#ifdef HAVE_XIP_CODECS
             && hdr->magic != CODEC_XIP_MAGIC
#endif
            )
        || hdr->target_id != TARGET_ID
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
        || hdr->load_addr != codecbuf
        || hdr->end_addr > codecbuf + codec_limit
#endif
        )
    {
        logf("codec header error");
        dev_log("codec header error (end %p, arena end %p)",
                    hdr ? hdr->end_addr : NULL, codecbuf + codec_limit);
        lc_close(curr_handle);
        curr_handle = NULL;
        module_arena_release_codec();
        return CODEC_ERROR;
    }

    if (hdr->api_version != CODEC_API_VERSION ||
#ifdef HAVE_EXACT_MODULE_API
        hdr->build_id != MODULE_BUILD_ID ||
        c_hdr->api_size != sizeof(struct codec_api))
#else
        c_hdr->api_size > sizeof(struct codec_api))
#endif
    {
        logf("codec api version error");
        /* e.g. a card codec from another build (refresh the card's codecs) */
        dev_log("codec api %u build %08lx, core wants %u %08lx",
                    hdr->api_version, hdr->build_id,
                    CODEC_API_VERSION, MODULE_BUILD_ID);
        lc_close(curr_handle);
        curr_handle = NULL;
        module_arena_release_codec();
        return CODEC_ERROR;
    }

#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    codec_size = hdr->end_addr - codecbuf;
#else
    codec_size = 0;
#endif

    *(c_hdr->api) = api;

    logf("Codec: calling entrypoint");
    int status = c_hdr->entry_point(CODEC_LOAD);
#ifdef HAVE_SHARED_CODEC_PLUGIN_BUFFER
    if (status < 0)
        codec_close();
#endif
    return status;
}

#if defined(HAVE_CODEC_BUFFERING)
int codec_load_buf(int hid, struct codec_api *api)
{
    if (!module_arena_acquire_codec())
    {
        dev_log("codec: module arena busy (state %x)", module_arena_state());
        return CODEC_ERROR;
    }
    int rc = bufread(hid, codec_limit, codecbuf);

    if (rc < 0) {
        logf("Codec: cannot read buf handle");
        module_arena_release_codec();
        return CODEC_ERROR;
    }

    curr_handle = lc_open_from_mem(codecbuf, rc);

    if (curr_handle == NULL) {
        logf("Codec: load error");
        module_arena_release_codec();
        return CODEC_ERROR;
    }

    return codec_load_ram(api);
}
#endif /* HAVE_CODEC_BUFFERING */

int codec_load_file(const char *plugin, struct codec_api *api)
{
    if (!module_arena_acquire_codec())
    {
        dev_log("codec: module arena busy (state %x)", module_arena_state());
        return CODEC_ERROR;
    }
    char path[MAX_PATH];

    codec_get_full_path(path, plugin);

#ifdef HAVE_XIP_CODECS
    /* a codec in flash needs no file: its header there is the handle */
    curr_handle = (void *)codec_find_xip(plugin);
    if (curr_handle == NULL)
#endif
    curr_handle = lc_open(path, codecbuf, codec_limit);

    if (curr_handle == NULL) {
        logf("Codec: cannot read file");
        dev_log("codec %s: no file, or over %lu B", path,
                    (unsigned long)codec_limit);
        module_arena_release_codec();
        return CODEC_ERROR;
    }

    return codec_load_ram(api);
}

#ifdef TARGET_ARENA_PCM_SIZE
void *codec_arena_reserve_top(size_t size)
{
    if (size > CODEC_SIZE || (curr_handle && codec_size > CODEC_SIZE - size))
        return NULL;
    codec_limit = CODEC_SIZE - size;
    return codecbuf + codec_limit;
}

size_t codec_arena_need(int afmt)
{
    const char *name = get_codec_filename(afmt);
    struct lc_header hdr;
    if (name == NULL)
        return 0;
#ifdef HAVE_XIP_CODECS
    const struct codec_xip_header *xip = codec_find_xip(name);
    if (xip != NULL)
        hdr = xip->hdr.lc_hdr;
    else
#endif
    {
        char path[MAX_PATH];
        codec_get_full_path(path, name);
        int fd = open(path, O_RDONLY);
        if (fd < 0)
            return 0;
        ssize_t n = read(fd, &hdr, sizeof hdr);
        close(fd);
        if (n != (ssize_t)sizeof hdr)
            return 0;
    }
    if (hdr.load_addr != codecbuf
#ifdef HAVE_XIP_CODECS
        || (hdr.magic != CODEC_MAGIC && hdr.magic != CODEC_XIP_MAGIC)
#else
        || hdr.magic != CODEC_MAGIC
#endif
        || hdr.end_addr < hdr.load_addr)
        return 0;
    return hdr.end_addr - hdr.load_addr;
}
#endif /* TARGET_ARENA_PCM_SIZE */

int codec_run_proc(void)
{
    if (curr_handle == NULL) {
        logf("Codec: no codec to run");
        return CODEC_ERROR;
    }

    logf("Codec: entering run state");
    return c_hdr->run_proc();
}

int codec_close(void)
{
    int status = CODEC_OK;

    if (curr_handle != NULL) {
        logf("Codec: cleaning up");
        status = c_hdr->entry_point(CODEC_UNLOAD);
        lc_close(curr_handle);
        curr_handle = NULL;
        module_arena_release_codec();
    }

    return status;
}

#ifdef HAVE_RECORDING
enc_callback_t codec_get_enc_callback(void)
{
    if (curr_handle == NULL ||
        c_hdr->lc_hdr.magic != CODEC_ENC_MAGIC) {
        logf("Codec: not an encoder");
        return NULL;
    }

    return c_hdr->rec_extension[0];
}
#endif /* HAVE_RECORDING */
