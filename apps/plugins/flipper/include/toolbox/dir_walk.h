/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef FLIPPER_DIR_WALK_H
#define FLIPPER_DIR_WALK_H
#include <furi.h>
#include <storage/storage.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct DirWalk DirWalk;
typedef enum { DirWalkOK, DirWalkError, DirWalkLast } DirWalkResult;
DirWalk* dir_walk_alloc(Storage* storage);
void dir_walk_free(DirWalk* walk);
void dir_walk_set_recursive(DirWalk* walk, bool recursive);
bool dir_walk_open(DirWalk* walk, const char* path);
DirWalkResult dir_walk_read(DirWalk* walk, FuriString* path, FileInfo* info);
#ifdef __cplusplus
}
#endif
#endif
