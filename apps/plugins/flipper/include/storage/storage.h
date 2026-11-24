/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef FLIPPER_STORAGE_H
#define FLIPPER_STORAGE_H
#include <furi.h>

/* Paths: "/ext/..." is the card root. APP_DATA_PATH and APP_ASSETS_PATH are
 * per game directories on the card (see flipper_game.data_dir). */
#define EXT_PATH(path) "/ext/" path
#define APP_DATA_PATH(path) "/data/" path
#define APP_ASSETS_PATH(path) "/assets/" path
#define STORAGE_APP_DATA_PATH_PREFIX "/data"
#define STORAGE_APP_ASSETS_PATH_PREFIX "/assets"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Storage Storage;
typedef struct File File;

typedef enum {
    FSE_OK,
    FSE_NOT_READY,
    FSE_EXIST,
    FSE_NOT_EXIST,
    FSE_INVALID_PARAMETER,
    FSE_DENIED,
    FSE_INVALID_NAME,
    FSE_INTERNAL,
    FSE_NOT_IMPLEMENTED,
    FSE_ALREADY_OPEN,
} FS_Error;

typedef enum { FSAM_READ = 1, FSAM_WRITE = 2, FSAM_READ_WRITE = 3 } FS_AccessMode;
typedef enum {
    FSOM_OPEN_EXISTING,
    FSOM_OPEN_ALWAYS,
    FSOM_OPEN_APPEND,
    FSOM_CREATE_NEW,
    FSOM_CREATE_ALWAYS,
} FS_OpenMode;

#define FSF_DIRECTORY 1
typedef struct {
    uint8_t flags;
    uint64_t size;
} FileInfo;

static inline bool file_info_is_dir(const FileInfo* info) {
    return (info->flags & FSF_DIRECTORY) != 0;
}

File* storage_file_alloc(Storage* storage);
void storage_file_free(File* file);
bool storage_file_open(File* file, const char* path, FS_AccessMode access, FS_OpenMode mode);
bool storage_file_close(File* file);
size_t storage_file_read(File* file, void* buf, size_t size);
size_t storage_file_write(File* file, const void* buf, size_t size);
bool storage_file_seek(File* file, uint32_t offset, bool from_start);
uint64_t storage_file_tell(File* file);
uint64_t storage_file_size(File* file);
bool storage_file_eof(File* file);
FS_Error storage_file_get_error(File* file);
bool storage_file_truncate(File* file);
bool storage_file_sync(File* file);
void storage_common_resolve_path_and_ensure_app_directory(Storage* storage, FuriString* path);

FS_Error storage_common_stat(Storage* storage, const char* path, FileInfo* info);
FS_Error storage_common_mkdir(Storage* storage, const char* path);
bool storage_file_exists(Storage* storage, const char* path);
bool storage_dir_exists(Storage* storage, const char* path);
bool storage_simply_mkdir(Storage* storage, const char* path);
bool storage_simply_remove(Storage* storage, const char* path);
FS_Error storage_common_migrate(Storage* storage, const char* old_path, const char* new_path);

#ifdef __cplusplus
}
#endif
#endif
