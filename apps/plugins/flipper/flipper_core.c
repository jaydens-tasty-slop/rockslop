/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Flipper Zero compatibility layer: Furi kernel services on Rockbox.
 *
 * The app's main runs on the plugin thread. All blocking Furi calls (queue
 * get, delay, ...) service a pump that polls the keys, runs timer callbacks
 * and redraws the screen, so no callback ever races with the app's main.
 * Other threads created with furi_thread_* are real Rockbox threads. */
#include "plugin.h"
#include "lib/display_text.h"
#include "lib/playback_control.h"
#include "lib/plugin_fpu.h"
#include <setjmp.h>
#include <flipper_math.h>
#include <furi.h>
#include <furi_hal.h>
#include <notification/notification_messages.h>
#include "flipper_game.h"
#include "flipper_internal.h"


#define LONG_MS 300
#define REPEAT_MS 150
#define BACK_HOLD_MS 500   /* SELECT is also the power key: stay well below ~1 s */
#define QUIT_GRACE_MS 2000

/* ----------------------------------------------------------------------- */
/* Heap: first fit, address ordered free list with coalescing. */

struct hblock { uint32_t size; uint32_t pad; struct hblock *next; };
#define HDR 8u

static struct { uint8_t *base; uint32_t size; struct hblock *free; } heap;

static void heap_init(void)
{
    size_t sz;
    uint8_t *p = rb->plugin_get_buffer(&sz);
    uintptr_t a = ((uintptr_t)p + 7) & ~(uintptr_t)7;
    sz -= a - (uintptr_t)p;
    if (flipper_game.heap_limit && sz > flipper_game.heap_limit)
        sz = flipper_game.heap_limit;
    heap.base = (uint8_t *)a;
    heap.size = sz & ~(size_t)7;
    heap.free = (struct hblock *)a;
    heap.free->size = heap.size;
    heap.free->next = NULL;
}

void *malloc(size_t n)
{
    struct hblock **pp, *b;
    uint32_t need = ((uint32_t)n + HDR + 7u) & ~7u;
    if (need < 16) need = 16;
    for (pp = &heap.free; (b = *pp); pp = &b->next) {
        if (b->size < need) continue;
        if (b->size - need >= 16) {
            struct hblock *r = (struct hblock *)((uint8_t *)b + need);
            r->size = b->size - need;
            r->next = b->next;
            *pp = r;
            b->size = need;
        } else {
            *pp = b->next;
        }
        return (uint8_t *)b + HDR;
    }
    return NULL;
}

void free(void *ptr)
{
    struct hblock *b, **pp, *n;
    if (!ptr) return;
    b = (struct hblock *)((uint8_t *)ptr - HDR);
    for (pp = &heap.free; (n = *pp) && n < b; pp = &n->next)
        ;
    b->next = n;
    *pp = b;
    if (n && (uint8_t *)b + b->size == (uint8_t *)n) {  /* merge forward */
        b->size += n->size;
        b->next = n->next;
    }
    if (pp != &heap.free) {                             /* merge backward */
        struct hblock *p = (struct hblock *)((uint8_t *)pp - offsetof(struct hblock, next));
        if ((uint8_t *)p + p->size == (uint8_t *)b) {
            p->size += b->size;
            p->next = b->next;
        }
    }
}

void *calloc(size_t n, size_t m)
{
    void *p = malloc(n * m);
    if (p) memset(p, 0, n * m);
    return p;
}

void *realloc(void *ptr, size_t n)
{
    void *q;
    uint32_t old;
    if (!ptr) return malloc(n);
    old = ((struct hblock *)((uint8_t *)ptr - HDR))->size - HDR;
    if (n <= old) return ptr;
    q = malloc(n);
    if (q) {
        memcpy(q, ptr, old);
        free(ptr);
    }
    return q;
}

/* ----------------------------------------------------------------------- */
/* libc and libm pieces the plugin environment does not link in. */

int rand(void) { return rb->rand(); }
void srand(unsigned s) { rb->srand(s); }
long random(void) { return rb->rand(); }
size_t strlen(const char *s) { return rb->strlen(s); }
int strcmp(const char *a, const char *b) { return rb->strcmp(a, b); }
int strncmp(const char *a, const char *b, size_t n) { return rb->strncmp(a, b, n); }
char *strcpy(char *d, const char *s) { return rb->strcpy(d, s); }
char *strncpy(char *d, const char *s, size_t n) { return rb->strncpy(d, s, n); }
char *strchr(const char *s, int c) { return rb->strchr(s, c); }
char *strrchr(const char *s, int c) { return rb->strrchr(s, c); }
int atoi(const char *s) { return rb->atoi(s); }
long strtol(const char *s, char **e, int b) { return rb->strtol(s, e, b); }
unsigned long strtoul(const char *s, char **e, int b) { return (unsigned long)rb->strtol(s, e, b); }

