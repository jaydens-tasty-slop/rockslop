/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Flipper Zero compatibility layer: storage on the Rockbox file system.
 *
 * "/data/..." and "/assets/..." (APP_DATA_PATH, APP_ASSETS_PATH) live in the
 * game's card directory, "/ext/..." is the card root. */
#include "plugin.h"
#include <furi.h>
#include <storage/storage.h>
#include <toolbox/dir_walk.h>
#include <toolbox/path.h>
#include <flipper_format/flipper_format.h>
#include "flipper_game.h"

void *flipper_storage_record(void)
{
    static int storage;
    return &storage;
}

static const char *xpath(const char *p, char *out, size_t n)
{
    if (!strncmp(p, "/data/", 6) || !strncmp(p, "/assets/", 8))
        rb->snprintf(out, n, "%s%s", flipper_game.data_dir, p);
    else if (!strncmp(p, "/ext/", 5))
        rb->strlcpy(out, p + 4, n);
    else
        rb->strlcpy(out, p, n);
    return out;
}

static void make_parents(const char *path)
{
    char tmp[MAX_PATH];
    char *p;
    rb->strlcpy(tmp, path, sizeof(tmp));
    for (p = tmp + 1; *p; p++) {
        if (*p != '/') continue;
        *p = 0;
        if (!rb->dir_exists(tmp)) rb->mkdir(tmp);
        *p = '/';
    }
}

/* ----------------------------------------------------------------------- */

struct File {
    int fd;
    FS_Error err;
};

File *storage_file_alloc(Storage *s)
{
    File *f = calloc(1, sizeof(*f));
    UNUSED(s);
    if (f) f->fd = -1;
    return f;
}

bool storage_file_close(File *f)
{
    if (f->fd >= 0) rb->close(f->fd);
    f->fd = -1;
    return true;
}

void storage_file_free(File *f)
{
    if (!f) return;
    storage_file_close(f);
    free(f);
}

bool storage_file_open(File *f, const char *path, FS_AccessMode access, FS_OpenMode mode)
{
    char real[MAX_PATH];
    int flags;
    xpath(path, real, sizeof(real));
    storage_file_close(f);
    if (access == FSAM_READ) flags = O_RDONLY;
    else flags = (access == FSAM_WRITE ? O_WRONLY : O_RDWR);
    switch (mode) {
    case FSOM_OPEN_ALWAYS: flags |= O_CREAT; break;
    case FSOM_OPEN_APPEND: flags |= O_CREAT | O_APPEND; break;
    case FSOM_CREATE_NEW: flags |= O_CREAT | O_EXCL; break;
    case FSOM_CREATE_ALWAYS: flags |= O_CREAT | O_TRUNC; break;
    default: break;
    }
    if (flags & O_CREAT) make_parents(real);
    f->fd = rb->open(real, flags, 0666);
    f->err = f->fd >= 0 ? FSE_OK : FSE_NOT_EXIST;
    return f->fd >= 0;
}

size_t storage_file_read(File *f, void *buf, size_t n)
{
    ssize_t r = f->fd >= 0 ? rb->read(f->fd, buf, n) : -1;
    if (r < 0) { f->err = FSE_INTERNAL; return 0; }
    return r;
}

size_t storage_file_write(File *f, const void *buf, size_t n)
{
    ssize_t r = f->fd >= 0 ? rb->write(f->fd, buf, n) : -1;
    if (r < 0) { f->err = FSE_INTERNAL; return 0; }
    return r;
}

bool storage_file_seek(File *f, uint32_t off, bool from_start)
{
    return rb->lseek(f->fd, off, from_start ? SEEK_SET : SEEK_CUR) >= 0;
}
uint64_t storage_file_tell(File *f) { return rb->lseek(f->fd, 0, SEEK_CUR); }
uint64_t storage_file_size(File *f) { return rb->ffilesize(f->fd); }
bool storage_file_eof(File *f) { return storage_file_tell(f) >= storage_file_size(f); }
FS_Error storage_file_get_error(File *f) { return f->err; }
bool storage_file_truncate(File *f) { return rb->ftruncate(f->fd, rb->lseek(f->fd, 0, SEEK_CUR)) == 0; }
bool storage_file_sync(File *f) { UNUSED(f); return true; }

