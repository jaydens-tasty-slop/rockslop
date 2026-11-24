/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Flipper Zero source-compatibility layer for Rockbox: Furi core.
 * Only the symbols that ported games use are provided. */
#ifndef FLIPPER_FURI_H
#define FLIPPER_FURI_H

#ifdef __cplusplus
extern "C" {    /* the Rockbox libc headers have no C++ guards */
#endif
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdarg.h>
#ifndef FLIPPER_NO_LIBM
#include <flipper_math.h>
#endif

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif
#define COUNT_OF(x) (sizeof(x) / sizeof((x)[0]))
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef CLAMP
#define CLAMP(x, upper, lower) (MIN(upper, MAX(x, lower)))
#endif

/* libc pieces provided by the layer (flipper_core.c, flipper_string.c). */
size_t strlcpy(char* dst, const char* src, size_t size);
size_t strlcat(char* dst, const char* src, size_t size);
long random(void);
double strtod(const char* s, char** end);
unsigned long strtoul(const char* s, char** end, int base);

#define FURI_WARN_UNUSED __attribute__((warn_unused_result))
#define FURI_PACKED __attribute__((packed))

void furi_crash(const char* message) __attribute__((noreturn));
#define furi_check(c) ((c) ? (void)0 : furi_crash(#c))
#define furi_assert(c) ((c) ? (void)0 : furi_crash(#c))
#define furi_assert_msg(c, m) ((c) ? (void)0 : furi_crash(m))

/* Logging is compiled out. */
#define FURI_LOG_E(tag, ...) ((void)(tag))
#define FURI_LOG_W(tag, ...) ((void)(tag))
#define FURI_LOG_I(tag, ...) ((void)(tag))
#define FURI_LOG_D(tag, ...) ((void)(tag))
#define FURI_LOG_T(tag, ...) ((void)(tag))

typedef enum {
    FuriStatusOk = 0,
    FuriStatusError = -1,
    FuriStatusErrorTimeout = -2,
    FuriStatusErrorResource = -3,
    FuriStatusErrorParameter = -4,
    FuriStatusErrorISR = -5,
} FuriStatus;

#define FuriWaitForever 0xFFFFFFFFU
#define FuriFlagWaitAny 0x00000000U

/* Ticks are milliseconds, as on the Flipper. */
uint32_t furi_get_tick(void);
uint32_t furi_kernel_get_tick_frequency(void);
uint32_t furi_ms_to_ticks(uint32_t ms);
uint32_t furi_ticks_to_ms(uint32_t ticks);
void furi_delay_ms(uint32_t ms);
void furi_delay_us(uint32_t us);
void furi_delay_tick(uint32_t ticks);

/* Message queue: fixed-size messages copied by value. */
typedef struct FuriMessageQueue FuriMessageQueue;
FuriMessageQueue* furi_message_queue_alloc(uint32_t msg_count, uint32_t msg_size);
void furi_message_queue_free(FuriMessageQueue* q);
FuriStatus furi_message_queue_put(FuriMessageQueue* q, const void* msg, uint32_t timeout);
FuriStatus furi_message_queue_get(FuriMessageQueue* q, void* msg, uint32_t timeout);
uint32_t furi_message_queue_get_count(FuriMessageQueue* q);
uint32_t furi_message_queue_get_space(FuriMessageQueue* q);
FuriStatus furi_message_queue_reset(FuriMessageQueue* q);

/* Mutex. */
typedef enum { FuriMutexTypeNormal, FuriMutexTypeRecursive } FuriMutexType;
typedef struct FuriMutex FuriMutex;
FuriMutex* furi_mutex_alloc(FuriMutexType type);
void furi_mutex_free(FuriMutex* m);
FuriStatus furi_mutex_acquire(FuriMutex* m, uint32_t timeout);
FuriStatus furi_mutex_release(FuriMutex* m);

/* Timer: callbacks run from the main thread's event pump. */
typedef enum { FuriTimerTypeOnce = 0, FuriTimerTypePeriodic = 1 } FuriTimerType;
typedef void (*FuriTimerCallback)(void* context);
typedef struct FuriTimer FuriTimer;
FuriTimer* furi_timer_alloc(FuriTimerCallback cb, FuriTimerType type, void* context);
void furi_timer_free(FuriTimer* t);
FuriStatus furi_timer_start(FuriTimer* t, uint32_t ticks);
FuriStatus furi_timer_restart(FuriTimer* t, uint32_t ticks);
FuriStatus furi_timer_stop(FuriTimer* t);
uint32_t furi_timer_is_running(FuriTimer* t);
void furi_timer_set_thread_priority(int priority);
#define FuriTimerThreadPriorityNormal 0
#define FuriTimerThreadPriorityElevated 1

/* Thread (a real Rockbox thread). */
typedef struct FuriThread FuriThread;
typedef int32_t (*FuriThreadCallback)(void* context);
typedef enum { FuriThreadPriorityIdle, FuriThreadPriorityLowest, FuriThreadPriorityLow,
               FuriThreadPriorityNormal, FuriThreadPriorityHigh } FuriThreadPriority;
FuriThread* furi_thread_alloc(void);
void furi_thread_free(FuriThread* t);
void furi_thread_set_name(FuriThread* t, const char* name);
void furi_thread_set_stack_size(FuriThread* t, size_t size);
void furi_thread_set_priority(FuriThread* t, FuriThreadPriority p);
void furi_thread_set_callback(FuriThread* t, FuriThreadCallback cb);
void furi_thread_set_context(FuriThread* t, void* context);
void furi_thread_start(FuriThread* t);
FuriStatus furi_thread_join(FuriThread* t);

/* PubSub. */
typedef struct FuriPubSub FuriPubSub;
typedef struct FuriPubSubSubscription FuriPubSubSubscription;
typedef void (*FuriPubSubCallback)(const void* message, void* context);
FuriPubSub* furi_pubsub_alloc(void);
void furi_pubsub_free(FuriPubSub* p);
FuriPubSubSubscription* furi_pubsub_subscribe(FuriPubSub* p, FuriPubSubCallback cb, void* context);
void furi_pubsub_unsubscribe(FuriPubSub* p, FuriPubSubSubscription* s);
void furi_pubsub_publish(FuriPubSub* p, void* message);

/* Records. */
#define RECORD_GUI "gui"
#define RECORD_INPUT_EVENTS "input_events"
#define RECORD_NOTIFICATION "notification"
#define RECORD_STORAGE "storage"
#define RECORD_DIALOGS "dialogs"
void* furi_record_open(const char* name);
void furi_record_close(const char* name);
void furi_record_create(const char* name, void* data);
bool furi_record_destroy(const char* name);

/* FuriString: a growable C string. */
typedef struct FuriString FuriString;
FuriString* furi_string_alloc(void);
FuriString* furi_string_alloc_set_str(const char* s);
void furi_string_free(FuriString* s);
const char* furi_string_get_cstr(const FuriString* s);
char furi_string_get_char(const FuriString* s, size_t index);
size_t furi_string_size(const FuriString* s);
void furi_string_reset(FuriString* s);
void furi_string_set_str(FuriString* s, const char* str);
void furi_string_cat_str(FuriString* s, const char* str);
void furi_string_right(FuriString* s, size_t start);
int furi_string_printf(FuriString* s, const char* format, ...);
int furi_string_vprintf(FuriString* s, const char* format, va_list args);
int furi_string_cat_printf(FuriString* s, const char* format, ...);
FuriString* furi_string_alloc_printf(const char* format, ...);

/* Misc HAL. */
uint32_t furi_hal_random_get(void);
void furi_hal_random_fill_buf(uint8_t* buf, uint32_t len);

#ifdef __cplusplus
}
#endif
#endif