double strtod(const char *s, char **end)
{
    double v = 0, scale = 1;
    int neg = 0, ex = 0, eneg = 0, any = 0;
    const char *p = s;
    if (*p == '-') { neg = 1; p++; } else if (*p == '+') p++;
    for (; *p >= '0' && *p <= '9'; p++, any = 1) v = v * 10 + (*p - '0');
    if (*p == '.') {
        for (p++; *p >= '0' && *p <= '9'; p++, any = 1) { v = v * 10 + (*p - '0'); scale *= 10; }
    }
    if (any && (*p == 'e' || *p == 'E')) {
        const char *q = p + 1;
        if (*q == '-') { eneg = 1; q++; } else if (*q == '+') q++;
        if (*q >= '0' && *q <= '9') {
            for (; *q >= '0' && *q <= '9'; q++) ex = ex * 10 + (*q - '0');
            p = q;
        }
    }
    v /= scale;
    while (ex-- > 0) v = eneg ? v / 10 : v * 10;
    if (end) *end = (char *)(any ? p : s);
    return neg ? -v : v;
}

int snprintf(char *b, size_t n, const char *f, ...)
{
    va_list ap;
    int r;
    va_start(ap, f);
    r = rb->vsnprintf(b, n, f, ap);
    va_end(ap);
    return r;
}
int vsnprintf(char *b, size_t n, const char *f, va_list ap)
{
    return rb->vsnprintf(b, n, f, ap);
}

/* Double precision sin/cos/sqrt: games here are not performance critical. */
double sqrt(double x)
{
    double r, p;
    int i;
    if (x <= 0) return 0;
    r = x > 1 ? x : 1;
    for (i = 0; i < 40; i++) {
        p = r;
        r = 0.5 * (r + x / r);
        if (r == p) break;
    }
    return r;
}
#ifdef PLUGIN_FPU
float sqrtf(float x)
{
    float r;
    if (x <= 0) return 0;
    __asm__("vsqrt.f32 %0, %1" : "=t"(r) : "t"(x));   /* the builtin would call sqrtf for errno */
    return r;
}
#else
float sqrtf(float x) { return (float)sqrt(x); }
#endif

double cos(double x);
double sin(double x)
{
    const double pi = 3.14159265358979323846;
    double x2, t, s;
    int n;
    x -= 2 * pi * (long)(x / (2 * pi));
    if (x > pi) x -= 2 * pi;
    else if (x < -pi) x += 2 * pi;
    if (x > pi / 2) x = pi - x;
    else if (x < -pi / 2) x = -pi - x;
    x2 = x * x;
    t = s = x;
    for (n = 1; n < 9; n++) {
        t *= -x2 / ((2 * n) * (2 * n + 1));
        s += t;
    }
    return s;
}
double cos(double x) { return sin(x + 1.57079632679489661923); }
float atanf(float x) { return (float)atan(x); }
float infinityf(void) { return 3.0e38f; }
float roundf(float x) { return x < 0 ? -(float)(long)(-x + 0.5f) : (float)(long)(x + 0.5f); }
float floorf(float x) { float t = (float)(long)x; return t > x ? t - 1.0f : t; }
float ceilf(float x) { float t = (float)(long)x; return t < x ? t + 1.0f : t; }
double fmax(double a, double b) { return a > b ? a : b; }
double fmin(double a, double b) { return a < b ? a : b; }
float fmaxf(float a, float b) { return a > b ? a : b; }
float fminf(float a, float b) { return a < b ? a : b; }
#ifdef PLUGIN_FPU
/* Single precision on the FPU (the double versions above stay soft). */
float sinf(float x)
{
    const float pi = 3.14159265f;
    float x2, t, s;
    int n;
    x -= 2 * pi * (float)(long)(x / (2 * pi));
    if (x > pi) x -= 2 * pi;
    else if (x < -pi) x += 2 * pi;
    if (x > pi / 2) x = pi - x;
    else if (x < -pi / 2) x = -pi - x;
    x2 = x * x;
    t = s = x;
    for (n = 1; n < 7; n++) {
        t *= -x2 / (float)((2 * n) * (2 * n + 1));
        s += t;
    }
    return s;
}
float cosf(float x) { return sinf(x + 1.57079633f); }
#else
float sinf(float x) { return (float)sin(x); }
float cosf(float x) { return (float)cos(x); }
#endif
double fabs(double x) { return x < 0 ? -x : x; }
float fabsf(float x) { return x < 0 ? -x : x; }
double floor(double x) { double i = (long)x; return i > x ? i - 1 : i; }
double ceil(double x) { double i = (long)x; return i < x ? i + 1 : i; }
double fmod(double a, double b) { return a - b * (long)(a / b); }
double atan2(double y, double x);
double atan(double x)
{
    /* argument reduction to [0, 1], then a short series */
    int inv = 0, neg = 0;
    double s, t, x2;
    int n;
    if (x < 0) { x = -x; neg = 1; }
    if (x > 1) { x = 1 / x; inv = 1; }
    if (x > 0.4142135623730951) {          /* atan(x) = pi/4 + atan((x-1)/(x+1)) */
        x = (x - 1) / (x + 1);
        t = s = x; x2 = x * x;
        for (n = 1; n < 12; n++) { t *= -x2; s += t / (2 * n + 1); }
        s += 0.78539816339744830962;
    } else {
        t = s = x; x2 = x * x;
        for (n = 1; n < 12; n++) { t *= -x2; s += t / (2 * n + 1); }
    }
    if (inv) s = 1.57079632679489661923 - s;
    return neg ? -s : s;
}
double atan2(double y, double x)
{
    const double pi = 3.14159265358979323846;
    if (x == 0) return y > 0 ? pi / 2 : y < 0 ? -pi / 2 : 0;
    if (x > 0) return atan(y / x);
    return y >= 0 ? atan(y / x) + pi : atan(y / x) - pi;
}

