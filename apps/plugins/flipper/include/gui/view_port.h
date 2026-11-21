/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef FLIPPER_VIEW_PORT_H
#define FLIPPER_VIEW_PORT_H
#include <furi.h>
#include <input/input.h>
#include <gui/canvas.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ViewPortOrientationHorizontal,
    ViewPortOrientationHorizontalFlip,
    ViewPortOrientationVertical,
    ViewPortOrientationVerticalFlip,
} ViewPortOrientation;

typedef struct ViewPort ViewPort;
typedef void (*ViewPortDrawCallback)(Canvas* canvas, void* context);
typedef void (*ViewPortInputCallback)(InputEvent* event, void* context);

ViewPort* view_port_alloc(void);
void view_port_free(ViewPort* vp);
void view_port_enabled_set(ViewPort* vp, bool enabled);
void view_port_draw_callback_set(ViewPort* vp, ViewPortDrawCallback cb, void* context);
void view_port_input_callback_set(ViewPort* vp, ViewPortInputCallback cb, void* context);
void view_port_update(ViewPort* vp);
void view_port_set_orientation(ViewPort* vp, ViewPortOrientation orientation);

#ifdef __cplusplus
}
#endif
#endif
