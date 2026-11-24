/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Flipper Zero compatibility layer: View and ViewDispatcher (minimal).
 * Built on the ViewPort; only the games that use views list this file. */
#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>

#define MAX_VIEWS 8

struct View {
    ViewDrawCallback draw;
    ViewInputCallback input;
    ViewNavigationCallback previous;
    void *context, *model;
    ViewDispatcher *dispatcher;
};

struct ViewDispatcher {
    struct { uint32_t id; View *view; } views[MAX_VIEWS];
    View *current;
    ViewPort *port;
    Gui *gui;
    bool stop;
    void *event_ctx;
    bool (*nav)(void *ctx);
    void (*tick)(void *ctx);
    uint32_t tick_ms;
};

View *view_alloc(void) { return calloc(1, sizeof(View)); }
void view_free(View *v) { view_free_model(v); free(v); }
void view_set_context(View *v, void *c) { v->context = c; }
void view_set_draw_callback(View *v, ViewDrawCallback cb) { v->draw = cb; }
void view_set_input_callback(View *v, ViewInputCallback cb) { v->input = cb; }
void view_set_previous_callback(View *v, ViewNavigationCallback cb) { v->previous = cb; }

void view_allocate_model(View *v, ViewModelType type, size_t size)
{
    UNUSED(type);
    v->model = calloc(1, size);
}
void view_free_model(View *v) { free(v->model); v->model = NULL; }
void *view_get_model(View *v) { return v->model; }

void view_commit_model(View *v, bool update)
{
    if (update && v->dispatcher && v->dispatcher->port)
        view_port_update(v->dispatcher->port);
}

static void vd_draw(Canvas *canvas, void *ctx)
{
    ViewDispatcher *d = ctx;
    if (d->current && d->current->draw)
        d->current->draw(canvas, d->current->model);
}

static void vd_input(InputEvent *ev, void *ctx)
{
    ViewDispatcher *d = ctx;
    View *v = d->current;
    bool consumed = false;

    if (!v) return;
    if (ev->key == InputKeyBack && ev->type == InputTypeLong) {
        d->stop = true;     /* the layer's Quit holds Back */
        return;
    }
    if (v->input) consumed = v->input(ev, v->context);
    if (consumed || ev->key != InputKeyBack) return;
    if (ev->type == InputTypeShort) {
        uint32_t id;
        if (d->nav) {
            if (!d->nav(d->event_ctx)) d->stop = true;
            return;
        }
        id = v->previous ? v->previous(v->context) : VIEW_NONE;
        if (id == VIEW_NONE) d->stop = true;
        else if (id != VIEW_IGNORE) view_dispatcher_switch_to_view(d, id);
    }
}

ViewDispatcher *view_dispatcher_alloc(void) { return calloc(1, sizeof(ViewDispatcher)); }
void view_dispatcher_free(ViewDispatcher *d) { free(d); }
void view_dispatcher_enable_queue(ViewDispatcher *d) { UNUSED(d); }

void view_dispatcher_attach_to_gui(ViewDispatcher *d, Gui *gui, ViewDispatcherType type)
{
    UNUSED(type);
    d->gui = gui;
    d->port = view_port_alloc();
    view_port_draw_callback_set(d->port, vd_draw, d);
    view_port_input_callback_set(d->port, vd_input, d);
    gui_add_view_port(gui, d->port, GuiLayerFullscreen);
}

void view_dispatcher_add_view(ViewDispatcher *d, uint32_t id, View *v)
{
    for (int i = 0; i < MAX_VIEWS; i++) {
        if (!d->views[i].view) {
            d->views[i].id = id;
            d->views[i].view = v;
            v->dispatcher = d;
            return;
        }
    }
    furi_crash("too many views");
}

void view_dispatcher_remove_view(ViewDispatcher *d, uint32_t id)
{
    for (int i = 0; i < MAX_VIEWS; i++) {
        if (d->views[i].view && d->views[i].id == id) {
            if (d->current == d->views[i].view) d->current = NULL;
            d->views[i].view->dispatcher = NULL;
            d->views[i].view = NULL;
        }
    }
}

void view_dispatcher_switch_to_view(ViewDispatcher *d, uint32_t id)
{
    for (int i = 0; i < MAX_VIEWS; i++) {
        if (d->views[i].view && d->views[i].id == id) {
            d->current = d->views[i].view;
            view_port_update(d->port);
        }
    }
}

void view_dispatcher_run(ViewDispatcher *d)
{
    uint32_t last = furi_get_tick();
    while (!d->stop) {
        furi_delay_ms(10);
        if (d->tick && furi_get_tick() - last >= d->tick_ms) {
            last = furi_get_tick();
            d->tick(d->event_ctx);
        }
    }
    view_port_enabled_set(d->port, false);
    gui_remove_view_port(d->gui, d->port);
    view_port_free(d->port);
    d->port = NULL;
}

void view_dispatcher_stop(ViewDispatcher *d) { d->stop = true; }

void view_dispatcher_set_event_callback_context(ViewDispatcher *d, void *ctx) { d->event_ctx = ctx; }
void view_dispatcher_set_navigation_event_callback(ViewDispatcher *d, bool (*cb)(void *ctx)) { d->nav = cb; }
void view_dispatcher_set_tick_event_callback(ViewDispatcher *d, void (*cb)(void *ctx), uint32_t period_ms)
{
    d->tick = cb;
    d->tick_ms = period_ms;
}