/* ----------------------------------------------------------------------- */
/* State. */

static struct {
    unsigned main_id;
    long pause_ticks;               /* time spent in menus, hidden from the app */
    bool in_pump;
    bool usb;
    bool quitting;                  /* menu Quit chosen or app told to stop */
    uint32_t quit_start;            /* furi tick the quit sequence began */
    uint8_t quit_stage;
    uint32_t quit_t;
    jmp_buf exit_jmp;
    bool app_running;
} S;

/* ----------------------------------------------------------------------- */
/* Time. */

uint32_t furi_get_tick(void)
{
    return (uint32_t)(*rb->current_tick - S.pause_ticks) * (1000 / HZ);
}
uint32_t furi_kernel_get_tick_frequency(void) { return 1000; }
uint32_t furi_ms_to_ticks(uint32_t ms) { return ms; }
uint32_t furi_ticks_to_ms(uint32_t t) { return t; }

bool flipper_is_main(void) { return rb->thread_self() == S.main_id; }

void flipper_checkpoint(void)
{
    if (!S.quitting) return;
    if (flipper_is_main()) {
        if (furi_get_tick() - S.quit_start > QUIT_GRACE_MS &&
            S.app_running)
            longjmp(S.exit_jmp, 1);
    } else {
        rb->thread_exit();
    }
}

void flipper_wait_slice(void)
{
    if (flipper_is_main()) {
        flipper_pump();
        rb->sleep(1);
    } else {
        flipper_checkpoint();
        rb->sleep(1);
    }
}

void furi_delay_ms(uint32_t ms)
{
    uint32_t end = furi_get_tick() + ms;
    flipper_checkpoint();
    if (flipper_is_main()) flipper_pump();
    while ((int32_t)(end - furi_get_tick()) > 0)
        flipper_wait_slice();
}
void furi_delay_tick(uint32_t t) { furi_delay_ms(t); }
void furi_delay_us(uint32_t us) { furi_delay_ms((us + 999) / 1000); }

void panicf(const char *fmt, ...)
{
    UNUSED(fmt);
    furi_crash("panic");
}

void furi_crash(const char *message)
{
    rb->splashf(HZ * 3, "Flipper layer: %s", message);
    for (;;) rb->sleep(HZ);
}

uint32_t furi_hal_random_get(void) { return ((uint32_t)rb->rand() << 16) ^ rb->rand(); }
void furi_hal_random_fill_buf(uint8_t *b, uint32_t n)
{
    while (n--) *b++ = rb->rand();
}

/* ----------------------------------------------------------------------- */
/* Message queue (by value). */

struct FuriMessageQueue {
    uint8_t *buf;
    uint32_t msg_size, cap, head, count;
    struct mutex lock;
};

FuriMessageQueue *furi_message_queue_alloc(uint32_t n, uint32_t size)
{
    FuriMessageQueue *q = malloc(sizeof(*q));
    if (!q) furi_crash("queue alloc");
    q->buf = malloc(n * size);
    if (!q->buf) furi_crash("queue alloc");
    q->msg_size = size; q->cap = n; q->head = q->count = 0;
    rb->mutex_init(&q->lock);
    return q;
}

void furi_message_queue_free(FuriMessageQueue *q)
{
    free(q->buf);
    free(q);
}

static FuriStatus msgq_wait(FuriMessageQueue *q, uint32_t timeout, bool want_msg)
{
    uint32_t end = furi_get_tick() + timeout;
    for (;;) {
        bool ok;
        rb->mutex_lock(&q->lock);
        ok = want_msg ? q->count > 0 : q->count < q->cap;
        rb->mutex_unlock(&q->lock);
        if (ok) return FuriStatusOk;
        if (timeout != FuriWaitForever && (int32_t)(end - furi_get_tick()) <= 0)
            return FuriStatusErrorTimeout;
        if (S.in_pump && flipper_is_main())
            return FuriStatusErrorTimeout;      /* input callback: drop, never wait */
        flipper_wait_slice();
    }
}

FuriStatus furi_message_queue_put(FuriMessageQueue *q, const void *msg, uint32_t timeout)
{
    if (msgq_wait(q, timeout, false) != FuriStatusOk) return FuriStatusErrorTimeout;
    rb->mutex_lock(&q->lock);
    if (q->count == q->cap) {
        rb->mutex_unlock(&q->lock);
        return FuriStatusErrorTimeout;
    }
    memcpy(q->buf + ((q->head + q->count) % q->cap) * q->msg_size, msg, q->msg_size);
    q->count++;
    rb->mutex_unlock(&q->lock);
    return FuriStatusOk;
}