void storage_common_resolve_path_and_ensure_app_directory(Storage *s, FuriString *path)
{
    /* Paths are already per game; just make sure the directory exists. */
    UNUSED(path);
    storage_common_mkdir(s, STORAGE_APP_DATA_PATH_PREFIX);
}

FS_Error storage_common_stat(Storage *s, const char *path, FileInfo *info)
{
    char real[MAX_PATH];
    int fd;
    UNUSED(s);
    xpath(path, real, sizeof(real));
    if (rb->dir_exists(real)) {
        if (info) { info->flags = FSF_DIRECTORY; info->size = 0; }
        return FSE_OK;
    }
    fd = rb->open(real, O_RDONLY);
    if (fd < 0) return FSE_NOT_EXIST;
    if (info) { info->flags = 0; info->size = rb->ffilesize(fd); }
    rb->close(fd);
    return FSE_OK;
}

FS_Error storage_common_mkdir(Storage *s, const char *path)
{
    char real[MAX_PATH];
    UNUSED(s);
    xpath(path, real, sizeof(real));
    make_parents(real);
    if (rb->dir_exists(real)) return FSE_EXIST;
    return rb->mkdir(real) == 0 ? FSE_OK : FSE_INTERNAL;
}

bool storage_file_exists(Storage *s, const char *path)
{
    return storage_common_stat(s, path, NULL) == FSE_OK;
}

bool storage_dir_exists(Storage *s, const char *path)
{
    char real[MAX_PATH];
    UNUSED(s);
    xpath(path, real, sizeof(real));
    return rb->dir_exists(real);
}

bool storage_simply_mkdir(Storage *s, const char *path)
{
    FS_Error e = storage_common_mkdir(s, path);
    return e == FSE_OK || e == FSE_EXIST;
}

/* Nothing to migrate from the Flipper's old locations. */
FS_Error storage_common_migrate(Storage *s, const char *old_path, const char *new_path)
{
    UNUSED(s); UNUSED(old_path); UNUSED(new_path);
    return FSE_OK;
}

bool storage_simply_remove(Storage *s, const char *path)
{
    char real[MAX_PATH];
    UNUSED(s);
    xpath(path, real, sizeof(real));
    return rb->remove(real) == 0;
}

/* ----------------------------------------------------------------------- */

struct DirWalk {
    DIR *dir;
    char base[MAX_PATH];
};

DirWalk *dir_walk_alloc(Storage *s) { UNUSED(s); return calloc(1, sizeof(DirWalk)); }

void dir_walk_free(DirWalk *w)
{
    if (w->dir) rb->closedir(w->dir);
    free(w);
}

void dir_walk_set_recursive(DirWalk *w, bool r) { UNUSED(w); UNUSED(r); /* flat only */ }

bool dir_walk_open(DirWalk *w, const char *path)
{
    char real[MAX_PATH];
    rb->strlcpy(w->base, path, sizeof(w->base));
    w->dir = rb->opendir(xpath(path, real, sizeof(real)));
    return w->dir != NULL;
}

DirWalkResult dir_walk_read(DirWalk *w, FuriString *out, FileInfo *info)
{
    struct dirent *e;
    char full[MAX_PATH];
    if (!w->dir) return DirWalkError;
    while ((e = rb->readdir(w->dir))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        rb->snprintf(full, sizeof(full), "%s/%s", w->base, e->d_name);
        furi_string_set_str(out, full);
        if (info) storage_common_stat(NULL, full, info);
        return DirWalkOK;
    }
    return DirWalkLast;
}

void path_extract_filename_no_ext(const char *path, FuriString *name)
{
    const char *base = strrchr(path, '/');
    const char *dot;
    char tmp[MAX_PATH];
    base = base ? base + 1 : path;
    rb->strlcpy(tmp, base, sizeof(tmp));
    dot = strrchr(tmp, '.');
    if (dot) tmp[dot - tmp] = 0;
    furi_string_set_str(name, tmp);
}

