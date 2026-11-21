/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef FLIPPER_GUI_H
#define FLIPPER_GUI_H
#include <furi.h>
#include <input/input.h>
#include <gui/canvas.h>
#include <gui/view_port.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GuiLayerDesktop,
    GuiLayerWindow,
    GuiLayerStatusBarLeft,
    GuiLayerStatusBarRight,
    GuiLayerFullscreen,
    GuiLayerMAX,
} GuiLayer;

typedef struct Gui Gui;
typedef void (*GuiCanvasCommitCallback)(
    uint8_t* data, size_t size, CanvasOrientation orientation, void* context);

void gui_add_view_port(Gui* gui, ViewPort* view_port, GuiLayer layer);
void gui_remove_view_port(Gui* gui, ViewPort* view_port);
void gui_add_framebuffer_callback(Gui* gui, GuiCanvasCommitCallback cb, void* context);
void gui_remove_framebuffer_callback(Gui* gui, GuiCanvasCommitCallback cb, void* context);
Canvas* gui_direct_draw_acquire(Gui* gui);
void gui_direct_draw_release(Gui* gui);

#ifdef __cplusplus
}
#endif
#endif