FuriStatus furi_message_queue_get(FuriMessageQueue *q, void *msg, uint32_t timeout)
{
    for (;;) {
        FuriStatus st = msgq_wait(q, timeout, true);
        if (st != FuriStatusOk) return st;
        rb->mutex_lock(&q->lock);
        if (q->count) {
            memcpy(msg, q->buf + q->head * q->msg_size, q->msg_size);
            q->head = (q->head + 1) % q->cap;
            q->count--;
            rb->mutex_unlock(&q->lock);
            return FuriStatusOk;
        }
        rb->mutex_unlock(&q->lock);
    }
}

uint32_t furi_message_queue_get_count(FuriMessageQueue *q) { return q->count; }
uint32_t furi_message_queue_get_space(FuriMessageQueue *q) { return q->cap - q->count; }
FuriStatus furi_message_queue_reset(FuriMessageQueue *q)
{
    rb->mutex_lock(&q->lock);
    q->head = q->count = 0;
    rb->mutex_unlock(&q->lock);
    return FuriStatusOk;
}

/* ----------------------------------------------------------------------- */
/* Mutex: owner tracked here so that timeouts and try-lock work. */

struct FuriMutex {
    struct mutex guard;
    unsigned owner;
    uint16_t count;
    bool recursive;
};

FuriMutex *furi_mutex_alloc(FuriMutexType type)
{
    FuriMutex *m = malloc(sizeof(*m));
    if (!m) furi_crash("mutex alloc");
    rb->mutex_init(&m->guard);
    m->owner = 0; m->count = 0; m->recursive = type == FuriMutexTypeRecursive;
    return m;
}

void furi_mutex_free(FuriMutex *m) { free(m); }

FuriStatus furi_mutex_acquire(FuriMutex *m, uint32_t timeout)
{
    unsigned self = rb->thread_self();
    uint32_t end = furi_get_tick() + timeout;
    for (;;) {
        bool got = false;
        rb->mutex_lock(&m->guard);
        if (m->count == 0) {
            m->owner = self; m->count = 1; got = true;
        } else if (m->recursive && m->owner == self) {
            m->count++; got = true;
        }
        rb->mutex_unlock(&m->guard);
        if (got) return FuriStatusOk;
        if (timeout != FuriWaitForever && (int32_t)(end - furi_get_tick()) <= 0)
            return FuriStatusErrorResource;
        if (timeout == 0) return FuriStatusErrorResource;
        flipper_wait_slice();
    }
}

FuriStatus furi_mutex_release(FuriMutex *m)
{
    FuriStatus st = FuriStatusErrorResource;
    rb->mutex_lock(&m->guard);
    if (m->count && m->owner == rb->thread_self()) {
        m->count--;
        st = FuriStatusOk;
    }
    rb->mutex_unlock(&m->guard);
    return st;
}

/* ----------------------------------------------------------------------- */
/* Timers: serviced from the main thread's pump. */

struct FuriTimer {
    FuriTimerCallback cb;
    void *ctx;
    FuriTimerType type;
    bool running;
    uint32_t period, due;
    struct FuriTimer *next;
};
static FuriTimer *timers;

FuriTimer *furi_timer_alloc(FuriTimerCallback cb, FuriTimerType type, void *ctx)
{
    FuriTimer *t = calloc(1, sizeof(*t));
    if (!t) furi_crash("timer alloc");
    t->cb = cb; t->ctx = ctx; t->type = type;
    t->next = timers;
    timers = t;
    return t;
}

void furi_timer_free(FuriTimer *t)
{
    FuriTimer **pp;
    for (pp = &timers; *pp; pp = &(*pp)->next)
        if (*pp == t) { *pp = t->next; break; }
    free(t);
}

FuriStatus furi_timer_start(FuriTimer *t, uint32_t ticks)
{
    t->period = ticks ? ticks : 1;
    t->due = furi_get_tick() + t->period;
    t->running = true;
    return FuriStatusOk;
}
FuriStatus furi_timer_restart(FuriTimer *t, uint32_t ticks) { return furi_timer_start(t, ticks); }
FuriStatus furi_timer_stop(FuriTimer *t) { t->running = false; return FuriStatusOk; }
uint32_t furi_timer_is_running(FuriTimer *t) { return t->running; }
void furi_timer_set_thread_priority(int priority) { UNUSED(priority); }

static void run_timers(void)
{
    FuriTimer *t;
    uint32_t now = furi_get_tick();
    for (t = timers; t; t = t->next) {
        if (!t->running || (int32_t)(now - t->due) < 0) continue;
        if (t->type == FuriTimerTypeOnce) t->running = false;
        else {
            t->due += t->period;
            if ((int32_t)(now - t->due) >= 0) t->due = now + t->period;
        }
        t->cb(t->ctx);
        now = furi_get_tick();
        t = timers;     /* callbacks may free or restart timers */
        if (!t) break;
    }
}

