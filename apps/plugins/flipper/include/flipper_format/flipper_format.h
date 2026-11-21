/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef FLIPPER_FORMAT_H
#define FLIPPER_FORMAT_H
#include <furi.h>
#include <storage/storage.h>
#ifdef __cplusplus
extern "C" {
#endif

/* The Flipper "key: value" text format, header and uint32 values only. */
typedef struct FlipperFormat FlipperFormat;
FlipperFormat* flipper_format_file_alloc(Storage* storage);
void flipper_format_free(FlipperFormat* ff);
bool flipper_format_file_open_existing(FlipperFormat* ff, const char* path);
bool flipper_format_file_open_always(FlipperFormat* ff, const char* path);
bool flipper_format_file_close(FlipperFormat* ff);
bool flipper_format_read_header(FlipperFormat* ff, FuriString* filetype, uint32_t* version);
bool flipper_format_read_uint32(FlipperFormat* ff, const char* key, uint32_t* data, uint16_t count);
bool flipper_format_write_header_cstr(FlipperFormat* ff, const char* filetype, uint32_t version);
bool flipper_format_write_uint32(FlipperFormat* ff, const char* key, const uint32_t* data, uint16_t count);

#ifdef __cplusplus
}
#endif
#endif