void path_extract_extension(FuriString *path, char *ext, size_t n)
{
    const char *p = furi_string_get_cstr(path);
    const char *dot = strrchr(p, '.');
    const char *slash = strrchr(p, '/');
    if (!dot || (slash && dot < slash)) { ext[0] = 0; return; }
    rb->strlcpy(ext, dot, n);
}

/* ----------------------------------------------------------------------- */
/* Flipper Format, reduced to a "Key: value" text file. */

struct FlipperFormat {
    File *file;
    char *text;             /* whole file when reading */
    bool writing;
};

FlipperFormat *flipper_format_file_alloc(Storage *s)
{
    FlipperFormat *f = calloc(1, sizeof(*f));
    if (f) f->file = storage_file_alloc(s);
    return f;
}

bool flipper_format_file_close(FlipperFormat *f)
{
    free(f->text);
    f->text = NULL;
    return storage_file_close(f->file);
}

void flipper_format_free(FlipperFormat *f)
{
    if (!f) return;
    flipper_format_file_close(f);
    storage_file_free(f->file);
    free(f);
}

bool flipper_format_file_open_existing(FlipperFormat *f, const char *path)
{
    uint64_t size;
    if (!storage_file_open(f->file, path, FSAM_READ, FSOM_OPEN_EXISTING)) return false;
    size = storage_file_size(f->file);
    if (size > 4096) return false;
    f->text = malloc(size + 1);
    if (!f->text) return false;
    f->text[storage_file_read(f->file, f->text, size)] = 0;
    return true;
}

bool flipper_format_file_open_always(FlipperFormat *f, const char *path)
{
    f->writing = true;
    return storage_file_open(f->file, path, FSAM_WRITE, FSOM_CREATE_ALWAYS);
}

/* Value text following "key: " at the start of a line, or NULL. */
static const char *find_key(const FlipperFormat *f, const char *key)
{
    size_t n = strlen(key);
    const char *p = f->text;
    while (p && *p) {
        if (!strncmp(p, key, n) && p[n] == ':') {
            p += n + 1;
            while (*p == ' ') p++;
            return p;
        }
        p = strchr(p, '\n');
        if (p) p++;
    }
    return NULL;
}

bool flipper_format_read_header(FlipperFormat *f, FuriString *type, uint32_t *version)
{
    const char *t = find_key(f, "Filetype"), *v = find_key(f, "Version");
    const char *e;
    char tmp[64];
    size_t n;
    if (!t || !v) return false;
    e = strchr(t, '\n');
    n = e ? (size_t)(e - t) : strlen(t);
    if (n >= sizeof(tmp)) n = sizeof(tmp) - 1;
    memcpy(tmp, t, n);
    tmp[n] = 0;
    furi_string_set_str(type, tmp);
    *version = rb->atoi(v);
    return true;
}

bool flipper_format_read_uint32(FlipperFormat *f, const char *key, uint32_t *data, uint16_t count)
{
    const char *v = find_key(f, key);
    uint16_t i;
    char *end;
    if (!v) return false;
    for (i = 0; i < count; i++) {
        data[i] = rb->strtol(v, &end, 10);
        v = end;
    }
    return true;
}


bool flipper_format_write_header_cstr(FlipperFormat *f, const char *type, uint32_t version)
{
    char line[96];
    int n = rb->snprintf(line, sizeof(line), "Filetype: %s\nVersion: %lu\n", type,
                         (unsigned long)version);
    return storage_file_write(f->file, line, n) == (size_t)n;
}

bool flipper_format_write_uint32(FlipperFormat *f, const char *key, const uint32_t *data,
                                 uint16_t count)
{
    uint16_t i;
    char line[96];
    int n = rb->snprintf(line, sizeof(line), "%s:", key);
    if (storage_file_write(f->file, line, n) != (size_t)n) return false;
    for (i = 0; i < count; i++) {
        n = rb->snprintf(line, sizeof(line), " %lu", (unsigned long)data[i]);
        if (storage_file_write(f->file, line, n) != (size_t)n) return false;
    }
    return storage_file_write(f->file, "\n", 1) == 1;
}