/* ----------------------------------------------------------------------- */
/* Threads. */

struct FuriThread {
    char name[16];
    size_t stack;
    FuriThreadCallback cb;
    void *ctx;
    uint8_t *stack_mem;
    unsigned id;
    bool started;
    struct FuriThread *next;
};
static FuriThread *threads, *starting;

FuriThread *furi_thread_alloc(void)
{
    FuriThread *t = calloc(1, sizeof(*t));
    if (!t) furi_crash("thread alloc");
    t->stack = 2048;
    t->next = threads;
    threads = t;
    return t;
}

void furi_thread_set_name(FuriThread *t, const char *n) { rb->strlcpy(t->name, n, sizeof(t->name)); }
void furi_thread_set_stack_size(FuriThread *t, size_t s) { t->stack = s; }
void furi_thread_set_priority(FuriThread *t, FuriThreadPriority p) { UNUSED(t); UNUSED(p); }
void furi_thread_set_callback(FuriThread *t, FuriThreadCallback cb) { t->cb = cb; }
void furi_thread_set_context(FuriThread *t, void *c) { t->ctx = c; }

static void thread_trampoline(void)
{
    FuriThread *t = starting;
    starting = NULL;
    t->cb(t->ctx);
    rb->thread_exit();
}

void furi_thread_start(FuriThread *t)
{
    if (t->stack < 2048) t->stack = 2048;      /* Rockbox needs room for context switches */
    t->stack_mem = malloc(t->stack);
    if (!t->stack_mem) furi_crash("thread stack");
    starting = t;
    t->id = rb->create_thread(thread_trampoline, t->stack_mem, t->stack, 0, t->name
                              IF_PRIO(, PRIORITY_BACKGROUND) IF_COP(, CPU));
    t->started = true;
    rb->yield();
}

FuriStatus furi_thread_join(FuriThread *t)
{
    if (t->started) {
        rb->thread_wait(t->id);
        t->started = false;
    }
    return FuriStatusOk;
}

void furi_thread_free(FuriThread *t)
{
    FuriThread **pp;
    furi_thread_join(t);
    for (pp = &threads; *pp; pp = &(*pp)->next)
        if (*pp == t) { *pp = t->next; break; }
    free(t->stack_mem);
    free(t);
}

static void join_all_threads(void)
{
    FuriThread *t;
    for (t = threads; t; t = t->next) furi_thread_join(t);
}

/* ----------------------------------------------------------------------- */
/* PubSub. */

struct FuriPubSubSubscription {
    FuriPubSubCallback cb;
    void *ctx;
    struct FuriPubSubSubscription *next;
};
struct FuriPubSub {
    struct FuriPubSubSubscription *subs;
};

FuriPubSub *furi_pubsub_alloc(void) { return calloc(1, sizeof(FuriPubSub)); }
void furi_pubsub_free(FuriPubSub *p) { free(p); }

FuriPubSubSubscription *furi_pubsub_subscribe(FuriPubSub *p, FuriPubSubCallback cb, void *ctx)
{
    FuriPubSubSubscription *s = malloc(sizeof(*s));
    if (!s) furi_crash("pubsub");
    s->cb = cb; s->ctx = ctx; s->next = p->subs;
    p->subs = s;
    return s;
}

void furi_pubsub_unsubscribe(FuriPubSub *p, FuriPubSubSubscription *s)
{
    FuriPubSubSubscription **pp;
    for (pp = &p->subs; *pp; pp = &(*pp)->next)
        if (*pp == s) { *pp = s->next; break; }
    free(s);
}

void furi_pubsub_publish(FuriPubSub *p, void *msg)
{
    FuriPubSubSubscription *s;
    for (s = p->subs; s; s = s->next) s->cb(msg, s->ctx);
}

/* ----------------------------------------------------------------------- */
/* Records. */

static FuriPubSub *input_pubsub;
FuriPubSub *flipper_input_pubsub(void)
{
    if (!input_pubsub) input_pubsub = furi_pubsub_alloc();
    return input_pubsub;
}
extern void *flipper_gui_record(void);
extern void *flipper_storage_record(void);

struct record { const char *name; void *data; struct record *next; };
static struct record *records;

static char notification_record;    /* a non-NULL handle: notification_message ignores it */

void *furi_record_open(const char *name)
{
    struct record *r;
    if (!strcmp(name, RECORD_GUI)) return flipper_gui_record();
    if (!strcmp(name, RECORD_INPUT_EVENTS)) return flipper_input_pubsub();
    if (!strcmp(name, RECORD_STORAGE)) return flipper_storage_record();
    for (r = records; r; r = r->next)
        if (!strcmp(r->name, name)) return r->data;
    if (!strcmp(name, RECORD_NOTIFICATION)) return &notification_record;
    return NULL;        /* dialogs: absent, calls are no-ops */
}
void furi_record_close(const char *name) { UNUSED(name); }
void furi_record_create(const char *name, void *data)
{
    struct record *r = malloc(sizeof(*r));
    if (!r) return;
    r->name = name; r->data = data; r->next = records;
    records = r;
}
bool furi_record_destroy(const char *name)
{
    struct record **pp;
    for (pp = &records; *pp; pp = &(*pp)->next)
        if (!strcmp((*pp)->name, name)) { struct record *r = *pp; *pp = r->next; free(r); return true; }
    return false;
}

