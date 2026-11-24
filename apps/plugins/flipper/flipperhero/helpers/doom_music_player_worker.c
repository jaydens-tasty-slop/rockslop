#include "doom_music_player_worker.h"

#include <furi_hal.h>
#include <furi.h>

#include <stdlib.h>

#define TAG "MusicPlayerWorker"

#define MUSIC_PLAYER_FILETYPE "Flipper Music Format"
#define MUSIC_PLAYER_VERSION  0

#define SEMITONE_PAUSE 0xFF

#define NOTE_C4             261.63f
#define NOTE_C4_SEMITONE    (4.0f * 12.0f)

typedef struct {
    uint8_t semitone;
    uint8_t duration;
    uint8_t dots;
} NoteBlock;


struct MusicPlayerWorker {
    // Rockbox: the notes are played from a timer (no worker thread).
    FuriTimer* timer;
    bool should_work;
    bool update_defaults;
    size_t next_note;
    uint32_t note_end;
    float cur_volume;

    MusicPlayerWorkerCallback callback;
    void* callback_context;

    float volume;
    uint32_t bpm;
    uint32_t duration;
    uint32_t octave;
    NoteBlock* notes;
    size_t note_count, note_cap;
};

#define TICK_MS 10

// 2^(n/12) without powf: whole octaves by doubling, the rest from a table.
static float semitones_to_ratio(int n) {
    static const float ratio[12] = {1.0f,      1.059463f, 1.122462f, 1.189207f, 1.259921f, 1.334840f,
                                    1.414214f, 1.498307f, 1.587401f, 1.681793f, 1.781797f, 1.887749f};
    float r = 1.0f;
    while(n >= 12) {
        r *= 2;
        n -= 12;
    }
    while(n < 0) {
        r /= 2;
        n += 12;
    }
    return r * ratio[n];
}

// Called every TICK_MS while playing: a note fades out until its time is up,
// then the next one starts (the tune loops).
static void music_player_worker_tick(void* context) {
    MusicPlayerWorker* instance = context;
    if(!instance->should_work || instance->note_count == 0) return;

    if(instance->note_end && (int32_t)(furi_get_tick() - instance->note_end) < 0) {
        instance->cur_volume *= 0.973f;
        furi_hal_speaker_set_volume(instance->cur_volume);
        return;
    }

    if(instance->next_note >= instance->note_count) instance->next_note = 0;
    NoteBlock* note_block = &instance->notes[instance->next_note++];

    float note_from_a4 = (float)note_block->semitone - NOTE_C4_SEMITONE;
    float frequency = NOTE_C4 * semitones_to_ratio((int)note_from_a4);
    float duration = 60.0f * furi_kernel_get_tick_frequency() * 4 / instance->bpm /
                     note_block->duration;
    uint32_t dots = note_block->dots;
    while(dots > 0) {
        duration += duration / 2;
        dots--;
    }
    instance->note_end = furi_get_tick() + (uint32_t)duration;
    instance->cur_volume = instance->volume;

    if(instance->callback) {
        instance->callback(
            note_block->semitone,
            note_block->dots,
            note_block->duration,
            0.0,
            instance->callback_context);
    }

    furi_hal_speaker_stop();
    furi_hal_speaker_start(frequency, instance->cur_volume);
}

MusicPlayerWorker* music_player_worker_alloc() {
    MusicPlayerWorker* instance = malloc(sizeof(MusicPlayerWorker));
    memset(instance, 0, sizeof(*instance));
    instance->timer = furi_timer_alloc(music_player_worker_tick, FuriTimerTypePeriodic, instance);
    instance->volume = 1.0f;
    return instance;
}

void music_player_worker_clear_notes(MusicPlayerWorker* instance) {
    furi_assert(instance);
    instance->update_defaults = true;
    instance->note_count = 0;
}

void music_player_worker_free(MusicPlayerWorker* instance) {
    furi_assert(instance);
    furi_timer_free(instance->timer);
    free(instance->notes);
    free(instance);
}

static bool is_digit(const char c) {
    return c >= '0' && c <= '9';
}

