/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef FLIPPER_PATH_H
#define FLIPPER_PATH_H
#include <furi.h>
#ifdef __cplusplus
extern "C" {
#endif
void path_extract_filename_no_ext(const char* path, FuriString* name);
void path_extract_extension(FuriString* path, char* ext, size_t ext_len);
#ifdef __cplusplus
}
#endif
#endif