/* ----------------------------------------------------------------------- */

bool furi_hal_rtc_is_flag_set(FuriHalRtcFlag f) { UNUSED(f); return false; }

const char *input_get_key_name(InputKey k)
{
    static const char *const n[] = {"Up", "Down", "Right", "Left", "Ok", "Back"};
    return k < InputKeyMAX ? n[k] : "?";
}
const char *input_get_type_name(InputType t)
{
    static const char *const n[] = {"Press", "Release", "Short", "Long", "Repeat"};
    return t < InputTypeMAX ? n[t] : "?";
}

/* ----------------------------------------------------------------------- */
/* Input: derive Flipper press/release/short/long/repeat from key state. */

struct keystate {
    bool down, long_sent, ignore;
    uint32_t t_down, t_rep;
};
static struct keystate keys[InputKeyMAX];
static bool back_fired, menu_fired;
static uint32_t chord_since;

static const uint8_t phys[InputKeyBack] = {
    [InputKeyUp] = BUTTON_UP, [InputKeyDown] = BUTTON_DOWN,
    [InputKeyRight] = BUTTON_RIGHT, [InputKeyLeft] = BUTTON_LEFT,
    [InputKeyOk] = BUTTON_SELECT,
};

static void emit(InputKey key, InputType type)
{
    static uint32_t seq;
    InputEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.sequence = seq++;
    ev.key = key;
    ev.type = type;
    if (input_pubsub) furi_pubsub_publish(input_pubsub, &ev);
    flipper_gui_dispatch(&ev);
}

/* Taps generated for the SELECT split: Press now, Release (and Short) later,
 * so that a game polling once per frame still sees the key down. */
struct tap { bool active; InputKey key; uint32_t release_at; };
static struct tap taps[2];
#define TAP_MS 60

static void tap_key(InputKey key)
{
    unsigned i;
    for (i = 0; i < ARRAYLEN(taps); i++) {
        if (taps[i].active) continue;
        taps[i].active = true;
        taps[i].key = key;
        taps[i].release_at = furi_get_tick() + TAP_MS;
        emit(key, InputTypePress);
        return;
    }
}

static void tap_service(uint32_t now)
{
    unsigned i;
    for (i = 0; i < ARRAYLEN(taps); i++) {
        if (taps[i].active && (int32_t)(now - taps[i].release_at) >= 0) {
            taps[i].active = false;
            emit(taps[i].key, InputTypeRelease);
            emit(taps[i].key, InputTypeShort);
        }
    }
}

static struct { bool down, fired; uint32_t t; } sel;

static void sel_update(bool down, uint32_t now)
{
    if (down && !sel.down) {
        sel.down = true; sel.fired = false; sel.t = now;
    } else if (!down && sel.down) {
        sel.down = false;
        if (!sel.fired) tap_key(flipper_game.sel_tap_key - 1);
    } else if (down && !sel.fired && now - sel.t >= flipper_game.sel_long_ms) {
        sel.fired = true;
        tap_key(flipper_game.sel_long_key - 1);
    }
}

static void key_update(InputKey k, bool down, uint32_t now)
{
    struct keystate *s = &keys[k];
    bool muted = k == InputKeyOk && flipper_game.sel_tap_key;
    if (s->ignore) {
        if (!down) s->ignore = false;
        return;
    }
    if (down && !s->down) {
        s->down = true; s->long_sent = false; s->t_down = now;
        if (!muted) emit(k, InputTypePress);
    } else if (!down && s->down) {
        s->down = false;
        if (!muted) emit(k, InputTypeRelease);
        if (!muted && !s->long_sent) emit(k, InputTypeShort);
    } else if (down) {
        if (!s->long_sent) {
            if (now - s->t_down >= LONG_MS) {
                s->long_sent = true; s->t_rep = now;
                if (!muted) emit(k, InputTypeLong);
            }
        } else if (now - s->t_rep >= REPEAT_MS) {
            s->t_rep += REPEAT_MS;
            if (!muted) emit(k, InputTypeRepeat);
        }
    }
}

static int show_menu(bool resuming);
static void begin_quit(void);

static void pause_menu(void)
{
    long t0 = *rb->current_tick;
    int i, r;
    /* The app must not see keys stay down across the menu. */
    for (i = 0; i < InputKeyMAX; i++) {
        if (keys[i].down) {
            keys[i].down = false;
            if (!(i == InputKeyOk && flipper_game.sel_tap_key)) emit(i, InputTypeRelease);
        }
        keys[i].ignore = true;
    }
    memset(&sel, 0, sizeof(sel));
    for (i = 0; i < (int)ARRAYLEN(taps); i++) {
        if (taps[i].active) emit(taps[i].key, InputTypeRelease);
        taps[i].active = false;
    }
    while (rb->button_status()) rb->sleep(1);   /* held keys would scroll the menu */
    rb->button_clear_queue();
    flipper_sound_stop();
    r = show_menu(true);
    S.pause_ticks += *rb->current_tick - t0;
    flipper_gui_refresh_all();
    if (r != 0) begin_quit();
}