static bool is_letter(const char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool is_space(const char c) {
    return c == ' ' || c == '\t';
}

static size_t extract_number(const char* string, uint32_t* number) {
    size_t ret = 0;
    while(is_digit(*string)) {
        *number *= 10;
        *number += (*string - '0');
        string++;
        ret++;
    }
    return ret;
}

static size_t extract_dots(const char* string, uint32_t* number) {
    size_t ret = 0;
    while(*string == '.') {
        *number += 1;
        string++;
        ret++;
    }
    return ret;
}

static size_t extract_char(const char* string, char* symbol) {
    if(is_letter(*string)) {
        *symbol = *string;
        return 1;
    } else {
        return 0;
    }
}

static size_t extract_sharp(const char* string, char* symbol) {
    if(*string == '#' || *string == '_') {
        *symbol = '#';
        return 1;
    } else {
        return 0;
    }
}

static size_t skip_till(const char* string, const char symbol) {
    size_t ret = 0;
    while(*string != '\0' && *string != symbol) {
        string++;
        ret++;
    }
    if(*string != symbol) {
        ret = 0;
    }
    return ret;
}

static bool music_player_worker_add_note(
    MusicPlayerWorker* instance,
    uint8_t semitone,
    uint8_t duration,
    uint8_t dots) {
    NoteBlock note_block;

    note_block.semitone = semitone;
    note_block.duration = duration;
    note_block.dots = dots;

    if(instance->note_count == instance->note_cap) {
        instance->note_cap = instance->note_cap ? instance->note_cap * 2 : 64;
        instance->notes = realloc(instance->notes, instance->note_cap * sizeof(NoteBlock));
        furi_check(instance->notes);
    }
    instance->notes[instance->note_count++] = note_block;

    return true;
}

static int8_t note_to_semitone(const char note) {
    switch(note) {
    case 'C':
        return 0;
    // C#
    case 'D':
        return 2;
    // D#
    case 'E':
        return 4;
    case 'F':
        return 5;
    // F#
    case 'G':
        return 7;
    // G#
    case 'A':
        return 9;
    // A#
    case 'B':
        return 11;
    default:
        return 0;
    }
}

static bool music_player_worker_parse_notes(MusicPlayerWorker* instance, const char* string) {
    const char* cursor = string;
    bool result = true;

    while(*cursor != '\0') {
        if(!is_space(*cursor)) {
            uint32_t duration = 0;
            char note_char = '\0';
            char sharp_char = '\0';
            uint32_t octave = 0;
            uint32_t dots = 0;

            // Parsing
            cursor += extract_number(cursor, &duration);
            cursor += extract_char(cursor, &note_char);
            cursor += extract_sharp(cursor, &sharp_char);
            cursor += extract_number(cursor, &octave);
            cursor += extract_dots(cursor, &dots);

            // Post processing
            note_char = ((note_char) >= 'a' && (note_char) <= 'z' ? (note_char) - 32 : (note_char));
            if(!duration) {
                duration = instance->duration;
            }
            if(!octave) {
                octave = instance->octave;
            }

            // Validation
            bool is_valid = true;
            is_valid &= (duration >= 1 && duration <= 128);
            is_valid &= ((note_char >= 'A' && note_char <= 'G') || note_char == 'P');
            is_valid &= (sharp_char == '#' || sharp_char == '\0');
            is_valid &= (octave <= 16);
            is_valid &= (dots <= 16);
            if(!is_valid) {
                FURI_LOG_E(
                    TAG,
                    "Invalid note: %lu%c%c%lu.%lu",
                    duration,
                    note_char == '\0' ? '_' : note_char,
                    sharp_char == '\0' ? '_' : sharp_char,
                    octave,
                    dots);
                result = false;
                break;
            }

            // Note to semitones
            uint8_t semitone = 0;
            if(note_char == 'P') {
                semitone = SEMITONE_PAUSE;
            } else {
                semitone += octave * 12;
                semitone += note_to_semitone(note_char);
                semitone += sharp_char == '#' ? 1 : 0;
            }

            if(music_player_worker_add_note(instance, semitone, duration, dots)) {
                FURI_LOG_D(
                    TAG,
                    "Added note: %c%c%lu.%lu = %u %lu",
                    note_char == '\0' ? '_' : note_char,
                    sharp_char == '\0' ? '_' : sharp_char,
                    octave,
                    dots,
                    semitone,
                    duration);
            } else {
                FURI_LOG_E(
                    TAG,
                    "Invalid note: %c%c%lu.%lu = %u %lu",
                    note_char == '\0' ? '_' : note_char,
                    sharp_char == '\0' ? '_' : sharp_char,
                    octave,
                    dots,
                    semitone,
                    duration);
            }
            cursor += skip_till(cursor, ',');
        }

        if(*cursor != '\0') cursor++;
    }

    return result;
}

bool music_player_worker_load_rtttl_from_string(MusicPlayerWorker* instance, const char* string) {
    furi_assert(instance);

    if(instance->update_defaults) {
        instance->duration = 0;
        instance->octave = 0;
        instance->bpm = 0;
        instance->update_defaults = false;
    }
    const char* cursor = string;

    // Skip name
    cursor += skip_till(cursor, ':');
    if(*cursor != ':') {
        return false;
    }

    // Duration
    cursor += skip_till(cursor, '=');
    if(*cursor != '=') {
        return false;
    }
    cursor++;
    cursor += extract_number(cursor, &instance->duration);

    // Octave
    cursor += skip_till(cursor, '=');
    if(*cursor != '=') {
        return false;
    }
    cursor++;
    cursor += extract_number(cursor, &instance->octave);

    // BPM
    cursor += skip_till(cursor, '=');
    if(*cursor != '=') {
        return false;
    }
    cursor++;
    cursor += extract_number(cursor, &instance->bpm);

    // Notes
    cursor += skip_till(cursor, ':');
    if(*cursor != ':') {
        return false;
    }
    cursor++;
    if(!music_player_worker_parse_notes(instance, cursor)) {
        return false;
    }

    return true;
}

void music_player_worker_set_callback(
    MusicPlayerWorker* instance,
    MusicPlayerWorkerCallback callback,
    void* context) {
    furi_assert(instance);
    instance->callback = callback;
    instance->callback_context = context;
}

void music_player_worker_set_volume(MusicPlayerWorker* instance, float volume) {
    furi_assert(instance);
    instance->volume = volume;
}

void music_player_worker_start(MusicPlayerWorker* instance) {
    furi_assert(instance);
    furi_assert(instance->should_work == false);

    if(!furi_hal_speaker_acquire(1000)) return;
    instance->should_work = true;
    instance->next_note = 0;
    instance->note_end = 0;
    furi_timer_start(instance->timer, furi_ms_to_ticks(TICK_MS));
}

void music_player_worker_stop(MusicPlayerWorker* instance) {
    furi_assert(instance);

    if(!instance->should_work) return;
    instance->should_work = false;
    furi_timer_stop(instance->timer);
    furi_hal_speaker_stop();
    furi_hal_speaker_release();
}
