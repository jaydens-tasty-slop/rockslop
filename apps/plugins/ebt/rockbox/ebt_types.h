/* Song data structures of EBT Tracker, from ebt_vars.h of the upstream
 * ESPboy_EBT source (WTFPL). Only what the synth, player and loader need. */
#ifndef EBT_TYPES_H
#define EBT_TYPES_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#ifndef __cplusplus
#include <stdbool.h>
#endif

#define BOOL uint8_t
#define TRUE 1
#define FALSE 0

#define MAX_PATTERNS			(160+1)
#define MAX_PATTERN_LEN			32
#define MAX_ORDER_LEN			128
#define MAX_CHANNELS			4
#define MAX_INSTRUMENTS			(128+1)
#define MAX_EFFECTS_PER_ROW		2
#define MAX_INSTRUMENT_NAME_LEN	4
#define MAX_SONG_INFO_LEN		10

#define DEFAULT_SONG_SPEED		6
#define DEFAULT_PATTERN_LEN		16
#define DEFAULT_PATTERN_LOOP	0
#define DEFAULT_VOLUME			4
#define DEFAULT_BASE_NOTE		3 * 12

#define FRAME_RATE				60	/* player frames per second */

#ifdef __cplusplus
struct effect_struct {
	uint8_t type;	//0 for no effect, 1 for effect that is displayed as 0 (arpeggio) and so on
	uint8_t param;
};

struct pattern_row_struct {
	uint8_t note;
	uint8_t ins;
	effect_struct effect[MAX_EFFECTS_PER_ROW];
};

struct pattern_struct {
	pattern_row_struct rows[MAX_PATTERN_LEN];
	uint8_t len;
	uint8_t loop;
};

struct order_pos_struct {
	uint8_t ptn[MAX_CHANNELS];
	signed char trans[MAX_CHANNELS];
};

struct order_struct {
	order_pos_struct pos[MAX_ORDER_LEN];
	int16_t loop_start;
	int16_t loop_end;
};

struct instrument_struct {
	uint8_t wave;				//0..63
	uint8_t volume;				//1..4
	int8_t offset;				//-127..127
	int8_t detune;				//-100..100
	int8_t slide;				//-127..127
	uint8_t mod_delay;			//0..255
	uint8_t mod_speed;			//0..255
	uint8_t mod_depth;			//0..255
	uint8_t cut_time;			//0..255
	uint8_t fixed_pitch;		//0..1
	uint8_t base_note;			//0..96
	int8_t aux_id;				//relative reference
	uint8_t aux_mix;
	char name[MAX_INSTRUMENT_NAME_LEN];
};

struct song_struct {
	pattern_struct ptns[MAX_PATTERNS];
	instrument_struct ins[MAX_INSTRUMENTS];	//instrument 0 holds the default
	order_struct order;
	uint8_t speed_even;
	uint8_t speed_odd;
	uint8_t speed_interleave;
	int8_t pan_default[MAX_CHANNELS];	//-4..0..4
	char name[MAX_SONG_INFO_LEN];
	char author[MAX_SONG_INFO_LEN];
};

#endif

#define MUTE_CH1	0x01
#define MUTE_CH2	0x02
#define MUTE_CH3	0x04
#define MUTE_CH4	0x08

#ifdef __cplusplus
extern "C" {
#endif

/* Public interface of ebt_core.cpp (the synth, player and song loader). */

struct ebt_status {
	int order;                      /* order list position in play, -1 stopped */
	int row;
	char name[MAX_SONG_INFO_LEN + 1];
	char author[MAX_SONG_INFO_LEN + 1];
	uint8_t note[MAX_CHANNELS];     /* pattern note at the current row */
	uint8_t level[MAX_CHANNELS];    /* 0..4: what the synth plays now */
};

/* Provided by the front end: next line of the open song file, or NULL. */
const char *ebt_file_get_line(void);

/* Parse a song through ebt_file_get_line(); false if it is not a song. */
bool ebt_core_load(void);
void ebt_core_start(int sample_rate, unsigned mute);
void ebt_core_stop(void);
void ebt_core_set_mute(unsigned mute);
void ebt_core_status(struct ebt_status *st);
/* Pattern note of a channel at order position/row, 0xff beyond the pattern. */
int ebt_core_note(int order, int ch, int row);
/* Interleaved stereo, `frames` pairs. */
void ebt_core_render(int16_t *out, int frames);

#ifdef __cplusplus
}
#endif

#endif