static void poll_input(void)
{
    uint32_t now = furi_get_tick();
    int st = rb->button_status();
    int raw = st;
    InputKey k;
    bool back_trig;

    /* Keys are read from button_status(); the queue is drained only so that
     * system events (USB) reach the default handler. SELECT is the power
     * key: a held SELECT in a game posts SYS_POWEROFF, so drop it here (the
     * menus still power off). */
    for (;;) {
        long ev = rb->button_get_w_tmo(0);
        if (ev == BUTTON_NONE) break;
        if (ev == SYS_POWEROFF)
            continue;
        if (IS_SYSEVENT(ev) && rb->default_event_handler(ev) == SYS_USB_CONNECTED) {
            S.usb = true;
            begin_quit();
        }
    }
    if (flipper_game.up_is_ok && (st & BUTTON_UP)) st = (st & ~BUTTON_UP) | BUTTON_SELECT;
#ifdef HAVE_BLUETOOTH_HID
    /* Pad A is OK, like SELECT. */
    for (k = 0; k < InputKeyBack; k++)
        key_update(k, (st & phys[k]) != 0 || (k == InputKeyOk && (st & BUTTON_PAD_A)), now);
    if (flipper_game.sel_tap_key && !keys[InputKeyOk].ignore)
        sel_update(keys[InputKeyOk].down, now);
#else
    for (k = 0; k < InputKeyBack; k++)
        key_update(k, (st & phys[k]) != 0, now);
    if (flipper_game.sel_tap_key && !keys[InputKeyOk].ignore) sel_update((st & BUTTON_SELECT) != 0, now);
#endif
    tap_service(now);

#ifdef HAVE_BLUETOOTH_HID
    /* Pad B is Back (also alongside the long-UP Back). */
    if (flipper_game.back_up_hold_ms) {
        struct keystate *u = &keys[InputKeyUp];
        key_update(InputKeyBack, (u->down && !keys[InputKeyOk].down &&
                   now - u->t_down >= flipper_game.back_up_hold_ms) ||
                   (st & BUTTON_PAD_B) != 0, now);
    } else {
        key_update(InputKeyBack, (st & BUTTON_PAD_B) != 0, now);
    }
#else
    if (flipper_game.back_up_hold_ms) {
        /* Back as a long UP: held from the threshold until UP is released.
         * Not with SELECT down: SELECT+UP is the pause menu chord (on a
         * title screen Back would quit the game first). */
        struct keystate *u = &keys[InputKeyUp];
        key_update(InputKeyBack, u->down && !(raw & BUTTON_SELECT) &&
                   now - u->t_down >= flipper_game.back_up_hold_ms, now);
    }
#endif
    /* Holding SELECT+UP opens the pause menu. */
    if ((raw & BUTTON_SELECT) && (raw & BUTTON_UP)) {
        if (!chord_since) chord_since = now ? now : 1;
        if (!menu_fired && !S.quitting &&
            now - chord_since >= (flipper_game.menu_hold_ms ? flipper_game.menu_hold_ms : BACK_HOLD_MS)) {
            menu_fired = true;
            pause_menu();
        }
    } else {
        chord_since = 0;
        menu_fired = false;
    }
#ifdef HAVE_BLUETOOTH_HID
    /* Pad Options opens the pause menu, PS quits. */
    {
        static bool pad_menu_held;
        bool m = (st & BUTTON_PAD_MENU) != 0;
        bool press = m && !pad_menu_held;
        pad_menu_held = m;
        if (press && !S.quitting) pause_menu();
        else if ((st & BUTTON_PAD_HOME) && !S.quitting) begin_quit();
    }
#endif
    /* A lone SELECT hold sends Back to games that ask for it. */
    back_trig = flipper_game.back == FLIPPER_BACK_PASS_OK && !(raw & BUTTON_UP) &&
                keys[InputKeyOk].down && now - keys[InputKeyOk].t_down >= BACK_HOLD_MS;
    if (!back_trig) {
        back_fired = false;
    } else if (!back_fired && !S.quitting) {
        back_fired = true;
        sel.fired = true;   /* the hold was Back, not a SELECT tap */
        /* One complete tap: a held virtual key would repeat and exit. */
        emit(InputKeyBack, InputTypePress);
        emit(InputKeyBack, InputTypeRelease);
        emit(InputKeyBack, InputTypeShort);
    }
}

