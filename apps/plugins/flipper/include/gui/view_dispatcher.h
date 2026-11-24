/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef FLIPPER_VIEW_DISPATCHER_H
#define FLIPPER_VIEW_DISPATCHER_H
#include <gui/gui.h>
#include <gui/view.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Minimal ViewDispatcher (flipper_views.c): views are switched by id, input
 * goes to the current view; an unconsumed short Back runs its previous
 * callback (VIEW_NONE stops, VIEW_IGNORE does nothing). No scene manager or
 * custom events; navigation and tick callbacks are supported. */
#define VIEW_NONE 0xFFFFFFFF
#define VIEW_IGNORE 0xFFFFFFFE

typedef struct ViewDispatcher ViewDispatcher;
typedef enum { ViewDispatcherTypeDesktop, ViewDispatcherTypeWindow, ViewDispatcherTypeFullscreen } ViewDispatcherType;

ViewDispatcher* view_dispatcher_alloc(void);
void view_dispatcher_free(ViewDispatcher* vd);
void view_dispatcher_enable_queue(ViewDispatcher* vd);
void view_dispatcher_attach_to_gui(ViewDispatcher* vd, Gui* gui, ViewDispatcherType type);
void view_dispatcher_add_view(ViewDispatcher* vd, uint32_t id, View* view);
void view_dispatcher_remove_view(ViewDispatcher* vd, uint32_t id);
void view_dispatcher_switch_to_view(ViewDispatcher* vd, uint32_t id);
void view_dispatcher_run(ViewDispatcher* vd);
void view_dispatcher_stop(ViewDispatcher* vd);
void view_dispatcher_set_event_callback_context(ViewDispatcher* vd, void* context);
void view_dispatcher_set_navigation_event_callback(ViewDispatcher* vd, bool (*callback)(void* context));
void view_dispatcher_set_tick_event_callback(ViewDispatcher* vd, void (*callback)(void* context), uint32_t period_ms);

#ifdef __cplusplus
}
#endif
#endif
