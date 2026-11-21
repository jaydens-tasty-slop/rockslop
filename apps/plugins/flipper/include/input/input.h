/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef FLIPPER_INPUT_H
#define FLIPPER_INPUT_H
#include <furi.h>

typedef enum {
    InputKeyUp,
    InputKeyDown,
    InputKeyRight,
    InputKeyLeft,
    InputKeyOk,
    InputKeyBack,
    InputKeyMAX,
} InputKey;

typedef enum {
    InputTypePress,
    InputTypeRelease,
    InputTypeShort,
    InputTypeLong,
    InputTypeRepeat,
    InputTypeMAX,
} InputType;

typedef struct {
    union {
        uint32_t sequence;
        struct {
            uint8_t sequence_source : 2;
            uint32_t sequence_counter : 30;
        };
    };
    InputKey key;
    InputType type;
} InputEvent;

const char* input_get_key_name(InputKey key);
const char* input_get_type_name(InputType type);
#endif