static void quit_sequence(void)
{
    uint32_t now = furi_get_tick();
    switch (S.quit_stage) {
    case 0:
        emit(InputKeyBack, InputTypePress);
        S.quit_t = now;
        S.quit_stage = 1;
        break;
    case 1:
        if (flipper_game.quit == FLIPPER_QUIT_SHORT) {
            emit(InputKeyBack, InputTypeRelease);
            emit(InputKeyBack, InputTypeShort);
            S.quit_stage = 3;
            S.quit_t = now;
        } else if (now - S.quit_t >= LONG_MS) {
            emit(InputKeyBack, InputTypeLong);
            S.quit_t = now;
            S.quit_stage = 2;
        }
        break;
    case 2:
        if (now - S.quit_t >= 50) {
            emit(InputKeyBack, InputTypeRelease);
            S.quit_stage = 3;
            S.quit_t = now;
        }
        break;
    case 3:
        /* Some apps treat Back as "go to the title screen": ask again. */
        if (now - S.quit_t >= 600) S.quit_stage = 0;
        break;
    }
}

static void begin_quit(void)
{
    S.quitting = true;
    S.quit_stage = 0;
    S.quit_start = furi_get_tick();
}

void flipper_pump(void)
{
    if (S.in_pump) return;
    S.in_pump = true;
    if (S.quitting) {
        quit_sequence();
        flipper_checkpoint();
    } else {
        poll_input();
    }
    run_timers();
    flipper_gui_redraw();
    S.in_pump = false;
}

/* ----------------------------------------------------------------------- */
/* Start / pause menu, help and credits. */

static void show_text(const char *const *lines)
{
    static char *words[220];
    static char buf[1200];
    static struct style_text style[] = {{0, TEXT_CENTER | TEXT_UNDERLINE}, LAST_STYLE_ITEM};
    unsigned n = 0, used = 0;
    const char *const *l;

    for (l = lines; *l; l++) {
        const char *p = *l;
        while (*p) {
            unsigned len = 0;
            while (*p == ' ') p++;
            while (p[len] && p[len] != ' ') len++;
            if (!len) break;
            if (n >= ARRAYLEN(words) - 2 || used + len + 1 > sizeof(buf)) goto done;
            memcpy(buf + used, p, len);
            buf[used + len] = 0;
            words[n++] = buf + used;
            used += len + 1;
            p += len;
        }
        words[n++] = "";
    }
done:
    rb->lcd_setfont(FONT_UI);
    display_text(n, words, style, NULL, true);
    rb->lcd_setfont(FONT_SYSFIXED);
}

static const char *const layer_credits[] = {
    "Flipper Zero source compatibility layer for the YP3 Rockbox port.",
    "Runs Flipper games unmodified-ish as Rockbox plugins.",
    NULL,
};

static void show_about(void)
{
    show_text(flipper_game.credits);
    show_text(layer_credits);
}

/* Returns 0 to play/resume, 1 to quit, 2 to quit because USB was plugged. */
static int show_menu(bool resuming)
{
    static char title[32];
    MENUITEM_STRINGLIST(start_menu, title, NULL,
                        "Start Game", "Help", "About", "Playback Control", "Quit");
    MENUITEM_STRINGLIST(pause_menu_items, title, NULL,
                        "Resume Game", "Help", "About", "Playback Control", "Quit");
    int sel = 0;
    rb->strlcpy(title, flipper_game.name, sizeof(title));
    for (;;) {
        switch (rb->do_menu(resuming ? &pause_menu_items : &start_menu, &sel, NULL, false)) {
        case 0: return 0;
        case 1: show_text(flipper_game.help); break;
        case 2: show_about(); break;
        case 3: playback_control(NULL); break;
        case 4: return 1;
        case MENU_ATTACHED_USB: S.usb = true; return 2;
        default: if (resuming) return 0; return 1;
        }
    }
}

/* ----------------------------------------------------------------------- */

const char *flipper_data_path(const char *name)
{
    static char path[MAX_PATH];
    rb->snprintf(path, sizeof(path), "%s/%s", flipper_game.data_dir, name);
    return path;
}

enum plugin_status plugin_start(const void *param)
{
    UNUSED(param);
    memset(&S, 0, sizeof(S));
    memset(keys, 0, sizeof(keys));
    back_fired = menu_fired = false;
    chord_since = 0;
    S.main_id = rb->thread_self();
    heap_init();
    plugin_fpu_enable(true);      /* games run on this thread only (no furi threads) */
    flipper_gui_reset();

    if (show_menu(false) == 0) {
        /* Keys that opened the game must not leak into it. */
        InputKey k;
        for (k = 0; k < InputKeyMAX; k++) keys[k].ignore = true;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        if (flipper_game.boost) rb->cpu_boost(true);
#endif
        if (!setjmp(S.exit_jmp)) {
            S.app_running = true;
            flipper_game.entry(NULL);
        }
        S.app_running = false;
        S.quitting = true;
        S.quit_stage = 3;
        join_all_threads();
        flipper_sound_close();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        if (flipper_game.boost) rb->cpu_boost(false);
#endif
    }
    rb->lcd_set_background(rb->global_settings->bg_color);
    rb->lcd_setfont(FONT_UI);
    plugin_fpu_enable(false);
    return S.usb ? PLUGIN_USB_CONNECTED : PLUGIN_OK;
}
