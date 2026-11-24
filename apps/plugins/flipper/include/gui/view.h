/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef FLIPPER_VIEW_H
#define FLIPPER_VIEW_H
#include <furi.h>
#include <input/input.h>
#include <gui/canvas.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Minimal View: draw and input callbacks, a model and a "previous view"
 * callback (flipper_views.c). Models are not locked: input, timers and
 * drawing all run on the main thread. */
typedef struct View View;
typedef void (*ViewDrawCallback)(Canvas* canvas, void* model);
typedef bool (*ViewInputCallback)(InputEvent* event, void* context);
typedef uint32_t (*ViewNavigationCallback)(void* context);
typedef enum { ViewModelTypeLockFree, ViewModelTypeLocking } ViewModelType;

View* view_alloc(void);
void view_free(View* view);
void view_set_context(View* view, void* context);
void view_set_draw_callback(View* view, ViewDrawCallback callback);
void view_set_input_callback(View* view, ViewInputCallback callback);
void view_set_previous_callback(View* view, ViewNavigationCallback callback);
void view_allocate_model(View* view, ViewModelType type, size_t size);
void view_free_model(View* view);
void* view_get_model(View* view);
void view_commit_model(View* view, bool update);

#define with_view_model(view, type, code, update) \
    {                                             \
        type = view_get_model(view);              \
        code;                                     \
        view_commit_model(view, update);          \
    }

#ifdef __cplusplus
}
#endif
#endif
